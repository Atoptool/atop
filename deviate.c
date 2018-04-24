/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains functions to calculate the differences for
** the system-level and process-level counters since the previous sample.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
** --------------------------------------------------------------------------
** Copyright (C) 2000-2010 Gerlof Langeveld
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
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
** --------------------------------------------------------------------------
**
** $Log: deviate.c,v $
** Revision 1.45  2010/10/23 14:02:03  gerlof
** Show counters for total number of running and sleep (S and D) threads.
**
** Revision 1.44  2010/05/18 19:19:43  gerlof
** Introduce CPU frequency and scaling (JC van Winkel).
**
** Revision 1.43  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.42  2010/03/04 10:52:08  gerlof
** Support I/O-statistics on logical volumes and MD devices.
**
** Revision 1.41  2009/12/31 11:34:21  gerlof
** Sanity-check to bypass kernel-bug showing 497 days of CPU-consumption.
**
** Revision 1.40  2009/12/17 11:58:25  gerlof
** Gather and display new counters: dirty cache and guest cpu usage.
**
** Revision 1.39  2008/02/25 14:51:18  gerlof
** Experimental code for HTTP-statistics.
**
** Revision 1.38  2008/01/07 11:33:43  gerlof
** Cosmetic changes.
**
** Revision 1.37  2008/01/07 10:17:24  gerlof
** Implement possibility to make summaries.
**
** Revision 1.36  2007/11/05 12:13:16  gerlof
** Match processes not only on pid, but also on start time.
**
** Revision 1.35  2007/11/05 11:42:47  gerlof
** Bug-solution for new-process indicator on 64-bits machines.
**
** Revision 1.34  2007/08/17 09:44:59  gerlof
** Experimental: gather info about HTTP statistics.
**
** Revision 1.33  2007/08/16 11:59:32  gerlof
** Add support for atopsar reporting.
** Concerns addition of lots of counters.
**
** Revision 1.32  2007/07/03 09:01:07  gerlof
** Support Apache-statistics.
**
** Revision 1.31  2007/03/20 13:02:03  gerlof
** Introduction of variable supportflags.
**
** Revision 1.30  2007/03/20 11:18:57  gerlof
** Add counter for cancelled writes.
**
** Revision 1.29  2007/02/13 09:21:04  gerlof
** Removed external declarations.
**
** Revision 1.28  2007/01/22 08:28:18  gerlof
** Support steal-time from /proc/stat.
**
** Revision 1.27  2007/01/18 10:43:18  gerlof
** Support for network-interface busy-percentage (speed and duplex).
**
** Revision 1.26  2006/11/13 13:47:26  gerlof
** Implement load-average counters, context-switches and interrupts.
**
** Revision 1.25  2006/02/07 06:45:33  gerlof
** Removed swap-counter.
**
** Revision 1.24  2006/01/30 09:13:33  gerlof
** Extend memory counters (a.o. page scans).
**
** Revision 1.23  2005/10/31 12:45:29  gerlof
** Support account-record version 3 (used by Mandriva).
**
** Revision 1.22  2005/10/21 09:49:38  gerlof
** Per-user accumulation of resource consumption.
**
** Revision 1.21  2004/12/14 15:05:47  gerlof
** Implementation of patch-recognition for disk and network-statistics.
**
** Revision 1.20  2004/10/28 08:30:51  gerlof
** New counter: vm committed space
**
** Revision 1.19  2004/09/24 10:02:01  gerlof
** Wrong cpu-numbers for system level statistics.
**
** Revision 1.18  2004/09/02 10:49:18  gerlof
** Added sleep-average to process-info.
**
** Revision 1.17  2004/08/31 13:27:04  gerlof
** Add new info for threading.
**
** Revision 1.16  2004/05/07 05:49:40  gerlof
** *** empty log message ***
**
** Revision 1.15  2004/05/06 09:46:55  gerlof
** Ported to kernel-version 2.6.
**
** Revision 1.14  2003/07/07 09:26:33  gerlof
** Cleanup code (-Wall proof).
**
** Revision 1.13  2003/07/03 11:17:49  gerlof
** Corrected calculations for exited processes.
**
** Revision 1.12  2003/06/30 11:30:57  gerlof
** Enlarge counters to 'long long'.
**
** Revision 1.11  2003/06/24 06:21:12  gerlof
** Limit number of system resource lines.
**
** Revision 1.10  2003/01/24 14:20:16  gerlof
** If possible, also show commandline when process has exited.
**
** Revision 1.9  2002/09/16 08:58:08  gerlof
** Add indicator for newly created processes.
**
** Revision 1.8  2002/08/27 04:47:46  gerlof
** Minor comment updates.
**
** Revision 1.7  2002/07/24 11:12:20  gerlof
** Redesigned to ease porting to other UNIX-platforms.
**
** Revision 1.6  2002/07/10 04:59:37  root
** Counters pin/pout renamed to swin/swout (Linux conventions).
**
** Revision 1.5  2002/01/22 13:39:20  gerlof
** Support for number of cpu's.
**
** Revision 1.4  2001/11/22 08:33:10  gerlof
** Add priority per process.
**
** Revision 1.3  2001/11/07 09:18:22  gerlof
** Use /proc instead of /dev/kmem for process-level statistics.
**
** Revision 1.2  2001/10/03 08:58:41  gerlof
** Improved subtraction which is overflow-proof
**
** Revision 1.1  2001/10/02 10:43:23  gerlof
** Initial revision
**
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <memory.h>
#include <string.h>

#include "atop.h"
#include "ifprop.h"
#include "photoproc.h"
#include "photosyst.h"

#define	MAX32BITVAL	0x100000000LL

static void calcdiff(struct tstat *, struct tstat *, struct tstat *,
							char, count_t);

/*
** calculate the process-activity during the last sample
*/
void
deviattask(struct tstat    *curtpres, int ntaskpres,
           struct tstat    *curpexit, int nprocexit,
	   struct devtstat *devtstat,
	   struct sstat    *devsstat)
{
	register int		c, d, pall=0, pact=0;
	register struct tstat	*curstat, *devstat, *thisproc;
	struct tstat		prestat;
	struct pinfo		*pinfo;
	count_t			totusedcpu;
	char			hashtype = 'p';

	/*
	** needed for sanity check later on...
	*/
	totusedcpu	= devsstat->cpu.all.stime + devsstat->cpu.all.utime +
			  devsstat->cpu.all.ntime + devsstat->cpu.all.itime +
			  devsstat->cpu.all.wtime + devsstat->cpu.all.Itime +
			  devsstat->cpu.all.Stime + devsstat->cpu.all.steal;

	/*
	** make new list of all tasks in the task-database;
	** after handling all task, the left-overs are tasks
	** that have disappeared since the previous sample
	*/
	pdb_makeresidue();

	/*
 	** remove allocated lists of previous sample and initialize counters
	*/
	if (devtstat->taskall)
		free(devtstat->taskall);

	if (devtstat->procall)
		free(devtstat->procall);

	if (devtstat->procactive)
		free(devtstat->procactive);

	memset(devtstat, 0, sizeof *devtstat);

	/*
	** create list for the sample deviations of all tasks
	*/
 	devtstat->ntaskall = ntaskpres + nprocexit;
	devtstat->taskall  = malloc(devtstat->ntaskall * sizeof(struct tstat));

	ptrverify(devtstat->taskall, "Malloc failed for %d deviated tasks\n",
                                  devtstat->ntaskall);

	/*
	** calculate deviations per present task
	*/
	for (c=0, thisproc = devtstat->taskall; c < ntaskpres; c++)
	{
		char	newtask = 0;

		curstat = curtpres+c;
		devstat = devtstat->taskall+c;

		if (curstat->gen.isproc)
		{
			thisproc = devstat;	// remember last process seen

			devtstat->nprocall++;

			if (curstat->gen.state == 'Z')
			{
				devtstat->totzombie++;
			}
			else
			{
				devtstat->totrun   += curstat->gen.nthrrun;
				devtstat->totslpi  += curstat->gen.nthrslpi;
				devtstat->totslpu  += curstat->gen.nthrslpu;
			}
		}

		/*
		** get previous figures from task-database
		*/
		if ( pdb_gettask(curstat->gen.pid, curstat->gen.isproc,
		                 curstat->gen.btime, &pinfo))
		{
			/*
			** task already present in the previous sample
			**
			** save stats from previous sample (to use for
			** further calculations) and store new statistics
			** in task-database
			*/
			if (memcmp(curstat, &pinfo->tstat, 
					           sizeof(struct tstat)) == EQ)
			{
				/*
 				** no activity for task
				*/
				curstat->gen.wasinactive = 1;
			}
 			else
			{
				/*
 				** save the values of the previous sample
				** and overwrite the previous sample in
				** the database with the current sample
				*/
				prestat 	= pinfo->tstat;
				pinfo->tstat 	= *curstat;

				curstat->gen.wasinactive = 0;

				devtstat->ntaskactive++;

				if (curstat->gen.isproc)
				{
					devtstat->nprocactive++;
				}
				else
				{
					if (thisproc->gen.wasinactive)
					{
						thisproc->gen.wasinactive = 0;
						devtstat->ntaskactive++;
						devtstat->nprocactive++;
					}
				}
			}
		}
		else
		{
			/*
			** new task which must have been started during
			** last interval
			*/
			memset(&prestat, 0, sizeof(prestat));

			curstat->gen.wasinactive = 0;
			devtstat->ntaskactive++;

			if (curstat->gen.isproc)
			{
				devtstat->nprocactive++;
			}
			else
			{
				if (thisproc->gen.wasinactive)
				{
					thisproc->gen.wasinactive = 0;
					devtstat->ntaskactive++;
					devtstat->nprocactive++;
				}
			}

			/*
			** create new task struct
			*/
			pinfo = calloc(1, sizeof(struct pinfo));

			ptrverify(pinfo, "Malloc failed for new pinfo\n");

			pinfo->tstat = *curstat;

			/*
			** add new task to task-database
			*/
			pdb_addtask(curstat->gen.pid, pinfo);

			newtask = 1;
		}

		/*
		** do the difference calculations
		*/
		calcdiff(devstat, curstat, &prestat, newtask, totusedcpu);
	}

	/*
	** calculate deviations per exited process
	*/
	if (nprocexit > 0 && supportflags&NETATOPD)
	{
		if (curpexit->gen.pid)
			hashtype = 'p';
		else
			hashtype = 'b';

		netatop_exithash(hashtype);
	}

	for (d=c, c=0; c < nprocexit; c++)
	{
		/*
		** check if this process has been started AND
		** finished since previous sample;
		** if so, it has no use to check if there is still 
		** existing info present in the process-database
		*/
		curstat = curpexit+c;
		curstat->gen.wasinactive = 0;

		devtstat->nprocall++;
		devtstat->nprocactive++;
		devtstat->ntaskactive++;

		if (curstat->gen.pid)	/* acctrecord contains pid? */
		{
			if ( pdb_gettask(curstat->gen.pid, 1,
			                 curstat->gen.btime, &pinfo))
					prestat = pinfo->tstat;
				else
					memset(&prestat, 0, sizeof(prestat));
		}
		else
		{
			if ( curstat->gen.btime > pretime )
			{
				/*
				** process-start and -finish in same interval
				*/
				memset(&prestat, 0, sizeof(prestat));
			}
			else
			{
				/*
				** process must be known in process-database;
				** try to match one of the remaining processes
				** against this exited one
				*/
				if ( pdb_srchresidue(curstat, &pinfo) )
					prestat = pinfo->tstat;
				else
					memset(&prestat, 0, sizeof(prestat));
		 	}
		}

		/*
		** now do the calculations
		*/
		devstat = devtstat->taskall+d;
		memset(devstat, 0, sizeof *devstat);

		devstat->gen = curstat->gen;

		if ( curstat->gen.pid == 0 )
			devstat->gen.pid    = prestat.gen.pid;

		if (!prestat.gen.pid)
			devstat->gen.excode |= ~(INT_MAX);

		strcpy(devstat->gen.cmdline, prestat.gen.cmdline);

		/*
		** due to the strange exponent-type storage of values
		** in the process accounting record, the resource-value
		** in the exit-record might have been smaller than the
		** stored value of the last registered sample; in that
		** case the deviation should be set to zero
		*/
		if (curstat->cpu.stime > prestat.cpu.stime)
			devstat->cpu.stime  = curstat->cpu.stime -
			                      prestat.cpu.stime;

		if (curstat->cpu.utime > prestat.cpu.utime)
			devstat->cpu.utime  = curstat->cpu.utime -
			                      prestat.cpu.utime;

		if (curstat->mem.minflt > prestat.mem.minflt)
			devstat->mem.minflt = curstat->mem.minflt - 
			                      prestat.mem.minflt;

		if (curstat->mem.majflt > prestat.mem.majflt)
			devstat->mem.majflt = curstat->mem.majflt -
			                      prestat.mem.majflt;

		if (curstat->dsk.rio > (prestat.dsk.rio + prestat.dsk.wio))
			devstat->dsk.rio    = curstat->dsk.rio  -
			                      prestat.dsk.rio   -
			                      prestat.dsk.wio;

		/*
		** try to match the network counters of netatop
		*/
		if (supportflags & NETATOPD)
		{
			unsigned long	val = (hashtype == 'p' ?
						curstat->gen.pid :
						curstat->gen.btime);

			netatop_exitfind(val, devstat, &prestat);
		}

		if (prestat.gen.pid > 0)
			pdb_deltask(prestat.gen.pid, prestat.gen.isproc);

		d++;
	}

	/*
	** remove unused entries from RESIDUE chain
	*/
	pdb_cleanresidue();

	/*
	** create and fill other pointer lists
	*/
	devtstat->procall    = malloc(devtstat->nprocall *
						sizeof(struct tstat *));
	devtstat->procactive = malloc(devtstat->nprocactive *
						sizeof(struct tstat *));

	ptrverify(devtstat->procall, "Malloc failed for %d processes\n",
                                  devtstat->nprocall);

	ptrverify(devtstat->procactive, "Malloc failed for %d active procs\n",
                                  devtstat->nprocactive);


        for (c=0, thisproc=devstat=devtstat->taskall; c < devtstat->ntaskall;
								c++, devstat++)
        {
        	if (devstat->gen.isproc)
		{
        		devtstat->procall[pall++] = devstat;

			if (! devstat->gen.wasinactive)
       				devtstat->procactive[pact++] = devstat;
		}
        }
}

/*
** calculate the differences between the current sample and
** the previous sample for a task
*/
static void
calcdiff(struct tstat *devstat, struct tstat *curstat, struct tstat *prestat,
	                                      char newtask, count_t totusedcpu)
{
	/*
 	** for inactive tasks, set all counters to zero
	*/
	if (curstat->gen.wasinactive)
		memset(devstat, 0, sizeof *devstat);

	/*
	** copy all static values from the current task settings
	*/
	devstat->gen          = curstat->gen;

	if (newtask)
		devstat->gen.excode |= ~(INT_MAX);

	devstat->cpu.nice     = curstat->cpu.nice;
	devstat->cpu.prio     = curstat->cpu.prio;
	devstat->cpu.rtprio   = curstat->cpu.rtprio;
	devstat->cpu.policy   = curstat->cpu.policy;
	devstat->cpu.curcpu   = curstat->cpu.curcpu;
	devstat->cpu.sleepavg = curstat->cpu.sleepavg;

	devstat->mem.vexec    = curstat->mem.vexec;
	devstat->mem.vmem     = curstat->mem.vmem;
	devstat->mem.rmem     = curstat->mem.rmem;
	devstat->mem.pmem     = curstat->mem.pmem;
	devstat->mem.vdata    = curstat->mem.vdata;
	devstat->mem.vstack   = curstat->mem.vstack;
	devstat->mem.vlibs    = curstat->mem.vlibs;
	devstat->mem.vswap    = curstat->mem.vswap;

	/*
 	** for inactive tasks, only the static values had to be copied, while
	** all use counters have been set to zero
	*/
	if (curstat->gen.wasinactive)
		return;

	devstat->cpu.stime  = 
		subcount(curstat->cpu.stime, prestat->cpu.stime);
	devstat->cpu.utime  =
		subcount(curstat->cpu.utime, prestat->cpu.utime);

	/*
	** particular kernel versions sometimes supply a smaller
	** amount for consumed CPU-ticks than a previous sample;
	** with unsigned calculations this results in 497 days of
	** CPU-consumption so a sanity-check is needed here...
	*/
	if (devstat->cpu.stime > totusedcpu)
		devstat->cpu.stime = 1;

	if (devstat->cpu.utime > totusedcpu)
		devstat->cpu.utime = 1;

	/*
	** do further calculations
	*/
	devstat->dsk.rio    =
		subcount(curstat->dsk.rio, prestat->dsk.rio);
	devstat->dsk.rsz    =
		subcount(curstat->dsk.rsz, prestat->dsk.rsz);
	devstat->dsk.wio    =
		subcount(curstat->dsk.wio, prestat->dsk.wio);
	devstat->dsk.wsz    =
		subcount(curstat->dsk.wsz, prestat->dsk.wsz);
	devstat->dsk.cwsz   =
		subcount(curstat->dsk.cwsz, prestat->dsk.cwsz);

	devstat->mem.vgrow  = curstat->mem.vmem   - prestat->mem.vmem;
	devstat->mem.rgrow  = curstat->mem.rmem   - prestat->mem.rmem;

	devstat->mem.minflt = 
		subcount(curstat->mem.minflt, prestat->mem.minflt);
	devstat->mem.majflt =
		subcount(curstat->mem.majflt, prestat->mem.majflt);

	/*
 	** network counters: due to an unload/load of the netatop module,
	** previous counters might be larger than the current
	*/
	if (curstat->net.tcpsnd >= prestat->net.tcpsnd)
		devstat->net.tcpsnd =
			subcount(curstat->net.tcpsnd, prestat->net.tcpsnd);
	else
		devstat->net.tcpsnd = curstat->net.tcpsnd;

	if (curstat->net.tcpssz >= prestat->net.tcpssz)
		devstat->net.tcpssz =
			subcount(curstat->net.tcpssz, prestat->net.tcpssz);
	else
		devstat->net.tcpssz = curstat->net.tcpssz;

	if (curstat->net.tcprcv >= prestat->net.tcprcv)
		devstat->net.tcprcv =
			subcount(curstat->net.tcprcv, prestat->net.tcprcv);
	else
		devstat->net.tcprcv = curstat->net.tcprcv;

	if (curstat->net.tcprsz >= prestat->net.tcprsz)
		devstat->net.tcprsz =
			subcount(curstat->net.tcprsz, prestat->net.tcprsz);
	else
		devstat->net.tcprsz = curstat->net.tcprsz;

	if (curstat->net.udpsnd >= prestat->net.udpsnd)
		devstat->net.udpsnd =
			subcount(curstat->net.udpsnd, prestat->net.udpsnd);
	else
		devstat->net.udpsnd = curstat->net.udpsnd;

	if (curstat->net.udpssz >= prestat->net.udpssz)
		devstat->net.udpssz =
			subcount(curstat->net.udpssz, prestat->net.udpssz);
	else
		devstat->net.udpssz = curstat->net.udpssz;

	if (curstat->net.udprcv >= prestat->net.udprcv)
		devstat->net.udprcv =
			subcount(curstat->net.udprcv, prestat->net.udprcv);
	else
		devstat->net.udprcv = curstat->net.udprcv;

	if (curstat->net.udprsz >= prestat->net.udprsz)
		devstat->net.udprsz =
			subcount(curstat->net.udprsz, prestat->net.udprsz);
	else
		devstat->net.udprsz = curstat->net.udprsz;
}

/*
** calculate the system-activity during the last sample
*/
void
deviatsyst(struct sstat *cur, struct sstat *pre, struct sstat *dev,
							long interval)
{
	register int	i, j;
	count_t		*cdev, *ccur, *cpre;
	struct ifprop	ifprop;

	dev->cpu.nrcpu     = cur->cpu.nrcpu;
	dev->cpu.devint    = subcount(cur->cpu.devint, pre->cpu.devint);
	dev->cpu.csw       = subcount(cur->cpu.csw,    pre->cpu.csw);
	dev->cpu.nprocs    = subcount(cur->cpu.nprocs, pre->cpu.nprocs);

	dev->cpu.all.stime = subcount(cur->cpu.all.stime, pre->cpu.all.stime);
	dev->cpu.all.utime = subcount(cur->cpu.all.utime, pre->cpu.all.utime);
	dev->cpu.all.ntime = subcount(cur->cpu.all.ntime, pre->cpu.all.ntime);
	dev->cpu.all.itime = subcount(cur->cpu.all.itime, pre->cpu.all.itime);
	dev->cpu.all.wtime = subcount(cur->cpu.all.wtime, pre->cpu.all.wtime);
	dev->cpu.all.Itime = subcount(cur->cpu.all.Itime, pre->cpu.all.Itime);
	dev->cpu.all.Stime = subcount(cur->cpu.all.Stime, pre->cpu.all.Stime);

	dev->cpu.all.steal = subcount(cur->cpu.all.steal, pre->cpu.all.steal);
	dev->cpu.all.guest = subcount(cur->cpu.all.guest, pre->cpu.all.guest);

	dev->cpu.all.instr = subcount(cur->cpu.all.instr, pre->cpu.all.instr);
	dev->cpu.all.cycle = subcount(cur->cpu.all.cycle, pre->cpu.all.cycle);

	for (i=0; i < dev->cpu.nrcpu; i++)
	{
		count_t 	ticks;

		dev->cpu.cpu[i].cpunr = cur->cpu.cpu[i].cpunr;
		dev->cpu.cpu[i].stime = subcount(cur->cpu.cpu[i].stime,
					         pre->cpu.cpu[i].stime);
		dev->cpu.cpu[i].utime = subcount(cur->cpu.cpu[i].utime,
				 	         pre->cpu.cpu[i].utime);
		dev->cpu.cpu[i].ntime = subcount(cur->cpu.cpu[i].ntime,
					         pre->cpu.cpu[i].ntime);
		dev->cpu.cpu[i].itime = subcount(cur->cpu.cpu[i].itime,
					         pre->cpu.cpu[i].itime);
		dev->cpu.cpu[i].wtime = subcount(cur->cpu.cpu[i].wtime,
					         pre->cpu.cpu[i].wtime);
		dev->cpu.cpu[i].Itime = subcount(cur->cpu.cpu[i].Itime,
					         pre->cpu.cpu[i].Itime);
		dev->cpu.cpu[i].Stime = subcount(cur->cpu.cpu[i].Stime,
					         pre->cpu.cpu[i].Stime);

		dev->cpu.cpu[i].steal = subcount(cur->cpu.cpu[i].steal,
					         pre->cpu.cpu[i].steal);
		dev->cpu.cpu[i].guest = subcount(cur->cpu.cpu[i].guest,
					         pre->cpu.cpu[i].guest);

		dev->cpu.cpu[i].instr = subcount(cur->cpu.cpu[i].instr,
					         pre->cpu.cpu[i].instr);
		dev->cpu.cpu[i].cycle = subcount(cur->cpu.cpu[i].cycle,
					         pre->cpu.cpu[i].cycle);

		ticks 		      = cur->cpu.cpu[i].freqcnt.ticks;

		dev->cpu.cpu[i].freqcnt.maxfreq = 
					cur->cpu.cpu[i].freqcnt.maxfreq;
		dev->cpu.cpu[i].freqcnt.cnt = ticks ?
					subcount(cur->cpu.cpu[i].freqcnt.cnt,
					         pre->cpu.cpu[i].freqcnt.cnt)
					       : cur->cpu.cpu[i].freqcnt.cnt;

		dev->cpu.cpu[i].freqcnt.ticks = ticks ?
					subcount(cur->cpu.cpu[i].freqcnt.ticks,
					         pre->cpu.cpu[i].freqcnt.ticks)
					       : cur->cpu.cpu[i].freqcnt.ticks;
	}

	dev->cpu.lavg1		= cur->cpu.lavg1;
	dev->cpu.lavg5		= cur->cpu.lavg5;
	dev->cpu.lavg15		= cur->cpu.lavg15;

	dev->mem.physmem	= cur->mem.physmem;
	dev->mem.freemem	= cur->mem.freemem;
	dev->mem.buffermem	= cur->mem.buffermem;
	dev->mem.slabmem	= cur->mem.slabmem;
	dev->mem.slabreclaim	= cur->mem.slabreclaim;
	dev->mem.committed	= cur->mem.committed;
	dev->mem.commitlim	= cur->mem.commitlim;
	dev->mem.cachemem	= cur->mem.cachemem;
	dev->mem.cachedrt	= cur->mem.cachedrt;
	dev->mem.totswap	= cur->mem.totswap;
	dev->mem.freeswap	= cur->mem.freeswap;

	dev->mem.shmem		= cur->mem.shmem;
	dev->mem.shmrss		= cur->mem.shmrss;
	dev->mem.shmswp		= cur->mem.shmswp;

	dev->mem.tothugepage	= cur->mem.tothugepage;
	dev->mem.freehugepage	= cur->mem.freehugepage;
	dev->mem.hugepagesz	= cur->mem.hugepagesz;

	dev->mem.vmwballoon	= cur->mem.vmwballoon;

	dev->mem.swouts		= subcount(cur->mem.swouts,  pre->mem.swouts);
	dev->mem.swins		= subcount(cur->mem.swins,   pre->mem.swins);
	dev->mem.pgscans	= subcount(cur->mem.pgscans, pre->mem.pgscans);
	dev->mem.pgsteal	= subcount(cur->mem.pgsteal, pre->mem.pgsteal);
	dev->mem.allocstall	= subcount(cur->mem.allocstall,
				                         pre->mem.allocstall);

	/*
	** structures with network-related counters are considered
	** as tables of frequency-counters that have to be subtracted;
	** values that do not represent a frequency are corrected afterwards
	*/
	for (cdev = (count_t *)&dev->net.ipv4,
	     ccur = (count_t *)&cur->net.ipv4,
	     cpre = (count_t *)&pre->net.ipv4,
	     i    = 0;
		i < (sizeof dev->net.ipv4 / sizeof(count_t));
	     		cdev++, ccur++, cpre++, i++)
				*cdev = *ccur - *cpre;
	
	dev->net.ipv4.Forwarding = cur->net.ipv4.Forwarding;
	dev->net.ipv4.DefaultTTL = cur->net.ipv4.DefaultTTL;

		/* ------------- */

	for (cdev = (count_t *)&dev->net.icmpv4,
	     ccur = (count_t *)&cur->net.icmpv4,
	     cpre = (count_t *)&pre->net.icmpv4,
	     i    = 0;
		i < (sizeof dev->net.icmpv4 / sizeof(count_t));
	     		cdev++, ccur++, cpre++, i++)
				*cdev = *ccur - *cpre;

		/* ------------- */

	for (cdev = (count_t *)&dev->net.udpv4,
	     ccur = (count_t *)&cur->net.udpv4,
	     cpre = (count_t *)&pre->net.udpv4,
	     i    = 0;
		i < (sizeof dev->net.udpv4 / sizeof(count_t));
	     		cdev++, ccur++, cpre++, i++)
				*cdev = *ccur - *cpre;

		/* ------------- */

	for (cdev = (count_t *)&dev->net.ipv6,
	     ccur = (count_t *)&cur->net.ipv6,
	     cpre = (count_t *)&pre->net.ipv6,
	     i    = 0;
		i < (sizeof dev->net.ipv6 / sizeof(count_t));
	     		cdev++, ccur++, cpre++, i++)
				*cdev = *ccur - *cpre;

		/* ------------- */

	for (cdev = (count_t *)&dev->net.icmpv6,
	     ccur = (count_t *)&cur->net.icmpv6,
	     cpre = (count_t *)&pre->net.icmpv6,
	     i    = 0;
		i < (sizeof dev->net.icmpv6 / sizeof(count_t));
	     		cdev++, ccur++, cpre++, i++)
				*cdev = *ccur - *cpre;

		/* ------------- */

	for (cdev = (count_t *)&dev->net.udpv6,
	     ccur = (count_t *)&cur->net.udpv6,
	     cpre = (count_t *)&pre->net.udpv6,
	     i    = 0;
		i < (sizeof dev->net.udpv6 / sizeof(count_t));
	     		cdev++, ccur++, cpre++, i++)
				*cdev = *ccur - *cpre;

		/* ------------- */

	for (cdev = (count_t *)&dev->net.tcp,
	     ccur = (count_t *)&cur->net.tcp,
	     cpre = (count_t *)&pre->net.tcp,
	     i    = 0;
		i < (sizeof dev->net.tcp / sizeof(count_t));
	     		cdev++, ccur++, cpre++, i++)
				*cdev = *ccur - *cpre;

	dev->net.tcp.RtoAlgorithm = cur->net.tcp.RtoAlgorithm;
	dev->net.tcp.RtoMin       = cur->net.tcp.RtoMin;
	dev->net.tcp.RtoMax       = cur->net.tcp.RtoMax;
	dev->net.tcp.MaxConn      = cur->net.tcp.MaxConn;
	dev->net.tcp.CurrEstab    = cur->net.tcp.CurrEstab;

	/*
	** calculate deviations for interfaces
	**
	** refresh all interface properties
	*/
        regainrootprivs();	/* get root privileges      */

	initifprop();		/* refresh interface info   */

	if (! droprootprivs())  /* drop setuid-root privs   */
		cleanstop(42);

	for (i=0; cur->intf.intf[i].name[0]; i++)
	{
		strcpy(ifprop.name, cur->intf.intf[i].name);

		getifprop(&ifprop);

		cur->intf.intf[i].type   = ifprop.type;
		cur->intf.intf[i].speed  = ifprop.speed;
		cur->intf.intf[i].speedp = ifprop.speed;
		cur->intf.intf[i].duplex = ifprop.fullduplex;
	}

	if (pre->intf.intf[0].name[0] == '\0')	/* first sample? */
	{
		for (i=0; cur->intf.intf[i].name[0]; i++)
		{
			strcpy(pre->intf.intf[i].name, cur->intf.intf[i].name);

			pre->intf.intf[i].type   = cur->intf.intf[i].type;
			pre->intf.intf[i].speed  = cur->intf.intf[i].speed;
			pre->intf.intf[i].speedp = cur->intf.intf[i].speedp;
			pre->intf.intf[i].duplex = cur->intf.intf[i].duplex;
 		}
	}
	
	for (i=0, j=0; cur->intf.intf[i].name[0]; i++, j++)
	{
		/*
		** be sure that we have the same interface
		** (interfaces could have been added or removed since
		** previous sample)
		*/
		if (strcmp(cur->intf.intf[i].name, pre->intf.intf[j].name) != 0)
		{
			// try to resync
			for (j=0; pre->intf.intf[j].name[0]; j++)
			{
				if (strcmp(cur->intf.intf[i].name,
				           pre->intf.intf[j].name) == 0)
					break;
			}

			// resync not succeeded?
			if (! pre->intf.intf[j].name[0])
			{
				memcpy(&dev->intf.intf[i],
				       &cur->intf.intf[i],
				       sizeof cur->intf.intf[i]);

				j = 0;
				continue;
			}
		}

		/*
		** calculate interface deviations for this sample
		*/
		strcpy(dev->intf.intf[i].name, cur->intf.intf[i].name);

		dev->intf.intf[i].rbyte = subcount(cur->intf.intf[i].rbyte,
           	                                   pre->intf.intf[j].rbyte);
		dev->intf.intf[i].rpack = subcount(cur->intf.intf[i].rpack,
		                                   pre->intf.intf[j].rpack);
		dev->intf.intf[i].rerrs = subcount(cur->intf.intf[i].rerrs,
		                                   pre->intf.intf[j].rerrs);
		dev->intf.intf[i].rdrop = subcount(cur->intf.intf[i].rdrop,
		                                   pre->intf.intf[j].rdrop);
		dev->intf.intf[i].rfifo = subcount(cur->intf.intf[i].rfifo,
		                                   pre->intf.intf[j].rfifo);
		dev->intf.intf[i].rframe= subcount(cur->intf.intf[i].rframe,
		                                   pre->intf.intf[j].rframe);
		dev->intf.intf[i].rcompr= subcount(cur->intf.intf[i].rcompr,
		                                   pre->intf.intf[j].rcompr);
		dev->intf.intf[i].rmultic=subcount(cur->intf.intf[i].rmultic,
		                                   pre->intf.intf[j].rmultic);

		dev->intf.intf[i].sbyte = subcount(cur->intf.intf[i].sbyte,
		                                   pre->intf.intf[j].sbyte);
		dev->intf.intf[i].spack = subcount(cur->intf.intf[i].spack,
		                                   pre->intf.intf[j].spack);
		dev->intf.intf[i].serrs = subcount(cur->intf.intf[i].serrs,
		                                   pre->intf.intf[j].serrs);
		dev->intf.intf[i].sdrop = subcount(cur->intf.intf[i].sdrop,
		                                   pre->intf.intf[j].sdrop);
		dev->intf.intf[i].sfifo = subcount(cur->intf.intf[i].sfifo,
		                                   pre->intf.intf[j].sfifo);
		dev->intf.intf[i].scollis= subcount(cur->intf.intf[i].scollis,
		                                   pre->intf.intf[j].scollis);
		dev->intf.intf[i].scarrier= subcount(cur->intf.intf[i].scarrier,
		                                   pre->intf.intf[j].scarrier);
		dev->intf.intf[i].scompr= subcount(cur->intf.intf[i].scompr,
		                                   pre->intf.intf[j].scompr);

		dev->intf.intf[i].type  	= cur->intf.intf[i].type;
		dev->intf.intf[i].duplex	= cur->intf.intf[i].duplex;
		dev->intf.intf[i].speed 	= cur->intf.intf[i].speed;
		dev->intf.intf[i].speedp 	= pre->intf.intf[j].speed;

		cur->intf.intf[i].speedp 	= pre->intf.intf[j].speed;
	}

	dev->intf.intf[i].name[0] = '\0';
	dev->intf.nrintf = i;

	/*
	** calculate deviations for disks
	*/
	for (i=j=0; cur->dsk.dsk[i].name[0]; i++)
	{
		int	realj = j;

		/*
 		** check if disk has been added or removed since
		** previous interval
		*/
		if ( strcmp(cur->dsk.dsk[i].name, pre->dsk.dsk[j].name) != 0)
		{
			for (j++; pre->dsk.dsk[j].name[0]; j++)
			{
				if ( strcmp(cur->dsk.dsk[i].name,
						pre->dsk.dsk[j].name) == 0)
					break;
			}

			/*
			** either the corresponding entry has been found
			** in the case that a disk has been removed, or
			** an empty entry has been found (all counters
			** on zero) in the case that a disk has been added
			** during the last sample
			*/
		}

		strcpy(dev->dsk.dsk[i].name, cur->dsk.dsk[i].name);

		dev->dsk.dsk[i].nread  = subcount(cur->dsk.dsk[i].nread,
		                                  pre->dsk.dsk[j].nread);
		dev->dsk.dsk[i].nwrite = subcount(cur->dsk.dsk[i].nwrite,
		                                  pre->dsk.dsk[j].nwrite);
		dev->dsk.dsk[i].nrsect = subcount(cur->dsk.dsk[i].nrsect,
		                                  pre->dsk.dsk[j].nrsect);
		dev->dsk.dsk[i].nwsect = subcount(cur->dsk.dsk[i].nwsect,
		                                  pre->dsk.dsk[j].nwsect);
		dev->dsk.dsk[i].io_ms  = subcount(cur->dsk.dsk[i].io_ms,
		                                  pre->dsk.dsk[j].io_ms);
		dev->dsk.dsk[i].avque  = subcount(cur->dsk.dsk[i].avque,
		                                  pre->dsk.dsk[j].avque);

		/*
		** determine new j
		*/
		if (pre->dsk.dsk[j].name)	// existing matching entry
			j++;
		else
			j = realj;		// empty entry: stick to old j
	}

	dev->dsk.dsk[i].name[0] = '\0';
	dev->dsk.ndsk = i;

	/*
	** calculate deviations for multiple devices
	*/
	for (i=j=0; cur->dsk.mdd[i].name[0]; i++)
	{
		int	realj = j;

		/*
 		** check if md has been added or removed since
		** previous interval
		*/
		if ( strcmp(cur->dsk.mdd[i].name, pre->dsk.mdd[j].name) != 0)
		{
			for (j++; pre->dsk.mdd[j].name[0]; j++)
			{
				if ( strcmp(cur->dsk.mdd[i].name,
						pre->dsk.mdd[j].name) == 0)
					break;
			}

			/*
			** either the corresponding entry has been found
			** in the case that a md has been removed, or
			** an empty entry has been found (all counters
			** on zero) in the case that a md has been added
			** during the last sample
			*/
		}

		strcpy(dev->dsk.mdd[i].name, cur->dsk.mdd[i].name);

		dev->dsk.mdd[i].nread  = subcount(cur->dsk.mdd[i].nread,
		                                  pre->dsk.mdd[j].nread);
		dev->dsk.mdd[i].nwrite = subcount(cur->dsk.mdd[i].nwrite,
		                                  pre->dsk.mdd[j].nwrite);
		dev->dsk.mdd[i].nrsect = subcount(cur->dsk.mdd[i].nrsect,
		                                  pre->dsk.mdd[j].nrsect);
		dev->dsk.mdd[i].nwsect = subcount(cur->dsk.mdd[i].nwsect,
		                                  pre->dsk.mdd[j].nwsect);
		dev->dsk.mdd[i].io_ms  = subcount(cur->dsk.mdd[i].io_ms,
		                                  pre->dsk.mdd[j].io_ms);
		dev->dsk.mdd[i].avque  = subcount(cur->dsk.mdd[i].avque,
		                                  pre->dsk.mdd[j].avque);

		/*
		** determine new j
		*/
		if (pre->dsk.mdd[j].name)	// existing matching entry
			j++;
		else
			j = realj;		// empty entry: stick to old j
	}

	dev->dsk.mdd[i].name[0] = '\0';
	dev->dsk.nmdd = i;

	/*
	** calculate deviations for LVM logical volumes
	*/
	for (i=j=0; cur->dsk.lvm[i].name[0]; i++)
	{
		int	realj = j;

		/*
 		** check if logical volume has been added or removed since
		** previous interval
		*/
		if ( strcmp(cur->dsk.lvm[i].name, pre->dsk.lvm[j].name) != 0)
		{
			for (j++; pre->dsk.lvm[j].name[0]; j++)
			{
				if ( strcmp(cur->dsk.lvm[i].name,
						pre->dsk.lvm[j].name) == 0)
					break;
			}

			/*
			** either the corresponding entry has been found
			** in the case that a logical volume has been removed,
			** or an empty entry has been found (all counters
			** on zero) in the case that a logical volume has
			** been added during the last sample
			*/
		}

		strcpy(dev->dsk.lvm[i].name, cur->dsk.lvm[i].name);

		dev->dsk.lvm[i].nread  = subcount(cur->dsk.lvm[i].nread,
		                                  pre->dsk.lvm[j].nread);
		dev->dsk.lvm[i].nwrite = subcount(cur->dsk.lvm[i].nwrite,
		                                  pre->dsk.lvm[j].nwrite);
		dev->dsk.lvm[i].nrsect = subcount(cur->dsk.lvm[i].nrsect,
		                                  pre->dsk.lvm[j].nrsect);
		dev->dsk.lvm[i].nwsect = subcount(cur->dsk.lvm[i].nwsect,
		                                  pre->dsk.lvm[j].nwsect);
		dev->dsk.lvm[i].io_ms  = subcount(cur->dsk.lvm[i].io_ms,
		                                  pre->dsk.lvm[j].io_ms);
		dev->dsk.lvm[i].avque  = subcount(cur->dsk.lvm[i].avque,
		                                  pre->dsk.lvm[j].avque);

		/*
		** determine new j
		*/
		if (pre->dsk.lvm[j].name)	// existing matching entry
			j++;
		else
			j = realj;		// empty entry: stick to old j
	}

	dev->dsk.lvm[i].name[0] = '\0';
	dev->dsk.nlvm = i;

	/*
	** calculate deviations for NFS
	*/
	dev->nfs.server.netcnt    = subcount(cur->nfs.server.netcnt,
	                                     pre->nfs.server.netcnt);
	dev->nfs.server.netudpcnt = subcount(cur->nfs.server.netudpcnt,
	                                     pre->nfs.server.netudpcnt);
	dev->nfs.server.nettcpcnt = subcount(cur->nfs.server.nettcpcnt,
	                                     pre->nfs.server.nettcpcnt);
	dev->nfs.server.nettcpcon = subcount(cur->nfs.server.nettcpcon,
	                                     pre->nfs.server.nettcpcon);

	dev->nfs.server.rpccnt    = subcount(cur->nfs.server.rpccnt,
	                                     pre->nfs.server.rpccnt);
	dev->nfs.server.rpcread   = subcount(cur->nfs.server.rpcread,
	                                     pre->nfs.server.rpcread);
	dev->nfs.server.rpcwrite  = subcount(cur->nfs.server.rpcwrite,
	                                     pre->nfs.server.rpcwrite);
	dev->nfs.server.rpcbadfmt = subcount(cur->nfs.server.rpcbadfmt,
	                                     pre->nfs.server.rpcbadfmt);
	dev->nfs.server.rpcbadaut = subcount(cur->nfs.server.rpcbadaut,
	                                     pre->nfs.server.rpcbadaut);
	dev->nfs.server.rpcbadcln = subcount(cur->nfs.server.rpcbadcln,
	                                     pre->nfs.server.rpcbadcln);
	
	dev->nfs.server.rchits    = subcount(cur->nfs.server.rchits,
	                                     pre->nfs.server.rchits);
	dev->nfs.server.rcmiss    = subcount(cur->nfs.server.rcmiss,
	                                     pre->nfs.server.rcmiss);
	dev->nfs.server.rcnoca    = subcount(cur->nfs.server.rcnoca,
	                                     pre->nfs.server.rcnoca);

	dev->nfs.server.nrbytes   = subcount(cur->nfs.server.nrbytes,
	                                     pre->nfs.server.nrbytes);
	dev->nfs.server.nwbytes   = subcount(cur->nfs.server.nwbytes,
	                                     pre->nfs.server.nwbytes);

	dev->nfs.client.rpccnt        = subcount(cur->nfs.client.rpccnt,
	                                         pre->nfs.client.rpccnt);
	dev->nfs.client.rpcread       = subcount(cur->nfs.client.rpcread,
	                                         pre->nfs.client.rpcread);
	dev->nfs.client.rpcwrite      = subcount(cur->nfs.client.rpcwrite,
	                                         pre->nfs.client.rpcwrite);
	dev->nfs.client.rpcretrans    = subcount(cur->nfs.client.rpcretrans,
	                                         pre->nfs.client.rpcretrans);
	dev->nfs.client.rpcautrefresh = subcount(cur->nfs.client.rpcautrefresh,
	                                         pre->nfs.client.rpcautrefresh);


	for (i=j=0; i < cur->nfs.nfsmounts.nrmounts; i++, j++)
	{
		/*
 		** check if nfsmounts have been added or removed since
		** previous interval
		*/
		if ( strcmp(cur->nfs.nfsmounts.nfsmnt[i].mountdev,
		            pre->nfs.nfsmounts.nfsmnt[j].mountdev) != 0)
		{
			for (j=0; j < pre->nfs.nfsmounts.nrmounts; j++)
			{
			    if ( strcmp(cur->nfs.nfsmounts.nfsmnt[i].mountdev,
		                        pre->nfs.nfsmounts.nfsmnt[j].mountdev)
									== 0)
					break;
			}

			/*
			** either the corresponding entry has been found
			** in the case that a container has been removed,
			** or an empty entry has been found (all counters
			** on zero) in the case that a container has
			** been added during the last sample
			*/
		}

		strcpy(dev->nfs.nfsmounts.nfsmnt[i].mountdev,
		       cur->nfs.nfsmounts.nfsmnt[i].mountdev);

                dev->nfs.nfsmounts.nfsmnt[i].age = 
                                    cur->nfs.nfsmounts.nfsmnt[i].age;

		if (dev->nfs.nfsmounts.nfsmnt[i].age <= interval)
			memset(&(pre->nfs.nfsmounts.nfsmnt[j]), 0, 
					sizeof(struct pernfsmount));

                dev->nfs.nfsmounts.nfsmnt[i].bytesread = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].bytesread,
                                   pre->nfs.nfsmounts.nfsmnt[j].bytesread);

                dev->nfs.nfsmounts.nfsmnt[i].byteswrite = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].byteswrite,
                                   pre->nfs.nfsmounts.nfsmnt[j].byteswrite);

                dev->nfs.nfsmounts.nfsmnt[i].bytesdread = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].bytesdread,
                                   pre->nfs.nfsmounts.nfsmnt[j].bytesdread);

                dev->nfs.nfsmounts.nfsmnt[i].bytesdwrite = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].bytesdwrite,
                                   pre->nfs.nfsmounts.nfsmnt[j].bytesdwrite);

                dev->nfs.nfsmounts.nfsmnt[i].bytestotread = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].bytestotread,
                                   pre->nfs.nfsmounts.nfsmnt[j].bytestotread);

                dev->nfs.nfsmounts.nfsmnt[i].bytestotwrite = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].bytestotwrite,
                                   pre->nfs.nfsmounts.nfsmnt[j].bytestotwrite);

                dev->nfs.nfsmounts.nfsmnt[i].pagesmread = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].pagesmread,
                                   pre->nfs.nfsmounts.nfsmnt[j].pagesmread);

                dev->nfs.nfsmounts.nfsmnt[i].pagesmwrite = 
                          subcount(cur->nfs.nfsmounts.nfsmnt[i].pagesmwrite,
                                   pre->nfs.nfsmounts.nfsmnt[j].pagesmwrite);
	}

	dev->nfs.nfsmounts.nrmounts = cur->nfs.nfsmounts.nrmounts;

	/*
	** calculate deviations for containers
	*/
	for (i=j=0; i < cur->cfs.nrcontainer; i++, j++)
	{
		/*
 		** check if containers have been added or removed since
		** previous interval
		*/
		if (cur->cfs.cont[i].ctid != pre->cfs.cont[j].ctid)
		{
			for (j=0; j < pre->cfs.nrcontainer; j++)
			{
				if (cur->cfs.cont[i].ctid ==
						pre->cfs.cont[j].ctid)
					break;
			}

			/*
			** either the corresponding entry has been found
			** in the case that a container has been removed,
			** or an empty entry has been found (all counters
			** on zero) in the case that a container has
			** been added during the last sample
			*/
		}

		dev->cfs.cont[i].ctid    = cur->cfs.cont[i].ctid;
		dev->cfs.cont[i].numproc = cur->cfs.cont[i].numproc;

		dev->cfs.cont[i].system  = subcount(cur->cfs.cont[i].system,
		                                    pre->cfs.cont[i].system);
		dev->cfs.cont[i].user    = subcount(cur->cfs.cont[i].user,
		                                    pre->cfs.cont[i].user);
		dev->cfs.cont[i].nice    = subcount(cur->cfs.cont[i].nice,
		                                    pre->cfs.cont[i].nice);
		dev->cfs.cont[i].uptime  = subcount(cur->cfs.cont[i].uptime,
		                                    pre->cfs.cont[i].uptime);

		dev->cfs.cont[i].physpages = cur->cfs.cont[i].physpages;
	}

	dev->cfs.nrcontainer = cur->cfs.nrcontainer;

	/*
	** application-specific counters
	*/
#if	HTTPSTATS
	if (cur->www.uptime >= pre->www.uptime)
	{
		dev->www.accesses  = subcount(cur->www.accesses,
                                              pre->www.accesses);
		dev->www.totkbytes = subcount(cur->www.totkbytes,
                                              pre->www.totkbytes);
	}
	else
	{
		dev->www.accesses  = cur->www.accesses;
		dev->www.totkbytes = cur->www.totkbytes;
	}

	dev->www.bworkers  = cur->www.bworkers;
	dev->www.iworkers  = cur->www.iworkers;
#endif
}

/*
** add the values of a new sample to a structure holding the totals
** for the indicated category (c=cpu, m=memory, d=disk, n=network).
*/
void
totalsyst(char category, struct sstat *new, struct sstat *tot)
{
	register int	i;
	count_t		*ctot, *cnew;

	switch (category)
	{
	   case 'c':	/* accumulate cpu-related counters */
		tot->cpu.nrcpu      = new->cpu.nrcpu;
		tot->cpu.devint    += new->cpu.devint;
		tot->cpu.csw       += new->cpu.csw;
		tot->cpu.nprocs    += new->cpu.nprocs;

		tot->cpu.all.stime += new->cpu.all.stime;
		tot->cpu.all.utime += new->cpu.all.utime;
		tot->cpu.all.ntime += new->cpu.all.ntime;
		tot->cpu.all.itime += new->cpu.all.itime;
		tot->cpu.all.wtime += new->cpu.all.wtime;
		tot->cpu.all.Itime += new->cpu.all.Itime;
		tot->cpu.all.Stime += new->cpu.all.Stime;
		tot->cpu.all.steal += new->cpu.all.steal;
		tot->cpu.all.guest += new->cpu.all.guest;

		if (new->cpu.nrcpu == 1)
		{
			tot->cpu.cpu[0] = tot->cpu.all;
		}
		else
		{
			for (i=0; i < new->cpu.nrcpu; i++)
			{
				tot->cpu.cpu[i].cpunr  = new->cpu.cpu[i].cpunr;
				tot->cpu.cpu[i].stime += new->cpu.cpu[i].stime;
				tot->cpu.cpu[i].utime += new->cpu.cpu[i].utime;
				tot->cpu.cpu[i].ntime += new->cpu.cpu[i].ntime;
				tot->cpu.cpu[i].itime += new->cpu.cpu[i].itime;
				tot->cpu.cpu[i].wtime += new->cpu.cpu[i].wtime;
				tot->cpu.cpu[i].Itime += new->cpu.cpu[i].Itime;
				tot->cpu.cpu[i].Stime += new->cpu.cpu[i].Stime;
				tot->cpu.cpu[i].steal += new->cpu.cpu[i].steal;
				tot->cpu.cpu[i].guest += new->cpu.cpu[i].guest;
			}
		}

		tot->cpu.lavg1	 = new->cpu.lavg1;
		tot->cpu.lavg5	 = new->cpu.lavg5;
		tot->cpu.lavg15	 = new->cpu.lavg15;
		break;

	   case 'm':	/* accumulate memory-related counters */
		tot->mem.physmem	 = new->mem.physmem;
		tot->mem.freemem	 = new->mem.freemem;
		tot->mem.buffermem	 = new->mem.buffermem;
		tot->mem.slabmem	 = new->mem.slabmem;
		tot->mem.slabreclaim	 = new->mem.slabreclaim;
		tot->mem.committed	 = new->mem.committed;
		tot->mem.commitlim	 = new->mem.commitlim;
		tot->mem.cachemem	 = new->mem.cachemem;
		tot->mem.cachedrt	 = new->mem.cachedrt;
		tot->mem.totswap	 = new->mem.totswap;
		tot->mem.freeswap	 = new->mem.freeswap;

		tot->mem.shmem		 = new->mem.shmem;
		tot->mem.shmrss		 = new->mem.shmrss;
		tot->mem.shmswp		 = new->mem.shmswp;

		tot->mem.swouts		+= new->mem.swouts;
		tot->mem.swins		+= new->mem.swins;
		tot->mem.pgscans	+= new->mem.pgscans;
		tot->mem.allocstall	+= new->mem.allocstall;
		break;

	   case 'n':	/* accumulate network-related counters */
		tot->nfs.server.rpccnt     += new->nfs.server.rpccnt;
		tot->nfs.server.rpcread    += new->nfs.server.rpcread;
		tot->nfs.server.rpcwrite   += new->nfs.server.rpcwrite;
		tot->nfs.server.rpcbadfmt  += new->nfs.server.rpcbadfmt;
		tot->nfs.server.rpcbadaut  += new->nfs.server.rpcbadaut;
		tot->nfs.server.rpcbadcln  += new->nfs.server.rpcbadcln;

		tot->nfs.server.netcnt     += new->nfs.server.netcnt;
		tot->nfs.server.nettcpcnt  += new->nfs.server.nettcpcnt;
		tot->nfs.server.netudpcnt  += new->nfs.server.netudpcnt;
		tot->nfs.server.nettcpcon  += new->nfs.server.nettcpcon;

		tot->nfs.server.rchits     += new->nfs.server.rchits;
		tot->nfs.server.rcmiss     += new->nfs.server.rcmiss;
		tot->nfs.server.rcnoca     += new->nfs.server.rcnoca;

		tot->nfs.server.nrbytes    += new->nfs.server.nrbytes;
		tot->nfs.server.nwbytes    += new->nfs.server.nwbytes;

		tot->nfs.client.rpccnt        += new->nfs.client.rpccnt;
		tot->nfs.client.rpcread       += new->nfs.client.rpcread;
		tot->nfs.client.rpcwrite      += new->nfs.client.rpcwrite;
		tot->nfs.client.rpcretrans    += new->nfs.client.rpcretrans;
		tot->nfs.client.rpcautrefresh += new->nfs.client.rpcautrefresh;

		/*
		** other structures with network counters are considered
		** as tables of frequency-counters that will be accumulated;
		** values that do not represent a frequency are corrected
		** afterwards
		*/
		for (ctot = (count_t *)&tot->net.ipv4,
		     cnew = (count_t *)&new->net.ipv4, i = 0;
			i < (sizeof tot->net.ipv4 / sizeof(count_t));
		     		ctot++, cnew++, i++)
					*ctot += *cnew;

		tot->net.ipv4.Forwarding = new->net.ipv4.Forwarding;
		tot->net.ipv4.DefaultTTL = new->net.ipv4.DefaultTTL;
	
			/* ------------- */
	
		for (ctot = (count_t *)&tot->net.icmpv4,
		     cnew = (count_t *)&new->net.icmpv4, i = 0;
			i < (sizeof tot->net.icmpv4 / sizeof(count_t));
		     		ctot++, cnew++, i++)
					*ctot += *cnew;
	
			/* ------------- */
	
		for (ctot = (count_t *)&tot->net.udpv4,
		     cnew = (count_t *)&new->net.udpv4, i = 0;
			i < (sizeof tot->net.udpv4 / sizeof(count_t));
		     		ctot++, cnew++, i++)
					*ctot += *cnew;
	
			/* ------------- */
	
		for (ctot = (count_t *)&tot->net.ipv6,
		     cnew = (count_t *)&new->net.ipv6, i = 0;
			i < (sizeof tot->net.ipv6 / sizeof(count_t));
		     		ctot++, cnew++, i++)
					*ctot += *cnew;
	
			/* ------------- */
	
		for (ctot = (count_t *)&tot->net.icmpv6,
		     cnew = (count_t *)&new->net.icmpv6, i = 0;
			i < (sizeof tot->net.icmpv6 / sizeof(count_t));
		     		ctot++, cnew++, i++)
					*ctot += *cnew;
	
			/* ------------- */
	
		for (ctot = (count_t *)&tot->net.udpv6,
		     cnew = (count_t *)&new->net.udpv6, i = 0;
			i < (sizeof tot->net.udpv6 / sizeof(count_t));
		     		ctot++, cnew++, i++)
					*ctot += *cnew;
	
			/* ------------- */
	
		for (ctot = (count_t *)&tot->net.tcp,
		     cnew = (count_t *)&new->net.tcp, i = 0;
			i < (sizeof tot->net.tcp / sizeof(count_t));
		     		ctot++, cnew++, i++)
					*ctot += *cnew;
	
		tot->net.tcp.RtoAlgorithm = new->net.tcp.RtoAlgorithm;
		tot->net.tcp.RtoMin       = new->net.tcp.RtoMin;
		tot->net.tcp.RtoMax       = new->net.tcp.RtoMax;
		tot->net.tcp.MaxConn      = new->net.tcp.MaxConn;
		tot->net.tcp.CurrEstab    = new->net.tcp.CurrEstab;
	
		for (i=0; new->intf.intf[i].name[0]; i++)
		{
			/*
			** check if an interface has been added or removed;
			** in that case, zero all counters
			*/
			if (strcmp(new->intf.intf[i].name,
			           tot->intf.intf[i].name) != 0)
			{
				tot->intf.intf[i].rbyte    = 0;
				tot->intf.intf[i].rpack    = 0;
				tot->intf.intf[i].rerrs    = 0;
				tot->intf.intf[i].rdrop    = 0;
				tot->intf.intf[i].rfifo    = 0;
				tot->intf.intf[i].rframe   = 0;
				tot->intf.intf[i].rcompr   = 0;
				tot->intf.intf[i].rmultic  = 0;
	
				tot->intf.intf[i].sbyte    = 0;
				tot->intf.intf[i].spack    = 0;
				tot->intf.intf[i].serrs    = 0;
				tot->intf.intf[i].sdrop    = 0;
				tot->intf.intf[i].sfifo    = 0;
				tot->intf.intf[i].scollis  = 0;
				tot->intf.intf[i].scarrier = 0;
				tot->intf.intf[i].scompr   = 0;
			}
	
			/*
			** accumulate counters for this sample
			*/
			strcpy(tot->intf.intf[i].name, new->intf.intf[i].name);
	
			tot->intf.intf[i].rbyte   += new->intf.intf[i].rbyte;
			tot->intf.intf[i].rpack   += new->intf.intf[i].rpack;
			tot->intf.intf[i].rerrs   += new->intf.intf[i].rerrs;
			tot->intf.intf[i].rdrop   += new->intf.intf[i].rdrop;
			tot->intf.intf[i].rfifo   += new->intf.intf[i].rfifo;
			tot->intf.intf[i].rframe  += new->intf.intf[i].rframe;
			tot->intf.intf[i].rcompr  += new->intf.intf[i].rcompr;
			tot->intf.intf[i].rmultic += new->intf.intf[i].rmultic;
	
			tot->intf.intf[i].sbyte   += new->intf.intf[i].sbyte;
			tot->intf.intf[i].spack   += new->intf.intf[i].spack;
			tot->intf.intf[i].serrs   += new->intf.intf[i].serrs;
			tot->intf.intf[i].sdrop   += new->intf.intf[i].sdrop;
			tot->intf.intf[i].sfifo   += new->intf.intf[i].sfifo;
			tot->intf.intf[i].scollis += new->intf.intf[i].scollis;
			tot->intf.intf[i].scarrier+= new->intf.intf[i].scarrier;
			tot->intf.intf[i].scompr  += new->intf.intf[i].scompr;
	
			tot->intf.intf[i].type     = new->intf.intf[i].type;
			tot->intf.intf[i].speed    = new->intf.intf[i].speed;
			tot->intf.intf[i].duplex   = new->intf.intf[i].duplex;
		}
	
		tot->intf.intf[i].name[0] = '\0';
		tot->intf.nrintf          = i;

#if		HTTPSTATS
		tot->www.accesses  += new->www.accesses;
		tot->www.totkbytes += new->www.totkbytes;
		tot->www.bworkers   = new->www.bworkers;
		tot->www.iworkers   = new->www.iworkers;
#endif
		break;

	   case 'd':	/* accumulate disk-related counters */
		for (i=0; new->dsk.dsk[i].name[0]; i++)
		{
			strcpy(tot->dsk.dsk[i].name, new->dsk.dsk[i].name);
	
			tot->dsk.dsk[i].nread  += new->dsk.dsk[i].nread;
			tot->dsk.dsk[i].nwrite += new->dsk.dsk[i].nwrite;
			tot->dsk.dsk[i].nrsect += new->dsk.dsk[i].nrsect;
			tot->dsk.dsk[i].nwsect += new->dsk.dsk[i].nwsect;
			tot->dsk.dsk[i].io_ms  += new->dsk.dsk[i].io_ms;
			tot->dsk.dsk[i].avque  += new->dsk.dsk[i].avque;
		}
	
		tot->dsk.dsk[i].name[0] = '\0';
		tot->dsk.ndsk = i;

		for (i=0; new->dsk.lvm[i].name[0]; i++)
		{
			strcpy(tot->dsk.lvm[i].name, new->dsk.lvm[i].name);
	
			tot->dsk.lvm[i].nread  += new->dsk.lvm[i].nread;
			tot->dsk.lvm[i].nwrite += new->dsk.lvm[i].nwrite;
			tot->dsk.lvm[i].nrsect += new->dsk.lvm[i].nrsect;
			tot->dsk.lvm[i].nwsect += new->dsk.lvm[i].nwsect;
			tot->dsk.lvm[i].io_ms  += new->dsk.lvm[i].io_ms;
			tot->dsk.lvm[i].avque  += new->dsk.lvm[i].avque;
		}
	
		tot->dsk.lvm[i].name[0] = '\0';
		tot->dsk.nlvm = i;

		for (i=0; new->dsk.mdd[i].name[0]; i++)
		{
			strcpy(tot->dsk.mdd[i].name, new->dsk.mdd[i].name);
	
			tot->dsk.mdd[i].nread  += new->dsk.mdd[i].nread;
			tot->dsk.mdd[i].nwrite += new->dsk.mdd[i].nwrite;
			tot->dsk.mdd[i].nrsect += new->dsk.mdd[i].nrsect;
			tot->dsk.mdd[i].nwsect += new->dsk.mdd[i].nwsect;
			tot->dsk.mdd[i].io_ms  += new->dsk.mdd[i].io_ms;
			tot->dsk.mdd[i].avque  += new->dsk.mdd[i].avque;
		}
	
		tot->dsk.mdd[i].name[0] = '\0';
		tot->dsk.nmdd = i;
		break;
	}
}


/*
** Generic function to subtract two counters taking into 
** account the possibility of overflow of a 32-bit kernel-counter.
*/
count_t
subcount(count_t newval, count_t oldval)
{
	if (newval >= oldval)
		return newval - oldval;
	else
		return MAX32BITVAL + newval - oldval;
}
