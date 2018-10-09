/*
** ATOP - System & Process Monitor 
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains the Linux-specific functions to calculate
** figures to be visualized.
** ==========================================================================
** Author:      Gerlof Langeveld
**              Original version.
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        July 2002
**
** Author:	JC van Winkel - AT Computing, Nijmegen, Holland
**              Complete redesign.
** E-mail:      jc@ATComputing.nl
** Date:        November 2009
** --------------------------------------------------------------------------
** Copyright (C) 2009-2010 JC van Winkel
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
** $Log: showlinux.c,v $
** Revision 1.70  2010/10/23 14:04:12  gerlof
** Counters for total number of running and sleep threads (JC van Winkel).
**
** Revision 1.69  2010/05/18 19:20:08  gerlof
** Introduce CPU frequency and scaling (JC van Winkel).
**
** Revision 1.68  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.67  2010/04/17 17:20:33  gerlof
** Allow modifying the layout of the columns in the system lines.
**
** Revision 1.66  2010/03/16 21:14:46  gerlof
** Program and user selection can be combined with program and user
** accumulation.
**
** Revision 1.65  2010/03/04 10:53:26  gerlof
** Support I/O-statistics on logical volumes and MD devices.
**
** Revision 1.64  2010/01/18 18:06:28  gerlof
** Modified priorities for system-level columns.
**
** Revision 1.63  2010/01/16 12:54:33  gerlof
** Corrected order of columns.
**
** Revision 1.62  2010/01/16 11:38:02  gerlof
** Corrected counters for patched kernels (JC van Winkel).
**
** Revision 1.61  2010/01/08 11:25:56  gerlof
** Corrected column-width and priorities of network-stats.
**
** Revision 1.60  2010/01/03 18:27:19  gerlof
** *** empty log message ***
**
** Revision 1.59  2009/12/19 21:01:28  gerlof
** Improved syntax checking for ownprocline keyword (JC van Winkel).
**
** Revision 1.58  2009/12/17 11:59:28  gerlof
** Gather and display new counters: dirty cache and guest cpu usage.
**
** Revision 1.57  2009/12/17 10:51:19  gerlof
** Allow own defined process line with key 'o' and a definition
** in the atoprc file.
**
** Revision 1.56  2009/12/17 09:13:19  gerlof
** Reformatted some fields for better grouping of info.
**
** Revision 1.55  2009/12/12 10:11:18  gerlof
** Register and display end date and end time for process.
**
** Revision 1.54  2009/12/12 09:06:48  gerlof
** \Corrected cumulated disk I/O per user/program (JC van Winkel).
**
** Revision 1.53  2009/12/10 14:02:39  gerlof
** Add EUID, SUID and FSUID (and similar for GID's).
**
** Revision 1.52  2009/12/10 11:56:34  gerlof
** Various bug-solutions.
**
** Revision 1.51  2009/12/10 10:08:01  gerlof
** Major redesign for improved user interface (variable number of columns).
** Made by JC van Winkel.
**
** Revision 1.49  2008/03/06 08:38:28  gerlof
** Register/show ppid of a process.
**
** Revision 1.48  2008/01/18 07:37:05  gerlof
** Show information about the state of the individual threads
** in the scheduling report shown with keystroke 's'.
**
** Revision 1.47  2008/01/07 11:34:18  gerlof
** Correct the sort-order of network-interfaces (on busy-percentage).
**
** Revision 1.46  2007/11/07 09:23:29  gerlof
** Modified format for avg1, avg5 and avg15 (CPL) when counters too large.
**
** Revision 1.45  2007/11/05 11:43:25  gerlof
** Bug-solution for new-process indicator on 64-bits machines.
**
** Revision 1.44  2007/11/05 10:57:56  gerlof
** Bug-solution for huge exit code on 64-bits machines.
**
** Revision 1.43  2007/08/17 09:45:57  gerlof
** Experimental: gather info about HTTP statistics.
**
** Revision 1.42  2007/08/16 12:02:04  gerlof
** Add support for atopsar reporting.
** Concerns modification of networking-counters.
**
** Revision 1.41  2007/07/04 10:18:16  gerlof
** Bug-solution for division by zero.
**
** Revision 1.40  2007/07/03 09:02:29  gerlof
** Support Apache-statistics.
**
** Revision 1.39  2007/03/22 10:12:54  gerlof
** Support for io counters (>= kernel 2.6.20).
**
** Revision 1.38  2007/03/21 14:22:24  gerlof
** Handle io counters maintained from 2.6.20
**
** Revision 1.37  2007/02/13 10:36:09  gerlof
** Removal of external declarations.
** Use of hertz variable instead of HZ.
**
** Revision 1.36  2007/01/26 12:11:07  gerlof
** Add configuration-value 'swoutcritsec'.
**
** Revision 1.35  2007/01/26 10:25:42  gerlof
** Introduce steal percentage for virtual machines.
** Correct bug: when one interface is colored all subsequent interfaces
**              are colored.
**
** Revision 1.34  2007/01/18 10:58:45  gerlof
** Only check for committed limit if it is not zero.
**
** Revision 1.33  2007/01/18 10:37:09  gerlof
** Add support for colors.
** Add support for automatic determination of most critical resource.
** Add support for parsing of new arguments in ~/.atoprc
**
** Revision 1.32  2006/11/13 13:48:46  gerlof
** Implement load-average counters, context-switches and interrupts.
**
** Revision 1.31  2006/02/07 08:38:49  gerlof
** Swapped the zombie counter and exit counter in the PRC-line.
**
** Revision 1.30  2006/02/07 08:30:07  gerlof
** Add possibility to show counters per second.
** Ease parsing of output-lines by fixed number of columns per line.
**
** Revision 1.29  2006/01/30 09:24:12  gerlof
** PRC-line: 'exits' modified to 'exit' to save space.
**
** Revision 1.28  2006/01/30 09:14:26  gerlof
** Extend memory counters (a.o. page scans).
**
** Revision 1.27  2005/11/04 14:16:45  gerlof
** Minor bug-solutions.
**
** Revision 1.26  2005/10/28 09:51:29  gerlof
** All flags/subcommands are defined as macro's.
** Subcommand 'p' has been changed to 'z' (pause).
**
** Revision 1.25  2005/10/21 09:51:11  gerlof
** Per-user accumulation of resource consumption.
**
** Revision 1.24  2004/12/14 15:06:48  gerlof
** Implementation of patch-recognition for disk and network-statistics.
**
** Revision 1.23  2004/10/28 08:31:41  gerlof
** New counter: vm committed space
**
** Revision 1.22  2004/09/24 10:02:46  gerlof
** Wrong cpu-numbers for system level statistics.
**
** Revision 1.21  2004/09/23 08:21:10  gerlof
** Added wait-percentage per cpu.
**
** Revision 1.20  2004/09/23 07:37:34  gerlof
** Consistent handling of CPU percentages on system-level and process-level.
**
** Revision 1.19  2004/09/13 09:20:21  gerlof
** Modify subcommands (former 's' -> 'v', 'v' -> 'V', new 's').
**
** Revision 1.18  2004/09/02 10:55:21  root
** Added sleep-average to process-info.
**
** Revision 1.17  2004/08/31 09:53:31  gerlof
** Show information about underlying threads.
**
** Revision 1.16  2004/06/01 11:58:34  gerlof
** Regular expressions for selections on process-name and user-name.
**
** Revision 1.15  2004/05/06 09:47:59  gerlof
** Ported to kernel-version 2.6.
**
** Revision 1.14  2003/07/07 09:27:34  gerlof
** Cleanup code (-Wall proof).
**
** Revision 1.13  2003/07/03 12:04:25  gerlof
** Minor bug fixes.
**
** Revision 1.12  2003/06/30 11:29:57  gerlof
** Enlarge counters to 'long long'.
**
** Revision 1.11  2003/06/24 06:22:10  gerlof
** Limit number of system resource lines.
**
** Revision 1.10  2003/02/07 10:43:22  gerlof
** Solved a division-by-zero bug for process-percentage.
**
** Revision 1.9  2003/01/24 14:20:57  gerlof
** If possible, also show commandline when process has exited.
**
** Revision 1.8  2003/01/17 07:32:49  gerlof
** Show the full command-line per process (option 'c').
**
** Revision 1.7  2002/10/04 10:05:54  gerlof
** Bug-solution: New process indicator in static output set when needed.
**
** Revision 1.6  2002/10/03 11:14:42  gerlof
** Modify (effective) uid/gid to real uid/gid.
**
** Revision 1.5  2002/09/26 13:52:51  gerlof
** Limit header lines by not showing disks.
** Limit header lines by not showing disks.
**
** Revision 1.4  2002/09/16 08:59:13  gerlof
** Change field EXCODE to STATUS for support of indicator of newly created
** processes.
**
** Revision 1.3  2002/09/02 08:42:44  gerlof
** Bug-solution: blank line after header when more than 999 screens of
** process-list information.
**
** Revision 1.2  2002/08/30 07:11:20  gerlof
** Minor changes in the header-line of the process list.
**
** Revision 1.1  2002/07/24 11:14:16  gerlof
** Initial revision
**
**
** Initial
**
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <curses.h>
#include <pwd.h>
#include <grp.h>
#include <regex.h>

#include "atop.h"
#include "photoproc.h"
#include "photosyst.h"
#include "showgeneric.h"
#include "showlinux.h"

static void	make_proc_dynamicgen(void);

/*
** critical percentages for occupation-percentage;
** these defaults can be overruled via the config-file
*/
int   cpubadness = 90;        /* percentage           */
int   membadness = 90;        /* percentage           */
int   swpbadness = 80;        /* percentage           */
int   dskbadness = 70;        /* percentage           */
int   netbadness = 90;        /* percentage           */
int   pagbadness = 10;        /* number per second    */

int   almostcrit = 80;        /* percentage           */

/*
 * tables with all sys_printdefs
 */
sys_printdef *prcsyspdefs[] = {
	&syspdef_PRCSYS,
	&syspdef_PRCUSER,
	&syspdef_PRCNPROC,
        &syspdef_PRCNRUNNING,
        &syspdef_PRCNSLEEPING,
        &syspdef_PRCNDSLEEPING,
	&syspdef_PRCNZOMBIE,
	&syspdef_PRCCLONES,
	&syspdef_PRCNNEXIT,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *cpusyspdefs[] = {
	&syspdef_CPUSYS,
	&syspdef_CPUUSER,
	&syspdef_CPUIRQ,
	&syspdef_CPUIDLE,
	&syspdef_CPUWAIT,
	&syspdef_BLANKBOX,
	&syspdef_CPUIPC,
	&syspdef_CPUCYCLE,
	&syspdef_CPUFREQ,
	&syspdef_CPUSCALE,
	&syspdef_CPUSTEAL,
	&syspdef_CPUGUEST,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *cpisyspdefs[] = {
	&syspdef_CPUISYS,
	&syspdef_CPUIUSER,
	&syspdef_CPUIIRQ,
	&syspdef_CPUIIDLE,
	&syspdef_CPUIWAIT,
	&syspdef_BLANKBOX,
	&syspdef_CPUIIPC,
	&syspdef_CPUICYCLE,
	&syspdef_CPUIFREQ,
	&syspdef_CPUISCALE,
	&syspdef_CPUISTEAL,
	&syspdef_CPUIGUEST,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *cplsyspdefs[] = {
	&syspdef_CPLAVG1,
	&syspdef_CPLAVG5,
	&syspdef_CPLAVG15,
	&syspdef_CPLCSW,
	&syspdef_CPLNUMCPU,
	&syspdef_CPLINTR,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *memsyspdefs[] = {
	&syspdef_MEMTOT,
	&syspdef_MEMFREE,
	&syspdef_MEMCACHE,
	&syspdef_MEMDIRTY,
	&syspdef_MEMBUFFER,
	&syspdef_MEMSLAB,
	&syspdef_RECSLAB,
	&syspdef_BLANKBOX,
	&syspdef_SHMEM,
	&syspdef_SHMRSS,
	&syspdef_SHMSWP,
	&syspdef_BLANKBOX,
	&syspdef_VMWBAL,
	&syspdef_BLANKBOX,
	&syspdef_HUPTOT,
	&syspdef_HUPUSE,
        0
};
sys_printdef *swpsyspdefs[] = {
	&syspdef_SWPTOT,
	&syspdef_SWPFREE,
	&syspdef_SWPCOMMITTED,
	&syspdef_SWPCOMMITLIM,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *pagsyspdefs[] = {
	&syspdef_PAGSCAN,
	&syspdef_PAGSTEAL,
	&syspdef_PAGSTALL,
	&syspdef_PAGSWIN,
	&syspdef_PAGSWOUT,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *contsyspdefs[] = {
	&syspdef_CONTNAME,
	&syspdef_CONTNPROC,
	&syspdef_CONTCPU,
	&syspdef_CONTMEM,
	&syspdef_BLANKBOX,
	0
};
sys_printdef *dsksyspdefs[] = {
	&syspdef_DSKNAME,
	&syspdef_DSKBUSY,
	&syspdef_DSKNREAD,
	&syspdef_DSKNWRITE,
	&syspdef_DSKMBPERSECWR,
	&syspdef_DSKMBPERSECRD,
	&syspdef_DSKKBPERWR,
	&syspdef_DSKKBPERRD,
	&syspdef_DSKAVQUEUE,
	&syspdef_DSKAVIO,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *nfsmntsyspdefs[] = {
	&syspdef_NFMPATH,
	&syspdef_NFMSERVER,
	&syspdef_NFMTOTREAD,
	&syspdef_NFMTOTWRITE,
	&syspdef_NFMNREAD,
	&syspdef_NFMNWRITE,
	&syspdef_NFMDREAD,
	&syspdef_NFMDWRITE,
	&syspdef_NFMMREAD,
	&syspdef_NFMMWRITE,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *nfcsyspdefs[] = {
	&syspdef_NFCRPCCNT,
	&syspdef_NFCRPCREAD,
	&syspdef_NFCRPCWRITE,
	&syspdef_NFCRPCRET,
	&syspdef_NFCRPCARF,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *nfssyspdefs[] = {
	&syspdef_NFSRPCCNT,
	&syspdef_NFSRPCREAD,
	&syspdef_NFSRPCWRITE,
	&syspdef_NFSNRBYTES,
	&syspdef_NFSNWBYTES,
	&syspdef_NFSNETTCP,
	&syspdef_NFSNETUDP,
	&syspdef_NFSBADFMT,
	&syspdef_NFSBADAUT,
	&syspdef_NFSBADCLN,
	&syspdef_NFSRCHITS,
	&syspdef_NFSRCMISS,
	&syspdef_NFSRCNOCA,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *nettranssyspdefs[] = {
	&syspdef_NETTRANSPORT,
	&syspdef_NETTCPI,
	&syspdef_NETTCPO,
	&syspdef_NETUDPI,
	&syspdef_NETUDPO,
	&syspdef_NETTCPACTOPEN,
	&syspdef_NETTCPPASVOPEN,
	&syspdef_NETTCPRETRANS,
	&syspdef_NETTCPINERR,
	&syspdef_NETTCPORESET,
	&syspdef_NETUDPNOPORT,
	&syspdef_NETUDPINERR,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *netnetsyspdefs[] = {
	&syspdef_NETNETWORK,
	&syspdef_NETIPI,
	&syspdef_NETIPO,
	&syspdef_NETIPFRW,
	&syspdef_NETIPDELIV,
	&syspdef_NETICMPIN,
	&syspdef_NETICMPOUT,
	&syspdef_BLANKBOX,
        0
};
sys_printdef *netintfsyspdefs[] = {
	&syspdef_NETNAME,
	&syspdef_NETPCKI,
	&syspdef_NETPCKO,
	&syspdef_NETSPEEDMAX,
	&syspdef_NETSPEEDIN,
	&syspdef_NETSPEEDOUT,
	&syspdef_NETCOLLIS,
	&syspdef_NETMULTICASTIN,
	&syspdef_NETRCVERR,
	&syspdef_NETSNDERR,
	&syspdef_NETRCVDROP,
	&syspdef_NETSNDDROP,
	&syspdef_BLANKBOX,
        0
};


/*
 * table with all proc_printdefs
 */
proc_printdef *allprocpdefs[]= 
{
	&procprt_PID,
	&procprt_TID,
	&procprt_PPID,
	&procprt_SYSCPU,
	&procprt_USRCPU,
	&procprt_VGROW,
	&procprt_RGROW,
	&procprt_MINFLT,
	&procprt_MAJFLT,
	&procprt_VSTEXT,
	&procprt_VSIZE,
	&procprt_RSIZE,
	&procprt_PSIZE,
	&procprt_VSLIBS,
	&procprt_VDATA,
	&procprt_VSTACK,
	&procprt_SWAPSZ,
	&procprt_CMD,
	&procprt_RUID,
	&procprt_EUID,
	&procprt_SUID,
	&procprt_FSUID,
	&procprt_RGID,
	&procprt_EGID,
	&procprt_SGID,
	&procprt_FSGID,
	&procprt_CTID,
	&procprt_VPID,
	&procprt_CID,
	&procprt_STDATE,
	&procprt_STTIME,
	&procprt_ENDATE,
	&procprt_ENTIME,
	&procprt_THR,
	&procprt_TRUN,
	&procprt_TSLPI,
	&procprt_TSLPU,
	&procprt_POLI,
	&procprt_NICE,
	&procprt_PRI,
	&procprt_RTPR,
	&procprt_CURCPU,
	&procprt_ST,
	&procprt_EXC,
	&procprt_S,
	&procprt_COMMAND_LINE,
	&procprt_NPROCS,
	&procprt_RDDSK,
	&procprt_WRDSK,
	&procprt_CWRDSK,
	&procprt_WCANCEL,
	&procprt_TCPRCV,
	&procprt_TCPRASZ,
	&procprt_TCPSND,
	&procprt_TCPSASZ,
	&procprt_UDPRCV,
	&procprt_UDPRASZ,
	&procprt_UDPSND,
	&procprt_UDPSASZ,
	&procprt_RNET,
	&procprt_SNET,
	&procprt_BANDWI,
	&procprt_BANDWO,
	&procprt_SORTITEM,
        0
};

/*
 * table with all proc_printdefs with PID/TID width to be initialized
 */
proc_printdef *idprocpdefs[]= 
{
	&procprt_PID,
	&procprt_TID,
	&procprt_PPID,
	&procprt_VPID,
	0
};


/***************************************************************/
/*
 * output definitions for process data
 * these should be user configurable
 */
proc_printpair userprocs[MAXITEMS];
proc_printpair memprocs[MAXITEMS];
proc_printpair schedprocs[MAXITEMS];
proc_printpair genprocs[MAXITEMS];
proc_printpair dskprocs[MAXITEMS];
proc_printpair netprocs[MAXITEMS];
proc_printpair varprocs[MAXITEMS];
proc_printpair cmdprocs[MAXITEMS];
proc_printpair ownprocs[MAXITEMS];
proc_printpair totusers[MAXITEMS];
proc_printpair totprocs[MAXITEMS];
proc_printpair totconts[MAXITEMS];


/*****************************************************************/
/*
 * output definitions for system data
 * these should be user configurable
 */
sys_printpair sysprcline[MAXITEMS];
sys_printpair allcpuline[MAXITEMS];
sys_printpair indivcpuline[MAXITEMS];
sys_printpair cplline[MAXITEMS];
sys_printpair memline[MAXITEMS];
sys_printpair swpline[MAXITEMS];
sys_printpair pagline[MAXITEMS];
sys_printpair contline[MAXITEMS];
sys_printpair dskline[MAXITEMS];
sys_printpair nettransportline[MAXITEMS];
sys_printpair netnetline[MAXITEMS];
sys_printpair netinterfaceline[MAXITEMS];
sys_printpair nfsmountline[MAXITEMS];
sys_printpair nfcline[MAXITEMS];
sys_printpair nfsline[MAXITEMS];

typedef struct {
        const char *name;
        int        prio;
} name_prio;

/*
** make an string,int pair array from a string.  chop based on spaces/tabs
** example: input: "ABCD:3  EFG:1   QWE:16"
**         output: { { "ABCD", 3 }, {"EFG", 1},  { "QWE", 16}, { 0, 0 }  }
*/
static void
makeargv(char *line, const char *linename, name_prio *vec) 
{
        int   i=0;
        char *p=line;
        char *name=0;
        char *prio=0;

        // find pair and scan it
        while (*p && i<MAXITEMS-1) 
        {
                // skip initial spaces
                while (*p && (*p==' ' || *p=='\t'))
                {
                        ++p;
                }
                if (! *p) 
                {
                        break;
                }
                name=p;
                // found a new word; let's chop!
                while (*p && *p !=':')
                {
                        ++p;
                }
                if (*p==':')
                {
                        *p=0;
                }
                else
                {
		        fprintf(stderr,
	                "atoprc - %s: no name:prio pair for "
                        "`%s'\n", name, linename);
                        cleanstop(1);
                }

                /* now get number */
                p++;
                prio=p;
                errno = 0;    /* To distinguish success/failure after call */

                long lprio=strtol(p, &p, 10);

                if (prio==p || errno == ERANGE || lprio >= INT_MAX || lprio <0)
                {
		        fprintf(stderr,
			"atoprc - %s: item `%s` has "
                        "invalid priority `", linename, name);
                        while (*prio && *prio !=' ') {
                            fputc(*prio, stderr);
                            prio++;
                        }
                        fprintf(stderr, "'\n");
                        cleanstop(1);
                }
                vec[i].name=name;
                vec[i].prio=lprio;

                ++i;
        }
                
        vec[i].name=0;
}


/*
 * make_sys_prints: make array of sys_printpairs
 * input: string, sys_printpair array, maxentries
 */
void
make_sys_prints(sys_printpair *ar, int maxn, const char *pairs, 
                sys_printdef *permissables[], const char *linename)
{
        name_prio items[MAXITEMS];
        int n=strlen(pairs);

        char str[n+1];
        strcpy(str, pairs);

        makeargv(str, linename, items);

        int i;
        for(i=0; items[i].name && i<maxn-1; ++i) 
        {
                const char *name=items[i].name;
                int j;
                for (j=0; permissables[j] != 0; ++j)
                {
                        if (strcmp(permissables[j]->configname, name)==0)
                        {
                                ar[i].f=permissables[j];
                                ar[i].prio=items[i].prio;
                                break;
                        }
                }
                if (permissables[j]==0)
                {
                        fprintf(stderr,
				"atoprc - own system line: item %s invalid in %s line!\n",
				name, linename);
                        cleanstop(1);
                }
        }
        ar[i].f=0;
        ar[i].prio=0;
}



/*
 * init_proc_prints: determine width of columns that are
 *                   dependent of dynamic values 
 */
void 
init_proc_prints()
{
	int 	i, numdigits = 5;
	char	linebuf[64];
	FILE	*fp;

	/*
	** determine maximum number of digits for PID/TID
	*/
	if ( (fp = fopen("/proc/sys/kernel/pid_max", "r")) != NULL)
	{
                if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
                {
                        numdigits = strlen(linebuf) - 1;
                }

                fclose(fp);
        }

	/*
	** fill number of digits for various PID/TID columns
	** and reformat header to new width
	*/
	for (i=0; idprocpdefs[i] != 0; i++)
	{
		idprocpdefs[i]->width = numdigits;

		if ( strlen(idprocpdefs[i]->head) < numdigits)
		{
			char *p = malloc(numdigits + 1);

			ptrverify(p,
				"Malloc failed for formatted header\n",
				numdigits + 1);

			sprintf(p, "%*s", numdigits, idprocpdefs[i]->head);
			idprocpdefs[i]->head = p;
		}
	}
}

/*
 * make_proc_prints: make array of proc_printpairs
 * input: string, proc_printpair array, maxentries
 */
void 
make_proc_prints(proc_printpair *ar, int maxn, const char *pairs, 
const char *linename)
{
        name_prio items[MAXITEMS];
        int n=strlen(pairs);

        char str[n+1];
        strcpy(str, pairs);

        makeargv(str, linename, items);

        int i;
        for(i=0; items[i].name && i<maxn-1; ++i) 
        {
                const char *name=items[i].name;
                int j;
                for (j=0; allprocpdefs[j] != 0; ++j)
                {
                        if (strcmp(allprocpdefs[j]->configname, name)==0)
                        {
                                ar[i].f=allprocpdefs[j];
                                ar[i].prio=items[i].prio;
                                break;
                        }
                }
                if (allprocpdefs[j]==0)
                {
                        fprintf(stderr,
				"atoprc - ownprocline: item %s invalid!\n",
				name);
                        cleanstop(1);
                }
        }
        ar[i].f=0;
        ar[i].prio=0;
}

/*
** calculate the total consumption on system-level for the 
** four main resources
*/
void
totalcap(struct syscap *psc, struct sstat *sstat,
                             struct tstat **proclist, int nactproc)
{
        register int    i;

        psc->nrcpu      = sstat->cpu.nrcpu;
        psc->availcpu   = sstat->cpu.all.stime +
                          sstat->cpu.all.utime +
                          sstat->cpu.all.ntime +
                          sstat->cpu.all.itime +
                          sstat->cpu.all.wtime +
                          sstat->cpu.all.Itime +
                          sstat->cpu.all.Stime +
                          sstat->cpu.all.steal;

        psc->availmem   = sstat->mem.physmem * pagesize/1024;

	/*
	** calculate total transfer issued by the active processes
	** for disk and for network
	*/
	for (psc->availnet=psc->availdsk=0, i=0; i < nactproc; i++) 
	{
		struct tstat 	*curstat = *(proclist+i);
		count_t		nett_wsz;

		psc->availnet += curstat->net.tcpssz;
		psc->availnet += curstat->net.tcprsz;
		psc->availnet += curstat->net.udpssz;
		psc->availnet += curstat->net.udprsz;

		if (curstat->dsk.wsz > curstat->dsk.cwsz)
			nett_wsz = curstat->dsk.wsz -
			           curstat->dsk.cwsz;
		else
			nett_wsz = 0;

		psc->availdsk += curstat->dsk.rsz;
		psc->availdsk += nett_wsz;
	}
}

/*
** calculate cumulative system- and user-time for all active processes
*/
void
pricumproc(struct sstat *sstat, struct devtstat *devtstat,
           int nexit, unsigned int noverflow, int avgval, int nsecs)
{

        static int firsttime=1;

        if (firsttime)
        {
                firsttime=0;

                if (sysprcline[0].f == 0)
                {
                    make_sys_prints(sysprcline, MAXITEMS,
                        "PRCSYS:8 "
                        "PRCUSER:8 "
	                "BLANKBOX:0 "
                        "PRCNPROC:7 "
                        "PRCNRUNNING:5 "
                        "PRCNSLEEPING:5 "
                        "PRCNDSLEEPING:5 "
                        "PRCNZOMBIE:5 "
                        "PRCCLONES:4 "
	                "BLANKBOX:0 "
                        "PRCNNEXIT:6", prcsyspdefs, "builtin sysprcline");
                }
                if (allcpuline[0].f == 0)
                {
                    make_sys_prints(allcpuline, MAXITEMS,
	                "CPUSYS:9 "
	                "CPUUSER:8 "
	                "CPUIRQ:6 "
	                "BLANKBOX:0 "
	                "CPUIDLE:7 "
	                "CPUWAIT:7 "
                        "CPUSTEAL:2 "
                        "CPUGUEST:3 "
	                "BLANKBOX:0 "
                        "CPUIPC:5 "
                        "CPUCYCLE:4 "
                        "CPUFREQ:4 "
                        "CPUSCALE:4 ", cpusyspdefs, "builtin allcpuline");
                }

                if (indivcpuline[0].f == 0)
                {
                    make_sys_prints(indivcpuline, MAXITEMS,
	                "CPUISYS:9 "
                        "CPUIUSER:8 "
	                "CPUIIRQ:6 "
	                "BLANKBOX:0 "
	                "CPUIIDLE:7 "
	                "CPUIWAIT:7 "
                        "CPUISTEAL:2 "
                        "CPUIGUEST:3 "
	                "BLANKBOX:0 "
                        "CPUIIPC:5 "
                        "CPUICYCLE:4 "
                        "CPUIFREQ:4 "
                        "CPUISCALE:4 ", cpisyspdefs, "builtin indivcpuline");
                }

                if (cplline[0].f == 0)
                {
                    make_sys_prints(cplline, MAXITEMS,
	                "CPLAVG1:4 "
	                "CPLAVG5:3 "
	                "CPLAVG15:2 "
	                "BLANKBOX:0 "
	                "CPLCSW:6 "
	                "CPLINTR:5 "
	                "BLANKBOX:0 "
	                "CPLNUMCPU:1", cplsyspdefs, "builtin cplline");
                }

                if (memline[0].f == 0)
                {
                    make_sys_prints(memline, MAXITEMS,
	                "MEMTOT:6 "
	                "MEMFREE:7 "
	                "MEMCACHE:5 "
	                "MEMDIRTY:3 "
	                "MEMBUFFER:5 "
	                "MEMSLAB:5 "
	                "RECSLAB:2 "
	                "BLANKBOX:0 "
	                "SHMEM:4 "
	                "SHMRSS:3 "
	                "SHMSWP:1 "
	                "BLANKBOX:0 "
	                "VMWBAL:4 "
	                "BLANKBOX:0 "
	                "HUPTOT:4 "
	                "HUPUSE:3 ", memsyspdefs, "builtin memline");
                }
                if (swpline[0].f == 0)
                {
                    make_sys_prints(swpline, MAXITEMS,
	                "SWPTOT:3 "
	                "SWPFREE:4 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "SWPCOMMITTED:5 "
	                "SWPCOMMITLIM:6", swpsyspdefs, "builtin swpline");
                }
                if (pagline[0].f == 0)
                {
                    make_sys_prints(pagline, MAXITEMS,
	                "PAGSCAN:3 "
	                "PAGSTEAL:3 "
	                "PAGSTALL:1 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "PAGSWIN:3 "
	                "PAGSWOUT:4", pagsyspdefs, "builtin pagline");
                }
                if (contline[0].f == 0)
                {
                    make_sys_prints(contline, MAXITEMS,
	                "CONTNAME:8 "
	                "CONTNPROC:7 "
	                "CONTCPU:6 "
	                "CONTMEM:6 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 ", contsyspdefs, "builtin contline");
                }
                if (dskline[0].f == 0)
                {
                    make_sys_prints(dskline, MAXITEMS,
	                "DSKNAME:8 "
	                "DSKBUSY:7 "
	                "DSKNREAD:6 "
	                "DSKNWRITE:6 "
	                "DSKKBPERRD:4 "
	                "DSKKBPERWR:4 "
                        "DSKMBPERSECRD:5 "
                        "DSKMBPERSECWR:5 "
	                "DSKAVQUEUE:1 "
	                "DSKAVIO:5", dsksyspdefs, "builtin dskline");
                }
                if (nfsmountline[0].f == 0)
                {
                    make_sys_prints(nfsmountline, MAXITEMS,
	                "NFMPATH:8 "
	                "NFMSERVER:8 "
			"NFMTOTREAD:8 "
			"NFMTOTWRITE:8 "
	                "BLANKBOX:0 "
			"NFMNREAD:7 "
			"NFMNWRITE:6 "
	                "BLANKBOX:0 "
			"NFMDREAD:5 "
			"NFMDWRITE:4 "
	                "BLANKBOX:0 "
			"NFMMREAD:3 "
			"NFMMWRITE:2 "
	                "BLANKBOX:0 "
                        "BLANKBOX:0", nfsmntsyspdefs, "builtin nfsmountline");
                }
                if (nfcline[0].f == 0)
                {
                    make_sys_prints(nfcline, MAXITEMS,
	                "NFCRPCCNT:8 "
	                "NFCRPCREAD:7 "
	                "NFCRPCWRITE:7 "
	                "NFCRPCRET:5 "
	                "NFCRPCARF:5 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 ", nfcsyspdefs, "builtin nfcline");
		}
                if (nfsline[0].f == 0)
                {
                    make_sys_prints(nfsline, MAXITEMS,
	                "NFSRPCCNT:8 "
	                "NFSRPCREAD:6 "
	                "NFSRPCWRITE:6 "
	                "BLANKBOX:0 "
	                "NFSNRBYTES:7 "
	                "NFSNWBYTES:7 "
	                "BLANKBOX:0 "
	                "NFSNETTCP:5 "
	                "NFSNETUDP:5 "
	                "BLANKBOX:0 "
	                "NFSRCHITS:3 "
	                "NFSRCMISS:2 "
	                "NFSRCNOCA:1 "
	                "BLANKBOX:0 "
	                "NFSBADFMT:4 "
	                "NFSBADAUT:4 "
	                "NFSBADCLN:4 ", nfssyspdefs, "builtin nfsline");
		}
                if (nettransportline[0].f == 0)
                {
                    make_sys_prints(nettransportline, MAXITEMS,
	                "NETTRANSPORT:9 "
	                "NETTCPI:8 "
	                "NETTCPO:8 "
	                "NETUDPI:8 "
	                "NETUDPO:8 "
                        "NETTCPACTOPEN:6 "
                        "NETTCPPASVOPEN:5 "
                        "NETTCPRETRANS:4 "
                        "NETTCPINERR:3 "
                        "NETTCPORESET:2 "
                        "NETUDPNOPORT:1 "
                        "NETUDPINERR:3", nettranssyspdefs, "builtin nettransportline");
                }
                if (netnetline[0].f == 0)
                {
                    make_sys_prints(netnetline, MAXITEMS,
                        "NETNETWORK:5 "
                        "NETIPI:4 "
                        "NETIPO:4 "
                        "NETIPFRW:4 "
                        "NETIPDELIV:4 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
	                "BLANKBOX:0 "
                        "NETICMPIN:1 "
                        "NETICMPOUT:1 ", netnetsyspdefs, "builtin netnetline");
                }
                if (netinterfaceline[0].f == 0)
                {
                    make_sys_prints(netinterfaceline, MAXITEMS,
	                "NETNAME:8 "
	                "BLANKBOX:0 "
	                "NETPCKI:7 "
	                "NETPCKO:7 "
	                "BLANKBOX:0 "
	                "NETSPEEDMAX:5 "
	                "NETSPEEDIN:6 "
	                "NETSPEEDOUT:6 "
	                "BLANKBOX:0 "
                        "NETCOLLIS:2 "
                        "NETMULTICASTIN:2 "
                        "NETRCVERR:4 "
                        "NETSNDERR:4 "
                        "NETRCVDROP:3 "
                        "NETSNDDROP:3", netintfsyspdefs, "builtin netinterfaceline");
                }
        }  // firsttime


        int     i;
        extraparam extra;


        for (i=0, extra.totut=extra.totst=0; i < devtstat->nprocactive; i++)
        {
		struct tstat *curstat = *(devtstat->procactive+i);

                extra.totut	+= curstat->cpu.utime;
                extra.totst 	+= curstat->cpu.stime;
        }

        extra.nproc	= devtstat->nprocall;
	extra.ntrun	= devtstat->totrun;
	extra.ntslpi	= devtstat->totslpi;
	extra.ntslpu	= devtstat->totslpu;
        extra.nzomb	= devtstat->totzombie;
        extra.nexit	= nexit;
        extra.noverflow	= noverflow;
        extra.avgval	= avgval;
        extra.nsecs	= nsecs;

        move(1, 0);
        showsysline(sysprcline, sstat, &extra, "PRC", 0);
}

/*
** print the header for the process list
*/
void
priphead(int curlist, int totlist, char *showtype, char *showorder,
							char autosort)
{
        static int      firsttime=1;
        static int      prev_supportflags = -1, prev_threadview = -1;

	/*
 	** determine once the layout of all per-process reports
	** except for the generic report (might change dynamically)
	*/
        if (firsttime) 
        {
		init_proc_prints();

                make_proc_prints(memprocs, MAXITEMS, 
                        "PID:10 TID:3 MINFLT:2 MAJFLT:2 VSTEXT:4 VSLIBS:4 "
			"VDATA:4 VSTACK:4 VSIZE:6 RSIZE:7 PSIZE:5 "
                        "VGROW:7 RGROW:8 SWAPSZ:5 RUID:1 EUID:0 "
                        "SORTITEM:9 CMD:10", 
                        "built-in memprocs");

                make_proc_prints(schedprocs, MAXITEMS, 
                        "PID:10 TID:6 CID:5 VPID:4 CTID:4 TRUN:7 TSLPI:7 "
			"TSLPU:7 POLI:8 NICE:9 PRI:9 RTPR:9 CPUNR:8 ST:8 "
			"EXC:8 S:8 SORTITEM:10 CMD:10", 
                        "built-in schedprocs");

                make_proc_prints(dskprocs, MAXITEMS, 
                        "PID:10 TID:4 RDDSK:9 "
                        "WRDSK:9 WCANCL:8 "
                        "SORTITEM:10 CMD:10", 
                        "built-in dskprocs");

                make_proc_prints(netprocs, MAXITEMS, 
                        "PID:10 TID:6 "
			"TCPRCV:9 TCPRASZ:4 TCPSND:9 TCPSASZ:4 "
			"UDPRCV:8 UDPRASZ:3 UDPSND:8 UDPSASZ:3 "
			"BANDWI:10 BANDWO:10 "
                        "SORTITEM:10 CMD:10", 
                        "built-in netprocs");

                make_proc_prints(varprocs, MAXITEMS,
                        "PID:10 TID:4 PPID:9 CID:2 VPID:1 CTID:1 "
			"RUID:8 RGID:8 EUID:5 EGID:4 "
     			"SUID:3 SGID:2 FSUID:3 FSGID:2 "
                        "STDATE:7 STTIME:7 ENDATE:5 ENTIME:5 "
			"ST:6 EXC:6 S:6 SORTITEM:10 CMD:10", 
                        "built-in varprocs");

                make_proc_prints(cmdprocs, MAXITEMS,
                        "PID:10 TID:4 S:8 SORTITEM:10 COMMAND-LINE:10", 
                        "built-in cmdprocs");

                make_proc_prints(totusers, MAXITEMS, 
                        "NPROCS:10 SYSCPU:9 USRCPU:9 VSIZE:6 "
                        "RSIZE:8 PSIZE:8 SWAPSZ:5 RDDSK:7 CWRDSK:7 "
			"RNET:6 SNET:6 SORTITEM:10 RUID:10", 
                        "built-in totusers");

                make_proc_prints(totprocs, MAXITEMS, 
                        "NPROCS:10 SYSCPU:9 USRCPU:9 VSIZE:6 "
                        "RSIZE:8 PSIZE:8 SWAPSZ:5 RDDSK:7 CWRDSK:7 "
			"RNET:6 SNET:6 SORTITEM:10 CMD:10", 
                        "built-in totprocs");

                make_proc_prints(totconts, MAXITEMS, 
                        "NPROCS:10 SYSCPU:9 USRCPU:9 VSIZE:6 "
                        "RSIZE:8 PSIZE:8 SWAPSZ:5 RDDSK:7 CWRDSK:7 "
			"RNET:6 SNET:6 SORTITEM:10 CID:10", 
                        "built-in totconts");
        }

	/*
 	** update the generic report if needed
	*/
	if (prev_supportflags != supportflags || prev_threadview != threadview)
	{
		make_proc_dynamicgen();

		prev_supportflags = supportflags;
		prev_threadview   = threadview;

		if (*showtype == MPROCNET && !(supportflags&NETATOP) )
		{
			*showtype  = MPROCGEN;
			*showorder = MSORTCPU;
		}
	}

        /*
        ** print the header line
        */
        switch (*showtype)
        {
           case MPROCGEN:
                showhdrline(genprocs, curlist, totlist, *showorder, autosort);
                break;

           case MPROCMEM:
                showhdrline(memprocs, curlist, totlist, *showorder, autosort);
                break;

           case MPROCDSK:
                showhdrline(dskprocs, curlist, totlist, *showorder, autosort);
                break;

           case MPROCNET:
                showhdrline(netprocs, curlist, totlist, *showorder, autosort);
                break;

           case MPROCVAR:
                showhdrline(varprocs, curlist, totlist, *showorder, autosort);
                break;

           case MPROCARG:
                showhdrline(cmdprocs, curlist, totlist, *showorder, autosort);
                break;

           case MPROCOWN:
                showhdrline(ownprocs, curlist, totlist, *showorder, autosort);
                break;

           case MPROCSCH:
                showhdrline(schedprocs, curlist, totlist, *showorder, autosort);
                break;

           case MCUMUSER:
                showhdrline(totusers, curlist, totlist, *showorder, autosort);
                break;

           case MCUMPROC:
                showhdrline(totprocs, curlist, totlist, *showorder, autosort);
                break;

           case MCUMCONT:
                showhdrline(totconts, curlist, totlist, *showorder, autosort);
                break;
        }
}

/*
** assemble the layout of the generic line,
** depending on the supported features (like
** I/O stats, network stats) and current view
*/
#define	FORMPID	"PID:10 "
#define	FORMTID	"TID:6 "
#define	FORMCID	"CID:5 "
#define	FORMCPU	"SYSCPU:9 USRCPU:9 "
#define FORMMEM	"VGROW:8 RGROW:8 "
#define FORMDSK	"RDDSK:7 CWRDSK:7 "
#define FORMNET	"RNET:6 SNET:6 "
#define FORMMSC	"RUID:3 EUID:2 ST:4 EXC:4 THR:4 S:4 CPUNR:4 "
#define FORMEND	"SORTITEM:10 CMD:10"

static void
make_proc_dynamicgen()
{
	char format[300], *p = format;

	memcpy(p, FORMPID, sizeof FORMPID -1);
	p += sizeof FORMPID -1;

	if (threadview)
	{
		memcpy(p, FORMTID, sizeof FORMTID -1);
		p += sizeof FORMTID -1;
	}

	if (supportflags & DOCKSTAT)
	{
		memcpy(p, FORMCID, sizeof FORMCID -1);
		p += sizeof FORMCID -1;
	}

	memcpy(p, FORMCPU, sizeof FORMCPU -1);
	p += sizeof FORMCPU -1;

	memcpy(p, FORMMEM, sizeof FORMMEM -1);
	p += sizeof FORMMEM -1;

	if (supportflags & IOSTAT)
	{
		memcpy(p, FORMDSK, sizeof FORMDSK -1);
		p += sizeof FORMDSK -1;
	}

	if (supportflags & NETATOP)
	{
		memcpy(p, FORMNET, sizeof FORMNET -1);
		p += sizeof FORMNET -1;
	}

	memcpy(p, FORMMSC, sizeof FORMMSC -1);
	p += sizeof FORMMSC -1;

	memcpy(p, FORMEND, sizeof FORMEND);
	p += sizeof FORMEND;

	make_proc_prints(genprocs, MAXITEMS, format, "built-in genprocs");
}

/*
** print the list of processes from the deviation-list
*/
int
priproc(struct tstat **proclist, int firstproc, int lastproc, int curline,
        int curlist, int totlist, char showtype, char showorder,
        struct syscap *sb, int nsecs, int avgval)
{
        register int            i;
        register struct tstat   *curstat;
        double                  perc;

        /*
        ** print info per actual process and maintain totals
        */
        for (i=firstproc; i < lastproc; i++)
        {
                curstat = *(proclist+i);

                if (screen && curline >= LINES) /* screen filled entirely ? */
                        break;

                /*
                ** calculate occupation-percentage of this process
                ** depending on selected resource
                */
                switch (showorder) 
                {
                   case MSORTCPU:
                        perc = 0.0;

                        if (sb->availcpu)
                        {
                                perc = (double)(curstat->cpu.stime +
                                                curstat->cpu.utime  ) * 100 /
                                                (sb->availcpu / sb->nrcpu);

                                if (perc > 100.0 * sb->nrcpu)
                                        perc = 100.0 * sb->nrcpu;

                                if (perc > 100.0 * curstat->gen.nthr)
                                        perc = 100.0 * curstat->gen.nthr;
                        }
                        break;

                   case MSORTMEM:
                        perc = 0.0;

                        if (sb->availmem)
                        {
                                perc = (double)curstat->mem.rmem *
                                               100.0 / sb->availmem;

                                if (perc > 100.0)
                                        perc = 100.0;
                        }
                        break;

                   case MSORTDSK:
                        perc = 0.0;

			if (sb->availdsk)
                        {
				count_t nett_wsz;


				if (curstat->dsk.wsz > curstat->dsk.cwsz)
					nett_wsz = curstat->dsk.wsz -
					           curstat->dsk.cwsz;
				else
					nett_wsz = 0;

				perc = (double)(curstat->dsk.rsz + nett_wsz) *
						100.0 / sb->availdsk;

				if (perc > 100.0)
					perc = 100.0;
                        }
                        break;

                   case MSORTNET:
                        perc = 0.0;

                        if (sb->availnet)
                        {
                                perc = (double)(curstat->net.tcpssz +
                                                curstat->net.tcprsz +
                                                curstat->net.udpssz +
                                                curstat->net.udprsz  ) *
                                                100.0 / sb->availnet;

                                if (perc > 100.0)
                                        perc = 100.0;
                        }
                        break;

                   default:
                        perc = 0.0;
                }

                /*
                ** now do the formatting of output
                */
                if (screen) {
                        move(curline,0);
                }

                switch (showtype)
                {
                   case MPROCGEN:
                        showprocline(genprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MPROCMEM:
                        showprocline(memprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MPROCDSK:
                        showprocline(dskprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MPROCNET:
                        showprocline(netprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MPROCVAR:
                        showprocline(varprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MPROCARG:
                        showprocline(cmdprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MPROCOWN:
                        showprocline(ownprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MPROCSCH:
                        showprocline(schedprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MCUMUSER:
                        showprocline(totusers, curstat, perc, nsecs, avgval);
                        break;

                   case MCUMPROC:
                        showprocline(totprocs, curstat, perc, nsecs, avgval);
                        break;

                   case MCUMCONT:
                        showprocline(totconts, curstat, perc, nsecs, avgval);
                        break;
                }

                curline++;
        }

        return curline;
}


/*
** print the system-wide statistics
*/
static void	pridisklike(extraparam *, struct perdsk *, char *,
		      char *, int, unsigned int *, int *, int, regex_t *);

int
prisyst(struct sstat *sstat, int curline, int nsecs, int avgval,
        int fixedhead, struct sselection *selp, char *highorderp,
        int maxcpulines, int maxdsklines, int maxmddlines,
	int maxlvmlines, int maxintlines, int maxnfslines, int maxcontlines)
{
        extraparam      extra;
        int             lin;
        count_t         busy;
        unsigned int    badness, highbadness=0;

        extra.nsecs	= nsecs;
        extra.avgval	= avgval;

        /*
        ** CPU statistics
        */
        extra.cputot = sstat->cpu.all.stime + sstat->cpu.all.utime +
                       sstat->cpu.all.ntime + sstat->cpu.all.itime +
                       sstat->cpu.all.wtime + sstat->cpu.all.Itime +
                       sstat->cpu.all.Stime + sstat->cpu.all.steal;

        busy   = (extra.cputot - sstat->cpu.all.itime - sstat->cpu.all.wtime)
                                * 100.0 / extra.cputot;

        if (cpubadness)
                badness = busy * 100 / cpubadness;
        else
                badness = 0;

        if (highbadness < badness)
        {
                highbadness = badness;
                *highorderp = MSORTCPU;
        }

        if (extra.cputot == 0)
                extra.cputot = 1;             /* avoid divide-by-zero */

        extra.percputot = extra.cputot / sstat->cpu.nrcpu;

        if (extra.percputot == 0)
                extra.percputot = 1;          /* avoid divide-by-zero */

	if (screen)
 	       move(curline, 0);

        showsysline(allcpuline, sstat, &extra, "CPU", badness);
        curline++;

        if (sstat->cpu.nrcpu > 1)
        {
                for (extra.index=lin=0;
		     extra.index < sstat->cpu.nrcpu && lin < maxcpulines;
   		     extra.index++)
                {
                        extra.percputot =  sstat->cpu.cpu[extra.index].stime +
                                     sstat->cpu.cpu[extra.index].utime +
                                     sstat->cpu.cpu[extra.index].ntime +
                                     sstat->cpu.cpu[extra.index].itime +
                                     sstat->cpu.cpu[extra.index].wtime +
                                     sstat->cpu.cpu[extra.index].Itime +
                                     sstat->cpu.cpu[extra.index].Stime +
                                     sstat->cpu.cpu[extra.index].steal;

                        if (extra.percputot ==
				(sstat->cpu.cpu[extra.index].itime +
                                 sstat->cpu.cpu[extra.index].wtime  ) &&
                                 !fixedhead                             )
                                continue;       /* inactive cpu */

                        busy   = (extra.percputot -
					sstat->cpu.cpu[extra.index].itime -
                                        sstat->cpu.cpu[extra.index].wtime)
                                                  * 100.0 / extra.percputot;

                        if (cpubadness)
                                badness = busy * 100 / cpubadness;
                        else
                                badness = 0;

                        if (highbadness < badness)
                        {
                                highbadness = badness;
                                *highorderp = MSORTCPU;
                        }

                        if (extra.percputot == 0)
                                extra.percputot = 1; /* avoid divide-by-zero */


                        move(curline, 0);
                        showsysline(indivcpuline, sstat, &extra, "cpu",
								badness);
                        curline++;
                        lin++;
                }
        }

        /*
        ** other CPU-related statistics
        */
	if (screen)
   	     move(curline, 0);

        showsysline(cplline, sstat, &extra, "CPL", 0);
        curline++;

        /*
        ** MEMORY statistics
        */
        busy   = (sstat->mem.physmem - sstat->mem.freemem
                                     - sstat->mem.cachemem
                                     - sstat->mem.buffermem
                                     - sstat->mem.slabreclaim
                                     + sstat->mem.shmem)
                                                * 100.0 / sstat->mem.physmem;

        if (membadness)
                badness = busy * 100 / membadness;
        else
                badness = 0;

        if (highbadness < badness)
        {
                highbadness = badness;
                *highorderp = MSORTMEM;
        }

	if (screen)
	        move(curline, 0);

        showsysline(memline, sstat, &extra, "MEM", badness);
        curline++;

        /*
        ** SWAP statistics
        */
        busy   = (sstat->mem.totswap - sstat->mem.freeswap)
                                * 100.0 / sstat->mem.totswap;

        if (swpbadness)
        {
                badness = busy * 100 / swpbadness;
        }
        else
        {
                badness = 0;
        }

        if (highbadness < badness)
        {
                highbadness = badness;
                *highorderp = MSORTMEM;
        }

	if (screen)
        	move(curline, 0);

        showsysline(swpline, sstat, &extra, "SWP", badness);
        curline++;

        /*
        ** PAGING statistics
        */
        if (fixedhead             ||
            sstat->mem.pgscans    ||
            sstat->mem.pgsteal    ||
            sstat->mem.allocstall ||
            sstat->mem.swins      ||
            sstat->mem.swouts       )
        {
                busy = sstat->mem.swouts / nsecs * pagbadness;

                if (busy > 100)
                        busy = 100;

                if (membadness)
                        badness = busy * 100 / membadness;
                else
                        badness = 0;

                if (highbadness < badness)
                {
                        highbadness = badness;
                        *highorderp = MSORTMEM;
                }

                /*
                ** take care that this line is anyhow colored for
                ** 'almost critical' in case of swapouts > 1 per second
                */
                if (sstat->mem.swouts / nsecs > 0  &&
                    pagbadness && almostcrit && badness < almostcrit)
                        badness = almostcrit;

		if (screen)
                	move(curline, 0);

                showsysline(pagline, sstat, &extra,"PAG", badness);
                curline++;
        }

	/*
 	** Container statistics (if any)
	*/
        for (extra.index=0, lin=0;
	     extra.index < sstat->cfs.nrcontainer && lin < maxcontlines;
             extra.index++)
	{
        	if (fixedhead             		||
	            sstat->cfs.cont[extra.index].system	||
	            sstat->cfs.cont[extra.index].user	||
	            sstat->cfs.cont[extra.index].nice	  )
		{
			if (screen)
                		move(curline, 0);

                        showsysline(contline, sstat, &extra, "CON", 0);
                        curline++;
                        lin++;
		}
	}

        /*
        ** DISK statistics
        */
        extra.mstot = extra.cputot * 1000 / hertz / sstat->cpu.nrcpu; 

	pridisklike(&extra, sstat->dsk.lvm, "LVM", highorderp, maxlvmlines,
			&highbadness, &curline, fixedhead,
			selp->lvmnamesz ? &(selp->lvmregex) : (void *) 0);

	pridisklike(&extra, sstat->dsk.mdd, "MDD", highorderp, maxmddlines,
			&highbadness, &curline, fixedhead, (void *) 0);

	pridisklike(&extra, sstat->dsk.dsk, "DSK", highorderp, maxdsklines,
			&highbadness, &curline, fixedhead,
			selp->dsknamesz ? &(selp->dskregex) : (void *) 0);

        /*
        ** NFS server and client statistics
        */
	for (extra.index=0, lin=0;
	     extra.index < sstat->nfs.nfsmounts.nrmounts && lin < maxnfslines;
								extra.index++)
	{
		int i = extra.index;

		if ( (sstat->nfs.nfsmounts.nfsmnt[i].bytesread     +
		      sstat->nfs.nfsmounts.nfsmnt[i].byteswrite    +
		      sstat->nfs.nfsmounts.nfsmnt[i].bytesdread    +
		      sstat->nfs.nfsmounts.nfsmnt[i].bytesdwrite   +
		      sstat->nfs.nfsmounts.nfsmnt[i].bytestotread  +
		      sstat->nfs.nfsmounts.nfsmnt[i].bytestotwrite +
		      sstat->nfs.nfsmounts.nfsmnt[i].pagesmread    +
		      sstat->nfs.nfsmounts.nfsmnt[i].pagesmwrite    ) ||
		      sstat->nfs.nfsmounts.nfsmnt[i].age < nsecs      ||
		      fixedhead                                         )
		{
			if (screen)
                		move(curline, 0);

                        showsysline(nfsmountline, sstat, &extra, "NFM", 0);
                        curline++;
                        lin++;
		}
	}

        if (sstat->nfs.client.rpccnt || fixedhead )
        {
		if (screen)
                	move(curline, 0);

                showsysline(nfcline, sstat, &extra, "NFC", 0);
                curline++;
        }

        if (sstat->nfs.server.rpccnt || fixedhead )
        {
		if (screen)
                	move(curline, 0);

                showsysline(nfsline, sstat, &extra, "NFS", 0);
                curline++;
        }

        /*
        ** NET statistics
        */
        if (sstat->net.tcp.InSegs             ||
            sstat->net.tcp.OutSegs            ||
            sstat->net.udpv4.InDatagrams      ||
            sstat->net.udpv6.Udp6InDatagrams  ||
            sstat->net.udpv4.OutDatagrams     ||
            sstat->net.udpv6.Udp6OutDatagrams ||
            fixedhead )
        {
		if (screen)
                	move(curline, 0);

                showsysline(nettransportline, sstat, &extra, "NET", 0);
                curline++;
        }

        if (sstat->net.ipv4.InReceives ||
            sstat->net.ipv6.Ip6InReceives ||
            sstat->net.ipv4.OutRequests ||
            sstat->net.ipv6.Ip6OutRequests ||
            fixedhead )
        {
		if (screen)
                	move(curline, 0);

                showsysline(netnetline, sstat, &extra, "NET", 0);
                curline++;
        }

        for (extra.index=0, lin=0;
	     sstat->intf.intf[extra.index].name[0] && lin < maxintlines;
             extra.index++)
        {
                if (sstat->intf.intf[extra.index].rpack ||
                    sstat->intf.intf[extra.index].spack || fixedhead)
                {
			if (selp->itfnamesz && regexec(&(selp->itfregex),
			       sstat->intf.intf[extra.index].name, 0, NULL, 0))
				continue;	// suppress (not selected)

                        /*
                        ** calculate busy-percentage for interface
                        */

                       count_t ival, oval;

                        /*
                        ** convert byte-transfers to bit-transfers     (*    8)
                        ** convert bit-transfers  to kilobit-transfers (/ 1000)
                        ** per second
                        */
                        ival    = sstat->intf.intf[extra.index].rbyte/125/nsecs;
                        oval    = sstat->intf.intf[extra.index].sbyte/125/nsecs;

			/* speed known? */
                        if (sstat->intf.intf[extra.index].speed) 
                        {
                                if (sstat->intf.intf[extra.index].duplex)
                                       busy = (ival > oval ? ival : oval) /
					   (sstat->intf.intf[extra.index].speed
									*10);
                                else
                                       busy = (ival + oval) /
                                           (sstat->intf.intf[extra.index].speed
									*10);

                        }
                        else
                        {
                                busy = 0;
                        }

                        if (netbadness)
                                badness = busy * 100 / netbadness;
                        else
                                badness = 0;

                        if (highbadness < badness && (supportflags & NETATOP) )
                        {
                                highbadness = badness;
                                *highorderp = MSORTNET;
                        }

			if (screen)
                		move(curline, 0);

                        showsysline(netinterfaceline, sstat, &extra, 
                                      			"NET", badness);
                        curline++;
                        lin++;
                }
        }

        /*
        ** application statistics
        **
        ** WWW: notice that we cause one access ourselves by fetching
        **      the statistical counters
        */
#if     HTTPSTATS
        if (sstat->www.accesses > 1 || fixedhead )
        {
		char format1[8], format2[8], format3[8], format4[8], format5[8];

		if (screen)
               		move(curline, 0);

                printg("WWW | reqs  %s | totKB %s | byt/rq %s | iwork %s |"
                       " bwork %s |",
                        val2valstr(sstat->www.accesses,  format1, 6,
                                                         avgval, nsecs),
                        val2valstr(sstat->www.totkbytes, format2, 6,
                                                         avgval, nsecs),
                        val2valstr(sstat->www.accesses ?
                            sstat->www.totkbytes*1024/sstat->www.accesses : 0,
                                                         format3, 5, 0, 0),
                        val2valstr(sstat->www.iworkers,  format4, 6, 0, 0),
                        val2valstr(sstat->www.bworkers,  format5, 6, 0, 0) );
                if (!screen) 
                {
                        printg("\n");
                }
                curline++;
        }
#endif

        /*
        ** if the system is hardly loaded, still CPU-ordering of
        ** processes is most interesting (instead of memory)
        */
        if (highbadness < 70 && *highorderp == MSORTMEM)
                *highorderp = MSORTCPU;

        return curline;
}

/*
** handle all instances of a specific disk-like device
*/
static void
pridisklike(extraparam *ep, struct perdsk *dp, char *lp, char *highorderp,
	int maxlines, unsigned int *highbadp, int *curlinp, int fixedhead,
	regex_t *rep)
{
	int 		lin;
        count_t         busy;
        unsigned int    badness;

        for (ep->perdsk = dp, ep->index=0, lin=0;
	     ep->perdsk[ep->index].name[0] && lin < maxlines; ep->index++)
        {
		if (rep && regexec(rep, ep->perdsk[ep->index].name, 0, NULL, 0))
			continue;	// suppress (not selected)

                ep->iotot =  ep->perdsk[ep->index].nread +
		             ep->perdsk[ep->index].nwrite;

                busy        = (double)(ep->perdsk[ep->index].io_ms *
						100.0 / ep->mstot);

                if (dskbadness)
                        badness = busy * 100 / dskbadness;
                else
                        badness = 0;

                if (*highbadp < badness && (supportflags & IOSTAT) )
                {
                        *highbadp	= badness;
                        *highorderp 	= MSORTDSK;
                }

                if (ep->iotot || fixedhead)
                {
                        move(*curlinp, 0);
                        showsysline(dskline, 0, ep, lp, badness);
                        (*curlinp)++;
                        lin++;
                }
        }
}


/*
** sort-functions
*/
int
compcpu(const void *a, const void *b)
{
        register count_t acpu = (*(struct tstat **)a)->cpu.stime +
                                (*(struct tstat **)a)->cpu.utime;
        register count_t bcpu = (*(struct tstat **)b)->cpu.stime +
                                (*(struct tstat **)b)->cpu.utime;

        if (acpu < bcpu) return  1;
        if (acpu > bcpu) return -1;
                         return compmem(a, b);
}

int
compdsk(const void *a, const void *b)
{
	struct tstat	*ta = *(struct tstat **)a;
	struct tstat	*tb = *(struct tstat **)b;

        count_t	adsk;
        count_t bdsk;

	if (ta->dsk.wsz > ta->dsk.cwsz)
		adsk = ta->dsk.rio + ta->dsk.wsz - ta->dsk.cwsz;
	else
		adsk = ta->dsk.rio;

	if (tb->dsk.wsz > tb->dsk.cwsz)
		bdsk = tb->dsk.rio + tb->dsk.wsz - tb->dsk.cwsz;
	else
		bdsk = tb->dsk.rio;

        if (adsk < bdsk) return  1;
        if (adsk > bdsk) return -1;
                         return compcpu(a, b);
}

int
compmem(const void *a, const void *b)
{
        register count_t amem = (*(struct tstat **)a)->mem.rmem;
        register count_t bmem = (*(struct tstat **)b)->mem.rmem;

        if (amem < bmem) return  1;
        if (amem > bmem) return -1;
                         return  0;
}

int
compnet(const void *a, const void *b)
{
        register count_t anet = (*(struct tstat **)a)->net.tcpssz +
                                (*(struct tstat **)a)->net.tcprsz +
                                (*(struct tstat **)a)->net.udpssz +
                                (*(struct tstat **)a)->net.udprsz  ;
        register count_t bnet = (*(struct tstat **)b)->net.tcpssz +
                                (*(struct tstat **)b)->net.tcprsz +
                                (*(struct tstat **)b)->net.udpssz +
                                (*(struct tstat **)b)->net.udprsz  ;

        if (anet < bnet) return  1;
        if (anet > bnet) return -1;
                         return compcpu(a, b);
}

int
compusr(const void *a, const void *b)
{
        register int uida = (*(struct tstat **)a)->gen.ruid;
        register int uidb = (*(struct tstat **)b)->gen.ruid;

        if (uida > uidb) return  1;
        if (uida < uidb) return -1;
                         return  0;
}

int
compnam(const void *a, const void *b)
{
        register char *nama = (*(struct tstat **)a)->gen.name;
        register char *namb = (*(struct tstat **)b)->gen.name;

        return strcmp(nama, namb);
}

int
compcon(const void *a, const void *b)
{
        register char *containera = (*(struct tstat **)a)->gen.container;
        register char *containerb = (*(struct tstat **)b)->gen.container;

       return strcmp(containera, containerb);
}

int
cpucompar(const void *a, const void *b)
{
        register count_t aidle = ((struct percpu *)a)->itime +
                                 ((struct percpu *)a)->wtime;
        register count_t bidle = ((struct percpu *)b)->itime +
                                 ((struct percpu *)b)->wtime;

        if (aidle < bidle) return -1;
        if (aidle > bidle) return  1;
                           return  0;
}

int
diskcompar(const void *a, const void *b)
{
        register count_t amsio = ((struct perdsk *)a)->io_ms;
        register count_t bmsio = ((struct perdsk *)b)->io_ms;

        if (amsio < bmsio) return  1;
        if (amsio > bmsio) return -1;
                           return  0;
}

int
intfcompar(const void *a, const void *b)
{
        register count_t afactor=0, bfactor=0;

        count_t aspeed         = ((struct perintf *)a)->speed;
        count_t bspeed         = ((struct perintf *)b)->speed;
        char    aduplex        = ((struct perintf *)a)->duplex;
        char    bduplex        = ((struct perintf *)b)->duplex;
        count_t arbyte         = ((struct perintf *)a)->rbyte;
        count_t brbyte         = ((struct perintf *)b)->rbyte;
        count_t asbyte         = ((struct perintf *)a)->sbyte;
        count_t bsbyte         = ((struct perintf *)b)->sbyte;


        /*
        ** if speed of first interface known, calculate busy factor
        */
        if (aspeed)
        {
                if (aduplex)
                        afactor = (arbyte > asbyte ? arbyte : asbyte) 
                                                                * 10 / aspeed;
                else
                        afactor = (arbyte + asbyte)             * 10 / aspeed;
        }

        /*
        ** if speed of second interface known, calculate busy factor
        */
        if (bspeed)
        {
                if (bduplex)
                        bfactor = (brbyte > bsbyte ? brbyte : bsbyte)
                                                                * 10 / bspeed;
                else
                        bfactor = (brbyte + bsbyte)             * 10 / bspeed;
        }

        /*
        ** compare interfaces
        */
        if (aspeed && bspeed)
        {
                if (afactor < bfactor)  return  1;
                if (afactor > bfactor)  return -1;
                                        return  0;
        }

        if (!aspeed && !bspeed)
        {
                if ((arbyte + asbyte) < (brbyte + bsbyte))      return  1;
                if ((arbyte + asbyte) > (brbyte + bsbyte))      return -1;
                                                                return  0;
        }

        if (aspeed)
                return -1;
        else
                return  1;
}

int
nfsmcompar(const void *a, const void *b)
{
        const struct pernfsmount *na = a;
        const struct pernfsmount *nb = b;

        register count_t aused = na->bytesread    + na->byteswrite    + 
	                         na->bytesdread   + na->bytesdwrite   +
	                         na->bytestotread + na->bytestotwrite +
                                 na->pagesmread   + na->pagesmwrite;
        register count_t bused = nb->bytesread    + nb->byteswrite    + 
	                         nb->bytesdread   + nb->bytesdwrite   +
	                         nb->bytestotread + nb->bytestotwrite +
                                 nb->pagesmread   + nb->pagesmwrite;

        if (aused < bused) return  1;
        if (aused > bused) return -1;
                           return  0;
}

int
contcompar(const void *a, const void *b)
{
        const struct percontainer *ca = a;
        const struct percontainer *cb = b;

        register count_t aused = ca->system + ca->user + ca->nice;
        register count_t bused = cb->system + cb->user + cb->nice;

        if (aused < bused) return  1;
        if (aused > bused) return -1;
                           return  0;
}

/*
** handle modifications from the /etc/atoprc and ~/.atoprc file
*/
int
get_posval(char *name, char *val)
{
        int     value = atoi(val);

        if ( !numeric(val))
        {
                fprintf(stderr, "atoprc: %s value %s not a (positive) numeric\n", 
                name, val);
                exit(1);
        }

        if (value < 0)
        {
                fprintf(stderr,
                        "atoprc: %s value %d not positive\n", 
                        name, value);
                exit(1);
        }

        return value;
}

int
get_perc(char *name, char *val)
{
        int     value = get_posval(name, val);

        if (value < 0 || value > 100)
        {
                fprintf(stderr, "atoprc: %s value %d not in range 0-100\n", 
                        name, value);
                exit(1);
        }

        return value;
}

void
do_cpucritperc(char *name, char *val)
{
        cpubadness = get_perc(name, val);
}

void
do_memcritperc(char *name, char *val)
{
        membadness = get_perc(name, val);
}

void
do_swpcritperc(char *name, char *val)
{
        swpbadness = get_perc(name, val);
}

void
do_dskcritperc(char *name, char *val)
{
        dskbadness = get_perc(name, val);
}

void
do_netcritperc(char *name, char *val)
{
        netbadness = get_perc(name, val);
}

void
do_swoutcritsec(char *name, char *val)
{
        pagbadness = get_posval(name, val);
}

void
do_almostcrit(char *name, char *val)
{
        almostcrit = get_perc(name, val);
}

void
do_ownsysprcline(char *name, char *val)
{
        make_sys_prints(sysprcline, MAXITEMS, val, prcsyspdefs, name);
}

void
do_ownallcpuline(char *name, char *val)
{
        make_sys_prints(allcpuline, MAXITEMS, val, cpusyspdefs, name);
}

void
do_ownindivcpuline(char *name, char *val)
{
        make_sys_prints(indivcpuline, MAXITEMS, val, cpisyspdefs, name);
}

void
do_owncplline(char *name, char *val)
{
        make_sys_prints(cplline, MAXITEMS, val, cplsyspdefs, name);
}

void
do_ownmemline(char *name, char *val)
{
        make_sys_prints(memline, MAXITEMS, val, memsyspdefs, name);
}

void
do_ownswpline(char *name, char *val)
{
        make_sys_prints(swpline, MAXITEMS, val, swpsyspdefs, name);
}

void
do_ownpagline(char *name, char *val)
{
        make_sys_prints(pagline, MAXITEMS, val, pagsyspdefs, name);
}

void
do_owndskline(char *name, char *val)
{
        make_sys_prints(dskline, MAXITEMS, val, dsksyspdefs, name);
}

void
do_ownnettransportline(char *name, char *val)
{
        make_sys_prints(nettransportline, MAXITEMS, val, nettranssyspdefs, name);
}

void
do_ownnetnetline(char *name, char *val)
{
        make_sys_prints(netnetline, MAXITEMS, val, netnetsyspdefs, name);
}

void
do_ownnetinterfaceline(char *name, char *val)
{
        make_sys_prints(netinterfaceline, MAXITEMS, val, netintfsyspdefs, name);
}

void
do_ownprocline(char *name, char *val)
{
	make_proc_prints(ownprocs, MAXITEMS, val, name);
}
