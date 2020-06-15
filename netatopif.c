/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains functions to interface with the netatop
** module in the kernel. That module keeps track of network activity
** per process and thread.
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        August/September 2012
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

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <zlib.h>
#include <sys/mman.h>

#include "atop.h"
#include "photoproc.h"

#include "netatop.h"
#include "netatopd.h"

static int		netsock   = -1;
static int		netexitfd = -1;
static struct naheader	*nahp;
static int		semid     = -1;
static unsigned long	lastseq;

/*
** storage of last exited tasks read from exitfile
** every exitstore struct is registered in hash buckets,
** by its pid or by its begin time
*/
struct exitstore {
	struct exitstore  *next;
	unsigned char      isused;
	struct netpertask  npt;
};

#define NHASH		1024 	// must be power of two!
#define HASHCALC(x)	((x)&(NHASH-1))

static struct exitstore	*esbucket[NHASH];

static struct exitstore *exitall;
static int		 exitnum;
static char		 exithash;

static void	fill_networkcnt(struct tstat *, struct tstat *,
        	                struct exitstore *);

/*
** open a raw socket to the IP layer (root privs required)
*/
void
netatop_ipopen(void)
{
	netsock = socket(PF_INET, SOCK_RAW, IPPROTO_RAW);
}

/*
** check if at this moment the netatop kernel module is loaded and
** the netatopd daemon is active
*/
void
netatop_probe(void)
{
	struct sembuf   	semdecr = {1, -1, SEM_UNDO};
	socklen_t		sl = 0;
	struct stat		exstat;

	/*
 	** check if IP socket is open
	*/
	if (netsock == -1)
		return;

	/*
	** probe if the netatop module is active
	*/
	if ( getsockopt(netsock, SOL_IP, NETATOP_PROBE, NULL, &sl) != 0)
	{
		supportflags &= ~NETATOP;
		supportflags &= ~NETATOPD;
		return;
	}

	// set appropriate support flag
	supportflags |= NETATOP;

	/*
	** check if the netatopd daemon is active to register exited tasks
	** and decrement semaphore to indicate that we want to subscribe
	*/
	if (semid == -1)
	{
		if ( (semid = semget(SEMAKEY, 0, 0))    == -1 ||
	                      semop(semid, &semdecr, 1) == -1   )
		{
			supportflags &= ~NETATOPD;
			return;
		}
	}

	if (semctl(semid, 0, GETVAL, 0) != 1)
	{
		supportflags &= ~NETATOPD;
		return;
	}

	/*
	** check if exitfile still open and not removed by netatopd
	*/
	if (netexitfd != -1)
	{
		if ( fstat(netexitfd, &exstat) == 0 && 
		     exstat.st_nlink > 0              ) // not removed
		{
			supportflags |= NETATOPD;
			return;
		}
		else
		{
			(void) close(netexitfd);

			if (nahp)
				munmap(nahp, sizeof *nahp);

			netexitfd = -1;
			nahp = NULL;
		}
	}

	/*
	** open file with compressed stats of exited tasks
	** and (re)mmap the start record, mainly to obtain current sequence
	*/
	if (netexitfd == -1)
	{
		if ( (netexitfd = open(NETEXITFILE, O_RDONLY, 0)) == -1)
		{
			supportflags &= ~NETATOPD;
               		return;
		}
	}

	if ( (nahp = mmap((void *)0, sizeof *nahp, PROT_READ, MAP_SHARED,
						netexitfd, 0)) == (void *) -1)
	{
        	(void) close(netexitfd);
		netexitfd = -1;
		nahp = NULL;
		supportflags &= ~NETATOPD;
		return;
	}

	/*
	** if this is a new incarnation of the netatopd daemon,
	** position seek pointer on first task that is relevant to us
	** and remember last sequence number to know where to start
	*/
	(void) lseek(netexitfd, 0, SEEK_END);

	lastseq     = nahp->curseq;

	// set appropriate support flag
	supportflags |= NETATOPD;
}

void
netatop_signoff(void)
{
	struct sembuf  	semincr = {1, +1, SEM_UNDO};

	if (netsock == -1 || nahp == NULL)
		return;

	if (supportflags & NETATOPD)
	{
        	regainrootprivs();
		
	        (void) semop(semid, &semincr, 1);

		kill(nahp->mypid, SIGHUP);

       		if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");

		(void) munmap(nahp, sizeof *nahp);
		(void) close(netexitfd);
	}
}

/*
** read network counters for one existing task
** (type 'g' for thread group or type 't' for thread)
*/
void
netatop_gettask(pid_t id, char type, struct tstat *tp)
{
	struct netpertask	npt;
	socklen_t		socklen = sizeof npt;
	int 			cmd = (type == 'g' ?
				    NETATOP_GETCNT_TGID : NETATOP_GETCNT_PID);

	/*
	** if kernel module netatop not active on this system, skip call
	*/
	if (!(supportflags & NETATOP) ) {
		memset(&tp->net, 0, sizeof tp->net);
		return;
	}

	/*
 	** get statistics of this process/thread
	*/
	npt.id	= id;

        regainrootprivs();

	if (getsockopt(netsock, SOL_IP, cmd, &npt, &socklen) != 0) {
		memset(&tp->net, 0, sizeof tp->net);

        	if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");

		if (errno == ENOPROTOOPT || errno == EPERM)
		{
			supportflags &= ~NETATOP;
			supportflags &= ~NETATOPD;
			close(netsock);
			netsock = -1;
		}

		return;
	}

       	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

	/*
	** statistics available: fill counters
	*/
	tp->net.tcpsnd = npt.tc.tcpsndpacks;
	tp->net.tcprcv = npt.tc.tcprcvpacks;
	tp->net.tcpssz = npt.tc.tcpsndbytes;
	tp->net.tcprsz = npt.tc.tcprcvbytes;

	tp->net.udpsnd = npt.tc.udpsndpacks;
	tp->net.udprcv = npt.tc.udprcvpacks;
	tp->net.udpssz = npt.tc.udpsndbytes;
	tp->net.udprsz = npt.tc.udprcvbytes;
}

/*
** read all exited processes that have been added to the exitfile
** and store them into memory
*/
unsigned int
netatop_exitstore(void)
{
	socklen_t		socklen = 0, nexitnet, sz, nr=0;
	unsigned long		uncomplen;
	unsigned char		nextsize;
	unsigned char		readbuf[nahp->ntplen+100];
	unsigned char		databuf[nahp->ntplen];
	struct netpertask	*tmp = (struct netpertask *)databuf;
	struct exitstore	*esp;

        regainrootprivs();

	/*
	** force garbage collection:
	**   netatop module builds new list of exited processes that
	**   can be read by netatopd and written to exitfile
	*/
	if (getsockopt(netsock, SOL_IP, NETATOP_FORCE_GC, NULL, &socklen)!=0) {
        	if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");

		if (errno == ENOPROTOOPT || errno == EPERM)
		{
			supportflags &= ~NETATOP;
			supportflags &= ~NETATOPD;
			close(netsock);
			netsock = -1;
		}

		return 0;
	}

	/*
 	** wait until list of exited processes is read by netatopd
	** and available to be read by atop
	*/
	if (getsockopt(netsock, SOL_IP, NETATOP_EMPTY_EXIT, 0, &socklen) !=0) {
        	if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");

		if (errno == ENOPROTOOPT || errno == EPERM)
		{
			supportflags &= ~NETATOP;
			supportflags &= ~NETATOPD;
			close(netsock);
			netsock = -1;
		}

		return 0;
	}

       	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

	/*
	** verify how many exited processes are available to be read
	** from the exitfile
	*/
	nexitnet = nahp->curseq - lastseq;
	lastseq  = nahp->curseq;

	if (nexitnet == 0)
		return 0;

	/*
	** allocate storage for all exited processes
	*/
	exitall = malloc(nexitnet * sizeof(struct exitstore));

	ptrverify(exitall, "Malloc failed for %d exited netprocs\n", nexitnet);

	memset(exitall, 0, nexitnet * sizeof(struct exitstore));

	esp = exitall;

	/*
	** read next byte from exitfile that specifies the length
	** of the next record
	*/
	if ( read(netexitfd, &nextsize, 1) != 1) 
		return 0;

	/*
	** read the next record and (if possible) the byte specifying
	** the size of the next record
	*/
	while ( (sz = read(netexitfd, readbuf, nextsize+1)) >= nextsize)
	{
		/*
		** decompress record and store it
		*/
        	uncomplen = nahp->ntplen;

		if (nahp->ntplen <= sizeof(struct netpertask))
		{
			(void) uncompress((Byte *)&(esp->npt), &uncomplen,
							readbuf, nextsize);
		}
		else
		{
			(void) uncompress((Byte *)databuf, &uncomplen,
							readbuf, nextsize);
			esp->npt = *tmp;
		}

		esp++;
		nr++;

		/*
		** check if we have read all records
		*/
		if (nr == nexitnet)
		{
			/*
			** if we have read one byte too many:
			** reposition seek pointer 
			*/
			if (sz > nextsize)
				(void) lseek(netexitfd, -1, SEEK_CUR);

			break;
		}

		/*
		** prepare reading next record
		*/
		if (sz > nextsize)
			nextsize = readbuf[nextsize];
		else
			break;	// unexpected: more requested than available
	}

	exitnum = nr;

	return nr;
}

/*
** remove all stored exited processes from the hash bucket list
*/
void
netatop_exiterase(void)
{
	free(exitall);
	memset(esbucket, 0, sizeof esbucket);
	exitnum = 0;
}

/*
** add all stored tasks to a hash bucket, either
** by pid (argument 'p') or by begin time (argument 'b')
*/
void
netatop_exithash(char hashtype)
{
	int			i, h;
	struct exitstore	*esp;

	for (i=0, esp=exitall; i < exitnum; i++, esp++)
	{
		if (hashtype == 'p')
			h = HASHCALC(esp->npt.id);
		else
			h = HASHCALC(esp->npt.btime);

		esp->next   = esbucket[h];
		esbucket[h] = esp;
	}

	exithash = hashtype;
}

/*
** search for relevant exited network task and
** update counters in tstat struct
*/
void
netatop_exitfind(unsigned long key, struct tstat *dev, struct tstat *pre)
{
	int 			h = HASHCALC(key);
	struct exitstore	*esp;

	/*
	** if bucket empty, forget about it
	*/
	if ( (esp = esbucket[h]) == NULL)
		return;

	/*
	** search thru hash bucket list
	*/
	for (; esp; esp=esp->next)
	{
		switch (exithash)
		{
		   case 'p':		// search by PID
			if (key != esp->npt.id)
				continue;

			/*
			** correct PID found
			*/
			fill_networkcnt(dev, pre, esp);
			break;

		   case 'b':		// search by begin time
			if (esp->isused)
				continue;

			if (key != esp->npt.btime)
				continue;

			/*
			** btime is okay; additional checks required
			*/
			if ( strcmp(esp->npt.command, pre->gen.name) != 0)
				continue;

			if (esp->npt.tc.tcpsndpacks < pre->net.tcpsnd 	||
			    esp->npt.tc.tcpsndbytes < pre->net.tcpssz 	||
			    esp->npt.tc.tcprcvpacks < pre->net.tcprcv 	||
			    esp->npt.tc.tcprcvbytes < pre->net.tcprsz 	||
			    esp->npt.tc.udpsndpacks < pre->net.udpsnd 	||
			    esp->npt.tc.udpsndbytes < pre->net.udpssz 	||
			    esp->npt.tc.udprcvpacks < pre->net.udprcv 	||
			    esp->npt.tc.udprcvbytes < pre->net.udprsz 	  )
				continue;

			esp->isused = 1;
 
			fill_networkcnt(dev, pre, esp);
			break;
		}
	}
}

static void
fill_networkcnt(struct tstat *dev, struct tstat *pre, struct exitstore *esp)
{
	dev->net.tcpsnd	= esp->npt.tc.tcpsndpacks - pre->net.tcpsnd;
	dev->net.tcpssz	= esp->npt.tc.tcpsndbytes - pre->net.tcpssz;
	dev->net.tcprcv	= esp->npt.tc.tcprcvpacks - pre->net.tcprcv;
	dev->net.tcprsz	= esp->npt.tc.tcprcvbytes - pre->net.tcprsz;

	dev->net.udpsnd	= esp->npt.tc.udpsndpacks - pre->net.udpsnd;
	dev->net.udpssz	= esp->npt.tc.udpsndbytes - pre->net.udpssz;
	dev->net.udprcv	= esp->npt.tc.udprcvpacks - pre->net.udprcv;
	dev->net.udprsz	= esp->npt.tc.udprcvbytes - pre->net.udprsz;
}
