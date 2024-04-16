/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** Include-file describing cgroup-level counters to be maintained.
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        January/February 2024
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

#ifndef __CGROUPS__
#define __CGROUPS__

// structure containing general info and metrics per cgroup (directory)
//
struct cstat {
	// GENERAL INFO
	struct cggen {
		int	structlen;	// struct length including rounded name
		int	sequence;	// sequence number in chain/array
		int	parentseq;	// parent sequence number in chain/array
		int	depth;		// cgroup tree depth starting from 0
		int	nprocs;		// number of processes in cgroup
		int	procsbelow;	// number of processes in cgroups below
		int	namelen;	// cgroup name length (at end of struct)
		int	fullnamelen;	// cgroup path length
		int	ifuture[4];

		long	namehash;	// cgroup name hash of
					// full path name excluding slashes
		long	lfuture[4];
	} gen;

	// CONFIGURATION INFO
	struct cgconf {
		int	cpuweight;	// -1=max, -2=undefined
		int	cpumax;		// -1=max, -2=undefined (perc)

		count_t	memmax;		// -1=max, -2=undefined (pages)
		count_t	swpmax;		// -1=max, -2=undefined (pages)

		int	dskweight;	// -1=max, -2=undefined

		int	ifuture[5];
		count_t	cfuture[5];
	} conf;

	// CPU STATISTICS
	struct cgcpu {
		count_t	utime;		// time user   text (usec) -1=undefined
		count_t	stime;		// time system text (usec) -1=undefined

		count_t	somepres;	// some pressure (microsec)
		count_t	fullpres;	// full pressure (microsec)

		count_t	cfuture[5];
	} cpu;

	// MEMORY STATISTICS
	struct cgmem {
		count_t	current;	// current memory (pages)   -1=undefined
		count_t	anon;		// anonymous memory (pages) -1=undefined
		count_t	file;		// file memory (pages)      -1=undefined
		count_t	kernel;		// kernel memory (pages)    -1=undefined
		count_t	shmem;		// shared memory (pages)    -1=undefined

		count_t	somepres;	// some pressure (microsec)
		count_t	fullpres;	// full pressure (microsec)

		count_t	cfuture[5];
	} mem;

	// DISK I/O STATISTICS
	struct cgdsk {
		count_t	rbytes;		// total bytes read on all physical disks
		count_t	wbytes;		// total bytes written on all physical disks
		count_t	rios;		// total read I/Os on all physical disks
		count_t	wios;		// total write I/Os on all physical disks

		count_t	somepres;	// some pressure (microsec)
		count_t	fullpres;	// full pressure (microsec)

		count_t	cfuture[5];
	} dsk;

	// cgroup name with variable length
	char	cgname[];
};

// structure to be used for chaining the cstat struct and pid list
// that are maintained per cgroup
//
struct cgchainer {
	struct cgchainer	*next;
	struct cgchainer	*hashnext;

	struct cstat		*cstat;		// cgroup info and stats
	pid_t			*proclist;	// PID list of cgroup

	unsigned long		vlinemask;	// bit list for tree drawing:
						// bit '1' for continuous line
	char			stub;		// boolean for tree drawing:
						// true means corner
};

#define CGRMAXDEPTH   (sizeof(unsigned long)*8)	// #bits in vlinemask

// structure to be used for printing a merged list
// of cgroups and processes
//
struct tstat;

struct cglinesel {
	struct cgchainer *cgp;	// always filled with reference to cgroup
	struct tstat     *tsp;	// filled for process info, otherwise NULL
};

int	mergecgrouplist(struct cglinesel **, int,
			struct cgchainer **, int,
			struct tstat **, int, char);

int              cgroupv2support(void);
void             photocgroup(void);
int              deviatcgroup(struct cgchainer **, int *);
struct cgchainer **cgsort(struct cgchainer *, int, char);
char             *cggetpath(struct cgchainer *, struct cgchainer *, int);
void             cgwipecur(void);
void             cgbuildarray(struct cgchainer **, char *, char *, int);
void		 cgfillref(struct devtstat *, struct cgchainer *, int, int);


#endif
