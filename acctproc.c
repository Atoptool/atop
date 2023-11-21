/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains functions to manipulate with process-accounting
** features (switch it on/off and read the process-accounting records).
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2, or (at your option) any
** later version.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU General Public License for more details.
*/

#define	_FILE_OFFSET_BITS	64

#define _GNU_SOURCE
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "atop.h"
#include "photoproc.h"
#include "acctproc.h"
#include "atopacctd.h"

#define	ACCTDIR		"/var/cache/atop.d"
#define	ACCTFILE	"atop.acct"
#define	ACCTENV		"ATOPACCT"

static	char 	acctatop;	/* boolean: atop's own accounting busy ?  */
static	off_t	acctsize;	/* previous size of account file	  */
static	int	acctrecsz;	/* size of account record		  */
static	int	acctversion;	/* version of account record layout	  */
static	int   	acctfd = -1;	/* fd of account file (-1 = not open)     */

static	long	curshadowseq;	/* current shadow file sequence number    */
static	long 	maxshadowrec;   /* number of records per shadow file      */

static	char	*pacctdir = PACCTDIR;

static count_t 	acctexp (comp_t  ct);
static int	acctvers(int);
static void	acctrestarttrial(void);
static void	switchshadow(void);
static int	atopacctd(int);

/*
** possible process accounting files used by (ps)acct package
*/
struct pacctadm {
	char		*name;
	struct stat	stat;
} pacctadm[] = {
	{ "/var/log/pacct",		{0, }, },
	{ "/var/account/pacct",		{0, }, },
	{ "/var/log/account/pacct",	{0, }, }
};

struct pacctadm *pacctcur;	// pointer to entry in use

/*
** Semaphore-handling
**
**  A semaphore-group with two semaphores is created
**
**     0 - Binary semaphore (mutex) to get access to active atop-counter
**
**     1 - Active atop-counter (inverted).
**         This semaphore is initialized at some high value and is
**         decremented by every atop-incarnation which uses the private
**         accounting-file and incremented as soon as such atop stops again.
**         If an atop-incarnation stops and it appears to be the last one
**         using the private accounting-file, accounting is stopped
**         and the file removed
**	   (Yes, I know: it's not proper usage of the semaphore-principle).
*/ 
#define	ATOPACCTKEY	3121959
#define	ATOPACCTTOT	100

struct sembuf	semclaim     =  {0, -1, SEM_UNDO},
		semrelse     =  {0, +1, SEM_UNDO},
		semdecre     =  {1, -1, SEM_UNDO},
		semincre     =  {1, +1, SEM_UNDO};

struct sembuf	semreglock[] = {{0, -1, SEM_UNDO|IPC_NOWAIT},
				{1, -1, SEM_UNDO|IPC_NOWAIT}},
		semunlock    =  {1, +1, SEM_UNDO};

/*
** switch on the process-accounting mechanism
**
** return value:
**    0 -     activated (success)
**    1 - not activated: unreadable accounting file
**    2 - not activated: empty environment variable ATOPACCT
**    3 - not activated: no access to semaphore group
**    4 - not activated: impossible to create own accounting file
**    5 - not activated: no root privileges
*/
int
acctswon(void)
{
	int			i, j, sematopid;
	static unsigned short 		vals[] = {1, ATOPACCTTOT};
	union {unsigned short *array;}	arg = {vals};
	struct stat		statbuf;
	char			*ep;
	int			ret;

	/*
	** when a particular environment variable is present, atop should
	** use a specific accounting-file (as defined by the environment
	** variable) or should use no process accounting at all (when
	** contents of environment variable is empty)
	*/
	if ( (ep = getenv(ACCTENV)) )
	{
		/*
		** environment variable exists
		*/
		if (*ep)
		{
			/*
			** open active account file with the specified name
			*/
			if (! droprootprivs() )
				mcleanstop(42, "failed to drop root privs\n");

			if ( (acctfd = open(ep, O_RDONLY) ) == -1)
				return 1;

			if ( !acctvers(acctfd) )
			{
				(void) close(acctfd);
				acctfd = -1;
				return 1;
			}

			supportflags |= ACCTACTIVE;
			return 0;
		}
		else
		{
			/*
			** no contents
			*/
			return 2;
		}
	}

	/*
 	** when the atopacctd daemon is active on this system,
	** it should be the preferred way to read the accounting records
	*/
	if ( (ret = atopacctd(1)) >= 0 )
		return ret;

	/*
	** check if process accounting is already switched on 
	** for one of the standard accounting-files; in that case we
	** open this file and get our info from here ....
	*/
	for (i=0, j=0; i < sizeof pacctadm/sizeof pacctadm[0]; i++)
	{
		if ( stat(pacctadm[i].name, &pacctadm[i].stat) == 0)
			j++;
	}

	if (j)
	{
		/*
		** at least one file is present; check if it is really in use
		** at this moment for accounting by forking a child-process
		** which immediately finishes to force a new accounting-record
		*/
		if ( fork() == 0 )
			exit(0);	/* let the child finish */

		(void) wait((int *) 0); /* let the parent wait  */

		for (i=0; i < sizeof pacctadm/sizeof pacctadm[0]; i++)
		{
			if ( stat(pacctadm[i].name, &statbuf) == 0)
			{
				/*
				** accounting-file has grown?
				*/
				if (statbuf.st_size > pacctadm[i].stat.st_size)
				{
					/*
					** open active account file
					*/
					if ( (acctfd = open(pacctadm[i].name,
							    O_RDONLY) ) == -1)
						return 1;

					if ( !acctvers(acctfd) )
					{
						(void) close(acctfd);
						acctfd = -1;
						return 1;
					}

					supportflags |= ACCTACTIVE;

					pacctcur = &pacctadm[i];  // register current

					return 0;
				}
			}
		}
	}

	/*
	** process-accounting is not yet switched on in a standard way;
	** check if another atop has switched on accounting already
	**
	** first try to create a semaphore group exclusively; if this succeeds,
	** this is the first atop-incarnation since boot and the semaphore group
	** should be initialized`
	*/
	if ( (sematopid = semget(ATOPACCTKEY, 2, 0600|IPC_CREAT|IPC_EXCL)) >= 0)
		(void) semctl(sematopid, 0, SETALL, arg);
	else
		sematopid = semget(ATOPACCTKEY, 0, 0);

	/*
	** check if we got access to the semaphore group
	*/
	if (sematopid == -1)
		return 3;

	/*
	** the semaphore group is opened now; claim exclusive rights
	*/
	(void) semop(sematopid, &semclaim, 1);

	/*
	** are we the first to use the accounting-mechanism ?
	*/
	if (semctl(sematopid, 1, GETVAL, 0) == ATOPACCTTOT)
	{
		/*
		** create a new separate directory below /tmp
		** for the accounting file;
		** if this directory exists (e.g. previous atop-run killed)
		** it will be cleaned and newly created
		*/
		if ( mkdir(ACCTDIR, 0700) == -1)
		{
			if (errno == EEXIST)
			{
				(void) acct(0);
				(void) unlink(ACCTDIR "/" ACCTFILE);
				(void) rmdir(ACCTDIR);
			}

			if ( mkdir(ACCTDIR, 0700) == -1)
			{
				/*
				** persistent failure
				*/
				(void) semop(sematopid, &semrelse, 1);
				return 4;
			}
		}

		/*
		** create accounting file in new directory
		*/
		(void) close( creat(ACCTDIR "/" ACCTFILE, 0600) );

		/*
		** switch on accounting
		*/
		if ( acct(ACCTDIR "/" ACCTFILE) < 0)
		{
			(void) unlink(ACCTDIR "/" ACCTFILE);
			(void) rmdir(ACCTDIR);
			(void) semop(sematopid, &semrelse, 1);

			return 5;
		}
	}

	/*
	** accounting is switched on now;
	** open accounting-file
	*/
	if ( (acctfd = open(ACCTDIR "/" ACCTFILE, O_RDONLY) ) < 0)
	{
		(void) acct(0);
		(void) unlink(ACCTDIR "/" ACCTFILE);
		(void) rmdir(ACCTDIR);

		(void) semop(sematopid, &semrelse, 1);
		return 1;
	}

	/*
	** accounting successfully switched on
	*/
	(void) semop(sematopid, &semdecre, 1);
	(void) semop(sematopid, &semrelse, 1);

	acctatop = 1;

	/*
	** determine version of accounting-record
	*/ 
	if (fstat(acctfd, &statbuf) == -1)
	{
		(void) acct(0);
		(void) close(acctfd);
		(void) unlink(ACCTDIR "/" ACCTFILE);
		(void) rmdir(ACCTDIR);

		acctfd = -1;
		return 1;

	}

	if (statbuf.st_size == 0)	/* no acct record written yet */
	{
		/*
		** let's write one record
		*/
		if ( fork() == 0 )
			exit(0);	/* let the child finish */

		(void) wait((int *) 0); /* let the parent wait  */
	}

	if ( !acctvers(acctfd) )
	{
		(void) acct(0);
		(void) close(acctfd);
		(void) unlink(ACCTDIR "/" ACCTFILE);
		(void) rmdir(ACCTDIR);

		acctfd = -1;
		return 1;

	}

	supportflags |= ACCTACTIVE;
	return 0;
}

/*
** try to use process accounting via the atopacctd daemon
** swon:	1 - initial switch on
**		0 - switch on again after the atopacct service has been down
*/
static int
atopacctd(int swon)
{
	int sempacctpubid;

	acctfd = -1;	// reset to not being open

	/*
 	** open semaphore group that has been initialized by atopacctd
	** semaphore 0: 100 counting down to reflect number of users of atopacctd
	** semaphore 1: value 0 is locked and value 1 is unlocked (binary semaphore)
	*/
	if ( (sempacctpubid = semget(PACCTPUBKEY, 2, 0)) != -1)
	{
		FILE			*cfp;
		char			shadowpath[128];
		struct flock		flock;
		struct timespec		maxsemwait = {3, 0};

		if (! droprootprivs() )
			mcleanstop(42, "failed to drop root privs\n");

		/*
		** lock binary semaphore and decrement semaphore 0 (extra user)
		*/
		if (semtimedop(sempacctpubid, semreglock, 2, &maxsemwait) == -1)
		{
			regainrootprivs();
			return 3;
		}

		/*
		** open the 'current' file, containing the current
		** shadow sequence number and maximum number of records
		** per shadow file
		*/
		snprintf(shadowpath, sizeof shadowpath, "%s/%s/%s",
					pacctdir, PACCTSHADOWD, PACCTSHADOWC);

		if ( (cfp = fopen(shadowpath, "r")) )
		{
			if (fscanf(cfp, "%ld/%ld",
					&curshadowseq, &maxshadowrec) == 2)
			{
				fclose(cfp);

				/*
				** open the current shadow file
				*/
				snprintf(shadowpath, sizeof shadowpath,
						PACCTSHADOWF, pacctdir,
						PACCTSHADOWD, curshadowseq);

				if ( (acctfd = open(shadowpath, O_RDONLY))!=-1)
				{
					if ( swon && !acctvers(acctfd) )
					{
						int maxcnt = 40;

						if ( fork() == 0 )
							exit(0);

						(void) wait((int *) 0);

						while ( !acctvers(acctfd) &&
								      --maxcnt)
							usleep(50000);

						if (!acctversion)
						{
							(void) close(acctfd);
							acctfd = -1;

							semop(sempacctpubid,
								&semrelse, 1);
							semop(sempacctpubid,
								&semunlock, 1);

							regainrootprivs();
							maxshadowrec = 0;
							return -1;  // try other
						}
					}

					/*
					** set read lock on current shadow file
					*/
					flock.l_type    = F_RDLCK;
					flock.l_whence  = SEEK_SET;
					flock.l_start   = 0;
					flock.l_len     = 1;

					if ( fcntl(acctfd, F_SETLK, &flock)
									!= -1)
					{
						supportflags |= ACCTACTIVE;
						regainrootprivs();
						semop(sempacctpubid,
								&semunlock, 1);
						return 0;
					}

					(void) close(acctfd);
					acctfd = -1;
				}
				else
				{
					perror("open shadowpath");
					abort();
				}
			}
			else
			{
				fprintf(stderr,
					"fscanf failed on shadow currency\n");
				fclose(cfp);
				maxshadowrec = 0;
			}
		}

		(void) semop(sempacctpubid, &semrelse, 1);
		(void) semop(sempacctpubid, &semunlock, 1);
	}

	return -1;	// try another accounting mechanism
}

/*
** determine the version of the accounting-record layout/length
** and reposition the seek-pointer to the end of the accounting file
*/
static int
acctvers(int fd)
{
	struct acct 	tmprec;

	/*
	** read first record from accounting file to verify
	** the second byte (always contains version number)
	*/
	if ( read(fd, &tmprec, sizeof tmprec) < sizeof tmprec)
		return 0;

	switch (tmprec.ac_version & 0x0f)
	{
	   case 2:
 		acctrecsz     = sizeof(struct acct);
		acctversion   = 2;
		break;

	   case 3:
	   	acctrecsz     = sizeof(struct acct_v3);
		acctversion   = 3;
		break;

	   default:
		mcleanstop(8, "Unknown format of process accounting file\n");
	}

	/*
	** accounting successfully switched on
	*/
	acctsize = acctprocnt() * acctrecsz;

	/*
	** reposition to actual file-size
	*/
	(void) lseek(fd, acctsize, SEEK_SET);

	return 1;
}

/*
** switch off the process-accounting mechanism
*/
void
acctswoff(void)
{
	int		sematopid;
	struct stat	before, after;

	/*
	** if accounting not supported, skip call
	*/
	if (acctfd == -1)
		return;

	/*
	** our private accounting-file in use?
	*/
	if (acctatop)
	{
		acctatop = 0;

		/*
		** claim the semaphore to get exclusive rights for
		** the accounting-manipulation
		*/
		sematopid = semget(ATOPACCTKEY, 0, 0);

		(void) semop(sematopid, &semclaim, 1);
		(void) semop(sematopid, &semincre, 1);

		/*
		** check if we were the last user of accounting
		*/
		if (semctl(sematopid, 1, GETVAL, 0) == ATOPACCTTOT)
		{
			/*
			** switch off private accounting
			**
			** verify if private accounting is still in use to
			** avoid that we switch off process accounting that
			** has been activated manually in the meantime
			*/
			(void) fstat(acctfd, &before);

			if ( fork() == 0 )	/* create a child and   */
				exit(0);	/* let the child finish */

			(void) wait((int *) 0); /* let the parent wait  */

			(void) fstat(acctfd, &after);

			if (after.st_size > before.st_size)
			{
				/*
				** remove the directory and file
				*/
				regainrootprivs(); /* get root privs again */

				(void) acct(0);
				(void) unlink(ACCTDIR "/" ACCTFILE);
				(void) rmdir(ACCTDIR);

				if (! droprootprivs() )
					mcleanstop(42,
						"failed to drop root privs\n");
			}
		}

		(void) semop(sematopid, &semrelse, 1);
	}

	/*
	** anyhow close the accounting-file again
	*/
	(void) close(acctfd);	/* close account file again */
	acctfd = -1;

	supportflags &= ~ACCTACTIVE;
}

/*
** get the number of exited processes written
** in the process-account file since the previous sample
*/
unsigned long
acctprocnt(void)
{
	struct stat	statacc;

	/*
 	** handle atopacctd-based process accounting on bases of
	** fixed-chunk shadow files
	*/
	if (maxshadowrec)	// atopacctd has been detected before?
	{
		unsigned long	numrecs = 0;
		long		newseq;
		char		shadowpath[128];
		FILE		*cfp;

		/*
		** determine the current size of the current shadow file
		** (fails if acctfd == -1) and determine if the file
		** has not been deleted by stopping the atopacct service
		*/
		if (fstat(acctfd, &statacc) == -1 || statacc.st_nlink == 0)
		{
			/* close the previous obsolete shadow file */
			(void) close(acctfd);
	
			acctsize = 0;
	
			/* reacquire the current real acctfd */
			if (atopacctd(0))
				return 0;	// reaqcuire failed

			if (fstat(acctfd, &statacc) == -1)
				return 0;
		}

		/*
		** verify how many new processes are added to the current
		** shadow file
		*/
		numrecs = (statacc.st_size - acctsize) / acctrecsz;

		/*
		** verify if subsequent shadow files are involved
		** (i.e. if the current file is full)
		*/
		if (statacc.st_size / acctrecsz < maxshadowrec)
			return numrecs;

		/*
 		** more shadow files available
		** get current shadow file
		*/ 	
		snprintf(shadowpath, sizeof shadowpath, "%s/%s/%s",
					pacctdir, PACCTSHADOWD, PACCTSHADOWC);

		if ( (cfp = fopen(shadowpath, "r")) )
		{
			if (fscanf(cfp, "%ld", &newseq) == 1)
			{
				fclose(cfp);
			}
			else
			{
				fclose(cfp);
				return numrecs;
			}
		}
		else
		{
			return numrecs;
		}

		if (newseq == curshadowseq)
			return numrecs;

		snprintf(shadowpath, sizeof shadowpath, PACCTSHADOWF,
					pacctdir, PACCTSHADOWD, newseq);

		/*
		** determine the size of newest shadow file
		*/
		if (stat(shadowpath, &statacc) == -1)
		{
			fprintf(stderr, "failed to stat the size of newest shadow file\n");
			return numrecs;
		}

		/*
		** Check cases like statacc.st_size / acctrecsz > maxshadowrec,
		** and atopacctd restarts at the same time, now newseq is zero.
		** Omit this interval's statistics by returning zero.
		*/
		if (newseq > curshadowseq)
		{
			numrecs += ((newseq - curshadowseq - 1) * maxshadowrec) +
				   (statacc.st_size / acctrecsz);
		}
		else
		{
			numrecs = 0;
		}

		return numrecs;
	}
	else
	/*
 	** classic process accounting on bases of directly opened
	** process accounting file
	*/
	{
		/*
 		** determine size of current process accounting file
		*/
		if (acctfd == -1 || fstat(acctfd, &statacc) == -1)
			return 0;

		/*
 		** accounting reset?
		*/
		if (acctsize > statacc.st_size)
		{
			/*
			** reposition to start of file
			*/
			(void) lseek(acctfd, 0, SEEK_SET);
			acctsize = 0;
		}

		/*
 		** using account file managed by (ps)acct package?
		*/
		if (pacctcur)	
		{
			/*
			** check inode of the current file and compare this
			** with the inode of the opened file; if not equal,
			** a file rotation has taken place and the size of
			** the new file has to be added
			*/
			if ( stat(pacctcur->name, &(pacctcur->stat)) == 0)
			{
				if (statacc.st_ino != pacctcur->stat.st_ino)
				{
					return (statacc.st_size - acctsize +
						pacctcur->stat.st_size) /
						acctrecsz;
				}
			}
		}

		return (statacc.st_size - acctsize) / acctrecsz;
	}
}

/*
** reposition the seek offset in the process accounting file to skip
** processes that have not been read
*/
void
acctrepos(unsigned int noverflow)
{
	/*
	** if accounting not supported, skip call
	*/
	if (acctfd == -1)
		return;

	if (maxshadowrec)
	{
		int	i;
		off_t	virtoffset  = acctsize + noverflow * acctrecsz;
		off_t	maxshadowsz = maxshadowrec * acctrecsz;
		long	switches    = virtoffset / maxshadowsz;
		acctsize            = virtoffset % maxshadowsz;

		for (i=0; i < switches; i++)
			switchshadow();

		(void) lseek(acctfd, acctsize, SEEK_SET);
	}
	else
	{
		/*
		** just reposition to skip superfluous records
		*/
		(void) lseek(acctfd, noverflow * acctrecsz, SEEK_CUR);
		acctsize += noverflow * acctrecsz;

		/*
		** when the new seek pointer is beyond the current file size
		** and reading from a process accounting file written by
		** the (ps)acct package, a logrotation might have taken place
		*/
		if (pacctcur)
		{
			struct stat	statacc;

			/*
			** determine the size of the current accounting file
			*/
			if (fstat(acctfd, &statacc) == -1)
				return;

			/*
			** seek pointer beyond file size and rotated to
			** new account file?
			*/
			if (acctsize > statacc.st_size &&
			    statacc.st_ino != pacctcur->stat.st_ino)
			{
				/*
 				** - close old file
 				** - open new file
 				** - adapt acctsize to actual offset in new file
 				**   and seek to that offset
				*/
				(void) close(acctfd);

				if ( (acctfd = open(pacctcur->name,
						    O_RDONLY) ) == -1)
					return;   // open failed

				acctsize = acctsize - statacc.st_size;
				(void) lseek(acctfd, acctsize, SEEK_SET);

				if (fstat(acctfd, &statacc) == -1)
					return;   // no new inode info
			}
		}
	}
}


/*
** read the process records from the process accounting file,
** that are written since the previous cycle
*/
unsigned long
acctphotoproc(struct tstat *accproc, int nrprocs)
{
	register int 		nrexit;
	register struct tstat 	*api;
	struct acct 		acctrec;
	struct acct_v3 		acctrec_v3;
	struct stat		statacc;
	int			filled;

	/*
	** if accounting not supported, skip call
	*/
	if (acctfd == -1)
		return 0;

	/*
	** determine the size of the (current) account file
	*/
	if (fstat(acctfd, &statacc) == -1)
		return 0;

	/*
	** check all exited processes in accounting file
	*/
	for  (nrexit=0, api=accproc; nrexit < nrprocs; )
	{
		/*
		** in case of shadow accounting files, we might have to
		** switch from the current accounting file to the next
		*/
		if (maxshadowrec && acctsize >= statacc.st_size)
		{
			switchshadow();

			/*
			** determine the size of the new (current) account file
			** and initialize the current offset within that file
			*/
			if (fstat(acctfd, &statacc) == -1)
				return 0;

			acctsize  = 0;
		}

		/*
 		** in case of account file managed by (ps)acct package,
		** be aware that a switch to a newer logfile might have
		** to be done
		*/
		if (pacctcur && acctsize >= statacc.st_size)
		{
			/*
			** check inode of the current file and compare this
			** with the inode of the opened file; if not equal,
			** a file rotation has taken place and the file
			** has to be reopened
			*/
			if ( stat(pacctcur->name, &(pacctcur->stat)) == 0)
			{
				if (statacc.st_ino != pacctcur->stat.st_ino)
				{
					(void) close(acctfd);

					if ( (acctfd = open(pacctcur->name,
							    O_RDONLY) ) == -1)
						return 0; // open failed

					if (fstat(acctfd, &statacc) == -1)
						return 0; // no new inode info

					acctsize  = 0;    // reset size new file
				}
			}
		}

		/*
		** read the next record
		*/
		switch (acctversion)
		{
		   case 2:
			if ( read(acctfd, &acctrec, acctrecsz) < acctrecsz )
				break;	/* unexpected end of account file */

			/*
			** fill process info from accounting-record
			*/
			api->gen.state  = 'E';
			api->gen.nthr   = 1;
			api->gen.isproc = 1;
			api->gen.pid    = 0;
			api->gen.tgid   = 0;
			api->gen.ppid   = 0;
			api->gen.excode = acctrec.ac_exitcode;
			api->gen.ruid   = acctrec.ac_uid16;
			api->gen.rgid   = acctrec.ac_gid16;
			api->gen.btime  = acctrec.ac_btime;
			api->gen.elaps  = acctrec.ac_etime;
			api->cpu.stime  = acctexp(acctrec.ac_stime);
			api->cpu.utime  = acctexp(acctrec.ac_utime);
			api->mem.minflt = acctexp(acctrec.ac_minflt);
			api->mem.majflt = acctexp(acctrec.ac_majflt);
			api->dsk.rio    = acctexp(acctrec.ac_rw);

			strncpy(api->gen.name, acctrec.ac_comm, PNAMLEN);
			api->gen.name[PNAMLEN] = '\0';
			filled = 1;
			break;

		   case 3:
			if ( read(acctfd, &acctrec_v3, acctrecsz) < acctrecsz )
				break;	/* unexpected end of account file */

			/*
			** fill process info from accounting-record
			*/
			api->gen.state  = 'E';
			api->gen.pid    = acctrec_v3.ac_pid;
			api->gen.tgid   = acctrec_v3.ac_pid;
			api->gen.ppid   = acctrec_v3.ac_ppid;
			api->gen.nthr   = 1;
			api->gen.isproc = 1;
			api->gen.excode = acctrec_v3.ac_exitcode;
			api->gen.ruid   = acctrec_v3.ac_uid;
			api->gen.rgid   = acctrec_v3.ac_gid;
			api->gen.btime  = acctrec_v3.ac_btime;
			api->gen.elaps  = acctrec_v3.ac_etime;
			api->cpu.stime  = acctexp(acctrec_v3.ac_stime);
			api->cpu.utime  = acctexp(acctrec_v3.ac_utime);
			api->mem.minflt = acctexp(acctrec_v3.ac_minflt);
			api->mem.majflt = acctexp(acctrec_v3.ac_majflt);
			api->dsk.rio    = acctexp(acctrec_v3.ac_rw);

			strncpy(api->gen.name, acctrec_v3.ac_comm, PNAMLEN);
			api->gen.name[PNAMLEN] = '\0';
			filled = 1;
			break;
		}

		if (filled == 1) {
			nrexit++;
			api++;
			acctsize += acctrecsz;

			filled = 0;
		}
	}

	if (acctsize > ACCTMAXFILESZ && !maxshadowrec)
		acctrestarttrial();

	return nrexit;
}

/*
** when the size of the private accounting file exceeds a certain limit,
** it might be useful to stop process accounting, truncate the
** process accounting file to zero and start process accounting again
**
** this will only be done if this atop process is the only one
** that is currently using the accounting file
*/
static void
acctrestarttrial(void)
{
	struct stat 	statacc;
	int		sematopid;

	/*
	** not private accounting-file in use?
	*/
	if (!acctatop)
		return;		// do not restart

	/*
	** still remaining records in accounting file that are
	** written between the moment that the number of exited
	** processes was counted and the moment that all processes
	** were read
	*/
	(void) fstat(acctfd, &statacc);

	if (acctsize != statacc.st_size)
		return;		// do not restart

	/*
	** claim the semaphore to get exclusive rights for
	** the accounting-manipulation
	*/
	sematopid = semget(ATOPACCTKEY, 0, 0);

	(void) semop(sematopid, &semclaim, 1);

	/*
	** check if there are more users of accounting file
	*/
	if (semctl(sematopid, 1, GETVAL, 0) < ATOPACCTTOT-1)
	{
		(void) semop(sematopid, &semrelse, 1);
		return;		// do not restart
	}

	/*
	** restart is possible
	**
	** - switch off accounting
	** - truncate the file
	** - switch on accounting
	*/
	regainrootprivs();	// get root privs again 

	(void) acct(0);		// switch off accounting

	if ( truncate(ACCTDIR "/" ACCTFILE, 0) == 0)
		(void) lseek(acctfd, 0, SEEK_SET);

	(void) acct(ACCTDIR "/" ACCTFILE);

	if (! droprootprivs() )
		mcleanstop(42, "failed to drop root privs\n");

	acctsize = 0;

	(void) semop(sematopid, &semrelse, 1);
}

/*
** expand the special compression-methods
*/
static count_t
acctexp(comp_t ct)
{
        count_t	exp;
        count_t val;

        exp = (ct >> 13) & 0x7;		/* obtain  3 bits base-8 exponent */
        val =  ct & 0x1fff;		/* obtain 13 bits mantissa        */

        while (exp-- > 0)
                val <<= 3;

        return val;
}

/*
** switch to the next accounting shadow file
*/
static void
switchshadow(void)
{
	int		tmpfd;
	char		shadowpath[128];
	struct flock	flock;

	/*
	** determine path name of new shadow file
	*/
	curshadowseq++;

	snprintf(shadowpath, sizeof shadowpath, PACCTSHADOWF,
					pacctdir, PACCTSHADOWD, curshadowseq);

	/*
	** open new shadow file, while the previous is also kept open
	** (to keep the read lock until a read lock is set for the new
	** shadow file)
	*/
	if ( (tmpfd = open(shadowpath, O_RDONLY)) != -1)
	{
		/*
		** set read lock on new shadow file
		*/
        	flock.l_type    = F_RDLCK;
        	flock.l_whence  = SEEK_SET;
        	flock.l_start   = 0;
        	flock.l_len     = 1;

                if ( fcntl(tmpfd, F_SETLK, &flock) != -1)
		{
			(void) close(acctfd);	// implicit release read lock
			acctfd = tmpfd;
			return;
		}
		else
		{
			(void) close(tmpfd);
		}
	}
}

/*
** handle the option 'pacctdir' in the /etc/atoprc file
*/
void
do_pacctdir(char *tagname, char *tagvalue)
{
	char		shadowpath[128];
	struct stat	dirstat;

	/*
	** copy the directory pathname to an own buffer
	*/
	if ( (pacctdir = malloc(strlen(tagvalue)+1)) == NULL )
	{
		perror("malloc pacctdir");
		exit(11);
	}

	strcpy(pacctdir, tagvalue);

	/*
	** verify if the atopacctd daemon is active
	*/
	if ( semget(PACCTPUBKEY, 0, 0) == -1)
	{
		fprintf(stderr, "Warning: option '%s' specified "
		                "while atopacctd not running!\n", tagname);
		sleep(2);
		return;
	}

	/*
	** verify that the topdirectory exists
	*/
        if ( stat(pacctdir, &dirstat) == -1 )
        {
		fprintf(stderr, "Warning: option '%s' specified - ", tagname);
		perror(pacctdir);
                sleep(2);
		return;
        }

        if (! S_ISDIR(dirstat.st_mode) )
        {
		fprintf(stderr, "Warning: option '%s' specified - ", tagname);
                fprintf(stderr, "%s not a directory\n", pacctdir);
                sleep(2);
		return;
        }

	/*
	** verify that the topdirectory contains the required subdirectory
	*/
	snprintf(shadowpath, sizeof shadowpath, "%s/%s",
						pacctdir, PACCTSHADOWD);

        if ( stat(shadowpath, &dirstat) == -1 )
        {
		fprintf(stderr, "Warning: option '%s' specified - ", tagname);
		perror(shadowpath);
                sleep(2);
		return;
        }

        if (! S_ISDIR(dirstat.st_mode) )
        {
		fprintf(stderr, "Warning: option '%s' specified - ", tagname);
                fprintf(stderr, "%s not a directory\n", shadowpath);
                sleep(2);
		return;
        }
}
