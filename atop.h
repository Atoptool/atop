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
#ifndef __ATOP__
#define __ATOP__

#include <time.h>

#define	EQ		0
#define SECONDSINDAY	86400
#define RAWNAMESZ	256

/*
** memory-size formatting possibilities
*/
#define	BFORMAT		0
#define	KBFORMAT	1
#define	KBFORMAT_INT	2
#define	MBFORMAT	3
#define	MBFORMAT_INT	4
#define	GBFORMAT	5
#define	GBFORMAT_INT	6
#define	TBFORMAT	7
#define	TBFORMAT_INT	8
#define	PBFORMAT	9
#define	PBFORMAT_INT	10
#define	EBFORMAT	11
#define	EBFORMAT_INT	12
#define	OVFORMAT	13

typedef	long long	count_t;

struct tstat;
struct devtstat;
struct sstat;
struct cgchainer;
struct netpertask;

/* 
** miscellaneous flags
*/
#define RRBOOT		0x0001
#define RRLAST  	0x0002
#define RRNETATOP	0x0004
#define RRNETATOPD	0x0008
#define RRACCTACTIVE	0x0010
#define RRIOSTAT	0x0020
#define RRCONTAINERSTAT	0x0040
#define RRGPUSTAT	0x0080
#define RRCGRSTAT	0x0100

struct visualize {
	char	(*show_samp)  (time_t, int,
	                struct devtstat *, struct sstat *, struct cgchainer *,
			int, int, int, unsigned int, char);
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
extern int 		fdinotify;
extern pid_t 		twinpid;
extern int      	linelen;
extern char      	acctreason;
extern char		deviatonly;
extern char		usecolors;
extern char		threadview;
extern char		calcpss;
extern char		getwchan;
extern char		rawname[];
extern char		rawreadflag;
extern char		rmspaces;
extern time_t		begintime, endtime, cursortime;	// epoch or time in day
extern char		flaglist[];
extern struct visualize vis;

extern char		displaymode;
extern char		barmono;

extern int      	osrel;
extern int		osvers;
extern int      	ossub;

extern unsigned short	hertz;
extern unsigned int	pagesize;
extern unsigned int	pidwidth;
extern unsigned int	nrgpus;

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
#define	IOSTAT		0x00000004
#define	NETATOP		0x00000010
#define	NETATOPD	0x00000020
#define	CONTAINERSTAT	0x00000040
#define	GPUSTAT		0x00000080
#define	CGROUPV2	0x00000100
#define	NETATOPBPF	0x00001000


/*
** in rawlog file, the four least significant bits 
** are moved to the per-sample flags and therefor dummy
** in the support flags of the general header
*/
#define	RAWLOGNG	(ACCTACTIVE|IOSTAT|NETATOP|NETATOPD)

/*
** structure containing the start-addresses of functions for visualization
*/
char		generic_samp (time_t, int,
		            struct devtstat *, struct sstat *,
			    struct cgchainer *, int, int,
		            int, unsigned int, char);
void		generic_error(const char *, ...);
void		generic_end  (void);
void		generic_usage(void);

/*
** miscellaneous prototypes
*/
int		atopsar(int, char *[]);
char   		*convtime(time_t, char *);
char   		*convdate(time_t, char *);
int   		getbranchtime(char *, time_t *);
time_t		normalize_epoch(time_t, long);


char   		*val2valstr(count_t, char *, int, int, int);
char   		*val2memstr(count_t, char *, int, int, int);
char		*val2cpustr(count_t, char *);
char            *val2Hzstr(count_t, char *);
int             val2elapstr(int, char *);

int		compcpu(const void *, const void *);
int		compdsk(const void *, const void *);
int		compmem(const void *, const void *);
int		compnet(const void *, const void *);
int		compgpu(const void *, const void *);
int		compusr(const void *, const void *);
int		compnam(const void *, const void *);
int		compcon(const void *, const void *);

int		cpucompar (const void *, const void *);
int		gpucompar (const void *, const void *);
int		diskcompar(const void *, const void *);
int		intfcompar(const void *, const void *);
int		ifbcompar(const void *, const void *);
int		nfsmcompar(const void *, const void *);
int		contcompar(const void *, const void *);
int		memnumacompar(const void *, const void *);
int		cpunumacompar(const void *, const void *);
int		llccompar(const void *, const void *);

int  		rawread(void);
char		rawwrite (time_t, int,
		            struct devtstat *, struct sstat *,
			    struct cgchainer *, int, int,
		            int, unsigned int, char);

int 		numeric(char *);
void		getalarm(int);
unsigned long long	getboot(void);
char 		*getstrvers(void);
unsigned short 	getnumvers(void);
void		ptrverify(const void *, const char *, ...);
void		mcleanstop(int, const char *, ...);
void		cleanstop(int);
int		getpidwidth(void);
void		prusage(char *);

int		rootprivs(void);
int		droprootprivs(void);
void		regainrootprivs(void);
FILE 		*fopen_tryroot(const char *, const char *);

void		netatop_ipopen(void);
void		netatop_probe(void);
void		netatop_signoff(void);
void		netatop_gettask(pid_t, char, struct tstat *);
unsigned int	netatop_exitstore(void);
void		netatop_exiterase(void);
void		netatop_exithash(char);
void		netatop_exitfind(unsigned long, struct tstat *, struct tstat *);

void		netatop_bpf_ipopen(void);
void		netatop_bpf_probe(void);
void		netatop_bpf_gettask(void);
void		netatop_bpf_exitfind(unsigned long, struct tstat *, struct tstat *);

void		set_oom_score_adj(void);
int		run_in_guest(void);

void		getusr1(int), getusr2(int);
void		do_pacctdir(char *, char *);
void		do_atopsarflags(char *, char *);

int		netlink_open(void);
int		netlink_recv(int, int);

int		getutsname(struct tstat *);
void		resetutsname(void);

#endif
