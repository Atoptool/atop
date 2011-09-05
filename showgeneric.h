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
#define USERSTUB	9999999
#define MAXUSERSEL	64

struct syscap {
	int	nrcpu;
	count_t	availcpu;
	count_t	availmem;
	count_t	availdsk;
	count_t	availnet;
};

struct selection {
	char	username[256];
	uid_t	userid[MAXUSERSEL];

	char	procname[64];
	int	procnamesz;
	regex_t	procregex;
};

/*
** color names
*/
#define	COLORLOW	2
#define	COLORMED	3
#define	COLORHIGH	4

/*
** list with keystrokes/flags
*/
#define	MPROCGEN	'g'
#define	MPROCMEM	'm'
#define	MPROCDSK	'd'
#define	MPROCNET	'n'
#define	MPROCSCH	's'
#define	MPROCVAR	'v'
#define	MPROCARG	'c'
#define MPROCOWN	'o'

#define	MCUMUSER	'u'
#define	MCUMPROC	'p'

#define	MSORTCPU	'C'
#define	MSORTDSK	'D'
#define	MSORTMEM	'M'
#define	MSORTNET	'N'
#define	MSORTAUTO	'A'

#define	MCOLORS		'x'
#define	MSYSFIXED	'f'
#define	MSYSLIMIT	'l'

#define	MSELPROC	'P'
#define	MSELUSER	'U'

#define	MALLPROC	'a'
#define	MKILLPROC	'k'
#define	MLISTFW		0x06
#define	MLISTBW		0x02
#define MREDRAW         0x0c
#define	MINTERVAL	'i'
#define	MPAUSE		'z'
#define	MQUIT		'q'
#define	MRESET		'r'
#define	MSAMPNEXT	't'
#define	MSAMPPREV	'T'
#define	MSAMPBRANCH	'b'
#define	MVERSION	'V'
#define	MAVGVAL		'1'
#define	MHELP1		'?'
#define	MHELP2		'h'

/*
** general function prototypes
*/
void	totalcap   (struct syscap *, struct sstat *, struct pstat *, int);
void	pricumproc (struct pstat *,  struct sstat *, int, int, int, int, 
             				   	int, int, int, int, int);

void	showgenproc(struct pstat *, double, int, int);
void	showmemproc(struct pstat *, double, int, int);
void	showdskproc(struct pstat *, double, int, int);
void	shownetproc(struct pstat *, double, int, int);
void	showvarproc(struct pstat *, double, int, int);
void	showschproc(struct pstat *, double, int, int);
void	showtotproc(struct pstat *, double, int, int);
void	showcmdproc(struct pstat *, double, int, int);

void	printg     (const char *, ...);
int	prisyst(struct sstat  *, int, int, int, int, int, char *,
                int, int, int, int, int);
int	priproc(struct pstat  *, int, int, int, int, int, char, char,
	        struct syscap *, struct selection *, int, int);
void	priphead(int, int, char, char, char);
