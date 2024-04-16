/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** Include-file describing process-level counters maintained and functions
** to access the process-database.
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

#ifndef __PHOTOPROC__
#define __PHOTOPROC__

#define	PNAMLEN		15
#define	CMDLEN		255
#define	CGRLEN		64
#define	UTSLEN		15

/* 
** structure containing only relevant process-info extracted 
** from kernel's process-administration
*/
struct tstat {
	/* GENERAL TASK INFO 					*/
	struct gen {
		int	tgid;		/* threadgroup identification 	*/
		int	pid;		/* process identification 	*/
		int	ppid;           /* parent process identification*/
		int	ruid;		/* real  user  identification 	*/
		int	euid;		/* eff.  user  identification 	*/
		int	suid;		/* saved user  identification 	*/
		int	fsuid;		/* fs    user  identification 	*/
		int	rgid;		/* real  group identification 	*/
		int	egid;		/* eff.  group identification 	*/
		int	sgid;		/* saved group identification 	*/
		int	fsgid;		/* fs    group identification 	*/
		int	nthr;		/* number of threads in tgroup 	*/
		char	name[PNAMLEN+1];/* process name string       	*/
		char 	isproc;		/* boolean: process level?      */
		char 	state;		/* process state ('E' = exited)	*/
		int	excode;		/* process exit status		*/
		time_t 	btime;		/* process start time (epoch)	*/
		time_t 	elaps;		/* process elaps time (hertz)	*/
		char	cmdline[CMDLEN+1];/* command-line string       	*/
		int	nthrslpi;	/* # threads in state 'S'       */
		int	nthrslpu;	/* # threads in state 'D'       */
		int	nthrrun;	/* # threads in state 'R'       */
		int	nthridle;	/* # threads in state 'I'	*/

		int	ctid;		/* OpenVZ container ID		*/
		int	vpid;		/* OpenVZ virtual PID		*/

		int	wasinactive;	/* boolean: task inactive	*/

		char	utsname[UTSLEN+1];/* UTS name container or pod  */

		int	cgroupix;	/* index in devchain -1=invalid */
					/* lazy filling (parsable/json) */
		int	ifuture[4];	/* reserved for future use	*/
	} gen;

	/* CPU STATISTICS						*/
	struct cpu {
		count_t	utime;		/* time user   text (ticks) 	*/
		count_t	stime;		/* time system text (ticks) 	*/
		int	nice;		/* nice value                   */
		int	prio;		/* priority                     */
		int	rtprio;		/* realtime priority            */
		int	policy;		/* scheduling policy            */
		int	curcpu;		/* current processor            */
		int	sleepavg;       /* sleep average percentage     */
		int	ifuture[6];	/* reserved for future use	*/
		char	wchan[16];	/* wait channel string    	*/
		count_t	rundelay;	/* schedstat rundelay (nanosec)	*/
		count_t	blkdelay;	/* blkio delay (ticks)		*/
		count_t nvcsw;		/* voluntary cxt switch counts  */
		count_t nivcsw;		/* involuntary csw counts       */
		count_t	cfuture[3];	/* reserved for future use	*/
	} cpu;

	/* DISK STATISTICS						*/
	struct dsk {
		count_t	rio;		/* number of read requests 	*/
		count_t	rsz;		/* cumulative # sectors read	*/
		count_t	wio;		/* number of write requests 	*/
		count_t	wsz;		/* cumulative # sectors written	*/
		count_t	cwsz;		/* cumulative # written sectors */
					/* being cancelled              */
		count_t	cfuture[4];	/* reserved for future use	*/
	} dsk;

	/* MEMORY STATISTICS						*/
	struct mem {
		count_t	minflt;		/* number of page-reclaims 	*/
		count_t	majflt;		/* number of page-faults 	*/
		count_t	vexec;		/* virtmem execfile (Kb)        */
		count_t	vmem;		/* virtual  memory  (Kb)	*/
		count_t	rmem;		/* resident memory  (Kb)	*/
		count_t	pmem;		/* resident memory  (Kb)	*/
		count_t vgrow;		/* virtual  growth  (Kb)    	*/
		count_t rgrow;		/* resident growth  (Kb)     	*/
		count_t vdata;		/* virtmem data     (Kb)     	*/
		count_t vstack;		/* virtmem stack    (Kb)     	*/
		count_t vlibs;		/* virtmem libexec  (Kb)     	*/
		count_t vswap;		/* swap space used  (Kb)     	*/
		count_t	vlock;		/* virtual locked   (Kb) 	*/
		count_t	cfuture[7];	/* reserved for future use	*/
	} mem;

	/* NETWORK STATISTICS						*/
	struct net {
		count_t tcpsnd;		/* number of TCP-packets sent	*/
		count_t tcpssz;		/* cumulative size packets sent	*/
		count_t	tcprcv;		/* number of TCP-packets recved	*/
		count_t tcprsz;		/* cumulative size packets rcvd	*/
		count_t	udpsnd;		/* number of UDP-packets sent	*/
		count_t udpssz;		/* cumulative size packets sent	*/
		count_t	udprcv;		/* number of UDP-packets recved	*/
		count_t udprsz;		/* cumulative size packets sent	*/
		count_t	avail1;		/* */
		count_t	avail2;		/* */
		count_t	cfuture[4];	/* reserved for future use	*/
	} net;

	struct gpu {
		char	state;		// A - active, E - Exit, '\0' - no use
		char	bfuture[3];	//
		short	nrgpus;		// number of GPUs for this process
		int32_t	gpulist;	// bitlist with GPU numbers

		int	gpubusy;	// gpu busy perc process lifetime      -1 = n/a
		int	membusy;	// memory busy perc process lifetime   -1 = n/a
		count_t	timems;		// milliseconds accounting   -1 = n/a
					// value 0   for active process,
					// value > 0 after termination

		count_t	memnow;		// current    memory consumption in KiB
		count_t	memcum;		// cumulative memory consumption in KiB
		count_t	sample;		// number of samples
		count_t	cfuture[3];	//
	} gpu;
};


struct pinfo {
	struct pinfo	*phnext;	/* next process in hash    chain */
	struct pinfo	*prnext;	/* next process in residue chain */
	struct pinfo	*prprev;	/* prev process in residue chain */

	struct tstat	tstat;		/* per-process statistics        */
};

/*
** structure to maintains all deviation info related to one sample
*/
struct devtstat {
        struct tstat     *taskall;	// all processes (also exited) and threads
        struct tstat    **procall;	// all processes (also exited)
        struct tstat    **procactive;	// all processes (also exited) being active

	unsigned long	ntaskall;
        unsigned long	ntaskactive;
	unsigned long	nprocall;
	unsigned long	nprocactive;

        unsigned long   totrun, totslpi, totslpu, totidle, totzombie;
};

/*
** prototypes of process-database functions
*/
int		pdb_gettask(int, char, time_t, struct pinfo **);
void		pdb_addtask(int, struct pinfo *);
int		pdb_deltask(int, char);
int		pdb_makeresidue(void);
int		pdb_cleanresidue(void);
int		pdb_srchresidue(struct tstat *, struct pinfo **);

/*
** prototypes for raw process-statistics functions
*/
struct netpertask;

void		deviattask(struct tstat *, unsigned long,
 		           struct tstat *, unsigned long, 
 		           struct devtstat *, struct sstat *);

unsigned long	photoproc(struct tstat *, int);
unsigned long	counttasks(void);

#endif
