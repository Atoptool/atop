/*
** The atopacctd daemon switches on the process accounting feature
** in the kernel and passes all process accounting records to a
** shadow file whenever they have been written by the kernel.
** Client processes (like atop) can read the shadow files instead of
** the original process accounting file.
** This has the following advantages:
** - The original process accounting file is kept to a limited size
**   by truncating it regularly.
** - When no client processes are active, no shadow files will be written.
** - A shadow file has a limited size. The name of a shadow file contains
**   a sequence number. When the current shadow file reaches its maximum
**   size, writing continues in the next shadow file.
**   Shadow files that are not used by client processes any more, are
**   automatically removed.
** ----------------------------------------------------------------------
** Copyright (C) 2014    Gerlof Langeveld (gerlof.langeveld@atoptool.nl)
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 2 as
** published by the Free Software Foundation.
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/statvfs.h>

#include "atop.h"
#include "photoproc.h"
#include "acctproc.h"
#include "version.h"
#include "atopacctd.h"

#define	EVER	;;

/*
** Semaphore-handling
**
** Two semaphore groups are created with one semaphores each.
**
** The private semaphore (group) specifies the number of atopacctd processes
** running (to be sure that only one daemon is active at the time).
**
** The public semaphore (group) reflects the number of processes using
** the process accounting shadow files, i.e. the number of clients. This
** semaphore starts at a high value and should be decremented whenever a
** client starts using the shadow files and incremented again whenever that
** client terminates.
*/
static int		semprv;
static int		sempub;
#define SEMTOTAL        100
#define	NUMCLIENTS	(SEMTOTAL - semctl(sempub, 0, GETVAL, 0))

static char		atopacctdversion[] = ATOPVERS;
static char		atopacctddate[]    = ATOPDATE;

static unsigned long	maxshadowrec = MAXSHADOWREC;
static char		*pacctdir    = PACCTDIR;

/*
** function prototypes
*/
static int		createshadow(long);
static int		pass2shadow(int, char *, int);
static void		gcshadows(long *, long);
static void		setcurrent(long);
static int		acctsize(struct acct *);

int			netlink_open(void);	// from netlink.c


int
main(int argc, char *argv[])
{
	int			i, nfd, afd, sfd, asz, nsz, ssz;
	int			partsz, remsz;
	int 			shadowbusy = 0;
	struct stat		dirstat;

	struct sembuf		semincr = {0, +1, SEM_UNDO};

	char			abuf[8192], nbuf[8192];
	char			shadowdir[128], accountpath[128];
	int			arecsize = 0;
	unsigned long long	atotsize = 0, stotsize = 0, maxshadowsz;
	long			oldshadow = 0, curshadow = 0;
	time_t			gclast = time(0);  // last garbage collection

	/*
	** argument passed?
	*/
	if (argc == 2)
	{
		/*
		** version number required (flag -v or -V)?
		*/
		if (*argv[1] == '-')	// flag?
		{
			if ( *(argv[1]+1) == 'v' || *(argv[1]+1) == 'V')
			{
				printf("%s  <gerlof.langeveld@atoptool.nl>\n",
								getstrvers());
				return 0;
			}
			else
			{
				fprintf(stderr,
				     	"Usage: atopacctd [-v|topdirectory]\n"
					"Default topdirectory: %s\n", PACCTDIR);
				exit(1);
			}
		}

		/*
		** if first argument is not a flags, it should be the name
		** of an alternative top directory (to be validated later on)
		*/
		pacctdir = argv[1];
	}
	else
	{
		if (argc != 1)
		{
			fprintf(stderr,
			     	"Usage: atopacctd [-v|topdirectory]\n"
				"Default topdirectory: %s\n", PACCTDIR);
			exit(1);
		}
	}

	/*
 	** verify if we are running with the right privileges
	*/
	if (geteuid() != 0)
	{
		fprintf(stderr, "Root privileges are needed!\n");
		exit(1);
	}

	/*
	** verify that the top directory is not word-writable
	** and owned by root
	*/
	if ( stat(pacctdir, &dirstat) == -1 )
	{
		perror(pacctdir);
		fprintf(stderr, "Usage: atopacctd [-v|topdirectory]\n"
				"Default topdirectory: %s\n", PACCTDIR);
		exit(2);
	}

	if (! S_ISDIR(dirstat.st_mode) )
	{
		fprintf(stderr, "atopacctd: %s is not a directory\n", pacctdir);
		exit(2);
	}

	if (dirstat.st_uid != 0)
	{
		fprintf(stderr,
			"atopacctd: directory %s must be owned by root\n",
			pacctdir);
		exit(2);
	}

	if (dirstat.st_mode & (S_IWGRP|S_IWOTH))
	{
		fprintf(stderr,
			"atopacctd: directory %s must not be writable "
			"for group/others\n", pacctdir);
		exit(2);
	}

	/*
	** create the semaphore groups and initialize the semaphores;
	** if the private group already exists, verify if another
	** atopacctd daemon is already running
	*/
	if ( (semprv = semget(PACCTPUBKEY-1, 0, 0)) >= 0)	// exists?
	{
		if ( semctl(semprv, 0, GETVAL, 0) > 0)
		{
			fprintf(stderr, "atopacctd is already running!\n");
			exit(3);
		}
	}
	else
	{
		if ( (semprv = semget(PACCTPUBKEY-1, 1,
					0600|IPC_CREAT|IPC_EXCL)) >= 0)
		{
			(void) semctl(semprv, 0, SETVAL, 0);
		}
		else
		{
			perror("cannot create private semaphore");
			exit(3);
		}
	}

	if ( (sempub = semget(PACCTPUBKEY, 0, 0)) == -1)	// not existing?
	{
		if ( (sempub = semget(PACCTPUBKEY, 1,
						0666|IPC_CREAT|IPC_EXCL)) >= 0)
		{
			(void) semctl(sempub, 0, SETVAL, SEMTOTAL);
		}
		else
		{
			perror("cannot create public semaphore");
			exit(3);
		}
	}

	/*
	** daemonize this process
	*/
	if ( fork() )
		exit(0);	// implicitly switch to background

	setsid();

	if ( fork() )
		exit(0);

	for (i=0; i < 1024; i++)
	{
		if (i != 2)	// stderr will be closed later on
			close(i);
	}

	umask(022);

	chdir("/tmp");

	/*
	** increase semaphore to define that atopacctd is running
	*/
	if ( semop(semprv, &semincr, 1) == -1)
       	{
		perror("cannot increment private semaphore");
		exit(4);
	}

	/*
	** create source accounting file to which the kernel can write
	** its records
	*/
	snprintf(accountpath, sizeof accountpath, "%s/%s", pacctdir, PACCTORIG);

	(void) unlink(accountpath);

	if ( (afd = creat(accountpath, 0600)) == -1)
       	{
		perror(accountpath);
		exit(5);
	}

	(void) close(afd);

	/*
 	** open the accounting file for read
	*/
	if ( (afd = open(accountpath, O_RDONLY)) == -1)
       	{
		perror(accountpath);
		exit(5);
	}

	/*
 	** create directory to store the shadow files
	*/
	snprintf(shadowdir, sizeof shadowdir, "%s/%s",
					pacctdir, PACCTSHADOWD);

	if ( mkdir(shadowdir, 0755) == -1)
       	{
		perror(shadowdir);
		exit(5);
	}

	sfd = createshadow(curshadow);
	setcurrent(curshadow);

	/*
	** switch on accounting
	*/
	if ( acct(accountpath) == -1)
	{
		perror("cannot switch on process accounting");
		(void) unlink(accountpath);
		exit(5);
	}

	/*
	** open syslog interface for failure messages
	*/
	openlog("atopacctd", LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "%s  <gerlof.langeveld@atoptool.nl>", getstrvers());

	/*
	** raise priority (be sure the nice value becomes -20,
	** independent of the current nice value)
	*/
	(void) nice(-39);

	/*
 	** connect to NETLINK socket of kernel to be triggered
	** when processes have finished
	*/
	if ( (nfd = netlink_open()) == -1)
	{
		(void) acct(0);
		(void) unlink(accountpath);
		exit(6);
	}

	/*
	** close stderr
	** from now on, only messages via syslog
	*/
	(void) close(2);

	/*
	** main loop
	*/
	for (EVER)
	{
		int maxcnt = 50;	// retry counter

		/*
 		** wait for info from NETLINK indicating that at least
		** one process has finished; the real contents of the
		** NETLINK message is ignored, it is only used as trigger
		** that something can be read from the accounting file
		*/
		nsz = recv(nfd, nbuf, sizeof nbuf, 0);

		switch (nsz)
		{
		   case 0:		// EOF?
			break;

		   case -1:		// failure?
			switch (errno)
			{
			   // acceptable errors that might indicate that
			   // processes have terminated
			   case EINTR:
			   case ENOMEM:
			   case ENOBUFS:
				break;

			   default:
				syslog(LOG_ERR, "unexpected recv error: %s\n",
							strerror(errno));
				exit(8);
			}
		}

		/*
 		** read process accounting record(s)
		** such record may not immediately be available (timing matter)
		*/
		while ((asz = read(afd, abuf, sizeof abuf)) == 0 && maxcnt--)
			usleep(10000);

		switch (asz)
		{
		   case 0:		// EOF?
			continue;	// wait for NETLINK again

		   case -1:		// failure?
			syslog(LOG_INFO, "%s - unexpected read error: %s\n",
						accountpath, strerror(errno));
			break;

		   default:
			/*
 			** only once: determine the size of an accounting
			** record and calculate the maximum size for each
			** shadow file
			*/
			if (!arecsize)
			{
				arecsize = acctsize((struct acct *)abuf);

				if (arecsize)
				{
					maxshadowsz = maxshadowrec * arecsize;
				}
				else
				{
					syslog(LOG_ERR,
					   "cannot determine size of record\n");
					exit(9);
				}
			}

			/*
 			** truncate process accounting file regularly
			*/
			atotsize += asz;	// maintain current size

			if (atotsize >= MAXORIGSZ)
			{
				if (truncate(accountpath, 0) != -1)
				{
					lseek(afd, 0, SEEK_SET);
					atotsize = 0;
				}
			}

			/*
 			** determine if any client is using the shadow
			** accounting files; if not, verify if clients
			** have been using the shadow files till now and
			** cleanup has to be performed
			*/
			if (NUMCLIENTS == 0)
			{
				/*
				** did last client just disappeared?
				*/
				if (shadowbusy)
				{
					/*
					** remove all shadow files
					*/
					gcshadows(&oldshadow, curshadow+1);
					oldshadow = 0;
					curshadow = 0;
					stotsize  = 0;

					/*
 					** create new file with sequence 0
					*/
					(void) close(sfd);

					sfd = createshadow(curshadow);
					setcurrent(curshadow);

					shadowbusy = 0;
				}
		
				continue;
			}

			shadowbusy = 1;

			/*
 			** pass process accounting data to shadow file
			** but be careful to fill a shadow file exactly
			** to its maximum and not more...
			*/
			if (stotsize + asz <= maxshadowsz) 	// would fit?
			{
				/*
				** pass all data
				*/
				if ( (ssz = pass2shadow(sfd, abuf, asz)) > 0)
					stotsize += ssz;

				remsz  = 0;
				partsz = 0;
			}
			else
			{
				/*
				** calculate the remainder that has to
				** be written to a new shadow file
				*/
				partsz = maxshadowsz - stotsize;
				remsz  = asz - partsz;

				/*
				** pass first part of the data
				*/
				if ( (ssz = pass2shadow(sfd, abuf, partsz)) > 0)
					stotsize += ssz;
			}

			/*
 			** verify if current shadow file has reached its
			** maximum size; if so, switch to next sequence number
			** and write remaining data (if any)
			*/
			if (stotsize >= maxshadowsz)
			{
				close(sfd);

				sfd = createshadow(++curshadow);
				setcurrent(curshadow);

				stotsize = 0;

				/*
				** pass remaining part of the data
				*/
				if (remsz)
				{
					if ( (ssz = pass2shadow(sfd,
						    abuf+partsz, remsz)) > 0)
						stotsize += ssz;
				}
			}
		}

		/*
 		** verify if garbage collection is needed
		** i.e. removal of shadow files that are not in use any more
		*/
		if ( shadowbusy && time(0) > gclast + GCINTERVAL )
		{
			gcshadows(&oldshadow, curshadow);
			gclast = time(0);
		}
	}

	(void) acct(0);
	(void) unlink(accountpath);
	return 0;
}

/*
** create first shadow file with requested sequence number
*/
static int
createshadow(long seq)
{
	int	sfd;
	char	shadowpath[128];

	snprintf(shadowpath, sizeof shadowpath, PACCTSHADOWF,
				pacctdir,  PACCTSHADOWD, seq);

	/*
	** open the shadow file for write
	*/
	if ( (sfd = creat(shadowpath, 0644)) == -1)
	{
		perror(shadowpath);
		exit(5);
	}

	return sfd;
}

/*
** pass process accounting data to shadow file
*/
static int
pass2shadow(int sfd, char *sbuf, int ssz)
{
	static unsigned long long	nrskipped;
	struct statvfs			statvfs;

	/*
	** check if the filesystem is not filled for more than 95%
	*/
	if ( fstatvfs(sfd, &statvfs) != -1)
	{
		if (statvfs.f_bfree * 100 / statvfs.f_blocks < 5)
		{
			if (nrskipped == 0)	// first skip?
			{
				syslog(LOG_ERR, "Filesystem > 95%% full; "
						"pacct writing skipped\n");
			}

			nrskipped++;

			return 0;
		}
	}

	if (nrskipped)
	{
		syslog(LOG_ERR, "Pacct writing continued (%llu skipped)\n",
								nrskipped);
		nrskipped = 0;
	}

	/*
 	** pass process accounting record(s) to shadow file
	*/
	if ( write(sfd, sbuf, ssz) == -1 )
	{
		syslog(LOG_ERR, "unexpected write error to shadow file: %s\n",
 		     					strerror(errno));
		exit(7);
	}

	return ssz;
}

/*
** remove old shadow files not being in use any more
**
** When a reading process (atop) opens a shadow file,
** it places a read lock on the first byte of that file.
** More then one read lock is allowed on that first byte
** (in case of more atop incarnations).
** If at least one read lock exists, a write lock (to be
** tried here) will fail which means that the file is still
** in use by at least one reader.
*/
static void
gcshadows(long *oldshadow, long curshadow)
{
	struct flock	flock;
	int		tmpsfd;
	char		shadowpath[128];
	long 		save_old;

	/*
	** fill flock structure: write lock on first byte
	*/
	flock.l_type 	= F_WRLCK;
	flock.l_whence 	= SEEK_SET;
	flock.l_start 	= 0;
	flock.l_len 	= 1;
	
	for (save_old = *oldshadow; *oldshadow < curshadow; (*oldshadow)++)
	{
		/*
		** assemble path name of shadow file (starting by oldest)
		*/
		snprintf(shadowpath, sizeof shadowpath, PACCTSHADOWF,
					pacctdir, PACCTSHADOWD, *oldshadow);

		/*
		** try to open oldest existing file for write
 		** and verify if it is in use
		*/
		if ( (tmpsfd = open(shadowpath, O_WRONLY)) == -1)
			break;

		if ( fcntl(tmpsfd, F_SETLK, &flock) == -1)  // no-wait trial
		{
			close(tmpsfd);
			break;		// setting lock failed, so still in use
		}

		/*
 		** lock successfully set, so file is unused: remove file;
		** closing the file implicitly removes the succeeded lock
		*/
		(void) close(tmpsfd);
		(void) unlink(shadowpath);
	}
}

/*
** write sequence number of current (newest) file
*/
static void
setcurrent(long curshadow)
{
	int	cfd, len;
	char	currentpath[128], currentdata[128];

	/*
	** assemble file name of currency file
	*/
	snprintf(currentpath, sizeof currentpath, "%s/%s/%s",
			pacctdir, PACCTSHADOWD, PACCTSHADOWC);

	/*
	** assemble ASCII string to be written: seqnumber/maxrecords
	*/
	len = snprintf(currentdata, sizeof currentdata, "%ld/%lu",
					curshadow, maxshadowrec);

	/*
	** create new currency file and write assembled string
	*/
	if ( (cfd = creat(currentpath, 0644)) != -1)
	{
		(void) write(cfd, currentdata, len);
		close(cfd);
	}
	else
	{
		syslog(LOG_ERR, "current number could not be written: %s\n",
 		     					strerror(errno));
	}
}

/*
** determine the size of an accounting record
*/
static int
acctsize(struct acct *parec)
{
	switch (parec->ac_version & 0x0f)
	{
	   case 2:
 		return sizeof(struct acct);

	   case 3:
	   	return sizeof(struct acct_v3);

	   default:
		return 0;
	}
}

/*
** generate version number and date
*/
char *
getstrvers(void)
{
	static char vers[256];

	snprintf(vers, sizeof vers, "Version: %s - %s",
		atopacctdversion, atopacctddate);

	return vers;
}
