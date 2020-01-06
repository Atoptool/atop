/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** Include-file describing prototypes and structures for visualization
** of counters.
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        July 2002
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
int 	parsedef(char *);
char	parseout(time_t, int,
		struct devtstat *, struct sstat *,
		int, unsigned int, char);

void	calc_freqscale(count_t maxfreq, count_t cnt, count_t ticks, 
count_t *freq, int *freqperc);
void 	print_CPU(char *, struct sstat *, struct tstat *, int);
void 	print_cpu(char *, struct sstat *, struct tstat *, int);
void 	print_CPL(char *, struct sstat *, struct tstat *, int);
void 	print_GPU(char *, struct sstat *, struct tstat *, int);
void 	print_MEM(char *, struct sstat *, struct tstat *, int);
void 	print_SWP(char *, struct sstat *, struct tstat *, int);
void 	print_PAG(char *, struct sstat *, struct tstat *, int);
void 	print_PSI(char *, struct sstat *, struct tstat *, int);
void 	print_LVM(char *, struct sstat *, struct tstat *, int);
void 	print_MDD(char *, struct sstat *, struct tstat *, int);
void 	print_DSK(char *, struct sstat *, struct tstat *, int);
void 	print_NFM(char *, struct sstat *, struct tstat *, int);
void 	print_NFC(char *, struct sstat *, struct tstat *, int);
void 	print_NFS(char *, struct sstat *, struct tstat *, int);
void 	print_NET(char *, struct sstat *, struct tstat *, int);
void 	print_IFB(char *, struct sstat *, struct tstat *, int);

void 	print_PRG(char *, struct sstat *, struct tstat *, int);
void 	print_PRC(char *, struct sstat *, struct tstat *, int);
void 	print_PRM(char *, struct sstat *, struct tstat *, int);
void 	print_PRD(char *, struct sstat *, struct tstat *, int);
void 	print_PRN(char *, struct sstat *, struct tstat *, int);
void 	print_PRE(char *, struct sstat *, struct tstat *, int);
