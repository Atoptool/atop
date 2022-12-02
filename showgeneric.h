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
#define MAXPID		32

struct syscap {
	int	nrcpu;
	count_t	availcpu;
	count_t	availmem;
	count_t	availdsk;
	count_t	availnet;
	int	nrgpu;
	count_t	availgpumem; 	// GPU memory in Kb!
	int	nrmemnuma;
	int	nrcpunuma;
};

struct pselection {
	char	username[256];
	uid_t	userid[MAXUSERSEL];

	pid_t	pid[MAXPID];

	char	progname[64];
	int	prognamesz;
	regex_t	progregex;

	char	argname[64];
	int	argnamesz;
	regex_t	argregex;

	char 	container[16];

	char	states[16];
};

struct sselection {
	char	lvmname[64];	// logical volume selection
	int	lvmnamesz;
	regex_t	lvmregex;

	char	dskname[64];	// disk selection
	int	dsknamesz;
	regex_t	dskregex;

	char	itfname[64];	// network interface selection
	int	itfnamesz;
	regex_t	itfregex;
};

/*
** color names
*/
#define	COLORINFO	2
#define	COLORALMOST	3
#define	COLORCRIT	4
#define	COLORTHR	5

/*
** list with keystrokes/flags
*/
#define	MPROCGEN	'g'
#define	MPROCMEM	'm'
#define	MPROCDSK	'd'
#define	MPROCNET	'n'
#define MPROCGPU	'e'
#define	MPROCSCH	's'
#define	MPROCVAR	'v'
#define	MPROCARG	'c'
#define	MPROCCGR	'X'
#define MPROCOWN	'o'

#define	MCUMUSER	'u'
#define	MCUMPROC	'p'
#define	MCUMCONT	'j'

#define	MSORTCPU	'C'
#define	MSORTDSK	'D'
#define	MSORTMEM	'M'
#define	MSORTNET	'N'
#define MSORTGPU	'E'
#define	MSORTAUTO	'A'

#define	MTHREAD		'y'
#define	MTHRSORT	'Y'
#define	MCALCPSS	'R'
#define	MGETWCHAN	'W'
#define	MSUPEXITS	'G'
#define	MCOLORS		'x'
#define	MSYSFIXED	'f'
#define	MSYSNOSORT	'F'
#define	MSYSLIMIT	'l'
#define MRMSPACES	'Z'

#define	MSELUSER	'U'
#define	MSELPROC	'P'
#define	MSELCONT	'J'
#define	MSELPID		'I'
#define	MSELARG		'/'
#define	MSELSTATE	'Q'
#define	MSELSYS		'S'

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
void	totalcap   (struct syscap *, struct sstat *, struct tstat **, int);
void	pricumproc (struct sstat *,  struct devtstat *,
				int, unsigned int, int, int);

void	showgenproc(struct tstat *, double, int, int);
void	showmemproc(struct tstat *, double, int, int);
void	showdskproc(struct tstat *, double, int, int);
void	shownetproc(struct tstat *, double, int, int);
void	showvarproc(struct tstat *, double, int, int);
void	showschproc(struct tstat *, double, int, int);
void	showtotproc(struct tstat *, double, int, int);
void	showcmdproc(struct tstat *, double, int, int);

void	printg     (const char *, ...);
int	prisyst(struct sstat  *, int, int, int, int, struct sselection *,
			char *, int, int, int, int, int, int, int, int, int, int, int, int);
int	priproc(struct tstat  **, int, int, int, int, int, char, char,
	        struct syscap *, int, int);
void	priphead(int, int, char *, char *, char, count_t);
