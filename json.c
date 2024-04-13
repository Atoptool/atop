/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** ==========================================================================
** Author:      Fei Li & zhenwei pi
** E-mail:      lifei.shirley@bytedance.com, pizhenwei@bytedance.com
** Date:        August 2019
** --------------------------------------------------------------------------
** Copyright (C) 2019-2022 bytedance.com
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
** Revision 1.1  2019/08/08 14:02:19
** Initial revision
** Add support for json style output, basing on the parseable.c file.
**
*/
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <limits.h>

#include "atop.h"
#include "photosyst.h"
#include "photoproc.h"
#include "cgroups.h"
#include "json.h"

#define LEN_HP_SIZE	64
#define LINE_BUF_SIZE	1024

static void json_print_CPU(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_cpu(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_CPL(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_GPU(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_MEM(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_SWP(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_PAG(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_PSI(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_LVM(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_MDD(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_DSK(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_NFM(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_NFC(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_NFS(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_NET(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_IFB(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_NUM(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_NUC(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_LLC(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);

static void json_print_CGR(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);

static void json_print_PRG(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_PRC(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_PRM(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_PRD(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_PRN(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);
static void json_print_PRE(char *, struct sstat *, struct tstat *, int,
                                                   struct cgchainer *, int);

/*
** table with possible labels and the corresponding
** print-function for json style output
*/
struct labeldef {
	char	*label;
	short   valid;
	short   cgroupref;
	void	(*prifunc)(char *, struct sstat *,
	                           struct tstat *, int,
                                   struct cgchainer *, int);
};

static struct labeldef	labeldef[] = {
	{ "CPU",	0, 0,	json_print_CPU },
	{ "cpu",	0, 0,	json_print_cpu },
	{ "CPL",	0, 0,	json_print_CPL },
	{ "GPU",	0, 0,	json_print_GPU },
	{ "MEM",	0, 0,	json_print_MEM },
	{ "SWP",	0, 0,	json_print_SWP },
	{ "PAG",	0, 0,	json_print_PAG },
	{ "PSI",	0, 0,	json_print_PSI },
	{ "LVM",	0, 0,	json_print_LVM },
	{ "MDD",	0, 0,	json_print_MDD },
	{ "DSK",	0, 0,	json_print_DSK },
	{ "NFM",	0, 0,	json_print_NFM },
	{ "NFC",	0, 0,	json_print_NFC },
	{ "NFS",	0, 0,	json_print_NFS },
	{ "NET",	0, 0,	json_print_NET },
	{ "IFB",	0, 0,	json_print_IFB },
	{ "NUM",	0, 0,	json_print_NUM },
	{ "NUC",	0, 0,	json_print_NUC },
	{ "LLC",	0, 0,	json_print_LLC },

	{ "CGR",	0, 0,	json_print_CGR },

	{ "PRG",	0, 1,	json_print_PRG },
	{ "PRC",	0, 1,	json_print_PRC },
	{ "PRM",	0, 1,	json_print_PRM },
	{ "PRD",	0, 0,	json_print_PRD },
	{ "PRN",	0, 0,	json_print_PRN },
	{ "PRE",	0, 0,	json_print_PRE },
};

static int numlabels = sizeof labeldef / sizeof(struct labeldef);

/*
** analyse the json-definition string that has been
** passed as argument with the flag -J
*/
int jsondef(char *pd)
{
	register int	i;
	char		*p, *ep = pd + strlen(pd);

	/*
	** check if string passed behind -J is not another flag
	*/
	if (*pd == '-')
	{
		printf("json labels should be followed by label list\n");
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
				printf("json labels not supported: %s\n", pd);
				return 0;
			}
		}

		pd = p+1;
	}

	setbuf(stdout, (char *)0);

	return 1;
}

/*
** produce json output for an interval
*/
char jsonout(time_t curtime, int numsecs,
         struct devtstat *devtstat, struct sstat *sstat,
	 struct cgchainer *devchain, int ncgroups, int npids,
         int nexit, unsigned int noverflow, char flag)
{
	register int	i, j, k, cgroupref_created = 0;
	char		header[256];
	struct tstat	*tmp = devtstat->taskall;

	printf("{\"host\": \"%s\", "
		"\"timestamp\": %ld, "
		"\"elapsed\": %d",
		utsname.nodename,
		curtime,
		numsecs
		);

	/* Replace " with # in case json can not parse this out */
	for (k = 0; k < devtstat->ntaskall; k++, tmp++) {
		for (j = 0; (j < sizeof(tmp->gen.name)) && tmp->gen.name[j]; j++)
			if ((tmp->gen.name[j] == '\"') || (tmp->gen.name[j] == '\\'))
				tmp->gen.name[j] = '#';

		for (j = 0; (j < sizeof(tmp->gen.cmdline) && tmp->gen.cmdline[j]); j++)
			if ((tmp->gen.cmdline[j] == '\"') || (tmp->gen.cmdline[j] == '\\'))
				tmp->gen.cmdline[j] = '#';
	}

	/*
	** iterate all labels defined in labeldef[]
	*/
	for (i=0; i < numlabels; i++) {
		if (!labeldef[i].valid)
			continue;

		/*
		** when cgroup index is needed to map the tstat to a cgroup,
		** once fill the tstat.gen.cgroupix variables
		*/
		if (supportflags & CGROUPV2 &&
		    labeldef[i].cgroupref   && !cgroupref_created)
		{
			cgfillref(devtstat, devchain, ncgroups, npids);
			cgroupref_created = 1;
		}

		/* prepare generic columns */
		snprintf(header, sizeof header, "\"%s\"",
			labeldef[i].label);

		/* call all print-functions */
		(labeldef[i].prifunc)(header, sstat,
				devtstat->taskall, devtstat->ntaskall,
				devchain, ncgroups);
	}

	printf("}\n");

	return '\0';
}

/*
** print functions for system-level statistics
*/
static void json_calc_freqscale(count_t maxfreq, count_t cnt, count_t ticks,
               count_t *freq, int *freqperc)
{
	// if ticks != 0,do full calcs
	if (maxfreq && ticks) {
		*freq = cnt/ticks;
		*freqperc = 100* *freq / maxfreq;
	} else if (maxfreq) { // max frequency is known so % can be calculated
		*freq = cnt;
		*freqperc = 100 * cnt / maxfreq;
	} else if (cnt) {   // no max known, set % to 100
		*freq = cnt;
		*freqperc = 100;
	} else {            // nothing is known: set freq to 0, % to 100
		*freq = 0;
		*freqperc = 100;
	}
}

static void
json_print_CPU(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	count_t maxfreq=0;
	count_t cnt=0;
	count_t ticks=0;
	count_t freq;
	int freqperc;
	int i;

	// calculate average clock frequency
	for (i = 0; i < ss->cpu.nrcpu; i++) {
		cnt += ss->cpu.cpu[i].freqcnt.cnt;
		ticks += ss->cpu.cpu[i].freqcnt.ticks;
	}
	maxfreq = ss->cpu.cpu[0].freqcnt.maxfreq;
	json_calc_freqscale(maxfreq, cnt, ticks, &freq, &freqperc);

	if (ss->cpu.all.instr == 1) {
		ss->cpu.all.instr = 0;
		ss->cpu.all.cycle = 0;
	}

	printf(", %s: {"
		"\"hertz\": %u, "
		"\"nrcpu\": %lld, "
		"\"stime\": %lld, "
		"\"utime\": %lld, "
		"\"ntime\": %lld, "
		"\"itime\": %lld, "
		"\"wtime\": %lld, "
		"\"Itime\": %lld, "
		"\"Stime\": %lld, "
		"\"steal\": %lld, "
		"\"guest\": %lld, "
		"\"freq\": %lld, "
		"\"freqperc\": %d, "
		"\"instr\": %lld, "
		"\"cycle\": %lld}",
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

static void
json_print_cpu(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;
	count_t maxfreq=0;
	count_t cnt=0;
	count_t ticks=0;
	count_t freq;
	int freqperc;

	printf(", %s: [", hp);

	for (i = 0; i < ss->cpu.nrcpu; i++) {
		if (i > 0) {
			printf(", ");
		}
		cnt = ss->cpu.cpu[i].freqcnt.cnt;
		ticks = ss->cpu.cpu[i].freqcnt.ticks;
		maxfreq= ss->cpu.cpu[0].freqcnt.maxfreq;

		json_calc_freqscale(maxfreq, cnt, ticks, &freq, &freqperc);

		printf("{\"cpuid\": %d, "
			"\"stime\": %lld, "
			"\"utime\": %lld, "
			"\"ntime\": %lld, "
			"\"itime\": %lld, "
			"\"wtime\": %lld, "
			"\"Itime\": %lld, "
			"\"Stime\": %lld, "
			"\"steal\": %lld, "
			"\"guest\": %lld, "
			"\"freq\": %lld, "
			"\"freqperc\": %d, "
			"\"instr\": %lld, "
			"\"cycle\": %lld}",
			i,
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
			ss->cpu.cpu[i].cycle);
	}

	printf("]");
}

static void
json_print_CPL(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	printf(", %s: {"
		"\"lavg1\": %.2f, "
		"\"lavg5\": %.2f, "
		"\"lavg15\": %.2f, "
		"\"csw\": %lld, "
		"\"devint\": %lld}",
		hp,
		ss->cpu.lavg1,
		ss->cpu.lavg5,
		ss->cpu.lavg15,
		ss->cpu.csw,
		ss->cpu.devint);
}

static void
json_print_GPU(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	int	i;

	printf(", %s: [", hp);

	for (i = 0; i < ss->gpu.nrgpus; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"gpuid\": %d, "
			"\"busid\": \"%.19s\", "
			"\"type\": \"%.19s\", "
			"\"gpupercnow\": %d, "
			"\"mempercnow\": %d, "
			"\"memtotnow\": %lld, "
			"\"memusenow\": %lld, "
			"\"samples\": %lld, "
			"\"gpuperccum\": %lld, "
			"\"memperccum\": %lld, "
			"\"memusecum\": %lld}",
			i,
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

	printf("]");
}

static void
json_print_MEM(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	printf(", %s: {"
		"\"physmem\": %lld, "
		"\"freemem\": %lld, "
		"\"cachemem\": %lld, "
		"\"buffermem\": %lld, "
		"\"slabmem\": %lld, "
		"\"cachedrt\": %lld, "
		"\"slabreclaim\": %lld, "
		"\"vmwballoon\": %lld, "
		"\"shmem\": %lld, "
		"\"shmrss\": %lld, "
		"\"shmswp\": %lld, "
		"\"hugepagesz\": %lld, "
		"\"tothugepage\": %lld, "
		"\"freehugepage\": %lld, "
		"\"lhugepagesz\": %lld, "
		"\"ltothugepage\": %lld, "
		"\"lfreehugepage\": %lld, "
		"\"availablemem\": %lld, "
		"\"anonhugepage\": %lld}",
		hp,
		ss->mem.physmem * pagesize,
		ss->mem.freemem * pagesize,
		ss->mem.cachemem * pagesize,
		ss->mem.buffermem * pagesize,
		ss->mem.slabmem * pagesize,
		ss->mem.cachedrt * pagesize,
		ss->mem.slabreclaim * pagesize,
		ss->mem.vmwballoon * pagesize,
		ss->mem.shmem * pagesize,
		ss->mem.shmrss * pagesize,
		ss->mem.shmswp * pagesize,
		ss->mem.shugepagesz,
		ss->mem.stothugepage,
		ss->mem.sfreehugepage,
		ss->mem.lhugepagesz,
		ss->mem.ltothugepage,
		ss->mem.lfreehugepage,
		ss->mem.availablemem * pagesize,
		ss->mem.anonhugepage * pagesize);
}

static void
json_print_SWP(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	printf(", %s: {"
		"\"totswap\": %lld, "
		"\"freeswap\": %lld, "
		"\"swcac\": %lld, "
		"\"committed\": %lld, "
		"\"commitlim\": %lld}",
		hp,
		ss->mem.totswap * pagesize,
		ss->mem.freeswap * pagesize,
		ss->mem.swapcached * pagesize,
		ss->mem.committed * pagesize,
		ss->mem.commitlim * pagesize);
}

static void
json_print_PAG(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	printf(", %s: {"
		"\"compacts\": %lld, "
		"\"numamigs\": %lld, "
		"\"migrates\": %lld, "
		"\"pgscans\": %lld, "
		"\"allocstall\": %lld, "
		"\"pgins\": %lld, "
		"\"pgouts\": %lld, "
		"\"swins\": %lld, "
		"\"swouts\": %lld, "
		"\"oomkills\": %lld}",
		hp,
		ss->mem.compactstall,
		ss->mem.numamigrate,
		ss->mem.pgmigrate,
		ss->mem.pgscans,
		ss->mem.allocstall,
		ss->mem.pgins,
		ss->mem.pgouts,
		ss->mem.swins,
		ss->mem.swouts,
		ss->mem.oomkills);
}

static void
json_print_PSI(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	if ( !(ss->psi.present) )
		return;

	printf(", %s: {"
		"\"psi\": \"%c\", "
		"\"cs10\": %.1f, "
		"\"cs60\": %.1f, "
		"\"cs300\": %.1f, "
		"\"cstot\": %llu, "
		"\"ms10\": %.1f, "
		"\"ms60\": %.1f, "
		"\"ms300\": %.1f, "
		"\"mstot\": %llu, "
		"\"mf10\": %.1f, "
		"\"mf60\": %.1f, "
		"\"mf300\": %.1f, "
		"\"mftot\": %llu, "
		"\"ios10\": %.1f, "
		"\"ios60\": %.1f, "
		"\"ios300\": %.1f, "
		"\"iostot\": %llu, "
		"\"iof10\": %.1f, "
		"\"iof60\": %.1f, "
		"\"iof300\": %.1f, "
		"\"ioftot\": %llu}",
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

static void
json_print_LVM(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int	i;

	printf(", %s: [", hp);

	for (i = 0; ss->dsk.lvm[i].name[0]; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"lvmname\": \"%.19s\", "
			"\"io_ms\": %lld, "
			"\"nread\": %lld, "
			"\"nrsect\": %lld, "
			"\"nwrite\": %lld, "
			"\"nwsect\": %lld, "
			"\"avque\": %lld, "
			"\"inflight\": %lld}",
			ss->dsk.lvm[i].name,
			ss->dsk.lvm[i].io_ms,
			ss->dsk.lvm[i].nread,
			ss->dsk.lvm[i].nrsect,
			ss->dsk.lvm[i].nwrite,
			ss->dsk.lvm[i].nwsect,
			ss->dsk.lvm[i].avque,
			ss->dsk.lvm[i].inflight);
	}

	printf("]");
}

static void
json_print_MDD(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;

	printf(", %s: [", hp);

	for (i = 0; ss->dsk.mdd[i].name[0]; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"mddname\": \"%.19s\", "
			"\"io_ms\": %lld, "
			"\"nread\": %lld, "
			"\"nrsect\": %lld, "
			"\"nwrite\": %lld, "
			"\"nwsect\": %lld, "
			"\"avque\": %lld, "
			"\"inflight\": %lld}",
			ss->dsk.mdd[i].name,
			ss->dsk.mdd[i].io_ms,
			ss->dsk.mdd[i].nread,
			ss->dsk.mdd[i].nrsect,
			ss->dsk.mdd[i].nwrite,
			ss->dsk.mdd[i].nwsect,
			ss->dsk.mdd[i].avque,
			ss->dsk.mdd[i].inflight);
	}

	printf("]");
}

static void
json_print_DSK(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int	i;

        printf(", %s: [", hp);

	for (i = 0; ss->dsk.dsk[i].name[0]; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"dskname\": \"%.19s\", "
			"\"io_ms\": %lld, "
			"\"nread\": %lld, "
			"\"nrsect\": %lld, "
			"\"ndiscrd\": %lld, "
			"\"nwrite\": %lld, "
			"\"nwsect\": %lld, "
			"\"avque\": %lld, "
			"\"inflight\": %lld}",
			ss->dsk.dsk[i].name,
			ss->dsk.dsk[i].io_ms,
			ss->dsk.dsk[i].nread,
			ss->dsk.dsk[i].nrsect,
			ss->dsk.dsk[i].ndisc,
			ss->dsk.dsk[i].nwrite,
			ss->dsk.dsk[i].nwsect,
			ss->dsk.dsk[i].avque,
			ss->dsk.dsk[i].inflight);
	}

	printf("]");
}

static void
json_print_NFM(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int	i;

        printf(", %s: [", hp);

	for (i = 0; i < ss->nfs.nfsmounts.nrmounts; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"mountdev\": \"%.19s\", "
			"\"bytestotread\": %lld, "
			"\"bytestotwrite\": %lld, "
			"\"bytesread\": %lld, "
			"\"byteswrite\": %lld, "
			"\"bytesdread\": %lld, "
			"\"bytesdwrite\": %lld, "
			"\"pagesmread\": %lld, "
			"\"pagesmwrite\": %lld}",
			ss->nfs.nfsmounts.nfsmnt[i].mountdev,
			ss->nfs.nfsmounts.nfsmnt[i].bytestotread,
			ss->nfs.nfsmounts.nfsmnt[i].bytestotwrite,
			ss->nfs.nfsmounts.nfsmnt[i].bytesread,
			ss->nfs.nfsmounts.nfsmnt[i].byteswrite,
			ss->nfs.nfsmounts.nfsmnt[i].bytesdread,
			ss->nfs.nfsmounts.nfsmnt[i].bytesdwrite,
			ss->nfs.nfsmounts.nfsmnt[i].pagesmread * pagesize,
			ss->nfs.nfsmounts.nfsmnt[i].pagesmwrite * pagesize);
	}

	printf("]");
}

static void
json_print_NFC(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	printf(", %s: {"
		"\"rpccnt\": %lld, "
		"\"rpcread\": %lld, "
		"\"rpcwrite\": %lld, "
		"\"rpcretrans\": %lld, "
		"\"rpcautrefresh\": %lld}",
		hp,
		ss->nfs.client.rpccnt,
		ss->nfs.client.rpcread,
		ss->nfs.client.rpcwrite,
		ss->nfs.client.rpcretrans,
		ss->nfs.client.rpcautrefresh);
}

static void
json_print_NFS(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	printf(", %s: {"
		"\"rpccnt\": %lld, "
		"\"rpcread\": %lld, "
		"\"rpcwrite\": %lld, "
		"\"nrbytes\": %lld, "
		"\"nwbytes\": %lld, "
		"\"rpcbadfmt\": %lld, "
		"\"rpcbadaut\": %lld, "
		"\"rpcbadcln\": %lld, "
		"\"netcnt\": %lld, "
		"\"nettcpcnt\": %lld, "
		"\"netudpcnt\": %lld, "
		"\"nettcpcon\": %lld, "
		"\"rchits\": %lld, "
		"\"rcmiss\": %lld, "
		"\"rcnocache\": %lld}",
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

static void
json_print_NET(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;

	printf(", \"NET_GENERAL\": {"
		"\"rpacketsTCP\": %lld, "
		"\"spacketsTCP\": %lld, "
		"\"activeOpensTCP\": %lld, "
		"\"passiveOpensTCP\": %lld, "
		"\"retransSegsTCP\": %lld, "
		"\"rpacketsUDP\": %lld, "
		"\"spacketsUDP\": %lld, "
		"\"rpacketsIP\": %lld, "
		"\"spacketsIP\": %lld, "
		"\"dpacketsIP\": %lld, "
		"\"fpacketsIP\": %lld, "
		"\"icmpi\" : %lld, "
		"\"icmpo\" : %lld}",
		ss->net.tcp.InSegs,
		ss->net.tcp.OutSegs,
		ss->net.tcp.ActiveOpens,
		ss->net.tcp.PassiveOpens,
		ss->net.tcp.RetransSegs,
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
		ss->net.ipv6.Ip6OutForwDatagrams,
		ss->net.icmpv4.InMsgs +
		ss->net.icmpv6.Icmp6InMsgs,
		ss->net.icmpv4.OutMsgs +
		ss->net.icmpv6.Icmp6OutMsgs);

        printf(", %s: [", hp);

	for (i = 0; ss->intf.intf[i].name[0]; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"name\": \"%.19s\", "
			"\"rpack\": %lld, "
			"\"rbyte\": %lld, "
			"\"rerrs\": %lld, "
			"\"spack\": %lld, "
			"\"sbyte\": %lld, "
			"\"serrs\": %lld, "
			"\"speed\": \"%ld\", "
			"\"duplex\": %d}",
			ss->intf.intf[i].name,
			ss->intf.intf[i].rpack,
			ss->intf.intf[i].rbyte,
			ss->intf.intf[i].rerrs,
			ss->intf.intf[i].spack,
			ss->intf.intf[i].sbyte,
			ss->intf.intf[i].serrs,
			ss->intf.intf[i].speed,
			ss->intf.intf[i].duplex);
	}

	printf("]");
}

static void
json_print_IFB(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;

        printf(", %s: [", hp);

	for (i = 0; i < ss->ifb.nrports; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"ibname\": \"%.19s\", "
			"\"portnr\": \"%hd\", "
			"\"lanes\": \"%hd\", "
			"\"maxrate\": %lld, "
			"\"rcvb\": %lld, "
			"\"sndb\": %lld, "
			"\"rcvp\": %lld, "
			"\"sndp\": %lld}",
			ss->ifb.ifb[i].ibname,
			ss->ifb.ifb[i].portnr,
			ss->ifb.ifb[i].lanes,
			ss->ifb.ifb[i].rate,
			ss->ifb.ifb[i].rcvb,
			ss->ifb.ifb[i].sndb,
			ss->ifb.ifb[i].rcvp,
			ss->ifb.ifb[i].sndp);
	}

	printf("]");
}

static void
json_print_NUM(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;

	printf(", %s: [", hp);

	for (i = 0; i < ss->memnuma.nrnuma; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"numanr\": \"%d\", "
			"\"frag\": \"%f\", "
			"\"totmem\": %lld, "
			"\"freemem\": %lld, "
			"\"active\": %lld, "
			"\"inactive\": %lld, "
			"\"filepage\": %lld, "
			"\"dirtymem\": %lld, "
			"\"slabmem\": %lld, "
			"\"slabreclaim\": %lld, "
			"\"shmem\": %lld, "
			"\"tothp\": %lld, "
			"\"freehp\": %lld}",
			ss->memnuma.numa[i].numanr,
			ss->memnuma.numa[i].frag * 100.0,
			ss->memnuma.numa[i].totmem * pagesize,
			ss->memnuma.numa[i].freemem * pagesize,
			ss->memnuma.numa[i].active * pagesize,
			ss->memnuma.numa[i].inactive * pagesize,
			ss->memnuma.numa[i].filepage * pagesize,
			ss->memnuma.numa[i].dirtymem * pagesize,
			ss->memnuma.numa[i].slabmem * pagesize,
			ss->memnuma.numa[i].slabreclaim * pagesize,
			ss->memnuma.numa[i].shmem * pagesize,
			ss->memnuma.numa[i].tothp * ss->mem.shugepagesz,
			ss->memnuma.numa[i].freehp * ss->mem.shugepagesz);
	}

	printf("]");
}

static void
json_print_NUC(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;

	printf(", %s: [", hp);

	for (i = 0; i < ss->cpunuma.nrnuma; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"numanr\": \"%d\", "
			"\"stime\": %lld, "
			"\"utime\": %lld, "
			"\"ntime\": %lld, "
			"\"itime\": %lld, "
			"\"wtime\": %lld, "
			"\"Itime\": %lld, "
			"\"Stime\": %lld, "
			"\"steal\": %lld, "
			"\"guest\": %lld}",
			ss->cpunuma.numa[i].numanr,
			ss->cpunuma.numa[i].stime,
			ss->cpunuma.numa[i].utime,
			ss->cpunuma.numa[i].ntime,
			ss->cpunuma.numa[i].itime,
			ss->cpunuma.numa[i].wtime,
			ss->cpunuma.numa[i].Itime,
			ss->cpunuma.numa[i].Stime,
			ss->cpunuma.numa[i].steal,
			ss->cpunuma.numa[i].guest);
	}

	printf("]");
}

static void
json_print_LLC(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;

        printf(", %s: [", hp);

	for (i = 0; i < ss->llc.nrllcs; i++) {
		if (i > 0) {
			printf(", ");
		}
		printf("{\"LLC\": \"%3d\", "
			"\"occupancy\": \"%3.1f%%\", "
			"\"mbm_total\": \"%lld\", "
			"\"mbm_local\": %lld}",
			ss->llc.perllc[i].id,
			ss->llc.perllc[i].occupancy * 100,
			ss->llc.perllc[i].mbm_total,
			ss->llc.perllc[i].mbm_local);
	}

	printf("]");
}

/*
** print functions for cgroups-level statistics
*/
static void
json_print_CGR(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	if ( !(supportflags & CGROUPV2) )
		return;

	register int	i, p;
	char		*cgrpath;

        printf(", %s: [", hp);

	for (i=0; i < ncgroups; i++) {
		if (i > 0) {
			printf(", ");
		}

                cgrpath = cggetpath(cs+i, cs, 1);

                // print cgroup level metrics
                //
                printf( "{\"path\": \"%s\", "
			 "\"nprocs\": %d, "
			 "\"procsbelow\": %d, "
			 "\"utime\": %lld, "
			 "\"stime\": %lld, "
			 "\"cpuweight\": %d, "
			 "\"cpumax\": %d, "
			 "\"cpupsisome\": %lld, "
			 "\"cpupsitotal\": %lld, "

			 "\"memcurrent\": %lld, "
			 "\"memanon\": %lld, "
			 "\"memfile\": %lld, "
			 "\"memkernel\": %lld, "
			 "\"memshmem\": %lld, "
			 "\"memmax\": %lld, "
			 "\"swpmax\": %lld, "
			 "\"mempsisome\": %lld, "
			 "\"mempsitotal\": %lld, "

			 "\"diskrbytes\": %lld, "
			 "\"diskwbytes\": %lld, "
			 "\"diskrios\": %lld, "
			 "\"diskwios\": %lld, "
			 "\"diskweight\": %d, "
			 "\"diskpsisome\": %lld, "
			 "\"diskpsitotal\": %lld, "

			 "\"pidlist\": [",

			cgrpath,
                        (cs+i)->cstat->gen.nprocs,
                        (cs+i)->cstat->gen.procsbelow,
                        (cs+i)->cstat->cpu.utime,
                        (cs+i)->cstat->cpu.stime,
                        (cs+i)->cstat->conf.cpuweight,
                        (cs+i)->cstat->conf.cpumax,
                        (cs+i)->cstat->cpu.somepres,
                        (cs+i)->cstat->cpu.fullpres,

                        (cs+i)->cstat->mem.current > 0 ?
			       (cs+i)->cstat->mem.current * pagesize :
			       (cs+i)->cstat->mem.current, 
                        (cs+i)->cstat->mem.anon > 0 ?
                        	(cs+i)->cstat->mem.anon * pagesize :
                        	(cs+i)->cstat->mem.anon,
                        (cs+i)->cstat->mem.file > 0 ?
                        	(cs+i)->cstat->mem.file * pagesize :
                        	(cs+i)->cstat->mem.file,
                        (cs+i)->cstat->mem.kernel > 0 ?
                        	(cs+i)->cstat->mem.kernel * pagesize :
                        	(cs+i)->cstat->mem.kernel,
                        (cs+i)->cstat->mem.shmem > 0 ?
                        	(cs+i)->cstat->mem.shmem * pagesize :
                        	(cs+i)->cstat->mem.shmem,
                        (cs+i)->cstat->conf.memmax > 0 ?
                        	(cs+i)->cstat->conf.memmax * pagesize :
                        	(cs+i)->cstat->conf.memmax,
                        (cs+i)->cstat->conf.swpmax > 0 ?
                        	(cs+i)->cstat->conf.swpmax * pagesize :
                        	(cs+i)->cstat->conf.swpmax,
                        (cs+i)->cstat->mem.somepres,
                        (cs+i)->cstat->mem.fullpres,

                        (cs+i)->cstat->dsk.rbytes,
                        (cs+i)->cstat->dsk.wbytes,
                        (cs+i)->cstat->dsk.rios,
                        (cs+i)->cstat->dsk.wios,
                        (cs+i)->cstat->conf.dskweight,
                        (cs+i)->cstat->dsk.somepres,
                        (cs+i)->cstat->dsk.fullpres);

		free(cgrpath);

                // generate related pidlist
                //
                if ((cs+i)->cstat->gen.nprocs)
                {
                        for (p=0; p < (cs+i)->cstat->gen.nprocs; p++) {
				if (p > 0)
					printf(", ");

                                printf("%d", (cs+i)->proclist[p]);
			}

                }

                printf("]}");
	}

	printf("]");
}

/*
** print functions for process-level statistics
*/
static void
json_print_PRG(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int	i, exitcode;
	static char	st[3];
	char		*cgrpath;

	printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		/* For one thread whose pid==tgid and isproc=n, it has the same
		   value with pid==tgid and isproc=y, thus filter it out. */
		if (ps->gen.tgid == ps->gen.pid && !ps->gen.isproc)
			continue;

		if (ps->gen.excode & ~(INT_MAX))
			st[0]='N';
		else
			st[0]='-';

		if (ps->gen.excode & 0xff)      // killed by signal?
		{
			exitcode = (ps->gen.excode & 0x7f) + 256;
			if (ps->gen.excode & 0x80)
				st[1] = 'C';
			else
				st[1] = 'S';
		}
		else
		{
			exitcode = (ps->gen.excode >>   8) & 0xff;
			st[1] = 'E';
		}

		if (i > 0) {
			printf(", ");
		}

		if (supportflags & CGROUPV2 && ps->gen.cgroupix != -1)
			cgrpath = cggetpath((cs + ps->gen.cgroupix), cs, 1);
		else
			cgrpath = "-";

		/*
		** using getpwuid() & getpwuid to convert ruid & euid to string
		** seems better, but the two functions take a long time
		*/
		printf("{\"pid\": %d, "
			"\"cmd\": \"%.19s\", "
			"\"state\": \"%c\", "
			"\"ruid\": %d, "
			"\"rgid\": %d, "
			"\"tgid\": %d, "
			"\"nthr\": %d, "
			"\"st\": \"%s\", "
			"\"exitcode\": %d, "
			"\"btime\": \"%ld\", "
			"\"cmdline\": \"(%.130s)\", "
			"\"ppid\": %d, "
			"\"nthrrun\": %d, "
			"\"nthrslpi\": %d, "
			"\"nthrslpu\": %d, "
			"\"nthridle\": %d, "
			"\"euid\": %d, "
			"\"egid\": %d, "
			"\"elaps\": \"%ld\", "
			"\"isproc\": %d, "
			"\"cid\": \"%.19s\", "
			"\"cgroup\": \"%s\"}",
			ps->gen.pid,
			ps->gen.name,
			ps->gen.state,
			ps->gen.ruid,
			ps->gen.rgid,
			ps->gen.tgid,
			ps->gen.nthr,
			st,
			exitcode,
			ps->gen.btime,
			ps->gen.cmdline,
			ps->gen.ppid,
			ps->gen.nthrrun,
			ps->gen.nthrslpi,
			ps->gen.nthrslpu,
			ps->gen.nthridle,
			ps->gen.euid,
			ps->gen.egid,
			ps->gen.elaps,
			!!ps->gen.isproc, /* convert to boolean */
			ps->gen.utsname[0] ? ps->gen.utsname:"-",
			cgrpath);

		if (supportflags & CGROUPV2 && ps->gen.cgroupix != -1)
			free(cgrpath);
	}

	printf("]");
}

static void
json_print_PRC(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int	i;
	char		*cgrpath;

        printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		if (ps->gen.tgid == ps->gen.pid && !ps->gen.isproc)
			continue;
		if (i > 0) {
			printf(", ");
		}

		if (supportflags & CGROUPV2 && ps->gen.cgroupix != -1)
			cgrpath = cggetpath((cs + ps->gen.cgroupix), cs, 1);
		else
			cgrpath = "-";

		printf("{\"pid\": %d, "
			"\"cmd\": \"%.19s\", "
			"\"utime\": %lld, "
			"\"stime\": %lld, "
			"\"nice\": %d, "
			"\"prio\": %d, "
			"\"curcpu\": %d, "
			"\"tgid\": %d, "
			"\"isproc\": %d, "
			"\"rundelay\": %lld, "
			"\"blkdelay\": %lld, "
			"\"nvcsw\": %llu, "
			"\"nivcsw\": %llu, "
			"\"sleepavg\": %d, "
			"\"cgroup\": \"%s\"}",
			ps->gen.pid,
			ps->gen.name,
			ps->cpu.utime,
			ps->cpu.stime,
			ps->cpu.nice,
			ps->cpu.prio,
			ps->cpu.curcpu,
			ps->gen.tgid,
			!!ps->gen.isproc,
			ps->cpu.rundelay/1000000,
			ps->cpu.blkdelay*1000/hertz,
			ps->cpu.nvcsw,
			ps->cpu.nivcsw,
			ps->cpu.sleepavg,
			cgrpath);

		if (supportflags & CGROUPV2 && ps->gen.cgroupix != -1)
			free(cgrpath);
	}

	printf("]");
}

static void
json_print_PRM(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int	i;
	char		*cgrpath;

        printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		if (ps->gen.tgid == ps->gen.pid && !ps->gen.isproc)
			continue;
		if (i > 0) {
			printf(", ");
		}

		if (supportflags & CGROUPV2 && ps->gen.cgroupix != -1)
			cgrpath = cggetpath((cs + ps->gen.cgroupix), cs, 1);
		else
			cgrpath = "-";

		printf("{\"pid\": %d, "
			"\"cmd\": \"%.19s\", "
			"\"vmem\": %lld, "
			"\"rmem\": %lld, "
			"\"vexec\": %lld, "
			"\"vgrow\": %lld, "
			"\"rgrow\": %lld, "
			"\"minflt\": %lld, "
			"\"majflt\": %lld, "
			"\"vlibs\": %lld, "
			"\"vdata\": %lld, "
			"\"vstack\": %lld, "
			"\"vlock\": %lld, "
			"\"vswap\": %lld, "
			"\"pmem\": %lld, "
			"\"cgroup\": \"%s\"}",
			ps->gen.pid,
			ps->gen.name,
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
			ps->mem.vlock,
			ps->mem.vswap,
			ps->mem.pmem == (unsigned long long)-1LL ?
			0:ps->mem.pmem,
			cgrpath);

		if (supportflags & CGROUPV2 && ps->gen.cgroupix != -1)
			free(cgrpath);
	}

	printf("]");
}

static void
json_print_PRD(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	register int i;

        printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		if (ps->gen.tgid == ps->gen.pid && !ps->gen.isproc)
			continue;
		if (i > 0) {
			printf(", ");
		}
		printf("{\"pid\": %d, "
			"\"cmd\": \"%.19s\", "
			"\"rio\": %lld, "
			"\"rsz\": %lld, "
			"\"wio\": %lld, "
			"\"wsz\": %lld, "
			"\"cwsz\": %lld}",
			ps->gen.pid,
			ps->gen.name,
			ps->dsk.rio, ps->dsk.rsz,
			ps->dsk.wio, ps->dsk.wsz, ps->dsk.cwsz);
	}

	printf("]");
}

static void
json_print_PRN(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	if (!(supportflags & NETATOP))
		return;

	register int i;

        printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		if (ps->gen.tgid == ps->gen.pid && !ps->gen.isproc)
			continue;
		if (i > 0) {
			printf(", ");
		}
		printf("{\"pid\": %d, "
			"\"cmd\": \"%.19s\", "
			"\"tcpsnd\": \"%lld\", "
			"\"tcpssz\": \"%lld\", "
			"\"tcprcv\": \"%lld\", "
			"\"tcprsz\": \"%lld\", "
			"\"udpsnd\": \"%lld\", "
			"\"udpssz\": \"%lld\", "
			"\"udprcv\": \"%lld\", "
			"\"udprsz\": \"%lld\"}",
			ps->gen.pid,
			ps->gen.name,
			ps->net.tcpsnd, ps->net.tcpssz,
			ps->net.tcprcv, ps->net.tcprsz,
			ps->net.udpsnd, ps->net.udpssz,
			ps->net.udprcv, ps->net.udprsz);
	}

	printf("]");
}

static void
json_print_PRE(char *hp, struct sstat *ss,
                         struct tstat *ps, int nact,
			 struct cgchainer *cs, int ncgroups)
{
	if ( !(supportflags & GPUSTAT) )
		return;

	register int i;

        printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		if (ps->gen.tgid == ps->gen.pid && !ps->gen.isproc)
			continue;
		if (i > 0) {
			printf(", ");
		}
		printf("{\"pid\": %d, "
			"\"cmd\": \"%.19s\", "
			"\"gpustate\": \"%c\", "
			"\"nrgpus\": %d, "
			"\"gpulist\": \"%x\", "
			"\"gpubusy\": %d, "
			"\"membusy\": %d, "
			"\"memnow\": %lld, "
			"\"memcum\": %lld, "
			"\"sample\": %lld}",
			ps->gen.pid,
			ps->gen.name,
			ps->gpu.state == '\0' ? 'N':ps->gpu.state,
			ps->gpu.nrgpus,
			ps->gpu.gpulist,
			ps->gpu.gpubusy,
			ps->gpu.membusy,
			ps->gpu.memnow,
			ps->gpu.memcum,
			ps->gpu.sample);
	}

	printf("]");
}
