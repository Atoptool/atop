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
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 2025
**
** Sort on any column
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
#include <regex.h>
#include <limits.h>

#include "atop.h"
#include "photoproc.h"
#include "photosyst.h"
#include "cgroups.h"
#include "showgeneric.h"
#include "showlinux.h"

static void	format_bandw(char *, int, count_t);
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
char *procprt_GPUPROCTYPE_ae(struct tstat *, int, int);
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
char *procprt_RESOURCE_ae(struct tstat *, int, int);

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


// AND with 0x1f to saving entries in table
//
static char *columnhead[] = {
	[MPERCCPU&0x1f] = "CPU",
	[MPERCDSK&0x1f] = "DSK",
	[MPERCGPU&0x1f] = "GPU",
	[MPERCMEM&0x1f] = "MEM",
	[MPERCNET&0x1f] = "NET",
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
showprochead(detail_printpair *elemptr, int curlist, int totlist, struct procview *pv)
{
        detail_printpair curelem;

        char *chead="";
        int  resource = pv->showresource;
        int  curline, col;
        char pagindic[10];
        int  pagindiclen;
        int  n=0;
        char buf[16];

        colspacings=getspacings(elemptr);

        elemptr=newelems;     // point to adjusted array
        
        if (screen) 
	{
		getyx(stdscr, curline, col);	// get current line
		(void) col;			// intentionally unused
	}
	else
	{
                printg("\n");
	}

        while ((curelem=*elemptr).pf != NULL) 
        {
                if (curelem.pf->elementnr == 0)     // empty header==special: RESOURCE
                {
			snprintf(buf, sizeof buf, "%*s", procprt_RESOURCE.width,
						columnhead[resource&0x1f]);
                        chead = buf;
                } 
                else 
                {
                        chead = curelem.pf->head;
                }

                if (screen)
                {
			// print sort criterium column? then switch on color
			//
			if (curelem.pf->elementnr == pv->sortcolumn)
			{
				if (usecolors)
					attron(COLOR_PAIR(FGCOLORINFO));
				else
					attron(A_BOLD);
			}

			// print column header
			//
                        printg("%-*s", curelem.pf->width, chead);

			// print sort criterium column? then switch off color
			//
			if (curelem.pf->elementnr == pv->sortcolumn)
			{
				if (usecolors)
					attroff(COLOR_PAIR(FGCOLORINFO));
				else
					attroff(A_BOLD);
			}

                        printg("%*s", colspacings[n], "");
                }
                else
                {
                        printg("%s ", chead);
                }
                              
                elemptr++;
                n++;
        }

        if (screen)   // add page number, eat from last header if needed...
        {
                pagindiclen = snprintf(pagindic, sizeof pagindic, "%d/%d", curlist, totlist);

		move(curline, COLS-pagindiclen);
                printg("%s", pagindic);
        }
	else
	{
		printg("\n");
	}
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
                // what to print?  RESOURCE, or active process or
                // exited process?

                if (curelem.pf->elementnr == 0)     // empty header==special: RESOURCE
                {
                        printg("%*.0lf%%", procprt_RESOURCE.width-1, perc);
                }
                else if (curstat->gen.state != 'E')  // active process
                {
                        printg("%s", curelem.pf->ac.doactiveconverts(curstat, 
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

/***************************************************************/
/* Generic functions to translate UID or GID number into a     */
/* string of minimal 8 characters, to be defined by 'collen'.  */
/* The buffer 'strbuf' in which the string is stored should    */
/* have a size that is minimally 1 larger than the column      */
/* length.                                                     */
/* In this case, the return value is 0.                        */
/*                                                             */
/* When no translation is wanted (-I flag) or when the         */
/* name is longer than the required column length, a string    */
/* representation of the UID or GID number is returned.        */
/* In this case, the return value is 1.                        */
/***************************************************************/
static int
uid2str(uid_t uid, char *strbuf, int collen)
{
        char *username;

	if (!idnamesuppress && (username = uid2name(uid)) && strlen(username) <= collen)
	{
		snprintf(strbuf, collen+1, "%-*.*s", collen, collen, username);
		return 0;
	}
        else 
	{
		snprintf(strbuf, collen+1, "%-*d", collen, uid);
		return 1;
	}
}

static int
gid2str(gid_t gid, char *strbuf, int collen)
{
        char *grpname;

	if (!idnamesuppress && (grpname = gid2name(gid)) && strlen(grpname) <= collen)
	{
		snprintf(strbuf, collen+1, "%-*.*s", collen, collen, grpname);
		return 0;
	}
        else 
	{
		snprintf(strbuf, collen+1, "%-*d", collen, gid);
		return 1;
	}
}


/***************************************************************/
/* PROCESS PRINT FUNCTIONS                                     */
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
/* Definition of column that shows specific resource           */
/* percentage to be influenced by keys C, M, D, N, E.          */
/***************************************************************/
char *
procprt_RESOURCE_ae(struct tstat *curstat, int avgval, int nsecs)
{
        return "";   // dummy function
}

int
compcpu(const void *a, const void *b, void *dir)
{
        register count_t acpu = (*(struct tstat **)a)->cpu.stime +
 	                        (*(struct tstat **)a)->cpu.utime;
        register count_t bcpu = (*(struct tstat **)b)->cpu.stime +
 	                        (*(struct tstat **)b)->cpu.utime;

        if (acpu > bcpu)
		return   1 * *(int *)dir;

        if (acpu < bcpu)
		return  -1 * *(int *)dir;

        return compmem(a, b, dir);
}

int
compmem(const void *a, const void *b, void *dir)
{
        register count_t amem = (*(struct tstat **)a)->mem.rmem;
        register count_t bmem = (*(struct tstat **)b)->mem.rmem;

        if (amem > bmem)
		return   1 * *(int *)dir;

        if (amem < bmem)
		return  -1 * *(int *)dir;

        return  0;
}

int
compdsk(const void *a, const void *b, void *dir)
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

        if (adsk > bdsk)
		return   1 * *(int *)dir;

        if (adsk < bdsk)
		return  -1 * *(int *)dir;

        return compcpu(a, b, dir);
}

int
compnet(const void *a, const void *b, void *dir)
{
        register count_t anet = (*(struct tstat **)a)->net.tcpssz +
                                (*(struct tstat **)a)->net.tcprsz +
                                (*(struct tstat **)a)->net.udpssz +
                                (*(struct tstat **)a)->net.udprsz;
        register count_t bnet = (*(struct tstat **)b)->net.tcpssz +
                                (*(struct tstat **)b)->net.tcprsz +
                                (*(struct tstat **)b)->net.udpssz +
                                (*(struct tstat **)b)->net.udprsz;

        if (anet > bnet)
		return   1 * *(int *)dir;

        if (anet < bnet)
		return  -1 * *(int *)dir;

	return compcpu(a, b, dir);
}

int
compgpu(const void *a, const void *b, void *dir)	// always descending
{
        register char 	 astate = (*(struct tstat **)a)->gpu.state;
        register char 	 bstate = (*(struct tstat **)b)->gpu.state;

        register count_t abusy  = (*(struct tstat **)a)->gpu.gpubusycum;
        register count_t bbusy  = (*(struct tstat **)b)->gpu.gpubusycum;
        register count_t amem   = (*(struct tstat **)a)->gpu.memnow;
        register count_t bmem   = (*(struct tstat **)b)->gpu.memnow;

        if (!astate)		// no GPU usage?
		abusy = amem = -2; 

        if (!bstate)		// no GPU usage?
		bbusy = bmem = -2; 

	if (abusy == -1 || bbusy == -1)
	{
                if (amem > bmem)
			return   1 * *(int *)dir;

                if (amem < bmem) return -1;
			return  -1 * *(int *)dir;

                return  0;
	}
	else
	{
                if (abusy > bbusy)
			return   1 * *(int *)dir;

                if (abusy < bbusy)
			return  -1 * *(int *)dir;

       		return  0;
	}
}

detail_printdef procprt_RESOURCE =   // width is dynamically defined!
   {0, "", "RESOURCE", .ac.doactiveconverts = procprt_RESOURCE_ae, procprt_RESOURCE_ae, compcpu, -1, 4, 0};
/***************************************************************/
int comptid(const void *, const void *, void *);

char *
procprt_TID_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.isproc)
        	snprintf(buf, sizeof buf, "%*s", procprt_TID.width, "-");
	else
        	snprintf(buf, sizeof buf, "%*d", procprt_TID.width, curstat->gen.pid);
        return buf;
}

int
comptid(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.pid;
        register int bval = (*(struct tstat **)b)->gen.pid;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TID = 
   {0, "TID", "TID", .ac.doactiveconverts = procprt_TID_ae, procprt_TID_ae, comptid, 1, 5, 0}; //DYNAMIC WIDTH!
/***************************************************************/
int comppid(const void *, const void *, void *);

char *
procprt_PID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        snprintf(buf, sizeof buf, "%*d", procprt_PID.width, curstat->gen.tgid);
        return buf;
}

char *
procprt_PID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        if (curstat->gen.pid == 0)
        	snprintf(buf, sizeof buf, "%*s", procprt_PID.width, "?");
	else
        	snprintf(buf, sizeof buf, "%*d", procprt_PID.width, curstat->gen.tgid);
        return buf;
}

int
comppid(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.tgid;
        register int bval = (*(struct tstat **)b)->gen.tgid;

        register int apid = (*(struct tstat **)a)->gen.pid;
        register int bpid = (*(struct tstat **)b)->gen.pid;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && apid == 0)
		aval = 0;

	if (bstate == 'E' && bpid == 0)
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_PID = 
   {0, "PID", "PID", .ac.doactiveconverts = procprt_PID_a, procprt_PID_e, comppid, 1, 5}; //DYNAMIC WIDTH!
/***************************************************************/
int compppid(const void *, const void *, void *);

char *
procprt_PPID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        snprintf(buf, sizeof buf, "%*d", procprt_PPID.width, curstat->gen.ppid);
        return buf;
}

char *
procprt_PPID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.ppid)
        	snprintf(buf, sizeof buf, "%*d", procprt_PPID.width, curstat->gen.ppid);
	else
		snprintf(buf, sizeof buf, "%*s", procprt_PPID.width, "-");
        return buf;
}

int
compppid(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.ppid;
        register int bval = (*(struct tstat **)b)->gen.ppid;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_PPID = 
   {0, "PPID", "PPID", .ac.doactiveconverts = procprt_PPID_a, procprt_PPID_e, compppid, 1, 5, 0}; //DYNAMIC WIDTH!
/***************************************************************/
int compvpid(const void *, const void *, void *);

char *
procprt_VPID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

        snprintf(buf, sizeof buf, "%*d", procprt_VPID.width, curstat->gen.vpid);
        return buf;
}

char *
procprt_VPID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	snprintf(buf, sizeof buf, "%*s", procprt_VPID.width, "-");
        return buf;
}

int
compvpid(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.vpid;
        register int bval = (*(struct tstat **)b)->gen.vpid;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_VPID = 
   {0, "VPID", "VPID", .ac.doactiveconverts = procprt_VPID_a, procprt_VPID_e, compvpid, 1, 5, 0}; //DYNAMIC WIDTH!
/***************************************************************/
int compctid(const void *, const void *, void *);

char *
procprt_CTID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[32];

        snprintf(buf, sizeof buf, "%5d", curstat->gen.ctid);
        return buf;
}

char *
procprt_CTID_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    -";
}

int
compctid(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.ctid;
        register int bval = (*(struct tstat **)b)->gen.ctid;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_CTID = 
   {0, " CTID", "CTID", .ac.doactiveconverts = procprt_CTID_a, procprt_CTID_e, compctid, 1, 5, 0};
/***************************************************************/
#define HOSTUTS		"-----host-----"

int compcid(const void *, const void *, void *);

char *
procprt_CID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.utsname[0])
        	snprintf(buf, sizeof buf, "%-15.15s", curstat->gen.utsname);
	else
        	snprintf(buf, sizeof buf, "%-15s", HOSTUTS);

        return buf;
}

char *
procprt_CID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	if (curstat->gen.utsname[0])
        	snprintf(buf, sizeof buf, "%-15s", curstat->gen.utsname);
	else
        	snprintf(buf, sizeof buf, "%-15s", "?");

        return buf;
}

int
compcid(const void *a, const void *b, void *dir)
{
        register char *aval = (*(struct tstat **)a)->gen.utsname;
        register char *bval = (*(struct tstat **)b)->gen.utsname;

	if (*aval == 0 && *bval == 0)
		return 0;

	if (*aval == 0)
		aval = HOSTUTS;

	if (*bval == 0)
		bval = HOSTUTS;

	// empty string means 'host' 
	//
	return strcmp(aval, bval) * *(int *)dir;
}

detail_printdef procprt_CID = 
   {0, "CID/POD        ", "CID", .ac.doactiveconverts = procprt_CID_a, procprt_CID_e, compcid, -1, 15, 0};
/***************************************************************/
int compsyscpu(const void *, const void *, void *);

char *
procprt_SYSCPU_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2cpustr(curstat->cpu.stime*1000/hertz, buf);
        return buf;
}

int
compsyscpu(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->cpu.stime;
        register count_t bval = (*(struct tstat **)b)->cpu.stime;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_SYSCPU = 
   {0, "SYSCPU", "SYSCPU", .ac.doactiveconverts = procprt_SYSCPU_ae, procprt_SYSCPU_ae, compsyscpu, -1, 6, 0};
/***************************************************************/
int compusrcpu(const void *, const void *, void *);

char *
procprt_USRCPU_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2cpustr(curstat->cpu.utime*1000/hertz, buf);
        return buf;
}

int
compusrcpu(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->cpu.utime;
        register count_t bval = (*(struct tstat **)b)->cpu.utime;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_USRCPU = 
   {0, "USRCPU", "USRCPU", .ac.doactiveconverts = procprt_USRCPU_ae, procprt_USRCPU_ae, compusrcpu, -1, 6, 0};
/***************************************************************/
int compvgrow(const void *, const void *, void *);

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

int
compvgrow(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vgrow;
        register count_t bval = (*(struct tstat **)b)->mem.vgrow;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_VGROW = 
   {0, " VGROW", "VGROW", .ac.doactiveconverts = procprt_VGROW_a, procprt_VGROW_e, compvgrow, -1, 6, 0};
/***************************************************************/
int comprgrow(const void *, const void *, void *);

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

int
comprgrow(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.rgrow;
        register count_t bval = (*(struct tstat **)b)->mem.rgrow;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_RGROW = 
   {0, " RGROW", "RGROW", .ac.doactiveconverts = procprt_RGROW_a, procprt_RGROW_e, comprgrow, -1, 6, 0};
/***************************************************************/
int compminflt(const void *, const void *, void *);

char *
procprt_MINFLT_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2valstr(curstat->mem.minflt, buf, 6, avgval, nsecs);
        return buf;
}

int
compminflt(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.minflt;
        register count_t bval = (*(struct tstat **)b)->mem.minflt;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_MINFLT = 
   {0, "MINFLT", "MINFLT", .ac.doactiveconverts = procprt_MINFLT_ae, procprt_MINFLT_ae, compminflt, -1, 6, 0};
/***************************************************************/
int compmajflt(const void *, const void *, void *);

char *
procprt_MAJFLT_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2valstr(curstat->mem.majflt, buf, 6, avgval, nsecs);
        return buf;
}

int
compmajflt(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.majflt;
        register count_t bval = (*(struct tstat **)b)->mem.majflt;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_MAJFLT = 
   {0, "MAJFLT", "MAJFLT", .ac.doactiveconverts = procprt_MAJFLT_ae, procprt_MAJFLT_ae, compmajflt, -1, 6, 0};
/***************************************************************/
int compvstext(const void *, const void *, void *);

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

int
compvstext(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vexec;
        register count_t bval = (*(struct tstat **)b)->mem.vexec;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_VSTEXT = 
   {0, "VSTEXT", "VSTEXT", .ac.doactiveconverts = procprt_VSTEXT_a, procprt_VSTEXT_e, compvstext, -1, 6, 0};
/***************************************************************/
int compvsize(const void *a, const void *b, void *dir);

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

int
compvsize(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vmem;
        register count_t bval = (*(struct tstat **)b)->mem.vmem;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_VSIZE = 
   {0, " VSIZE", "VSIZE", .ac.doactiveconverts = procprt_VSIZE_a, procprt_VSIZE_e, compvsize, -1, 6, 0};
/***************************************************************/
int comprsize(const void *a, const void *b, void *dir);

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

int
comprsize(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.rmem;
        register count_t bval = (*(struct tstat **)b)->mem.rmem;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_RSIZE = 
   {0, " RSIZE", "RSIZE", .ac.doactiveconverts = procprt_RSIZE_a, procprt_RSIZE_e, comprsize, -1, 6, 0};
/***************************************************************/
int comppsize(const void *, const void *, void *);

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

int
comppsize(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.pmem;
        register count_t bval = (*(struct tstat **)b)->mem.pmem;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_PSIZE = 
   {0, " PSIZE", "PSIZE", .ac.doactiveconverts = procprt_PSIZE_a, procprt_PSIZE_e, comppsize, -1, 6, 0};
/***************************************************************/
int compvlibs(const void *, const void *, void *);

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

int
compvlibs(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vlibs;
        register count_t bval = (*(struct tstat **)b)->mem.vlibs;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_VSLIBS = 
   {0, "VSLIBS", "VSLIBS", .ac.doactiveconverts = procprt_VSLIBS_a, procprt_VSLIBS_e, compvlibs, -1, 6, 0};
/***************************************************************/
int compvdata(const void *, const void *, void *);

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

int
compvdata(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vdata;
        register count_t bval = (*(struct tstat **)b)->mem.vdata;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_VDATA = 
   {0, " VDATA", "VDATA", .ac.doactiveconverts = procprt_VDATA_a, procprt_VDATA_e, compvdata, -1, 6, 0};
/***************************************************************/
int compvstack(const void *, const void *, void *);

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

int
compvstack(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vstack;
        register count_t bval = (*(struct tstat **)b)->mem.vstack;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_VSTACK = 
   {0, "VSTACK", "VSTACK", .ac.doactiveconverts = procprt_VSTACK_a, procprt_VSTACK_e, compvstack, -1, 6, 0};
/***************************************************************/
int compswapsz(const void *, const void *, void *);

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

int
compswapsz(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vswap;
        register count_t bval = (*(struct tstat **)b)->mem.vswap;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_SWAPSZ = 
   {0, "SWAPSZ", "SWAPSZ", .ac.doactiveconverts = procprt_SWAPSZ_a, procprt_SWAPSZ_e, compswapsz, -1, 6, 0};
/***************************************************************/
int complocksz(const void *, const void *, void *);

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

int
complocksz(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->mem.vlock;
        register count_t bval = (*(struct tstat **)b)->mem.vlock;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_LOCKSZ = 
   {0, "LOCKSZ", "LOCKSZ", .ac.doactiveconverts = procprt_LOCKSZ_a, procprt_LOCKSZ_e, complocksz, -1, 6, 0};
/***************************************************************/
int compcmd(const void *, const void *, void *);

char *
procprt_CMD_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%-14.14s", curstat->gen.name);
        return buf;
}

char *
procprt_CMD_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15]="<";
        char        helpbuf[15];

        snprintf(helpbuf, sizeof helpbuf, "<%.12s>",  curstat->gen.name);
        snprintf(buf,     sizeof buf,     "%-14.14s", helpbuf);
        return buf;
}

int
compcmd(const void *a, const void *b, void *dir)
{
        register char *aval = (*(struct tstat **)a)->gen.name;
        register char *bval = (*(struct tstat **)b)->gen.name;

	return strcmp(aval, bval) * *(int *)dir;
}

detail_printdef procprt_CMD = 
   {0, "CMD           ", "CMD", .ac.doactiveconverts = procprt_CMD_a, procprt_CMD_e, compcmd, 1, 14, 0};
/***************************************************************/
int compruid(const void *, const void *, void *);

char *
procprt_RUID_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	uid2str(curstat->gen.ruid, buf, procprt_RUID.width);

        return buf;
}

int
compruid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.ruid;
        register int	bval = (*(struct tstat **)b)->gen.ruid;
	int		anumeric, bnumeric;

        static char abuf[64], bbuf[64];

	anumeric = uid2str(aval, abuf, 32);
	bnumeric = uid2str(bval, bbuf, 32);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_RUID = 
   {0, "RUID    ", "RUID", .ac.doactiveconverts = procprt_RUID_ae, procprt_RUID_ae, compruid, 1, 8, 0}; // DYNAMIC WIDTH
/***************************************************************/
int compeuid(const void *, const void *, void *);

char *
procprt_EUID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	uid2str(curstat->gen.euid, buf, procprt_EUID.width);

        return buf;
}

char *
procprt_EUID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	snprintf(buf, sizeof buf, "%-*s", procprt_EUID.width, "-");

	return buf;
}

int
compeuid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.euid;
        register int	bval = (*(struct tstat **)b)->gen.euid;
	int		anumeric, bnumeric;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

        static char abuf[64], bbuf[64];

	anumeric = uid2str(aval, abuf, 32);
	bnumeric = uid2str(bval, bbuf, 32);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	if (astate == 'E')
	{
		memset(abuf, 'z', sizeof(abuf)-1);
		abuf[sizeof(abuf)-1] = '\0';
	}

	if (bstate == 'E')
	{
		memset(bbuf, 'z', sizeof(bbuf)-1);
		bbuf[sizeof(bbuf)-1] = '\0';
	}

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_EUID = 
   {0, "EUID    ", "EUID", .ac.doactiveconverts = procprt_EUID_a, procprt_EUID_e, compeuid, 1, 8, 0}; // DYNAMIC WIDTH
/***************************************************************/
int compsuid(const void *, const void *, void *);

char *
procprt_SUID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];

	uid2str(curstat->gen.suid, buf, sizeof buf-1);

        return buf;
}

char *
procprt_SUID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

int
compsuid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.suid;
        register int	bval = (*(struct tstat **)b)->gen.suid;
	int		anumeric, bnumeric;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;


        static char abuf[9], bbuf[9];

	anumeric = uid2str(aval, abuf, sizeof abuf-1);
	bnumeric = uid2str(bval, bbuf, sizeof bbuf-1);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	if (astate == 'E')
	{
		memset(abuf, 'z', sizeof(abuf)-1);
		abuf[sizeof(abuf)-1] = '\0';
	}

	if (bstate == 'E')
	{
		memset(bbuf, 'z', sizeof(bbuf)-1);
		bbuf[sizeof(bbuf)-1] = '\0';
	}

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_SUID = 
   {0, "SUID    ", "SUID", .ac.doactiveconverts = procprt_SUID_a, procprt_SUID_e, compsuid, 1, 8, 0};
/***************************************************************/
int compfsuid(const void *, const void *, void *);

char *
procprt_FSUID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];

	uid2str(curstat->gen.fsuid, buf, sizeof buf-1);

        return buf;
}

char *
procprt_FSUID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

int
compfsuid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.fsuid;
        register int	bval = (*(struct tstat **)b)->gen.fsuid;
	int		anumeric, bnumeric;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

        static char abuf[9], bbuf[9];

	anumeric = uid2str(aval, abuf, sizeof abuf-1);
	bnumeric = uid2str(bval, bbuf, sizeof bbuf-1);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	if (astate == 'E')
	{
		memset(abuf, 'z', sizeof(abuf)-1);
		abuf[sizeof(abuf)-1] = '\0';
	}

	if (bstate == 'E')
	{
		memset(bbuf, 'z', sizeof(bbuf)-1);
		bbuf[sizeof(bbuf)-1] = '\0';
	}

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_FSUID = 
   {0, "FSUID   ", "FSUID", .ac.doactiveconverts = procprt_FSUID_a, procprt_FSUID_e, compfsuid, 1, 8, 0};
/***************************************************************/
int comprgid(const void *, const void *, void *);

char *
procprt_RGID_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	gid2str(curstat->gen.rgid, buf, procprt_RGID.width);

        return buf;
}

int
comprgid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.rgid;
        register int	bval = (*(struct tstat **)b)->gen.rgid;
	int		anumeric, bnumeric;

        static char abuf[64], bbuf[64];

	anumeric = gid2str(aval, abuf, 32);
	bnumeric = gid2str(bval, bbuf, 32);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_RGID = 
   {0, "RGID    ", "RGID", .ac.doactiveconverts = procprt_RGID_ae, procprt_RGID_ae, comprgid, 1, 8, 0}; // DYNAMIC WIDTH
/***************************************************************/
int compegid(const void *, const void *, void *);

char *
procprt_EGID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	gid2str(curstat->gen.egid, buf, procprt_EGID.width);

        return buf;
}

char *
procprt_EGID_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[64];

	snprintf(buf, sizeof buf, "%-*s", procprt_EGID.width, "-");

	return buf;
}

int
compegid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.egid;
        register int	bval = (*(struct tstat **)b)->gen.egid;
	int		anumeric, bnumeric;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

        static char abuf[64], bbuf[64];

	anumeric = gid2str(aval, abuf, 32);
	bnumeric = gid2str(bval, bbuf, 32);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	if (astate == 'E')
	{
		memset(abuf, 'z', sizeof(abuf)-1);
		abuf[sizeof(abuf)-1] = '\0';
	}

	if (bstate == 'E')
	{
		memset(bbuf, 'z', sizeof(bbuf)-1);
		bbuf[sizeof(bbuf)-1] = '\0';
	}

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_EGID = 
   {0, "EGID    ", "EGID", .ac.doactiveconverts = procprt_EGID_a, procprt_EGID_e, compegid, 1, 8, 0}; // DYNAMIC WIDTH
/***************************************************************/
int compsgid(const void *, const void *, void *);

char *
procprt_SGID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	gid2str(curstat->gen.sgid, buf, 8);

        return buf;
}

char *
procprt_SGID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

int
compsgid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.sgid;
        register int	bval = (*(struct tstat **)b)->gen.sgid;
	int		anumeric, bnumeric;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

        static char abuf[9], bbuf[9];

	anumeric = gid2str(aval, abuf, sizeof abuf-1);
	bnumeric = gid2str(bval, bbuf, sizeof bbuf-1);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	if (astate == 'E')
	{
		memset(abuf, 'z', sizeof(abuf)-1);
		abuf[sizeof(abuf)-1] = '\0';
	}

	if (bstate == 'E')
	{
		memset(bbuf, 'z', sizeof(bbuf)-1);
		bbuf[sizeof(bbuf)-1] = '\0';
	}

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_SGID = 
   {0, "SGID    ", "SGID", .ac.doactiveconverts = procprt_SGID_a, procprt_SGID_e, compsgid, 1, 8, 0};
/***************************************************************/
int compfsgid(const void *, const void *, void *);

char *
procprt_FSGID_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	gid2str(curstat->gen.fsgid, buf, 8);

        return buf;
}

char *
procprt_FSGID_e(struct tstat *curstat, int avgval, int nsecs)
{
	return "-       ";
}

int
compfsgid(const void *a, const void *b, void *dir)
{
        register int	aval = (*(struct tstat **)a)->gen.fsgid;
        register int	bval = (*(struct tstat **)b)->gen.fsgid;
	int		anumeric, bnumeric;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

        static char abuf[9], bbuf[9];

	anumeric = gid2str(aval, abuf, sizeof abuf-1);
	bnumeric = gid2str(bval, bbuf, sizeof bbuf-1);

	if (anumeric && bnumeric)
		return (aval - bval) * *(int *)dir;

	if (astate == 'E')
	{
		memset(abuf, 'z', sizeof(abuf)-1);
		abuf[sizeof(abuf)-1] = '\0';
	}

	if (bstate == 'E')
	{
		memset(bbuf, 'z', sizeof(bbuf)-1);
		bbuf[sizeof(bbuf)-1] = '\0';
	}

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_FSGID = 
   {0, "FSGID   ", "FSGID", .ac.doactiveconverts = procprt_FSGID_a, procprt_FSGID_e, compfsgid, 1, 8, 0};
/***************************************************************/
int compdate(const void *, const void *, void *);

char *
procprt_STDATE_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[11];

        convdate(curstat->gen.btime, buf);
        return buf;
}

int
compdate(const void *a, const void *b, void *dir)
{
        register time_t aval = (*(struct tstat **)a)->gen.btime;
        register time_t bval = (*(struct tstat **)b)->gen.btime;

        static char abuf[32], bbuf[32];

	// concatenate date and time strings to order time
	// with the same date as well
	//
        convdate(aval, abuf);
        convdate(bval, bbuf);

        convtime(aval, abuf+10);
        convtime(bval, bbuf+10);

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_STDATE = 
   {0, "  STDATE  ", "STDATE", .ac.doactiveconverts = procprt_STDATE_ae, procprt_STDATE_ae, compdate, 1, 10, 0};
/***************************************************************/
int comptime(const void *, const void *, void *);

char *
procprt_STTIME_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[9];

        convtime(curstat->gen.btime, buf);
        return buf;
}

int
comptime(const void *a, const void *b, void *dir)
{
        register time_t aval = (*(struct tstat **)a)->gen.btime;
        register time_t bval = (*(struct tstat **)b)->gen.btime;

        static char abuf[11], bbuf[11];

        convtime(aval, abuf);
        convtime(bval, bbuf);

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_STTIME = 
   {0, " STTIME ", "STTIME", .ac.doactiveconverts = procprt_STTIME_ae, procprt_STTIME_ae, comptime, 1, 8, 0};
/***************************************************************/
int compendate(const void *, const void *, void *);

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

int
compendate(const void *a, const void *b, void *dir)
{
        register time_t aval =	(*(struct tstat **)a)->gen.btime +
        			(*(struct tstat **)a)->gen.elaps/hertz;
        register time_t bval = 	(*(struct tstat **)b)->gen.btime +
        			(*(struct tstat **)b)->gen.elaps/hertz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

        static char abuf[32], bbuf[32];

	if (astate != 'E')
		aval = INT_MAX;

	if (bstate != 'E')
		bval = INT_MAX;

	// concatenate date and time strings to order time
	// with the same date as well
	//
        convdate(aval, abuf);
        convdate(bval, bbuf);

        convtime(aval, abuf+10);
        convtime(bval, bbuf+10);

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_ENDATE = 
   {0, "  ENDATE  ", "ENDATE", .ac.doactiveconverts = procprt_ENDATE_a, procprt_ENDATE_e, compendate, 1, 10, 0};
/***************************************************************/
int compentime(const void *, const void *, void *);

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

int
compentime(const void *a, const void *b, void *dir)
{
        register time_t aval =	(*(struct tstat **)a)->gen.btime +
        			(*(struct tstat **)a)->gen.elaps/hertz;
        register time_t bval = 	(*(struct tstat **)b)->gen.btime +
        			(*(struct tstat **)b)->gen.elaps/hertz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

        static char abuf[9], bbuf[9];

        convtime(aval, abuf);
        convtime(bval, bbuf);

	if (astate != 'E')
		strcpy(abuf, "99:99:99");

	if (bstate != 'E')
		strcpy(bbuf, "99:99:99");

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_ENTIME = 
   {0, " ENTIME ", "ENTIME", .ac.doactiveconverts = procprt_ENTIME_a, procprt_ENTIME_e, compentime, 1, 8, 0};
/***************************************************************/
int compthr(const void *, const void *, void *);

char *
procprt_THR_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%4d", curstat->gen.nthr);
        return buf;
}

char *
procprt_THR_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   0";
}

int
compthr(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.nthr;
        register int bval = (*(struct tstat **)b)->gen.nthr;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_THR = 
   {0, " THR", "THR", .ac.doactiveconverts = procprt_THR_a, procprt_THR_e, compthr, -1, 4, 0};
/***************************************************************/
int compthrr(const void *, const void *, void *);

char *
procprt_TRUN_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%4d", curstat->gen.nthrrun);
        return buf;
}

char *
procprt_TRUN_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   0";
}

int
compthrr(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.nthrrun;
        register int bval = (*(struct tstat **)b)->gen.nthrrun;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TRUN = 
   {0, "TRUN", "TRUN", .ac.doactiveconverts = procprt_TRUN_a, procprt_TRUN_e, compthrr, -1, 4, 0};
/***************************************************************/
int compthrs(const void *, const void *, void *);

char *
procprt_TSLPI_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%5d", curstat->gen.nthrslpi);
        return buf;
}

char *
procprt_TSLPI_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0";
}

int
compthrs(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.nthrslpi;
        register int bval = (*(struct tstat **)b)->gen.nthrslpi;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TSLPI = 
   {0, "TSLPI", "TSLPI", .ac.doactiveconverts = procprt_TSLPI_a, procprt_TSLPI_e, compthrs, -1, 5, 0};
/***************************************************************/
int compthru(const void *, const void *, void *);

char *
procprt_TSLPU_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%5d", curstat->gen.nthrslpu);
        return buf;
}

char *
procprt_TSLPU_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0";
}

int
compthru(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.nthrslpu;
        register int bval = (*(struct tstat **)b)->gen.nthrslpu;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TSLPU = 
   {0, "TSLPU", "TSLPU", .ac.doactiveconverts = procprt_TSLPU_a, procprt_TSLPU_e, compthru, -1, 5, 0};
/***************************************************************/
int compthri(const void *, const void *, void *);

char *
procprt_TIDLE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%5d", curstat->gen.nthridle);
        return buf;
}

char *
procprt_TIDLE_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    0";
}

int
compthri(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.nthridle;
        register int bval = (*(struct tstat **)b)->gen.nthridle;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 0;

	if (bstate == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TIDLE = 
   {0, "TIDLE", "TIDLE", .ac.doactiveconverts = procprt_TIDLE_a, procprt_TIDLE_e, compthri, -1, 5, 0};
/***************************************************************/
int comppoli(const void *, const void *, void *);

#define SCHED_NORMAL	0
#define SCHED_FIFO	1
#define SCHED_RR	2
#define SCHED_BATCH	3
#define SCHED_ISO	4
#define SCHED_IDLE	5
#define SCHED_DEADLINE	6
#define SCHED_UNKNOWN1	7
#define SCHED_UNKNOWN2	8
#define SCHED_UNKNOWN3	9

static char *policies[] = {
	[SCHED_NORMAL]   = "norm",
	[SCHED_FIFO]     = "fifo",
	[SCHED_RR]       = "rr  ",
	[SCHED_BATCH]    = "btch",
	[SCHED_ISO]      = "iso ",
	[SCHED_IDLE]     = "idle",
	[SCHED_DEADLINE] = "dead",
	[SCHED_UNKNOWN1] = "7   ",
	[SCHED_UNKNOWN2] = "8   ",
	[SCHED_UNKNOWN3] = "9   ",
};

char *
procprt_POLI_a(struct tstat *curstat, int avgval, int nsecs)
{
        if (curstat->cpu.policy <= SCHED_UNKNOWN3)
        	return policies[curstat->cpu.policy];
	else
        	return "?   ";
}

char *
procprt_POLI_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "-   ";
}

int
comppoli(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->cpu.policy;
        register int bval = (*(struct tstat **)b)->cpu.policy;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	char *abuf, *bbuf;

        if (aval < SCHED_UNKNOWN1)
		abuf = policies[aval];
	else
		abuf = "zzzz";

        if (bval < SCHED_UNKNOWN1)
		bbuf = policies[bval];
	else
		bbuf = "zzzz";

	if (astate == 'E')
		abuf = "zzzz";

	if (bstate == 'E')
		bbuf = "zzzz";

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_POLI = 
   {0, "POLI", "POLI", .ac.doactiveconverts = procprt_POLI_a, procprt_POLI_e, comppoli, 1, 4, 0};
/***************************************************************/
int compnice(const void *, const void *, void *);

char *
procprt_NICE_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%4d", curstat->cpu.nice);
        return buf;
}

char *
procprt_NICE_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   -";
}

int
compnice(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->cpu.nice;
        register int bval = (*(struct tstat **)b)->cpu.nice;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 99;	// anyhow higher than 19

	if (bstate == 'E')
		bval = 99;	// anyhow higher than 19

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_NICE = 
   {0, "NICE", "NICE", .ac.doactiveconverts = procprt_NICE_a, procprt_NICE_e, compnice, 1, 4, 0};
/***************************************************************/
int comppri(const void *, const void *, void *);

char *
procprt_PRI_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%3d", curstat->cpu.prio);
        return buf;
}

char *
procprt_PRI_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "  -";
}

int
comppri(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->cpu.prio;
        register int bval = (*(struct tstat **)b)->cpu.prio;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 9999;

	if (bstate == 'E')
		bval = 9999;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_PRI = 
   {0, "PRI", "PRI", .ac.doactiveconverts = procprt_PRI_a, procprt_PRI_e, comppri, 1, 3, 0};
/***************************************************************/
int comprtpr(const void *, const void *, void *);

char *
procprt_RTPR_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%4d", curstat->cpu.rtprio);
        return buf;
}

char *
procprt_RTPR_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "   -";
}

int
comprtpr(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->cpu.rtprio;
        register int bval = (*(struct tstat **)b)->cpu.rtprio;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = -1;

	if (bstate == 'E')
		bval = -1;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_RTPR = 
   {0, "RTPR", "RTPR", .ac.doactiveconverts = procprt_RTPR_a, procprt_RTPR_e, comprtpr, -1, 4, 0};
/***************************************************************/
int compcpunr(const void *, const void *, void *);

char *
procprt_CURCPU_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[15];

        snprintf(buf, sizeof buf, "%5d", curstat->cpu.curcpu);
        return buf;
}

char *
procprt_CURCPU_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "    -";
}

int
compcpunr(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->cpu.curcpu;
        register int bval = (*(struct tstat **)b)->cpu.curcpu;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E')
		aval = 999999;

	if (bstate == 'E')
		bval = 999999;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_CURCPU = 
   {0, "CPUNR", "CPUNR", .ac.doactiveconverts = procprt_CURCPU_a, procprt_CURCPU_e, compcpunr, 1, 5, 0};
/***************************************************************/
int compst(const void *, const void *, void *);

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

int
compst(const void *a, const void *b, void *dir)
{
        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	char	abuf[5], bbuf[5];

	if (astate == 'E')
		strncpy(abuf, procprt_ST_e(*(struct tstat **)a, 0, 0), 3);
	else
		strncpy(abuf, procprt_ST_a(*(struct tstat **)a, 0, 0), 3);

	abuf[2] = '\0';

	if (bstate == 'E')
		strncpy(bbuf, procprt_ST_e(*(struct tstat **)b, 0, 0), 3);
	else
		strncpy(bbuf, procprt_ST_a(*(struct tstat **)b, 0, 0), 3);

	bbuf[2] = '\0';

	return strcmp(abuf, bbuf) * *(int *)dir;
}

detail_printdef procprt_ST = 
   {0, "ST", "ST", .ac.doactiveconverts = procprt_ST_a, procprt_ST_e, compst, -1, 2, 0};
/***************************************************************/
int compexc(const void *, const void *, void *);

char *
procprt_EXC_a(struct tstat *curstat, int avgval, int nsecs)
{
        return "  -";
}

char *
procprt_EXC_e(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[4];


        snprintf(buf, sizeof buf, "%3d", 
                 curstat->gen.excode & 0xff ?
                          curstat->gen.excode & 0x7f : 
                          (curstat->gen.excode>>8) & 0xff);
        return buf;
}

int
compexc(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.excode;
        register int bval = (*(struct tstat **)b)->gen.excode;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	aval = aval & 0xff ? aval & 0x7f : (aval>>8) & 0xff;
	bval = bval & 0xff ? bval & 0x7f : (bval>>8) & 0xff;

	if (astate != 'E')
		aval = -1;

	if (bstate != 'E')
		bval = -1;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_EXC = 
   {0, "EXC", "EXC", .ac.doactiveconverts = procprt_EXC_a, procprt_EXC_e, compexc, -1, 3, 0};
/***************************************************************/
int compstate(const void *, const void *, void *);

char *
procprt_S_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[2] = "E";

        buf[0] = curstat->gen.state;
        return buf;
}

char *
procprt_S_e(struct tstat *curstat, int avgval, int nsecs)
{
        return "E";
}

int
compstate(const void *a, const void *b, void *dir)
{
        register unsigned char aval = (*(struct tstat **)a)->gen.state;
        register unsigned char bval = (*(struct tstat **)b)->gen.state;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_S = 
   {0, "S", "S", .ac.doactiveconverts = procprt_S_a, procprt_S_e, compstate, -1, 1, 0};
/***************************************************************/
int compcmdline(const void *, const void *, void *);

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
                snprintf(buf, sizeof buf, "%-*.*s", curwidth, curwidth, pline+curoffset);
        else
                snprintf(buf, sizeof buf, "%.*s", CMDLEN, pline+curoffset);

        return buf;
}

int
compcmdline(const void *a, const void *b, void *dir)
{
        register char *acmdline = (*(struct tstat **)a)->gen.cmdline;
        register char *bcmdline = (*(struct tstat **)b)->gen.cmdline;

        register char *acmdname = (*(struct tstat **)a)->gen.name;
        register char *bcmdname = (*(struct tstat **)b)->gen.name;

        register char *acmd = *acmdline ? acmdline : acmdname;
        register char *bcmd = *bcmdline ? bcmdline : bcmdname;

	return strcmp(acmd, bcmd) * *(int *)dir;
}

detail_printdef procprt_COMMAND_LINE = 
    {0, "COMMAND-LINE (horizontal scroll with <- and -> keys)", "COMMAND-LINE", 
        .ac.doactiveconverts = procprt_COMMAND_LINE_ae, procprt_COMMAND_LINE_ae, compcmdline, 1, 0, 1};
/***************************************************************/
int compnprocs(const void *, const void *, void *);

char *
procprt_NPROCS_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

        val2valstr(curstat->gen.pid, buf, 6, 0, 0); // pid abused as proc counter
        return buf;
}

int
compnprocs(const void *a, const void *b, void *dir)
{
        register int aval = (*(struct tstat **)a)->gen.pid;
        register int bval = (*(struct tstat **)b)->gen.pid;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_NPROCS = 
   {0, "NPROCS", "NPROCS", .ac.doactiveconverts = procprt_NPROCS_ae, procprt_NPROCS_ae, compnprocs, -1, 6, 0};
/***************************************************************/ 
int comprddsk(const void *, const void *, void *);

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

int
comprddsk(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->dsk.rsz;
        register count_t bval = (*(struct tstat **)b)->dsk.rsz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' || !(supportflags & IOSTAT))
		aval = 0;

	if (bstate == 'E' || !(supportflags & IOSTAT))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_RDDSK = 
   {0, " RDDSK", "RDDSK", .ac.doactiveconverts = procprt_RDDSK_a, procprt_RDDSK_e, comprddsk, -1, 6, 0};
/***************************************************************/
int compwrdsk(const void *, const void *, void *);

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

int
compwrdsk(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->dsk.wsz;
        register count_t bval = (*(struct tstat **)b)->dsk.wsz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' || !(supportflags & IOSTAT))
		aval = 0;

	if (bstate == 'E' || !(supportflags & IOSTAT))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_WRDSK = 
   {0, " WRDSK", "WRDSK", .ac.doactiveconverts = procprt_WRDSK_a, procprt_WRDSK_e, compwrdsk, -1, 6, 0};
/***************************************************************/
int compcwrdsk(const void *, const void *, void *);

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

int
compcwrdsk(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->dsk.wsz;
        register count_t bval = (*(struct tstat **)b)->dsk.wsz;

        register count_t acal = (*(struct tstat **)a)->dsk.cwsz;
        register count_t bcal = (*(struct tstat **)b)->dsk.cwsz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	aval = aval - acal;
	bval = bval - bcal;

	if (astate == 'E' || !(supportflags & IOSTAT) || aval < 0)
		aval = 0;

	if (bstate == 'E' || !(supportflags & IOSTAT) || bval < 0)
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_CWRDSK = 
   {0," CWRDSK", "CWRDSK", .ac.doactiveconverts = procprt_CWRDSK_a, procprt_WRDSK_e, compcwrdsk, -1, 6, 0};
/***************************************************************/
int compwcancel(const void *, const void *, void *);

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

int
compwcancel(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->dsk.cwsz;
        register count_t bval = (*(struct tstat **)b)->dsk.cwsz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' || !(supportflags & IOSTAT))
		aval = 0;

	if (bstate == 'E' || !(supportflags & IOSTAT))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_WCANCEL = 
   {0,"WCANCL", "WCANCL", .ac.doactiveconverts = procprt_WCANCEL_a, procprt_WCANCEL_e, compwcancel, -1, 6, 0};
/***************************************************************/
int comptcprcv(const void *, const void *, void *);

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

int
comptcprcv(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcprcv;
        register count_t bval = (*(struct tstat **)b)->net.tcprcv;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TCPRCV = 
   {0, "TCPRCV", "TCPRCV", .ac.doactiveconverts = procprt_TCPRCV_a, procprt_TCPRCV_e, comptcprcv, -1, 6, 0};
/***************************************************************/
int comptcprasz(const void *, const void *, void *);

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

int
comptcprasz(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcprcv;
        register count_t bval = (*(struct tstat **)b)->net.tcprcv;

        register count_t asiz = (*(struct tstat **)a)->net.tcprsz;
        register count_t bsiz = (*(struct tstat **)b)->net.tcprsz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	aval = aval ? asiz/aval : 0;
	bval = bval ? bsiz/bval : 0;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TCPRASZ = 
   {0, "TCPRASZ", "TCPRASZ", .ac.doactiveconverts = procprt_TCPRASZ_a, procprt_TCPRASZ_e, comptcprasz, -1, 7, 0};
/***************************************************************/
int comptcpsnd(const void *, const void *, void *);

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

int
comptcpsnd(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcpsnd;
        register count_t bval = (*(struct tstat **)b)->net.tcpsnd;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TCPSND = 
   {0, "TCPSND", "TCPSND", .ac.doactiveconverts = procprt_TCPSND_a, procprt_TCPSND_e, comptcpsnd, -1, 6, 0};
/***************************************************************/
int comptcpsasz(const void *, const void *, void *);

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

int
comptcpsasz(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcpsnd;
        register count_t bval = (*(struct tstat **)b)->net.tcpsnd;

        register count_t asiz = (*(struct tstat **)a)->net.tcpssz;
        register count_t bsiz = (*(struct tstat **)b)->net.tcpssz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	aval = aval ? asiz/aval : 0;
	bval = bval ? bsiz/bval : 0;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_TCPSASZ = 
   {0, "TCPSASZ", "TCPSASZ", .ac.doactiveconverts = procprt_TCPSASZ_a, procprt_TCPSASZ_e, comptcpsasz, -1, 7, 0};
/***************************************************************/
int compudprcv(const void *, const void *, void *);

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


int
compudprcv(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.udprcv;
        register count_t bval = (*(struct tstat **)b)->net.udprcv;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_UDPRCV = 
   {0, "UDPRCV", "UDPRCV", .ac.doactiveconverts = procprt_UDPRCV_a, procprt_UDPRCV_e, compudprcv, -1, 6, 0};
/***************************************************************/
int compudprasz(const void *, const void *, void *);

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

int
compudprasz(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.udprcv;
        register count_t bval = (*(struct tstat **)b)->net.udprcv;

        register count_t asiz = (*(struct tstat **)a)->net.udprsz;
        register count_t bsiz = (*(struct tstat **)b)->net.udprsz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	aval = aval ? asiz/aval : 0;
	bval = bval ? bsiz/bval : 0;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_UDPRASZ = 
   {0, "UDPRASZ", "UDPRASZ", .ac.doactiveconverts = procprt_UDPRASZ_a, procprt_UDPRASZ_e, compudprasz, -1, 7, 0};
/***************************************************************/
int compudpsnd(const void *, const void *, void *);

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

int
compudpsnd(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.udpsnd;
        register count_t bval = (*(struct tstat **)b)->net.udpsnd;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_UDPSND = 
   {0, "UDPSND", "UDPSND", .ac.doactiveconverts = procprt_UDPSND_a, procprt_UDPSND_e, compudpsnd, -1, 6, 0};
/***************************************************************/
int compudpsasz(const void *, const void *, void *);

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

int
compudpsasz(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.udpsnd;
        register count_t bval = (*(struct tstat **)b)->net.udpsnd;

        register count_t asiz = (*(struct tstat **)a)->net.udpssz;
        register count_t bsiz = (*(struct tstat **)b)->net.udpssz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	aval = aval ? asiz/aval : 0;
	bval = bval ? bsiz/bval : 0;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_UDPSASZ = 
   {0, "UDPSASZ", "UDPSASZ", .ac.doactiveconverts = procprt_UDPSASZ_a, procprt_UDPSASZ_e, compudpsasz, -1, 7, 0};
/***************************************************************/
int comprnet(const void *, const void *, void *);

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

int
comprnet(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcprcv +
                                (*(struct tstat **)a)->net.udprcv;
        register count_t bval = (*(struct tstat **)b)->net.tcprcv +
                                (*(struct tstat **)b)->net.udprcv;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_RNET = 
   {0, " RNET", "RNET", .ac.doactiveconverts = procprt_RNET_a, procprt_RNET_e, comprnet, -1, 5, 0};
/***************************************************************/
int compsnet(const void *, const void *, void *);

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

int
compsnet(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcpsnd +
                                (*(struct tstat **)a)->net.udpsnd;
        register count_t bval = (*(struct tstat **)b)->net.tcpsnd +
                                (*(struct tstat **)b)->net.udpsnd;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_SNET = 
   {0, " SNET", "SNET", .ac.doactiveconverts = procprt_SNET_a, procprt_SNET_e, compsnet, -1, 5, 0};
/***************************************************************/
int compbandwi(const void *, const void *, void *);

char *
procprt_BANDWI_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[16];
	count_t     rkbps = (curstat->net.tcprsz+curstat->net.udprsz)/125/nsecs;

	format_bandw(buf, sizeof buf, rkbps);
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

		format_bandw(buf, sizeof buf, rkbps);
        	return buf;
	}
	else
        	return "        -";
}

int
compbandwi(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcprsz +
                                (*(struct tstat **)a)->net.udprsz;
        register count_t bval = (*(struct tstat **)b)->net.tcprsz +
                                (*(struct tstat **)b)->net.udprsz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_BANDWI = 
   {0, "   BANDWI", "BANDWI", .ac.doactiveconverts = procprt_BANDWI_a, procprt_BANDWI_e, compbandwi, -1, 9, 0};
/***************************************************************/
int compbandwo(const void *, const void *, void *);

char *
procprt_BANDWO_a(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[16];
	count_t     skbps = (curstat->net.tcpssz+curstat->net.udpssz)/125/nsecs;

	format_bandw(buf, sizeof buf, skbps);
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

		format_bandw(buf, sizeof buf, skbps);
       		return buf;
	}
	else
        	return "        -";
}

int
compbandwo(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->net.tcpssz +
                                (*(struct tstat **)a)->net.udpssz;
        register count_t bval = (*(struct tstat **)b)->net.tcpssz +
                                (*(struct tstat **)b)->net.udpssz;

        register unsigned char astate = (*(struct tstat **)a)->gen.state;
        register unsigned char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		aval = 0;

	if (bstate == 'E' && !(supportflags & (NETATOPD|NETATOPBPF)))
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_BANDWO = 
   {0, "   BANDWO", "BANDWO", .ac.doactiveconverts = procprt_BANDWO_a, procprt_BANDWO_e, compbandwo, -1, 9, 0};
/***************************************************************/
static void
format_bandw(char *buf, int bufsize, count_t kbps)
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

        snprintf(buf, bufsize, "%4lld %cbps", kbps%100000, c);
}
/***************************************************************/
int compgputype(const void *, const void *, void *);

char *
procprt_GPUPROCTYPE_ae(struct tstat *curstat, int avgval, int nsecs)
{
	static char buf[2];

	if (!curstat->gpu.state)
		return "-";

        snprintf(buf, sizeof buf, "%c", curstat->gpu.type);

        return buf;
}

int
compgputype(const void *a, const void *b, void *dir)
{
        register char aval = (*(struct tstat **)a)->gpu.type;
        register char bval = (*(struct tstat **)b)->gpu.type;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (!astat)
		aval = 'z';;

	if (!bstat)
		bval = 'z';;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_GPUPROCTYPE = 
   {0, "T", "GPUPROCTYPE", .ac.doactiveconverts = procprt_GPUPROCTYPE_ae, procprt_GPUPROCTYPE_ae, compgputype, 0, 1, 0};
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
   {0, " GPUNUMS", "GPULIST", .ac.doactiveconverts = procprt_GPULIST_ae, procprt_GPULIST_ae, NULL, 0, 8, 0};
/***************************************************************/
int compgpumemnow(const void *, const void *, void *);

char *
procprt_GPUMEMNOW_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	if (!curstat->gpu.state)
		return "     -";

        val2memstr(curstat->gpu.memnow*1024, buf, BFORMAT, 0, 0);
        return buf;
}

int
compgpumemnow(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->gpu.memnow;
        register count_t bval = (*(struct tstat **)b)->gpu.memnow;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (!astat)
		aval = 0;

	if (!bstat)
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_GPUMEMNOW = 
   {0, "MEMNOW", "GPUMEM", .ac.doactiveconverts = procprt_GPUMEMNOW_ae, procprt_GPUMEMNOW_ae, compgpumemnow, -1, 6, 0};
/***************************************************************/
int compgpumemavg(const void *, const void *, void *);

char *
procprt_GPUMEMAVG_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char buf[10];

	if (!curstat->gpu.state)
		return "     -";

	if (curstat->gpu.samples == 0)
		return("    0K");

       	val2memstr(curstat->gpu.memcum/curstat->gpu.samples*1024, buf, BFORMAT, 0, 0);
       	return buf;
}

int
compgpumemavg(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->gpu.memcum;
        register count_t bval = (*(struct tstat **)b)->gpu.memcum;

        register count_t asmp = (*(struct tstat **)a)->gpu.samples;
        register count_t bsmp = (*(struct tstat **)b)->gpu.samples;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (!astat || !asmp)
		aval = 0;
	else
		aval /= asmp;

	if (!bstat || !bsmp)
		bval = 0;
	else
		bval /= bsmp;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_GPUMEMAVG = 
   {0, "MEMAVG", "GPUMEMAVG", .ac.doactiveconverts = procprt_GPUMEMAVG_ae, procprt_GPUMEMAVG_ae, compgpumemavg, -1, 6, 0};
/***************************************************************/
int compgpugpubusy(const void *, const void *, void *);

char *
procprt_GPUGPUBUSY_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char 	buf[16], perc[10];

	if (!curstat->gpu.state)
		return "      -";

	if (curstat->gpu.gpubusycum == -1)
		return "    N/A";

	if (nsecs)
	{
       		val2valstr(curstat->gpu.gpubusycum / nsecs, perc, 6, 0, 0);
       		snprintf(buf, sizeof buf, "%6s%%", perc);
	}
	else
	{
		snprintf(buf, sizeof buf, "%6d%%", 0);
	}

       	return buf;
}

int
compgpugpubusy(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->gpu.gpubusycum;
        register count_t bval = (*(struct tstat **)b)->gpu.gpubusycum;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (!astat || aval == -1)
		aval = 0;

	if (!bstat || bval == -1)
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_GPUGPUBUSY = 
   {0, "GPUBUSY", "GPUGPUBUSY", .ac.doactiveconverts = procprt_GPUGPUBUSY_ae, procprt_GPUGPUBUSY_ae, compgpugpubusy, -1, 7, 0};
/***************************************************************/
int compgpumembusy(const void *, const void *, void *);

char *
procprt_GPUMEMBUSY_ae(struct tstat *curstat, int avgval, int nsecs)
{
        static char 	buf[16], perc[10];

	if (!curstat->gpu.state)
		return "      -";

	if (curstat->gpu.membusycum == -1)
		return "    N/A";

	if (nsecs)
	{
       		val2valstr(curstat->gpu.membusycum / nsecs, perc, 6, 0, 0);
       		snprintf(buf, sizeof buf, "%6s%%", perc);
	}
	else
	{
		snprintf(buf, sizeof buf, "%6d%%", 0);
	}

        return buf;
}

int
compgpumembusy(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->gpu.membusycum;
        register count_t bval = (*(struct tstat **)b)->gpu.membusycum;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (!astat || aval == -1)
		aval = 0;

	if (!bstat || bval == -1)
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_GPUMEMBUSY = 
   {0, "MEMBUSY", "GPUMEMBUSY", .ac.doactiveconverts = procprt_GPUMEMBUSY_ae, procprt_GPUMEMBUSY_ae, compgpumembusy, -1, 7, 0};
/***************************************************************/
int compwchan(const void *, const void *, void *);

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

int
compwchan(const void *a, const void *b, void *dir)
{
        register char *aval  = (*(struct tstat **)a)->cpu.wchan;
        register char *bval  = (*(struct tstat **)b)->cpu.wchan;

        register char astate = (*(struct tstat **)a)->gen.state;
        register char bstate = (*(struct tstat **)b)->gen.state;

	if (astate == 'R')
		aval = " ";

	if (bstate == 'R')
		bval = " ";

	return strcmp(aval, bval) * *(int *)dir;
}

detail_printdef procprt_WCHAN =
   {0, "WCHAN          ", "WCHAN", .ac.doactiveconverts = procprt_WCHAN_a, procprt_WCHAN_e, compwchan, 1, 15, 0};
/***************************************************************/
int comprundelay(const void *, const void *, void *);

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

int
comprundelay(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->cpu.rundelay;
        register count_t bval = (*(struct tstat **)b)->cpu.rundelay;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (astat == 'E')
		aval = 0;

	if (bstat == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_RUNDELAY =
   {0, "RDELAY", "RDELAY", .ac.doactiveconverts = procprt_RUNDELAY_a, procprt_RUNDELAY_e, comprundelay, -1, 6, 0};
/***************************************************************/
int compblkdelay(const void *, const void *, void *);

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

int
compblkdelay(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->cpu.blkdelay;
        register count_t bval = (*(struct tstat **)b)->cpu.blkdelay;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (astat == 'E')
		aval = 0;

	if (bstat == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_BLKDELAY =
   {0, "BDELAY", "BDELAY", .ac.doactiveconverts = procprt_BLKDELAY_a, procprt_BLKDELAY_e, compblkdelay, -1, 6, 0};
/***************************************************************/
int compnvcsw(const void *, const void *, void *);

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

int
compnvcsw(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->cpu.nvcsw;
        register count_t bval = (*(struct tstat **)b)->cpu.nvcsw;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (astat == 'E')
		aval = 0;

	if (bstat == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_NVCSW =
   {0, " NVCSW", "NVCSW", .ac.doactiveconverts = procprt_NVCSW_a, procprt_NVCSW_e, compnvcsw, -1, 6, 0};
/***************************************************************/
int compnivcsw(const void *, const void *, void *);

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

int
compnivcsw(const void *a, const void *b, void *dir)
{
        register count_t aval = (*(struct tstat **)a)->cpu.nivcsw;
        register count_t bval = (*(struct tstat **)b)->cpu.nivcsw;

        register char astat = (*(struct tstat **)a)->gpu.state;
        register char bstat = (*(struct tstat **)b)->gpu.state;

	if (astat == 'E')
		aval = 0;

	if (bstat == 'E')
		bval = 0;

	return (aval - bval) * *(int *)dir;
}

detail_printdef procprt_NIVCSW =
   {0, "NIVCSW", "NIVCSW", .ac.doactiveconverts = procprt_NIVCSW_a, procprt_NIVCSW_e, compnivcsw, -1, 6, 0};

/***************************************************************/
/* CGROUP LEVEL FORMATTING                                     */
/***************************************************************/

/*
 * showcgrouphead: show header line for cgroups.
 * if in interactive mode, also add a page number
 * if in interactive mode, columns are aligned to fill out rows
 */
void
showcgrouphead(detail_printpair *elemptr, int curlist, int totlist, struct procview *pv)
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
        while ((curelem = *elemptr).pf != 0) 
        {
		chead = curelem.pf->head;

                if (screen)
                {
			// print header, optionally colored when it is
			// the current sort criterion
			//
			if (curelem.pf->elementnr == pv->sortcolumn)
			{
				if (usecolors)
					attron(COLOR_PAIR(FGCOLORINFO));
				else
					attron(A_BOLD);
			}

                        printg("%s", chead);

			if (curelem.pf->elementnr == pv->sortcolumn)
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
                        col += snprintf(buf+col, sizeof buf-col,"%s%s ", "", chead);
                }
                              
                elemptr++;
                n++;
        }

        if (screen)   // add page number, eat from last header if needed...
        {
                pagindiclen = snprintf(pagindic, sizeof pagindic, "%d/%d", curlist, totlist);
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

		buf = curelem.pf->ac.doactiveconvertc(cgchain, tstat,
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
			snprintf(buf, sizeof buf, "%-*s", cgroupprt_CGROUP_PATH.width, "/");
			break;

		   default:
			// draw continuous vertical bars for
			// previous levels if not stub
			//
			for (i=0; i < cgrdepth-1; i++)
			{
				if (i >= CGRMAXDEPTH || vlinemask & (1ULL<<i))
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

   			snprintf(buf, sizeof buf, " %-*.*s", maxnamelen, maxnamelen,
						cgrname+curoffset);
		}
	}
        else
	{
                snprintf(buf, sizeof buf, "%*s%-*.*s", cgrdepth*2, "",
				cgroupprt_CGROUP_PATH.width - cgrdepth*2,
				cgroupprt_CGROUP_PATH.width - cgrdepth*2,
				cgrname);
	}

        return buf;
}

detail_printdef cgroupprt_CGROUP_PATH = 
    {0, "CGROUP (scroll: <- ->)    ", "CGRPATH", 
	.ac.doactiveconvertc = cgroup_CGROUP_PATH, NULL, NULL, 0, 26, 0};
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
   {0, "NPROCS", "CGRNPROCS", .ac.doactiveconvertc = cgroup_CGRNPROCS, NULL, NULL, 0, 6, 0};
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
   {0, "PBELOW", "CGRNPROCSB", .ac.doactiveconvertc = cgroup_CGRNPROCSB, NULL, NULL, 0, 6, 0};
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
   {0, "CPUBUSY", "CGRCPUBUSY", .ac.doactiveconvertc = cgroup_CGRCPUBUSY, NULL, NULL, 0, 7, 0};
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
   {0, "CPUPS", "CGRCPUPSI", .ac.doactiveconvertc = cgroup_CGRCPUPSI, NULL, NULL, 0, 5, 0};
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
   {0, "CPUMAX", "CGRCPUMAX", .ac.doactiveconvertc = cgroup_CGRCPUMAX, NULL, NULL, 0, 6, 0};
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
   {0, "CPUWGT", "CGRCPUWGT", .ac.doactiveconvertc = cgroup_CGRCPUWGT, NULL, NULL, 0, 6, 0};
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
   {0, "MEMORY", "CGRMEMORY", .ac.doactiveconvertc = cgroup_CGRMEMORY, NULL, NULL, 0, 6, 0};
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
   {0, "MEMPS", "CGRMEMPSI", .ac.doactiveconvertc = cgroup_CGRMEMPSI, NULL, NULL, 0, 5, 0};
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
   {0, "MEMMAX", "CGRMEMMAX", .ac.doactiveconvertc = cgroup_CGRMEMMAX, NULL, NULL, 0, 6, 0};
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
   {0, "SWPMAX", "CGRSWPMAX", .ac.doactiveconvertc = cgroup_CGRSWPMAX, NULL, NULL, 0, 6, 0};
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
   {0, "DISKIO", "CGRDISKIO", .ac.doactiveconvertc = cgroup_CGRDISKIO, NULL, NULL, 0, 6, 0};
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
   {0, "DSKPS", "CGRDSKPSI", .ac.doactiveconvertc = cgroup_CGRDSKPSI, NULL, NULL, 0, 5, 0};
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
   {0, "IOWGT", "CGRDSKWGT", .ac.doactiveconvertc = cgroup_CGRDSKWGT, NULL, NULL, 0, 5, 0};
/***************************************************************/
char *
cgroup_CGRPID(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[64];

	if (tstat)	// process info?
        	snprintf(buf, sizeof buf, "%*d", cgroupprt_CGRPID.width, tstat->gen.pid);
	else		// only cgroup info
        	snprintf(buf, sizeof buf, "%*s", cgroupprt_CGRPID.width, " ");

        return buf;
}

detail_printdef cgroupprt_CGRPID = 
   {0, "PID", "CGRPID", .ac.doactiveconvertc = cgroup_CGRPID, NULL, NULL, 0, 5, 0}; //DYNAMIC WIDTH!
/***************************************************************/
char *
cgroup_CGRCMD(struct cgchainer *cgchain, struct tstat *tstat,
		int avgval, int nsecs, count_t cputicks, int nrcpu, int *color)
{
        static char buf[16];

	if (tstat)	// process info?
	{
        	snprintf(buf, sizeof buf, "%-14.14s", tstat->gen.name);
	}
	else		// cgroup info
	{
		if (cgroupdepth == 8 && cgchain->cstat->gen.depth == 0)
		{
			snprintf(buf, sizeof buf, "[suppressed]");
			*color = FGCOLORBORDER;
		}
		else
		{
        		snprintf(buf, sizeof buf, "%-14.14s", " ");
		}
	}

        return buf;
}

detail_printdef cgroupprt_CGRCMD = 
   {0, "CMD           ", "CGRCMD", .ac.doactiveconvertc = cgroup_CGRCMD, NULL, NULL, 0, 14, 0};
/***************************************************************/
