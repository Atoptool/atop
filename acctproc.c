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
**
** $Log: acctproc.c,v $
** Revision 1.28  2010/04/23 12:20:19  gerlof
** Modified mail-address in header.
**
** Revision 1.27  2009/12/12 10:12:01  gerlof
** Register and display end date and end time for process.
**
** Revision 1.26  2008/03/06 08:37:25  gerlof
** Register/show ppid of a process.
**
** Revision 1.25  2008/01/14 09:22:23  gerlof
** Support for environment variable ATOPACCT to specify the name of a
** particular process accouting file.
**
** Revision 1.24  2007/08/17 08:50:26  gerlof
** Verify if private accounting used before switching off accounting.
**
** Revision 1.23  2007/03/20 13:01:51  gerlof
** Introduction of variable supportflags.
**
** Revision 1.22  2007/02/13 09:14:49  gerlof
** New boolean introduced to indicate if accounting is active.
**
** Revision 1.21  2006/02/07 06:46:15  gerlof
** Removed swap-counter.
**
** Revision 1.20  2005/11/04 14:17:10  gerlof
** Improved recognition of certain version process accounting file.
**
** Revision 1.19  2005/10/31 12:44:58  gerlof
** Support account-record version 3 (used by Mandriva).
**
** Revision 1.18  2005/10/31 08:55:05  root
** *** empty log message ***
**
** Revision 1.17  2004/12/14 15:05:00  gerlof
** Implementation of patch-recognition for disk and network-statistics.
**
** Revision 1.16  2004/06/01 11:57:22  gerlof
** Consider other standard accounting-files, i.e. /var/account/pacct.
**
** Revision 1.15  2004/05/06 09:48:21  gerlof
** Ported to kernel-version 2.6.
**
** Revision 1.14  2003/07/07 09:26:15  gerlof
** Cleanup code (-Wall proof).
**
** Revision 1.13  2003/07/02 06:43:11  gerlof
** Modified include-file sys/acct.h to linux/acct.h to make it
** work on Alpha-based systems as well.
**
** Revision 1.12  2003/06/27 12:31:24  gerlof
** Adapt long to long long.
**
** Revision 1.11  2003/04/03 08:32:58  gerlof
** Cosmetic changes.
**
** Revision 1.10  2003/01/14 09:01:45  gerlof
** Small cosmetic changes.
**
** Revision 1.9  2002/10/03 11:12:03  gerlof
** Modify (effective) uid/gid to real uid/gid.
**
** Revision 1.8  2002/07/24 11:11:12  gerlof
** Redesigned to ease porting to other UNIX-platforms.
**
** Revision 1.7  2002/07/11 07:28:29  root
** Some additions related to secure accounting file handling
**
** Revision 1.6  2002/07/08 09:14:54  root
** Modified secure handling of accounting file
** (inspired by Tobias Rittweiler).
**
** Revision 1.5  2001/11/07 09:16:55  gerlof
** Allow users to run atop without process-accounting switched on.
**
** Revision 1.4  2001/10/16 06:15:52  gerlof
** Partly redesigned.
**
** Revision 1.3  2001/10/04 13:02:24  gerlof
** Redesign of the way to determine how many atop's are using process-accounting
** on basis of semaphores.
**
** Revision 1.2  2001/10/04 08:46:46  gerlof
** Improved verification of kernel-symbol addresses
**
** Revision 1.1  2001/10/02 10:38:35  gerlof
** Initial revision
**
*/

#define	_FILE_OFFSET_BITS	64

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>

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
static void	acctrestarttrial();
static void	switchshadow(void);

struct pacctadm {
	char		*name;
	struct stat	stat;
} pacctadm[] = {
	{ "/var/log/pacct",		{0, }, },
	{ "/var/account/pacct",		{0, }, }
};

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

struct sembuf	semclaim = {0, -1, SEM_UNDO},
		semrelse = {0, +1, SEM_UNDO},
		semdecre = {1, -1, SEM_UNDO},
		semincre = {1, +1, SEM_UNDO};

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
	int			i, j, sematopid, sempacctpubid;
	static ushort 		vals[] = {1, ATOPACCTTOT};
	union {ushort *array;}	arg = {vals};
	struct stat		statbuf;
	char			*ep;

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
	if ( (sempacctpubid = semget(PACCTPUBKEY, 0, 0)) != -1)
	{
		FILE 			*cfp;
		char    		shadowpath[128];
        	struct flock            flock;

		if (! droprootprivs() )
			mcleanstop(42, "failed to drop root privs\n");

		(void) semop(sempacctpubid, &semclaim, 1);

		snprintf(shadowpath, sizeof shadowpath, "%s/%s/%s",
					pacctdir, PACCTSHADOWD, PACCTSHADOWC);

		if ( (cfp = fopen(shadowpath, "r")) )
		{
			if (fscanf(cfp, "%ld/%lu",
					&curshadowseq, &maxshadowrec) == 2)
			{
				fclose(cfp);

				snprintf(shadowpath, sizeof shadowpath,
						PACCTSHADOWF, pacctdir,
						PACCTSHADOWD, curshadowseq);

				if ( (acctfd = open(shadowpath, O_RDONLY))!=-1)
				{
					if ( !acctvers(acctfd) )
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

							regainrootprivs();
							return 1;
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
						return 0;
                			}

                       			(void) close(acctfd);
				}
			}
			else
			{
				fclose(cfp);
				maxshadowrec = 0;
			}
		}

		(void) semop(sempacctpubid, &semrelse, 1);
	}

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
	** if accounting not supported, skip call
	*/
	if (acctfd == -1)
		return 0;

	/*
	** determine the current size of the accounting file
	*/
	if (fstat(acctfd, &statacc) == -1)
		return 0;

	/*
 	** handle atopacctd-based process accounting on bases of
	** fixed-chunk shadow files
	*/
	if (maxshadowrec)
	{
		unsigned long	numrecs = 0;
		long		newseq;
		char		shadowpath[128];
		FILE		*cfp;

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
			return numrecs;

		numrecs += ((newseq - curshadowseq - 1) * maxshadowrec) +
		           (statacc.st_size / acctrecsz);

		return numrecs;
	}
	else
	/*
 	** classic process accounting on bases of directly opened
	** process accounting file
	*/
	{
		if (acctsize > statacc.st_size)	/* accounting reset? */
		{
			/*
			** reposition to start of file
			*/
			(void) lseek(acctfd, 0, SEEK_SET);
			acctsize = 0;
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
		acctsize   += noverflow * acctrecsz;
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

	/*
	** if accounting not supported, skip call
	*/
	if (acctfd == -1)
		return 0;

	/*
	** determine the size of the (current) account file
	** and the current offset within that file
	*/
	if (fstat(acctfd, &statacc) == -1)
		return 0;

	/*
	** check all exited processes in accounting file
	*/
	for  (nrexit=0, api=accproc; nrexit < nrprocs;
				nrexit++, api++, acctsize += acctrecsz)
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

			strcpy(api->gen.name, acctrec.ac_comm);
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

			strcpy(api->gen.name, acctrec_v3.ac_comm);
			break;
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
acctrestarttrial()
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
