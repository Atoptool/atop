/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of 
** the system on system-level as well as process-level.
**
** This program converts a raw logfile created by a particular version
** of atop to (by default) the current version of atop.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Initial:     July/August 2018
** --------------------------------------------------------------------------
** Copyright (C) 2018-2019 Gerlof Langeveld
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
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <string.h>
#include <regex.h>
#include <zlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdarg.h>

#include "atop.h"
#include "photosyst.h"
#include "photoproc.h"

#include "prev/photosyst_20.h"
#include "prev/photoproc_20.h"

#include "prev/photosyst_21.h"
#include "prev/photoproc_21.h"

#include "prev/photosyst_22.h"
#include "prev/photoproc_22.h"

#include "prev/photosyst_23.h"
#include "prev/photoproc_23.h"

#include "prev/photosyst_24.h"
#include "prev/photoproc_24.h"


///////////////////////////////////////////////////////////////
// Conversion functions
// --------------------
// The structures with system level and process level info
// consists of sub-structures, generally for cpu, memory,
// disk and network values. These sub-structures might have
// changed from a specific version of atop to the next version.
// For modified sub-structures, conversion functions have to be
// written. These conversion functions will be called in a chained
// way for one version increment at the time. Suppose that a
// raw log is offered that has been created by atop 2.0 to be
// converted to atop 2.3, every sub-structure will be converted
// from 2.0 to 2.1, from 2.1 to 2.2 and finally from 2.2 to 2.3.
// When a sub-structure has NOT been changed from one version to
// another, it will just be copied by the generic conversion
// function 'justcopy' to the proper location in the structure
// of the next version.
// All conversion steps are controlled by the convs[] table.
///////////////////////////////////////////////////////////////

// Generic function that just copies an old structure byte-wise to
// a new structure (new structure implicitly padded with binary zeroes)
//
void
justcopy(void *old, void *new, count_t oldsize, count_t newsize)
{
	if (oldsize)
		memcpy(new, old, newsize > oldsize ? oldsize : newsize);
}

// Specific functions that convert an old sstat sub-structure to
// a new sub-structure (system level)
//
void
scpu_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// cfuture[1] --> cfuture[4] in struct percpu
	//
	struct cpustat_20	*c20 = old;
	struct cpustat_21	*c21 = new;
	int			i;

	memcpy(c21, c20, (char *)&c20->all - (char *)c20);	// base values
	memcpy(&c21->all, &c20->all, sizeof(struct percpu_20));

	for (i=0; i < MAXCPU_20; i++)
	    memcpy( &(c21->cpu[i]), &(c20->cpu[i]), sizeof(struct percpu_20));
}

void
sdsk_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// MAXDSK and MAXLVM have been enlarged
	//
	struct dskstat_20	*d20 = old;
	struct dskstat_21	*d21 = new;

	d21->ndsk 	= d20->ndsk;
	d21->nmdd 	= d20->nmdd;
	d21->nlvm 	= d20->nlvm;

	memcpy(d21->dsk, d20->dsk, sizeof d20->dsk);
	memcpy(d21->mdd, d20->mdd, sizeof d20->mdd);
	memcpy(d21->lvm, d20->lvm, sizeof d20->lvm);
}

void
sint_to_22(void *old, void *new, count_t oldsize, count_t newsize)
{
	// members 'type' and 'speedp' added to struct perintf
	//
	struct intfstat_21	*i21 = old;
	struct intfstat_22	*i22 = new;
	int			i;

	i22->nrintf 	= i21->nrintf;

	for (i=0; i < MAXINTF_21; i++)
	{
		memcpy(&(i22->intf[i]), &(i21->intf[i]),
					sizeof(struct perintf_21));

		// correct last members by refilling
		// 
		i22->intf[i].type 	= '?';
		i22->intf[i].speed	= i21->intf[i].speed;
		i22->intf[i].speedp	= i21->intf[i].speed;
		i22->intf[i].duplex	= i21->intf[i].duplex;

		memset(i22->intf[i].cfuture, 0, sizeof i22->intf[i].cfuture);
	}
}

// Specific functions that convert an old tstat sub-structure to
// a new sub-structure (process level)
//
void
tgen_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// member 'envid' inserted in struct gen
	//
	struct gen_20	*g20 = old;
	struct gen_21	*g21 = new;

	memcpy(g21, g20, (char *)g20->ifuture - (char *)g20);	// base values
	g21->envid = 0;
}

void
tmem_to_21(void *old, void *new, count_t oldsize, count_t newsize)
{
	// members 'pmem' and 'cfuture[4]' inserted in struct mem
	//
	struct mem_20	*m20 = old;
	struct mem_21	*m21 = new;

	m21->minflt	= m20->minflt;
	m21->majflt	= m20->majflt;
	m21->vexec	= m20->vexec;
	m21->vmem	= m20->vmem;
	m21->rmem	= m20->rmem;
	m21->pmem	= 0;
	m21->vgrow	= m20->vgrow;
	m21->rgrow	= m20->rgrow;
	m21->vdata	= m20->vdata;
	m21->vstack	= m20->vstack;
	m21->vlibs	= m20->vlibs;
	m21->vswap	= m20->vswap;
}

void
tgen_to_22(void *old, void *new, count_t oldsize, count_t newsize)
{
	// member 'envid' removed, members 'ctid' and 'vpid' inserted in struct gen
	//
	struct gen_21	*g21 = old;
	struct gen_22	*g22 = new;

	memcpy(g22, g21, (char *)&g21->envid - (char *)g21);	// copy base values
	g22->ctid = g21->envid;
	g22->vpid = 0;
}

///////////////////////////////////////////////////////////////
// conversion definition for various structs in sstat and tstat
//
#define	SETVERSION(major, minor)	((major << 8) | minor)
#define	STROFFSET(str, begin)		((long)((char *)str - (char *)begin))

struct sconvstruct {
	count_t	 structsize;
	void	*structptr;
	void	(*structconv)(void *, void *, count_t, count_t);
};

struct tconvstruct {
	count_t	 structsize;
	long	structoffset;
	void	(*structconv)(void *, void *, count_t, count_t);
};

struct sstat_20		sstat_20;
struct sstat_21		sstat_21;
struct sstat_22		sstat_22;
struct sstat_23		sstat_23;
struct sstat_24		sstat_24;
struct sstat		sstat;

struct tstat_20		tstat_20;
struct tstat_21		tstat_21;
struct tstat_22		tstat_22;
struct tstat_23		tstat_23;
struct tstat_24		tstat_24;
struct tstat		tstat;

struct convertall {
	int			version;	// version of raw log

	unsigned int		sstatlen;	// length of struct sstat
	void			*sstat;		// pointer to sstat struct

	unsigned int		tstatlen;	// length of struct tstat
	void			*tstat;		// pointer to tstat structs

	// conversion definition for subparts within sstat
	struct sconvstruct 	scpu;
	struct sconvstruct 	smem;
	struct sconvstruct 	snet;
	struct sconvstruct 	sintf;
	struct sconvstruct 	sdsk;	
	struct sconvstruct 	snfs;
	struct sconvstruct 	scfs;
	struct sconvstruct 	swww;
	struct sconvstruct 	spsi;
	struct sconvstruct 	sgpu;
	struct sconvstruct 	sifb;

	// conversion definition for subparts within tstat
	struct tconvstruct 	tgen;
	struct tconvstruct 	tcpu;
	struct tconvstruct 	tdsk;
	struct tconvstruct 	tmem;
	struct tconvstruct 	tnet;
	struct tconvstruct 	tgpu;
} convs[] = 
{
	{SETVERSION(2,0),
		 sizeof(struct sstat_20), 	&sstat_20,
		 sizeof(struct tstat_20), 	NULL,

		{sizeof(struct cpustat_20),  	&sstat_20.cpu,	NULL},
		{sizeof(struct memstat_20),  	&sstat_20.mem,	NULL},
		{sizeof(struct netstat_20),  	&sstat_20.net,	NULL},
		{sizeof(struct intfstat_20), 	&sstat_20.intf,	NULL},
		{sizeof(struct dskstat_20),  	&sstat_20.dsk,	NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{sizeof(struct wwwstat_20),  	&sstat_20.www,	NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_20),  
			STROFFSET(&tstat_20.gen, &tstat_20),	NULL},
		{sizeof(struct cpu_20),
			STROFFSET(&tstat_20.cpu, &tstat_20),	NULL},
		{sizeof(struct dsk_20), 
			STROFFSET(&tstat_20.dsk, &tstat_20),	NULL},
		{sizeof(struct mem_20),
			STROFFSET(&tstat_20.mem, &tstat_20),	NULL},
		{sizeof(struct net_20),
			STROFFSET(&tstat_20.net, &tstat_20),	NULL},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,1), // 2.0 --> 2.1
		 sizeof(struct sstat_21), 	&sstat_21,
		 sizeof(struct tstat_21), 	NULL,

		{sizeof(struct cpustat_21),  	&sstat_21.cpu,	scpu_to_21},
		{sizeof(struct memstat_21),  	&sstat_21.mem,	justcopy},
		{sizeof(struct netstat_21),  	&sstat_21.net,	justcopy},
		{sizeof(struct intfstat_21), 	&sstat_21.intf,	justcopy},
		{sizeof(struct dskstat_21),  	&sstat_21.dsk,	sdsk_to_21},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{sizeof(struct wwwstat_21),  	&sstat_21.www,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_21),
			STROFFSET(&tstat_21.gen, &tstat_21),	tgen_to_21},
		{sizeof(struct cpu_21),
			STROFFSET(&tstat_21.cpu, &tstat_21),	justcopy},
		{sizeof(struct dsk_21), 
			STROFFSET(&tstat_21.dsk, &tstat_21),	justcopy},
		{sizeof(struct mem_21),
			STROFFSET(&tstat_21.mem, &tstat_21),	tmem_to_21},
		{sizeof(struct net_21),
			STROFFSET(&tstat_21.net, &tstat_21),	justcopy},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,2), // 2.1 --> 2.2
		 sizeof(struct sstat_22), 	&sstat_22,
		 sizeof(struct tstat_22), 	NULL,

		{sizeof(struct cpustat_22),  	&sstat_22.cpu,	justcopy},
		{sizeof(struct memstat_22),  	&sstat_22.mem,	justcopy},
		{sizeof(struct netstat_22),  	&sstat_22.net,	justcopy},
		{sizeof(struct intfstat_22), 	&sstat_22.intf,	sint_to_22},
		{sizeof(struct dskstat_22),  	&sstat_22.dsk,	justcopy},
		{sizeof(struct nfsstat_22),  	&sstat_22.nfs,	NULL},
		{sizeof(struct contstat_22), 	&sstat_22.cfs,	NULL},
		{sizeof(struct wwwstat_22),  	&sstat_22.www,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_22), 
			STROFFSET(&tstat_22.gen, &tstat_22),	tgen_to_22},
		{sizeof(struct cpu_22),
			STROFFSET(&tstat_22.cpu, &tstat_22),	justcopy},
		{sizeof(struct dsk_22),
			STROFFSET(&tstat_22.dsk, &tstat_22),	justcopy},
		{sizeof(struct mem_22),
			STROFFSET(&tstat_22.mem, &tstat_22),	justcopy},
		{sizeof(struct net_22),
			STROFFSET(&tstat_22.net, &tstat_22),	justcopy},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,3), // 2.2 --> 2.3
		 sizeof(struct sstat_23), 	&sstat_23,
		 sizeof(struct tstat_23), 	NULL,

		{sizeof(struct cpustat_23),  	&sstat_23.cpu,	justcopy},
		{sizeof(struct memstat_23),  	&sstat_23.mem,	justcopy},
		{sizeof(struct netstat_23),  	&sstat_23.net,	justcopy},
		{sizeof(struct intfstat_23), 	&sstat_23.intf,	justcopy},
		{sizeof(struct dskstat_23),  	&sstat_23.dsk,	justcopy},
		{sizeof(struct nfsstat_23),  	&sstat_23.nfs,	justcopy},
		{sizeof(struct contstat_23), 	&sstat_23.cfs,	justcopy},
		{sizeof(struct wwwstat_23),  	&sstat_23.www,	justcopy},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},
		{0,  				NULL,		NULL},

		{sizeof(struct gen_23),
			STROFFSET(&tstat_23.gen, &tstat_23),	justcopy},
		{sizeof(struct cpu_23),
			STROFFSET(&tstat_23.cpu, &tstat_23),	justcopy},
		{sizeof(struct dsk_23),
			STROFFSET(&tstat_23.dsk, &tstat_23),	justcopy},
		{sizeof(struct mem_23), 
			STROFFSET(&tstat_23.mem, &tstat_23),	justcopy},
		{sizeof(struct net_23),
			STROFFSET(&tstat_23.net, &tstat_23),	justcopy},
		{0,  				0,		NULL},
	},

	{SETVERSION(2,4), // 2.3 --> 2.4
		 sizeof(struct sstat_24),	&sstat_24,
		 sizeof(struct tstat_24), 	NULL,

		{sizeof(struct cpustat_24),  	&sstat_24.cpu,	justcopy},
		{sizeof(struct memstat_24),  	&sstat_24.mem,	justcopy},
		{sizeof(struct netstat_24),  	&sstat_24.net,	justcopy},
		{sizeof(struct intfstat_24), 	&sstat_24.intf,	justcopy},
		{sizeof(struct dskstat_24),  	&sstat_24.dsk,	justcopy},
		{sizeof(struct nfsstat_24),  	&sstat_24.nfs,	justcopy},
		{sizeof(struct contstat_24), 	&sstat_24.cfs,	justcopy},
		{sizeof(struct wwwstat_24),  	&sstat_24.www,	justcopy},
		{0,  				&sstat_24.psi,	NULL},
		{0,  				&sstat_24.gpu,	NULL},
		{0,  				&sstat_24.ifb,	NULL},

		{sizeof(struct gen_24),
			STROFFSET(&tstat_24.gen, &tstat_24),	justcopy},
		{sizeof(struct cpu_24),
			STROFFSET(&tstat_24.cpu, &tstat_24),	justcopy},
		{sizeof(struct dsk_24),
			STROFFSET(&tstat_24.dsk, &tstat_24),	justcopy},
		{sizeof(struct mem_24),
			STROFFSET(&tstat_24.mem, &tstat_24),	justcopy},
		{sizeof(struct net_24),
			STROFFSET(&tstat_24.net, &tstat_24),	justcopy},
		{sizeof(struct gpu_24),
			STROFFSET(&tstat_24.gpu, &tstat_24),	justcopy},
	},
};

int	numconvs = sizeof convs / sizeof(struct convertall);

///////////////////////////////////////////////////////////////
// End of conversion functions
///////////////////////////////////////////////////////////////


/*
** structure which describes the raw file contents
**
** layout raw file:    rawheader
**
**                     rawrecord                           \
**                     compressed system-level statistics   | sample 1
**                     compressed process-level statistics /
**
**                     rawrecord                           \
**                     compressed system-level statistics   | sample 2
**                     compressed process-level statistics /
**
** etcetera .....
*/
#define	MYMAGIC		(unsigned int) 0xfeedbeef

struct rawheader {
	unsigned int	magic;

	unsigned short	aversion;	/* creator atop version with MSB */
	unsigned short	future1;	/* can be reused 		 */
	unsigned short	future2;	/* can be reused 		 */
	unsigned short	rawheadlen;	/* length of struct rawheader    */
	unsigned short	rawreclen;	/* length of struct rawrecord    */
	unsigned short	hertz;		/* clock interrupts per second   */
	unsigned short	sfuture[6];	/* future use                    */
	unsigned int	sstatlen;	/* length of struct sstat        */
	unsigned int	tstatlen;	/* length of struct tstat        */
	struct utsname	utsname;	/* info about this system        */
	char		cfuture[8];	/* future use                    */

	unsigned int	pagesize;	/* size of memory page (bytes)   */
	int		supportflags;  	/* used features                 */
	int		osrel;		/* OS release number             */
	int		osvers;		/* OS version number             */
	int		ossub;		/* OS version subnumber          */
	int		ifuture[6];	/* future use                    */
};

struct rawrecord {
	time_t		curtime;	/* current time (epoch)         */

	unsigned short	flags;		/* various flags                */
	unsigned short	sfuture[3];	/* future use                   */

	unsigned int	scomplen;	/* length of compressed sstat   */
	unsigned int	pcomplen;	/* length of compressed tstat's */
	unsigned int	interval;	/* interval (number of seconds) */
	unsigned int	ndeviat;	/* number of tasks in list      */
	unsigned int	nactproc;	/* number of processes in list  */
	unsigned int	ntask;		/* total number of tasks        */
	unsigned int    totproc;	/* total number of processes	*/
	unsigned int    totrun;		/* number of running  threads	*/
	unsigned int    totslpi;	/* number of sleeping threads(S)*/
	unsigned int    totslpu;	/* number of sleeping threads(D)*/
	unsigned int	totzomb;	/* number of zombie processes   */
	unsigned int	nexit;		/* number of exited processes   */
	unsigned int	noverflow;	/* number of overflow processes */
	unsigned int	ifuture[6];	/* future use                   */
};

// function prototypes
//
static int	openin(char *);
static void	readin(int, void *, int);

static int	openout(char *);
static void	writeout(int, void *, int);
static void	writesamp(int, struct rawrecord *, void *, int, void *,
				int, int);

static void	copy_file(int, int);
static void	convert_samples(int, int, struct rawheader *, int,  int);
static void	do_sconvert(struct sconvstruct *, struct sconvstruct *);
static void	do_tconvert(void *, void *,
			struct tconvstruct *, struct tconvstruct *);

static int	getrawsstat(int, struct sstat *, int);
static int	getrawtstat(int, struct tstat *, int, int);
static void	testcompval(int, char *);

int
main(int argc, char *argv[])
{
	int			ifd, ofd;
	struct rawheader	irh, orh;
	int			i, versionix, targetix = -1;
	int			c, major, minor, targetvers;
	char			*infile, *outfile;

	// verify the command line arguments:
	// 	optional flags and mandatory input and output filename
	//
	if (argc < 2)
		prusage(argv[0]);

	while ((c = getopt(argc, argv, "?t:")) != EOF)
	{
		switch (c)
		{
		   case '?':		// usage wanted ?
			prusage(argv[0]);
			break;

		   case 't':		// target version
			if ( sscanf(optarg, "%d.%d", &major, &minor) != 2)
			{
				fprintf(stderr,
					"target version format: major.minor\n");
				prusage(argv[0]);
			}

			targetvers = SETVERSION(major, minor);

			// search for target version in conversion table
			//
			for (i=0, targetix=-1; i < numconvs; i++)
			{
				if (targetvers == convs[i].version)
				{
					targetix = i;
					break;
				}
			}

			if (targetix == -1)	// incorrect target version?
			{
				fprintf(stderr,
					"target version incorrect!");
				prusage(argv[0]);
			}
			break;

		   default:
			prusage(argv[0]);
			break;
		}
	}

	if (optind >= argc)
		prusage(argv[0]);

	infile  = argv[optind++];

	// determine target version (default: latest version)
	//
	if (targetix == -1)	// no specific target version requested?
		targetix = numconvs - 1;

	// open the input file and verify magic number
	//
	if ( (ifd = openin(infile)) == -1)
	{
		prusage(argv[0]);
		exit(2);
	}

	readin(ifd, &irh, sizeof irh);

	if (irh.magic != MYMAGIC)
	{
		fprintf(stderr,
			"File %s does not contain atop/atopsar data "
			"(wrong magic number)\n", infile);
		exit(3);
	}

	printf("Version of %s: %d.%d\n", infile,
			(irh.aversion >> 8) & 0x7f, irh.aversion & 0xff);

	if (irh.rawheadlen != sizeof(struct rawheader) ||
	    irh.rawreclen  != sizeof(struct rawrecord)   )
	{
		fprintf(stderr,
			"File %s created with atop compiled "
			"for other CPU architecture\n", infile);
		exit(3);
	}

	// search for version of input file in conversion table
	//
	for (i=0, versionix=-1; i < numconvs; i++)
	{
		if (convs[i].version == (irh.aversion & 0x7fff))
		{
			versionix = i;
			break;
		}
	}

	if (versionix == -1)
	{
		fprintf(stderr,
			"This version is not supported for conversion!\n");
		exit(11);
	}

	if (versionix > targetix)
	{
		fprintf(stderr, "Downgrading of version is not supported!\n");
		exit(11);
	}

	if (irh.sstatlen != convs[versionix].sstatlen ||
	    irh.tstatlen != convs[versionix].tstatlen   )
	{
 		fprintf(stderr,
 			"File %s contains unexpected internal structures\n",
 			infile);
 		fprintf(stderr, "sstat: %d/%d, tstat: %d/%d\n",
 			irh.sstatlen, convs[versionix].sstatlen,
 	    		irh.tstatlen, convs[versionix].tstatlen);
 				
 		exit(11);
	}

	// handle the output file 
	//
	if (optind >= argc)
		exit(0);

	outfile = argv[optind++];

	if (strcmp(infile, outfile) == 0)
	{
		fprintf(stderr,
	 	    "input file and output file should not be identical!\n");
		exit(12);
	}

	// open the output file 
	//
	if ( (ofd = openout(outfile)) == -1)
	{
		prusage(argv[0]);
		exit(4);
	}

	// write raw header to output file
	//
	orh = irh;

	orh.aversion	= convs[targetix].version | 0x8000;
	orh.sstatlen	= convs[targetix].sstatlen;
	orh.tstatlen	= convs[targetix].tstatlen;

	writeout(ofd, &orh, sizeof orh);

	printf("Version of %s: %d.%d\n", outfile,
			(orh.aversion >> 8) & 0x7f, orh.aversion & 0xff);

	// copy and convert every sample, unless the version of the
	// input file is identical to the target version (then just copy)
	//
	if (versionix < targetix)
		convert_samples(ifd, ofd, &irh, versionix, targetix);
	else
		copy_file(ifd, ofd);

	close(ifd);
	close(ofd);

	return 0;
}

//
// Function that reads the input file sample-by-sample,
// converts it to an output sample and writes that to the output file
//
static void
convert_samples(int ifd, int ofd, struct rawheader *irh, int ivix, int ovix)
{
	struct rawrecord	irr, orr;
	int			i, t;
	count_t			count = 0;

	while ( read(ifd, &irr, irh->rawreclen) == irh->rawreclen)
	{
		count++;

		// read compressed system-level statistics and decompress
		//
		if ( !getrawsstat(ifd, convs[ivix].sstat, irr.scomplen) )
			exit(7);

		// read compressed process-level statistics and decompress
		//
		convs[ivix].tstat = malloc(convs[ivix].tstatlen * irr.ndeviat);

		ptrverify(convs[ivix].tstat,
			"Malloc failed for %d stored tasks\n", irr.ndeviat);

		if ( !getrawtstat(ifd, convs[ivix].tstat,
					irr.pcomplen, irr.ndeviat) )
			exit(7);

		// STEP-BY-STEP CONVERSION FROM OLD VERSION TO NEW VERSION
		// 
		for (i=ivix; i < ovix; i++)
		{
			// convert system-level statistics to newer version
			//
			memset(convs[i+1].sstat, 0, convs[i+1].sstatlen);

			do_sconvert(&(convs[i].scpu),  &(convs[i+1].scpu));
			do_sconvert(&(convs[i].smem),  &(convs[i+1].smem));
			do_sconvert(&(convs[i].snet),  &(convs[i+1].snet));
			do_sconvert(&(convs[i].sintf), &(convs[i+1].sintf));
			do_sconvert(&(convs[i].sdsk),  &(convs[i+1].sdsk));
			do_sconvert(&(convs[i].snfs),  &(convs[i+1].snfs));
			do_sconvert(&(convs[i].scfs),  &(convs[i+1].scfs));
			do_sconvert(&(convs[i].spsi),  &(convs[i+1].spsi));
			do_sconvert(&(convs[i].sgpu),  &(convs[i+1].sgpu));
			do_sconvert(&(convs[i].sifb),  &(convs[i+1].sifb));
			do_sconvert(&(convs[i].swww),  &(convs[i+1].swww));

			// convert process-level statistics to newer version
			//
			convs[i+1].tstat = malloc(convs[i+1].tstatlen *
								irr.ndeviat);
			ptrverify(convs[ivix].tstat,
			   "Malloc failed for %d stored tasks\n", irr.ndeviat);

			memset(convs[i+1].tstat, 0, convs[i+1].tstatlen *
								irr.ndeviat);

			for (t=0; t < irr.ndeviat; t++)	// for every task
			{
				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen), 
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tgen), &(convs[i+1].tgen));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen), 
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tcpu), &(convs[i+1].tcpu));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen), 
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tdsk), &(convs[i+1].tdsk));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen), 
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tmem), &(convs[i+1].tmem));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen), 
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tnet), &(convs[i+1].tnet));

				do_tconvert(
				    convs[i].tstat  +(t*convs[i].tstatlen), 
				    convs[i+1].tstat+(t*convs[i+1].tstatlen),
				    &(convs[i].tgpu), &(convs[i+1].tgpu));
			}

			free(convs[i].tstat);
		}

		// write new sample to output file
		//
		orr = irr;

		writesamp(ofd, &orr, convs[ovix].sstat, convs[ovix].sstatlen,
		                     convs[ovix].tstat, convs[ovix].tstatlen,
		                     irr.ndeviat);

		// cleanup
		// 
		free(convs[ovix].tstat);
	}

	printf("Samples converted: %llu\n", count);
}

//
// Function that calls the appropriate function to convert a struct
// from the sstat structure
//
static void
do_sconvert(struct sconvstruct *cur, struct sconvstruct *next)
{
	if (next->structconv)
	{
		(*(next->structconv))(cur->structptr,  next->structptr,
		                      cur->structsize, next->structsize);
	}
}

//
// Function that calls the appropriate function to convert a struct
// from one of the tstat structures
//
static void
do_tconvert(void *curtstat,          void *nexttstat,
	    struct tconvstruct *cur, struct tconvstruct *next)
{
	if (next->structconv)
	{
		(*(next->structconv))(curtstat + cur->structoffset,
		                      nexttstat + next->structoffset,
		                      cur->structsize, next->structsize);
	}
}


//
// Function to copy input file transparently to output file
//
static void
copy_file(int ifd, int ofd)
{
	unsigned char	buf[64*1024];
	int		nr, nw;

	(void) lseek(ifd, 0, SEEK_SET);
	(void) lseek(ofd, 0, SEEK_SET);

	while ( (nr = read(ifd, buf, sizeof buf)) > 0)
	{
		if ( (nw = write(ofd, buf, nr)) < nr)
		{
			if (nw == -1)
			{
				perror("write output file");
				exit(42);
			}
			else
			{
 				fprintf(stderr,
 					"Output file saturated\n");
				exit(42);
			}
		}
	}

	if (nr == -1)
	{
		perror("read input file");
		exit(42);
	}
	else
	{
		printf("Raw file copied (version already up-to-date)\n");
	}
}

//
// Function that opens an existing raw file and
// verifies the magic number
//
static int
openin(char *infile)
{
	int	rawfd;

	/*
	** open raw file for reading
	*/
	if ( (rawfd = open(infile, O_RDONLY)) == -1)
	{
		fprintf(stderr, "%s - ", infile);
		perror("open for reading");
		return -1;
	}

	return rawfd;
}

//
// Function that reads a chunk of bytes from
// the input raw file
//
static void
readin(int rawfd, void *buf, int size)
{
	/*
	** read the requested chunk and verify
	*/
	if ( read(rawfd, buf, size) < size)
	{
		fprintf(stderr, "can not read raw file\n");
		close(rawfd);
		exit(9);
	}
}


//
// Function that creates a raw log for output
//
static int
openout(char *outfile)
{
	int	rawfd;

	/*
	** create new output file
	*/
	if ( (rawfd = creat(outfile, 0666)) == -1)
	{
		fprintf(stderr, "%s - ", outfile);
		perror("create raw output file");
		return -1;
	}

	return rawfd;
}

//
// Function that reads a chunk of bytes from
// the input raw file
//
static void
writeout(int rawfd, void *buf, int size)
{
	/*
	** write the provided chunk and verify
	*/
	if ( write(rawfd, buf, size) < size)
	{
		fprintf(stderr, "can not write raw file\n");
		close(rawfd);
		exit(10);
	}
}

//
// Function that shows the usage message
//
void
prusage(char *name)
{
	fprintf(stderr,
		"Usage: %s [-t version] rawinput [rawoutput]\n", name);
	fprintf(stderr,
		"\t-t version      target version (default: %d.%d) for output\n",
			(convs[numconvs-1].version >> 8) & 0x7f,
			 convs[numconvs-1].version  & 0x7f);

	exit(1);
}


//
// Function to read the system-level statistics from the current offset
//
static int
getrawsstat(int rawfd, struct sstat *sp, int complen)
{
	Byte		*compbuf;
	unsigned long	uncomplen = sizeof(struct sstat);
	int		rv;

	compbuf = malloc(complen);

	ptrverify(compbuf, "Malloc failed for reading compressed sysstats\n");

	if ( read(rawfd, compbuf, complen) < complen)
	{
		free(compbuf);
		fprintf(stderr,
			"Failed to read %d bytes for system\n", complen);
		return 0;
	}

	rv = uncompress((Byte *)sp, &uncomplen, compbuf, complen);

	testcompval(rv, "uncompress");

	free(compbuf);

	return 1;
}

//
// Function to read the process-level statistics from the current offset
//
static int
getrawtstat(int rawfd, struct tstat *pp, int complen, int ndeviat)
{
	Byte		*compbuf;
	unsigned long	uncomplen = sizeof(struct tstat) * ndeviat;
	int		rv;

	compbuf = malloc(complen);

	ptrverify(compbuf, "Malloc failed for reading compressed procstats\n");

	if ( read(rawfd, compbuf, complen) < complen)
	{
		free(compbuf);
		fprintf(stderr,
			"Failed to read %d bytes for tasks\n", complen);
		return 0;
	}

	rv = uncompress((Byte *)pp, &uncomplen, compbuf, complen);

	testcompval(rv, "uncompress");

	free(compbuf);

	return 1;
}

//
// Function that writes a new sample to the output file
//
static void
writesamp(int ofd, struct rawrecord *rr,
	 void *sstat, int sstatlen, void *tstat, int tstatlen, int ntask)
{
	int			rv;
	Byte			scompbuf[sstatlen], *pcompbuf;
	unsigned long		scomplen = sizeof scompbuf;
	unsigned long		pcomplen = tstatlen * ntask;

	/*
	** compress system- and process-level statistics
	*/
	rv = compress(scompbuf, &scomplen,
				(Byte *)sstat, (unsigned long)sstatlen);

	testcompval(rv, "compress");

	pcompbuf = malloc(pcomplen);

	ptrverify(pcompbuf, "Malloc failed for compression buffer\n");

	rv = compress(pcompbuf, &pcomplen, (Byte *)tstat,
						(unsigned long)pcomplen);

	testcompval(rv, "compress");

	rr->scomplen	= scomplen;
	rr->pcomplen	= pcomplen;

	if ( write(ofd, rr, sizeof *rr) == -1)
	{
		perror("write raw record");
		exit(7);
	}

	/*
	** write compressed system status structure to file
	*/
	if ( write(ofd, scompbuf, scomplen) == -1)
	{
		perror("write raw status record");
		exit(7);
	}

	/*
	** write compressed list of process status structures to file
	*/
	if ( write(ofd, pcompbuf, pcomplen) == -1)
	{
		perror("write raw process record");
		exit(7);
	}

	free(pcompbuf);
}


//
// check success of (de)compression
//
static void
testcompval(int rv, char *func)
{
	switch (rv)
	{
	   case Z_OK:
	   case Z_STREAM_END:
	   case Z_NEED_DICT:
		break;

	   case Z_MEM_ERROR:
		fprintf(stderr, "%s: failed due to lack of memory\n", func);
		exit(7);

	   case Z_BUF_ERROR:
		fprintf(stderr, "%s: failed due to lack of room in buffer\n",
								func);
		exit(7);

   	   case Z_DATA_ERROR:
		fprintf(stderr, 
		        "%s: failed due to corrupted/incomplete data\n", func);
		exit(7);

	   default:
		fprintf(stderr,
		        "%s: unexpected error %d\n", func, rv);
		exit(7);
	}
}

//
// generic pointer verification after malloc
//
void
ptrverify(const void *ptr, const char *errormsg, ...)
{
        if (!ptr)
        {
                va_list args;
                va_start(args, errormsg);
                vfprintf(stderr, errormsg, args);
                va_end  (args);

                exit(13);
        }
}
