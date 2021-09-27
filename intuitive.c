/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** ==========================================================================
** Author:      Fei Li
** E-mail:      lifei.shirley@bytedance.com
** Date:        August 2021
** --------------------------------------------------------------------------
** Copyright (c) 2021 ByteDance Inc. All rights reserved.
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
** Revision 1.1  2021/09/26 14:48:19
** Initial revision
** Add support for intuitively display all statistics at a glance, so that
** we can quickly determine if cpu or mem is balanced, especially in cases
** when machines have more cpus than the default 'LINES'.
**
** Usage examples:
** ./atop -I 2 //will show intuitive indicators each two seconds
**
*/

#include <math.h>
#include <ncurses.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#include "atop.h"
#include "photosyst.h"
#include "showgeneric.h"
#include "intuitive.h"

#define SHOWPERC 'p'
#define SHOWBAR 'i'
#define QUITINT 'q'
static char lastch = SHOWBAR;

enum COLORIndicator
{
	COLORMeter = 1,

	COLORSYSCPU,
	COLORUSERCPU,
	COLORIRQCPU,
	COLORWAITCPU,
	COLORCPUMAX,

	COLORNUMA
};

/*
** Let's assume only display 3 indicator: sys/user/irq:
** - 0   < cpus <= 128: 4 panel
** - 128 < cpus <= 256: 8 panel
*/
#define DefaultPanels 8

#define EACHCPUCOL  4

int printperline(float perc, int maxblank, int x, int y) {
	register int i;
	float barperc = 100.0 / maxblank;

	for (i = 0; i < (int) round(perc / barperc) && i < maxblank; i++) {
		mvprintw(x, y, "|");
		y++;
	}

	return i;
}

int printcpuperc(float perc, int maxblank, int x, int y, bool last) {
	int register i, s = 0;

	for (i = 0; i < (maxblank - 3) / 4 - 4; i++) {
		mvprintw(x, y++, " ");
		s++;
	}
	// Reserve 4 blank for percentage, like '100%'
	if (perc > 0.0) {
		mvprintw(x, y, "%3d%%", (int) round(perc));
		s += 4;
		y += 4;
	} else {
		for (i = 0; i < 4; i++) {
			mvprintw(x, y++, " ");
			s++;
		}
	}

	if (!last)
		mvprintw(x, y, "|");
	else
		mvprintw(x, y, " ");
	s++;

	return s;
}

/*
** Intuitively display all system statistics at a glance for an interval
*/

char
intuitiveout(time_t curtime, int numsecs, struct devtstat *devtstat,
         struct devtstat *filtertstat, struct sstat *sstat,
         int nexit, unsigned int noverflow, char flag)
{
	int panels = DefaultPanels;
	int startrow = 1, startcol = 0;
	register int i, r = 0, c = 0;
	count_t percputot;
	float percpusys, percpuuser, percpuirq, percpuiowait = 0.0;
	int curnuma, cpunuma, curcpu, curpanel = 0;
	int nextcol = 0, eachcol = 0, eachblank;
	int eachcpucol = 7; //3 cpus + 2 [ + 2 ]
	int maxcpurows, currow;
	char buf[10];
	int pernumablank;
	int key;
	char showtype = SHOWBAR;

	if (sstat->cpu.nrcpu < 1) {
		mcleanstop(1, "Warning: intuitive mode is not supported if cpu number is less than 1.\n");
		return '\0';
	}

	if (sstat->memnuma.nrnuma <= 0) {
		mcleanstop(1, "Warning: intuitive mode is not supported if there is no numa.\n");
		return '\0';
	}

	if (sstat->cpu.nrcpu <= 128) {
		panels = DefaultPanels / 2;
	}
	maxcpurows = sstat->cpu.nrcpu / panels;

	initscr();
	eachblank = (COLS - eachcpucol * panels) / panels;

	if (has_colors()) {
		use_default_colors();
		if ( start_color() == OK ) {
			init_pair(COLORMeter, COLOR_CYAN, COLOR_BLACK);
			init_pair(COLORSYSCPU, COLOR_RED, COLOR_BLACK);
			init_pair(COLORUSERCPU, COLOR_GREEN, COLOR_BLACK);
			init_pair(COLORIRQCPU, COLOR_YELLOW, COLOR_BLACK);
			init_pair(COLORWAITCPU, COLOR_MAGENTA, COLOR_BLACK);
			init_pair(COLORNUMA, COLOR_WHITE, COLOR_BLACK);
		} else {
			waddstr(stdscr,"can not init color!");
		}
	} else {
		endwin();
		printf("no color support on this terminal... \n");
	}

	while (1) {
		r = 0;
		c = 0;
		startcol = 0;
		curpanel = 0;
		// 1.1 Yes, the following lines are to show the header..
		if (lastch == SHOWPERC || showtype == SHOWPERC)
			mvprintw(r, c, " Total %lld cpus, show percentage for each cpu indicator, i.e. cpunr[", sstat->cpu.nrcpu);
		else
			//printw(" Total %lld cpus, each panel shows [sys|user|irq|iowait] 4 cpu indicators.\n", sstat->cpu.nrcpu);
			mvprintw(r, c, " Total %lld cpus, each panel shows [", sstat->cpu.nrcpu);
		attron(COLOR_PAIR(COLORSYSCPU));
		mvprintw(r, getcurx(stdscr), "sys");
		attroff(COLOR_PAIR(COLORSYSCPU));
		mvprintw(r, getcurx(stdscr), "|");
		attron(COLOR_PAIR(COLORUSERCPU));
		mvprintw(r, getcurx(stdscr), "user");
		attroff(COLOR_PAIR(COLORUSERCPU));
		mvprintw(r, getcurx(stdscr), "|");
		attron(COLOR_PAIR(COLORIRQCPU));
		mvprintw(r, getcurx(stdscr), "irq");
		attroff(COLOR_PAIR(COLORIRQCPU));
		mvprintw(r, getcurx(stdscr), "|");
		attron(COLOR_PAIR(COLORWAITCPU));
		mvprintw(r, getcurx(stdscr), "iowait");
		attroff(COLOR_PAIR(COLORWAITCPU));
		if (lastch == SHOWPERC || showtype == SHOWPERC)
			mvprintw(r, getcurx(stdscr), "]\n");
		else
			mvprintw(r, getcurx(stdscr), "] %d cpu indicators.\n", (COLORCPUMAX - 2));

		// 1.2 To print numa lines
		pernumablank = COLS / sstat->memnuma.nrnuma;
		for (i = 0; i < sstat->memnuma.nrnuma; i++) {
			attron(COLOR_PAIR(COLORNUMA));
			attron(A_BOLD);
			mvprintw(startrow, startcol + i * pernumablank, "[numa%d]", i);
			attroff(A_BOLD);
			attroff(COLOR_PAIR(COLORNUMA));
		}

		r = 1;
		c = 0;
		// 1.3 To print each cpu
		for (curnuma = 0; curnuma < sstat->memnuma.nrnuma; curnuma++) {
			for (curcpu = 0; curcpu < sstat->cpu.nrcpu; curcpu++) {
				cpunuma = sstat->cpu.cpu[curcpu].numanr;
				if (maxcpurows >= 2 && r > maxcpurows) {
					r = 1;
					curpanel++;
				}
				if (curnuma != cpunuma) {
					continue;
				}

				c = COLS / panels * curpanel;
				if (maxcpurows < 2 || (maxcpurows >= 2 && r <= maxcpurows)) {
					percputot = sstat->cpu.cpu[curcpu].stime + sstat->cpu.cpu[curcpu].utime +
						sstat->cpu.cpu[curcpu].ntime + sstat->cpu.cpu[curcpu].itime +
						sstat->cpu.cpu[curcpu].wtime + sstat->cpu.cpu[curcpu].Itime +
						sstat->cpu.cpu[curcpu].Stime + sstat->cpu.cpu[curcpu].steal;

					//0. handle 'cpu_nums and ['
					sprintf(buf, "%3d ", curcpu);
					attron(COLOR_PAIR(COLORMeter));
					mvprintw(startrow + r, startcol + c, buf);
					c += EACHCPUCOL;
					attron(A_BOLD);
					mvprintw(startrow + r, startcol + c, "[");
					attroff(A_BOLD);
					attroff(COLOR_PAIR(COLORMeter));
					c += 1;

					nextcol = c + eachblank;
					currow = startrow + r;

					//1. handle sys cpu
					percpusys = sstat->cpu.cpu[curcpu].stime * 100.0 / percputot;
					attron(COLOR_PAIR(COLORSYSCPU));
					if (lastch == SHOWPERC || showtype == SHOWPERC)
						eachcol = printcpuperc(percpusys, eachblank, currow, startcol + c, false);
					else
						eachcol = printperline(percpusys, eachblank, currow, startcol + c);
					attroff(COLOR_PAIR(COLORSYSCPU));
					c += eachcol;

					//2. handle user cpu
					percpuuser = sstat->cpu.cpu[curcpu].utime * 100.0 / percputot;
					attron(COLOR_PAIR(COLORUSERCPU));
					if (lastch == SHOWPERC || showtype == SHOWPERC)
						eachcol = printcpuperc(percpuuser, eachblank, currow, startcol + c, false);
					else
						eachcol = printperline(percpuuser, eachblank, currow, startcol + c);
					attroff(COLOR_PAIR(COLORUSERCPU));
					c += eachcol;

					//3. handle irq cpu
					percpuirq = (sstat->cpu.cpu[curcpu].Itime + sstat->cpu.cpu[curcpu].Stime) * 100.0 / percputot;
					attron(COLOR_PAIR(COLORIRQCPU));
					if (lastch == SHOWPERC || showtype == SHOWPERC)
						eachcol = printcpuperc(percpuirq, eachblank, currow, startcol + c, false);
					else
						eachcol = printperline(percpuirq, eachblank, currow, startcol + c);
					attroff(COLOR_PAIR(COLORIRQCPU));
					c += eachcol;

					//4. handle iowait cpu
					percpuiowait = sstat->cpu.cpu[curcpu].wtime * 100.0 / percputot;
					attron(COLOR_PAIR(COLORWAITCPU));
					if (lastch == SHOWPERC || showtype == SHOWPERC)
						eachcol = printcpuperc(percpuiowait, eachblank, currow, startcol + c, true);
					else
						eachcol = printperline(percpuiowait, eachblank, currow, startcol + c);
					attroff(COLOR_PAIR(COLORWAITCPU));
					c += eachcol;

					//5. handle the '] '
					attron(COLOR_PAIR(COLORMeter));
					attron(A_BOLD);
					//mvprintw(startrow + r, c, "] ");
					mvprintw(startrow + r, startcol + nextcol, "] ");
					attroff(A_BOLD);
					attroff(COLOR_PAIR(COLORMeter));

					r++;
				}
			}
		}

		switch(key = mvgetch(r, 0)) {
			case SHOWPERC:
				showtype = SHOWPERC;
				clear();
				lastch = SHOWPERC;
				break;
			case SHOWBAR:
				showtype = SHOWBAR;
				clear();
				lastch = SHOWBAR;
				break;
			case QUITINT:
				endwin();
				exit(0);
			default:
				showtype = SHOWBAR;
				clear();
				return '\0';
		}
	}

	return '\0';
}
