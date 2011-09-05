/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** Include-file describing miscellaneous constants and function-prototypes.
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
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
#define	EQ		0
#define SECSDAY		86400
#define RAWNAMESZ	256

/*
** memory-size formatting possibilities
*/
#define	ANYFORMAT	0
#define	KBFORMAT	1
#define	MBFORMAT	2
#define	GBFORMAT	3
#define	TBFORMAT	4
#define	OVFORMAT	9

typedef	long long	count_t;

struct pstat;
struct sstat;

/* 
** miscellaneous flags
*/
#define RRBOOT  0x0001
#define RRLAST  0x0002

struct visualize {
	char	(*show_samp)  (time_t, int, struct sstat *, struct pstat *,
				int, int, int, int, int, int, int, char);
	void	(*show_error) (const char *, ...);
	void	(*show_end)   (void);
	void	(*show_usage) (void);
};

/*
** external values
*/
extern struct utsname   utsname;
extern int              utsnodenamelen;
extern time_t   	pretime;
extern time_t   	curtime;
extern unsigned long    interval;
extern unsigned long	sampcnt;
extern char      	screen;
extern int      	linelen;
extern char		deviatonly;
extern char		rawname[];
extern char		rawreadflag;
extern unsigned int	begintime, endtime;
extern char		flaglist[];
extern struct visualize vis;

extern int      	osrel;
extern int		osvers;
extern int      	ossub;

extern unsigned short	hertz;
extern unsigned int	pagesize;

extern int		supportflags;

extern int		cpubadness;
extern int		membadness;
extern int		swpbadness;
extern int		dskbadness;
extern int		netbadness;
extern int		pagbadness;
extern int		almostcrit;

/*
** bit-values for supportflags
*/
#define	ACCTACTIVE	0x00000001
#define	PATCHSTAT	0x00000002
#define	IOSTAT		0x00000004
#define	PATCHACCT	0x00000008

/*
** structure containing the start-addresses of functions for visualization
*/
char		generic_samp (time_t, int, struct sstat *, struct pstat *,
		             int, int, int, int, int, int, int, char);
void		generic_error(const char *, ...);
void		generic_end  (void);
void		generic_usage(void);

/*
** miscellaneous prototypes
*/
int		atopsar(int, char *[]);
char   		*convtime(time_t, char *);
char   		*convdate(time_t, char *);
int   		hhmm2secs(char *, unsigned int *);
int		daysecs(time_t);

char   		*val2valstr(count_t, char *, int, int, int);
char   		*val2memstr(count_t, char *, int, int, int);
char		*val2cpustr(count_t, char *);
char            *val2Hzstr(count_t, char *);
int             val2elapstr(int, char *);

int		compcpu(const void *, const void *);
int		compdsk(const void *, const void *);
int		compmem(const void *, const void *);
int		compnet(const void *, const void *);
int		compusr(const void *, const void *);
int		compnam(const void *, const void *);

int		cpucompar (const void *, const void *);
int		diskcompar(const void *, const void *);
int		intfcompar(const void *, const void *);

count_t		subcount(count_t, count_t);
void  		rawread(void);
char		rawwrite(time_t, int, struct sstat *, struct pstat *,
		                       int, int, int, int, int, int, int, char);

int 		numeric(char *);
void		getalarm(int);
time_t		getboot(void);
char 		*getstrvers(void);
unsigned short 	getnumvers(void);
void		cleanstop(int);
void		prusage(char *);
