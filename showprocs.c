/*
** ATOP - System & Process Monitor 
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as cgroupess-level.
**
** This source-file contains the Linux-specific functions to calculate
** figures to be visualized on process/thread level and cgroup level.
** ==========================================================================
** Author:      JC van Winkel - AT Computing, Nijmegen, Holland
** E-mail:      jc@ATComputing.nl
** Date:        November 2009
**
** Process level
** --------------------------------------------------------------------------
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        January 2024
**
** Additions for cgroups
** --------------------------------------------------------------------------
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
#include "cgroups.h"
#include "showgeneric.h"
#include "showlinux.h"

static void	format_bandw(char *, count_t);
static void	gettotwidth(detail_printpair *, int *, int *, int *);
static int 	*getspacings(detail_printpair *);

char *procprt_TID_ae(struct tstat *, int, int);
char *procprt_PID_a(struct tstat *, int, int);
char *procprt_PID_e(struct tstat *, int, int);
char *procprt_PPID_a(struct tstat *, int, int);
char *procprt_PPID_e(struct tstat *, int, int);
char *procprt_VPID_a(struct tstat *, int, int);
char *procprt_VPID_e(struct tstat *, int, int);
char *procprt_CTID_a(struct tstat *, int, int);
char *procprt_CTID_e(struct tstat *, int, int);
char *procprt_CID_a(struct tstat *, int, int);
char *procprt_CID_e(struct tstat *, int, int);
char *procprt_SYSCPU_ae(struct tstat *, int, int);
char *procprt_USRCPU_ae(struct tstat *, int, int);
char *procprt_VGROW_a(struct tstat *, int, int);
char *procprt_VGROW_e(struct tstat *, int, int);
char *procprt_RGROW_a(struct tstat *, int, int);
char *procprt_RGROW_e(struct tstat *, int, int);
char *procprt_MINFLT_ae(struct tstat *, int, int);
char *procprt_MAJFLT_ae(struct tstat *, int, int);
char *procprt_VSTEXT_a(struct tstat *, int, int);
char *procprt_VSTEXT_e(struct tstat *, int, int);
char *procprt_VSIZE_a(struct tstat *, int, int);
char *procprt_VSIZE_e(struct tstat *, int, int);
char *procprt_RSIZE_a(struct tstat *, int, int);
char *procprt_RSIZE_e(struct tstat *, int, int);
char *procprt_PSIZE_a(struct tstat *, int, int);
char *procprt_PSIZE_e(struct tstat *, int, int);
char *procprt_VSLIBS_a(struct tstat *, int, int);
char *procprt_VSLIBS_e(struct tstat *, int, int);
char *procprt_VDATA_a(struct tstat *, int, int);
char *procprt_VDATA_e(struct tstat *, int, int);
char *procprt_VSTACK_a(struct tstat *, int, int);
char *procprt_VSTACK_e(struct tstat *, int, int);
char *procprt_SWAPSZ_a(struct tstat *, int, int);
char *procprt_SWAPSZ_e(struct tstat *, int, int);
char *procprt_LOCKSZ_a(struct tstat *, int, int);
char *procprt_LOCKSZ_e(struct tstat *, int, int);
char *procprt_CMD_a(struct tstat *, int, int);
char *procprt_CMD_e(struct tstat *, int, int);
char *procprt_RUID_ae(struct tstat *, int, int);
char *procprt_EUID_a(struct tstat *, int, int);
char *procprt_EUID_e(struct tstat *, int, int);
char *procprt_SUID_a(struct tstat *, int, int);
char *procprt_SUID_e(struct tstat *, int, int);
char *procprt_FSUID_a(struct tstat *, int, int);
char *procprt_FSUID_e(struct tstat *, int, int);
char *procprt_RGID_ae(struct tstat *, int, int);
char *procprt_EGID_a(struct tstat *, int, int);
char *procprt_EGID_e(struct tstat *, int, int);
char *procprt_SGID_a(struct tstat *, int, int);
char *procprt_SGID_e(struct tstat *, int, int);
char *procprt_FSGID_a(struct tstat *, int, int);
char *procprt_FSGID_e(struct tstat *, int, int);
char *procprt_STDATE_ae(struct tstat *, int, int);
char *procprt_STTIME_ae(struct tstat *, int, int);
char *procprt_ENDATE_a(struct tstat *, int, int);
char *procprt_ENDATE_e(struct tstat *, int, int);
char *procprt_ENTIME_a(struct tstat *, int, int);
char *procprt_ENTIME_e(struct tstat *, int, int);
char *procprt_THR_a(struct tstat *, int, int);
char *procprt_THR_e(struct tstat *, int, int);
char *procprt_TRUN_a(struct tstat *, int, int);
char *procprt_TRUN_e(struct tstat *, int, int);
char *procprt_TSLPI_a(struct tstat *, int, int);
char *procprt_TSLPI_e(struct tstat *, int, int);
char *procprt_TSLPU_a(struct tstat *, int, int);
char *procprt_TSLPU_e(struct tstat *, int, int);
char *procprt_TIDLE_a(struct tstat *, int, int);
char *procprt_TIDLE_e(struct tstat *, int, int);
char *procprt_POLI_a(struct tstat *, int, int);
char *procprt_POLI_e(struct tstat *, int, int);
char *procprt_NICE_a(struct tstat *, int, int);
char *procprt_NICE_e(struct tstat *, int, int);
char *procprt_PRI_a(struct tstat *, int, int);
char *procprt_PRI_e(struct tstat *, int, int);
char *procprt_RTPR_a(struct tstat *, int, int);
char *procprt_RTPR_e(struct tstat *, int, int);
char *procprt_CURCPU_a(struct tstat *, int, int);
char *procprt_CURCPU_e(struct tstat *, int, int);
char *procprt_ST_a(struct tstat *, int, int);
char *procprt_ST_e(struct tstat *, int, int);
char *procprt_EXC_a(struct tstat *, int, int);
char *procprt_EXC_e(struct tstat *, int, int);
char *procprt_S_a(struct tstat *, int, int);
char *procprt_S_e(struct tstat *, int, int);
char *procprt_COMMAND_LINE_ae(struct tstat *, int, int);
char *procprt_NPROCS_ae(struct tstat *, int, int);
char *procprt_RDDSK_a(struct tstat *, int, int);
char *procprt_RDDSK_e(struct tstat *, int, int);
char *procprt_WRDSK_a(struct tstat *, int, int);
char *procprt_WRDSK_e(struct tstat *, int, int);
char *procprt_CWRDSK_a(struct tstat *, int, int);
char *procprt_WCANCEL_a(struct tstat *, int, int);
char *procprt_WCANCEL_e(struct tstat *, int, int);
char *procprt_BANDWI_a(struct tstat *, int, int);
char *procprt_BANDWI_e(struct tstat *, int, int);
char *procprt_BANDWO_a(struct tstat *, int, int);
char *procprt_BANDWO_e(struct tstat *, int, int);
char *procprt_GPULIST_ae(struct tstat *, int, int);
char *procprt_GPUMEMNOW_ae(struct tstat *, int, int);
char *procprt_GPUMEMAVG_ae(struct tstat *, int, int);
char *procprt_GPUGPUBUSY_ae(struct tstat *, int, int);
char *procprt_GPUMEMBUSY_ae(struct tstat *, int, int);
char *procprt_WCHAN_a(struct tstat *, int, int);
char *procprt_WCHAN_e(struct tstat *, int, int);
char *procprt_RUNDELAY_a(struct tstat *, int, int);
char *procprt_RUNDELAY_e(struct tstat *, int, int);
char *procprt_BLKDELAY_a(struct tstat *, int, int);
char *procprt_BLKDELAY_e(struct tstat *, int, int);
char *procprt_NVCSW_a(struct tstat *, int, int);
char *procprt_NVCSW_e(struct tstat *, int, int);
char *procprt_NIVCSW_a(struct tstat *, int, int);
char *procprt_NIVCSW_e(struct tstat *, int, int);
char *procprt_SORTITEM_ae(struct tstat *, int, int);

char *cgroup_CGROUP_PATH(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRNPROCS(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRNPROCSB(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRCPUBUSY(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRCPUPSI(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRCPUWGT(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRCPUMAX(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRMEMORY(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRMEMPSI(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRMEMMAX(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRSWPMAX(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRDISKIO(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRDSKPSI(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRDSKWGT(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRPID(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);
char *cgroup_CGRCMD(struct cgchainer *, struct tstat *,
				int, int, count_t, int, int *);


static char     *columnhead[] = {
	[MSORTCPU]= "CPU", [MSORTMEM]= "MEM",
	[MSORTDSK]= "DSK", [MSORTNET]= "NET",
	[MSORTGPU]= "GPU",
};

/***************************************************************/
static int *colspacings;     // ugly static var, 
                             // but saves a lot of recomputations
                             // points to table with intercolumn 
                             // spacings
			    
static detail_printpair newelems[MAXITEMS];
                             // ugly static var,
                             // but saves a lot of recomputations
                             // contains the actual list of items to
                             // be printed
/***************************************************************/
/*
 * gettotwidth: calculate the sum of widths and number of columns
 * Also copys the detail_printpair elements to the static array newelems
 * for later removal of lower priority elements.
 * Params:
 * elemptr: the array of what to print
 * nitems: (ref) returns the number of printitems in the array
 * sumwidth: (ref) returns the total width of the printitems in the array
 * varwidth: (ref) returns the number of variable width items in the array
 */
static void
gettotwidth(detail_printpair *elemptr, int *nitems, int *sumwidth, int* varwidth) 
{
        int i;
        int col;
        int varw=0;

        for (i=0, col=0; elemptr[i].pf!=0; ++i) 
        {
                col += (elemptr[i].pf->varwidth ? 0 : elemptr[i].pf->width);
                varw += elemptr[i].pf->varwidth;
                newelems[i]=elemptr[i];    // copy element
        }
        newelems[i].pf=0;
        *nitems=i;
        *sumwidth=col;
        *varwidth=varw;
}



/***************************************************************/
/*
 * getspacings: determine how much extra space there is for
 * inter-column space.
 * returns an int array this number of spaces to add after each column
 * also removes items from the newelems array if the available width
 * is lower than what is needed.  The lowest priority columns are
 * removed first.
 *
 * Note: this function is only to be called when screen is true.
 */
static int *
getspacings(detail_printpair *elemptr) 
{
        static int spacings[MAXITEMS];

        int col=0;
        int nitems;
        int varwidth=0;
        int j;
        int maxw=screen ? COLS : linelen;    // for non screen: 80 columns max

        // get width etc; copy elemptr array to static newelms
        gettotwidth(elemptr, &nitems, &col, &varwidth);

        /* cases:
         *   1) nitems==1: just one column, no spacing needed.  Done
         *
         *   2) total width is more than COLS: remove low prio columns
         *      2a)  a varwidth column: no spacing needed
         *      2b)  total width is less than COLS: compute inter spacing
         */

        if (nitems==1)          // no inter column spacing if 1 column
        {
                spacings[0]=0;
                return spacings;
        }

        // Check if available width is less than required.
        // If so, delete columns to make things fit
        // space required:
        // width + (nitems-1) * 1 space + 12 for a varwidth column.
        while (col + nitems-1+ 12*varwidth > maxw)  
        {    
                int lowestprio=999999;
                int lowestprio_index=-1;
                int i;
                for (i=0; i<nitems; ++i) 
                {
                        if (newelems[i].prio < lowestprio) 
                        {
                                lowestprio=newelems[i].prio;
                                lowestprio_index=i;
                        }
                }
                
                // lowest priority item found, remove from newelems;
                col -= newelems[lowestprio_index].pf->width;
                varwidth -= newelems[lowestprio_index].pf->varwidth;
                memmove(newelems+lowestprio_index, 
                        newelems+lowestprio_index+1, 
                        (nitems-lowestprio_index)* sizeof(detail_printpair));   
                       // also copies final 0 entry
                nitems--;
        }


        /* if there is a var width column, handle that separately */
        if (varwidth) 
        {
                for (j=0; j<nitems; ++j) 
                {
                        spacings[j]=1;
                        if (elemptr[j].pf->varwidth)
                        {
                                elemptr[j].pf->width=maxw-col-(nitems-1);
                                // only nitems-1 in-between spaces
                                // needed
                        }

                }
                return spacings;
        }

        // avoid division by 0
        if (nitems==1)
        {
                spacings[0]=0;
                return spacings;
        }

        /* fixed columns, spread whitespace over columns */
        double over=(0.0+maxw-col)/(nitems-1);
        double todo=over;

        for (j=0; j<nitems-1; ++j)   // last column gets no space appended
        {
                spacings[j]=(int)todo+0.5;
                todo-=spacings[j];
                todo+=over;
        }
        spacings[j]=0;
        return spacings;
}


/*
 * showprochead: show header line for processes.
 * if in interactive mode, also add a page numer
 * if in interactive mode, columns are aligned to fill out rows
 */
void
showprochead(detail_printpair* elemptr, int curlist, int totlist, 
                  char showorder, char autosort) 
{
        detail_printpair curelem;

        char *chead="";
        char *autoindic="";
        int order=showorder;
        int col=0;
        int align;
        char pagindic[10];
        int pagindiclen;
        int n=0;
        int bufsz;
        int maxw=screen ? COLS : linelen;    // for non screen: 80 columns max

        colspacings=getspacings(elemptr);
        bufsz=maxw+1;

        elemptr=newelems;     // point to adjusted array
        char buf[bufsz+2];    // long live dynamically sized auto arrays...
        
        if (!screen) 
                printg("\n");

        while ((curelem=*elemptr).pf!=0) 
        {
		int widen = 0;

                if (curelem.pf->head==0)     // empty header==special: SORTITEM
                {
                        chead     = columnhead[order];
                        autoindic = autosort ? "A" : " ";
			widen     = procprt_SORTITEM.width-3;
                } 
                else 
                {
                        chead=curelem.pf->head;
                        autoindic="";
                }

                if (screen)
                {
                        col += sprintf(buf+col, "%*s%s%*s",
				widen, autoindic, chead, colspacings[n], "");
                }
                else
                {
                        col += sprintf(buf+col, "%s%s ", autoindic, chead);
                }
                              
                elemptr++;
                n++;
        }

        if (screen)   // add page number, eat from last header if needed...
        {
                pagindiclen=sprintf(pagindic,"%d/%d", curlist, totlist);
                align=COLS-col-pagindiclen;    // extra spaces needed
            
                if (align >= 0)     // align by adding spaces
                {
                        sprintf(buf+col, "%*s", align+pagindiclen, pagindic);
                }
                else if (col+align >= 0)
                {    // align by removing from the right
                        sprintf(buf+col+align, "%s", pagindic);
                }
        }

        printg("%s", buf);

        if (!screen) 
                printg("\n");
}



/***************************************************************/
/*
 * showprocline: show line for processes.
 * if in interactive mode, columns are aligned to fill out rows
 * params:
 *     elemptr: pointer to array of print definition structs ptrs
 *     curstat: the process to print
 *     perc: the sort order used
 *     nsecs: number of seconds elapsed between previous and this sample
 *     avgval: is averaging out per second needed?
 */
void
showprocline(detail_printpair* elemptr, struct tstat *curstat, 
                            double perc, int nsecs, int avgval) 
{
        detail_printpair curelem;
        
        elemptr=newelems;      // point to static array
        int n=0;

	if (screen && threadview)
	{
		if (usecolors && !curstat->gen.isproc)
		{
			attron(COLOR_PAIR(FGCOLORTHR));
		}
		else
		{
			if (!usecolors && curstat->gen.isproc)
				attron(A_BOLD);
		}
	}

        while ((curelem=*elemptr).pf!=0) 
        {
                // what to print?  SORTITEM, or active process or
                // exited process?

                if (curelem.pf->head==0)                // empty string=sortitem
                {
                        printg("%*.0lf%%", procprt_SORTITEM.width-1, perc);
                }
                else if (curstat->gen.state != 'E')  // active process
                {
                        printg("%s", curelem.pf->doactiveconvert(curstat, 
                                                        avgval, nsecs));
                }
                else                                 // exited process
                {
                        printg("%s", curelem.pf->doexitconvert(curstat, 
                                                        avgval, nsecs));
                }

                if (screen)
                {
                        printg("%*s",colspacings[n], "");
                }
                else
                {
                        printg(" ");
                }

                elemptr++;
                n++;
        }

	if (screen && threadview)
	{
		if (usecolors && !curstat->gen.isproc)
		{
			attroff(COLOR_PAIR(FGCOLORTHR));
		}
		else
		{
			if (!usecolors && curstat->gen.isproc)
				attroff(A_BOLD);
		}
	}

        if (!screen) 
        {
                printg("\n");
        }
}


/*******************************************************************/
/* PROCESS PRINT FUNCTIONS */
/***************************************************************/
char *
procprt_NOTAVAIL_4(struct tstat *curstat, int avgval, int nsecs)
{
        return "   ?";
}

char *
procprt_NOTAVAIL_5(struct tstat *curstat, int avgval, int nsecs)
{
        return "    ?";
}

char *
procprt_NOTAVAIL_6(struct tstat *curstat, int avgval, int nsecs)
{
        return "     ?";
}

char *
procprt_NOTAVAIL_7(struct tstat *curstat, int avgval, int nsecs)
{
        return "      ?";
}
/***************************************************************/
char *
procprt_TID_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.isproc)
        	sprintf(buf, "%*s", procprt_TID.width, "-");
	else
        	sprintf(buf, "%*d", procprt_TID.width, curstat->gen.pid);
        return buf;
}

detail_printdef procprt_TID = 
   { "TID", "TID", procprt_TID_ae, procprt_TID_ae, ' ', 5}; //DYNAMIC WIDTH!
/***************************************************************/
char *
procprt_PID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        sprintf(buf, "%*d", procprt_PID.width, curstat->gen.tgid);
        return buf;
}

char *
procprt_PID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        if (curstat->gen.pid == 0)
        	sprintf(buf, "%*s", procprt_PID.width, "?");
	else
        	sprintf(buf, "%*d", procprt_PID.width, curstat->gen.tgid);
        return buf;
}

detail_printdef procprt_PID = 
   { "PID", "PID", procprt_PID_a, procprt_PID_e, ' ', 5}; //DYNAMIC WIDTH!
/***************************************************************/
char *
procprt_PPID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        sprintf(buf, "%*d", procprt_PPID.width, curstat->gen.ppid);
        return buf;
}

char *
procprt_PPID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.ppid)
        	sprintf(buf, "%*d", procprt_PPID.width, curstat->gen.ppid);
	else
		sprintf(buf, "%*s", procprt_PPID.width, "-");
        return buf;
}

detail_printdef procprt_PPID = 
   { "PPID", "PPID", procprt_PPID_a, procprt_PPID_e, ' ', 5}; //DYNAMIC WIDTH!
/***************************************************************/
char *
procprt_VPID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        sprintf(buf, "%*d", procprt_VPID.width, curstat->gen.vpid);
        return buf;
}

char *
procprt_VPID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	sprintf(buf, "%*s", procprt_VPID.width, "-");
        return buf;
}

detail_printdef procprt_VPID = 
   { "VPID", "VPID", procprt_VPID_a, procprt_VPID_e, ' ', 5}; //DYNAMIC WIDTH!
/***************************************************************/
char *
procprt_CTID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[32];

        sprintf(buf, "%5d", curstat->gen.ctid);
        return buf;
}

char *
procprt_CTID_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    -";
}

detail_printdef procprt_CTID = 
   { " CTID", "CTID", procprt_CTID_a, procprt_CTID_e, ' ', 5};
/***************************************************************/
char *
procprt_CID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.utsname[0])
        	sprintf(buf, "%-15s", curstat->gen.utsname);
	else
        	sprintf(buf, "%-15s", "host-----------");

        return buf;
}

char *
procprt_CID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.utsname[0])
        	sprintf(buf, "%-15s", curstat->gen.utsname);
	else
        	sprintf(buf, "%-15s", "?");

        return buf;
}

detail_printdef procprt_CID = 
   { "CID/POD        ", "CID", procprt_CID_a, procprt_CID_e, ' ', 15};
/***************************************************************/
char *
procprt_SYSCPU_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2cpustr(curstat->cpu.stime*1000/hertz, buf);
        return buf;
}

detail_printdef procprt_SYSCPU = 
   { "SYSCPU", "SYSCPU", procprt_SYSCPU_ae, procprt_SYSCPU_ae, ' ', 6};
/***************************************************************/
char *
procprt_USRCPU_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2cpustr(curstat->cpu.utime*1000/hertz, buf);
        return buf;
}

detail_printdef procprt_USRCPU = 
   { "USRCPU", "USRCPU", procprt_USRCPU_ae, procprt_USRCPU_ae, ' ', 6};
/***************************************************************/
char *
procprt_VGROW_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vgrow*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_VGROW_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_VGROW = 
   { " VGROW", "VGROW", procprt_VGROW_a, procprt_VGROW_e, ' ', 6};
/***************************************************************/
char *
procprt_RGROW_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.rgrow*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_RGROW_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_RGROW = 
   { " RGROW", "RGROW", procprt_RGROW_a, procprt_RGROW_e, ' ', 6};
/***************************************************************/
char *
procprt_MINFLT_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2valstr(curstat->mem.minflt, buf, 6, avgval, nsecs);
        return buf;
}

detail_printdef procprt_MINFLT = 
   { "MINFLT", "MINFLT", procprt_MINFLT_ae, procprt_MINFLT_ae, ' ', 6};
/***************************************************************/
char *
procprt_MAJFLT_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2valstr(curstat->mem.majflt, buf, 6, avgval, nsecs);
        return buf;
}

detail_printdef procprt_MAJFLT = 
   { "MAJFLT", "MAJFLT", procprt_MAJFLT_ae, procprt_MAJFLT_ae, ' ', 6};
/***************************************************************/
char *
procprt_VSTEXT_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vexec*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_VSTEXT_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_VSTEXT = 
   { "VSTEXT", "VSTEXT", procprt_VSTEXT_a, procprt_VSTEXT_e, ' ', 6};
/***************************************************************/
char *
procprt_VSIZE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vmem*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_VSIZE_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_VSIZE = 
   { " VSIZE", "VSIZE", procprt_VSIZE_a, procprt_VSIZE_e, ' ', 6};
/***************************************************************/
char *
procprt_RSIZE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.rmem*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_RSIZE_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_RSIZE = 
   { " RSIZE", "RSIZE", procprt_RSIZE_a, procprt_RSIZE_e, ' ', 6};
/***************************************************************/
char *
procprt_PSIZE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	if (curstat->mem.pmem == (unsigned long long)-1LL)	
        	return "    ?K";

        val2memstr(curstat->mem.pmem*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_PSIZE_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_PSIZE = 
   { " PSIZE", "PSIZE", procprt_PSIZE_a, procprt_PSIZE_e, ' ', 6};
/***************************************************************/
char *
procprt_VSLIBS_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vlibs*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_VSLIBS_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_VSLIBS = 
   { "VSLIBS", "VSLIBS", procprt_VSLIBS_a, procprt_VSLIBS_e, ' ', 6};
/***************************************************************/
char *
procprt_VDATA_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vdata*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_VDATA_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_VDATA = 
   { " VDATA", "VDATA", procprt_VDATA_a, procprt_VDATA_e, ' ', 6};
/***************************************************************/
char *
procprt_VSTACK_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vstack*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_VSTACK_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_VSTACK = 
   { "VSTACK", "VSTACK", procprt_VSTACK_a, procprt_VSTACK_e, ' ', 6};
/***************************************************************/
char *
procprt_SWAPSZ_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vswap*1024, buf, BFORMAT, 0, 0);
        return buf;
}

char *
procprt_SWAPSZ_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_SWAPSZ = 
   { "SWAPSZ", "SWAPSZ", procprt_SWAPSZ_a, procprt_SWAPSZ_e, ' ', 6};
/***************************************************************/
char *
procprt_LOCKSZ_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2memstr(curstat->mem.vlock*1024, buf, KBFORMAT, 0, 0);
        return buf;
}

char *
procprt_LOCKSZ_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0K";
}

detail_printdef procprt_LOCKSZ = 
   { "LOCKSZ", "LOCKSZ", procprt_LOCKSZ_a, procprt_LOCKSZ_e, ' ', 6};
/***************************************************************/
char *
procprt_CMD_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%-14.14s", curstat->gen.name);
        return buf;
}

char *
procprt_CMD_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15]="<";
        char        helpbuf[15];

        sprintf(helpbuf, "<%.12s>",  curstat->gen.name);
        sprintf(buf,     "%-14.14s", helpbuf);
        return buf;
}

detail_printdef procprt_CMD = 
   { "CMD           ", "CMD", procprt_CMD_a, procprt_CMD_e, ' ', 14};
/***************************************************************/
char *
procprt_RUID_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];
        struct passwd   *pwd;

        if ( (pwd = getpwuid(curstat->gen.ruid)) )
        {
                        sprintf(buf, "%-8.8s", pwd->pw_name);
        } 
        else 
        {
                        snprintf(buf, sizeof buf, "%-8d", curstat->gen.ruid);
        }
        return buf;
}

detail_printdef procprt_RUID = 
   { "RUID    ", "RUID", procprt_RUID_ae, procprt_RUID_ae, ' ', 8};
/***************************************************************/
char *
procprt_EUID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];
        struct passwd   *pwd;

        if ( (pwd = getpwuid(curstat->gen.euid)) )
        {
                        sprintf(buf, "%-8.8s", pwd->pw_name);
        } 
        else 
        {
                        snprintf(buf, sizeof buf, "%-8d", curstat->gen.euid);
        }
        return buf;
}

char *
procprt_EUID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

detail_printdef procprt_EUID = 
   { "EUID    ", "EUID", procprt_EUID_a, procprt_EUID_e, ' ', 8};
/***************************************************************/
char *
procprt_SUID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];
        struct passwd   *pwd;

        if ( (pwd = getpwuid(curstat->gen.suid)) )
        {
                        sprintf(buf, "%-8.8s", pwd->pw_name);
        } 
        else 
        {
                        snprintf(buf, sizeof buf, "%-8d", curstat->gen.suid);
        }
        return buf;
}

char *
procprt_SUID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

detail_printdef procprt_SUID = 
   { "SUID    ", "SUID", procprt_SUID_a, procprt_SUID_e, ' ', 8};
/***************************************************************/
char *
procprt_FSUID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];
        struct passwd   *pwd;

        if ( (pwd = getpwuid(curstat->gen.fsuid)) )
        {
                        sprintf(buf, "%-8.8s", pwd->pw_name);
        } 
        else 
        {
                        snprintf(buf, sizeof buf, "%-8d", curstat->gen.fsuid);
        }
        return buf;
}

char *
procprt_FSUID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

detail_printdef procprt_FSUID = 
   { "FSUID   ", "FSUID", procprt_FSUID_a, procprt_FSUID_e, ' ', 8};
/***************************************************************/
char *
procprt_RGID_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        struct group    *grp;
        char *groupname;
        char grname[16];

        if ( (grp = getgrgid(curstat->gen.rgid)) )
        {
                        groupname = grp->gr_name;
        }
        else
        {
                        snprintf(grname, sizeof grname, "%d",curstat->gen.rgid);
                        groupname = grname;
        }

        sprintf(buf, "%-8.8s", groupname);
        return buf;
}

detail_printdef procprt_RGID = 
   { "RGID    ", "RGID", procprt_RGID_ae, procprt_RGID_ae, ' ', 8};
/***************************************************************/
char *
procprt_EGID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        struct group    *grp;
        char *groupname;
        char grname[16];

        if ( (grp = getgrgid(curstat->gen.egid)) )
        {
                        groupname = grp->gr_name;
        }
        else
        {
                        snprintf(grname, sizeof grname, "%d",curstat->gen.egid);
                        groupname = grname;
        }

        sprintf(buf, "%-8.8s", groupname);
        return buf;
}

char *
procprt_EGID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

detail_printdef procprt_EGID = 
   { "EGID    ", "EGID", procprt_EGID_a, procprt_EGID_e, ' ', 8};
/***************************************************************/
char *
procprt_SGID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        struct group    *grp;
        char *groupname;
        char grname[16];

        if ( (grp = getgrgid(curstat->gen.sgid)) )
        {
                        groupname = grp->gr_name;
        }
        else
        {
                        snprintf(grname, sizeof grname, "%d",curstat->gen.sgid);
                        groupname = grname;
        }

        sprintf(buf, "%-8.8s", groupname);
        return buf;
}

char *
procprt_SGID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

detail_printdef procprt_SGID = 
   { "SGID    ", "SGID", procprt_SGID_a, procprt_SGID_e, ' ', 8};
/***************************************************************/
char *
procprt_FSGID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        struct group    *grp;
        char *groupname;
        char grname[16];

        if ( (grp = getgrgid(curstat->gen.fsgid)) )
        {
                        groupname = grp->gr_name;
        }
        else
        {
                        snprintf(grname, sizeof grname,"%d",curstat->gen.fsgid);
                        groupname = grname;
        }

        sprintf(buf, "%-8.8s", groupname);
        return buf;
}

char *
procprt_FSGID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

detail_printdef procprt_FSGID = 
   { "FSGID   ", "FSGID", procprt_FSGID_a, procprt_FSGID_e, ' ', 8};
/***************************************************************/
char *
procprt_STDATE_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[11];

        convdate(curstat->gen.btime, buf);
        return buf;
}

detail_printdef procprt_STDATE = 
   { "  STDATE  ", "STDATE", procprt_STDATE_ae, procprt_STDATE_ae, ' ', 10};
/***************************************************************/
char *
procprt_STTIME_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];

        convtime(curstat->gen.btime, buf);
        return buf;
}

detail_printdef procprt_STTIME = 
   { " STTIME ", "STTIME", procprt_STTIME_ae, procprt_STTIME_ae, ' ', 8};
/***************************************************************/
char *
procprt_ENDATE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[11];

	strcpy(buf, "  active  ");

        return buf;
}

char *
procprt_ENDATE_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[11];

        convdate(curstat->gen.btime + curstat->gen.elaps/hertz, buf);

        return buf;
}

detail_printdef procprt_ENDATE = 
   { "  ENDATE  ", "ENDATE", procprt_ENDATE_a, procprt_ENDATE_e, ' ', 10};
/***************************************************************/
char *
procprt_ENTIME_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];

	strcpy(buf, " active ");

        return buf;
}

char *
procprt_ENTIME_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];

        convtime(curstat->gen.btime + curstat->gen.elaps/hertz, buf);

        return buf;
}

detail_printdef procprt_ENTIME = 
   { " ENTIME ", "ENTIME", procprt_ENTIME_a, procprt_ENTIME_e, ' ', 8};
/***************************************************************/
char *
procprt_THR_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%4d", curstat->gen.nthr);
        return buf;
}

char *
procprt_THR_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   0";
}

detail_printdef procprt_THR = 
   { " THR", "THR", procprt_THR_a, procprt_THR_e, ' ', 4};
/***************************************************************/
char *
procprt_TRUN_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%4d", curstat->gen.nthrrun);
        return buf;
}

char *
procprt_TRUN_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   0";
}

detail_printdef procprt_TRUN = 
   { "TRUN", "TRUN", procprt_TRUN_a, procprt_TRUN_e, ' ', 4};
/***************************************************************/
char *
procprt_TSLPI_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%5d", curstat->gen.nthrslpi);
        return buf;
}

char *
procprt_TSLPI_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0";
}

detail_printdef procprt_TSLPI = 
   { "TSLPI", "TSLPI", procprt_TSLPI_a, procprt_TSLPI_e, ' ', 5};
/***************************************************************/
char *
procprt_TSLPU_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%5d", curstat->gen.nthrslpu);
        return buf;
}

char *
procprt_TSLPU_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0";
}

detail_printdef procprt_TSLPU = 
   { "TSLPU", "TSLPU", procprt_TSLPU_a, procprt_TSLPU_e, ' ', 5};
/***************************************************************/
char *
procprt_TIDLE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%5d", curstat->gen.nthridle);
        return buf;
}

char *
procprt_TIDLE_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0";
}

detail_printdef procprt_TIDLE = 
   { "TIDLE", "TIDLE", procprt_TIDLE_a, procprt_TIDLE_e, ' ', 5};
/***************************************************************/
#define SCHED_NORMAL	0
#define SCHED_FIFO	1
#define SCHED_RR	2
#define SCHED_BATCH	3
#define SCHED_ISO	4
#define SCHED_IDLE	5
#define SCHED_DEADLINE	6

char *
procprt_POLI_a(struct tstat *curstat, int avgval, int nsecs)
{
        switch (curstat->cpu.policy)
        {
                case SCHED_NORMAL:
                        return "norm";
                        break;
                case SCHED_FIFO:
                        return "fifo";
                        break;
                case SCHED_RR:
                        return "rr  ";
                        break;
                case SCHED_BATCH:
                        return "btch";
                        break;
                case SCHED_ISO:
                        return "iso ";
                        break;
                case SCHED_IDLE:
                        return "idle";
                        break;
                case SCHED_DEADLINE:
                        return "dead";
                        break;
        }
        return "?   ";
}

char *
procprt_POLI_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "-   ";
}

detail_printdef procprt_POLI = 
   { "POLI", "POLI", procprt_POLI_a, procprt_POLI_e, ' ', 4};
/***************************************************************/
char *
procprt_NICE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%4d", curstat->cpu.nice);
        return buf;
}

char *
procprt_NICE_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   -";
}

detail_printdef procprt_NICE = 
   { "NICE", "NICE", procprt_NICE_a, procprt_NICE_e, ' ', 4};
/***************************************************************/
char *
procprt_PRI_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%3d", curstat->cpu.prio);
        return buf;
}

char *
procprt_PRI_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "  -";
}

detail_printdef procprt_PRI = 
   { "PRI", "PRI", procprt_PRI_a, procprt_PRI_e, ' ', 3};
/***************************************************************/
char *
procprt_RTPR_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%4d", curstat->cpu.rtprio);
        return buf;
}

char *
procprt_RTPR_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   -";
}

detail_printdef procprt_RTPR = 
   { "RTPR", "RTPR", procprt_RTPR_a, procprt_RTPR_e, ' ', 4};
/***************************************************************/
char *
procprt_CURCPU_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        sprintf(buf, "%5d", curstat->cpu.curcpu);
        return buf;
}

char *
procprt_CURCPU_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    -";
}

detail_printdef procprt_CURCPU = 
   { "CPUNR", "CPUNR", procprt_CURCPU_a, procprt_CURCPU_e, ' ', 5};
/***************************************************************/
char *
procprt_ST_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[3]="--";
        if (curstat->gen.excode & ~(INT_MAX))
        {
                buf[0]='N';
        } 
        else
        { 
                buf[0]='-';
        }
        return buf;
}

char *
procprt_ST_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[3];
        if (curstat->gen.excode & ~(INT_MAX))
        {
                buf[0]='N';
        } 
        else 
        { 
                buf[0]='-';
        }
        if (curstat->gen.excode & 0xff) 
        {
                if (curstat->gen.excode & 0x80)
                        buf[1] = 'C';
                else
                        buf[1] = 'S';
        } 
        else 
        {
                buf[1] = 'E';
        }
        return buf;
}

detail_printdef procprt_ST = 
   { "ST", "ST", procprt_ST_a, procprt_ST_e, ' ', 2};
/***************************************************************/
char *
procprt_EXC_a(struct tstat *curstat, int avgval, int nsecs)
{
        return "  -";
}

char *
procprt_EXC_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[4];


        sprintf(buf, "%3d", 
                 curstat->gen.excode & 0xff ?
                          curstat->gen.excode & 0x7f : 
                          (curstat->gen.excode>>8) & 0xff);
        return buf;
}


detail_printdef procprt_EXC = 
   { "EXC", "EXC", procprt_EXC_a, procprt_EXC_e, ' ', 3};
/***************************************************************/
char *
procprt_S_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[2]="E";

        buf[0]=curstat->gen.state;
        return buf;
}

char *
procprt_S_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "E";

}

detail_printdef procprt_S = 
   { "S", "S", procprt_S_a, procprt_S_e, ' ', 1};

/***************************************************************/
char *
procprt_COMMAND_LINE_ae(struct tstat *curstat, int avgval, int nsecs)
{
        extern detail_printdef procprt_COMMAND_LINE;
        extern int	startoffset;	// influenced by -> and <- keys

        static char	buf[CMDLEN+1];

	char	*pline     = curstat->gen.cmdline[0] ?
		                curstat->gen.cmdline : curstat->gen.name;

        int 	curwidth   = procprt_COMMAND_LINE.width <= CMDLEN ?
				procprt_COMMAND_LINE.width : CMDLEN;

        int 	cmdlen     = strlen(pline);
        int 	curoffset  = startoffset <= cmdlen ? startoffset : cmdlen;

        if (screen) 
                sprintf(buf, "%-*.*s", curwidth, curwidth, pline+curoffset);
        else
                sprintf(buf, "%.*s", CMDLEN, pline+curoffset);

        return buf;
}

detail_printdef procprt_COMMAND_LINE = 
       { "COMMAND-LINE (horizontal scroll with <- and -> keys)",
	"COMMAND-LINE", 
        procprt_COMMAND_LINE_ae, procprt_COMMAND_LINE_ae, ' ', 0, 1};
/***************************************************************/
char *
procprt_NPROCS_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2valstr(curstat->gen.pid, buf, 6, 0, 0); // pid abused as proc counter
        return buf;
}

detail_printdef procprt_NPROCS = 
   { "NPROCS", "NPROCS", procprt_NPROCS_ae, procprt_NPROCS_ae, ' ', 6};
/***************************************************************/
char *
procprt_RDDSK_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	if (supportflags & IOSTAT)
        	val2memstr(curstat->dsk.rsz*512, buf, BFORMAT, avgval, nsecs);
	else
		strcpy(buf, "nopriv");

        return buf;
}

char *
procprt_RDDSK_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "     -";
}

detail_printdef procprt_RDDSK = 
   { " RDDSK", "RDDSK", procprt_RDDSK_a, procprt_RDDSK_e, ' ', 6};
/***************************************************************/
char *
procprt_WRDSK_a(struct tstat *curstat, int avgval, int nsecs) 
{
        static char buf[10];

	if (supportflags & IOSTAT)
        	val2memstr(curstat->dsk.wsz*512, buf, BFORMAT, avgval, nsecs);
	else
		strcpy(buf, "nopriv");

        return buf;
}

char *
procprt_WRDSK_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "     -";
}

detail_printdef procprt_WRDSK = 
   { " WRDSK", "WRDSK", procprt_WRDSK_a, procprt_WRDSK_e, ' ', 6};
/***************************************************************/
char *
procprt_CWRDSK_a(struct tstat *curstat, int avgval, int nsecs) 
{
	count_t nett_wsz;
        static char buf[10];

	if (curstat->dsk.wsz > curstat->dsk.cwsz)
		nett_wsz = curstat->dsk.wsz - curstat->dsk.cwsz;
	else
		nett_wsz = 0;

        val2memstr(nett_wsz*512, buf, BFORMAT, avgval, nsecs);

        return buf;
}

detail_printdef procprt_CWRDSK = 
   {" WRDSK", "CWRDSK", procprt_CWRDSK_a, procprt_WRDSK_e, ' ', 6};
/***************************************************************/
char *
procprt_WCANCEL_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	if (supportflags & IOSTAT)
        	val2memstr(curstat->dsk.cwsz*512, buf, BFORMAT, avgval, nsecs);
	else
		strcpy(buf, "nopriv");

        return buf;
}

char *
procprt_WCANCEL_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "     -";
}

detail_printdef procprt_WCANCEL = 
   {"WCANCL", "WCANCL", procprt_WCANCEL_a, procprt_WCANCEL_e, ' ', 6};
/***************************************************************/
char *
procprt_TCPRCV_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        val2valstr(curstat->net.tcprcv, buf, 6, avgval, nsecs);

        return buf;
}

char *
procprt_TCPRCV_e(struct tstat *curstat, int avgval, int nsecs) 
{      
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	val2valstr(curstat->net.tcprcv, buf, 6, avgval, nsecs);

        	return buf;
	}
	else
        	return "     -";
}


detail_printdef procprt_TCPRCV = 
   { "TCPRCV", "TCPRCV", procprt_TCPRCV_a, procprt_TCPRCV_e, ' ', 6};
/***************************************************************/
char *
procprt_TCPRASZ_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        int avgtcpr = curstat->net.tcprcv ?
                                  curstat->net.tcprsz / curstat->net.tcprcv : 0;

        val2valstr(avgtcpr, buf, 7, 0, 0);
        return buf;
}

char *
procprt_TCPRASZ_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	int avgtcpr = curstat->net.tcprcv ?
                                  curstat->net.tcprsz / curstat->net.tcprcv : 0;

        	val2valstr(avgtcpr, buf, 7, 0, 0);
	        return buf;
	}
	else
        	return "      -";
}

detail_printdef procprt_TCPRASZ = 
   { "TCPRASZ", "TCPRASZ", procprt_TCPRASZ_a, procprt_TCPRASZ_e, ' ', 7};
/***************************************************************/
char *
procprt_TCPSND_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        val2valstr(curstat->net.tcpsnd, buf, 6, avgval, nsecs);

        return buf;
}

char *
procprt_TCPSND_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	val2valstr(curstat->net.tcpsnd, buf, 6, avgval, nsecs);

        	return buf;
	}
	else
        	return "     -";
}

detail_printdef procprt_TCPSND = 
   { "TCPSND", "TCPSND", procprt_TCPSND_a, procprt_TCPSND_e, ' ', 6};
/***************************************************************/
char *
procprt_TCPSASZ_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        int avgtcps = curstat->net.tcpsnd ?
                                  curstat->net.tcpssz / curstat->net.tcpsnd : 0;

        val2valstr(avgtcps, buf, 7, 0, 0);
        return buf;
}

char *
procprt_TCPSASZ_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	int avgtcps = curstat->net.tcpsnd ?
                                  curstat->net.tcpssz / curstat->net.tcpsnd : 0;

        	val2valstr(avgtcps, buf, 7, 0, 0);
        	return buf;
	}
	else
        	return "      -";
}

detail_printdef procprt_TCPSASZ = 
   { "TCPSASZ", "TCPSASZ", procprt_TCPSASZ_a, procprt_TCPSASZ_e, ' ', 7};
/***************************************************************/
char *
procprt_UDPRCV_a(struct tstat *curstat, int avgval, int nsecs)        
{
        static char buf[10];
        
        val2valstr(curstat->net.udprcv, buf, 6, avgval, nsecs);

        return buf;
}

char *
procprt_UDPRCV_e(struct tstat *curstat, int avgval, int nsecs) 
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	val2valstr(curstat->net.udprcv, buf, 6, avgval, nsecs);

        	return buf;
	}
	else
        	return "     -";
}


detail_printdef procprt_UDPRCV = 
   { "UDPRCV", "UDPRCV", procprt_UDPRCV_a, procprt_UDPRCV_e, ' ', 6};
/***************************************************************/
char *
procprt_UDPRASZ_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        int avgudpr = curstat->net.udprcv ?
                          curstat->net.udprsz / curstat->net.udprcv : 0;

        val2valstr(avgudpr, buf, 7, 0, 0);
        return buf;
}

char *
procprt_UDPRASZ_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	int avgudpr = curstat->net.udprcv ?
                          curstat->net.udprsz / curstat->net.udprcv : 0;

        	val2valstr(avgudpr, buf, 7, 0, 0);
        	return buf;
	}
	else
        	return "      -";
}


detail_printdef procprt_UDPRASZ = 
   { "UDPRASZ", "UDPRASZ", procprt_UDPRASZ_a, procprt_UDPRASZ_e, ' ', 7};
/***************************************************************/
char *
procprt_UDPSND_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        val2valstr(curstat->net.udpsnd, buf, 6, avgval, nsecs);

        return buf;
}

char *
procprt_UDPSND_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	val2valstr(curstat->net.udpsnd, buf, 6, avgval, nsecs);

        	return buf;
	}
	else
        	return "     -";
}

detail_printdef procprt_UDPSND = 
   { "UDPSND", "UDPSND", procprt_UDPSND_a, procprt_UDPSND_e, ' ', 6};
/***************************************************************/
char *
procprt_UDPSASZ_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        int avgudps = curstat->net.udpsnd ?
                                  curstat->net.udpssz / curstat->net.udpsnd : 0;

        val2valstr(avgudps, buf, 7, 0, 0);
        return buf;
}

char *
procprt_UDPSASZ_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
        
        	int avgudps = curstat->net.udpsnd ?
                                  curstat->net.udpssz / curstat->net.udpsnd : 0;

        	val2valstr(avgudps, buf, 7, 0, 0);
        	return buf;
	}
	else
        	return "      -";
}


detail_printdef procprt_UDPSASZ = 
   { "UDPSASZ", "UDPSASZ", procprt_UDPSASZ_a, procprt_UDPSASZ_e, ' ', 7};
/***************************************************************/
char *
procprt_RNET_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        val2valstr(curstat->net.tcprcv + curstat->net.udprcv ,
					buf, 5, avgval, nsecs);

        return buf;
}

char *
procprt_RNET_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[10];
 
	        val2valstr(curstat->net.tcprcv + curstat->net.udprcv ,
					buf, 5, avgval, nsecs);

       		return buf;
	}
	else
        	return "    -";
}

detail_printdef procprt_RNET = 
   { " RNET", "RNET", procprt_RNET_a, procprt_RNET_e, ' ', 5};
/***************************************************************/
char *
procprt_SNET_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];
        
        val2valstr(curstat->net.tcpsnd + curstat->net.udpsnd,
                           		buf, 5, avgval, nsecs);
        return buf;
}

char *
procprt_SNET_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
	        static char buf[10];
        
       		val2valstr(curstat->net.tcpsnd + curstat->net.udpsnd,
                           		buf, 5, avgval, nsecs);
	        return buf;
	}
	else
        	return "    -";
}

detail_printdef procprt_SNET = 
   { " SNET", "SNET", procprt_SNET_a, procprt_SNET_e, ' ', 5};
/***************************************************************/
char *
procprt_BANDWI_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[16];
	count_t     rkbps = (curstat->net.tcprsz+curstat->net.udprsz)/125/nsecs;

	format_bandw(buf, rkbps);
        return buf;
}

char *
procprt_BANDWI_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[16];
		count_t     rkbps = (curstat->net.tcprsz + curstat->net.udprsz)
								/125/nsecs;

		format_bandw(buf, rkbps);
        	return buf;
	}
	else
        	return "        -";
}

detail_printdef procprt_BANDWI = 
   { "   BANDWI", "BANDWI", procprt_BANDWI_a, procprt_BANDWI_e, ' ', 9};
/***************************************************************/
char *
procprt_BANDWO_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[16];
	count_t     skbps = (curstat->net.tcpssz+curstat->net.udpssz)/125/nsecs;

	format_bandw(buf, skbps);
        return buf;
}

char *
procprt_BANDWO_e(struct tstat *curstat, int avgval, int nsecs)
{
	if (supportflags & NETATOPD || supportflags & NETATOPBPF )
	{
        	static char buf[16];
		count_t     skbps = (curstat->net.tcpssz + curstat->net.udpssz)
								/125/nsecs;

		format_bandw(buf, skbps);
       		return buf;
	}
	else
        	return "        -";
}

detail_printdef procprt_BANDWO = 
   { "   BANDWO", "BANDWO", procprt_BANDWO_a, procprt_BANDWO_e, ' ', 9};
/***************************************************************/
static void
format_bandw(char *buf, count_t kbps)
{
	char        c;

	if (kbps < 10000)
	{
                c='K';
        }
        else if (kbps < (count_t)10000 * 1000)
        {
                kbps/=1000;
                c = 'M';
        }
        else if (kbps < (count_t)10000 * 1000 * 1000)
        {
                kbps/=1000 * 1000;
                c = 'G';
        }
        else
        {
                kbps = kbps / 1000 / 1000 / 1000;
                c = 'T';
        }

        sprintf(buf, "%4lld %cbps", kbps%100000, c);
}
/***************************************************************/
char *
procprt_GPULIST_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char	buf[64];
        char		tmp[64], *p=tmp;
	int		i;

	if (!curstat->gpu.state)
		return "       -";

	if (!curstat->gpu.gpulist)
		return "       -";

	for (i=0; i < nrgpus; i++)
	{
		if (curstat->gpu.gpulist & 1<<i)
		{
			if (tmp == p)	// first?
				p += snprintf(p, sizeof tmp, "%d", i);
			else
				p += snprintf(p, sizeof tmp - (p-tmp), ",%d", i);

			if (p - tmp > 8)
			{
				snprintf(tmp, sizeof tmp, "0x%06x",
						curstat->gpu.gpulist);
				break;
			}
		}
	}

	snprintf(buf, sizeof buf, "%8.8s", tmp);
        return buf;
}

detail_printdef procprt_GPULIST = 
   { " GPUNUMS", "GPULIST", procprt_GPULIST_ae, procprt_GPULIST_ae, ' ', 8};
/***************************************************************/
char *
procprt_GPUMEMNOW_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	if (!curstat->gpu.state)
		return "     -";

        val2memstr(curstat->gpu.memnow*1024, buf, BFORMAT, 0, 0);
        return buf;
}

detail_printdef procprt_GPUMEMNOW = 
   { "MEMNOW", "GPUMEM", procprt_GPUMEMNOW_ae, procprt_GPUMEMNOW_ae, ' ', 6};
/***************************************************************/
char *
procprt_GPUMEMAVG_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	if (!curstat->gpu.state)
		return "     -";

	if (curstat->gpu.sample == 0)
		return("    0K");

       	val2memstr(curstat->gpu.nrgpus * curstat->gpu.memcum /
	           curstat->gpu.sample*1024, buf, BFORMAT, 0, 0);
       	return buf;
}

detail_printdef procprt_GPUMEMAVG = 
   { "MEMAVG", "GPUMEMAVG", procprt_GPUMEMAVG_ae, procprt_GPUMEMAVG_ae, ' ', 6};
/***************************************************************/
char *
procprt_GPUGPUBUSY_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char 	buf[16];

	if (!curstat->gpu.state)
		return "      -";

	if (curstat->gpu.gpubusy == -1)
		return "    N/A";

       	snprintf(buf, sizeof buf, "%6d%%", curstat->gpu.gpubusy);
       	return buf;
}

detail_printdef procprt_GPUGPUBUSY = 
   { "GPUBUSY", "GPUGPUBUSY", procprt_GPUGPUBUSY_ae, procprt_GPUGPUBUSY_ae, ' ', 7};
/***************************************************************/
char *
procprt_GPUMEMBUSY_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char 	buf[16];

	if (!curstat->gpu.state)
		return "      -";

	if (curstat->gpu.membusy == -1)
		return "    N/A";

        snprintf(buf, sizeof buf, "%6d%%", curstat->gpu.membusy);
        return buf;
}

detail_printdef procprt_GPUMEMBUSY = 
   { "MEMBUSY", "GPUMEMBUSY", procprt_GPUMEMBUSY_ae, procprt_GPUMEMBUSY_ae, ' ', 7};
/***************************************************************/
char *
procprt_WCHAN_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[32];

        if (curstat->gen.state != 'R')
		snprintf(buf, sizeof buf, "%-15.15s", curstat->cpu.wchan);
	else
		snprintf(buf, sizeof buf, "%-15.15s", " ");

        return buf;
}

char *
procprt_WCHAN_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[32];

	snprintf(buf, sizeof buf, "%-15.15s", " ");
        return buf;
}

detail_printdef procprt_WCHAN =
   { "WCHAN          ", "WCHAN", procprt_WCHAN_a, procprt_WCHAN_e, ' ', 15};
/***************************************************************/
char *
procprt_RUNDELAY_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2cpustr(curstat->cpu.rundelay/1000000, buf);
        return buf;
}

char *
procprt_RUNDELAY_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        snprintf(buf, sizeof buf, "     -");
        return buf;
}

detail_printdef procprt_RUNDELAY =
   { "RDELAY", "RDELAY", procprt_RUNDELAY_a, procprt_RUNDELAY_e, ' ', 6};
/***************************************************************/
char *
procprt_BLKDELAY_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2cpustr(curstat->cpu.blkdelay*1000/hertz, buf);
        return buf;
}

char *
procprt_BLKDELAY_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        snprintf(buf, sizeof buf, "     -");
        return buf;
}

detail_printdef procprt_BLKDELAY =
   { "BDELAY", "BDELAY", procprt_BLKDELAY_a, procprt_BLKDELAY_e, ' ', 6};
/***************************************************************/
char *
procprt_NVCSW_a(struct tstat *curstat, int avgval, int nsecs)
{
	static char buf[64];

        val2valstr(curstat->cpu.nvcsw, buf, 6, avgval, nsecs);
	return buf;
}

char *
procprt_NVCSW_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "     -";
}

detail_printdef procprt_NVCSW =
   { " NVCSW", "NVCSW", procprt_NVCSW_a, procprt_NVCSW_e, ' ', 6};
/***************************************************************/
char *
procprt_NIVCSW_a(struct tstat *curstat, int avgval, int nsecs)
{
	static char buf[64];

        val2valstr(curstat->cpu.nivcsw, buf, 6, avgval, nsecs);
	return buf;
}

char *
procprt_NIVCSW_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "     -";
}

detail_printdef procprt_NIVCSW =
   { "NIVCSW", "NIVCSW", procprt_NIVCSW_a, procprt_NIVCSW_e, ' ', 6};
/***************************************************************/
char *
procprt_SORTITEM_ae(struct tstat *curstat, int avgval, int nsecs)
{
        return "";   // dummy function
}

detail_printdef procprt_SORTITEM =   // width is dynamically defined!
   { 0, "SORTITEM", procprt_SORTITEM_ae, procprt_SORTITEM_ae, ' ', 4};


/***************************************************************/
/* CGROUP LEVEL FORMATTING                                     */
/***************************************************************/

/*
 * showcgrouphead: show header line for cgroups.
 * if in interactive mode, also add a page number
 * if in interactive mode, columns are aligned to fill out rows
 */
void
showcgrouphead(detail_printpair *elemptr, int curlist, int totlist, char showorder)
{
        detail_printpair curelem;

        char	*chead="";
        int	col=0, curline;
        char	pagindic[16];
        int	pagindiclen;
        int	n=0;
        int	bufsz;
        int	maxw=screen ? COLS : linelen; 

        colspacings = getspacings(elemptr);
        bufsz       = maxw+1;

        elemptr     = newelems;     // point to adjusted array

        char 	buf[bufsz+2];    // long live dynamically sized auto arrays...

        if (screen) 
		getyx(stdscr, curline, col);	// get current line
	else
                printg("\n");

	// show column by column
	//
        while ((curelem=*elemptr).pf!=0) 
        {
		chead = curelem.pf->head;

                if (screen)
                {
			// print header, optionally colored when it is
			// the current sort criterion
			//
			if (showorder == curelem.pf->sortcrit)
			{
				if (usecolors)
					attron(COLOR_PAIR(FGCOLORINFO));
				else
					attron(A_BOLD);
			}

                        printg("%s", chead);

			if (showorder == curelem.pf->sortcrit)
			{
				if (usecolors)
					attroff(COLOR_PAIR(FGCOLORINFO));
				else
					attroff(A_BOLD);
			}

			// print filler spaces
			//
                        printg("%*s", colspacings[n], "");
                }
                else
                {
                        col += sprintf(buf+col, "%s%s ", "", chead);
                }
                              
                elemptr++;
                n++;
        }

        if (screen)   // add page number, eat from last header if needed...
        {
                pagindiclen = sprintf(pagindic,"%d/%d", curlist, totlist);
		move(curline, COLS-pagindiclen);
                printg("%s", pagindic);
        }
	else	// no screen: print entire buffer at once
	{
        	printg("%s\n", buf);
	}
}


/***************************************************************/
/*
 * showcgroupline: show line for cgroups.
 * if in interactive mode, columns are aligned to fill out rows
 * params:
 *     elemptr: pointer to array of print definition structs ptrs
 *     cgchain: the cgroup to print
 *     tstat: the process to print (is NULL for a cgroup line)
 *     nsecs: number of seconds elapsed between previous and this sample
 *     avgval: is averaging out per second needed?
 *     cputicks: total ticks elapsed
 *     nrcpu: number of CPUs 
 */
void
showcgroupline(detail_printpair* elemptr,
		struct cgchainer *cgchain, struct tstat *tstat,
		int nsecs, int avgval, count_t cputicks, int nrcpu) 
{
        detail_printpair 	curelem;
        int			n=0, color=0, linecolor=0;

        elemptr=newelems;      // point to static array

	if (screen)
	{
		if (cgchain->cstat->gen.depth <= 1)	// zero or first cgroup level?
			linecolor = FGCOLORINFO;

		if (tstat)				// process info line?
			linecolor = FGCOLORBORDER;
	}

        while ((curelem=*elemptr).pf!=0) 
        {
		char *buf;


		color = 0;

		buf = curelem.pf->doactiveconvert(cgchain, tstat,
				avgval, nsecs, cputicks, nrcpu, &color);

                if (screen)
		{
			if (cgchain->cstat->gen.depth == 0 && !tstat)	// root cgroup?
				attron(A_BOLD);

			if (color == 0)	// no explicit color from conversion fc
				color = linecolor;

			if (color)
			{
				if (usecolors)
					attron(COLOR_PAIR(color));
				else
					attron(A_BOLD);
			}

			printg("%s", buf);
                        printg("%*s",colspacings[n], "");

			if (color)
			{
				if (usecolors)
					attroff(COLOR_PAIR(color));
				else
					attroff(A_BOLD);
			}

			if (cgchain->cstat->gen.depth == 0 && !tstat)	// root cgroup?
				attroff(A_BOLD);
		}
                else
		{
			printg("%s", buf);
                        printg(" ");
		}

                elemptr++;
                n++;
        }

        if (!screen) 
                printg("\n");
}

/***************************************************************/
char *
cgroup_CGROUP_PATH(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[4098];

        extern detail_printdef cgroupprt_CGROUP_PATH;
        extern int	startoffset;	// influenced by -> and <- keys
	int		i;

	char		*cgrname  = cgchain->cstat->cgname;
	int		namelen   = cgchain->cstat->gen.namelen;
	int		cgrdepth  = cgchain->cstat->gen.depth;
	unsigned long	vlinemask = cgchain->vlinemask;

	if (*cgrname == '\0')	// root cgroup?
	{
		cgrname = "/";
		namelen  = 1;
	}

	int	maxnamelen = cgroupprt_CGROUP_PATH.width - (cgrdepth*3);
        int 	curoffset  = startoffset <= namelen ? startoffset : namelen;

        if (screen) 
	{
		switch (cgrdepth)
		{
		   case 0:
			sprintf(buf, "%-*s", cgroupprt_CGROUP_PATH.width, "/");
			break;

		   default:
			// draw continuous vertical bars for
			// previous levels if not stub
			//
			for (i=0; i < cgrdepth-1; i++)
			{
				if (i >= CGRMAXDEPTH || vlinemask & (1<<i))
					addch(ACS_VLINE);
				else
					addch(' ');

				addch(' ');
				addch(' ');
			}

			if (!tstat)	// cgroup line
			{
				if (cgrdepth >= CGRMAXDEPTH || cgchain->stub)
	                		addch(ACS_LLCORNER);
				else
	                		addch(ACS_LTEE);

	                	addch(ACS_HLINE);
			}
			else		// process line
			{
				if (cgrdepth >= CGRMAXDEPTH || cgchain->stub)
					addch(' ');
				else
	                		addch(ACS_VLINE);

				addch(' ');
			}

   			sprintf(buf, " %-*.*s", maxnamelen, maxnamelen,
						cgrname+curoffset);
		}
	}
        else
	{
                sprintf(buf, "%*s%-*.*s", cgrdepth*2, "",
				cgroupprt_CGROUP_PATH.width - cgrdepth*2,
				cgroupprt_CGROUP_PATH.width - cgrdepth*2,
				cgrname);
	}

        return buf;
}

detail_printdef cgroupprt_CGROUP_PATH = 
       {"CGROUP (scroll: <- ->)    ", "CGRPATH", 
        cgroup_CGROUP_PATH, NULL, ' ', 26, 0};
/***************************************************************/
char *
cgroup_CGRNPROCS(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[10];

	if (tstat)	// process info?
		return "      ";

	// cgroup info
        val2valstr(cgchain->cstat->gen.nprocs, buf, 6, 0, 0); 
        return buf;
}

detail_printdef cgroupprt_CGRNPROCS = 
   { "NPROCS", "CGRNPROCS", cgroup_CGRNPROCS, NULL, ' ', 6};
/***************************************************************/
char *
cgroup_CGRNPROCSB(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[10];

	if (tstat)	// process info?
		return "      ";

	// cgroup info
        val2valstr(cgchain->cstat->gen.procsbelow, buf, 6, 0, 0); 
        return buf;
}

detail_printdef cgroupprt_CGRNPROCSB = 
   { "PBELOW", "CGRNPROCSB", cgroup_CGRNPROCSB, NULL, ' ', 6};
/***************************************************************/
char *
cgroup_CGRCPUBUSY(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[16];
	float		perc;
       	int		maxperc, badness = 0;

	if (!tstat)	// cgroup info?
	{
		if (cgchain->cstat->cpu.utime == -1)	// undefined?
			return "      -";

		perc	= (cgchain->cstat->cpu.utime +
			   cgchain->cstat->cpu.stime) /
			  (cputicks/nrcpu*100.0);

		maxperc = cgchain->cstat->conf.cpumax;

		// determine if CPU load is limited on system level
		//
		if (cpubadness)
			badness = perc / nrcpu * 100.0 / cpubadness;

		if (badness >= 100)
			*color = FGCOLORCRIT;

		// determine if CPU load is limited by cpu.max within cgroup
		//
		if (maxperc >= 0 && perc + 2 >= maxperc)
			*color = FGCOLORCRIT;
	}
	else		// process info
	{
		perc = (tstat->cpu.utime + tstat->cpu.stime) * 100.0 /
							(cputicks/nrcpu);
	}

	if (perc < 1000.0)
		snprintf(buf, sizeof buf, "%6.2f%%", perc);
	else
		if (perc < 10000.0)
			snprintf(buf, sizeof buf, "%6.1f%%", perc);
		else
			snprintf(buf, sizeof buf, "%6.0f%%", perc);

        return buf;
}

detail_printdef cgroupprt_CGRCPUBUSY =
   { "CPUBUSY", "CGRCPUBUSY", cgroup_CGRCPUBUSY, NULL, 'C', 7};
/***************************************************************/
char *
cgroup_CGRCPUPSI(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[16];
	float		perc;

	if (tstat)	// process info?
		return "     ";

	// cgroup info
	switch (cgchain->cstat->cpu.somepres)
	{
	   case -1:
        	return "    -";
	   default:
		perc = cgchain->cstat->cpu.somepres /
		       (cputicks/nrcpu*100.0);

		if (perc >= 25.0)
                	*color = FGCOLORCRIT;

		snprintf(buf, sizeof buf, "%4.0f%%", perc);
        	return buf;
	}
}

detail_printdef cgroupprt_CGRCPUPSI =
   { "CPUPS", "CGRCPUPSI", cgroup_CGRCPUPSI, NULL, ' ', 5};
/***************************************************************/
char *
cgroup_CGRCPUMAX(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[16];
	float       perc;
	int         maxperc;

	if (tstat)	// process info?
		return "      ";

	maxperc = cgchain->cstat->conf.cpumax;

	// when current cpu percentage is colored due to limitation
	// by cpu.max, also color cpu.max itself
	//
	if (cgchain->cstat->cpu.utime != -1)	// cpu usage available?
	{
		perc	= (cgchain->cstat->cpu.utime +
		           cgchain->cstat->cpu.stime) /
		          (cputicks/nrcpu*100.0);

		if (maxperc >= 0 && perc + 2 >= maxperc)
			*color = FGCOLORCRIT;
	}

	// cgroup info
	//
	switch (cgchain->cstat->conf.cpumax)
	{
	   case -1:
        	return "   max";
	   case -2:
        	return "     -";
	   default:
		snprintf(buf, sizeof buf, "%5d%%", maxperc);
        	return buf;
	}
}

detail_printdef cgroupprt_CGRCPUMAX =
   { "CPUMAX", "CGRCPUMAX", cgroup_CGRCPUMAX, NULL, ' ', 6};
/***************************************************************/
char *
cgroup_CGRCPUWGT(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[16];

	if (tstat)	// process info?
		return "      ";

	// cgroup info
	switch (cgchain->cstat->conf.cpuweight)
	{
	   case -2:
        	return "     -";
	   default:
		snprintf(buf, sizeof buf, "%6d", cgchain->cstat->conf.cpuweight);
        	return buf;
	}
}

detail_printdef cgroupprt_CGRCPUWGT =
   { "CPUWGT", "CGRCPUWGT", cgroup_CGRCPUWGT, NULL, ' ', 6};
/***************************************************************/
char *
cgroup_CGRMEMORY(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[16];
	count_t		memusage, maxusage;


	if (!tstat)	// show cgroup info?
	{
		if (cgchain->cstat->mem.current > 0)		// defined?
		{
			memusage = cgchain->cstat->mem.current;
		}
		else
		{
			if (cgchain->cstat->mem.anon == -1)	// undefined?
				return "     -";

			memusage = (cgchain->cstat->mem.anon +
			            cgchain->cstat->mem.file +
		                    cgchain->cstat->mem.kernel +
		                    cgchain->cstat->mem.shmem);
		}

        	maxusage =  cgchain->cstat->conf.memmax;

		// set color if occupation percentage > 95%
		//
		if (maxusage > 0 && memusage * 100 / maxusage > 95)
			*color = FGCOLORCRIT;

        	val2memstr(memusage * pagesize, buf, BFORMAT, 0, 0);
	}
	else		// show process info
	{
        	val2memstr(tstat->mem.rmem*1024, buf, BFORMAT, 0, 0);
	}

       	return buf;
}

detail_printdef cgroupprt_CGRMEMORY =
   { "MEMORY", "CGRMEMORY", cgroup_CGRMEMORY, NULL, 'M', 6};
/***************************************************************/
char *
cgroup_CGRMEMPSI(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[16];
	float		perc;

	if (tstat)	// process info?
		return "     ";

	// cgroup info
	switch (cgchain->cstat->mem.somepres)
	{
	   case -1:
        	return "    -";
	   default:
		perc = cgchain->cstat->mem.fullpres /
		       (cputicks/nrcpu*100.0);

		if (perc >= 20.0)
                	*color = FGCOLORCRIT;

		snprintf(buf, sizeof buf, "%4.0f%%", perc);
        	return buf;
	}
}

detail_printdef cgroupprt_CGRMEMPSI =
   { "MEMPS", "CGRMEMPSI", cgroup_CGRMEMPSI, NULL, ' ', 5};
/***************************************************************/
char *
cgroup_CGRMEMMAX(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[16];
	count_t		memusage, maxusage;

	if (tstat)	// process info?
		return "      ";

        maxusage =  cgchain->cstat->conf.memmax;

	if (cgchain->cstat->mem.anon != -1)	// current usage defined?
	{
		memusage = (cgchain->cstat->mem.anon +
		            cgchain->cstat->mem.file +
		            cgchain->cstat->mem.kernel);

		// set color if occupation percentage > 95%
		//
		if (maxusage > 0 && memusage * 100 / maxusage > 95)
			*color = FGCOLORCRIT;
	}

	// cgroup info
	switch (cgchain->cstat->conf.memmax)
	{
	   case -1:
        	return "   max";
	   case -2:
        	return "     -";
	   default:
        	val2memstr(maxusage*pagesize, buf, BFORMAT, 0, 0);
       	 	return buf;
	}
}

detail_printdef cgroupprt_CGRMEMMAX =
   { "MEMMAX", "CGRMEMMAX", cgroup_CGRMEMMAX, NULL, ' ', 6};
/***************************************************************/
char *
cgroup_CGRSWPMAX(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[16];

	if (tstat)	// process info?
		return "      ";

	// cgroup info
	switch (cgchain->cstat->conf.swpmax)
	{
	   case -1:
        	return "   max";
	   case -2:
        	return "     -";
	   default:
        	val2memstr(cgchain->cstat->conf.swpmax*pagesize, buf, BFORMAT, 0, 0);
        	return buf;
	}
}

detail_printdef cgroupprt_CGRSWPMAX =
   { "SWPMAX", "CGRSWPMAX", cgroup_CGRSWPMAX, NULL, ' ', 6};
/***************************************************************/
char *
cgroup_CGRDISKIO(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[16];

	if (!tstat)	// show cgroup info?
	{
		if (cgchain->cstat->dsk.rbytes == -1)		// not defined?
			return "     -";

        	val2memstr(cgchain->cstat->dsk.rbytes + cgchain->cstat->dsk.wbytes,
							buf, BFORMAT, avgval, nsecs);
	}
	else		// show process info
	{
		if (supportflags & IOSTAT)
        		val2memstr((tstat->dsk.rsz+tstat->dsk.wsz)*512,
							buf, BFORMAT, avgval, nsecs);
		else
			strcpy(buf, "nopriv");
	}

       	return buf;
}

detail_printdef cgroupprt_CGRDISKIO =
   { "DISKIO", "CGRDISKIO", cgroup_CGRDISKIO, NULL, 'D', 6};
/***************************************************************/
char *
cgroup_CGRDSKPSI(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char	buf[16];
	float		perc;

	if (tstat)	// process info?
		return "     ";

	// cgroup info
	switch (cgchain->cstat->dsk.somepres)
	{
	   case -1:
        	return "    -";
	   default:
		perc = cgchain->cstat->dsk.fullpres /
		       (cputicks/nrcpu*100.0);

		if (perc >= 25.0)
                	*color = FGCOLORCRIT;

		snprintf(buf, sizeof buf, "%4.0f%%", perc);
        	return buf;
	}
}

detail_printdef cgroupprt_CGRDSKPSI =
   { "DSKPS", "CGRDSKPSI", cgroup_CGRDSKPSI, NULL, ' ', 5};
/***************************************************************/
char *
cgroup_CGRDSKWGT(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[16];

	if (tstat)	// process info?
		return "     ";

	// cgroup info
	switch (cgchain->cstat->conf.dskweight)
	{
	   case -2:
        	return "    -";
	   default:
		snprintf(buf, sizeof buf, "%5d", cgchain->cstat->conf.dskweight);
        	return buf;
	}
}

detail_printdef cgroupprt_CGRDSKWGT =
   { "IOWGT", "CGRDSKWGT", cgroup_CGRDSKWGT, NULL, ' ', 5};
/***************************************************************/
char *
cgroup_CGRPID(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[64];

	if (tstat)	// process info?
        	sprintf(buf, "%*d", cgroupprt_CGRPID.width, tstat->gen.pid);
	else		// only cgroup info
        	sprintf(buf, "%*s", cgroupprt_CGRPID.width, " ");

        return buf;
}

detail_printdef cgroupprt_CGRPID = 
   { "PID", "CGRPID", cgroup_CGRPID, NULL, ' ', 5}; //DYNAMIC WIDTH!
/***************************************************************/
char *
cgroup_CGRCMD(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[16];

	if (tstat)	// process info?
	{
        	sprintf(buf, "%-14.14s", tstat->gen.name);
	}
	else		// cgroup info
	{
		if (cgroupdepth == 8 && cgchain->cstat->gen.depth == 0)
		{
			sprintf(buf, "[suppressed]");
			*color = FGCOLORBORDER;
		}
		else
		{
        		sprintf(buf, "%-14.14s", " ");
		}
	}

        return buf;
}

detail_printdef cgroupprt_CGRCMD = 
   { "CMD           ", "CGRCMD", cgroup_CGRCMD, NULL, ' ', 14};
/***************************************************************/
