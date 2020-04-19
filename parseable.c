/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        February 2007
** --------------------------------------------------------------------------
** Copyright (C) 2007-2010 Gerlof Langeveld
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
** $Id: parseable.c,v 1.13 2010/10/23 14:02:19 gerlof Exp $
** $Log: parseable.c,v $
** Revision 1.13  2010/10/23 14:02:19  gerlof
** Show counters for total number of running and sleep (S and D) threads.
**
** Revision 1.12  2010/05/18 19:20:55  gerlof
** Introduce CPU frequency and scaling (JC van Winkel).
**
** Revision 1.11  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.10  2010/03/04 10:52:21  gerlof
** Support I/O-statistics on logical volumes and MD devices.
**
** Revision 1.9  2010/01/08 14:46:42  gerlof
** Added label RESET in case of a sample with values since boot.
**
** Revision 1.8  2009/12/19 22:32:14  gerlof
** Add new counters to parseable output.
**
** Revision 1.7  2008/03/06 09:08:29  gerlof
** Bug-solution regarding parseable output of PPID.
**
** Revision 1.6  2008/03/06 08:38:03  gerlof
** Register/show ppid of a process.
**
** Revision 1.5  2008/01/18 08:03:40  gerlof
** Show information about the number of threads in state 'running',
** 'interruptible sleeping' and 'non-interruptible sleeping'.
**
** Revision 1.4  2007/12/11 13:33:12  gerlof
** Cosmetic change.
**
** Revision 1.3  2007/08/16 12:00:11  gerlof
** Add support for atopsar reporting.
** Concerns networking-counters that have been changed.
**
** Revision 1.2  2007/03/20 13:01:12  gerlof
** Introduction of variable supportflags.
**
** Revision 1.1  2007/02/19 11:55:43  gerlof
** Initial revision
**
*/
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include "atop.h"
#include "photosyst.h"
#include "photoproc.h"
#include "parseable.h"

void 	print_CPU();
void 	print_cpu();
void 	print_CPL();
void 	print_GPU();
void 	print_MEM();
void 	print_SWP();
void 	print_PAG();
void 	print_PSI();
void 	print_LVM();
void 	print_MDD();
void 	print_DSK();
void 	print_NFM();
void 	print_NFC();
void 	print_NFS();
void 	print_NET();
void 	print_IFB();

void 	print_PRG();
void 	print_PRC();
void 	print_PRM();
void 	print_PRD();
void 	print_PRN();
void 	print_PRE();

/*
** table with possible labels and the corresponding
** print-function for parseable output
*/
struct labeldef {
	char	*label;
	int	valid;
	void	(*prifunc)(char *, struct sstat *, struct tstat *, int);
};

static struct labeldef	labeldef[] = {
	{ "CPU",	0,	print_CPU },
	{ "cpu",	0,	print_cpu },
	{ "CPL",	0,	print_CPL },
	{ "GPU",	0,	print_GPU },
	{ "MEM",	0,	print_MEM },
	{ "SWP",	0,	print_SWP },
	{ "PAG",	0,	print_PAG },
	{ "PSI",	0,	print_PSI },
	{ "LVM",	0,	print_LVM },
	{ "MDD",	0,	print_MDD },
	{ "DSK",	0,	print_DSK },
	{ "NFM",	0,	print_NFM },
	{ "NFC",	0,	print_NFC },
	{ "NFS",	0,	print_NFS },
	{ "NET",	0,	print_NET },
	{ "IFB",	0,	print_IFB },

	{ "PRG",	0,	print_PRG },
	{ "PRC",	0,	print_PRC },
	{ "PRM",	0,	print_PRM },
	{ "PRD",	0,	print_PRD },
	{ "PRN",	0,	print_PRN },
	{ "PRE",	0,	print_PRE },
};

static int	numlabels = sizeof labeldef/sizeof(struct labeldef);

/*
** analyse the parse-definition string that has been
** passed as argument with the flag -P
*/
int
parsedef(char *pd)
{
	register int	i;
	char		*p, *ep = pd + strlen(pd);

	/*
	** check if string passed bahind -P is not another flag
	*/
	if (*pd == '-')
	{
		fprintf(stderr, "flag -P should be followed by label list\n");
		return 0;
	}

	/*
	** check list of comma-separated labels 
	*/
	while (pd < ep)
	{
		/*
		** exchange comma by null-byte
		*/
		if ( (p = strchr(pd, ',')) )
			*p = 0;	
		else
			p  = ep-1;

		/*
		** check if the next label exists
		*/
		for (i=0; i < numlabels; i++)
		{
			if ( strcmp(labeldef[i].label, pd) == 0)
			{
				labeldef[i].valid = 1;
				break;
			}
		}

		/*
		** non-existing label has been used
		*/
		if (i == numlabels)
		{
			/*
			** check if special label 'ALL' has been used
			*/
			if ( strcmp("ALL", pd) == 0)
			{
				for (i=0; i < numlabels; i++)
					labeldef[i].valid = 1;
				break;
			}
			else
			{
				fprintf(stderr, "label %s not found\n", pd);
				return 0;
			}
		}

		pd = p+1;
	}

	setbuf(stdout, (char *)0);

	return 1;
}

/*
** produce parseable output for an interval
*/
char
parseout(time_t curtime, int numsecs,
         struct devtstat *devtstat, struct sstat *sstat,
         int nexit, unsigned int noverflow, char flag)
{
	register int	i;
	char		datestr[32], timestr[32], header[256];

	/*
	** print reset-label for sample-values since boot
	*/
	if (flag&RRBOOT)
		printf("RESET\n");

	/*
	** search all labels which are selected before
	*/
	for (i=0; i < numlabels; i++)
	{
		if (labeldef[i].valid)
		{
			/*
			** prepare generic columns
			*/
			convdate(curtime, datestr);
			convtime(curtime, timestr);

			snprintf(header, sizeof header, "%s %s %ld %s %s %d",
				labeldef[i].label,
				utsname.nodename,
				curtime,
				datestr, timestr, numsecs);

			/*
			** call a selected print-function
			*/
			(labeldef[i].prifunc)(header, sstat,
				devtstat->taskall, devtstat->ntaskall);
		}
	}

	/*
	** print separator
	*/
	printf("SEP\n");

	return '\0';
}

/*
** print functions for system-level statistics
*/
void
calc_freqscale(count_t maxfreq, count_t cnt, count_t ticks, 
               count_t *freq, int *freqperc)
{
        // if ticks != 0, do full calcs
        if (maxfreq && ticks) 
        {
            *freq=cnt/ticks;
            *freqperc=100* *freq / maxfreq;
        } 
        else if (maxfreq)   // max frequency is known so % can be calculated
        {
            *freq=cnt;
            *freqperc=100*cnt/maxfreq;
        }
        else if (cnt)       // no max known, set % to 100
        {
            *freq=cnt;
            *freqperc=100;
        }
        else                // nothing is known: set freq to 0, % to 100
        {
            *freq=0;
            *freqperc=100;
        }
}


void
print_CPU(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
        count_t maxfreq=0;
        count_t cnt=0;
        count_t ticks=0;
        count_t freq;
        int freqperc;
        int i;

        // calculate average clock frequency
	for (i=0; i < ss->cpu.nrcpu; i++)
        {
                cnt    += ss->cpu.cpu[i].freqcnt.cnt;
                ticks  += ss->cpu.cpu[i].freqcnt.ticks;
        }
        maxfreq = ss->cpu.cpu[0].freqcnt.maxfreq;
        calc_freqscale(maxfreq, cnt, ticks, &freq, &freqperc);

	if (ss->cpu.all.instr == 1)
	{
        	ss->cpu.all.instr = 0;
        	ss->cpu.all.cycle = 0;
	}

	printf("%s %u %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %d %lld %lld\n",
			hp,
			hertz,
	        	ss->cpu.nrcpu,
	        	ss->cpu.all.stime,
        		ss->cpu.all.utime,
        		ss->cpu.all.ntime,
        		ss->cpu.all.itime,
        		ss->cpu.all.wtime,
        		ss->cpu.all.Itime,
        		ss->cpu.all.Stime,
        		ss->cpu.all.steal,
        		ss->cpu.all.guest,
                        freq,
                        freqperc,
        		ss->cpu.all.instr,
        		ss->cpu.all.cycle
                        );
}

void
print_cpu(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int i;
        count_t maxfreq=0;
        count_t cnt=0;
        count_t ticks=0;
        count_t freq;
        int freqperc;

	for (i=0; i < ss->cpu.nrcpu; i++)
	{
                cnt    = ss->cpu.cpu[i].freqcnt.cnt;
                ticks  = ss->cpu.cpu[i].freqcnt.ticks;
                maxfreq= ss->cpu.cpu[0].freqcnt.maxfreq;

                calc_freqscale(maxfreq, cnt, ticks, &freq, &freqperc);

		printf("%s %u %d %lld %lld %lld "
		       "%lld %lld %lld %lld %lld %lld %lld %d %lld %lld\n",
			hp, hertz, i,
	        	ss->cpu.cpu[i].stime,
        		ss->cpu.cpu[i].utime,
        		ss->cpu.cpu[i].ntime,
        		ss->cpu.cpu[i].itime,
        		ss->cpu.cpu[i].wtime,
        		ss->cpu.cpu[i].Itime,
        		ss->cpu.cpu[i].Stime,
        		ss->cpu.cpu[i].steal,
        		ss->cpu.cpu[i].guest,
                        freq,
                        freqperc,
        		ss->cpu.cpu[i].instr,
        		ss->cpu.cpu[i].cycle
			);
	}
}

void
print_CPL(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	printf("%s %lld %.2f %.2f %.2f %lld %lld\n",
			hp,
	        	ss->cpu.nrcpu,
	        	ss->cpu.lavg1,
	        	ss->cpu.lavg5,
	        	ss->cpu.lavg15,
	        	ss->cpu.csw,
	        	ss->cpu.devint);
}

void
print_GPU(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	int	i;

	for (i=0; i < ss->gpu.nrgpus; i++)
	{
		printf("%s %d %s %s %d %d %lld %lld %lld %lld %lld %lld\n",
			hp, i,
	        	ss->gpu.gpu[i].busid,
	        	ss->gpu.gpu[i].type,
	        	ss->gpu.gpu[i].gpupercnow,
	        	ss->gpu.gpu[i].mempercnow,
	        	ss->gpu.gpu[i].memtotnow,
	        	ss->gpu.gpu[i].memusenow,
	        	ss->gpu.gpu[i].samples,
	        	ss->gpu.gpu[i].gpuperccum,
	        	ss->gpu.gpu[i].memperccum,
	        	ss->gpu.gpu[i].memusecum);
	}
}

void
print_MEM(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	printf(	"%s %u %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld\n",
			hp,
			pagesize,
			ss->mem.physmem,
			ss->mem.freemem,
			ss->mem.cachemem,
			ss->mem.buffermem,
			ss->mem.slabmem,
			ss->mem.cachedrt,
			ss->mem.slabreclaim,
        		ss->mem.vmwballoon,
        		ss->mem.shmem,
        		ss->mem.shmrss,
        		ss->mem.shmswp,
        		ss->mem.hugepagesz,
        		ss->mem.tothugepage,
        		ss->mem.freehugepage);
}

void
print_SWP(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	printf(	"%s %u %lld %lld %lld %lld %lld\n",
			hp,
			pagesize,
			ss->mem.totswap,
			ss->mem.freeswap,
			(long long)0,
			ss->mem.committed,
			ss->mem.commitlim);
}

void
print_PAG(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	printf("%s %u %lld %lld %lld %lld %lld %lld\n",
			hp,
			pagesize,
			ss->mem.pgscans,
			ss->mem.allocstall,
			ss->mem.compact_stall,
			(long long)0,
			ss->mem.swins,
			ss->mem.swouts);
}

void
print_PSI(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	printf("%s %c %.1f %.1f %.1f %llu %.1f %.1f %.1f %llu "
	       "%.1f %.1f %.1f %llu %.1f %.1f %.1f %llu %.1f %.1f %.1f %llu\n",
		hp, ss->psi.present ? 'y' : 'n',
                ss->psi.cpusome.avg10, ss->psi.cpusome.avg60,
                ss->psi.cpusome.avg300, ss->psi.cpusome.total,
                ss->psi.memsome.avg10, ss->psi.memsome.avg60,
                ss->psi.memsome.avg300, ss->psi.memsome.total,
                ss->psi.memfull.avg10, ss->psi.memfull.avg60,
                ss->psi.memfull.avg300, ss->psi.memfull.total,
                ss->psi.iosome.avg10, ss->psi.iosome.avg60,
                ss->psi.iosome.avg300, ss->psi.iosome.total,
                ss->psi.iofull.avg10, ss->psi.iofull.avg60,
                ss->psi.iofull.avg300, ss->psi.iofull.total);
}

void
print_LVM(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int	i;

        for (i=0; ss->dsk.lvm[i].name[0]; i++)
	{
		printf(	"%s %s %lld %lld %lld %lld %lld\n",
			hp,
			ss->dsk.lvm[i].name,
			ss->dsk.lvm[i].io_ms,
			ss->dsk.lvm[i].nread,
			ss->dsk.lvm[i].nrsect,
			ss->dsk.lvm[i].nwrite,
			ss->dsk.lvm[i].nwsect);
	}
}

void
print_MDD(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int	i;

        for (i=0; ss->dsk.mdd[i].name[0]; i++)
	{
		printf(	"%s %s %lld %lld %lld %lld %lld\n",
			hp,
			ss->dsk.mdd[i].name,
			ss->dsk.mdd[i].io_ms,
			ss->dsk.mdd[i].nread,
			ss->dsk.mdd[i].nrsect,
			ss->dsk.mdd[i].nwrite,
			ss->dsk.mdd[i].nwsect);
	}
}

void
print_DSK(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int	i;

        for (i=0; ss->dsk.dsk[i].name[0]; i++)
	{
		printf(	"%s %s %lld %lld %lld %lld %lld\n",
			hp,
			ss->dsk.dsk[i].name,
			ss->dsk.dsk[i].io_ms,
			ss->dsk.dsk[i].nread,
			ss->dsk.dsk[i].nrsect,
			ss->dsk.dsk[i].nwrite,
			ss->dsk.dsk[i].nwsect);
	}
}

void
print_NFM(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int	i;

        for (i=0; i < ss->nfs.nfsmounts.nrmounts; i++)
	{
		printf("%s %s %lld %lld %lld %lld %lld %lld %lld %lld\n",
			hp,
			ss->nfs.nfsmounts.nfsmnt[i].mountdev,
			ss->nfs.nfsmounts.nfsmnt[i].bytestotread,
			ss->nfs.nfsmounts.nfsmnt[i].bytestotwrite,
			ss->nfs.nfsmounts.nfsmnt[i].bytesread,
			ss->nfs.nfsmounts.nfsmnt[i].byteswrite,
			ss->nfs.nfsmounts.nfsmnt[i].bytesdread,
			ss->nfs.nfsmounts.nfsmnt[i].bytesdwrite,
			ss->nfs.nfsmounts.nfsmnt[i].pagesmread,
			ss->nfs.nfsmounts.nfsmnt[i].pagesmwrite);
	}
}

void
print_NFC(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	printf(	"%s %lld %lld %lld %lld %lld\n",
			hp,
			ss->nfs.client.rpccnt,
			ss->nfs.client.rpcread,
			ss->nfs.client.rpcwrite,
			ss->nfs.client.rpcretrans,
			ss->nfs.client.rpcautrefresh);
}

void
print_NFS(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	printf(	"%s %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld "
	        "%lld %lld %lld %lld %lld\n",
			hp,
			ss->nfs.server.rpccnt,
			ss->nfs.server.rpcread,
			ss->nfs.server.rpcwrite,
			ss->nfs.server.nrbytes,
			ss->nfs.server.nwbytes,
			ss->nfs.server.rpcbadfmt,
			ss->nfs.server.rpcbadaut,
			ss->nfs.server.rpcbadcln,
			ss->nfs.server.netcnt,
			ss->nfs.server.nettcpcnt,
			ss->nfs.server.netudpcnt,
			ss->nfs.server.nettcpcon,
			ss->nfs.server.rchits,
			ss->nfs.server.rcmiss,
			ss->nfs.server.rcnoca);
}

void
print_NET(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int 	i;

	printf(	"%s %s %lld %lld %lld %lld %lld %lld %lld %lld\n",
			hp,
			"upper",
        		ss->net.tcp.InSegs,
   		     	ss->net.tcp.OutSegs,
       		 	ss->net.udpv4.InDatagrams +
				ss->net.udpv6.Udp6InDatagrams,
       		 	ss->net.udpv4.OutDatagrams +
				ss->net.udpv6.Udp6OutDatagrams,
       		 	ss->net.ipv4.InReceives  +
				ss->net.ipv6.Ip6InReceives,
       		 	ss->net.ipv4.OutRequests +
				ss->net.ipv6.Ip6OutRequests,
       		 	ss->net.ipv4.InDelivers +
       		 		ss->net.ipv6.Ip6InDelivers,
       		 	ss->net.ipv4.ForwDatagrams +
       		 		ss->net.ipv6.Ip6OutForwDatagrams);

	for (i=0; ss->intf.intf[i].name[0]; i++)
	{
		printf(	"%s %s %lld %lld %lld %lld %ld %d\n",
			hp,
			ss->intf.intf[i].name,
			ss->intf.intf[i].rpack,
			ss->intf.intf[i].rbyte,
			ss->intf.intf[i].spack,
			ss->intf.intf[i].sbyte,
			ss->intf.intf[i].speed,
			ss->intf.intf[i].duplex);
	}
}

void
print_IFB(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int 	i;

	for (i=0; i < ss->ifb.nrports; i++)
	{
		printf(	"%s %s %hd %hd %lld %lld %lld %lld %lld\n",
			hp,
			ss->ifb.ifb[i].ibname,
			ss->ifb.ifb[i].portnr,
			ss->ifb.ifb[i].lanes,
			ss->ifb.ifb[i].rate,
			ss->ifb.ifb[i].rcvb,
			ss->ifb.ifb[i].sndb,
			ss->ifb.ifb[i].rcvp,
			ss->ifb.ifb[i].sndp);
	}
}


/*
** print functions for process-level statistics
*/
void
print_PRG(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int i, exitcode;

	for (i=0; i < nact; i++, ps++)
	{
		if (ps->gen.excode & 0xff)      // killed by signal?
			exitcode = (ps->gen.excode & 0x7f) + 256;
		else
			exitcode = (ps->gen.excode >>   8) & 0xff;

		printf("%s %d (%s) %c %d %d %d %d %d %ld (%s) %d %d %d %d "
 		       "%d %d %d %d %d %d %ld %c %d %d %s\n",
			hp,
			ps->gen.pid,
			ps->gen.name,
			ps->gen.state,
			ps->gen.ruid,
			ps->gen.rgid,
			ps->gen.tgid,
			ps->gen.nthr,
			exitcode,
			ps->gen.btime,
			ps->gen.cmdline,
			ps->gen.ppid,
			ps->gen.nthrrun,
			ps->gen.nthrslpi,
			ps->gen.nthrslpu,
			ps->gen.euid,
			ps->gen.egid,
			ps->gen.suid,
			ps->gen.sgid,
			ps->gen.fsuid,
			ps->gen.fsgid,
			ps->gen.elaps,
			ps->gen.isproc ? 'y':'n',
			ps->gen.vpid,
			ps->gen.ctid,
			ps->gen.container[0] ? ps->gen.container:"-");
	}
}

void
print_PRC(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int i;

	for (i=0; i < nact; i++, ps++)
	{
		printf("%s %d (%s) %c %u %lld %lld %d %d %d %d %d %d %d %c\n",
				hp,
				ps->gen.pid,
				ps->gen.name,
				ps->gen.state,
				hertz,
				ps->cpu.utime,
				ps->cpu.stime,
				ps->cpu.nice,
				ps->cpu.prio,
				ps->cpu.rtprio,
				ps->cpu.policy,
				ps->cpu.curcpu,
				ps->cpu.sleepavg,
				ps->gen.tgid,
				ps->gen.isproc ? 'y':'n');
	}
}

void
print_PRM(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int i;

	for (i=0; i < nact; i++, ps++)
	{
		printf("%s %d (%s) %c %u %lld %lld %lld %lld %lld %lld "
		       "%lld %lld %lld %lld %lld %d %c %lld %lld\n",
				hp,
				ps->gen.pid,
				ps->gen.name,
				ps->gen.state,
				pagesize,
				ps->mem.vmem,
				ps->mem.rmem,
				ps->mem.vexec,
				ps->mem.vgrow,
				ps->mem.rgrow,
				ps->mem.minflt,
				ps->mem.majflt,
				ps->mem.vlibs,
				ps->mem.vdata,
				ps->mem.vstack,
				ps->mem.vswap,
				ps->gen.tgid,
				ps->gen.isproc ? 'y':'n',
				ps->mem.pmem == (unsigned long long)-1LL ?
								0:ps->mem.pmem,
				ps->mem.vlock);
	}
}

void
print_PRD(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int i;

	for (i=0; i < nact; i++, ps++)
	{
		printf("%s %d (%s) %c %c %c %lld %lld %lld %lld %lld %d n %c\n",
				hp,
				ps->gen.pid,
				ps->gen.name,
				ps->gen.state,
				'n',
				supportflags & IOSTAT ? 'y' : 'n',
				ps->dsk.rio, ps->dsk.rsz,
				ps->dsk.wio, ps->dsk.wsz, ps->dsk.cwsz,
				ps->gen.tgid,
				ps->gen.isproc ? 'y':'n');
	}
}

void
print_PRN(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int i;

	for (i=0; i < nact; i++, ps++)
	{
		printf("%s %d (%s) %c %c %lld %lld %lld %lld %lld %lld "
		       "%lld %lld %d %d %d %c\n",
				hp,
				ps->gen.pid,
				ps->gen.name,
				ps->gen.state,
				supportflags & NETATOP ? 'y' : 'n',
				ps->net.tcpsnd, ps->net.tcpssz,
				ps->net.tcprcv, ps->net.tcprsz,
				ps->net.udpsnd, ps->net.udpssz,
				ps->net.udprcv, ps->net.udprsz,
				0,              0,
				ps->gen.tgid,   ps->gen.isproc ? 'y':'n');
	}
}

void
print_PRE(char *hp, struct sstat *ss, struct tstat *ps, int nact)
{
	register int i;

	for (i=0; i < nact; i++, ps++)
	{
		printf("%s %d (%s) %c %c %d %x %d %d %lld %lld %lld\n",
				hp,
				ps->gen.pid,
				ps->gen.name,
				ps->gen.state,
				ps->gpu.state == '\0' ? 'N':ps->gpu.state,
				ps->gpu.nrgpus,
				ps->gpu.gpulist,
				ps->gpu.gpubusy,
				ps->gpu.membusy,
				ps->gpu.memnow,
				ps->gpu.memcum,
				ps->gpu.sample);
	}
}
