/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-/thread-level.
**
** This source-file contains functions to read and handle cgroup metrics.
** ==========================================================================
** Author:      	Gerlof Langeveld
** E-mail:      	gerlof.langeveld@atoptool.nl
** Initial date:	January 2024
** --------------------------------------------------------------------------
** Copyright (C) 2024 Gerlof Langeveld
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
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <regex.h>

#include "atop.h"
#include "cgroups.h"
#include "photosyst.h"
#include "photoproc.h"
#include "showgeneric.h"

static void		cgcalcdeviate(void);

static void		cgrewind(struct cgchainer **);
static struct cgchainer	*cgnext(struct cgchainer **, struct cgchainer **);
static void		cgwipe(struct cgchainer **, struct cgchainer **,
			       struct cgchainer **, struct cgchainer **);

static unsigned long	walkcgroup(char *, struct cgchainer *, int, long, int, int);
static void		getconfig(struct cstat *, struct cstat *);
static int		readconfigval(char *, count_t []);
static void		getmetrics(struct cstat *);
static void		getpressure(char *, count_t *, count_t *);

static long		hashcalc(char *, long, int);
static void		hashadd(struct cgchainer *[], struct cgchainer *);
static struct cgchainer	*hashfind(struct cgchainer *[], long);

static int		cgroupfilter(struct cstat *, int, char);

#define	CGROUPROOT	"/sys/fs/cgroup"

#define	CGROUPNHASH	128	// power of 2
#define	CGROUPMASK	(CGROUPNHASH-1)


// cgroup administration
// =====================
// three types of cgroup collections are maintained:
//
// - current linked list
//   -------------------
//   this administration consists of a chain of cgchainer structs
//   that each refer to a cstat struct containing the cgroup metrics
//  
//   this chain is built while gathering the current information by the
//   walkcgroup() function
//   this function mallocs one cgchainer struct and one cstat struct
//
// - previous linked list
//   --------------------
//   this administration consists of a chain of cgchainer structs
//   that each refer to a cstat struct containing the previous cgroup metrics
//
//   before building a new current chain, the current chain will be
//   saved as the previous chain
//
// - deviation array
//   ---------------
//   this administration consists of an array(!) of cgchainer structs in which
//   the cgchainer struct are still chained to the next element in the array
//

// current cgroup admi
//
static struct cgchainer	*cgcurfirst,	// first in linked list
			*cgcurlast,	// last  in linked list
			*cgcurcursor;

static unsigned int	cgcursize; 	// total size of all cstat structs
static unsigned int	cgcurnum; 	// number of cstat structs
static unsigned int	cgcurprocs; 	// number of processes in current list

static int		cgcursequence;	// maintain sequence number to be
					// assigned to every cgchainer struct
					//
					// later on this value can be used
					// as index in the deviation array

// previous cgroup admi
//
static struct cgchainer	*cgprefirst,	// first in linked list
			*cgprelast,	// last  in linked list
			*cgprecursor,
			*cgprehash[CGROUPNHASH];

// deviation cgroup admi
//
static struct cgchainer	*cgdevfirst,	// pointer to array (!)
			*cgdevcursor;


// Check if cgroup v2 is supported on this machine
//
// Return value:	1 - yes
// 			0 - no
//
int
cgroupv2support(void)
{
	FILE	*fp;
	char	line[128];

	// check if this kernel offers cgroups version 2
	//
	if ( (fp = fopen("/proc/1/cgroup", "r")) )
	{
		while (fgets(line, sizeof line, fp))
		{
			if (memcmp(line, "0::", 3) == 0) // equal?
			{
				supportflags |= CGROUPV2;
				break;
			}
		}

		fclose(fp);
	}

	return (supportflags&CGROUPV2) > 0;
}


// Gather all metrics from the entire cgroup tree,
// creating a chain of cgchainer structs
//
void
photocgroup(void)
{
	char		origdir[4096];

	// wipe previous cgroup chain (not needed any more)
	//
	cgwipe(&cgprefirst, &cgprelast, &cgprecursor, cgprehash);

	// move current cgroup chain to previous cgroup chain
	//
	cgprefirst  = cgcurfirst;
	cgprelast   = cgcurlast;

	cgcurfirst  = cgcurlast = cgcurcursor = NULL;

	// wipe deviation cgroup memory 
	// - wipe all contiguous cstat structs (start address in first cgchainer)
	// - wipe pidlist (start address in first cgchainer)
	// - wipe all contiguous cgchainer structs
	//
	if (cgdevfirst)
	{
		free(cgdevfirst->cstat);
		free(cgdevfirst->proclist);
		free(cgdevfirst);
	}

	// save current directory and move to top directory
	// of cgroup fs
	//
	if ( getcwd(origdir, sizeof origdir) == NULL)
		mcleanstop(53, "failed to save current dir\n");

	if ( chdir(CGROUPROOT) == -1)
		mcleanstop(54, "failed to change to " CGROUPROOT "\n");

	// read all subdirectory names below the cgroup top directory
	//
	cgcursequence = 0;
	cgcursize     = 0;
	cgcurnum      = 0;
	cgcurprocs    = 0;

	walkcgroup(".", NULL, -1, 0, 0, 0);

	// return to original directory
	//
	if ( chdir(origdir) == -1)
		mcleanstop(55, "cannot change to %s\n", origdir);
}


// Gather statistics from one cgroup directory level
// and walk the directory tree further downwards by nested calls.
// Every call to this function will add one struct to the current
// cgroup chain.
//
// Return value:	number of processes in this dir and the dirs underneath
// 			-1 = fail
//
static unsigned long
walkcgroup(char *dirname, struct cgchainer *cparent, int parentseq,
         long upperhash, int upperlen, int depth)
{
	FILE		*fp;
	DIR		*dirp;
	struct dirent	*entp;

	int		namelen = strlen(dirname);
	int 		cstatlen, i;

	unsigned long	procsbelow=0, proccnt=0, hash;

	struct cgchainer	*ccp;

	// change to new directory
	//
	if ( chdir(dirname) == -1)
		return -1;

	// --------------------------------------------
	// gather statistics for this cgroup directory
	// --------------------------------------------
	// - create new cgchainer struct
	//
	ccp = calloc(1, sizeof(struct cgchainer));
	ptrverify(ccp, "Malloc failed for current cgchainer\n");

	// - add cgchainer to current chain
	//
	if (cgcurfirst)
	{
		cgcurlast->next = ccp;
		cgcurlast       = ccp;
	}
	else
	{
		cgcurfirst = ccp;
		cgcurlast  = ccp;
	}

	// - create corresponding cstat struct
	//   with a rounded length to avoid unaligned structs later on
	//
        cstatlen = sizeof(struct cstat) + namelen + 1;

	if (cstatlen & 0x7)	// length is not 64-bit multiple?
		cstatlen = ((cstatlen >> 3) + 1) << 3;	// round up to 64-bit

	ccp->cstat = calloc(1, cstatlen);
	ptrverify(ccp->cstat, "Malloc failed for cstat of %d bytes\n", cstatlen);

	cgcursize += cstatlen;
	cgcurnum++;

	// - determine number of processes in this cgroup directory
	//
	if ( (fp = fopen("cgroup.procs", "r")) )
	{
		char line[64];

		while (fgets(line, sizeof line, fp))
			proccnt++;

		fclose(fp);
	}

	// - create corresponding pid list to cgchainer
	//   and fill it with the PIDs
	//
	if (proccnt)
	{
        	ccp->proclist = malloc(sizeof(pid_t) * proccnt);
		ptrverify(ccp->proclist,
		          "Malloc failed for proclist (%d pids)\n", proccnt);
	}
	else
	{
        	ccp->proclist = NULL;
	}

	i = 0;

	if ( (fp = fopen("cgroup.procs", "r")) )
	{
		char line[64];

		while (fgets(line, sizeof line, fp))
		{
			*(ccp->proclist+i) = strtol(line, NULL, 10);

			if (++i >= proccnt)
				break;
		}

		fclose(fp);
	}

	proccnt = i;	// number of processes might have shrunk

	cgcurprocs += proccnt;
	
	// - fill basic info in current cgchainer
	//
	strcpy(ccp->cstat->cgname, dirname);	// copy directory name

	if (*dirname == '.')			// top directory?
	{
		ccp->cstat->cgname[0] = '\0';	// wipe name
		namelen               = 0;
	}

	hash = hashcalc(ccp->cstat->cgname, upperhash, upperlen);

	ccp->cstat->gen.structlen   = cstatlen;
	ccp->cstat->gen.sequence    = cgcursequence++;	// assign unique sequence number
	ccp->cstat->gen.parentseq   = parentseq;
	ccp->cstat->gen.depth       = depth;
	ccp->cstat->gen.nprocs      = proccnt;
	ccp->cstat->gen.namelen     = namelen;
	ccp->cstat->gen.fullnamelen = upperlen + namelen; // excluding slashes
	ccp->cstat->gen.namehash    = hash;

	// - gather and store current cgroup configuration
	//
	getconfig(ccp->cstat, cparent ? cparent->cstat : NULL);

	// - gather and store current cgroup metrics
	//
	getmetrics(ccp->cstat);

	// --------------------------------------------
	// walk subdirectories by nested calls
	// --------------------------------------------
	//
	dirp = opendir(".");

	while ( (entp = readdir(dirp)) )
	{
		// skip dot files/directories
		//
		if (entp->d_name[0] == '.')
			continue;

		// check if this entry represents a subdirectory
		// to be walked as well
		//
		if (entp->d_type == DT_DIR)
			procsbelow += walkcgroup(entp->d_name,
				ccp, ccp->cstat->gen.sequence,
				hash, upperlen+namelen, depth+1);
	}

	closedir(dirp);

	chdir("..");

	ccp->cstat->gen.procsbelow = procsbelow;

	return procsbelow + proccnt;
}


// Gather configuration values for one specific cgroup
// value -2:	undefined
// value -1:	maximum
//
static void
getconfig(struct cstat *csp, struct cstat *csparent)
{
	count_t	retvals[2];

	// get cpu.weight 
	//
	csp->conf.cpuweight = -2;		// initial value (undefined)

	switch (readconfigval("cpu.weight", retvals))
	{
	   case 1:
		csp->conf.cpuweight = retvals[0];
		break;
	}

	// get cpu.max limitation
	//
	csp->conf.cpumax = -2;			// initial value (undefined)

	switch (readconfigval("cpu.max", retvals))
	{
	   case 2:
		if (retvals[0] == -1)
			csp->conf.cpumax = -1;	// max
		else
			csp->conf.cpumax = retvals[0] * 100 / retvals[1];
		break;
	}

	// get io.bpf.weight 
	//
	csp->conf.dskweight = -2;		// initial value (undefined)

	switch (readconfigval("io.bfq.weight", retvals))
	{
	   case 2:
		csp->conf.dskweight = retvals[1];
		break;
	}

	// get memory.max limitation
	//
	csp->conf.memmax = -2;			// initial value (undefined)

	switch (readconfigval("memory.max", retvals))
	{
	   case 1:
		if (retvals[0] == -1)
			csp->conf.memmax = -1;	// max
		else
			csp->conf.memmax = retvals[0] / pagesize;
		break;
	}

	// get memory.swap.max limitation
	//
	csp->conf.swpmax = -2;			// initial value (undefined)

	switch (readconfigval("memory.swap.max", retvals))
	{
	   case 1:
		if (retvals[0] == -1)
			csp->conf.swpmax = -1;	// max
		else
			csp->conf.swpmax = retvals[0] / pagesize;
		break;
	}
}


// read line with one or two values and
// fill one or (maximum) two values
// in retvals (value 'max' converted into -1)
//
// return value:	number of entries in retvals filled
//
static int
readconfigval(char *fname, count_t retvals[])
{
	char line[64];
	int  n;
	FILE *fp;

	if ( (fp = fopen(fname, "r")) )
	{
		char firststr[16];

		if (!fgets(line, sizeof line, fp))
		{
			fclose(fp);
			return 0;
		}

		fclose(fp);

		switch (n = sscanf(line, "%15s %llu", firststr, &retvals[1]))
		{
		   case 0:
			return 0;
		   case 1:
		   case 2:
			if ( strcmp(firststr, "max") == 0)
				retvals[0] = -1;
			else
				retvals[0] = atol(firststr);
			return n;
		   default:
			return 0;
		}
	}

	return 0;
}


// Gather metrics for one specific cgroup
//
static void
getmetrics(struct cstat *csp)
{
	FILE	*fp;
	char	line[256];
	int	cnt;

	// gather CPU metrics
	//
	csp->cpu.utime = -1;	// undefined
	csp->cpu.stime = -1;	// undefined
	cnt = 0;

	if ( (fp = fopen("cpu.stat", "r")) )
	{
		while (fgets(line, sizeof line, fp) && cnt < 2)
		{
			if (memcmp(line, "user_usec ", 10) == 0)
			{
				sscanf(line, "user_usec %lld", &(csp->cpu.utime));
				cnt++;
				continue;
			}

			if (memcmp(line, "system_usec ", 12) == 0)
			{
				sscanf(line, "system_usec %lld", &(csp->cpu.stime));
				cnt++;
				continue;
			}
                }

		fclose(fp);
	}

	getpressure("cpu.pressure", &(csp->cpu.somepres), &(csp->cpu.fullpres));

	// gather memory metrics
	//
	csp->mem.current = -1;	// undefined

	csp->mem.anon    = -1;	// undefined
	csp->mem.file    = -1;	// undefined
	csp->mem.kernel  = -1;	// undefined
	csp->mem.shmem   = -1;	// undefined

	cnt = 0;

	if ( (fp = fopen("memory.current", "r")) )
	{
		if (fgets(line, sizeof line, fp) != NULL)
			csp->mem.current = strtoll(line, NULL, 10) / pagesize;

		fclose(fp);
	}

	if ( (fp = fopen("memory.stat", "r")) )
	{
		while (fgets(line, sizeof line, fp) && cnt < 4)
		{
			if (memcmp(line, "anon ", 5) == 0)
			{
				sscanf(line, "anon %lld", &(csp->mem.anon));
				csp->mem.anon /= pagesize;
				cnt++;
				continue;
			}

			if (memcmp(line, "file ", 5) == 0)
			{
				sscanf(line, "file %lld", &(csp->mem.file));
				csp->mem.file /= pagesize;
				cnt++;
				continue;
			}

			if (memcmp(line, "kernel ", 7) == 0)
			{
				sscanf(line, "kernel %lld", &(csp->mem.kernel));
				csp->mem.kernel /= pagesize;
				cnt++;
				continue;
			}

			if (memcmp(line, "shmem ", 6) == 0)
			{
				sscanf(line, "shmem %lld", &(csp->mem.shmem));
				csp->mem.shmem /= pagesize;
				cnt++;
				continue;
			}
                }

		fclose(fp);
	}

	getpressure("memory.pressure", &(csp->mem.somepres), &(csp->mem.fullpres));

	// gather disk I/O metrics
	//
	//     253:2 rbytes=2544128 wbytes=2192896 rios=313 wios=22 dbytes=0 dios=0
	//     253:1 rbytes=143278080 wbytes=3843604480 rios=34222 wios=853554 ...
	//     253:0 rbytes=19492288000 wbytes=105266814976 rios=386493 wios=1691322
	//     11:0
	//     8:0 rbytes=328956266496 wbytes=109243312640 rios=764129 wios=1415575
	//
	csp->dsk.rbytes = 0;
	csp->dsk.wbytes = 0;
	csp->dsk.rios   = 0;
	csp->dsk.wios   = 0;

	if ( (fp = fopen("io.stat", "r")) )
	{
		int	major, minor;
		count_t	rbytes, wbytes, rios, wios;

		while (fgets(line, sizeof line, fp))
		{
			if (sscanf(line,
				"%d:%d rbytes=%lld wbytes=%lld rios=%lld wios=%lld",
					&major, &minor,
					&rbytes, &wbytes, &rios, &wios) == 6)
			{
				if (isdisk_major(major) == DSKTYPE)
				{
					csp->dsk.rbytes += rbytes;
					csp->dsk.wbytes += wbytes;

					csp->dsk.rios   += rios;
					csp->dsk.wios   += wios;
				}
			}
		}

		fclose(fp);
	}
	else
	{
		csp->dsk.rbytes = -1;	// undefined
		csp->dsk.wbytes = -1;	// undefined
		csp->dsk.rios   = -1;	// undefined
		csp->dsk.wios   = -1;	// undefined
	}

	getpressure("io.pressure", &(csp->dsk.somepres), &(csp->dsk.fullpres));
}

// Get total pressure values from file with a format similar to:
//
//     some avg10=0.00 avg60=0.00 avg300=0.00 total=9660682563
//     full avg10=0.00 avg60=0.00 avg300=0.00 total=9376892229 
//
static void
getpressure(char *fname, count_t *some, count_t *full)
{
	char 	psiformat[] = "%c%*s avg10=%f avg60=%f avg300=%f total=%llu",
		linebuf[256], psitype;
	float	a10, a60, a300;
	FILE	*fp;

	*some = -1;	// initially undefined
	*full = -1;	// initially undefined

	if ( (fp = fopen(fname, "r")) != NULL)
	{
		// handle first line: 'some' pressure
		//
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			sscanf(linebuf, psiformat,
				&psitype, &a10, &a60, &a300, some);
                }

		// handle second line: 'full' pressure
		//
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			sscanf(linebuf, psiformat,
				&psitype, &a10, &a60, &a300, full);
                }

		fclose(fp);
	}
}


// Calculate deviations between current cgroup chain and
// previous cgroup chain.
// Notice that the deviation chain structure is identical to
// the current chain structure, so they can be walked alongside.
// Also the deviation cstat struct is already a copy of the current
// cstat structure.
//
static void
cgcalcdeviate(void)
{
	struct cgchainer	*dp, *pp;
	int			insync = 1, prevfound;

	// walk thru the deviation chain and calculate the differences
	// with the previous chain
	//
	// when no cgroups have been added or removed since the
	// previous sample, we might assume that the previous chain
	// has the same order of elements as the deviation chain (insync)
	//
	// when cgroups have been added or removed since the
	// previous sample (!insync), we build a hash list for the
	// previous chain to find the correct entry based on the hashed name
	//
	cgrewind(&cgdevcursor);
	cgrewind(&cgprecursor);

	while ( (dp = cgnext(&cgdevfirst, &cgdevcursor)) )
	{
		// find previous cgroup struct
		//
		prevfound = 0;

		if (insync)
		{
			// assume unchanged cgroup chain: take next from previous chain
			//
			pp = cgnext(&cgprefirst, &cgprecursor);

			if (pp && dp->cstat->gen.namehash == pp->cstat->gen.namehash)
			{
				prevfound = 1;
			}
			else
			{
				insync = 0;

				// build hash list for previous chain to find names
				// based on hashed name for the rest of this loop
				//
				cgrewind(&cgprecursor);

				while ( (pp = cgnext(&cgprefirst, &cgprecursor)) )
					hashadd(cgprehash, pp);	
			}
		}

		if (!insync)
		{
			// modified cgroup chain: search previous by name hash
			//
			if ( (pp = hashfind(cgprehash, dp->cstat->gen.namehash)) )
				prevfound = 1;
		}

		// calculate deviations related to previous sample
		//
		if (prevfound)
		{
			if (dp->cstat->cpu.utime != -1)		// defined?
				dp->cstat->cpu.utime	-= pp->cstat->cpu.utime;

			if (dp->cstat->cpu.stime != -1)		// defined?
				dp->cstat->cpu.stime	-= pp->cstat->cpu.stime;

			if (dp->cstat->cpu.somepres != -1)	// defined?
				dp->cstat->cpu.somepres	-= pp->cstat->cpu.somepres;

			if (dp->cstat->cpu.fullpres != -1)	// defined?
				dp->cstat->cpu.fullpres	-= pp->cstat->cpu.fullpres;


			if (dp->cstat->mem.somepres != -1)	// defined?
				dp->cstat->mem.somepres	-= pp->cstat->mem.somepres;

			if (dp->cstat->mem.fullpres != -1)	// defined?
				dp->cstat->mem.fullpres	-= pp->cstat->mem.fullpres;


			if (dp->cstat->dsk.rbytes != -1)	// defined?
			{
				dp->cstat->dsk.rbytes	-= pp->cstat->dsk.rbytes;
				dp->cstat->dsk.wbytes	-= pp->cstat->dsk.wbytes;
				dp->cstat->dsk.rios	-= pp->cstat->dsk.rios;
				dp->cstat->dsk.wios	-= pp->cstat->dsk.wios;
			}

			if (dp->cstat->dsk.somepres != -1)	// defined?
				dp->cstat->dsk.somepres	-= pp->cstat->dsk.somepres;

			if (dp->cstat->dsk.fullpres != -1)	// defined?
				dp->cstat->dsk.fullpres	-= pp->cstat->dsk.fullpres;
		}
	}
}

// Rewind cglist
//
static void
cgrewind(struct cgchainer **cursor)
{
	*cursor = NULL;
}

// Get next cgchainer from current cglist
//
static struct cgchainer *
cgnext(struct cgchainer **first, struct cgchainer **cursor)
{
	if (! *cursor)
		*cursor = *first;
	else
		*cursor = (*cursor)->next;

	if (*cursor)
		return *cursor;
	else
		return NULL;
}


// Wipe current cgroup chain 
//
void
cgwipecur(void)
{
	cgwipe(&cgcurfirst, &cgcurlast, &cgcurcursor, NULL);
}


// Wipe all struct's from cgroup admi
//
static void
cgwipe(struct cgchainer **first,  struct cgchainer **last,
       struct cgchainer **cursor, struct cgchainer **hashlist)
{
	struct cgchainer *cp, *cpn;

	if (! *first)	// empty already?
		return;

	// free all chain members
	//
	for (cp=*first; cp; cp=cpn)
	{
		cpn = cp->next;

		if (cp->proclist)
			free(cp->proclist);

		free(cp->cstat);
		free(cp);
	}

	*first = *last = *cursor = NULL;

	if (hashlist)
		memset(hashlist, 0, sizeof(struct cgchainer *) * CGROUPNHASH);
}


// Create enough memory to copy all cstat structures from
// the current list to one contiguous area.
// Also create enough memory to copy all pid lists from
// the current list to one contiguous area.
// Next, create and fill an array of cgchainer structs.
//
// Returns:	number of cgchainer structs
//
int
deviatcgroup(struct cgchainer **cdpp, int *npids)
{
	struct cgchainer	*ccp = cgcurfirst;
	char			*allc, *allp, *cp, *pp;

	// allocate contiguous memory areas for all cstat structs
	// and all pid lists
	//
	allc = calloc(1, cgcursize);
	allp = malloc(sizeof(pid_t) * cgcurprocs);

	ptrverify(allc, "Malloc failed for contiguous cstats (%d bytes)\n", cgcursize);
	ptrverify(allp, "Malloc failed for contiguous pids (%d pids)\n", cgcurprocs);

	// concatenate all current cstat structs and all pid lists
	// into the new memory areas
	//
	ccp = cgcurfirst;
	cp  = allc;
	pp  = allp;

	while (ccp)
	{
		int cstatlen = ccp->cstat->gen.structlen;
		int plistlen = ccp->cstat->gen.nprocs * sizeof(pid_t);

		memcpy(cp, ccp->cstat, cstatlen);
		cp += cstatlen;

		if (ccp->proclist)
		{
			memcpy(pp, ccp->proclist, plistlen);
			pp += plistlen;
		}

		ccp = ccp->next;
	}

	// build array of cgchainer structs and a hash list for the
	// concatenated cstat structs for the deviation values
	//
	cgbuildarray(&cgdevfirst, allc, allp, cgcurnum);

	// calculate deviation values
	//
	cgcalcdeviate();

	*cdpp  = cgdevfirst;
	*npids = cgcurprocs;

	return cgcurnum;
}


// Create concatenated array of cgchainer structs referring to
// proper location in the concatenated cstat structs and
// the concatenated pid lists.
//
void
cgbuildarray(struct cgchainer **firstp, char *cstats, char *pids, int ncstats)
{
	struct cgchainer	*cdp;
	int			i;

	*firstp = malloc(sizeof(struct cgchainer) * ncstats);
	ptrverify(firstp, "Malloc failed for contiguous cgchainers (%d)\n", ncstats);

	for (cdp=*firstp, i=0; i < ncstats; cdp++, i++)
	{
		cdp->next     = cdp+1;
		cdp->hashnext = NULL;

		cdp->proclist = (pid_t *)pids;
		pids += sizeof(pid_t) * ((struct cstat *)cstats)->gen.nprocs;

		cdp->cstat    = (struct cstat *)cstats;
		cstats += ((struct cstat *)cstats)->gen.structlen;
	}

	cdp = *firstp + ncstats - 1;
	cdp->next = NULL;	// terminate chain
}


// Assemble full pathname of cgroup directory (from cgroup top directory)
// from the deviation array.
// For JSON output, double escape characters (backslashes) can be requested
// vi boolean 'escdouble'.
//
// Returns:	malloc'ed string with full path name to be freed later on
//
#define	MAXESCAPES	16

char *
cggetpath(struct cgchainer *cdp, struct cgchainer *cdbase, int escdouble)
{
	// calculate path length including slashes (depth) and terminating 0-byte
	//
	// in case of double escapes, add a (hopefully) worst-case number of
	// bytes to the malloc'ed buffer to double the backslashes
	//
	// in case of the top directory (without a parent), reserve one extra
	// byte for the '/' character
	//
	int	pathlen = cdp->cstat->gen.fullnamelen +
		          cdp->cstat->gen.depth   + 1 + 
			 (cdp->cstat->gen.sequence ? 0 : 1);

	char    *path = calloc(1, pathlen);
	char    *ps;

	ptrverify(path, "Malloc failed for concatenated cgpath (%d)\n", pathlen);

	// not the top directory?
	//
	if (cdp->cstat->gen.sequence)
	{
		// any other path: place all path components
		//                 including preceding slashes
		//
		while (cdp->cstat->gen.parentseq != -1)
		{
			// determine location of preceding slash
			// of this specific path component
			//
			ps = path + cdp->cstat->gen.fullnamelen -
			    	cdp->cstat->gen.namelen     +
			    	cdp->cstat->gen.depth       - 1;
	
			// store slash and path component
			//
			*ps = '/';
			memcpy(ps+1, cdp->cstat->cgname, cdp->cstat->gen.namelen);

			// climb upwards
			//
			cdp = cdbase + cdp->cstat->gen.parentseq;
		}

		*(path+pathlen-1) = '\0';	// terminate string
	}
	else	// exceptional case only for top directory
	{
		*path     = '/';
		*(path+1) = '\0';       // terminate string
	}

	// when double escapes are required and at least one backslash
	// is present, reallocate and convert the assembled path
	//
	// room for a worst-case number of backslashes is allocated
	// to avoid counting backslashes before transferring all bytes
	//
	if (escdouble && strchr(path, '\\'))
	{
		char	*escpath, *pi, *po;

		pathlen += MAXESCAPES;

	       	escpath  = malloc(pathlen);
		ptrverify(escpath, "Malloc failed for escape cgpath (%d)\n", pathlen);

		pi = path;
		po = escpath;

		while (*pi)
		{
			*po++ = *pi;

			if (*pi++ == '\\')	// insert additional backslash
				*po++ = '\\';

			if (po - escpath >= pathlen-1)
				break; // truncate to avoid overflow
		}

		*po = 0;

		free(path);	// free original string
		path = escpath;
	}

	return path;
}


// ===========================================================
// name hashing to find cgroup with specific path name
// ===========================================================

// Calculate hash for directory name.
// The 'basehash' contains the hashed value of the
// previous path components and the 'offset' is
// the offset of this (sub)string in a larger path.
// Notice that '/' characters will be ignored during
// calculation of the hash value (and ignored in the offset).
//
static long
hashcalc(char *p, long basehash, int offset)
{
	offset += 1;	// avoid multiplication by 0

	while (*p)
	{
		if (*p == '/')
		{
			p++;
			continue;
		}

		basehash += *p++ * offset++;
	}

	return basehash;
}


// Add new hash cgroup directory name to hash
//
static void
hashadd(struct cgchainer *hashlist[], struct cgchainer *cp)
{
	long hash = cp->cstat->gen.namehash;

	cp->hashnext = hashlist[hash&CGROUPMASK];
	hashlist[hash&CGROUPMASK] = cp;
}


// Find directory name in hash based on hashed name value
//
static struct cgchainer *
hashfind(struct cgchainer *hashlist[], long hash)
{
	struct cgchainer *cp;

	// search hash list
	//
	for (cp = hashlist[hash&CGROUPMASK]; cp; cp = cp->hashnext)
	{
		if (hash == cp->cstat->gen.namehash)
			return cp;
	}

	return NULL;
}


// ===========================================================
// PID hashing to find the processes that are related to
// a particular cgroup 
// ===========================================================
struct pid2tstat {
	struct pid2tstat	*hashnext;
	struct tstat		*tstat;
};

#define	PIDNHASH	512	// power of 2
#define	PIDMASK		(PIDNHASH-1)

static int	compselcpu(const void *, const void *);
static int	compselmem(const void *, const void *);
static int	compseldsk(const void *, const void *);


// Create a mixed list with cgroups and
// processes belonging to those cgroups
//
// Return:	number of entries (= screen lines) in the list
//
int
mergecgrouplist(struct cglinesel **cgroupselp, int newdepth,
		struct cgchainer **cgchainerp, int ncgroups,
		struct tstat **tpp, int nprocs, char showorder)
{
	int			ic, ip, im, is;
	struct cglinesel	*cgroupsel;
	struct pid2tstat	*pidhash[PIDNHASH], *pidlist, *pidcur;

	// create a hashlist to find the PIDs in a fast way
	//
	if (newdepth == 8 || newdepth == 9)	// depth requires processes?
	{
		// initialize hash list
		//
		memset(pidhash, 0, sizeof pidhash);

		// create one pid2tstat struct per tstat struct
		//
		pidlist = calloc(nprocs, sizeof(struct pid2tstat));

		ptrverify(pidlist,
			  "Malloc for pid2tstat structs failed (%d)\n", nprocs);

		pidcur = pidlist;

		// build hash list
		//
		for (ip=0; ip < nprocs; ip++, tpp++, pidcur++)
		{
			int hash = (*tpp)->gen.pid&PIDMASK;

			pidcur->hashnext = pidhash[hash];
			pidcur->tstat    = (*tpp);

			pidhash[hash]    = pidcur;
		}
	}
	else
	{
		nprocs = 0;	// ignore processes
	}

	// allocate space for merged list
	//
	cgroupsel = malloc(sizeof(struct cglinesel) * (ncgroups+nprocs));

	ptrverify(cgroupsel, "Malloc for cglinesel structs failed (%d)\n",
					ncgroups + nprocs);

	*cgroupselp = cgroupsel;

	// fill merged list
	//
	for (ic=im=0; ic < ncgroups; ic++)
	{
		int	cgroupnprocs;

		// take next cgroup and add it to the merged list
		//
		if ( cgroupfilter((*(cgchainerp+ic))->cstat,
							newdepth, showorder) )
		{
			(cgroupsel+im)->cgp = *(cgchainerp+ic);
			(cgroupsel+im)->tsp = NULL;

			im++;
		}
		else
		{
			// suppress this cgroup
			// when it is a stub cgroup for this level,
			// pass the stub to the previous valid entry
			// of this level
			//
			if ( (*(cgchainerp+ic))->stub)
			{
				int j;

				for (j=im-1; j > 0; j--)
				{
					int depth = (*(cgchainerp+ic))->cstat->gen.depth;

			     		if (depth > (cgroupsel+j)->cgp->cstat->gen.depth)
					{
						break;
					}

			     		if (depth == (cgroupsel+j)->cgp->cstat->gen.depth)
					{
						(cgroupsel+j)->cgp->stub = 7;
						break;
					}
					else
					{
						if (depth < CGRMAXDEPTH)
			     			   (cgroupsel+j)->cgp->vlinemask &= ~(1 << (depth-1));
					}
				}
			}

			continue;
		}
		
		// if relevant, search for processes that belong to this cgroup
		// and add them to the merged list
		//
		if (!nprocs)		// ignore processes anyhow?
			continue;

		cgroupnprocs = (*(cgchainerp+ic))->cstat->gen.nprocs;

		if (!cgroupnprocs)	// no processes in this cgroup?
			continue;

		for (ip=0, is=im; ip < cgroupnprocs; ip++)
		{
			int pid  = (*(cgchainerp+ic))->proclist[ip];
			int hash = pid&PIDMASK;

			// search for tstat struct relate to this PID via hashlist
			//
			for (pidcur=pidhash[hash]; pidcur; pidcur=pidcur->hashnext)
			{
				// process tstat found??
				//
				if (pidcur->tstat->gen.pid == pid)
				{
					// skip kernel processes in case of level 8
					//
					if (newdepth == 9              ||
					    pidcur->tstat->mem.vmem > 0  )
					{
						(cgroupsel+im)->cgp = *(cgchainerp+ic);
						(cgroupsel+im)->tsp = pidcur->tstat;

						im++;
					}
					break;
				}
			}
		}

		// sort the list of processes added for this cgroup
		//
		if (im-is > 1)
		{
			switch (showorder)
			{
		   	   case MSORTCPU:
				qsort(cgroupsel+is, im-is,
					sizeof(struct cglinesel), compselcpu);
				break;

		   	   case MSORTMEM:
				qsort(cgroupsel+is, im-is,
					sizeof(struct cglinesel), compselmem);
				break;

		   	   case MSORTDSK:
				qsort(cgroupsel+is, im-is,
					sizeof(struct cglinesel), compseldsk);
				break;
			}
		}
	}

	if (nprocs)
		free(pidlist);

	return im;
}


// Judge if a cgroup should be selected, based on the
// current requested directory level (depth) and
// based on the fact if unused branches are requested
//
// returns 0 (false) or 1 (true)
//
static int
cgroupfilter(struct cstat *csp, int newdepth, char showorder)
{
	// skip lower level of cgroups when required
	// by entering key 2 till 8 (9=max)
	//
	if (newdepth <  9 &&
	    newdepth <= csp->gen.depth)
		return 0;

	// skip this level and lower levels if
	// no processes are assigned at all
	//
	if (deviatonly               &&
	    showorder != MSORTMEM    &&
	    csp->gen.nprocs     == 0 &&
	    csp->gen.procsbelow == 0   )
		return 0;

	return 1;
}


// Funtion to be called by qsort() to compare the
// CPU time consumption of two processes in a cgroup.
//
static int
compselcpu(const void *a, const void *b)
{
	count_t acpu = ((struct cglinesel *)a)->tsp->cpu.utime +
	               ((struct cglinesel *)a)->tsp->cpu.stime;
	count_t bcpu = ((struct cglinesel *)b)->tsp->cpu.utime +
	               ((struct cglinesel *)b)->tsp->cpu.stime;

        if (acpu < bcpu)
                return  1;

        if (acpu > bcpu)
                return -1;

        return 0;
}


// Funtion to be called by qsort() to compare the
// memory consumption of two processes in a cgroup.
//
static int
compselmem(const void *a, const void *b)
{
	count_t amem = ((struct cglinesel *)a)->tsp->mem.rmem;
	count_t bmem = ((struct cglinesel *)b)->tsp->mem.rmem;

        if (amem < bmem)
                return  1;

        if (amem > bmem)
                return -1;

        return 0;
}


// Funtion to be called by qsort() to compare the
// disk activity of two processes in a cgroup.
//
static int
compseldsk(const void *a, const void *b)
{
	count_t adisk = ((struct cglinesel *)a)->tsp->dsk.rsz +
	                ((struct cglinesel *)a)->tsp->dsk.wsz;
	count_t bdisk = ((struct cglinesel *)b)->tsp->dsk.rsz +
	                ((struct cglinesel *)b)->tsp->dsk.wsz;

        if (adisk < bdisk)
                return  1;

        if (adisk > bdisk)
                return -1;

        return 0;
}


// Generate a list of pointers to cgchainer struct, sorted on
// specific resource consumption in the cgroup
//
struct cgsorter {
	struct cgchainer	*cgthis;

	struct cgsorter		*cgsame;
	struct cgsorter		*cgchild;
	struct cgsorter 	**sortlist;
	count_t			sortval;
	int			nrchild;
};

static struct cgsorter		cgroot;		// struct for root with depth zero

static struct cgchainer *sortlevel(int, struct cgsorter *, struct cgchainer *, int, char);
static struct cgchainer **mergelevels(struct cgsorter *, int);
static int		mergelevel(struct cgsorter *, struct cgchainer **, unsigned long);
static void		createsortlist(struct cgsorter *);
static int              compsortval(const void *, const void *);


// Main function to create a list of pointers to cgchainer structs,
// in the order of resource consumption while maintaining
// the proper cgroup directory structure.
//
// Return value: list with sorted pointers to cgchainer structs
//
struct cgchainer **
cgsort(struct cgchainer *cgphys, int cgsize, char showorder)
{
	struct cgchainer **cgsorted;

	// reinitialize the root cgsorter struct and fill
	// the pointer to the root cgchainer
	//
	memset(&cgroot, 0, sizeof cgroot);

	cgroot.cgthis = cgphys;

	// build a tree structure reflecting the directories of the cgroups
	// (skip the first entry representing the root cgroup)
	//
	sortlevel(1, &cgroot, &cgphys[1], cgsize-1, showorder);

	// merge all sorted cgchainer struct pointers into one
	// contiguous list
	//
	cgsorted = mergelevels(&cgroot, cgsize);

	return cgsorted;
}


// Create a tree structure of cgsorter structs (by nested calls).
// A cgsorter struct will be created per cgchainer struct,
// with the following members:
//
//	cgthis		pointer to corresponding cgchainer struct
//	cgsame		pointer to next cgsorter struct on same directory level
//	cgchild		pointer to first cgsorter struct on lower directory level
//	sortlist	pointer to list of pointers to child cgsorter structs
//			(same pointers as chained via cgchild) to be sorted
//	sortval		sort value (e.g. consumed CPU time or used memory)
//	nrchild		number of direct child members
//
static struct cgchainer *
sortlevel(int curlevel, struct cgsorter *cgparent,
		        struct cgchainer *cgp, int cgsize, char showorder)
{
	int              newlevel, cgleft = cgsize;
	struct cgchainer *cgc = cgp;
	struct cgsorter  *cgs = NULL;

	// as long as cgchainer structs are left...
	//
	while (cgleft > 0)
	{
		newlevel = cgc->cstat->gen.depth;

		// higher level in directory tree?
		// - create and sort the cgchainer pointers in order of resource utilization
		// - return to caller to continue with higher level
		//
		if (newlevel < curlevel)
		{
			createsortlist(cgparent);
			return cgc;	// leave this level
		}

		// same directory level: chain to same-level list
		// which is the child list of the higher level that called us 
		//
		if (newlevel == curlevel)
		{
			// malloc and fill new cgsorter struct
			//
			cgs = malloc(sizeof(struct cgsorter));
			ptrverify(cgs, "Malloc failed for cgsorter struct\n");

			cgs->cgthis  = cgc;
			cgs->cgsame  = cgparent->cgchild;
			cgs->cgchild = 0;
			cgs->nrchild = 0;

			switch (showorder)
			{
			   case MSORTCPU:
				cgs->sortval = cgc->cstat->cpu.utime +
				               cgc->cstat->cpu.stime;
				break;
			   case MSORTMEM:
				if (cgc->cstat->mem.current > 0)
				{
					cgs->sortval = cgc->cstat->mem.current;
				}
				else
				{
					cgs->sortval = cgc->cstat->mem.anon +
					               cgc->cstat->mem.file +
					               cgc->cstat->mem.kernel +
					               cgc->cstat->mem.shmem;
				}
				break;
			   case MSORTDSK:
				cgs->sortval = cgc->cstat->dsk.rbytes +
				               cgc->cstat->dsk.wbytes;
				break;
			   default:
				cgs->sortval = 0;	// no sorting
			}

			// chain cgsorter struct to this level
			//
			cgparent->cgchild = cgs;
			cgparent->nrchild++;

			cgc++;
			cgleft--;

			continue;
		}

		// newlevel > curlevel?
		// recursively call sortlevel() for each new
		// directory level underneath
		//
		cgc = sortlevel(newlevel, cgs, cgc, cgleft, showorder);
		cgleft = cgsize - (cgc - cgp);
	}

	createsortlist(cgparent);

	return cgc;
}


// Create a list of pointers to the cgchainer structs
// on the current level and sort it on resource consumption
//
static void
createsortlist(struct cgsorter *cgparent)
{
	int 		i;
       	struct cgsorter	*cgs;

	if (cgparent->nrchild == 1)	// no sorting needed with one child
	{	
		cgparent->sortlist = NULL;
	}
	else				// sorting needed by creating sortlist
	{
		cgparent->sortlist = calloc(cgparent->nrchild,
					sizeof(struct cgsorter *));
		ptrverify(cgparent->sortlist, "Malloc failed for cgsorter list\n");

		// fill elements of sortlist with pointers to child cgsorters
		//
		for (i=0, cgs=cgparent->cgchild; i < cgparent->nrchild;
							i++, cgs = cgs->cgsame)
		{
			*((cgparent->sortlist)+i) = cgs;
		}

		// sort list on sort value
		//
		qsort(cgparent->sortlist, cgparent->nrchild,
			sizeof(struct cgsorter *), compsortval);
	}
}

// Function to be called by qsort() to compare
// the sortvalue in two cgsorter structs
//
static int
compsortval(const void *a, const void *b)
{
        struct cgsorter *cga = *(struct cgsorter **)a;
        struct cgsorter *cgb = *(struct cgsorter **)b;

	if (cga->sortval < cgb->sortval)
		return 1;
	if (cga->sortval > cgb->sortval)
		return -1;
	return 0;
}


// Gather all sorted cgsorter structs from the entire tree
// and create one array of cgchainer pointers in sorted order.
//
static struct cgchainer **
mergelevels(struct cgsorter *cgrootp, int cgsize)
{
	struct cgchainer	**cgpp;
	unsigned long		vlinemask = 0;

	// allocate the list of pointers to be returned
	//
	cgpp = malloc(sizeof(struct cgchainer *) * cgsize);
	ptrverify(cgpp, "Malloc failed for cgchainer ptr list (%d)\n", cgsize);

	// fill the first entry with the root cgchainer struct pointer
	//
	*cgpp = cgrootp->cgthis;
        (*cgpp)->stub = 1;	// no more entries on this level
	(*cgpp)->vlinemask = vlinemask;

	mergelevel(cgrootp, cgpp+1, vlinemask);

	return cgpp;
}


// Gather all cgsorter structs from one directory level.
// To be called in a nested way for each level underneath.
//
static int 
mergelevel(struct cgsorter *cgparent, struct cgchainer **cgpp,
					unsigned long vlinemask)
{
	struct cgsorter *cgs, *cgsave;
	int 		i, j, depth = cgparent->cgthis->cstat->gen.depth;

	switch (cgparent->nrchild)
	{
	   case 0:	// should never occur....
		j = 0;
		break;

	   case 1:	// in case of one child no sortlist has been created
		*cgpp = cgparent->cgchild->cgthis;
		(*cgpp)->stub = 1;	// no more entries on this level

		if (depth < CGRMAXDEPTH)
			vlinemask &= ~(1 << depth);

		(*cgpp)->vlinemask = vlinemask;

		j = 1;

		if (cgparent->cgchild->nrchild)	// merge descendants on lower level?
			j += mergelevel(cgparent->cgchild, cgpp+1, vlinemask);

		free(cgparent->cgchild);
		break;

           default:
		for (i=0, j=0; i < cgparent->nrchild; i++, j++)
		{
			cgs = *((cgparent->sortlist)+i);

			*(cgpp+j) = cgs->cgthis;

			if (i == cgparent->nrchild -1)	// last on this level?
			{
				(*(cgpp+j))->stub = 1;	// no more entries on this level

				if (depth < CGRMAXDEPTH)
					vlinemask &= ~(1 << depth);

				(*(cgpp+j))->vlinemask = vlinemask;
			}
			else
			{
				(*(cgpp+j))->stub = 0;	// more entries on this level

				if (depth < CGRMAXDEPTH)
					vlinemask |= 1 << depth;

				(*(cgpp+j))->vlinemask = vlinemask;
			}

			if (cgs->nrchild)	// merge descendants on lower level?
				j += mergelevel(cgs, cgpp+j+1, vlinemask);
		}

		// for all cgroups of this level merged: free malloced areas
		//
		free(cgparent->sortlist);

		for (cgs=cgparent->cgchild; cgs; cgs=cgsave)
		{
			cgsave = cgs->cgsame;
			free(cgs);
		}
	}

	return j;
}


// ===========================================================
// PID hashing to find the cgroup that is related to
// a particular process 
// ===========================================================
struct pid2cgchainer {
	struct pid2cgchainer	*hashnext;
	pid_t			pid;
	int			cgindex;	// cgchainer index
};

// For every tstat struct, fill the reference (index) to
// the related cgroup, represented by the cgchainer struct
//
void
cgfillref(struct devtstat *devtstat, struct cgchainer *devchain,
					int ncgroups, int npids)
{
	int			ic, ip, it, hash;
	pid_t			pid;
	struct pid2cgchainer	*pidhash[PIDNHASH], *pidlist, *pidcur;
	struct cgchainer	*cp;
	struct tstat		*tp = devtstat->taskall;

	// create a hashlist to find the PIDs in a fast way
	// initialize hash list
	//
	memset(pidhash, 0, sizeof pidhash);

	// create one pid2cgchainer struct per cgroup pid
	//
	pidlist = calloc(npids, sizeof(struct pid2cgchainer));

	ptrverify(pidlist, "Malloc for pid2cgchainer structs failed (%d)\n", npids);

	// build hash list to find right cgchainer (cgroup) for a PID
	//
	for (ic=0, cp=devchain, pidcur=pidlist; ic < ncgroups; ic++, cp++)
	{
		for (ip=0;  ip < cp->cstat->gen.nprocs; ip++, pidcur++)
		{
			pid  = cp->proclist[ip];
			hash = pid & PIDMASK;

			pidcur->hashnext = pidhash[hash];
			pidcur->pid      = pid;
			pidcur->cgindex  = ic;

			pidhash[hash]    = pidcur;
		}
	}

	// connect every tstat struct to the concerning cgchainer
	// by filling the index
	//
	for (it=0; it < devtstat->ntaskall; it++, tp++)
	{
		tp->gen.cgroupix = -1;

		if (! tp->gen.isproc)	// skip threads
			continue;

		pid  = tp->gen.pid;
		hash = pid & PIDMASK;

		// search for cgchainer index related to this PID via hashlist
		//
		for (pidcur=pidhash[hash]; pidcur; pidcur=pidcur->hashnext)
		{
			if (pidcur->pid == pid)
			{
				tp->gen.cgroupix = pidcur->cgindex;
				break;
			}
		}
	}

	free(pidlist);
}
