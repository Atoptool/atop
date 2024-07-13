/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This program converts a raw logfile created by a particular version
** of atop to (by default) the current version of atop.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Initial:     July/August 2018
** --------------------------------------------------------------------------
** Copyright (C) 2018-2024 Gerlof Langeveld
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
*/
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <string.h>
#include <regex.h>
#include <zlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdarg.h>

#include "atop.h"
#include "photosyst.h"
#include "photoproc.h"
#include "cgroups.h"
#include "rawlog.h"

#include "prev/netstats_wrong.h"

#include "prev/photosyst_200.h"
#include "prev/photoproc_200.h"

#include "prev/photosyst_201.h"
#include "prev/photoproc_201.h"

#include "prev/photosyst_202.h"
#include "prev/photoproc_202.h"

#include "prev/photosyst_203.h"
#include "prev/photoproc_203.h"

#include "prev/photosyst_204.h"
#include "prev/photoproc_204.h"

#include "prev/photosyst_205.h"
#include "prev/photoproc_205.h"

#include "prev/photosyst_206.h"
#include "prev/photoproc_206.h"

#include "prev/photosyst_207.h"
#include "prev/photoproc_207.h"

#include "prev/photosyst_208.h"
#include "prev/photoproc_208.h"

#include "prev/photosyst_209.h"
#include "prev/photoproc_209.h"

#include "prev/photosyst_210.h"
#include "prev/photoproc_210.h"

#include "prev/photosyst_211.h"
#include "prev/photoproc_211.h"
#include "prev/cgroups_211.h"


void	justcopy(void *, void *, count_t, count_t);

void	scpu_to_21(void *, void *, count_t, count_t);
void	sdsk_to_21(void *, void *, count_t, count_t);

void	sint_to_22(void *, void *, count_t, count_t);

void	scpu_to_27(void *, void *, count_t, count_t);
void	smem_to_27(void *, void *, count_t, count_t);
void	sdsk_to_27(void *, void *, count_t, count_t);

void	smem_to_28(void *, void *, count_t, count_t);
void	snet_to_28(void *, void *, count_t, count_t);
void	sdsk_to_28(void *, void *, count_t, count_t);
void	smnu_to_28(void *, void *, count_t, count_t);
void	scnu_to_28(void *, void *, count_t, count_t);

void	sllc_to_210(void *, void *, count_t, count_t);

void	tgen_to_21(void *, void *, count_t, count_t);
void	tmem_to_21(void *, void *, count_t, count_t);

void	tgen_to_22(void *, void *, count_t, count_t);

void	tcpu_to_26(void *, void *, count_t, count_t);
void	tmem_to_26(void *, void *, count_t, count_t);

void	tcpu_to_28(void *, void *, count_t, count_t);
void	tmem_to_28(void *, void *, count_t, count_t);

void	tgen_to_210(void *, void *, count_t, count_t);

void	tgen_to_211(void *, void *, count_t, count_t);
void	tcpu_to_211(void *, void *, count_t, count_t);
void	tmem_to_211(void *, void *, count_t, count_t);

///////////////////////////////////////////////////////////////
// Conversion functions
// --------------------
// The structures with system level and process level info
// consists of sub-structures, generally for cpu, memory,
// disk and network values. These sub-structures might have
// changed from a specific version of atop to the next version.
// For modified sub-structures, conversion functions have to be
// written. These conversion functions will be called in a chained
// way for one version increment at the time. Suppose that a
// raw log is offered that has been created by atop 2.0 to be
// converted to atop 2.3, every sub-structure will be converted
// from 2.0 to 2.1, from 2.1 to 2.2 and finally from 2.2 to 2.3.
// When a sub-structure has NOT been changed from one version to
// another, it will just be copied by the generic conversion
// function 'justcopy' to the proper location in the structure
// of the next version.
// All conversion steps are controlled by the convs[] table.
///////////////////////////////////////////////////////////////

// Generic function that just copies an old structure byte-wise to
// a new structure (new structure implicitly padded with binary zeroes)
//
void
justcopy(void *old, void *new, count_t oldsize, count_t newsize)
{
	if (oldsize)
		memcpy(new, old, newsize > oldsize ? oldsize : newsize);
}

// /////////////////////////////////////////////////////////////////
// Specific functions that convert an old sstat sub-structure to
// a new sub-structure (system level)
// /////////////////////////////////////////////////////////////////
void
scpu_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// cfuture[1] --> cfuture[4] in struct percpu
	//
	struct cpustat_20	*c20 = old;
	struct cpustat_21	*c21 = new;
	int			i;

	memcpy(c21, c20, (char *)&c20->all - (char *)c20);	// base values
	memcpy(&c21->all, &c20->all, sizeof(struct percpu_20));

	for (i=0; i < MAXCPU_20; i++)
	    memcpy( &(c21->cpu[i]), &(c20->cpu[i]), sizeof(struct percpu_20));
}

void
sdsk_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// MAXDSK and MAXLVM have been enlarged
	//
	struct dskstat_20	*d20 = old;
	struct dskstat_21	*d21 = new;

	d21->ndsk 	= d20->ndsk;
	d21->nmdd 	= d20->nmdd;
	d21->nlvm 	= d20->nlvm;

	memcpy(d21->dsk, d20->dsk, sizeof d20->dsk);
	memcpy(d21->mdd, d20->mdd, sizeof d20->mdd);
	memcpy(d21->lvm, d20->lvm, sizeof d20->lvm);
}

void
sint_to_22(void *old, void *new, count_t oldsize, count_t newsize)
{
	// members 'type' and 'speedp' added to struct perintf
	//
	struct intfstat_21	*i21 = old;
	struct intfstat_22	*i22 = new;
	int			i;

	i22->nrintf 	= i21->nrintf;

	for (i=0; i < MAXINTF_21; i++)
	{
		memcpy(&(i22->intf[i]), &(i21->intf[i]),
					sizeof(struct perintf_21));

		// correct last members by refilling
		//
		i22->intf[i].type 	= '?';
		i22->intf[i].speed	= i21->intf[i].speed;
		i22->intf[i].speedp	= i21->intf[i].speed;
		i22->intf[i].duplex	= i21->intf[i].duplex;

		memset(i22->intf[i].cfuture, 0, sizeof i22->intf[i].cfuture);
	}
}

void
scpu_to_27(void *old, void *new, count_t oldsize, count_t newsize)
{
	// cfuture[2] --> cfuture[4] in struct percpu
	//
	struct cpustat_26	*c26 = old;
	struct cpustat_27	*c27 = new;
	int			i;

	memcpy(c27, c26, (char *)&c26->all - (char *)c26);	// base values
	memcpy(&c27->all, &c26->all, sizeof(struct percpu_26));

	for (i=0; i < MAXCPU_26; i++)
	    memcpy( &(c27->cpu[i]), &(c26->cpu[i]), sizeof(struct percpu_26));
}

void
smem_to_27(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct memstat_26	*m26 = old;
	struct memstat_27	*m27 = new;

	memcpy(m27, m26, sizeof *m26);

	m27->oomkills = -1;	// explicitly define 'unused'
}

void
sdsk_to_27(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct dskstat_26	*d26 = old;
	struct dskstat_27	*d27 = new;
	int			i;

	memcpy(d27, d26, oldsize);

	for (i=0; i < d27->ndsk; i++)
	    d27->dsk[i].ndisc = -1;	// explicitly define 'unused'

	for (i=0; i < d27->nmdd; i++)
	    d27->mdd[i].ndisc = -1;	// explicitly define 'unused'

	for (i=0; i < d27->nlvm; i++)
	    d27->lvm[i].ndisc = -1;	// explicitly define 'unused'
}

void
smem_to_28(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct memstat_27	*m27 = old;
	struct memstat_28	*m28 = new;

	memcpy(m28, m27, sizeof *m27);

	m28->tcpsock		= 0;	// new counter
	m28->udpsock		= 0;	// new counter

	m28->commitlim		= m27->commitlim;
	m28->committed		= m27->committed;
	m28->shmem		= m27->shmem;
	m28->shmrss		= m27->shmrss;
	m28->shmswp		= m27->shmswp;
	m28->slabreclaim	= m27->slabreclaim;
	m28->tothugepage	= m27->tothugepage;
	m28->freehugepage	= m27->freehugepage;
	m28->hugepagesz		= m27->hugepagesz;
	m28->vmwballoon		= m27->vmwballoon;
	m28->zfsarcsize		= m27->zfsarcsize;
	m28->swapcached		= m27->swapcached;
	m28->ksmsharing		= m27->ksmsharing;
	m28->ksmshared		= m27->ksmshared;
	m28->zswstored		= m27->zswstored;
	m28->zswtotpool		= m27->zswtotpool;
	m28->oomkills		= m27->oomkills;
	m28->compactstall	= m27->compactstall;
	m28->pgmigrate		= m27->pgmigrate;
	m28->numamigrate	= m27->numamigrate;

        m28->pgouts		= 0;	// new counter
        m28->pgins		= 0;	// new counter
        m28->pagetables		= 0;	// new counter

        memset(m28->cfuture, 0, sizeof m28->cfuture);
}

void
snet_to_28(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct netstat_wrong	*n27 = old;
	struct netstat		*n28 = new;

	// copy unmodified structs
	//
	memcpy(&(n28->ipv4),   &(n27->ipv4),   sizeof n28->ipv4);
	memcpy(&(n28->ipv6),   &(n27->ipv6),   sizeof n28->ipv6);

	memcpy(&(n28->udpv4),  &(n27->udpv4),  sizeof n28->udpv4);
	memcpy(&(n28->udpv6),  &(n27->udpv6),  sizeof n28->udpv6);

	memcpy(&(n28->icmpv6), &(n27->icmpv6), sizeof n28->icmpv6);

	// convert tcp_stats (last counter added)
	//
	memcpy(&(n28->tcp),    &(n27->tcp),    sizeof n27->tcp);
	n28->tcp.InCsumErrors = 0;

	// convert icmpv4_stats (counter added in the middle)
	//
	n28->icmpv4.InMsgs           = n27->icmpv4.InMsgs;
        n28->icmpv4.InErrors         = n27->icmpv4.InErrors;
        n28->icmpv4.InCsumErrors     = 0;    // new counter
        n28->icmpv4.InDestUnreachs   = n27->icmpv4.InDestUnreachs;
        n28->icmpv4.InTimeExcds      = n27->icmpv4.InTimeExcds;
        n28->icmpv4.InParmProbs      = n27->icmpv4.InParmProbs;
        n28->icmpv4.InSrcQuenchs     = n27->icmpv4.InSrcQuenchs;
        n28->icmpv4.InRedirects      = n27->icmpv4.InRedirects;
        n28->icmpv4.InEchos          = n27->icmpv4.InEchos;
        n28->icmpv4.InEchoReps       = n27->icmpv4.InEchoReps;
        n28->icmpv4.InTimestamps     = n27->icmpv4.InTimestamps;
        n28->icmpv4.InTimestampReps  = n27->icmpv4.InTimestampReps;
        n28->icmpv4.InAddrMasks      = n27->icmpv4.InAddrMasks;
        n28->icmpv4.InAddrMaskReps   = n27->icmpv4.InAddrMaskReps;
        n28->icmpv4.OutMsgs          = n27->icmpv4.OutMsgs;
        n28->icmpv4.OutErrors        = n27->icmpv4.OutErrors;
        n28->icmpv4.OutDestUnreachs  = n27->icmpv4.OutDestUnreachs;
        n28->icmpv4.OutTimeExcds     = n27->icmpv4.OutTimeExcds;
        n28->icmpv4.OutParmProbs     = n27->icmpv4.OutParmProbs;
        n28->icmpv4.OutSrcQuenchs    = n27->icmpv4.OutSrcQuenchs;
        n28->icmpv4.OutRedirects     = n27->icmpv4.OutRedirects;
        n28->icmpv4.OutEchos         = n27->icmpv4.OutEchos;
        n28->icmpv4.OutEchoReps      = n27->icmpv4.OutEchoReps;
        n28->icmpv4.OutTimestamps    = n27->icmpv4.OutTimestamps;
        n28->icmpv4.OutTimestampReps = n27->icmpv4.OutTimestampReps;
        n28->icmpv4.OutAddrMasks     = n27->icmpv4.OutAddrMasks;
        n28->icmpv4.OutAddrMaskReps  = n27->icmpv4.OutAddrMaskReps;
}

void
sdsk_to_28(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct dskstat_27	*d27 = old;
	struct dskstat_28	*d28 = new;
	int			i;

	d28->ndsk	= d27->ndsk;
	d28->nmdd	= d27->nmdd;
	d28->nlvm	= d27->nlvm;

	for (i=0; i < d28->ndsk; i++)
	    memcpy(&(d28->dsk[i]), &(d27->dsk[i]), sizeof d27->dsk[i]);

	for (i=0; i < d28->nmdd; i++)
	    memcpy(&(d28->mdd[i]), &(d27->mdd[i]), sizeof d27->mdd[i]);

	for (i=0; i < d28->nlvm; i++)
	    memcpy(&(d28->lvm[i]), &(d27->lvm[i]), sizeof d27->lvm[i]);
}

void
smnu_to_28(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct memnuma_27	*n27 = old;
	struct memnuma_28	*n28 = new;
	int			i;

	n28->nrnuma = n27->nrnuma;

	for (i=0; i < n28->nrnuma; i++)
	{
		n28->numa[i].numanr 		= i;
        	n28->numa[i].frag		= n27->numa[i].frag;
        	n28->numa[i].totmem		= n27->numa[i].totmem;
        	n28->numa[i].freemem		= n27->numa[i].freemem;
        	n28->numa[i].filepage		= n27->numa[i].filepage;
        	n28->numa[i].dirtymem		= n27->numa[i].dirtymem;
        	n28->numa[i].filepage		= n27->numa[i].filepage;
        	n28->numa[i].slabmem		= n27->numa[i].slabmem;
        	n28->numa[i].slabreclaim	= n27->numa[i].slabreclaim;
        	n28->numa[i].active		= n27->numa[i].active;
        	n28->numa[i].inactive		= n27->numa[i].inactive;
        	n28->numa[i].shmem		= n27->numa[i].shmem;
        	n28->numa[i].tothp		= n27->numa[i].tothp;
	}
}

void
scnu_to_28(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct cpunuma_27	*n27 = old;
	struct cpunuma_28	*n28 = new;
	int			i;

	n28->nrnuma = n27->nrnuma;

	for (i=0; i < n28->nrnuma; i++)
	{
		n28->numa[i].numanr 	= i;
        	n28->numa[i].nrcpu	= n27->numa[i].nrcpu;
        	n28->numa[i].stime	= n27->numa[i].stime;
        	n28->numa[i].utime	= n27->numa[i].utime;
        	n28->numa[i].ntime	= n27->numa[i].ntime;
        	n28->numa[i].itime	= n27->numa[i].itime;
        	n28->numa[i].wtime	= n27->numa[i].wtime;
        	n28->numa[i].Itime	= n27->numa[i].Itime;
        	n28->numa[i].Stime	= n27->numa[i].Stime;
        	n28->numa[i].steal	= n27->numa[i].steal;
        	n28->numa[i].guest	= n27->numa[i].guest;
	}
}

void
sllc_to_210(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct llcstat_29	*l29  = old;
	struct llcstat_210	*l210 = new;
	int			i;

	l210->nrllcs = l29->nrllcs;

	for (i=0; i < l210->nrllcs; i++)
	{
		l210->perllc[i].id 		= l29->perllc[i].id;
		l210->perllc[i].occupancy 	= l29->perllc[i].occupancy;
		l210->perllc[i].mbm_local 	= l29->perllc[i].mbm_local;
		l210->perllc[i].mbm_total 	= l29->perllc[i].mbm_total;
	}
}

// /////////////////////////////////////////////////////////////////
// Specific functions that convert an old tstat sub-structure to
// a new sub-structure (process level)
// /////////////////////////////////////////////////////////////////
void
tgen_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// member 'envid' inserted in struct gen
	//
	struct gen_20	*g20 = old;
	struct gen_21	*g21 = new;

	memcpy(g21, g20, (char *)g20->ifuture - (char *)g20);	// base values
	g21->envid = 0;
}

void
tmem_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// members 'pmem' and 'cfuture[4]' inserted in struct mem
	//
	struct mem_20	*m20 = old;
	struct mem_21	*m21 = new;

	m21->minflt	= m20->minflt;
	m21->majflt	= m20->majflt;
	m21->vexec	= m20->vexec;
	m21->vmem	= m20->vmem;
	m21->rmem	= m20->rmem;
	m21->pmem	= 0;
	m21->vgrow	= m20->vgrow;
	m21->rgrow	= m20->rgrow;
	m21->vdata	= m20->vdata;
	m21->vstack	= m20->vstack;
	m21->vlibs	= m20->vlibs;
	m21->vswap	= m20->vswap;
}

void
tgen_to_22(void *old, void *new, count_t oldsize, count_t newsize)
{
	// member 'envid' removed, members 'ctid' and 'vpid' inserted in struct gen
	//
	struct gen_21	*g21 = old;
	struct gen_22	*g22 = new;

	memcpy(g22, g21, (char *)&g21->envid - (char *)g21);	// copy base values
	g22->ctid = g21->envid;
	g22->vpid = 0;
}

void
tcpu_to_26(void *old, void *new, count_t oldsize, count_t newsize)
{
	// unused values appear not to be zeroed in version 2.5
	//
	struct cpu_25	*c25 = old;
	struct cpu_26	*c26 = new;

	memcpy(c26, c25, sizeof *c26);		// copy entire struct

        memset(&(c26->wchan), 0, sizeof c26->wchan);
	c26->rundelay = 0;
}

void
tmem_to_26(void *old, void *new, count_t oldsize, count_t newsize)
{
	// unused values appear not to be zeroed in version 2.5
	//
	struct mem_25	*m25 = old;
	struct mem_26	*m26 = new;

	memcpy(m26, m25, sizeof *m26);		// copy entire struct

	m26->vlock = 0;
}

void
tcpu_to_28(void *old, void *new, count_t oldsize, count_t newsize)
{
	// cgroup counters inserted
	//
	struct cpu_27	*c27 = old;
	struct cpu_28	*c28 = new;

        c28->utime	= c27->utime;
        c28->stime	= c27->stime;
        c28->nice	= c27->nice;
        c28->prio	= c27->prio;
        c28->rtprio	= c27->rtprio;
        c28->policy	= c27->policy;
        c28->curcpu	= c27->curcpu;
        c28->sleepavg	= c27->sleepavg;
        c28->rundelay	= c27->rundelay;

	memcpy(c28->wchan, c27->wchan, sizeof c28->wchan);

        c28->blkdelay	= 0;
        c28->cgcpuweight= 0;
        c28->cgcpumax	= 0;
        c28->cgcpumaxr	= 0;

	memset(c28->ifuture, 0, sizeof c28->ifuture);
	memset(c28->cfuture, 0, sizeof c28->cfuture);
}

void
tmem_to_28(void *old, void *new, count_t oldsize, count_t newsize)
{
	struct mem_27	*m27 = old;
	struct mem_28	*m28 = new;

	memcpy(m28, m27, sizeof *m27);		// copy old struct

	m28->cgmemmax	= 0;
	m28->cgmemmaxr	= 0;
	m28->cgswpmax 	= 0;
	m28->cgswpmaxr	= 0;

	memset(m28->cfuture, 0, sizeof m28->cfuture);
}

void
tgen_to_210(void *old, void *new, count_t oldsize, count_t newsize)
{
	// member 'nthridle' inserted, everything from member 'ctid' must shift
	//
	struct gen_29	*g29 = old;
	struct gen_210	*g210 = new;

	memcpy(g210, g29, (char *)&g29->ctid - (char *)g29);	// copy base values

	g210->nthridle 		= 0;
	g210->ctid 		= g29->ctid;
	g210->vpid 		= g29->vpid;
	g210->wasinactive 	= g29->wasinactive;

	memcpy(g210->utsname, g29->container, sizeof(g210->utsname));
	memcpy(g210->cgpath,  g29->cgpath,    sizeof(g210->cgpath));
}

void
tgen_to_211(void *old, void *new, count_t oldsize, count_t newsize)
{
	// member 'cgpath' removed, member cgroupix and ifuture instead
	//
	struct gen_210	*g210 = old;
	struct gen_211	*g211 = new;

	memcpy(g211, g210, sizeof *g211);	// copy base values
	memset(g211->ifuture, 0, sizeof g211->ifuture);
	g211->cgroupix = -1;
}

void
tcpu_to_211(void *old, void *new, count_t oldsize, count_t newsize)
{
	// set future values (released cgroup values) to zero
	//
	struct cpu_210	*c210 = old;
	struct cpu_211	*c211 = new;

	memcpy(c211, c210, sizeof *c211);	// copy base values
	memset(c211->ifuture, 0, sizeof c211->ifuture);
}

void
tmem_to_211(void *old, void *new, count_t oldsize, count_t newsize)
{
	// set future values (released cgroup values) to zero
	//
	struct mem_210	*m210 = old;
	struct mem_211	*m211 = new;

	memcpy(m211, m210, sizeof *m211);	// copy base values
	memset(m211->cfuture, 0, sizeof m211->cfuture);
}

///////////////////////////////////////////////////////////////
// conversion definition for various structs in sstat and tstat
//
#define	SETVERSION(major, minor)	((major << 8) | minor)
#define	STROFFSET(str, begin)		((long)((char *)str - (char *)begin))

struct sconvstruct {
	count_t	 structsize;
	void	*structptr;
	void	(*structconv)(void *, void *, count_t, count_t);
};

struct tconvstruct {
	count_t	 structsize;
	long	structoffset;
	void	(*structconv)(void *, void *, count_t, count_t);
};

struct sstat_20		sstat_20;
struct sstat_21		sstat_21;
struct sstat_22		sstat_22;
struct sstat_23		sstat_23;
struct sstat_24		sstat_24;
struct sstat_25		sstat_25;
struct sstat_26		sstat_26;
struct sstat_27		sstat_27;
struct sstat_28		sstat_28;
struct sstat_29		sstat_29;
struct sstat_210	sstat_210;
struct sstat_211	sstat_211;
struct sstat		sstat;

struct cstat		cstat;

struct tstat_20		tstat_20;
struct tstat_21		tstat_21;
struct tstat_22		tstat_22;
struct tstat_23		tstat_23;
struct tstat_24		tstat_24;
struct tstat_25		tstat_25;
struct tstat_26		tstat_26;
struct tstat_27		tstat_27;
struct tstat_28		tstat_28;
struct tstat_29		tstat_29;
struct tstat_210	tstat_210;
struct tstat_211	tstat_211;
struct tstat		tstat;

struct convertall {
	int			version;	// version of raw log

	unsigned int		sstatlen;	// length of struct sstat
	void			*sstat;		// pointer to sstat struct

	unsigned int		cstatlen;	// length of struct cstat (>= 2.11)
	void			*cstat;		// pointer to cstat struct

	unsigned int		tstatlen;	// length of struct tstat
	void			*tstat;		// pointer to tstat structs

	// conversion definition for subparts within sstat
	struct sconvstruct 	scpu;
	struct sconvstruct 	smem;
	struct sconvstruct 	snet;
	struct sconvstruct 	sintf;
	struct sconvstruct 	sdsk;	
	struct sconvstruct 	snfs;
	struct sconvstruct 	scfs;
	struct sconvstruct 	swww;
	struct sconvstruct 	spsi;
	struct sconvstruct 	sgpu;
	struct sconvstruct 	sifb;
        struct sconvstruct	smnum;
        struct sconvstruct	scnum;
        struct sconvstruct	sllc;

	// conversion definition for subparts within cstat
	// relevant from version 2.12 onwards
	
	// conversion definition for subparts within tstat
	struct tconvstruct 	tgen;
	struct tconvstruct 	tcpu;
	struct tconvstruct 	tdsk;
	struct tconvstruct 	tmem;
	struct tconvstruct 	tnet;
	struct tconvstruct 	tgpu;
} convs[] =
{
	{SETVERSION(2,0),
		 sizeof(struct sstat_20), 	&sstat_20,
		 0,				NULL,
		 sizeof(struct tstat_20), 	NULL,

		{sizeof(struct cpustat_20),  	&sstat_20.cpu,	NULL},
		{sizeof(struct memstat_20),  	&sstat_20.mem,	NULL},
		{sizeof(struct netstat_20),  	&sstat_20.net,	NULL},
		{sizeof(struct intfstat_20), 	&sstat_20.intf,	NULL},
		{sizeof(struct dskstat_20),  	&sstat_20.dsk,	NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{sizeof(struct wwwstat_20),  	&sstat_20.www,	NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_20),
			STROFFSET(&tstat_20.gen, &tstat_20),	NULL},
		{sizeof(struct cpu_20),
			STROFFSET(&tstat_20.cpu, &tstat_20),	NULL},
		{sizeof(struct dsk_20),
			STROFFSET(&tstat_20.dsk, &tstat_20),	NULL},
		{sizeof(struct mem_20),
			STROFFSET(&tstat_20.mem, &tstat_20),	NULL},
		{sizeof(struct net_20),
			STROFFSET(&tstat_20.net, &tstat_20),	NULL},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,1), // 2.0 --> 2.1
		 sizeof(struct sstat_21), 	&sstat_21,
		 0,				NULL,
		 sizeof(struct tstat_21), 	NULL,

		{sizeof(struct cpustat_21),  	&sstat_21.cpu,	scpu_to_21},
		{sizeof(struct memstat_21),  	&sstat_21.mem,	justcopy},
		{sizeof(struct netstat_21),  	&sstat_21.net,	justcopy},
		{sizeof(struct intfstat_21), 	&sstat_21.intf,	justcopy},
		{sizeof(struct dskstat_21),  	&sstat_21.dsk,	sdsk_to_21},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{sizeof(struct wwwstat_21),  	&sstat_21.www,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_21),
			STROFFSET(&tstat_21.gen, &tstat_21),	tgen_to_21},
		{sizeof(struct cpu_21),
			STROFFSET(&tstat_21.cpu, &tstat_21),	justcopy},
		{sizeof(struct dsk_21),
			STROFFSET(&tstat_21.dsk, &tstat_21),	justcopy},
		{sizeof(struct mem_21),
			STROFFSET(&tstat_21.mem, &tstat_21),	tmem_to_21},
		{sizeof(struct net_21),
			STROFFSET(&tstat_21.net, &tstat_21),	justcopy},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,2), // 2.1 --> 2.2
		 sizeof(struct sstat_22), 	&sstat_22,
		 0,				NULL,
		 sizeof(struct tstat_22), 	NULL,

		{sizeof(struct cpustat_22),  	&sstat_22.cpu,	justcopy},
		{sizeof(struct memstat_22),  	&sstat_22.mem,	justcopy},
		{sizeof(struct netstat_22),  	&sstat_22.net,	justcopy},
		{sizeof(struct intfstat_22), 	&sstat_22.intf,	sint_to_22},
		{sizeof(struct dskstat_22),  	&sstat_22.dsk,	justcopy},
		{sizeof(struct nfsstat_22),  	&sstat_22.nfs,	NULL},
		{sizeof(struct contstat_22), 	&sstat_22.cfs,	NULL},
		{sizeof(struct wwwstat_22),  	&sstat_22.www,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_22),
			STROFFSET(&tstat_22.gen, &tstat_22),	tgen_to_22},
		{sizeof(struct cpu_22),
			STROFFSET(&tstat_22.cpu, &tstat_22),	justcopy},
		{sizeof(struct dsk_22),
			STROFFSET(&tstat_22.dsk, &tstat_22),	justcopy},
		{sizeof(struct mem_22),
			STROFFSET(&tstat_22.mem, &tstat_22),	justcopy},
		{sizeof(struct net_22),
			STROFFSET(&tstat_22.net, &tstat_22),	justcopy},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,3), // 2.2 --> 2.3
		 sizeof(struct sstat_23), 	&sstat_23,
		 0,				NULL,
		 sizeof(struct tstat_23), 	NULL,

		{sizeof(struct cpustat_23),  	&sstat_23.cpu,	justcopy},
		{sizeof(struct memstat_23),  	&sstat_23.mem,	justcopy},
		{sizeof(struct netstat_23),  	&sstat_23.net,	justcopy},
		{sizeof(struct intfstat_23), 	&sstat_23.intf,	justcopy},
		{sizeof(struct dskstat_23),  	&sstat_23.dsk,	justcopy},
		{sizeof(struct nfsstat_23),  	&sstat_23.nfs,	justcopy},
		{sizeof(struct contstat_23), 	&sstat_23.cfs,	justcopy},
		{sizeof(struct wwwstat_23),  	&sstat_23.www,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_23),
			STROFFSET(&tstat_23.gen, &tstat_23),	justcopy},
		{sizeof(struct cpu_23),
			STROFFSET(&tstat_23.cpu, &tstat_23),	justcopy},
		{sizeof(struct dsk_23),
			STROFFSET(&tstat_23.dsk, &tstat_23),	justcopy},
		{sizeof(struct mem_23),
			STROFFSET(&tstat_23.mem, &tstat_23),	justcopy},
		{sizeof(struct net_23),
			STROFFSET(&tstat_23.net, &tstat_23),	justcopy},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,4), // 2.3 --> 2.4
		 sizeof(struct sstat_24),	&sstat_24,
		 0,				NULL,
		 sizeof(struct tstat_24), 	NULL,

		{sizeof(struct cpustat_24),  	&sstat_24.cpu,	justcopy},
		{sizeof(struct memstat_24),  	&sstat_24.mem,	justcopy},
		{sizeof(struct netstat_24),  	&sstat_24.net,	justcopy},
		{sizeof(struct intfstat_24), 	&sstat_24.intf,	justcopy},
		{sizeof(struct dskstat_24),  	&sstat_24.dsk,	justcopy},
		{sizeof(struct nfsstat_24),  	&sstat_24.nfs,	justcopy},
		{sizeof(struct contstat_24), 	&sstat_24.cfs,	justcopy},
		{sizeof(struct wwwstat_24),  	&sstat_24.www,	justcopy},
		{0,  				&sstat_24.psi,	NULL},
		{0,  				&sstat_24.gpu,	NULL},
		{0,  				&sstat_24.ifb,	NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_24),
			STROFFSET(&tstat_24.gen, &tstat_24),	justcopy},
		{sizeof(struct cpu_24),
			STROFFSET(&tstat_24.cpu, &tstat_24),	justcopy},
		{sizeof(struct dsk_24),
			STROFFSET(&tstat_24.dsk, &tstat_24),	justcopy},
		{sizeof(struct mem_24),
			STROFFSET(&tstat_24.mem, &tstat_24),	justcopy},
		{sizeof(struct net_24),
			STROFFSET(&tstat_24.net, &tstat_24),	justcopy},
		{sizeof(struct gpu_24),
			STROFFSET(&tstat_24.gpu, &tstat_24),	justcopy},
	},

	{SETVERSION(2,5), // 2.4 --> 2.5
		 sizeof(struct sstat_25),	&sstat_25,
		 0,				NULL,
		 sizeof(struct tstat_25), 	NULL,

		{sizeof(struct cpustat_25),  	&sstat_25.cpu,	justcopy},
		{sizeof(struct memstat_25),  	&sstat_25.mem,	justcopy},
		{sizeof(struct netstat_25),  	&sstat_25.net,	justcopy},
		{sizeof(struct intfstat_25), 	&sstat_25.intf,	justcopy},
		{sizeof(struct dskstat_25),  	&sstat_25.dsk,	justcopy},
		{sizeof(struct nfsstat_25),  	&sstat_25.nfs,	justcopy},
		{sizeof(struct contstat_25), 	&sstat_25.cfs,	justcopy},
		{sizeof(struct wwwstat_25),  	&sstat_25.www,	justcopy},
		{sizeof(struct pressure_25),  	&sstat_25.psi,	justcopy},
		{sizeof(struct gpustat_25),  	&sstat_25.gpu,	justcopy},
		{sizeof(struct ifbstat_25),  	&sstat_25.ifb,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_25),
			STROFFSET(&tstat_25.gen, &tstat_25),	justcopy},
		{sizeof(struct cpu_25),
			STROFFSET(&tstat_25.cpu, &tstat_25),	justcopy},
		{sizeof(struct dsk_25),
			STROFFSET(&tstat_25.dsk, &tstat_25),	justcopy},
		{sizeof(struct mem_25),
			STROFFSET(&tstat_25.mem, &tstat_25),	justcopy},
		{sizeof(struct net_25),
			STROFFSET(&tstat_25.net, &tstat_25),	justcopy},
		{sizeof(struct gpu_25),
			STROFFSET(&tstat_25.gpu, &tstat_25),	justcopy},
	},

	{SETVERSION(2,6), // 2.5 --> 2.6
		 sizeof(struct sstat_26),	&sstat_26,
		 0,				NULL,
		 sizeof(struct tstat_26), 	NULL,

		{sizeof(struct cpustat_26),  	&sstat_26.cpu,	justcopy},
		{sizeof(struct memstat_26),  	&sstat_26.mem,	justcopy},
		{sizeof(struct netstat_26),  	&sstat_26.net,	justcopy},
		{sizeof(struct intfstat_26), 	&sstat_26.intf,	justcopy},
		{sizeof(struct dskstat_26),  	&sstat_26.dsk,	justcopy},
		{sizeof(struct nfsstat_26),  	&sstat_26.nfs,	justcopy},
		{sizeof(struct contstat_26), 	&sstat_26.cfs,	justcopy},
		{sizeof(struct wwwstat_26),  	&sstat_26.www,	justcopy},
		{sizeof(struct pressure_26),  	&sstat_26.psi,	justcopy},
		{sizeof(struct gpustat_26),  	&sstat_26.gpu,	justcopy},
		{sizeof(struct ifbstat_26),  	&sstat_26.ifb,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_26),
			STROFFSET(&tstat_26.gen, &tstat_26),	justcopy},
		{sizeof(struct cpu_26),
			STROFFSET(&tstat_26.cpu, &tstat_26),	tcpu_to_26},
		{sizeof(struct dsk_26),
			STROFFSET(&tstat_26.dsk, &tstat_26),	justcopy},
		{sizeof(struct mem_26),
			STROFFSET(&tstat_26.mem, &tstat_26),	tmem_to_26},
		{sizeof(struct net_26),
			STROFFSET(&tstat_26.net, &tstat_26),	justcopy},
		{sizeof(struct gpu_26),
			STROFFSET(&tstat_26.gpu, &tstat_26),	justcopy},
	},

	{SETVERSION(2,7), // 2.6 --> 2.7
		 sizeof(struct sstat_27),	&sstat_27,
		 0,				NULL,
		 sizeof(struct tstat_27), 	NULL,

		{sizeof(struct cpustat_27),  	&sstat_27.cpu,	scpu_to_27},
		{sizeof(struct memstat_27),  	&sstat_27.mem,	smem_to_27},
		{sizeof(struct netstat_27),  	&sstat_27.net,	justcopy},
		{sizeof(struct intfstat_27), 	&sstat_27.intf,	justcopy},
		{sizeof(struct dskstat_27),  	&sstat_27.dsk,	sdsk_to_27},
		{sizeof(struct nfsstat_27),  	&sstat_27.nfs,	justcopy},
		{sizeof(struct contstat_27), 	&sstat_27.cfs,	justcopy},
		{sizeof(struct wwwstat_27),  	&sstat_27.www,	justcopy},
		{sizeof(struct pressure_27),  	&sstat_27.psi,	justcopy},
		{sizeof(struct gpustat_27),  	&sstat_27.gpu,	justcopy},
		{sizeof(struct ifbstat_27),  	&sstat_27.ifb,	justcopy},
		{0,  				&sstat_27.memnuma,	NULL},
		{0,  				&sstat_27.cpunuma,	NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_27),
			STROFFSET(&tstat_27.gen, &tstat_27),	justcopy},
		{sizeof(struct cpu_27),
			STROFFSET(&tstat_27.cpu, &tstat_27),	justcopy},
		{sizeof(struct dsk_27),
			STROFFSET(&tstat_27.dsk, &tstat_27),	justcopy},
		{sizeof(struct mem_27),
			STROFFSET(&tstat_27.mem, &tstat_27),	justcopy},
		{sizeof(struct net_27),
			STROFFSET(&tstat_27.net, &tstat_27),	justcopy},
		{sizeof(struct gpu_27),
			STROFFSET(&tstat_27.gpu, &tstat_27),	justcopy},
	},

	{SETVERSION(2,8), // 2.7 --> 2.8
		 sizeof(struct sstat_28),	&sstat_28,
		 0,				NULL,
		 sizeof(struct tstat_28), 	NULL,

		{sizeof(struct cpustat_28),  	&sstat_28.cpu,	   justcopy},
		{sizeof(struct memstat_28),  	&sstat_28.mem,	   smem_to_28},
		{sizeof(struct netstat_28),  	&sstat_28.net,	   snet_to_28},
		{sizeof(struct intfstat_28), 	&sstat_28.intf,	   justcopy},
		{sizeof(struct dskstat_28),  	&sstat_28.dsk,	   sdsk_to_28},
		{sizeof(struct nfsstat_28),  	&sstat_28.nfs,	   justcopy},
		{sizeof(struct contstat_28), 	&sstat_28.cfs,	   justcopy},
		{sizeof(struct wwwstat_28),  	&sstat_28.www,	   justcopy},
		{sizeof(struct pressure_28),  	&sstat_28.psi,	   justcopy},
		{sizeof(struct gpustat_28),  	&sstat_28.gpu,	   justcopy},
		{sizeof(struct ifbstat_28),  	&sstat_28.ifb,	   justcopy},
		{sizeof(struct memnuma_28), 	&sstat_28.memnuma, smnu_to_28},
		{sizeof(struct cpunuma_28),  	&sstat_28.cpunuma, scnu_to_28},
		{0,  				&sstat_28.llc,	   NULL},

		{sizeof(struct gen_28),
			STROFFSET(&tstat_28.gen, &tstat_28),	justcopy},
		{sizeof(struct cpu_28),
			STROFFSET(&tstat_28.cpu, &tstat_28),	tcpu_to_28},
		{sizeof(struct dsk_28),
			STROFFSET(&tstat_28.dsk, &tstat_28),	justcopy},
		{sizeof(struct mem_28),
			STROFFSET(&tstat_28.mem, &tstat_28),	tmem_to_28},
		{sizeof(struct net_28),
			STROFFSET(&tstat_28.net, &tstat_28),	justcopy},
		{sizeof(struct gpu_28),
			STROFFSET(&tstat_28.gpu, &tstat_28),	justcopy},
	},

	{SETVERSION(2,9), // 2.8 --> 2.9
		 sizeof(struct sstat_29),	&sstat_29,
		 0,				NULL,
		 sizeof(struct tstat_29), 	NULL,

		{sizeof(struct cpustat_29),  	&sstat_29.cpu,	   justcopy},
		{sizeof(struct memstat_29),  	&sstat_29.mem,	   justcopy},
		{sizeof(struct netstat_29),  	&sstat_29.net,	   justcopy},
		{sizeof(struct intfstat_29), 	&sstat_29.intf,	   justcopy},
		{sizeof(struct dskstat_29),  	&sstat_29.dsk,	   justcopy},
		{sizeof(struct nfsstat_29),  	&sstat_29.nfs,	   justcopy},
		{sizeof(struct contstat_29), 	&sstat_29.cfs,	   justcopy},
		{sizeof(struct wwwstat_29),  	&sstat_29.www,	   justcopy},
		{sizeof(struct pressure_29),  	&sstat_29.psi,	   justcopy},
		{sizeof(struct gpustat_29),  	&sstat_29.gpu,	   justcopy},
		{sizeof(struct ifbstat_29),  	&sstat_29.ifb,	   justcopy},
		{sizeof(struct memnuma_29), 	&sstat_29.memnuma, justcopy},
		{sizeof(struct cpunuma_29),  	&sstat_29.cpunuma, justcopy},
		{sizeof(struct llcstat_29),  	&sstat_29.llc,	   justcopy},

		{sizeof(struct gen_29),
			STROFFSET(&tstat_29.gen, &tstat_29),	justcopy},
		{sizeof(struct cpu_29),
			STROFFSET(&tstat_29.cpu, &tstat_29),	justcopy},
		{sizeof(struct dsk_29),
			STROFFSET(&tstat_29.dsk, &tstat_29),	justcopy},
		{sizeof(struct mem_29),
			STROFFSET(&tstat_29.mem, &tstat_29),	justcopy},
		{sizeof(struct net_29),
			STROFFSET(&tstat_29.net, &tstat_29),	justcopy},
		{sizeof(struct gpu_29),
			STROFFSET(&tstat_29.gpu, &tstat_29),	justcopy},
	},

	{SETVERSION(2,10), // 2.9 --> 2.10
		 sizeof(struct sstat_210),	&sstat_210,
		 0,				NULL,
		 sizeof(struct tstat_210), 	NULL,

		{sizeof(struct cpustat_210),  	&sstat_210.cpu,	   justcopy},
		{sizeof(struct memstat_210),  	&sstat_210.mem,	   justcopy},
		{sizeof(struct netstat_210),  	&sstat_210.net,	   justcopy},
		{sizeof(struct intfstat_210), 	&sstat_210.intf,   justcopy},
		{sizeof(struct dskstat_210),  	&sstat_210.dsk,	   justcopy},
		{sizeof(struct nfsstat_210),  	&sstat_210.nfs,	   justcopy},
		{sizeof(struct contstat_210), 	&sstat_210.cfs,	   justcopy},
		{sizeof(struct wwwstat_210),  	&sstat_210.www,	   justcopy},
		{sizeof(struct pressure_210),  	&sstat_210.psi,	   justcopy},
		{sizeof(struct gpustat_210),  	&sstat_210.gpu,	   justcopy},
		{sizeof(struct ifbstat_210),  	&sstat_210.ifb,	   justcopy},
		{sizeof(struct memnuma_210), 	&sstat_210.memnuma, justcopy},
		{sizeof(struct cpunuma_210),  	&sstat_210.cpunuma, justcopy},
		{sizeof(struct llcstat_210),  	&sstat_210.llc,	   sllc_to_210},

		{sizeof(struct gen_210),
			STROFFSET(&tstat_210.gen, &tstat_210),	tgen_to_210},
		{sizeof(struct cpu_210),
			STROFFSET(&tstat_210.cpu, &tstat_210),	justcopy},
		{sizeof(struct dsk_210),
			STROFFSET(&tstat_210.dsk, &tstat_210),	justcopy},
		{sizeof(struct mem_210),
			STROFFSET(&tstat_210.mem, &tstat_210),	justcopy},
		{sizeof(struct net_210),
			STROFFSET(&tstat_210.net, &tstat_210),	justcopy},
		{sizeof(struct gpu_210),
			STROFFSET(&tstat_210.gpu, &tstat_210),	justcopy},
	},

	{SETVERSION(2,11), // 2.10 --> 2.11
		 sizeof(struct sstat_211),	&sstat_211,
		 sizeof(struct cstat_211),	NULL,
		 sizeof(struct tstat_211), 	NULL,

		{sizeof(struct cpustat_211),  	&sstat_211.cpu,	   justcopy},
		{sizeof(struct memstat_211),  	&sstat_211.mem,	   justcopy},
		{sizeof(struct netstat_211),  	&sstat_211.net,	   justcopy},
		{sizeof(struct intfstat_211), 	&sstat_211.intf,   justcopy},
		{sizeof(struct dskstat_211),  	&sstat_211.dsk,	   justcopy},
		{sizeof(struct nfsstat_211),  	&sstat_211.nfs,	   justcopy},
		{sizeof(struct contstat_211), 	&sstat_211.cfs,	   justcopy},
		{sizeof(struct wwwstat_211),  	&sstat_211.www,	   justcopy},
		{sizeof(struct pressure_211),  	&sstat_211.psi,	   justcopy},
		{sizeof(struct gpustat_211),  	&sstat_211.gpu,	   justcopy},
		{sizeof(struct ifbstat_211),  	&sstat_211.ifb,	   justcopy},
		{sizeof(struct memnuma_211), 	&sstat_211.memnuma, justcopy},
		{sizeof(struct cpunuma_211),  	&sstat_211.cpunuma, justcopy},
		{sizeof(struct llcstat_211),  	&sstat_211.llc,	   justcopy},

		{sizeof(struct gen_211),
			STROFFSET(&tstat_211.gen, &tstat_211),	tgen_to_211},
		{sizeof(struct cpu_211),
			STROFFSET(&tstat_211.cpu, &tstat_211),	tcpu_to_211},
		{sizeof(struct dsk_211),
			STROFFSET(&tstat_211.dsk, &tstat_211),	justcopy},
		{sizeof(struct mem_211),
			STROFFSET(&tstat_211.mem, &tstat_211),	tmem_to_211},
		{sizeof(struct net_211),
			STROFFSET(&tstat_211.net, &tstat_211),	justcopy},
		{sizeof(struct gpu_211),
			STROFFSET(&tstat_211.gpu, &tstat_211),	justcopy},
	},
};

int	numconvs = sizeof convs / sizeof(struct convertall);

///////////////////////////////////////////////////////////////
// End of conversion functions
///////////////////////////////////////////////////////////////

// function prototypes
//
static int	openin(char *);
static void	readin(int, void *, int);

static int	openout(char *);
static void	writeout(int, void *, int);
static void	writesamp(int, struct rawrecord *, void *, int, void *,
				int, int);

static void	copy_file(int, int);
static void	convert_samples(int, int, struct rawheader *, int,  int);
static void	do_sconvert(struct sconvstruct *, struct sconvstruct *);
static void	do_tconvert(void *, void *,
			struct tconvstruct *, struct tconvstruct *);

static int	getrawsstat(int, struct sstat *, int);
static int	getrawtstat(int, struct tstat *, unsigned long, int, int);
static void	testcompval(int, char *, char *);

int
main(int argc, char *argv[])
{
	int			ifd, ofd;
	struct rawheader	irh, orh;
	int			i, versionix, targetix = -1;
	int			c, major, minor, targetvers;
	char			*infile, *outfile;

	// verify the command line arguments:
	// 	optional flags and mandatory input and output filename
	//
	if (argc < 2)
		prusage(argv[0]);

	while ((c = getopt(argc, argv, "?t:")) != EOF)
	{
		switch (c)
		{
		   case '?':		// usage wanted ?
			prusage(argv[0]);
			break;

		   case 't':		// target version
			if ( sscanf(optarg, "%d.%d", &major, &minor) != 2)
			{
				fprintf(stderr,
					"target version format: major.minor\n");
				prusage(argv[0]);
			}

			targetvers = SETVERSION(major, minor);

			// search for target version in conversion table
			//
			for (i=0, targetix=-1; i < numconvs; i++)
			{
				if (targetvers == convs[i].version)
				{
					targetix = i;
					break;
				}
			}

			if (targetix == -1)	// incorrect target version?
			{
				fprintf(stderr,
					"target version incorrect!");
				prusage(argv[0]);
			}
			break;

		   default:
			prusage(argv[0]);
			break;
		}
	}

	if (optind >= argc)
		prusage(argv[0]);

	infile  = argv[optind++];

	// determine target version (default: latest version)
	//
	if (targetix == -1)	// no specific target version requested?
		targetix = numconvs - 1;

	// open the input file and verify magic number
	//
	if ( (ifd = openin(infile)) == -1)
	{
		prusage(argv[0]);
		exit(2);
	}

	readin(ifd, &irh, sizeof irh);

	if (irh.magic != MYMAGIC)
	{
		fprintf(stderr,
			"File %s does not contain atop/atopsar data "
			"(wrong magic number)\n", infile);
		exit(3);
	}

	printf("Version of %s: %d.%d\n", infile,
			(irh.aversion >> 8) & 0x7f, irh.aversion & 0xff);

	if (irh.rawheadlen != sizeof(struct rawheader) ||
	    irh.rawreclen  != sizeof(struct rawrecord)   )
	{
		fprintf(stderr,
			"File %s created with atop compiled "
			"for other CPU architecture\n", infile);
		exit(3);
	}

	// search for version of input file in conversion table
	//
	for (i=0, versionix=-1; i < numconvs; i++)
	{
		if (convs[i].version == (irh.aversion & 0x7fff))
		{
			versionix = i;
			break;
		}
	}

	if (versionix == -1)
	{
		fprintf(stderr,
			"This version is not supported for conversion!\n");
		exit(11);
	}

	if (versionix > targetix)
	{
		fprintf(stderr, "Downgrading of version is not supported!\n");
		exit(11);
	}

	if (irh.sstatlen != convs[versionix].sstatlen ||
	    irh.tstatlen != convs[versionix].tstatlen   )
	{
 		fprintf(stderr,
 			"File %s contains unexpected internal structures\n",
 			infile);
 		fprintf(stderr, "sstat: %d/%d, tstat: %d/%d\n",
 			irh.sstatlen, convs[versionix].sstatlen,
 	    		irh.tstatlen, convs[versionix].tstatlen);
 				
 		exit(11);
	}

	// handle the output file
	//
	if (optind >= argc)
		exit(0);

	outfile = argv[optind++];

	if (strcmp(infile, outfile) == 0)
	{
		fprintf(stderr,
	 	    "input file and output file should not be identical!\n");
		exit(12);
	}

	// open the output file
	//
	if ( (ofd = openout(outfile)) == -1)
	{
		prusage(argv[0]);
		exit(4);
	}

	// write raw header to output file
	//
	orh = irh;

	orh.aversion	= convs[targetix].version | 0x8000;
	orh.sstatlen	= convs[targetix].sstatlen;
	orh.cstatlen	= convs[targetix].cstatlen;
	orh.tstatlen	= convs[targetix].tstatlen;

	if (orh.pidwidth == 0)	// no pid width know in old raw log?
		orh.pidwidth = getpidwidth();

	writeout(ofd, &orh, sizeof orh);

	printf("Version of %s: %d.%d\n", outfile,
			(orh.aversion >> 8) & 0x7f, orh.aversion & 0xff);

	// copy and convert every sample, unless the version of the
	// input file is identical to the target version (then just copy)
	//
	if (versionix < targetix)
		convert_samples(ifd, ofd, &irh, versionix, targetix);
	else
		copy_file(ifd, ofd);

	close(ifd);
	close(ofd);

	return 0;
}

//
// Function that reads the input file sample-by-sample,
// converts it to an output sample and writes that to the output file
//
static void
convert_samples(int ifd, int ofd, struct rawheader *irh, int ivix, int ovix)
{
	struct rawrecord	irr, orr;
	int			i, t;
	count_t			count = 0;

	while ( read(ifd, &irr, irh->rawreclen) == irh->rawreclen)
	{
		count++;

		// read compressed system-level statistics and decompress
		//
		if ( !getrawsstat(ifd, convs[ivix].sstat, irr.scomplen) )
			exit(7);

		// read compressed process-level statistics and decompress
		//
		convs[ivix].tstat = malloc(convs[ivix].tstatlen * irr.ndeviat);

		ptrverify(convs[ivix].tstat,
			"Malloc failed for %d stored tasks\n", irr.ndeviat);

		if ( !getrawtstat(ifd, convs[ivix].tstat,
		                       convs[ivix].tstatlen * irr.ndeviat,
		                       irr.pcomplen, irr.ndeviat) )
			exit(7);

		// STEP-BY-STEP CONVERSION FROM OLD VERSION TO NEW VERSION
		//
		for (i=ivix; i < ovix; i++)
		{
			// convert system-level statistics to newer version
			//
			memset(convs[i+1].sstat, 0, convs[i+1].sstatlen);

			do_sconvert(&(convs[i].scpu),   &(convs[i+1].scpu));
			do_sconvert(&(convs[i].smem),   &(convs[i+1].smem));
			do_sconvert(&(convs[i].snet),   &(convs[i+1].snet));
			do_sconvert(&(convs[i].sintf),  &(convs[i+1].sintf));
			do_sconvert(&(convs[i].sdsk),   &(convs[i+1].sdsk));
			do_sconvert(&(convs[i].snfs),   &(convs[i+1].snfs));
			do_sconvert(&(convs[i].scfs),   &(convs[i+1].scfs));
			do_sconvert(&(convs[i].spsi),   &(convs[i+1].spsi));
			do_sconvert(&(convs[i].sgpu),   &(convs[i+1].sgpu));
			do_sconvert(&(convs[i].sifb),   &(convs[i+1].sifb));
			do_sconvert(&(convs[i].smnum),  &(convs[i+1].smnum));
			do_sconvert(&(convs[i].scnum),  &(convs[i+1].scnum));
			do_sconvert(&(convs[i].swww),   &(convs[i+1].swww));

			// convert process-level statistics to newer version
			//
			convs[i+1].tstat = malloc(convs[i+1].tstatlen *
								irr.ndeviat);
			ptrverify(convs[ivix].tstat,
			   "Malloc failed for %d stored tasks\n", irr.ndeviat);

			memset(convs[i+1].tstat, 0, convs[i+1].tstatlen *
								irr.ndeviat);

			for (t=0; t < irr.ndeviat; t++)	// for every task
			{
				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen),
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tgen), &(convs[i+1].tgen));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen),
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tcpu), &(convs[i+1].tcpu));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen),
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tdsk), &(convs[i+1].tdsk));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen),
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tmem), &(convs[i+1].tmem));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen),
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tnet), &(convs[i+1].tnet));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen),
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tgpu), &(convs[i+1].tgpu));
			}

			free(convs[i].tstat);

			// in version 2.11 incompatible cgroups v2 metrics
			// are implemented and earlier metrics will be lost
			//
			if (convs[i].version == SETVERSION(2,10))
				irr.flags &= ~RRCGRSTAT;
		}

		// write new sample to output file
		//
		orr = irr;

		writesamp(ofd, &orr, convs[ovix].sstat, convs[ovix].sstatlen,
		                     convs[ovix].tstat, convs[ovix].tstatlen,
		                     irr.ndeviat);

		// cleanup
		//
		free(convs[ovix].tstat);
	}

	printf("Samples converted: %llu\n", count);
}

//
// Function that calls the appropriate function to convert a struct
// from the sstat structure
//
static void
do_sconvert(struct sconvstruct *cur, struct sconvstruct *next)
{
	if (next->structconv)
	{
		(*(next->structconv))(cur->structptr,  next->structptr,
		                      cur->structsize, next->structsize);
	}
}

//
// Function that calls the appropriate function to convert a struct
// from one of the tstat structures
//
static void
do_tconvert(void *curtstat,          void *nexttstat,
	    struct tconvstruct *cur, struct tconvstruct *next)
{
	if (next->structconv)
	{
		(*(next->structconv))(curtstat + cur->structoffset,
		                      nexttstat + next->structoffset,
		                      cur->structsize, next->structsize);
	}
}


//
// Function to copy input file transparently to output file
//
static void
copy_file(int ifd, int ofd)
{
	unsigned char	buf[64*1024];
	int		nr, nw;

	(void) lseek(ifd, 0, SEEK_SET);
	(void) lseek(ofd, 0, SEEK_SET);

	while ( (nr = read(ifd, buf, sizeof buf)) > 0)
	{
		if ( (nw = write(ofd, buf, nr)) < nr)
		{
			if (nw == -1)
			{
				perror("write output file");
				exit(42);
			}
			else
			{
 				fprintf(stderr,
 					"Output file saturated\n");
				exit(42);
			}
		}
	}

	if (nr == -1)
	{
		perror("read input file");
		exit(42);
	}
	else
	{
		printf("Raw file copied (version already up-to-date)\n");
	}
}

//
// Function that opens an existing raw file and
// verifies the magic number
//
static int
openin(char *infile)
{
	int	rawfd;

	/*
	** open raw file for reading
	*/
	if ( (rawfd = open(infile, O_RDONLY)) == -1)
	{
		fprintf(stderr, "%s - ", infile);
		perror("open for reading");
		return -1;
	}

	return rawfd;
}

//
// Function that reads a chunk of bytes from
// the input raw file
//
static void
readin(int rawfd, void *buf, int size)
{
	/*
	** read the requested chunk and verify
	*/
	if ( read(rawfd, buf, size) < size)
	{
		fprintf(stderr, "can not read raw file\n");
		close(rawfd);
		exit(9);
	}
}


//
// Function that creates a raw log for output
//
static int
openout(char *outfile)
{
	int	rawfd;

	/*
	** create new output file
	*/
	if ( (rawfd = creat(outfile, 0666)) == -1)
	{
		fprintf(stderr, "%s - ", outfile);
		perror("create raw output file");
		return -1;
	}

	return rawfd;
}

//
// Function that reads a chunk of bytes from
// the input raw file
//
static void
writeout(int rawfd, void *buf, int size)
{
	/*
	** write the provided chunk and verify
	*/
	if ( write(rawfd, buf, size) < size)
	{
		fprintf(stderr, "can not write raw file\n");
		close(rawfd);
		exit(10);
	}
}

//
// Function that shows the usage message
//
void
prusage(char *name)
{
	fprintf(stderr,
		"Usage: %s [-t version] rawinput [rawoutput]\n", name);
	fprintf(stderr,
		"\t-t version      target version (default: %d.%d) for output\n",
			(convs[numconvs-1].version >> 8) & 0x7f,
			 convs[numconvs-1].version  & 0x7f);

	exit(1);
}


//
// Function to read the system-level statistics from the current offset
//
static int
getrawsstat(int rawfd, struct sstat *sp, int complen)
{
	Byte		*compbuf;
	unsigned long	uncomplen = sizeof(struct sstat);
	int		rv;

	compbuf = malloc(complen);

	ptrverify(compbuf, "Malloc failed for reading compressed sysstats\n");

	if ( read(rawfd, compbuf, complen) < complen)
	{
		free(compbuf);
		fprintf(stderr,
			"Failed to read %d bytes for system\n", complen);
		return 0;
	}

	rv = uncompress((Byte *)sp, &uncomplen, compbuf, complen);

	testcompval(rv, "sstat", "uncompress");

	free(compbuf);

	return 1;
}

//
// Function to read the process-level statistics from the current offset
//
static int
getrawtstat(int rawfd, struct tstat *pp, unsigned long uncomplen,
                       int complen, int ndeviat)
{
	Byte		*compbuf;
	int		rv;

	compbuf = malloc(complen);

	ptrverify(compbuf, "Malloc failed for reading compressed procstats\n");

	if ( read(rawfd, compbuf, complen) < complen)
	{
		free(compbuf);
		fprintf(stderr,
			"Failed to read %d bytes for tasks\n", complen);
		return 0;
	}

	rv = uncompress((Byte *)pp, &uncomplen, compbuf, complen);

	testcompval(rv, "tstat", "uncompress");

	free(compbuf);

	return 1;
}

//
// Function that writes a new sample to the output file
//
static void
writesamp(int ofd, struct rawrecord *rr,
	 void *sstat, int sstatlen, void *tstat, int tstatlen, int ntask)
{
	int			rv;
	Byte			scompbuf[sstatlen], *pcompbuf;
	unsigned long		scomplen = sizeof scompbuf;
	unsigned long		pcomplen = tstatlen * ntask;

	/*
	** compress system- and process-level statistics
	*/
	rv = compress(scompbuf, &scomplen,
				(Byte *)sstat, (unsigned long)sstatlen);

	testcompval(rv, "sstat", "compress");

	pcompbuf = malloc(pcomplen);

	ptrverify(pcompbuf, "Malloc failed for compression buffer\n");

	rv = compress(pcompbuf, &pcomplen, (Byte *)tstat,
						(unsigned long)pcomplen);

	testcompval(rv, "tstat", "compress");

	rr->scomplen	= scomplen;
	rr->pcomplen	= pcomplen;

	if ( write(ofd, rr, sizeof *rr) == -1)
	{
		perror("write raw record");
		exit(7);
	}

	/*
	** write compressed system status structure to file
	*/
	if ( write(ofd, scompbuf, scomplen) == -1)
	{
		perror("write raw status record");
		exit(7);
	}

	/*
	** write compressed list of process status structures to file
	*/
	if ( write(ofd, pcompbuf, pcomplen) == -1)
	{
		perror("write raw process record");
		exit(7);
	}

	free(pcompbuf);
}


//
// check success of (de)compression
//
static void
testcompval(int rv, char *name, char *func)
{
	switch (rv)
	{
	   case Z_OK:
	   case Z_STREAM_END:
	   case Z_NEED_DICT:
		break;

	   case Z_MEM_ERROR:
		fprintf(stderr, "%s %s: failed due to lack of memory\n", name, func);
		exit(7);

	   case Z_BUF_ERROR:
		fprintf(stderr, "%s %s: failed due to lack of room in buffer\n",
								name, func);
		exit(7);

   	   case Z_DATA_ERROR:
		fprintf(stderr,
		        "%s %s: failed due to corrupted/incomplete data\n", name, func);
		exit(7);

	   default:
		fprintf(stderr,
		        "%s %s: unexpected error %d\n", name, func, rv);
		exit(7);
	}
}

//
// generic pointer verification after malloc
//
void
ptrverify(const void *ptr, const char *errormsg, ...)
{
        if (!ptr)
        {
                va_list args;
                va_start(args, errormsg);
                vfprintf(stderr, errormsg, args);
                va_end  (args);

                exit(13);
        }
}


/*
** return maximum number of digits for PID/TID
** from the current system
*/
int
getpidwidth(void)
{
	FILE 	*fp;
	char	linebuf[64];
	int	numdigits = 5;

        if ( (fp = fopen("/proc/sys/kernel/pid_max", "r")) != NULL)
        {
                if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
                {
                        numdigits = strlen(linebuf) - 1;
                }

                fclose(fp);
        }

	return numdigits;
}
