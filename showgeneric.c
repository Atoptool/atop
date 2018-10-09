/*
** ATOP - System & Process Monitor 
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
** 
** This source-file contains the print-functions to visualize the calculated
** figures.
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
** $Log: showgeneric.c,v $
** Revision 1.71  2010/10/25 19:08:32  gerlof
** When the number of lines is too small for the system-level
** lines, limit the number of variable resources automatically
** to a minimum.
**
** Revision 1.70  2010/10/23 14:04:05  gerlof
** Counters for total number of running and sleep threads (JC van Winkel).
**
** Revision 1.69  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.68  2010/04/23 09:58:11  gerlof
** Version (flag -V) handled earlier after startup.
**
** Revision 1.67  2010/04/23 07:57:32  gerlof
** Proper sorting of processes when switching from single process view
** to cumulative view (key 'u' or 'p') and vice versa.
**
** Revision 1.66  2010/04/17 17:20:26  gerlof
** Allow modifying the layout of the columns in the system lines.
**
** Revision 1.65  2010/03/16 21:13:38  gerlof
** Program and user selection can be combined with program and user
** accumulation.
**
** Revision 1.64  2010/03/16 20:18:46  gerlof
** Show in header-line if user selections and program selection are active.
**
** Revision 1.63  2010/03/16 20:08:51  gerlof
** Performance improvement: only sort system-resources once per interval.
**
** Revision 1.62  2010/03/04 10:53:01  gerlof
** Support I/O-statistics on logical volumes and MD devices.
**
** Revision 1.61  2009/12/17 10:55:07  gerlof
** *** empty log message ***
**
** Revision 1.60  2009/12/17 10:50:30  gerlof
** Allow own defined process line with key 'o' and a definition
** in the atoprc file.
**
** Revision 1.59  2009/12/17 09:03:26  gerlof
** Center message "....since boot" in status line on first screen.
**
** Revision 1.58  2009/12/17 08:55:15  gerlof
** Show messages on status line in color to draw attention.
**
** Revision 1.57  2009/12/17 08:16:14  gerlof
** Introduce branch-key to go to specific time in raw file.
**
** Revision 1.56  2009/12/12 09:06:39  gerlof
** Corrected cumulated disk I/O per user/program (JC van Winkel).
**
** Revision 1.55  2009/12/10 13:34:44  gerlof
** Show which toggle-keys are active in the header line.
**
** Revision 1.54  2009/12/10 11:55:03  gerlof
** Introduce system-wide /etc/atoprc
**
** Revision 1.53  2009/12/10 09:53:08  gerlof
** Improved display of header-line (JC van Winkel).
**
** Revision 1.52  2008/03/06 10:14:01  gerlof
** Modified help-messages.
**
** Revision 1.51  2008/02/25 13:47:21  gerlof
** Bug-solution: segmentation-fault in case of invalid regular expression.
**
** Revision 1.50  2008/01/07 11:33:58  gerlof
** Cosmetic changes.
**
** Revision 1.49  2008/01/07 10:18:24  gerlof
** Implement possibility to make summaries with atopsar.
**
** Revision 1.48  2007/03/22 10:12:17  gerlof
** Support for io counters (>= kernel 2.6.20).
**
** Revision 1.47  2007/03/21 14:22:34  gerlof
** Handle io counters maintained from 2.6.20
**
** Revision 1.46  2007/03/20 11:13:15  gerlof
** Cosmetic changes.
**
** Revision 1.45  2007/03/09 12:39:59  gerlof
** Do not allow 'N' and 'D' when kernel-patch is not installed.
**
** Revision 1.44  2007/02/13 10:31:53  gerlof
** Removal of external declarations.
**
** Revision 1.43  2007/01/18 10:41:45  gerlof
** Add support for colors.
** Add support for automatic determination of most critical resource.
**
** Revision 1.42  2006/11/13 13:48:36  gerlof
** Implement load-average counters, context-switches and interrupts.
**
** Revision 1.41  2006/04/03 05:42:35  gerlof
** *** empty log message ***
**
** Revision 1.40  2006/02/07 08:28:26  gerlof
** Improve screen-handling (less flashing) by exchanging clear()
** by werase() (contribution Folkert van Heusden).
**
** Revision 1.39  2005/11/04 14:16:16  gerlof
** Minor bug-solutions.
**
** Revision 1.38  2005/10/28 09:52:03  gerlof
** All flags/subcommands are defined as macro's.
** Subcommand 'p' has been changed to 'z' (pause).
**
** Revision 1.37  2005/10/24 06:12:17  gerlof
** Flag -L modified into -l.
**
** Revision 1.36  2005/10/21 09:50:46  gerlof
** Per-user accumulation of resource consumption.
** Possibility to send signal to process.
**
** Revision 1.35  2004/12/14 15:06:41  gerlof
** Implementation of patch-recognition for disk and network-statistics.
**
** Revision 1.34  2004/09/27 11:01:13  gerlof
** Corrected usage-info as suggested by Edelhard Becker.
**
** Revision 1.33  2004/09/13 09:19:14  gerlof
** Modify subcommands (former 's' -> 'v', 'v' -> 'V', new 's').
**
** Revision 1.32  2004/08/31 09:52:47  root
** information about underlying threads.
**
** Revision 1.31  2004/06/01 11:57:58  gerlof
** Regular expressions for selections on process-name and user-name.
**
** Revision 1.30  2003/07/07 09:27:24  gerlof
** Cleanup code (-Wall proof).
**
** Revision 1.29  2003/07/03 11:16:42  gerlof
** Implemented subcommand `r' (reset).
**
** Revision 1.28  2003/06/30 11:29:49  gerlof
** Handle configuration file ~/.atoprc
**
** Revision 1.27  2003/06/24 06:21:57  gerlof
** Limit number of system resource lines.
**
** Revision 1.26  2003/02/07 10:19:18  gerlof
** Possibility to show the version number and date.
**
** Revision 1.25  2003/01/17 07:32:16  gerlof
** Show the full command-line per process (option 'c').
**
** Revision 1.24  2002/10/30 13:47:20  gerlof
** Generate notification for statistics since boot.
**
** Revision 1.23  2002/10/08 12:00:30  gerlof
** *** empty log message ***
**
** Revision 1.22  2002/09/26 14:17:39  gerlof
** No beep when resizing the window.
**
** Revision 1.21  2002/09/26 13:52:26  gerlof
** Limit header lines by not showing disks.
**
** Revision 1.20  2002/09/18 07:15:59  gerlof
** Modified viewflag to rawreadflag.
**
** Revision 1.19  2002/09/17 13:17:39  gerlof
** Allow key 'T' to be pressed to view previous sample in raw file.
**
** Revision 1.18  2002/08/30 07:11:50  gerlof
** Minor changes to support viewing of raw atop data.
**
** Revision 1.17  2002/08/27 12:10:12  gerlof
** Allow raw data file to be written and to be read (with compression).
**
** Revision 1.16  2002/07/24 11:12:46  gerlof
** Changed to ease porting to other UNIX-platforms.
**
** Revision 1.15  2002/07/11 09:12:05  root
** Some minor updates.
**
** Revision 1.14  2002/07/10 05:00:37  root
** Counters pin/pout renamed to swin/swout (Linux conventions).
**
** Revision 1.13  2002/07/08 09:29:49  root
** Limitation for username and groupname (8 characters truncate).
**
** Revision 1.12  2002/07/02 07:14:02  gerlof
** More positions for the name of the disk-unit in the DSK-line.
**
** Revision 1.11  2002/01/22 13:40:42  gerlof
** Support for number of cpu's.
** Check if the window is large enough for the system-statistics.
**
** Revision 1.10  2001/11/30 09:09:36  gerlof
** Cosmetic chnage.
**
** Revision 1.9  2001/11/29 10:41:44  gerlof
** *** empty log message ***
**
** Revision 1.8  2001/11/29 10:38:16  gerlof
** Exit-code correctly printed.
**
** Revision 1.7  2001/11/26 11:18:45  gerlof
** Modified generic output in case that the kernel-patch is not installed.
**
** Revision 1.6  2001/11/13 08:24:50  gerlof
** Show blank columns for sockets and disk I/O when no kernel-patch installed.
**
** Revision 1.5  2001/11/07 09:19:28  gerlof
** Use /proc instead of /dev/kmem for process-level statistics.
**
** Revision 1.4  2001/10/05 13:46:32  gerlof
** Implemented paging through the process-list
**
** Revision 1.3  2001/10/04 08:47:27  gerlof
** Improved handling of error-messages
**
** Revision 1.2  2001/10/03 08:57:53  gerlof
** Improved help-screen shown in scrollable window
**
** Revision 1.1  2001/10/02 10:43:34  gerlof
** Initial revision
**
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
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

static struct pselection procsel = {"", {USERSTUB, }, {0,},
                                    "", 0, { 0, },  "", 0, { 0, } };
static struct sselection syssel;

static void	showhelp(int);
static int	paused;     	/* boolean: currently in pause-mode     */
static int	fixedhead;	/* boolean: fixate header-lines         */
static int	sysnosort;	/* boolean: suppress sort of resources  */
static int	avgval;		/* boolean: average values i.s.o. total */
static int	suppressexit;	/* boolean: suppress exited processes   */

static char	showtype  = MPROCGEN;
static char	showorder = MSORTCPU;

static int	maxcpulines = 999;  /* maximum cpu       lines          */
static int	maxdsklines = 999;  /* maximum disk      lines          */
static int	maxmddlines = 999;  /* maximum MDD       lines          */
static int	maxlvmlines = 999;  /* maximum LVM       lines          */
static int	maxintlines = 999;  /* maximum interface lines          */
static int	maxnfslines = 999;  /* maximum nfs mount lines          */
static int	maxcontlines = 999; /* maximum container lines          */

static short	colorinfo   = COLOR_GREEN;
static short	coloralmost = COLOR_CYAN;
static short	colorcrit   = COLOR_RED;
static short	colorthread = COLOR_YELLOW;

static int	cumusers(struct tstat **, struct tstat *, int);
static int	cumprogs(struct tstat **, struct tstat *, int);
static int	cumconts(struct tstat **, struct tstat *, int);
static void	accumulate(struct tstat *, struct tstat *);

static int	procsuppress(struct tstat *, struct pselection *);
static void	limitedlines(void);
static long	getnumval(char *, long, int);
static void	generic_init(void);


static int	(*procsort[])(const void *, const void *) = {
			[MSORTCPU&0x1f]=compcpu, 
			[MSORTMEM&0x1f]=compmem, 
			[MSORTDSK&0x1f]=compdsk, 
			[MSORTNET&0x1f]=compnet, 
};

extern proc_printpair ownprocs[];

/*
** global: incremented by -> key and decremented by <- key
*/
int	startoffset;

/*
** print the deviation-counters on process- and system-level
*/
char
generic_samp(time_t curtime, int nsecs,
           struct devtstat *devtstat, struct sstat *sstat, 
           int nexit, unsigned int noverflow, char flag)
{
	static int	callnr = 0;
	char		*p;


	register int	i, curline, statline, nproc;
	int		firstproc = 0, plistsz, alistsz, killpid, killsig;
	int		lastchar;
	char		format1[16], format2[16], hhmm[16];
	char		*statmsg = NULL, statbuf[80], genline[80];
	char		 *lastsortp, curorder, autoorder;
	char		buf[33];
	struct passwd 	*pwd;
	struct syscap	syscap;

	/*
	** curlist points to the active list of tstat-pointers that
	** should be displayed; ncurlist indicates the number of entries in
	** this list
	*/
	struct tstat	**curlist;
	int		ncurlist;

	/*
	** tXcumlist is a list of tstat-structs holding one entry
	** per accumulated (per user or per program) group of processes
	**
	** Xcumlist contains the pointers to all structs in tXcumlist
	** 
	** these lists will only be allocated 'lazy'
	** only when accumulation is requested
	*/
	struct tstat	*tpcumlist = 0;		// per program accumulation
	struct tstat	**pcumlist = 0;
	int		npcum      = 0;
	char		plastorder = 0;

	struct tstat	*tucumlist = 0;		// per user accumulation
	struct tstat	**ucumlist = 0;
	int		nucum      = 0;
	char		ulastorder = 0;

	struct tstat	*tccumlist = 0;		// per container accumulation
	struct tstat	**ccumlist = 0;
	int		nccum      = 0;
	char		clastorder = 0;

	/*
	** tsklist contains the pointers to all structs in tstat
	** sorted on process with the related threads immediately
	** following the process
	**
	** this list will be allocated 'lazy'
	*/
	struct tstat	**tsklist  = 0;
	int		ntsk       = 0;
	char		tlastorder = 0;
	char		zipagain   = 0;
	char		tdeviate   = 0;

	/*
	** sellist contains the pointers to the structs in tstat
	** that are currently selected on basis of a particular
	** username (regexp), program name (regexp), container name
	** or suppressed exited procs
	**
	** this list will be allocated 'lazy'
	*/
	struct tstat	**sellist  = 0;
	int		nsel       = 0;
	char		slastorder = 0;

	char		threadallowed = 0;


	if (callnr == 0)	/* first call? */
		generic_init();

	callnr++;

	startoffset = 0;

	/*
	** compute the total capacity of this system for the 
	** four main resources
	*/
	totalcap(&syscap, sstat, devtstat->procactive, devtstat->nprocactive);

	/*
	** sort per-cpu       		statistics on busy percentage
	** sort per-logical-volume      statistics on busy percentage
	** sort per-multiple-device     statistics on busy percentage
	** sort per-disk      		statistics on busy percentage
	** sort per-interface 		statistics on busy percentage (if known)
	*/
	if (!sysnosort)
	{
		if (sstat->cpu.nrcpu > 1 && maxcpulines > 0)
			qsort(sstat->cpu.cpu, sstat->cpu.nrcpu,
	 	               sizeof sstat->cpu.cpu[0], cpucompar);

		if (sstat->dsk.nlvm > 1 && maxlvmlines > 0)
			qsort(sstat->dsk.lvm, sstat->dsk.nlvm,
			       sizeof sstat->dsk.lvm[0], diskcompar);

		if (sstat->dsk.nmdd > 1 && maxmddlines > 0)
			qsort(sstat->dsk.mdd, sstat->dsk.nmdd,
			       sizeof sstat->dsk.mdd[0], diskcompar);

		if (sstat->dsk.ndsk > 1 && maxdsklines > 0)
			qsort(sstat->dsk.dsk, sstat->dsk.ndsk,
			       sizeof sstat->dsk.dsk[0], diskcompar);

		if (sstat->intf.nrintf > 1 && maxintlines > 0)
			qsort(sstat->intf.intf, sstat->intf.nrintf,
		  	       sizeof sstat->intf.intf[0], intfcompar);

		if (sstat->nfs.nfsmounts.nrmounts > 1 && maxnfslines > 0)
			qsort(sstat->nfs.nfsmounts.nfsmnt,
		              sstat->nfs.nfsmounts.nrmounts,
		  	      sizeof sstat->nfs.nfsmounts.nfsmnt[0],
				nfsmcompar);

		if (sstat->cfs.nrcontainer > 1 && maxcontlines > 0)
			qsort(sstat->cfs.cont, sstat->cfs.nrcontainer,
		  	       sizeof sstat->cfs.cont[0], contcompar);
	}

	/*
	** loop in which the system resources and the list of active
	** processes are shown; the loop will be preempted by receiving
	** a timer-signal or when the trigger-button is pressed.
	*/
	while (1)
	{
		curline = 1;

	        /*
       	 	** prepare screen or file output for new sample
        	*/
        	if (screen)
               	 	werase(stdscr);
        	else
                	printf("\n\n");

        	/*
        	** print general headerlines
        	*/
        	convdate(curtime, format1);       /* date to ascii string   */
        	convtime(curtime, format2);       /* time to ascii string   */

		if (screen)
			attron(A_REVERSE);

                int seclen	= val2elapstr(nsecs, buf);
                int lenavail 	= (screen ? COLS : linelen) -
						49 - seclen - utsnodenamelen;
                int len1	= lenavail / 3;
                int len2	= lenavail - len1 - len1; 

		printg("ATOP - %s%*s%s  %s%*s%c%c%c%c%c%c%c%c%c%c%c%c%c%c%*s%s"
		       " elapsed", 
			utsname.nodename, len1, "", 
			format1, format2, len1, "",
			threadview                    ? MTHREAD    : '-',
			fixedhead  		      ? MSYSFIXED  : '-',
			sysnosort  		      ? MSYSNOSORT : '-',
			deviatonly 		      ? '-'        : MALLPROC,
			usecolors  		      ? '-'        : MCOLORS,
			avgval     		      ? MAVGVAL    : '-',
			calcpss     		      ? MCALCPSS   : '-',
			suppressexit 		      ? MSUPEXITS  : '-',
			procsel.userid[0] != USERSTUB ? MSELUSER   : '-',
			procsel.prognamesz	      ? MSELPROC   : '-',
			procsel.container[0]	      ? MSELCONT   : '-',
			procsel.pid[0] != 0	      ? MSELPID    : '-',
			procsel.argnamesz	      ? MSELARG    : '-',
			syssel.lvmnamesz +
			syssel.dsknamesz +
			syssel.itfnamesz	      ? MSELSYS    : '-',
			len2, "", buf);

		if (screen)
			attroff(A_REVERSE);
                else
                        printg("\n");

		/*
		** print cumulative system- and user-time for all processes
		*/
		pricumproc(sstat, devtstat, nexit, noverflow, avgval, nsecs);

		if (noverflow)
		{
			snprintf(statbuf, sizeof statbuf, 
			         "Only %d exited processes handled "
			         "-- %u skipped!", nexit, noverflow);

			statmsg = statbuf;
		}

		curline=2;

		/*
		** print other lines of system-wide statistics
		*/
		if (showorder == MSORTAUTO)
			autoorder = MSORTCPU;
		else
			autoorder = showorder;

		curline = prisyst(sstat, curline, nsecs, avgval,
		                  fixedhead, &syssel, &autoorder,
		                  maxcpulines, maxdsklines, maxmddlines,
		                  maxlvmlines, maxintlines, maxnfslines,
		                  maxcontlines);

		/*
 		** if system-wide statistics do not fit,
		** limit the number of variable resource lines
		** and try again
		*/
		if (screen && curline+2 > LINES)
		{
			curline = 2;

			move(curline, 0);
			clrtobot();
			move(curline, 0);

			limitedlines();
			
			curline = prisyst(sstat, curline, nsecs, avgval,
					fixedhead,  &syssel, &autoorder,
					maxcpulines, maxdsklines, maxmddlines,
		                        maxlvmlines, maxintlines, maxnfslines,
		                        maxcontlines);

			/*
 			** if system-wide statistics still do not fit,
			** the window is really to small
			*/
			if (curline+2 > LINES)
			{
				endwin();	// finish curses interface

				fprintf(stderr,
				      "Not enough screen-lines available "
				      "(need at least %d lines)\n", curline+2);
				fprintf(stderr, "Please resize window....\n");

				cleanstop(1);
			}
			else
			{
				statmsg = "Number of variable resources"
				          " limited to fit in this window";
			}
		}

		statline = curline;

		if (screen)
        	        move(curline, 0);

		if (statmsg)
		{
			if (screen)
			{
				clrtoeol();
				if (usecolors)
					attron(COLOR_PAIR(COLORINFO));
			}

			printg(statmsg);

			if (screen)
			{
				if (usecolors)
					attroff(COLOR_PAIR(COLORINFO));
			}

			statmsg = NULL;
		}
		else
		{
			if (flag&RRBOOT)
			{
				if (screen)
				{
					if (usecolors)
						attron(COLOR_PAIR(COLORINFO));

					attron(A_BLINK);

					printg("%*s", (COLS-45)/2, " ");
				}
				else
				{
					printg("                   ");
				}

       				printg("*** system and process activity "
				       "since boot ***");

				if (screen)
				{
					if (usecolors)
						attroff(COLOR_PAIR(COLORINFO));
					attroff(A_BLINK);
				}
			}
		}

		/*
		** select the required list with tasks to be shown
		**
		** if cumulative figures required, accumulate resource
		** consumption of all processes in the current list
		*/
		switch (showtype)
		{
		   case MCUMUSER:
			threadallowed = 0;

			if (ucumlist)	/* previous list still available? */
			{
                                free(ucumlist);
                                free(tucumlist);
				ulastorder = 0;
			}

			if (deviatonly)
				nproc = devtstat->nprocactive;
			else
				nproc = devtstat->nprocall;

			/*
			** allocate space for new (temporary) list with
			** one entry per user (list has worst-case size)
			*/
			tucumlist = calloc(sizeof(struct tstat),    nproc);
			ucumlist  = malloc(sizeof(struct tstat *) * nproc);

			ptrverify(tucumlist,
			        "Malloc failed for %d ucum procs\n", nproc);
			ptrverify(ucumlist,
			        "Malloc failed for %d ucum ptrs\n",  nproc);

			for (i=0; i < nproc; i++)
			{
				/* fill pointers */
				ucumlist[i] = tucumlist+i;
			}

			nucum = cumusers(deviatonly ?
						devtstat->procactive :
						devtstat->procall,
						tucumlist, nproc);

			curlist   = ucumlist;
			ncurlist  = nucum;
			lastsortp = &ulastorder;
			break;

		   case MCUMPROC:
			threadallowed = 0;

			if (pcumlist)	/* previous list still available? */
			{
                                free(pcumlist);
                                free(tpcumlist);
				plastorder = 0;
			}

			if (deviatonly)
				nproc = devtstat->nprocactive;
			else
				nproc = devtstat->nprocall;

			/*
			** allocate space for new (temporary) list with
			** one entry per program (list has worst-case size)
			*/
			tpcumlist = calloc(sizeof(struct tstat),    nproc);
			pcumlist  = malloc(sizeof(struct tstat *) * nproc);

			ptrverify(tpcumlist,
			        "Malloc failed for %d pcum procs\n", nproc);
			ptrverify(pcumlist,
			        "Malloc failed for %d pcum ptrs\n",  nproc);

			for (i=0; i < nproc; i++)
			{	
				/* fill pointers */
				pcumlist[i] = tpcumlist+i;
			}

			npcum = cumprogs(deviatonly ?
						devtstat->procactive :
						devtstat->procall,
						tpcumlist, nproc);

			curlist   = pcumlist;
			ncurlist  = npcum;
			lastsortp = &plastorder;
			break;

		   case MCUMCONT:
			threadallowed = 0;

			if (ccumlist)	/* previous list still available? */
			{
                                free(ccumlist);
                                free(tccumlist);
				clastorder = 0;
			}

			if (deviatonly)
				nproc = devtstat->nprocactive;
			else
				nproc = devtstat->nprocall;

			/*
			** allocate space for new (temporary) list with
			** one entry per user (list has worst-case size)
			*/
			tccumlist = calloc(sizeof(struct tstat),    nproc);
			ccumlist  = malloc(sizeof(struct tstat *) * nproc);

			ptrverify(tccumlist,
			        "Malloc failed for %d ccum procs\n", nproc);
			ptrverify(ccumlist,
			        "Malloc failed for %d ccum ptrs\n",  nproc);

			for (i=0; i < nproc; i++)
			{
				/* fill pointers */
				ccumlist[i] = tccumlist+i;
			}

			nccum = cumconts(deviatonly ?
						devtstat->procactive :
						devtstat->procall,
						tccumlist, nproc);

			curlist   = ccumlist;
			ncurlist  = nccum;
			lastsortp = &clastorder;
			break;

		   default:
			threadallowed = 1;

			if (deviatonly && showtype  != MPROCMEM &&
			                  showorder != MSORTMEM   )
			{
				curlist   = devtstat->procactive;
				ncurlist  = devtstat->nprocactive;
			}
			else
			{
				curlist   = devtstat->procall;
				ncurlist  = devtstat->nprocall;
			}

			lastsortp = &tlastorder;

			if ( procsel.userid[0] == USERSTUB &&
			    !procsel.prognamesz            &&
			    !procsel.container[0]          &&
			    !procsel.argnamesz             &&
			    !procsel.pid[0]                &&
			    !suppressexit                    )
				/* no selection wanted */
				break;

			/*
			** selection specified for tasks:
			** create new (worst case) pointer list if needed
			*/
			if (sellist)	// remove previous list if needed
				free(sellist);

			sellist = malloc(sizeof(struct tstat *) * ncurlist);

			ptrverify(sellist,
			       "Malloc failed for %d select ptrs\n", ncurlist);

			for (i=nsel=0; i < ncurlist; i++)
			{
				if (procsuppress(*(curlist+i), &procsel))
					continue;

				if (curlist[i]->gen.state == 'E' &&
				    suppressexit                   )
					continue;

				sellist[nsel++] = curlist[i]; 
			}

			curlist    = sellist;
			ncurlist   = nsel;
			tlastorder = 0; /* new sort and zip normal view */
			slastorder = 0;	/* new sort and zip now         */
			lastsortp  = &slastorder;
		}

		/*
		** sort the list in required order 
		** (default CPU-consumption) and print the list
		*/
		if (showorder == MSORTAUTO)
			curorder = autoorder;
		else
			curorder = showorder;

		/*
 		** determine size of list to be displayed
		*/
		if (screen)
			plistsz = LINES-curline-2;
		else
			if (threadview && threadallowed)
				plistsz = devtstat->ntaskactive;
			else
				plistsz = ncurlist;


		if (ncurlist > 0 && plistsz > 0)
		{
			/*
 			** if sorting order is changed, sort again
 			*/
			if (*lastsortp != curorder)
			{
				qsort(curlist, ncurlist,
				        sizeof(struct tstat *),
				        procsort[(int)curorder&0x1f]);

				*lastsortp = curorder;

				zipagain = 1;
			}

			if (threadview && threadallowed)
			{
				int ntotal, j, t;

				if (deviatonly && showtype  != MPROCMEM &&
			      	                  showorder != MSORTMEM   )
					ntotal = devtstat->ntaskactive;
				else
					ntotal = devtstat->ntaskall;

				/*
  				** check if existing pointer list still usable
				** if not, allocate new pointer list to be able
				** to zip process list with references to threads
				*/
				if (!tsklist || ntsk != ntotal ||
							tdeviate != deviatonly)
				{
					if (tsklist)
						free(tsklist);	// remove current

					tsklist = malloc(sizeof(struct tstat *)
								    * ntotal);

					ptrverify(tsklist,
				             "Malloc failed for %d taskptrs\n",
				             ntotal);

					ntsk     = ntotal;
					tdeviate = deviatonly;

					zipagain = 1;
				}
				else
					j = ntotal;

				if (zipagain)
				{
					struct tstat *tall = devtstat->taskall;
					struct tstat *pcur;

					for (i=j=0; i < ncurlist; i++)
					{
					    pcur = curlist[i];

					    tsklist[j++] = pcur;

					    for (t = pcur - tall + 1;
					         t < devtstat->ntaskall &&
						 pcur->gen.tgid		&&
					         pcur->gen.tgid == 
					            (tall+t)->gen.tgid;
					         t++)
					    {
						if (deviatonly &&
							showtype  != MPROCMEM &&
						        showorder != MSORTMEM   )
						{
						  if (!(tall+t)->gen.wasinactive)
						  {
							tsklist[j++] = tall+t;
						  }
 						}
						else
							tsklist[j++] = tall+t;
					    }
					}

					zipagain = 0;
				}

				curlist  = tsklist;
				ncurlist = j;
			}

			/*
			** print the header
			** first determine the column-header for the current
			** sorting order of processes
			*/
			if (screen)
			{
				attron(A_REVERSE);
                                move(curline+1, 0);
                        }

			priphead(firstproc/plistsz+1, (ncurlist-1)/plistsz+1,
			       		&showtype, &curorder,
					showorder == MSORTAUTO ? 1 : 0);

			if (screen)
			{
				attroff(A_REVERSE);
				clrtobot();
			}

			/*
			** print the list
			*/
			priproc(curlist, firstproc, ncurlist, curline+2,
			        firstproc/plistsz+1, (ncurlist-1)/plistsz+1,
			        showtype, curorder, &syscap, nsecs, avgval);
		}

		alistsz = ncurlist;	/* preserve size of active list */

		/*
		** in case of writing to a terminal, the user can also enter
		** a character to switch options, etc
		*/
		if (screen)
		{
			/*
			** show blinking pause-indication if necessary
			*/
			if (paused)
			{
				move(statline, COLS-6);
				attron(A_BLINK);
				attron(A_REVERSE);
				printw("PAUSED");
				attroff(A_REVERSE);
				attroff(A_BLINK);
			}

			/*
			** await input-character or interval-timer expiration
			*/
			switch ( (lastchar = mvgetch(statline, 0)) )
			{
			   /*
			   ** timer expired
			   */
			   case ERR:
			   case 0:
				timeout(0);
				(void) getch();
				timeout(-1);
				if (tpcumlist) free(tpcumlist);
				if (pcumlist)  free(pcumlist);
				if (tucumlist) free(tucumlist);
				if (ucumlist)  free(ucumlist);
				if (tccumlist) free(tccumlist);
				if (ccumlist)  free(ccumlist);
				if (tsklist)   free(tsklist);
				if (sellist)   free(sellist);

				return lastchar;	

			   /*
			   ** stop it
			   */
			   case MQUIT:
				move(LINES-1, 0);
				clrtoeol();
				refresh();
				cleanstop(0);

			   /*
			   ** manual trigger for next sample
			   */
			   case MSAMPNEXT:
				if (paused)
					break;

				getalarm(0);

				if (tpcumlist) free(tpcumlist);
				if (pcumlist)  free(pcumlist);
				if (tucumlist) free(tucumlist);
				if (ucumlist)  free(ucumlist);
				if (tccumlist) free(tccumlist);
				if (ccumlist)  free(ccumlist);
				if (tsklist)   free(tsklist);
				if (sellist)   free(sellist);

				return lastchar;

			   /*
			   ** manual trigger for previous sample
			   */
			   case MSAMPPREV:
				if (!rawreadflag)
				{
					statmsg = "Only allowed when viewing "
					          "raw file!";
					beep();
					break;
				}

				if (paused)
					break;

				if (tpcumlist) free(tpcumlist);
				if (pcumlist)  free(pcumlist);
				if (tucumlist) free(tucumlist);
				if (ucumlist)  free(ucumlist);
				if (tccumlist) free(tccumlist);
				if (ccumlist)  free(ccumlist);
				if (tsklist)   free(tsklist);
				if (sellist)   free(sellist);

				return lastchar;

                           /*
			   ** branch to certain time stamp
                           */
                           case MSAMPBRANCH:
                                if (!rawreadflag)
                                {
                                        statmsg = "Only allowed when viewing "
                                                  "raw file!";
                                        beep();
                                        break;
                                }

                                if (paused)
                                        break;

                                echo();
                                move(statline, 0);
                                clrtoeol();
                                printw("Enter new time (format hh:mm): ");

                                hhmm[0] = '\0';
                                scanw("%15s\n", hhmm);
                                noecho();

                                if ( !hhmm2secs(hhmm, &begintime) )
                                {
                                        move(statline, 0);
                                        clrtoeol();
                                        statmsg = "Wrong time format!";
                                        beep();
                                        begintime = 0;
                                        break;
                                }

				if (tpcumlist) free(tpcumlist);
				if (pcumlist)  free(pcumlist);
				if (tucumlist) free(tucumlist);
				if (ucumlist)  free(ucumlist);
				if (tccumlist) free(tccumlist);
				if (ccumlist)  free(ccumlist);
				if (tsklist)   free(tsklist);
				if (sellist)   free(sellist);

                                return lastchar;

			   /*
			   ** sort order automatically depending on
			   ** most busy resource
			   */
			   case MSORTAUTO:
				showorder = MSORTAUTO;
				firstproc = 0;
				break;

			   /*
			   ** sort in cpu-activity order
			   */
			   case MSORTCPU:
				showorder = MSORTCPU;
				firstproc = 0;
				break;

			   /*
			   ** sort in memory-consumption order
			   */
			   case MSORTMEM:
				showorder = MSORTMEM;
				firstproc = 0;
				break;

			   /*
			   ** sort in disk-activity order
			   */
			   case MSORTDSK:
				if ( !(supportflags & IOSTAT) )
				{
					statmsg = "No disk-activity figures "
					          "available; request ignored!";
					break;
				}
				showorder = MSORTDSK;
				firstproc = 0;
				break;

			   /*
			   ** sort in network-activity order
			   */
			   case MSORTNET:
				if ( !(supportflags & NETATOP) )
				{
					statmsg = "Kernel module 'netatop' not "
					          "active or no root privs; "
					          "request ignored!";
					break;
				}
				showorder = MSORTNET;
				firstproc = 0;
				break;

			   /*
			   ** general figures per process
			   */
			   case MPROCGEN:
				showtype  = MPROCGEN;

				if (showorder != MSORTAUTO)
					showorder = MSORTCPU;

				firstproc = 0;
				break;

			   /*
			   ** memory-specific figures per process
			   */
			   case MPROCMEM:
				showtype  = MPROCMEM;

				if (showorder != MSORTAUTO)
					showorder = MSORTMEM;

				firstproc = 0;
				break;

			   /*
			   ** disk-specific figures per process
			   */
			   case MPROCDSK:
				if ( !(supportflags & IOSTAT) )
				{
					statmsg = "No disk-activity figures "
					          "available; request ignored!";
					break;
				}

				showtype  = MPROCDSK;

				if (showorder != MSORTAUTO)
					showorder = MSORTDSK;

				firstproc = 0;
				break;

			   /*
			   ** network-specific figures per process
			   */
			   case MPROCNET:
				if ( !(supportflags & NETATOP) )
				{
					statmsg = "Kernel module 'netatop' not "
					          "active or no root privs; "
					          "request ignored!";
					break;
				}

				showtype  = MPROCNET;

				if (showorder != MSORTAUTO)
					showorder = MSORTNET;

				firstproc = 0;
				break;

			   /*
			   ** various info per process
			   */
			   case MPROCVAR:
				showtype  = MPROCVAR;
				firstproc = 0;
				break;

			   /*
			   ** command line per process
			   */
			   case MPROCARG:
				showtype  = MPROCARG;
				firstproc = 0;
				break;

			   /*
			   ** own defined output per process
			   */
			   case MPROCOWN:
				if (! ownprocs[0].f)
				{
					statmsg = "Own process line is not "
					          "configured in rc-file; "
					          "request ignored";
					break;
				}

				showtype  = MPROCOWN;
				firstproc = 0;
				break;

			   /*
			   ** scheduling-values per process
			   */
			   case MPROCSCH:
				showtype  = MPROCSCH;

				if (showorder != MSORTAUTO)
					showorder = MSORTCPU;

				firstproc = 0;
				break;

			   /*
			   ** accumulated resource consumption per user
			   */
			   case MCUMUSER:
				statmsg = "Consumption per user; use 'a' to "
				          "toggle between all/active processes";

				showtype  = MCUMUSER;
				firstproc = 0;
				break;

			   /*
			   ** accumulated resource consumption per program
			   */
			   case MCUMPROC:
				statmsg = "Consumption per program; use 'a' to "
				          "toggle between all/active processes";

				showtype  = MCUMPROC;
				firstproc = 0;
				break;

			   /*
			   ** accumulated resource consumption per container
			   */
			   case MCUMCONT:
				statmsg = "Consumption per container; use 'a' to "
				          "toggle between all/active processes";

				showtype  = MCUMCONT;
				firstproc = 0;
				break;

			   /*
			   ** help wanted?
			   */
			   case MHELP1:
			   case MHELP2:
				alarm(0);	/* stop the clock         */

				move(1, 0);
				clrtobot();	/* blank the screen */
				refresh();

				showhelp(2);

				move(statline, 0);

				if (interval && !paused && !rawreadflag)
					alarm(3); /* force new sample     */

				firstproc = 0;
				break;

			   /*
			   ** send signal to process
			   */
			   case MKILLPROC:
				if (rawreadflag)
				{
					statmsg = "Not possible when viewing "
					          "raw file!";
					beep();
					break;
				}

				alarm(0);	/* stop the clock */

				killpid = getnumval("Pid of process: ",
						     0, statline);

				switch (killpid)
				{
				   case 0:
				   case -1:
					break;

				   case 1:
					statmsg = "Sending signal to pid 1 not "
					          "allowed!";
					beep();
					break;

				   default:
					clrtoeol();
					killsig = getnumval("Signal [%d]: ",
						     15, statline);

					if ( kill(killpid, killsig) == -1)
					{
						statmsg = "Not possible to "
						     "send signal to this pid!";
						beep();
					}
				}

				if (!paused)
					alarm(3); /* set short timer */

				firstproc = 0;
				break;

			   /*
			   ** change interval timeout
			   */
			   case MINTERVAL:
				if (rawreadflag)
				{
					statmsg = "Not possible when viewing "
					          "raw file!";
					beep();
					break;
				}

				alarm(0);	/* stop the clock */

				interval = getnumval("New interval in seconds "
						     "(now %d): ",
						     interval, statline);

				if (interval)
				{
					if (!paused)
						alarm(3); /* set short timer */
				}
				else
				{
					statmsg = "No timer set; waiting for "
					          "manual trigger ('t').....";
				}

				firstproc = 0;
				break;

			   /*
			   ** focus on specific user
			   */
			   case MSELUSER:
				alarm(0);	/* stop the clock */
				echo();

				move(statline, 0);
				clrtoeol();
				printw("Username as regular expression "
				       "(enter=all users): ");

				procsel.username[0] = '\0';
				scanw("%255s\n", procsel.username);

				noecho();

				if (procsel.username[0]) /* data entered ? */
				{
					regex_t		userregex;
					int		u = 0;

					if ( regcomp(&userregex,
						procsel.username, REG_NOSUB))
					{
						statmsg = "Invalid regular "
						          "expression!";
						beep();

						procsel.username[0] = '\0';
					}
					else
					{
						while ( (pwd = getpwent()))
						{
							if (regexec(&userregex,
							    pwd->pw_name, 0,
							    NULL, 0))
								continue;

							if (u < MAXUSERSEL-1)
							{
							  procsel.userid[u] =
								pwd->pw_uid;
							  u++;
							}
						}
						endpwent();

						procsel.userid[u] = USERSTUB;

						if (u == 0)
						{
							/*
							** possibly a numerical
							** value specified?
							*/
							if (numeric(
							     procsel.username))
							{
							 procsel.userid[0] =
							 atoi(procsel.username);
							 procsel.userid[1] =
								USERSTUB;
							}
							else
							{
							     statmsg =
								"No user-names "
							    	"match this "
								"pattern!";
							     beep();
							}
						}
					}
				}
				else
				{
					procsel.userid[0] = USERSTUB;
				}

				if (interval && !paused && !rawreadflag)
					alarm(3);  /* set short timer */

				firstproc = 0;
				break;

			   /*
			   ** focus on specific process-name
			   */
			   case MSELPROC:
				alarm(0);	/* stop the clock */
				echo();

				move(statline, 0);
				clrtoeol();
				printw("Process-name as regular "
				       "expression (enter=no regex): ");

				procsel.prognamesz  = 0;
				procsel.progname[0] = '\0';

				scanw("%63s\n", procsel.progname);
				procsel.prognamesz = strlen(procsel.progname);

				if (procsel.prognamesz)
				{
					if (regcomp(&procsel.progregex,
					         procsel.progname, REG_NOSUB))
					{
						statmsg = "Invalid regular "
						          "expression!";
						beep();

						procsel.prognamesz  = 0;
						procsel.progname[0] = '\0';
					}
				}

				noecho();

				move(statline, 0);

				if (interval && !paused && !rawreadflag)
					alarm(3);  /* set short timer */

				firstproc = 0;
				break;

			   /*
			   ** focus on specific container id
			   */
			   case MSELCONT:
				alarm(0);	/* stop the clock */
				echo();

				move(statline, 0);
				clrtoeol();
				printw("Containerid 12 postitions "
 				       "(enter=all, "
				       "'host'=host processes): ");

				procsel.container[0]  = '\0';
				scanw("%15s", procsel.container);
				procsel.container[12] = '\0';

				switch (strlen(procsel.container))
				{
                                   case 0:
					break;	// enter key pressed

				   case 4:	// host?
					if (strcmp(procsel.container, "host"))
					{
						statmsg="Invalid containerid!";
						beep();
						procsel.container[0] = '\0';
					}
					else
					{
						procsel.container[0] = 'H';
						procsel.container[1] = '\0';
					}
					break;

				   case 12:	// container id
					(void)strtol(procsel.container, &p, 16);

					if (*p)
					{
						statmsg ="Containerid not hex!";
						beep();
						procsel.container[0] = '\0';
					}
					break;

				   default:
					statmsg = "Invalid containerid!";
					beep();

					procsel.container[0] = '\0';
				}

				noecho();

				move(statline, 0);

				if (interval && !paused && !rawreadflag)
					alarm(3);  /* set short timer */

				firstproc = 0;
				break;

			   /*
			   ** focus on specific PIDs
			   */
			   case MSELPID:
				alarm(0);	/* stop the clock */
				echo();

				move(statline, 0);
				clrtoeol();
				printw("Comma-separated PIDs of processes "
				                 "(enter=no selection): ");

				scanw("%79s\n", genline);

				int  id = 0;

				char *pidp = strtok(genline, ",");

				while (pidp)
				{
					char *ep;

					if (id >= MAXPID-1)
					{
						procsel.pid[id] = 0;	// stub

						statmsg = "Maximum number of"
						          "PIDs reached!";
						beep();
						break;
					}

					procsel.pid[id] = strtol(pidp, &ep, 10);

					if (*ep)
					{
						statmsg = "Non-numerical PID!";
						beep();
						procsel.pid[0]  = 0;  // stub
						break;
					}

					id++;
					pidp = strtok(NULL, ",");
				}

				procsel.pid[id] = 0;	// stub

				noecho();

				move(statline, 0);

				if (interval && !paused && !rawreadflag)
					alarm(3);  /* set short timer */

				firstproc = 0;
				break;


			   /*
			   ** focus on specific command line arguments
			   */
			   case MSELARG:
				alarm(0);	/* stop the clock */
				echo();

				move(statline, 0);
				clrtoeol();
				printw("Command line string as regular "
				       "expression (enter=no regex): ");

				procsel.argnamesz  = 0;
				procsel.argname[0] = '\0';

				scanw("%63s\n", procsel.argname);
				procsel.argnamesz = strlen(procsel.argname);

				if (procsel.argnamesz)
				{
					if (regcomp(&procsel.argregex,
					         procsel.argname, REG_NOSUB))
					{
						statmsg = "Invalid regular "
						          "expression!";
						beep();

						procsel.argnamesz  = 0;
						procsel.argname[0] = '\0';
					}
				}

				noecho();

				move(statline, 0);

				if (interval && !paused && !rawreadflag)
					alarm(3);  /* set short timer */

				firstproc = 0;
				break;

			   /*
			   ** focus on specific system resource
			   */
			   case MSELSYS:
				alarm(0);	/* stop the clock */
				echo();

				move(statline, 0);
				clrtoeol();
				printw("Logical volume name as regular "
				       "expression (enter=no specific name): ");

				syssel.lvmnamesz  = 0;
				syssel.lvmname[0] = '\0';

				scanw("%63s\n", syssel.lvmname);
				syssel.lvmnamesz = strlen(syssel.lvmname);

				if (syssel.lvmnamesz)
				{
					if (regcomp(&syssel.lvmregex,
					         syssel.lvmname, REG_NOSUB))
					{
						statmsg = "Invalid regular "
						          "expression!";
						beep();

						syssel.lvmnamesz  = 0;
						syssel.lvmname[0] = '\0';
					}
				}

				move(statline, 0);
				clrtoeol();
				printw("Disk name as regular "
				       "expression (enter=no specific name): ");

				syssel.dsknamesz  = 0;
				syssel.dskname[0] = '\0';

				scanw("%63s\n", syssel.dskname);
				syssel.dsknamesz = strlen(syssel.dskname);

				if (syssel.dsknamesz)
				{
					if (regcomp(&syssel.dskregex,
					         syssel.dskname, REG_NOSUB))
					{
						statmsg = "Invalid regular "
						          "expression!";
						beep();

						syssel.dsknamesz  = 0;
						syssel.dskname[0] = '\0';
					}
				}

				move(statline, 0);
				clrtoeol();
				printw("Interface name as regular "
				       "expression (enter=no specific name): ");

				syssel.itfnamesz  = 0;
				syssel.itfname[0] = '\0';

				scanw("%63s\n", syssel.itfname);
				syssel.itfnamesz = strlen(syssel.itfname);

				if (syssel.itfnamesz)
				{
					if (regcomp(&syssel.itfregex,
					         syssel.itfname, REG_NOSUB))
					{
						statmsg = "Invalid regular "
						          "expression!";
						beep();

						syssel.itfnamesz  = 0;
						syssel.itfname[0] = '\0';
					}
				}

				noecho();

				move(statline, 0);

				if (interval && !paused && !rawreadflag)
					alarm(3);  /* set short timer */

				firstproc = 0;
				break;

			   /*
			   ** toggle pause-state
			   */
			   case MPAUSE:
				if (paused)
				{
					paused=0;
					clrtoeol();
					refresh();

					if (!rawreadflag)
						alarm(1);
				}
				else
				{
					paused=1;
					clrtoeol();
					refresh();
					alarm(0);	/* stop the clock */
				}
				break;

			   /*
			   ** toggle between modified processes and
			   ** all processes
			   */
			   case MALLPROC:
				if (deviatonly)
				{
					deviatonly=0;
					statmsg = "All processes/threads will be "
					          "shown/accumulated...";
				}
				else
				{
					deviatonly=1;
					statmsg = "Only active processes/threads "
					          "will be shown/accumulated...";
				}

				tlastorder = 0;
				firstproc  = 0;
				break;

			   /*
			   ** toggle average or total values
			   */
			   case MAVGVAL:
				if (avgval)
					avgval=0;
				else
					avgval=1;
				break;

			   /*
			   ** system-statistics lines:
			   **	         toggle fixed or variable
			   */
			   case MSYSFIXED:
				if (fixedhead)
				{
					fixedhead=0;
					statmsg = "Only active system-resources"
					          " will be shown ......";
				}
				else
				{
					fixedhead=1;
					statmsg = "Also inactive "
					  "system-resources will be shown.....";
				}

				firstproc = 0;
				break;

			   /*
			   ** system-statistics lines:
			   **	         toggle fixed or variable
			   */
			   case MSYSNOSORT:
				if (sysnosort)
				{
					sysnosort=0;
					statmsg = "System resources will be "
					          "sorted on utilization...";
				}
				else
				{
					sysnosort=1;
					statmsg = "System resources will not "
					          "be sorted on utilization...";
				}

				firstproc = 0;
				break;

			   /*
			   ** per-thread view wanted with sorting on
			   ** process level or thread level
			   */
			   case MTHREAD:
				if (threadview)
				{
					threadview = 0;
					statmsg    = "Thread view disabled";
					firstproc  = 0;
				}
				else
				{
					threadview = 1;
					statmsg    = "Thread view enabled";
					firstproc  = 0;
				}
				break;

			   /*
			   ** per-process PSS calculation wanted 
			   */
			   case MCALCPSS:
				if (calcpss)
				{
					calcpss    = 0;
					statmsg    = "PSIZE gathering disabled";
				}
				else
				{
					calcpss    = 1;
					statmsg    = "PSIZE gathering enabled";
				}
				break;

			   /*
			   ** suppression of exited processes in output
			   */
			   case MSUPEXITS:
				if (suppressexit)
				{
					suppressexit = 0;
					statmsg      = "Exited processes will "
					               "be shown/accumulated";
					firstproc    = 0;
				}
				else
				{
					suppressexit = 1;
					statmsg      = "Exited processes will "
					             "not be shown/accumulated";
					firstproc    = 0;
				}
				break;

			   /*
			   ** screen lines:
			   **	         toggle for colors
			   */
			   case MCOLORS:
				if (usecolors)
				{
					usecolors=0;
					statmsg = "No colors will be used...";
				}
				else
				{
					if (screen && has_colors())
					{
						usecolors=1;
						statmsg =
						   "Colors will be used...";
					}
					else
					{
						statmsg="No colors supported!";
					}
				}

				firstproc = 0;
				break;

			   /*
			   ** system-statistics lines:
			   **	         toggle no or all active disk
			   */
			   case MSYSLIMIT:
				alarm(0);	/* stop the clock */

				maxcpulines =
				  getnumval("Maximum lines for per-cpu "
				            "statistics (now %d): ",
				            maxcpulines, statline);

				if (sstat->dsk.nlvm > 0)
				{
					maxlvmlines =
					  getnumval("Maximum lines for LVM "
				            "statistics (now %d): ",
				            maxlvmlines, statline);
				}

				if (sstat->dsk.nmdd > 0)
				{
			  		maxmddlines =
					  getnumval("Maximum lines for MD "
					    "device statistics (now %d): ",
				            maxmddlines, statline);
				}

				maxdsklines =
				  getnumval("Maximum lines for disk "
				            "statistics (now %d): ",
				            maxdsklines, statline);

				maxintlines =
				  getnumval("Maximum lines for interface "
				            "statistics (now %d): ",
					    maxintlines, statline);

				maxnfslines =
				  getnumval("Maximum lines for NFS mount "
				            "statistics (now %d): ",
					    maxnfslines, statline);

				maxcontlines =
				  getnumval("Maximum lines for container "
				            "statistics (now %d): ",
					    maxcontlines, statline);

				if (interval && !paused && !rawreadflag)
					alarm(3);  /* set short timer */

				firstproc = 0;
				break;

			   /*
			   ** reset statistics 
			   */
			   case MRESET:
				getalarm(0);	/* restart the clock */
				paused = 0;

				if (tpcumlist) free(tpcumlist);
				if (pcumlist)  free(pcumlist);
				if (tucumlist) free(tucumlist);
				if (ucumlist)  free(ucumlist);
				if (tccumlist) free(tccumlist);
				if (ccumlist)  free(ccumlist);
				if (tsklist)   free(tsklist);
				if (sellist)   free(sellist);

				return lastchar;

			   /*
			   ** show version info
			   */
			   case MVERSION:
				statmsg = getstrvers();
				break;

			   /*
			   ** handle redraw request
			   */
			   case MREDRAW:
                                wclear(stdscr);
				break;

			   /*
			   ** handle arrow right for command line
			   */
			   case KEY_RIGHT:
				startoffset++;
				break;

			   /*
			   ** handle arrow left for command line
			   */
			   case KEY_LEFT:
				if (startoffset > 0)
					startoffset--;
				break;

			   /*
			   ** handle arrow down to go one line down
			   */
			   case KEY_DOWN:
				if (firstproc < alistsz-1)
					firstproc += 1;
				break;

			   /*
			   ** handle arrow up to go one line up
			   */
			   case KEY_UP:	
				if (firstproc > 0)
					firstproc -= 1;
				break;

			   /*
			   ** handle forward
			   */
			   case KEY_NPAGE:
			   case MLISTFW:
				if (alistsz-firstproc > plistsz)
					firstproc += plistsz;
				break;

			   /*
			   ** handle backward
			   */
			   case KEY_PPAGE:
			   case MLISTBW:
				if (firstproc >= plistsz)
					firstproc -= plistsz;
				else
					firstproc = 0;
				break;

			   /*
			   ** handle screen resize
			   */
			   case KEY_RESIZE:
				snprintf(statbuf, sizeof statbuf, 
					"Window resized to %dx%d...",
			         		COLS, LINES);
				statmsg = statbuf;

				timeout(0);
				(void) getch();
				timeout(-1);
				break;

			   /*
			   ** unknown key-stroke
			   */
			   default:
			        beep();
			}
		}
		else	/* no screen */
		{
			if (tpcumlist) free(tpcumlist);
			if (pcumlist)  free(pcumlist);
			if (tucumlist) free(tucumlist);
			if (ucumlist)  free(ucumlist);
			if (tccumlist) free(tccumlist);
			if (ccumlist)  free(ccumlist);
			if (tsklist)   free(tsklist);
			if (sellist)   free(sellist);

			return '\0';
		}
	}
}

/*
** accumulate all processes per user in new list
*/
static int
cumusers(struct tstat **curprocs, struct tstat *curusers, int numprocs)
{
	register int	i, numusers;

	/*
	** sort list of active processes in order of uid (increasing)
	*/
	qsort(curprocs, numprocs, sizeof(struct tstat *), compusr);

	/*
	** accumulate all processes per user in the new list
	*/
	for (numusers=i=0; i < numprocs; i++, curprocs++)
	{
		if (procsuppress(*curprocs, &procsel))
			continue;

		if ((*curprocs)->gen.state == 'E' && suppressexit)
			continue;
 
		if ( curusers->gen.ruid != (*curprocs)->gen.ruid )
		{
			if (curusers->gen.pid)
			{
				numusers++;
				curusers++;
			}
			curusers->gen.ruid = (*curprocs)->gen.ruid;
		}

		accumulate(*curprocs, curusers);
	}

	if (curusers->gen.pid)
		numusers++;

	return numusers;
}


/*
** accumulate all processes with the same name (i.e. same program)
** into a new list
*/
static int
cumprogs(struct tstat **curprocs, struct tstat *curprogs, int numprocs)
{
	register int	i, numprogs;

	/*
	** sort list of active processes in order of process-name
	*/
	qsort(curprocs, numprocs, sizeof(struct tstat *), compnam);

	/*
	** accumulate all processes with same name in the new list
	*/
	for (numprogs=i=0; i < numprocs; i++, curprocs++)
	{
		if (procsuppress(*curprocs, &procsel))
			continue;

		if ((*curprocs)->gen.state == 'E' && suppressexit)
			continue;

		if ( strcmp(curprogs->gen.name, (*curprocs)->gen.name) != 0)
		{
			if (curprogs->gen.pid)
			{
				numprogs++;
				curprogs++;
			}
			strcpy(curprogs->gen.name, (*curprocs)->gen.name);
		}

		accumulate(*curprocs, curprogs);
	}

	if (curprogs->gen.pid)
		numprogs++;

	return numprogs;
}

/*
** accumulate all processes per container in new list
*/
static int
cumconts(struct tstat **curprocs, struct tstat *curconts, int numprocs)
{
	register int	i, numconts;

	/*
	** sort list of active processes in order of container (increasing)
	*/
	qsort(curprocs, numprocs, sizeof(struct tstat *), compcon);

	/*
	** accumulate all processes per container in the new list
	*/
	for (numconts=i=0; i < numprocs; i++, curprocs++)
	{
		if (procsuppress(*curprocs, &procsel))
			continue;

		if ((*curprocs)->gen.state == 'E' && suppressexit)
			continue;
 
		if ( strcmp(curconts->gen.container,
                         (*curprocs)->gen.container) != 0)
		{
			if (curconts->gen.pid)
			{
				numconts++;
				curconts++;
			}
			strcpy(curconts->gen.container,
			    (*curprocs)->gen.container);
		}

		accumulate(*curprocs, curconts);
	}

	if (curconts->gen.pid)
		numconts++;

	return numconts;
}


/*
** accumulate relevant counters from individual task to
** combined task
*/
static void
accumulate(struct tstat *curproc, struct tstat *curstat)
{
	count_t		nett_wsz;

	curstat->gen.pid++;		/* misuse as counter */

	curstat->gen.isproc  = 1;
	curstat->gen.nthr   += curproc->gen.nthr;
	curstat->cpu.utime  += curproc->cpu.utime;
	curstat->cpu.stime  += curproc->cpu.stime;

	if (curproc->dsk.wsz > curproc->dsk.cwsz)
               	nett_wsz = curproc->dsk.wsz -curproc->dsk.cwsz;
	else
		nett_wsz = 0;

	curstat->dsk.rio    += curproc->dsk.rsz;
	curstat->dsk.wio    += nett_wsz;

	curstat->dsk.rsz     = curstat->dsk.rio;
	curstat->dsk.wsz     = curstat->dsk.wio;

	curstat->net.tcpsnd += curproc->net.tcpsnd;
	curstat->net.tcprcv += curproc->net.tcprcv;
	curstat->net.udpsnd += curproc->net.udpsnd;
	curstat->net.udprcv += curproc->net.udprcv;

	curstat->net.tcpssz += curproc->net.tcpssz;
	curstat->net.tcprsz += curproc->net.tcprsz;
	curstat->net.udpssz += curproc->net.udpssz;
	curstat->net.udprsz += curproc->net.udprsz;

	if (curproc->gen.state != 'E')
	{
		if (curstat->mem.pmem != -1)
		{
			if  (curproc->mem.pmem != -1)  // no errors?
				curstat->mem.pmem += curproc->mem.pmem;
			else
				curstat->mem.pmem  = -1;
		}

		curstat->mem.vmem   += curproc->mem.vmem;
		curstat->mem.rmem   += curproc->mem.rmem;
		curstat->mem.vlibs  += curproc->mem.vlibs;
		curstat->mem.vdata  += curproc->mem.vdata;
		curstat->mem.vstack += curproc->mem.vstack;
		curstat->mem.vswap  += curproc->mem.vswap;
		curstat->mem.rgrow  += curproc->mem.rgrow;
		curstat->mem.vgrow  += curproc->mem.vgrow;
	}
}


/*
** function that checks if the current process is selected or suppressed;
** returns 1 (suppress) or 0 (do not suppress)
*/
static int
procsuppress(struct tstat *curstat, struct pselection *sel)
{
	/*
	** check if only processes of a particular user
	** should be shown
	*/
	if (sel->userid[0] != USERSTUB)
	{
		int     u = 0;

		while (sel->userid[u] != USERSTUB)
		{
			if (sel->userid[u] == curstat->gen.ruid)
				break;
			u++;
		}

		if (sel->userid[u] != curstat->gen.ruid)
			return 1;
	}

	/*
	** check if only processes with particular PIDs
	** should be shown
	*/
	if (sel->pid[0])
	{
		int i = 0;

		while (sel->pid[i])
		{
			if (sel->pid[i] == curstat->gen.pid)
				break;
			i++;
		}

		if (sel->pid[i] != curstat->gen.pid)
			return 1;
	}

	/*
	** check if only processes with a particular name
	** should be shown
	*/
	if (sel->prognamesz &&
	    regexec(&(sel->progregex), curstat->gen.name, 0, NULL, 0))
		return 1;

	/*
	** check if only processes with a particular command line string
	** should be shown
	*/
	if (sel->argnamesz)
	{
		if (curstat->gen.cmdline[0])
		{
			if (regexec(&(sel->argregex), curstat->gen.cmdline,
								0, NULL, 0))
				return 1;
		}
		else
		{
			if (regexec(&(sel->argregex), curstat->gen.name,
								0, NULL, 0))
				return 1;
		}
	}

	/*
	** check if only processes related to a particular container
	** should be shown (container 'H' stands for native host processes)
	*/
	if (sel->container[0])
	{
		if (sel->container[0] == 'H')	// only host processes
		{
			if (curstat->gen.container[0])
				return 1;
		}
		else
		{
			if (memcmp(sel->container, curstat->gen.container, 12))
				return 1;
		}
	}

	return 0;
}


static void
limitedlines(void)
{
	maxcpulines  = 0;
	maxdsklines  = 3;
	maxmddlines  = 3;
	maxlvmlines  = 4;
	maxintlines  = 2;
	maxnfslines  = 2;
	maxcontlines = 0;
}

/*
** get a numerical value from the user and verify 
*/
static long
getnumval(char *ask, long valuenow, int statline)
{
	char numval[16];
	long retval;

	echo();
	move(statline, 0);
	clrtoeol();
	printw(ask, valuenow);

	numval[0] = 0;
	scanw("%15s", numval);

	move(statline, 0);
	noecho();

	if (numval[0])  /* data entered ? */
	{
		if ( numeric(numval) )
		{
			retval = atol(numval);
		}
		else
		{
			beep();
			clrtoeol();
			printw("Value not numeric (current value kept)!");
			refresh();
			sleep(2);
			retval = valuenow;
		}
	}
	else
	{
		retval = valuenow;
	}

	return retval;
}

/*
** generic print-function which checks if printf should be used
** (to file or pipe) or curses (to screen)
*/
void
printg(const char *format, ...)
{
	va_list	args;

	va_start(args, format);

	if (screen)
		vwprintw(stdscr, (char *) format, args);
	else
		vprintf(format, args);

	va_end  (args);
}

/*
** initialize generic sample output functions
*/
static void
generic_init(void)
{
	int i;

	/*
	** check if default sort order and/or showtype are overruled
	** by command-line flags
	*/
	for (i=0; flaglist[i]; i++)
	{
		switch (flaglist[i])
		{
		   case MSORTAUTO:
			showorder = MSORTAUTO;
			break;

		   case MSORTCPU:
			showorder = MSORTCPU;
			break;

		   case MSORTMEM:
			showorder = MSORTMEM;
			break;

		   case MSORTDSK:
			showorder = MSORTDSK;
			break;

		   case MSORTNET:
			showorder = MSORTNET;
			break;

		   case MPROCGEN:
			showtype  = MPROCGEN;
			showorder = MSORTCPU;
			break;

		   case MPROCMEM:
			showtype  = MPROCMEM;
			showorder = MSORTMEM;
			break;

		   case MPROCSCH:
			showtype  = MPROCSCH;
			showorder = MSORTCPU;
			break;

		   case MPROCDSK:
			if ( !(supportflags & IOSTAT) )
			{
				fprintf(stderr,
					"No disk-activity figures "
				        "available; request ignored\n");
				sleep(3);
				break;
			}

			showtype  = MPROCDSK;
			showorder = MSORTDSK;
			break;

		   case MPROCNET:
			if ( !(supportflags & NETATOP) )
			{
				fprintf(stderr, "Kernel module 'netatop' not "
					          "active; request ignored!");
				sleep(3);
				break;
			}

			showtype  = MPROCNET;
			showorder = MSORTNET;
			break;

		   case MPROCVAR:
			showtype  = MPROCVAR;
			break;

		   case MPROCARG:
			showtype  = MPROCARG;
			break;

		   case MPROCOWN:
			showtype  = MPROCOWN;
			break;

		   case MAVGVAL:
			if (avgval)
				avgval=0;
			else
				avgval=1;
			break;

		   case MCUMUSER:
			showtype  = MCUMUSER;
			break;

		   case MCUMPROC:
			showtype  = MCUMPROC;
			break;

		   case MCUMCONT:
			showtype  = MCUMCONT;
			break;

		   case MSYSFIXED:
			if (fixedhead)
				fixedhead=0;
			else
				fixedhead=1;
			break;

		   case MSYSNOSORT:
			if (sysnosort)
				sysnosort=0;
			else
				sysnosort=1;
			break;

		   case MTHREAD:
			if (threadview)
				threadview = 0;
			else
				threadview = 1;
			break;

		   case MCALCPSS:
			if (calcpss)
				calcpss = 0;
			else
				calcpss = 1;
			break;

		   case MSUPEXITS:
			if (suppressexit)
				suppressexit = 0;
			else
				suppressexit = 1;
			break;

		   case MCOLORS:
			if (usecolors)
				usecolors=0;
			else
				usecolors=1;
			break;

		   case MSYSLIMIT:
			limitedlines();
			break;

		   default:
			prusage("atop");
		}
	}

       	/*
       	** set stdout output on line-basis
       	*/
       	setvbuf(stdout, (char *)0, _IOLBF, BUFSIZ);

       	/*
       	** check if STDOUT is related to a tty or
       	** something else (file, pipe)
       	*/
       	if ( isatty(1) )
               	screen = 1;
       	else
             	screen = 0;

       	/*
       	** install catch-routine to finish in a controlled way
	** and activate cbreak-mode
       	*/
       	if (screen)
	{
		/*
		** initialize screen-handling via curses
		*/
		initscr();
		cbreak();
		noecho();
		keypad(stdscr, TRUE);

		if (COLS  < 30)
		{
			endwin();	// finish curses interface

			fprintf(stderr, "Not enough columns available\n"
			                "(need at least %d columns)\n", 30);
			fprintf(stderr, "Please resize window....\n");

			cleanstop(1);
		}

		if (has_colors())
		{
			use_default_colors();
			start_color();

			init_pair(COLORINFO,   colorinfo,   -1);
			init_pair(COLORALMOST, coloralmost, -1);
			init_pair(COLORCRIT,   colorcrit,   -1);
			init_pair(COLORTHR,    colorthread, -1);
		}
		else
		{
			usecolors = 0;
		}
	}

	signal(SIGINT,   cleanstop);
	signal(SIGTERM,  cleanstop);
}

/*
** show help information in interactive mode
*/
static struct helptext {
	char *helpline;
	char helparg;
} helptext[] = {
	{"Figures shown for active processes:\n", 		' '},
	{"\t'%c'  - generic info (default)\n",			MPROCGEN},
	{"\t'%c'  - memory details\n",				MPROCMEM},
	{"\t'%c'  - disk details\n",				MPROCDSK},
	{"\t'%c'  - network details\n",				MPROCNET},
	{"\t'%c'  - scheduling and thread-group info\n",	MPROCSCH},
	{"\t'%c'  - various info (ppid, user/group, date/time, status, "
	 "exitcode)\n",	MPROCVAR},
	{"\t'%c'  - full command line per process\n",		MPROCARG},
	{"\t'%c'  - use own output line definition\n",		MPROCOWN},
	{"\n",							' '},
	{"Sort list of processes in order of:\n",		' '},
	{"\t'%c'  - cpu activity\n",				MSORTCPU},
	{"\t'%c'  - memory consumption\n",			MSORTMEM},
	{"\t'%c'  - disk activity\n",				MSORTDSK},
	{"\t'%c'  - network activity\n",			MSORTNET},
	{"\t'%c'  - most active system resource (auto mode)\n",	MSORTAUTO},
	{"\n",							' '},
	{"Accumulated figures:\n",				' '},
	{"\t'%c'  - total resource consumption per user\n", 	MCUMUSER},
	{"\t'%c'  - total resource consumption per program (i.e. same "
	 "process name)\n",					MCUMPROC},
	{"\t'%c'  - total resource consumption per container\n",MCUMCONT},
	{"\n",							' '},
	{"Process selections (keys shown in header line):\n",	' '},
	{"\t'%c'  - focus on specific user name           "
	                              "(regular expression)\n", MSELUSER},
	{"\t'%c'  - focus on specific program name        "
	                              "(regular expression)\n", MSELPROC},
	{"\t'%c'  - focus on specific contained id (CID)\n",    MSELCONT},
	{"\t'%c'  - focus on specific command line string "
	                              "(regular expression)\n", MSELARG},
	{"\t'%c'  - focus on specific process id (PID)\n",      MSELPID},
	{"\n",							' '},
	{"System resource selections (keys shown in header line):\n",' '},
	{"\t'%c'  - focus on specific system resources    "
	                              "(regular expression)\n", MSELSYS},
	{"\n",							      ' '},
	{"Screen-handling:\n",					      ' '},
	{"\t^L   - redraw the screen                       \n",	      ' '},
	{"\tPgDn - show next page in the process list (or ^F)\n",     ' '},
	{"\tArDn - arrow-down for next line in process list\n",       ' '},
	{"\tPgUp - show previous page in the process list (or ^B)\n", ' '},
	{"\tArUp   arrow-up for previous line in process list\n",     ' '},
	{"\n",							' '},
	{"\tArRt - arrow-right for next character in full command line\n", ' '},
	{"\tArLt - arrow-left  for previous character in full command line\n",
									' '},
	{"\n",							' '},
	{"Presentation (keys shown in header line):\n",  	' '},
	{"\t'%c'  - show individual threads                        (toggle)\n",
		 						MTHREAD},
	{"\t'%c'  - show all processes (default: active processes) (toggle)\n",
								MALLPROC},
	{"\t'%c'  - show fixed number of header lines              (toggle)\n",
								MSYSFIXED},
	{"\t'%c'  - suppress sorting system resources              (toggle)\n",
								MSYSNOSORT},
	{"\t'%c'  - suppress exited processes in output            (toggle)\n",
								MSUPEXITS},
	{"\t'%c'  - no colors to indicate high occupation          (toggle)\n",
								MCOLORS},
	{"\t'%c'  - show average-per-second i.s.o. total values    (toggle)\n",
								MAVGVAL},
	{"\t'%c'  - calculate proportional set size (PSIZE)        (toggle)\n",
								MCALCPSS},
	{"\n",							' '},
	{"Raw file viewing:\n",					' '},
	{"\t'%c'  - show next     sample in raw file\n",	MSAMPNEXT},
	{"\t'%c'  - show previous sample in raw file\n",	MSAMPPREV},
	{"\t'%c'  - branch to certain time in raw file\n",	MSAMPBRANCH},
	{"\t'%c'  - rewind to begin of raw file\n",		MRESET},
	{"\n",							' '},
	{"Miscellaneous commands:\n",				' '},
	{"\t'%c'  - change interval timer (0 = only manual trigger)\n",
								MINTERVAL},
	{"\t'%c'  - manual trigger to force next sample\n",	MSAMPNEXT},
	{"\t'%c'  - reset counters to boot time values\n",	MRESET},
	{"\t'%c'  - pause button to freeze current sample (toggle)\n",
								MPAUSE},
	{"\n",							' '},
	{"\t'%c'  - limited lines for per-cpu, disk and interface resources\n",
								MSYSLIMIT},
	{"\t'%c'  - kill a process (i.e. send a signal)\n",	MKILLPROC},
	{"\n",							' '},
	{"\t'%c'  - version information\n",			MVERSION},
	{"\t'%c'  - help information\n",			MHELP1},
	{"\t'%c'  - help information\n",			MHELP2},
	{"\t'%c'  - quit this program\n",			MQUIT},
};

static int helplines = sizeof(helptext)/sizeof(struct helptext);

static void
showhelp(int helpline)
{
	int	winlines = LINES-helpline, shown, tobeshown=1, i;
	WINDOW	*helpwin;

	/*
	** create a new window for the help-info in which scrolling is
	** allowed
	*/
	helpwin = newwin(winlines, COLS, helpline, 0);
	scrollok(helpwin, 1);

	/*
	** show help-lines 
	*/
	for (i=0, shown=0; i < helplines; i++, shown++)
	{
		wprintw(helpwin, helptext[i].helpline, helptext[i].helparg);

		/*
		** when the window is full, start paging interactively
		*/
		if (i >= winlines-2 && shown >= tobeshown)
		{
			wmove    (helpwin, winlines-1, 0);
			wclrtoeol(helpwin);
			wprintw  (helpwin, "Press 'q' to leave help, " 
					"space for next page or "
					"other key for next line... ");

			switch (wgetch(helpwin))
			{
			   case 'q':
				delwin(helpwin);
				return;
			   case ' ':
				shown = 0;
				tobeshown = winlines-1;
				break;
			   default:
				shown = 0;
				tobeshown = 1;
			}

			wmove  (helpwin, winlines-1, 0);
		}
	}

	wmove    (helpwin, winlines-1, 0);
	wclrtoeol(helpwin);
	wprintw  (helpwin, "End of help - press 'q' to leave help... ");
        while ( wgetch(helpwin) != 'q' );
	delwin   (helpwin);
}

/*
** function to be called to print error-messages
*/
void
generic_error(const char *format, ...)
{
	va_list	args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end  (args);
}

/*
** function to be called when the program stops
*/
void
generic_end(void)
{
	endwin();
}

/*
** function to be called when usage-info is required
*/
void
generic_usage(void)
{
	printf("\t  -%c  show fixed number of lines with system statistics\n",
			MSYSFIXED);
	printf("\t  -%c  suppress sorting of system resources\n",
			MSYSNOSORT);
	printf("\t  -%c  suppress exited processes in output\n",
			MSUPEXITS);
	printf("\t  -%c  show limited number of lines for certain resources\n",
			MSYSLIMIT);
	printf("\t  -%c  show individual threads\n", MTHREAD);
	printf("\t  -%c  show average-per-second i.s.o. total values\n\n",
			MAVGVAL);
	printf("\t  -%c  no colors in case of high occupation\n",
			MCOLORS);
	printf("\t  -%c  show general process-info (default)\n",
			MPROCGEN);
	printf("\t  -%c  show memory-related process-info\n",
			MPROCMEM);
	printf("\t  -%c  show disk-related process-info\n",
			MPROCDSK);
	printf("\t  -%c  show network-related process-info\n",
			MPROCNET);
	printf("\t  -%c  show scheduling-related process-info\n",
			MPROCSCH);
	printf("\t  -%c  show various process-info (ppid, user/group, "
	                 "date/time)\n", MPROCVAR);
	printf("\t  -%c  show command line per process\n",
			MPROCARG);
	printf("\t  -%c  show own defined process-info\n",
			MPROCOWN);
	printf("\t  -%c  show cumulated process-info per user\n",
			MCUMUSER);
	printf("\t  -%c  show cumulated process-info per program "
	                "(i.e. same name)\n",
			MCUMPROC);
	printf("\t  -%c  show cumulated process-info per container\n\n",
			MCUMCONT);
	printf("\t  -%c  sort processes in order of cpu-consumption "
	                "(default)\n",
			MSORTCPU);
	printf("\t  -%c  sort processes in order of memory-consumption\n",
			MSORTMEM);
	printf("\t  -%c  sort processes in order of disk-activity\n",
			MSORTDSK);
	printf("\t  -%c  sort processes in order of network-activity\n",
			MSORTNET);
	printf("\t  -%c  sort processes in order of most active resource "
                        "(auto mode)\n",
			MSORTAUTO);
}

/*
** functions to handle a particular tag in the /etc/atoprc and .atoprc file
*/
void
do_username(char *name, char *val)
{
	struct passwd	*pwd;

	strncpy(procsel.username, val, sizeof procsel.username -1);
	procsel.username[sizeof procsel.username -1] = 0;

	if (procsel.username[0])
	{
		regex_t		userregex;
		int		u = 0;

		if (regcomp(&userregex, procsel.username, REG_NOSUB))
		{
			fprintf(stderr,
				"atoprc - %s: invalid regular expression %s\n",
				name, val);
			exit(1);
		}

		while ( (pwd = getpwent()))
		{
			if (regexec(&userregex, pwd->pw_name, 0, NULL, 0))
				continue;

			if (u < MAXUSERSEL-1)
			{
				procsel.userid[u] = pwd->pw_uid;
				u++;
			}
		}
		endpwent();

		procsel.userid[u] = USERSTUB;

		if (u == 0)
		{
			/*
			** possibly a numerical value has been specified
			*/
			if (numeric(procsel.username))
			{
			     procsel.userid[0] = atoi(procsel.username);
			     procsel.userid[1] = USERSTUB;
			}
			else
			{
				fprintf(stderr,
			       		"atoprc - %s: user-names matching %s "
                                        "do not exist\n", name, val);
				exit(1);
			}
		}
	}
	else
	{
		procsel.userid[0] = USERSTUB;
	}
}

void
do_procname(char *name, char *val)
{
	strncpy(procsel.progname, val, sizeof procsel.progname -1);
	procsel.prognamesz = strlen(procsel.progname);

	if (procsel.prognamesz)
	{
		if (regcomp(&procsel.progregex, procsel.progname, REG_NOSUB))
		{
			fprintf(stderr,
				"atoprc - %s: invalid regular expression %s\n",
				name, val);
			exit(1);
		}
	}
}

extern int get_posval(char *name, char *val);


void
do_maxcpu(char *name, char *val)
{
	maxcpulines = get_posval(name, val);
}

void
do_maxdisk(char *name, char *val)
{
	maxdsklines = get_posval(name, val);
}

void
do_maxmdd(char *name, char *val)
{
	maxmddlines = get_posval(name, val);
}

void
do_maxlvm(char *name, char *val)
{
	maxlvmlines = get_posval(name, val);
}

void
do_maxintf(char *name, char *val)
{
	maxintlines = get_posval(name, val);
}

void
do_maxnfsm(char *name, char *val)
{
	maxnfslines = get_posval(name, val);
}

void
do_maxcont(char *name, char *val)
{
	maxcontlines = get_posval(name, val);
}


struct colmap {
	char 	*colname;
	short	colval;
} colormap[] = {
	{ "red",	COLOR_RED,	},
	{ "green",	COLOR_GREEN,	},
	{ "yellow",	COLOR_YELLOW,	},
	{ "blue",	COLOR_BLUE,	},
	{ "magenta",	COLOR_MAGENTA,	},
	{ "cyan",	COLOR_CYAN,	},
	{ "black",	COLOR_BLACK,	},
	{ "white",	COLOR_WHITE,	},
};
static short
modify_color(char *colorname)
{
	int i;

	for (i=0; i < sizeof colormap/sizeof colormap[0]; i++)
	{
		if ( strcmp(colorname, colormap[i].colname) == 0)
			return colormap[i].colval;
	}

	// required color not found
	fprintf(stderr, "atoprc - invalid color used: %s\n", colorname);
	fprintf(stderr, "supported colors:");
	for (i=0; i < sizeof colormap/sizeof colormap[0]; i++)
		fprintf(stderr, " %s", colormap[i].colname);
	fprintf(stderr, "\n");

	exit(1);
}


void
do_colinfo(char *name, char *val)
{
	colorinfo = modify_color(val);
}

void
do_colalmost(char *name, char *val)
{
	coloralmost = modify_color(val);
}

void
do_colcrit(char *name, char *val)
{
	colorcrit = modify_color(val);
}

void
do_colthread(char *name, char *val)
{
	colorthread = modify_color(val);
}

void
do_flags(char *name, char *val)
{
	int	i;

	for (i=0; val[i]; i++)
	{
		switch (val[i])
		{
		   case '-':
			break;

		   case MSORTCPU:
			showorder = MSORTCPU;
			break;

		   case MSORTMEM:
			showorder = MSORTMEM;
			break;

		   case MSORTDSK:
			showorder = MSORTDSK;
			break;

		   case MSORTNET:
			showorder = MSORTNET;
			break;

		   case MSORTAUTO:
			showorder = MSORTAUTO;
			break;

		   case MPROCGEN:
			showtype  = MPROCGEN;
			showorder = MSORTCPU;
			break;

		   case MPROCMEM:
			showtype  = MPROCMEM;
			showorder = MSORTMEM;
			break;

		   case MPROCDSK:
			showtype  = MPROCDSK;
			showorder = MSORTDSK;
			break;

		   case MPROCNET:
			showtype  = MPROCNET;
			showorder = MSORTNET;
			break;

		   case MPROCVAR:
			showtype  = MPROCVAR;
			break;

		   case MPROCSCH:
			showtype  = MPROCSCH;
			showorder = MSORTCPU;
			break;

		   case MPROCARG:
			showtype  = MPROCARG;
			break;

		   case MPROCOWN:
			showtype  = MPROCOWN;
			break;

		   case MCUMUSER:
			showtype  = MCUMUSER;
			break;

		   case MCUMPROC:
			showtype  = MCUMPROC;
			break;

		   case MCUMCONT:
			showtype  = MCUMCONT;
			break;

		   case MALLPROC:
			deviatonly = 0;
			break;

		   case MAVGVAL:
			avgval=1;
			break;

		   case MSYSFIXED:
			fixedhead = 1;
			break;

		   case MSYSNOSORT:
			sysnosort = 1;
			break;

		   case MTHREAD:
			threadview = 1;
			break;

		   case MCOLORS:
			usecolors = 0;
			break;

		   case MCALCPSS:
			calcpss = 1;
			break;

		   case MSUPEXITS:
			suppressexit = 1;
			break;
		}
	}
}
