/*
** The atopacctd daemon switches on the process accounting feature
** in the kernel and let the process accounting records be written to
** a file, called the source file. After process accounting is active,
** the atopacctd daemon transfers every process accounting record that
** is available in the source file to a shadow file.
** Client processes (like atop) can read the shadow file instead of
** the original process accounting source file.
**
** This approach has the following advantages:
**
** - The atopacctd daemon keeps the source file to a limited size
**   by truncating it regularly.
**
** - The atopacct daemon takes care that a shadow file has a limited size.
**   As soon as the current shadow file reaches its maximum size, the
**   atopacctd daemon creates a subsequent shadow file. For this reason,
**   the name of a shadow file contains a sequence number.
**   Shadow files that are not used by client processes any more, are
**   automatically removed by the atopacctd daemon.
**
** - When no client processes are active (any more), all shadow files
**   will be deleted and no records will be transferred to shadow files
**   any more. As soon as at least one client is activated again, the
**   atopacctd daemon start writing shadow files again.
**
** The directory /run is used as the default top-directory. An
** alternative top-directory can be specified as command line argument
** (in that case, also modify /etc/atoprc to inform atop as a client).
** Below this top-directory the source file pacct_source is created and
** the subdirectory pacct_shadow.d as a 'container' for the shadow files.
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
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <sys/wait.h>

#include "atop.h"
#include "photoproc.h"
#include "acctproc.h"
#include "version.h"
#include "versdate.h"
#include "atopacctd.h"

#define	RETRYCNT	10	// # retries to read account record
#define	RETRYMS		25	// timeout (millisec) to read account record
#define	NORECINTERVAL	3600	// no-record-available interval (seconds)

#define PACCTSEC	3	// timeout (sec) to retry switch on pacct
#define POLLSEC		1	// timeout (sec) when NETLINK fails

#define GCINTERVAL      60      // garbage collection interval (seconds)

/*
** Semaphore-handling
**
** Two semaphore groups are created:
**
** The private semaphore (group) specifies the number of atopacctd processes
** running (to be sure that only one daemon is active at the time).
**
** The public semaphore group contains two semaphores:
**
**   0: the number of processes using the process accounting shadow files,
**	i.e. the number of (atop) clients. This semaphore starts at a high
**	value and should be decremented by every (atop) client that starts
**	using the shadow files and incremented again whenever that (atop)
**	client terminates.
**
**   1:	binary semphore that has to be claimed before using/modifying
**	semaphore 0 in this group.
*/
static int		semprv;


static int		sempub;
#define SEMTOTAL        100
#define	NUMCLIENTS	(SEMTOTAL - semctl(sempub, 0, GETVAL, 0))

struct sembuf   semlocknowait = {1, -1, SEM_UNDO|IPC_NOWAIT},
                semunlock     = {1, +1, SEM_UNDO};


static char		atopacctdversion[] = ATOPVERS;
static char		atopacctddate[]    = ATOPDATE;

static unsigned long	maxshadowrec = MAXSHADOWREC;
static char		*pacctdir    = PACCTDIR;

static char		cleanup_and_go = 0;

/*
** function prototypes
*/
static int	awaitprocterm(int, int, int, char *, int *,
				unsigned long *, unsigned long *);
static int	swonpacct(int, char *);
static int	createshadow(long);
static int	pass2shadow(int, char *, int);
static void	gcshadows(unsigned long *, unsigned long);
static void	setcurrent(long);
static int	acctsize(struct acct *);
static void	parent_cleanup(int);
static void	child_cleanup(int);


int
main(int argc, char *argv[])
{
	int			i, nfd, afd, sfd;
	int			parentpid;
	struct stat		dirstat;
	struct rlimit		rlim;
	FILE			*pidf;

	struct sembuf		semincr = {0, +1, SEM_UNDO};

	char			shadowdir[128], shadowpath[128];
	char			accountpath[128];
	unsigned long		oldshadow = 0, curshadow = 0;
	int 			shadowbusy = 0;
	time_t			gclast = time(0);

	struct sigaction	sigcleanup;
	int			liResult;

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
		** if first argument is not a flag, it should be the name
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
	** verify that the top directory is not world-writable
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
			"atopacctd: directory %s may not be writable "
			"for group/others\n", pacctdir);
		exit(2);
	}

	/*
	** create the semaphore groups and initialize the semaphores;
	** if the private semaphore already exists, verify if another
	** atopacctd daemon is already running
	*/
	if ( (semprv = semget(PACCTPRVKEY, 0, 0)) >= 0)	// exists?
	{
		if ( semctl(semprv, 0, GETVAL, 0) > 0)
		{
			fprintf(stderr, "atopacctd is already running!\n");
			exit(3);
		}
	}
	else
	{
		if ( (semprv = semget(PACCTPRVKEY, 1,
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

	// create new semaphore group
	//
	if ( (sempub = semget(PACCTPUBKEY, 0, 0)) != -1)	// existing?
		(void) semctl(sempub, 0, IPC_RMID, 0);

	if ( (sempub = semget(PACCTPUBKEY, 2, 0666|IPC_CREAT|IPC_EXCL)) >= 0)
	{
		(void) semctl(sempub, 0, SETVAL, SEMTOTAL);
		(void) semctl(sempub, 1, SETVAL, 1);
	}
	else
	{
		perror("cannot create public semaphore");
		exit(3);
	}

	/*
	** daemonize this process
	** i.e. be sure that the daemon is no session leader (any more)
	** and get rid of a possible bad context that might have been
	** inherited from ancestors
	*/
	parentpid = getpid();		// to be killed when initialized

	/*
 	** prepare cleanup signal handler
	*/
	memset(&sigcleanup, 0, sizeof sigcleanup);
	sigemptyset(&sigcleanup.sa_mask);
	sigcleanup.sa_handler	= parent_cleanup;
	(void) sigaction(SIGTERM, &sigcleanup, (struct sigaction *)0);

	if ( fork() )			// implicitly switch to background
   	{
		/*
 		** parent after forking first child:
		** wait for signal 15 from child before terminating
		** because systemd expects parent to terminate whenever
		** service is up and running
		*/
		pause();		// wait for signal from child
		exit(0);		// finish parent
	}

	setsid();			// become session leader to lose ctty

	if ( fork() )			// finish parent; continue in child
		exit(0);		// --> no session leader, no ctty

	sigcleanup.sa_handler	= child_cleanup;
	(void) sigaction(SIGTERM, &sigcleanup, (struct sigaction *)0);

	getrlimit(RLIMIT_NOFILE, &rlim);

	for (i=0; i < rlim.rlim_cur; i++)	// close all files, but
	{
		if (i != 2)			// do not close stderr 
			close(i);
	}

	umask(022);

	liResult = chdir("/tmp");			// go to a safe place

	if(liResult != 0)
	{
		char lcMessage[64];

		snprintf(lcMessage, sizeof(lcMessage) - 1,
		          "%s:%d - Error %d changing to tmp dir\n", 
		          __FILE__, __LINE__, errno );
		fprintf(stderr, "%s", lcMessage);
	}

	/*
	** increase semaphore to define that atopacctd is running
	*/
	if ( semop(semprv, &semincr, 1) == -1)
       	{
		perror("cannot increment private semaphore");
		kill(parentpid, SIGTERM);
		exit(4);
	}

	/*
	** create source accounting file to which the kernel can write
	** its records
	*/
	snprintf(accountpath, sizeof accountpath, "%s/%s", pacctdir, PACCTORIG);

	(void) unlink(accountpath);	// in case atopacctd previously killed

	if ( (afd = creat(accountpath, 0600)) == -1)
       	{
		perror(accountpath);
		kill(parentpid, SIGTERM);
		exit(5);
	}

	(void) close(afd);

	/*
 	** open the accounting file for read
	*/
	if ( (afd = open(accountpath, O_RDONLY)) == -1)
       	{
		perror(accountpath);
		kill(parentpid, SIGTERM);
		exit(5);
	}

	/*
 	** create directory to store the shadow files
	** when atopacctd was previously killed, rename the
	** old directory to a unique name
	*/
	snprintf(shadowdir, sizeof shadowdir, "%s/%s", pacctdir, PACCTSHADOWD);

	if ( stat(shadowdir, &dirstat) == 0 )	// already exists?
	{
		if (S_ISDIR(dirstat.st_mode))	// and is directory?
		{
			// define new name to save directory
			char newshadow[256];

			snprintf(newshadow, sizeof newshadow, "%s-old-%d",
							shadowdir, getpid());

			if ( rename(shadowdir, newshadow) == -1)
			{
				perror("could not rename old shadow directory");
				exit(5);
			}
		}
	}

	if ( mkdir(shadowdir, 0755) == -1)
       	{
		perror(shadowdir);
		kill(parentpid, SIGTERM);
		exit(5);
	}

	sfd = createshadow(curshadow);
	setcurrent(curshadow);

	/*
	** open syslog interface 
	*/
	openlog("atopacctd", LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "%s  <gerlof.langeveld@atoptool.nl>", getstrvers());

	/*
	** raise priority (be sure the nice value becomes -20,
	** independent of the current nice value)
	** this may fail without notice for non-privileged processes
	*/
	liResult = nice(-39);

	/*
 	** connect to NETLINK socket of kernel to be triggered
	** when processes have finished
	*/
	if ( (nfd = netlink_open()) == -1)
	{
		(void) unlink(accountpath);
		kill(parentpid, SIGTERM);
		exit(5);
	}

	/*
	** switch on accounting - inital
	*/
	if ( swonpacct(afd, accountpath) == -1)
	{
		(void) unlink(accountpath);
		kill(parentpid, SIGTERM);
		exit(6);
	}

	syslog(LOG_INFO, "accounting to %s", accountpath);

	/*
	** signal handling
	*/
	(void) signal(SIGHUP, SIG_IGN);

	(void) sigaction(SIGINT,  &sigcleanup, (struct sigaction *)0);
	(void) sigaction(SIGQUIT, &sigcleanup, (struct sigaction *)0);
	(void) sigaction(SIGTERM, &sigcleanup, (struct sigaction *)0);

	/*
 	** create PID file
	*/
	if ( (pidf = fopen(PIDFILE, "w")) )
	{
		fprintf(pidf, "%d\n", getpid());
		fclose(pidf);
	}

	/*
	** terminate parent: service  initialized
	*/
	kill(parentpid, SIGTERM);

	/*
	** main loop
	*/
	while (! cleanup_and_go)
	{
		int	state;
		time_t	curtime;

		/*
		** await termination of (at least) one process and
		** copy the process accounting record(s)
		*/
		state = awaitprocterm(nfd, afd, sfd, accountpath,
					&shadowbusy, &oldshadow, &curshadow);

		if (state == -1)	// irrecoverable error?
			break;

		/*
 		** garbage collection (i.e. removal of shadow files that
		** are not in use any more) is needed in case:
		**
		** - shadow files are currently maintained because
		**   at least one atop is running, and
		** - shadow files have not been removed for GCINTERVAL
		**   seconds, or
		** - the system clock has been modified (lowered)
		*/
		if ( shadowbusy &&
			(time(&curtime) > gclast + GCINTERVAL ||
		               curtime  < gclast                ) )
		{
			gcshadows(&oldshadow, curshadow);
			gclast = time(0);
		}
	}

	/*
	** cleanup and terminate
	*/
	(void) acct((char *) 0);	// disable process accounting
	(void) unlink(accountpath);	// remove source file

	for (; oldshadow <= curshadow; oldshadow++)	// remove shadow files
	{
		/*
		** assemble path name of shadow file (starting by oldest)
		*/
		snprintf(shadowpath, sizeof shadowpath, PACCTSHADOWF,
					pacctdir, PACCTSHADOWD, oldshadow);

		(void) unlink(shadowpath);
	}

	snprintf(shadowpath, sizeof shadowpath, "%s/%s/%s",
			pacctdir, PACCTSHADOWD, PACCTSHADOWC);

	(void) unlink(shadowpath);	// remove file 'current'
	(void) rmdir(shadowdir);	// remove shadow.d directory

	if (cleanup_and_go)
	{
		syslog(LOG_NOTICE, "Terminated by signal %d\n", cleanup_and_go);

		if (cleanup_and_go == SIGTERM)
			return 0;
		else
			return cleanup_and_go + 128;
	}
	else
	{
		syslog(LOG_NOTICE, "Terminated!\n");
		return 13;
	}
}


/*
** wait for at least one process termination and copy process accounting
** record(s) from the source process accounting file to the current
** shadow accounting file
**
** return code:	0 - no process accounting record read
**              1 - at least one process accounting record read
**             -1 - irrecoverable failure
*/
static int
awaitprocterm(int nfd, int afd, int sfd, char *accountpath,
	int *shadowbusyp, unsigned long *oldshadowp, unsigned long *curshadowp)
{
	static int			arecsize, netlinkactive = 1;
	static unsigned long long	atotsize, stotsize, maxshadowsz;
	static time_t			reclast;
	struct timespec			retrytimer = {0, RETRYMS/2*1000000};
	int				retrycount = RETRYCNT;
	int				asz, rv, ssz;
	char				abuf[16000];
	int				partsz, remsz;

	/*
	** neutral state:
	**
 	** wait for info from NETLINK indicating that at least
	** one process has finished; the real contents of the
	** NETLINK message is ignored, it is only used as trigger
	** that something can be read from the process accounting file
	**
	** unfortunately it is not possible to use inotify() on the
	** source file as a trigger that a new accounting record
	** has been written (does not work if the kernel itself
	** writes to the file)
	**
	** when the NETLINK interface fails due to kernel bug 190711,
	** we switch to polling mode:
	**	wait for timer and verify if process accounting
	**      records are available (repeatedly); ugly but the only
	**	thing we can do if we can't use NETLINK
	*/
	if (netlinkactive)
	{
		rv = netlink_recv(nfd, 0);

		if (rv == 0) 		// EOF?
		{
			syslog(LOG_ERR, "unexpected EOF on NETLINK\n");
			perror("unexpected EOF on NETLINK\n");
			return -1;
		}

		if (rv < 0)		// failure?
		{
			switch (-rv)
			{
		   	   // acceptable errors that might indicate that
		   	   // processes have terminated
		   	   case EINTR:
		   	   case ENOMEM:
		   	   case ENOBUFS:
				break;
	
		   	   default:
				syslog(LOG_ERR,
					"unexpected error on NETLINK: %s\n",
					strerror(-rv));
				fprintf(stderr,
					"unexpected error on NETLINK: %s\n",
                                       	strerror(-rv));

				if (-rv == EINVAL)
				{
				   	syslog(LOG_ERR,
				   	"(see ATOP README about kernel bug 190711)\n");
					fprintf(stderr,
				   	"(see ATOP README about kernel bug 190711)\n");
				}

				syslog(LOG_ERR,
					"switching to polling mode\n");
				fprintf(stderr,
					"switching to polling mode\n");

				netlinkactive = 0;	// polling mode wanted

				return 0;
			}
		}

		/*
 		** get rid of all other waiting finished processes via netlink
		** before handling the process accounting record(s)
		*/
		while ( netlink_recv(nfd, MSG_DONTWAIT) > 0 );
	}
	else	// POLLING MODE
	{
		sleep(POLLSEC);
		retrycount = 1;
	}

	/*
 	** read new process accounting record(s)
	** such record(s) may not immediately be available (timing matter),
	** so some retries might be necessary 
	*/
	while ((asz = read(afd, abuf, sizeof abuf)) == 0 && --retrycount)
	{
		nanosleep(&retrytimer, (struct timespec *)0);
		retrytimer.tv_nsec = RETRYMS*1000000;
	}

	switch (asz)
	{
	   case 0:		// EOF (no records available)?
		if (time(0) > reclast + NORECINTERVAL && reclast)
		{
			syslog(LOG_WARNING, "reactivate process accounting\n");

			if (truncate(accountpath, 0) != -1)
			{
				lseek(afd, 0, SEEK_SET);
				atotsize = swonpacct(afd, accountpath);
			}

			reclast = time(0);
		}

		return 0;	// wait for NETLINK again

	   case -1:		// failure?
		syslog(LOG_ERR, "%s - unexpected read error: %s\n",
					accountpath, strerror(errno));
		return -1;
	}

	reclast = time(0);

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
			       "cannot determine size of account record\n");
			return -1;
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
	if (semop(sempub, &semlocknowait, 1) == 0) 	// lock succeeded?
	{
		if (NUMCLIENTS == 0)
		{
			/*
			** did last client just disappear?
			*/
			if (*shadowbusyp)
			{
				/*
				** remove all shadow files
				*/
				gcshadows(oldshadowp, (*curshadowp)+1);
				*oldshadowp = 0;
				*curshadowp = 0;
				stotsize    = 0;
	
				/*
 				** create new file with sequence 0
				*/
				(void) close(sfd);
	
				sfd = createshadow(*curshadowp);
				setcurrent(*curshadowp);
	
				*shadowbusyp = 0;
			}
	
			(void) semop(sempub, &semunlock, 1);
			return 1;
		}

		(void) semop(sempub, &semunlock, 1);
	}

	*shadowbusyp = 1;

	/*
 	** transfer process accounting data to shadow file
	** but take care to fill a shadow file exactly
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
		** be written to the next shadow file
		*/
		partsz = maxshadowsz - stotsize;
		remsz  = asz - partsz;

		/*
		** transfer first part of the data to current
		** shadow file
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

		sfd = createshadow(++(*curshadowp));
		setcurrent(*curshadowp);

		stotsize = 0;

		/*
		** transfer remaining part of the data
		*/
		if (remsz)
		{
			if ( (ssz = pass2shadow(sfd, abuf+partsz, remsz)) > 0)
				stotsize += ssz;
		}
	}

	return 1;
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
** transfer process accounting data to shadow file
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
		if (statvfs.f_blocks == 0 			||
		    statvfs.f_bfree * 100 / statvfs.f_blocks < 5  )
		{
			if (nrskipped == 0)	// first skip?
			{
				syslog(LOG_WARNING,
					"Filesystem > 95%% full; "
					"pacct writing skipped\n");
			}

			nrskipped++;

			return 0;
		}
	}

	/*
  	** there is enough space in the filesystem (again)
	** verify if writing has been suspended due to lack of space (if so,
	** write a log message)
	*/
	if (nrskipped)
	{
		syslog(LOG_WARNING, "Pacct writing continued (%llu skipped)\n",
								nrskipped);
		nrskipped = 0;
	}

	/*
 	** transfer process accounting record(s) to shadow file
	*/
	if ( write(sfd, sbuf, ssz) == -1 )
	{
		syslog(LOG_ERR, "Unexpected write error to shadow file: %s\n",
 		     					strerror(errno));
		exit(7);
	}

	return ssz;
}


/*
** switch on the process accounting mechanism
**   first parameter:	file descriptor of open accounting file
**   second parameter:	name of accounting file
**   return value:	-1 in case of permanent failure,
** 			otherwise number of bytes read from accounting file
*/
static int
swonpacct(int afd, char *accountpath)
{
	int  n, acctokay = 0;
	char abuf[4096];

	/*
	** due to kernel bug 190271 (process accounting sometimes
	** does not work), we verify if process accounting really
	** works after switching it on. If not, we keep retrying
	** for a while.
	*/
	while (! acctokay)
	{
		int  maxcnt = 40;

		/*
		** switch on process accounting
		*/
		if ( acct(accountpath) == -1)
		{
			perror("cannot switch on process accounting");
			return -1;
		}

		/*
		** try if process accounting works by spawning a
		** child process that immediately finishes (should
		** result in a process accounting record)
		*/
		if ( fork() == 0 )	
			exit(0);

		(void) wait((int *)0);		// wait for child to finish

		while ( (n = read(afd, abuf, sizeof abuf)) <= 0 && --maxcnt)
			usleep(50000);

		if (n > 0)			// process accounting works
		{
			acctokay = 1;
		}
		else
		{
			syslog(LOG_ERR,
			   "Retrying to switch on process accounting\n");
			syslog(LOG_ERR,
			   "(see ATOP README about kernel bug 190271)\n");

			acct((char *)0);	// switch off process accounting
			sleep(PACCTSEC);	// wait a while before retrying
		}
	}

	return n;
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
gcshadows(unsigned long *oldshadowp, unsigned long curshadow)
{
	struct flock	flock;
	int		tmpsfd;
	char		shadowpath[128];

	/*
	** fill flock structure: write lock on first byte
	*/
	flock.l_type 	= F_WRLCK;
	flock.l_whence 	= SEEK_SET;
	flock.l_start 	= 0;
	flock.l_len 	= 1;
	
	for (; *oldshadowp < curshadow; (*oldshadowp)++)
	{
		/*
		** assemble path name of shadow file (starting by oldest)
		*/
		snprintf(shadowpath, sizeof shadowpath, PACCTSHADOWF,
					pacctdir, PACCTSHADOWD, *oldshadowp);

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
	static int	cfd = -1;
	char		currentpath[128], currentdata[128];
	int		len;
	int		liResult;

	/*
	** assemble file name of currency file and open (only once)
	*/
	if (cfd == -1)
	{
		snprintf(currentpath, sizeof currentpath, "%s/%s/%s",
			pacctdir, PACCTSHADOWD, PACCTSHADOWC);

		if ( (cfd = creat(currentpath, 0644)) == -1)
		{
			syslog(LOG_ERR, "Could not create currency file: %s\n",
 		     					strerror(errno));
			return;
		}
	}

	/*
	** assemble ASCII string to be written: seqnumber/maxrecords
	*/
	len = snprintf(currentdata, sizeof currentdata, "%ld/%lu",
					curshadow, maxshadowrec);

	/*
	** wipe currency file and write new assembled string
	*/
	liResult = ftruncate(cfd, 0);

	if(liResult != 0)
	{
		char lcMessage[64];

		snprintf(lcMessage, sizeof(lcMessage) - 1,
		          "%s:%d - Error %d ftruncate\n\n", __FILE__, __LINE__,
		          errno );
		fprintf(stderr, "%s", lcMessage);
	}

	(void) lseek(cfd, 0, SEEK_SET);

	liResult = write(cfd, currentdata, len);

	if(liResult == -1)
	{
		char lcMessage[64];

		snprintf(lcMessage, sizeof(lcMessage) - 1,
		          "%s:%d - Error %d writing\n\n", __FILE__, __LINE__,
		          errno );
		fprintf(stderr, "%s", lcMessage);
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

/*
** signal catchers:
**    set flag to be verified in main loop to cleanup and terminate
*/
void
child_cleanup(int sig)
{
	cleanup_and_go = sig;
}

void
parent_cleanup(int sig)
{
	exit(0);
}
