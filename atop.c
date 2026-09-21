/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of 
** the system on system-level as well as process-level.
**
** This source-file contains the main-function, which verifies the
** calling-parameters and takes care of initialization. 
** The engine-function drives the main sample-loop in which after the
** indicated interval-time a snapshot is taken of the system-level and
** process-level counters and the deviations are calculated and
** visualized for the user.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** Linux-port:  June 2000
** Modified: 	May 2001 - Ported to kernel 2.4
** --------------------------------------------------------------------------
** Copyright (C) 2000-2024 Gerlof Langeveld
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
** After initialization, the main-function calls the ENGINE.
** For every cycle (so after another interval) the ENGINE calls various 
** functions as shown below:
**
** +------------------------------------------------------------------------+
** |                           E  N  G  I  N  E                             |
** |                                                                        |
** |                                                                        |
** |    _____________________await interval-timer________________________   |
** |   |                                                                 ^  |
** |   |    _______    _______    _______       ________      ________   |  |
** |   |   ^       |  ^       |  ^       |     ^        |    ^        |  |  |
** +---|---|-------|--|-------|--|-------|-----|--------|----|--------|--|--+
**     |   |       |  |       |  |       |     |        |    |        |  |
**  +--V-----+  +--V----+  +--V----+  +--V--------+  +--V-------+  +--V-----+  
**  |        |  |       |  |       |  |           |  | deviate  |  |        |
**  | photo  |  | photo |  | photo |  |   acct    |  | ..cgroup |  | print  |
**  | cgroup |  | syst  |  | proc  |  | photoproc |  | ..syst   |  |        |
**  |        |  |       |  |       |  |           |  | ..proc   |  |        |
**  +--------+  +-------+  +-------+  +-----------+  +----------+  +--------+
**      ^           ^          ^            ^              |            |
**      |           |          |            |              |            |
**      |           |          |            V              V            V
**    _______     _____      _____      __________     ________     _________
**   /       \   /     \    /     \    /          \   /        \   /         \
**    /sys/fs     /proc      /proc      accounting       task       screen or
**    /cgroup                              file        database        file
**   \_______/   \_____/    \_____/    \__________/   \________/   \_________/
**
**    -	photocgroup()
**	Takes a snapshot of the counters related to resource usage on
** 	cgroup-level v2 (cpu, disk, memory).
**
**    -	photosyst()
**	Takes a snapshot of the counters related to resource-usage on
** 	system-level (cpu, disk, memory, network).
**
**    -	photoproc()
**	Takes a snapshot of the counters related to resource-usage of
**	tasks which are currently active. For this purpose the whole
**	task-list is read.
**
**    -	acctphotoproc()
**	Takes a snapshot of the counters related to resource-usage of
**	tasks which have been finished during the last interval.
**	For this purpose all new records in the accounting-file are read.
**
** When all counters have been gathered, functions are called to calculate
** the difference between the current counter values and the counter values
** of the previous cycle. These functions operate on cgroup level, system level
** as well as on task level.
** These differences are stored in a new structure (table). 
**
**    -	deviatcgroup()
**	Calculates the differences between the current cgroup-level
** 	counters and the corresponding counters of the previous cycle.
**
**    -	deviatsyst()
**	Calculates the differences between the current system-level
** 	counters and the corresponding counters of the previous cycle.
**
**    -	deviattask()
**	Calculates the differences between the current task-level
** 	counters and the corresponding counters of the previous cycle.
**	The per-task counters of the previous cycle are stored in the
**	task-database; this "database" is implemented as a linked list
**	of taskinfo structures in memory (so no disk-accesses needed).
**	Within this linked list hash-buckets are maintained for fast searches.
**	The entire task-database is handled via a set of well-defined 
** 	functions from which the name starts with "pdb_..." (see the
**	source-file procdbase.c).
**	The processes which have been finished during the last cycle
** 	are also treated by deviattask() in order to calculate what their
**	resource-usage was before they finished.
**
** All information is ready to be visualized now.
** There is a structure which holds the start-address of the
** visualization-function to be called. Initially this structure contains
** the address of the generic visualization-function ("generic_samp"), but
** these addresses can be modified in the main-function depending on particular
** flags. In this way various representation-layers (ASCII, graphical, ...)
** can be linked with 'atop'; the one to use can eventually be chosen
** at runtime. 
*/
#define _POSIX_C_SOURCE	200809L
#define _XOPEN_SOURCE
#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/utsname.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <regex.h>
#include <glib.h>
#include <sys/inotify.h>

#include "atop.h"
#include "acctproc.h"
#include "ifprop.h"
#include "photoproc.h"
#include "photosyst.h"
#include "cgroups.h"
#include "showgeneric.h"
#include "showlinux.h"
#include "parseable.h"
#include "json.h"
#include "gpucom.h"
#include "netatop.h"

#define	MAXFL		128      // maximum positions for command line flags
#define	MAXPARAM	80 

/*
** declaration of global variables
*/
struct utsname	utsname;
int		utsnodenamelen;
time_t 		pretime;	/* timing info				*/
time_t 		curtime;	/* timing info				*/
unsigned long	interval = 10;
unsigned long 	sampcnt;
char		screen;
int		fdinotify = -1;	/* inotify fd for twin mode  		*/
pid_t		twinpid;	/* PID of lower half for twin mode	*/
char		twindir[RAWNAMESZ] = "/tmp";
int		linelen  = 80;
char		acctreason;	/* accounting not active (return val) 	*/
char		irawname[RAWNAMESZ];
char		orawname[RAWNAMESZ];
char		rawreadflag;
char		idnamesuppress;	/* suppress UID/GID to name translation */
char		highpriosuppress; /* suppress high priority for atop    */
char		idnamemaximum;	/* UID/GID to maximum  name translation */
time_t		begintime, endtime, cursortime;	// epoch or time in day

char		flagrest[MAXFL]; /* flags remaining to be processed      */

char		deviatonly = 1;
char      	usecolors  = 1;  /* boolean: colors for high occupation  */
char		threadview = 0;	 /* boolean: show individual threads     */
char      	calcpss    = 0;  /* boolean: read/calculate process PSS  */
char      	getwchan   = 0;  /* boolean: obtain wchan string         */
char      	rmspaces   = 0;  /* boolean: remove spaces from command  */
		                 /* name in case of parsable output      */

char            displaymode = 'T';      /* 'T' = text, 'D' = draw        */
char            barmono     = 0; /* boolean: bar without categories?     */

char		prependenv  = 0; /* boolean: prepend selected            */
				 /* environment variables to cmdline     */

char		connectgpud    = 0; /* boolean: connect to atopgpud      */
char		connectnetatop = 0; /* boolean: connect to netatop(bpf)  */

regex_t		envregex;

unsigned short	hertz;
unsigned int	pidwidth;
unsigned int	pagesize;
unsigned int	nrgpus;
int 		osrel;
int 		osvers;
int 		ossub;

extern GHashTable *ghash_net;

int		supportflags;	/* supported features             	*/

struct handler	handlers[MAXHANDLERS];
int		numhandlers;

/*
** argument values
*/
static char		awaittrigger;	/* boolean: awaiting trigger */
static unsigned int 	nsamples = 0xffffffff;
static char		midnightflag;
static char		rawwriteflag;
static char		parseoutflag;
static char		jsonoutflag;
static char		screenoutflag;

char			twinmodeflag;

/*
** interpretation of defaults-file /etc/atoprc and $HOME/.atop
*/
static void		readrc(char *, int);

static void do_interval(char *, char *);
static void do_linelength(char *, char *);

static struct {
	char	*tag;
	void	(*func)(char *, char *);
	int	sysonly;
} manrc[] = {
	{	"flags",		do_flags,		0, },
	{	"twindir",		do_twindir,		0, },
	{	"interval",		do_interval,		0, },
	{	"linelen",		do_linelength,		0, },
	{	"username",		do_username,		0, },
	{	"procname",		do_procname,		0, },
	{	"maxlinecpu",		do_maxcpu,		0, },
	{	"maxlinegpu",		do_maxgpu,		0, },
	{	"maxlinedisk",		do_maxdisk,		0, },
	{	"maxlinemdd",		do_maxmdd,		0, },
	{	"maxlinelvm",		do_maxlvm,		0, },
	{	"maxlineintf",		do_maxintf,		0, },
	{	"maxlineifb",		do_maxifb,		0, },
	{	"maxlinenfsm",		do_maxnfsm,		0, },
	{	"maxlinecont",		do_maxcont,		0, },
	{	"maxlinenuma",		do_maxnuma,		0, },
	{	"maxlinellc",		do_maxllc,		0, },
	{	"colorinfo",		do_colinfo,		0, },
	{	"coloralmost",		do_colalmost,		0, },
	{	"colorcritical",	do_colcrit,		0, },
	{	"colorthread",		do_colthread,		0, },
	{	"ownallcpuline",	do_ownallcpuline,	0, },
	{	"ownonecpuline",	do_ownindivcpuline,	0, },
	{	"owncplline",		do_owncplline,		0, },
	{	"ownmemline",		do_ownmemline,		0, },
	{	"ownswpline",		do_ownswpline,		0, },
	{	"ownpagline",		do_ownpagline,		0, },
	{	"ownmemnumaline",	do_ownmemnumaline,	0, },
	{	"ownnumacpuline",	do_owncpunumaline,	0, },
	{	"ownllcline",		do_ownllcline,		0, },
	{	"owndskline",		do_owndskline,		0, },
	{	"ownnettrline",		do_ownnettransportline,	0, },
	{	"ownnetnetline",	do_ownnetnetline,	0, },
	{	"ownnetifline",	        do_ownnetinterfaceline,	0, },
	{	"ownifbline",	        do_owninfinibandline,	0, },
	{	"ownprocline",		do_ownprocline,		0, },
	{	"ownsysprcline",	do_ownsysprcline,	0, },
	{	"owndskline",	        do_owndskline,		0, },
	{	"cpucritperc",		do_cpucritperc,		0, },
	{	"gpucritperc",		do_gpucritperc,		0, },
	{	"memcritperc",		do_memcritperc,		0, },
	{	"swpcritperc",		do_swpcritperc,		0, },
	{	"dskcritperc",		do_dskcritperc,		0, },
	{	"netcritperc",		do_netcritperc,		0, },
	{	"swoutcritsec",		do_swoutcritsec,	0, },
	{	"almostcrit",		do_almostcrit,		0, },
	{	"atopsarflags",		do_atopsarflags,	0, },
	{	"perfevents",		do_perfevents,		0, },
	{	"pacctdir",		do_pacctdir,		1, },
};

/*
** internal prototypes
*/
static void	engine(void);
static void	twinprepare(void);
static void	twinclean(void);

/*
** table defining all flags and corresponding long options
**
** used flags: 123456789aBb:CcDdEe:FfGgHhIiJ:jKkL:lMmNnoP:pQRr::Sst::uVvWw:XxYyZz:
*/
#define	PHSHOWGPU (PHBASEVAL+1)

static struct pardef paramdef[] = {
	{ { "twin",       optional_argument, 0,                  't' },
		"twin mode: live measurement with possibility to review\n"
		"earlier samples (temporary raw file created in /tmp or\n"
		"in specified absolute directory path D)\n", 'D' },


	{ { "showbar",    no_argument,       0,                  'B' },
		"show bar graphs for system metrics", ' ' },

	{ { "blankbars",  no_argument,       0, MBARMONO   }, // 'H'
		"show bar graphs without categories\n", ' ' },


	{ { "showcgr",    no_argument,       0, MCGROUPS   }, // 'G'
		"show cgroups v2 metrics (control groups and\n"
		"related processes)", ' ' },

	{ { "cgrlevel2",  no_argument,       0,                  '2' }, NULL, ' ' },
	{ { "cgrlevel3",  no_argument,       0,                  '3' }, NULL, ' ' },
	{ { "cgrlevel4",  no_argument,       0,                  '4' }, NULL, ' ' },
	{ { "cgrlevel5",  no_argument,       0,                  '5' }, NULL, ' ' },
	{ { "cgrlevel6",  no_argument,       0,                  '6' }, NULL, ' ' },
	{ { "cgrlevel7",  no_argument,       0,                  '7' },
		"cgroups v2: define depth level -2 till -7 (default: -7)", ' ' },

	{ { "cgrprocusr", no_argument,       0,                  '8' },
		"cgroups v2: show processes per cgroups\n"
		"except kernel processes in root cgroup", ' ' },

	{ { "cgrprocall", no_argument,       0,                  '9' },
		"cgroups v2: show user and kernel processes per cgroup\n", ' ' },


	{ { "all",        no_argument,       0, MALLACTIVE }, // 'a'
		"show all processes, threads and cgroups instead of\n"
		"the active ones only (default)\n", ' ' },


	{ { "read",       optional_argument, 0,                  'r' },
		"read  raw data from atop logfile F\n"
		"symbolic file: y[y...] for yesterday (repeated)\n"
		"file name '-': read raw data from stdin", 'F' },

	{ { "write",      required_argument, 0,                  'w' },
		"write raw data to atop logfile F\n", 'F' },


	{ { "showgen",    no_argument,       0, MPROCGEN   }, // 'g'
		"show generic process information (default)", ' ' },

	{ { "showmem",    no_argument,       0, MPROCMEM   }, // 'm'
		"show memory-related process information", ' ' },

	{ { "showdsk",    no_argument,       0, MPROCDSK   }, // 'd'
		"show disk-related process information", ' ' },

	{ { "shownet",    no_argument,       0, MPROCNET   }, // 'n'
		"show network-related process information", ' ' },

	{ { "showgpu",    no_argument,       0, PHSHOWGPU  }, // 'e' place holder
		"show GPU-related process information", ' ' },

	{ { "showcmd",    no_argument,       0, MPROCARG   }, // 'c'
		"show command line per process", ' ' },

	{ { "showsched",  no_argument,       0, MPROCSCH   }, // 's'
		"show scheduling-related process information", ' ' },

	{ { "showvar",    no_argument,       0, MPROCVAR   }, // 'v'
		"show miscellaneous process information\n"
		"(ppid, user/group, start and end date/time)", ' ' },

	{ { "showown",    no_argument,       0, MPROCOWN   }, // 'o'
		"show self-defined process information\n", ' ' },


	{ { "threads",    no_argument,       0, MTHREAD    }, // 'y'
		"show threads within process", ' ' },

	{ { "sortthr",    no_argument,       0, MTHRSORT   }, // 'Y'
		"sort threads (when combined with -y)\n", ' ' },


	{ { "sortcpu",    no_argument,       0, MPERCCPU   }, // 'C'
		"sort processes by cpu consumption (default)", ' ' },

	{ { "sortmem",    no_argument,       0, MPERCMEM   }, // 'M'
		"sort processes by memory consumption", ' ' },

	{ { "sortdsk",    no_argument,       0, MPERCDSK   }, // 'D'
		"sort processes by disk activity", ' ' },

	{ { "sortnet",    no_argument,       0, MPERCNET   }, // 'N'
		"sort processes by network activity", ' ' },

	{ { "sortgpu",    no_argument,       0, MPERCGPU   }, // 'E'
		"sort processes by of GPU activity\n", ' ' },


	{ { "cumprocs",   no_argument,       0, MCUMPROC   }, // 'p'
		"show cumulated process information per program\n"
		"(i.e. same name)", ' ' },

	{ { "cumusers",   no_argument,       0, MCUMUSER   }, // 'u'
		"show cumulated process information per user", ' ' },

	{ { "cumconts",   no_argument,       0, MCUMCONT   }, // 'j'
		"show cumulated process info per container/pod\n", ' ' },


	{ { "begintime",  required_argument, 0,                  'b' },
		"begin from specified date/time [YYYYMMDD]hhmm[ss]", 'T' },

	{ { "endtime",    required_argument, 0,                  'e' },
		"finish after specified date/time [YYYYMMDD]hhmm[ss]\n", 'T' },


	{ { "parsable",   required_argument, 0,                  'P' },
		"generate parsable output for specified label(s)", 'L' },

	{ { "rmspaces",   no_argument,       0, MRMSPACES  }, // 'Z'
		"no spaces in parsable output for command (line)", ' ' },

	{ { "json",       required_argument, 0,                  'J' },
		"generate JSON output for specified label(s)\n", 'L' },


	{ { "netatop",    no_argument,       0,                  'K' },
		"connect to netatop/netatop-bpf interface\n"
		"(default: do not connect)", ' ' },

	{ { "gpud",       no_argument,       0,                  'k' },
		"connect to external atopgpud daemon\n"
		"(default: do not connect)\n", ' ' },


	{ { "pss",        no_argument,       0, MCALCPSS   }, // 'R'
		"calculate proportional set size (PSS) per process", ' ' },

	{ { "wchan",      no_argument,       0, MGETWCHAN  }, // 'W'
		"determine WCHAN (string) per thread", ' ' },

	{ { "showenv",    required_argument, 0,                  'z' },
		"specify regex E that matches environment variables\n"
		"to be prepended to the command line\n"
		"WARNING: don't use this flag when writing to\n"
		"(publicly readable) raw files!\n", 'E' },


	{ { "sysnosort",  no_argument,       0, MSYSNOSORT }, // 'F'
		"suppress sorting of system resources", ' ' },

	{ { "sysfixed",   no_argument,       0, MSYSFIXED  }, // 'f'
		"show fixed number of lines with system metrics", ' ' },

	{ { "noidname",   no_argument,       0,                  'I' },
		"suppress UID/GID to name translation\n"
		"(show numbers instead)", ' ' },

	{ { "maxidname",  no_argument,       0,                  'i' },
		"UID/GID translation to full name\n"
		"(default column width is 8)", ' ' },

	{ { "syslimit",   no_argument,       0, MSYSLIMIT  }, // 'l'
		"show limited number of lines for certain system resources", ' ' },

	{ { "avgpersec",  no_argument,       0, MAVGVAL    }, // '1'
		"show average-per-second instead of total values", ' ' },

	{ { "nocolors",   no_argument,       0, MCOLORS    }, // 'x'
		"no colors in case of high occupation", ' ' },

	{ { "noexits",    no_argument,       0, MSUPEXITS  }, // 'X'
		"suppress terminated processes in output", ' ' },

	{ { "nohighpri",  no_argument,       0,                  'Q' },
		"suppress higher CPU priority and memory locking for atop", ' ' },

	{ { "linelen",    required_argument, 0,                  'L' },
		"alternate line length (default 80) in case of\n"
		"non-fullscreen output", 'L' },

	{ { "version",    no_argument,       0, MVERSION   }, // 'V'
		"show version information", ' ' },

	{ { "midnight",   no_argument,       0,                  'S' },
		"finish atop automatically before midnight\n"
		"instead of #samples", ' ' },


	{ { "help",       no_argument,       0,                  'h' }, NULL },
};

static struct option long_opts[MAXPARAM];


int
main(int argc, char *argv[])
{
	register int	i;
	int		c;
	char		*p;
	struct rlimit	rlim;
	char		flaglist[MAXFL] = {'\0'}; // possible command flags

	/*
	** since privileged actions will be done later on, at this stage
	** the root-privileges are dropped by switching effective user-id
	** to real user-id (security reasons)
	*/
        if (! droprootprivs() )
	{
		fprintf(stderr, "not possible to drop root privs\n");
                exit(42);
	}

	/*
	** read defaults-files /etc/atoprc en $HOME/.atoprc (if any)
	*/
	readrc("/etc/atoprc", 1);

	if ( (p = getenv("HOME")) )
	{
		char path[1024];

		snprintf(path, sizeof path, "%s/.atoprc", p);

		readrc(path, 0);
	}

	/*
	** check if we are supposed to behave as 'atopsar'
	** i.e. system statistics only
	*/
	if ( (p = strrchr(argv[0], '/')))
		p++;
	else
		p = argv[0];

	if ( strcmp(p, "atopsar") == 0)
		return atopsar(argc, argv);

	/* 
	** interpret command-line arguments & flags 
	**
	** dynamically prepare calling arguments for getopt_long()
	*/
	prepcmdopts(paramdef, sizeof paramdef/sizeof(struct pardef),
			long_opts, MAXPARAM, flaglist, MAXFL);

	/*
	** generic flags will be handled here while
	** screen-related flags are passed to the print routines
	*/
	i = 0;

	while (i < MAXFL-1 && (c = getopt_long(argc, argv, flaglist, long_opts, NULL)) != -1)
	{
		switch (c)
		{
		   case '?':		/* usage wanted ?             */
		   case 'h':		/* usage wanted ?             */
			prusage(argv[0]);
			break;

		   case 'V':		/* version wanted ?           */
			printf("%s\n", getstrvers());
			exit(0);

		   case 'w':		/* writing of raw data ?      */
			safe_strcpy(orawname, optarg, sizeof orawname);

			if (!rawwriteflag)
			{	
				rawwriteflag++;
				handlers[numhandlers++].handle_sample = rawwrite;
			}

			break;

		   case 'r':		/* reading of raw data ?      */
			if (optarg == NULL)	// no additional argument without space in between?
			{
				// check additional argument with space in between?
				//
				if (optind < argc)
				{
					if (*argv[optind] ==  '-')
					{
						// just a '-' used on its own meaning stdin?
						if (*(argv[optind]+1) == '\0')
							optarg = argv[optind++];
					}
					else
					{
						optarg = argv[optind++];
					}
				}
			}

			if (optarg)
			{
				if (*optarg == '-')
					safe_strcpy(irawname, "/dev/stdin", sizeof irawname);
				else
					safe_strcpy(irawname, optarg, sizeof irawname);
			}

			rawreadflag++;
			break;

		   case 't':		/* twin mode ?		      */
			if (optarg == NULL)	// no additional argument without space in between?
			{
				// check additional argument with space in between?
				if (optind < argc && *argv[optind] !=  '-')
				{
					optarg = argv[optind++];
				}
			}

			// optional absolute path name of directory?
			if (optarg && *optarg == '/')
				safe_strcpy(twindir, optarg, sizeof twindir);

			twinmodeflag++;
			break;

		   case 'B':		/* bar graphs ?               */
			displaymode = 'D';
			break;

		   case 'H':		/* bar graphs without labels? */
			barmono = 1;
			break;

		   case 'S':		/* midnight limit ?           */
			midnightflag++;
			break;

		   case 'i':		/* ID translation max name?   */
			idnamemaximum++;
			break;

		   case 'I':		/* suppress ID translation ?  */
			idnamesuppress++;
			break;

		   case 'Q':		/* suppress high priority?    */
			highpriosuppress++;
			break;

		   case 'b':		/* begin time ?               */
			if ( !getbranchtime(optarg, &begintime) )
				prusage(argv[0]);
			break;

		   case 'e':		/* end   time ?               */
			if ( !getbranchtime(optarg, &endtime) )
				prusage(argv[0]);
			break;

		   case 'P':		/* parsable output?          */
			if ( !parsedef(optarg) )
				prusage(argv[0]);

			if (!parseoutflag)
			{
				parseoutflag++;
				handlers[numhandlers++].handle_sample = parseout;
			}
			break;

		   case 'J':		/* json output?          */
			if ( !jsondef(optarg) )
				prusage(argv[0]);

			if (!jsonoutflag)
			{
				jsonoutflag++;
				handlers[numhandlers++].handle_sample = jsonout;
			}
			break;

		   case 'L':		/* line length                */
			if ( !numeric(optarg) )
				prusage(argv[0]);

			linelen = atoi(optarg);
			break;

		   case MALLACTIVE:	/* all processes/cgroups ? */
			deviatonly = 0;
			break;

		   case MCALCPSS:	/* calculate PSS per sample ? */
			if (rawreadflag)
			{
				fprintf(stderr, "PSIZE gathering depends on rawfile\n");
				sleep(3);
				break;
			}

			calcpss = 1;

			if (!rootprivs())
			{
				fprintf(stderr, "PSIZE gathering only for own processes\n");
				sleep(3);
			}

			break;

		   case MGETWCHAN:	/* obtain wchan string?       */
			getwchan = 1;
			break;

		   case MRMSPACES:	/* remove spaces from command */
			rmspaces = 1;
			break;

		   case 'z':            /* prepend regex matching environment variables */
			if (regcomp(&envregex, optarg, REG_NOSUB|REG_EXTENDED))
			{
				fprintf(stderr, "Invalid environment regular expression!");
				prusage(argv[0]);
			}
			prependenv = 1;
			break;

		   case 'k':		/* try to open TCP connection to atopgpud */
			connectgpud = 1;
			break;

		   case 'K':		/* try to open connection to netatop/netatop-bpf */
			connectnetatop = 1;
			break;

		   default:		/* gather other flags */
			if (c == PHSHOWGPU)
				c = MPERCGPU;

			flagrest[i++] = c;
		}

		/*
		** check if this flag explicitly refers to
		** generic (screen) output
		*/
		if (strchr("gmdnsevcoBGaCMDNEAupjSf", c))
			screenoutflag++;
	}

	/*
	** get optional interval value and optional number of samples	
	*/
	if (optind < argc && optind < MAXFL)
	{
		if (!numeric(argv[optind]))
			prusage(argv[0]);
	
		interval = atoi(argv[optind]);

		optind++;

		if (optind < argc)
		{
			if (!numeric(argv[optind]) )
				prusage(argv[0]);

			if ( (nsamples = atoi(argv[optind])) < 1)
				prusage(argv[0]);
		}
	}

	/*
	** verify if the generic handler has to be installed as default
	** (no other handler choosen) or if the generic screen handler
	** has to be added due to an explicit flag
	*/
	if (numhandlers == 0 || screenoutflag)
		handlers[numhandlers++].handle_sample = generic_samp;

	/*
	** determine the name of this node (without domain-name)
	** and the kernel-version
	*/
	(void) uname(&utsname);

	if ( (p = strchr(utsname.nodename, '.')) )
		*p = '\0';

	utsnodenamelen = strlen(utsname.nodename);

	sscanf(utsname.release, "%d.%d.%d", &osrel, &osvers, &ossub);

	/*
	** determine the clock rate and memory page size for this machine
	*/
	hertz		= sysconf(_SC_CLK_TCK);
	pagesize	= sysconf(_SC_PAGESIZE);
	pidwidth	= getpidwidth();

	/*
	** check if twin mode wanted with two atop processes:
	**
	** - lower half: gather statistics and write to raw file
	** - upper half: read statistics and present to user
	**
	** consistency checks
	*/
	if (twinmodeflag)
		twinprepare();

	/*
	** check if raw data from a file must be viewed
	*/
	if (rawreadflag)
	{
		rawread();
		cleanstop(0);
	}

	/*
	** be sure to be leader of an own process group when
	** running as a daemon (or at least: when not interactive);
	** needed for systemd
	*/
	if (rawwriteflag)
		(void) setpgid(0, 0);

	/*
	** determine start-time for gathering current statistics
	*/
	curtime = getboot() / hertz;

	/*
	** catch signals for proper close-down
	*/
	signal(SIGHUP,  cleanstop);
	signal(SIGTERM, cleanstop);

	/*
	** regain the root-privileges that we dropped at the beginning
	** to do some privileged work
	*/
	regainrootprivs();

	if ( !highpriosuppress )
	{
		/*
		** lock ATOP in memory to get reliable samples (also when
		** memory is low and swapping is going on);
		** ignored if not running under superuser privileges!
		*/
		rlim.rlim_cur	= RLIM_INFINITY;
		rlim.rlim_max	= RLIM_INFINITY;

		if (setrlimit(RLIMIT_MEMLOCK, &rlim) == 0)
			(void) mlockall(MCL_CURRENT|MCL_FUTURE);
	
		/*
		** increment CPU scheduling-priority to get reliable samples (also
		** during heavy CPU load);
		** ignored if not running under superuser privileges!
		*/
		if ( nice(-20) == -1)
			;

		set_oom_score_adj();
	}

	/*
	** switch-on the process-accounting mechanism to register the
	** (remaining) resource-usage by processes which have finished
	*/
	acctreason = acctswon();

	/*
	** determine properties (like speed) of all interfaces
	*/
	initifprop();

	/*
	** open socket to the IP layer to issue getsockopt() calls later on
	*/
	if (connectnetatop)
		netatop_ipopen();

	/*
	** since privileged activities are finished now, there is no
	** need to keep running under root-privileges, so switch
	** effective user-id to real user-id
	*/
        if (! droprootprivs() )
		mcleanstop(42, "failed to drop root privs\n");

	/*
	** determine if cgroups v2 is supported
	*/
	cgroupv2support();

	/*
	** determine if real NUMA is used
	*/
	realnuma_support();

	/*
	** determine if zswap is used
	*/
	zswap_support();

	/*
	** start the engine now .....
	*/
	engine();

	cleanstop(0);

	return 0;	/* never reached */
}

/*
** The engine() drives the main-loop of the program
*/
static void
engine(void)
{
	struct sigaction 	sigact;
	static time_t		timelimit;

	/*
	** reserve space for cgroup-level statistics
	*/
	static struct cgchainer	*devcstat;
	int			ncgroups = 0;
	int			npids    = 0;
	int			i;

	/*
	** reserve space for system-level statistics
	*/
	static struct sstat	*cursstat; /* current   */
	static struct sstat	*presstat; /* previous  */
	static struct sstat	*devsstat; /* deviation */
	static struct sstat	*hlpsstat;

	/*
	** reserve space for task-level statistics
	*/
	static struct tstat	*curtpres;	/* current present list      */
	static unsigned long	 curtlen;	/* size of present list      */
	struct tstat		*curpexit;	/* exited process list	     */

	static struct devtstat	devtstat;	/* deviation info	     */

	unsigned long		ntaskpres;	/* number of tasks present   */
	unsigned long		nprocexit;	/* number of exited procs    */
	unsigned long		nprocexitnet;	/* number of exited procs    */
						/* via netatopd daemon       */

	unsigned long		noverflow;

	int         		nrgpuproc=0,	/* number of GPU processes    */
				gpupending=0;	/* boolean: request sent      */

	struct gpupidstat	*gp = NULL;

	/*
	** initialization: allocate required memory dynamically
	*/
	cursstat = calloc(1, sizeof(struct sstat));
	presstat = calloc(1, sizeof(struct sstat));
	devsstat = calloc(1, sizeof(struct sstat));

	ptrverify(cursstat, "Malloc failed for current sysstats\n");
	ptrverify(presstat, "Malloc failed for prev    sysstats\n");
	ptrverify(devsstat, "Malloc failed for deviate sysstats\n");

	/*
	** install the signal-handler for ALARM, USR1 and USR2 (triggers
	* for the next sample)
	*/
	memset(&sigact, 0, sizeof sigact);
	sigact.sa_handler = getusr1;
	sigaction(SIGUSR1, &sigact, (struct sigaction *)0);

	memset(&sigact, 0, sizeof sigact);
	sigact.sa_handler = getusr2;
	sigaction(SIGUSR2, &sigact, (struct sigaction *)0);

	memset(&sigact, 0, sizeof sigact);
	sigact.sa_handler = getalarm;
	sigaction(SIGALRM, &sigact, (struct sigaction *)0);

	if (interval > 0)
		alarm(interval);

	if (midnightflag)
	{
		time_t		timenow = time(0);
		struct tm	*tp = localtime(&timenow);

		tp->tm_hour = 23;
		tp->tm_min  = 59;
		tp->tm_sec  = 59;

		timelimit = mktime(tp);
	}

	/*
 	** open socket to the atopgpud daemon for GPU statistics
	** if explicitly required
	*/
	if (connectgpud)
	{
        	nrgpus = gpud_init();

		if (nrgpus)
			supportflags |= GPUSTAT;
	}

	/*
	** MAIN-LOOP:
	**    -	Wait for the requested number of seconds or for other trigger
	**
	**    -	System-level counters
	**		get current counters
	**		calculate the differences with the previous sample
	**
	**    -	Process-level counters
	**		get current counters from running & exited processes
	**		calculate the differences with the previous sample
	**
	**    -	Call the print-function to visualize the differences
	*/
	for (sampcnt=0; sampcnt < nsamples; sampcnt++)
	{
		char	lastcmd = ' ';

		/*
		** if the limit-flag is specified:
		**  check if the next sample is expected before midnight;
		**  if not, stop atop now 
		*/
		if (midnightflag && (curtime+interval) > timelimit)
			break;

		/*
		** wait for alarm-signal to arrive (except first sample)
		** or wait for SIGUSR1/SIGUSR2
		*/
		if (sampcnt > 0 && awaittrigger)
			pause();

		awaittrigger = 1;

		/*
		** gather time info for this sample
		*/
		pretime  = curtime;
		curtime  = time(0);		/* seconds since 1-1-1970 */

		/*
		** send request for statistics to atopgpud 
		*/
		if (nrgpus)
		{
			if ((gpupending = gpud_statrequest()) == 0)
				nrgpus = 0;
		}

		/*
		** take a snapshot of the current system-level metrics 
		** and calculate the deviations (i.e. calculate the activity
		** during the last sample)
		*/
		hlpsstat = cursstat;	/* swap current/prev. stats */
		cursstat = presstat;
		presstat = hlpsstat;

		photosyst(cursstat);	/* obtain new system-level counters */

		/*
		** take a snapshot of the current cgroup-level metrics 
		** when cgroups v2 supported
		*/
		if ( (supportflags&CGROUPV2) )
			photocgroup();

		/*
		** receive and parse response from atopgpud
		*/
		if (nrgpus && gpupending)
		{
			nrgpuproc = gpud_statresponse(nrgpus, cursstat->gpu.gpu, &gp);

			gpupending = 0;

			// connection lost or timeout on receive?
			if (nrgpuproc == -1)
			{
				nrgpus = 0;
				supportflags &= ~GPUSTAT;
			}

			cursstat->gpu.nrgpus = nrgpus;
		}

		deviatsyst(cursstat, presstat, devsstat,
				curtime-pretime > 0 ? curtime-pretime : 1);


		/*
		** take a snapshot of the current task-level statistics 
		** and calculate the deviations (i.e. calculate the activity
		** during the last sample)
		**
		** first register active tasks
		*/
		curtpres  = NULL;

		do
		{
			curtlen   = counttasks();	// worst-case value
			curtpres  = realloc(curtpres,
					curtlen * sizeof(struct tstat));

			ptrverify(curtpres, "Malloc failed for %lu tstats\n",
								curtlen);

			memset(curtpres, 0, curtlen * sizeof(struct tstat));
		}
		while ( (ntaskpres = photoproc(curtpres, curtlen)) == curtlen);

		/*
		** register processes that exited during last sample;
		** first determine how many processes exited
		**
		** the number of exited processes is limited to avoid
		** that atop explodes in memory and introduces OOM killing
		*/
		nprocexit = acctprocnt();	/* number of exited processes */

		if (nprocexit > MAXACCTPROCS)
		{
			noverflow = nprocexit - MAXACCTPROCS;
			nprocexit = MAXACCTPROCS;
		}
		else
			noverflow = 0;

		/*
		** determine how many processes have been exited
		** for the netatop module (only processes that have
		** used the network)
		*/
		if (nprocexit > 0 && (supportflags & NETATOPD))
			nprocexitnet = netatop_exitstore();
		else
			nprocexitnet = 0;

		/*
		** reserve space for the exited processes and read them
		*/
		if (nprocexit > 0)
		{
			curpexit = malloc(nprocexit * sizeof(struct tstat));

			ptrverify(curpexit,
			          "Malloc failed for %lu exited processes\n",
			          nprocexit);

			memset(curpexit, 0, nprocexit * sizeof(struct tstat));

			nprocexit = acctphotoproc(curpexit, nprocexit);

			/*
 			** reposition offset in accounting file when not
			** all exited processes have been read (i.e. skip
			** those processes)
			*/
			if (noverflow)
				acctrepos(noverflow);
		}
		else
		{
			curpexit    = NULL;
		}

		/*
 		** merge GPU per-process stats with other per-process stats
		*/
		if (nrgpus && nrgpuproc > 0)
			gpumergeproc(curtpres, ntaskpres,
		                     curpexit, nprocexit,
		 	             gp,       nrgpuproc);

		/*
		** calculate process-level deviations
		*/
		deviattask(curtpres, ntaskpres, curpexit, nprocexit,
		                     &devtstat, devsstat);

		if (supportflags & NETATOPBPF)
		{
			g_hash_table_destroy(ghash_net);
			ghash_net = NULL;
		}

		/*
		** calculate cgroup-level v2 deviations
		**
		** allocation and deallocation of structs
		** is arranged at a lower level
		*/
		if ( (supportflags&CGROUPV2) )
			ncgroups = deviatcgroup(&devcstat, &npids);

		/*
		** activate the installed print function to visualize
		** the deviations
		*/
		for (i=0; handlers[i].handle_sample; i++)
		{
			lastcmd = (handlers[i].handle_sample)(curtime,
				     curtime-pretime > 0 ? curtime-pretime : 1,
		           	     &devtstat, devsstat,
				     devcstat, ncgroups, npids,
		                     nprocexit, noverflow, sampcnt==0);
		}

		/*
		** release dynamically allocated memory
		*/
		if (nprocexit > 0)
			free(curpexit);

		free(curtpres);

		if ((supportflags & NETATOPD) && (nprocexitnet > 0))
			netatop_exiterase();

		free(gp);
		gp = NULL;	// avoid double free

		if (lastcmd == MRESET)	/* reset requested ? */
		{
			sampcnt = -1;

			curtime = getboot() / hertz;	// reset current time

			/* set current (will be 'previous') counters to 0 */
			memset(cursstat, 0, sizeof(struct sstat));

			/* remove all tasks in database */
			pdb_makeresidue();
			pdb_cleanresidue();

			/* remove current cgroup info */
			cgwipecur();
		}
	} /* end of main-loop */
}


/*
** print usage of this command
*/
void
prusage(char *myname)
{
	// print generic part
	//
	printf("Usage: %s [OPTION]... [INTERVAL [SAMPLES]]\n", myname);
	printf("\t\tor\n");
	printf("Usage: %s -w  FILE  [OPTION]... [INTERVAL [SAMPLES]]\n", myname);
	printf("       %s -r [FILE] [OPTION]...\n", myname);
	printf("\n");

	pricmdopts(paramdef, sizeof paramdef/sizeof(struct pardef));

	// print footer
	//
	printf("\n");
	printf("  INTERVAL: number of seconds   (minimum 0, default 10)\n");
	printf("  SAMPLES:  number of intervals (minimum 1, default infinite)\n");
	printf("\n");
	printf("When the interval value is zero, a new sample can be\n");
	printf("forced manually by sending signal USR1 (kill -USR1 pid_atop)\n");
	printf("or with the keystroke '%c' in interactive mode.\n", MSAMPNEXT);
	printf("\n");
	printf("Please refer to the man-page of 'atop' for more details.\n");

	cleanstop(1);
}


/*
** dynamically prepare calling arguments for getopt_long()
**
** - pd:	list of parameter definitions  (input)
** - opt:	list of struct option elements (output)
** - flags:	list of flags                  (output)
*/
void
prepcmdopts(struct pardef *pd,    int nrpardef,
            struct option *opts,  int maxparam,
	    char          *flags, int maxflags)
{
	int i, j;

	for (i=j=0; i < nrpardef && i < maxparam && j < maxflags-1; i++)
	{
		// support long options
		//
		opts[i] = pd[i].option;

		// build flags string
		//
		if (pd[i].option.val < PHBASEVAL)
			flags[j++] = pd[i].option.val;

		if (pd[i].option.has_arg != no_argument)
			flags[j++] = ':';

		if (pd[i].option.has_arg == optional_argument)
			flags[j++] = ':';
	}

	if (i == maxparam || j == maxflags-1)
	{
		fprintf(stderr, "internal failure while handling options!\n");
		cleanstop(1);
	}
}

/*
**
*/
#define MSGSTARTCOL	23

void
pricmdopts(struct pardef *pd, int nrpardef)
{
	int		i, j;
	char		msgprefix[MSGSTARTCOL+1];

	// print all options and help messages
	//
	memset(msgprefix, ' ', MSGSTARTCOL);
	msgprefix[MSGSTARTCOL] = '\0';

	for (i=0; i < nrpardef; i++)
	{
		char optionbuf[32];

		// help message vailable?
		//
		if (pd[i].helpmsg)
		{
			// print short flag, if available
			//
			if (pd[i].option.val < PHBASEVAL)
				printf("  -%c, ", pd[i].option.val);
			else
				printf("      ");

			// print long flag, along with (optional) argument
			//
			memset(optionbuf, '\0', sizeof optionbuf);

			safe_strcpy(optionbuf, pd[i].option.name, sizeof optionbuf);

			if (pd[i].option.has_arg == optional_argument)
				strcat(optionbuf, "[");

			if (pd[i].option.has_arg != no_argument)
			{
				strcat(optionbuf, "=");
				optionbuf[strlen(optionbuf)] = pd[i].symbarg;
			}

			if (pd[i].option.has_arg == optional_argument)
				strcat(optionbuf, "]");

			printf("--%-14.14s ", optionbuf);
		       
			// print message
			//
			// messages that contain a '\n' should continue
			// on the next line aligned in the right column,
			// except when the '\n' is at the last position
			//
			for (j=0; pd[i].helpmsg[j]; j++)
			{
				putchar(pd[i].helpmsg[j]);

				if (pd[i].helpmsg[j] == '\n' && pd[i].helpmsg[j+1])
					printf("%s", msgprefix);
			}

			putchar('\n');
		}
	}
}


/*
** handler for ALRM-signal
*/
void
getalarm(int sig)
{
	awaittrigger=0;

	if (interval > 0)
		alarm(interval);	/* restart the timer */
}

/*
** handler for USR1-signal
*/
void
getusr1(int sig)
{
	awaittrigger=0;
}

/*
** handler for USR2-signal
*/
void
getusr2(int sig)
{
	awaittrigger=0;
	nsamples = sampcnt;	// force stop after next sample
}

/*
** functions to handle a particular tag in the .atoprc file
*/
static void
do_interval(char *name, char *val)
{
	interval = get_posval(name, val);
}

static void
do_linelength(char *name, char *val)
{
	linelen = get_posval(name, val);
}

/*
** read RC-file and modify defaults accordingly
*/
static void
readrc(char *path, int syslevel)
{
	int	i, nr, line=0, errorcnt = 0;

	/*
	** check if this file is readable with the user's
	** *real uid/gid* with syscall access()
	*/
	if ( access(path, R_OK) == 0)
	{
		FILE	*fp;
		char	linebuf[256], tagname[20], tagvalue[256];

		fp = fopen(path, "r");
		if (fp == NULL) {
			fprintf(stderr, "Error: Could not open file %s\n", path);
			return;
		}

		while ( fgets(linebuf, sizeof linebuf, fp) )
		{
			line++;

			i = strlen(linebuf);

			if (i <= 1)	// empty line?
				continue;

			if (linebuf[i-1] == '\n')
				linebuf[i-1] = 0;

			nr = sscanf(linebuf, "%19s %255[^#]",
						tagname, tagvalue);

			switch (nr)
			{
			   case 0:
				continue;

			   case 1:
				if (tagname[0] == '#')
					continue;

				mcleanstop(1,
					"%s: syntax error line "
					"%d (no value specified)\n",
					path, line);

				break;		/* not reached */

			   default:
				if (tagname[0] == '#')
					continue;
				
				if (tagvalue[0] != '#')
					break;

				mcleanstop(1,
					"%s: syntax error line "
					"%d (no value specified)\n",
					path, line);
			}

			/*
			** tag name and tag value found
			** try to recognize tag name
			*/
			for (i=0; i < sizeof manrc/sizeof manrc[0]; i++)
			{
				if ( strcmp(tagname, manrc[i].tag) == 0)
				{
					if (manrc[i].sysonly && !syslevel)
					{
						fprintf(stderr,
						   "%s: warning at line %2d "
						   "- tag name %s not allowed "
						   "in private atoprc\n",
							path, line, tagname);

						errorcnt++;
						break;
					}

					manrc[i].func(tagname, tagvalue);
					break;
				}
			}

			/*
			** tag name not recognized
			*/
			if (i == sizeof manrc/sizeof manrc[0])
			{
				fprintf(stderr,
					"%s: warning at line %2d "
					"- tag name %s not recognized\n",
					path, line, tagname);

				errorcnt++;
			}
		}

		if (errorcnt)
			sleep(2);

		fclose(fp);
	}
}

/*
** prepare twin mode
*/
#define TWINNAME	"atoptwinXXXXXX"
static char		*tempname;

static void
twinprepare(void)
{
	char	eventbuf[1024];
	int	tempfd;

	/*
	** consistency checks for used options
	*/
	if (rawreadflag)
	{
		fprintf(stderr, "twin mode can not be combined with -r\n");
        	exit(42);
	}

	if (rawwriteflag)
	{
		fprintf(stderr, "twin mode can not be combined with -w\n");
        	exit(42);
	}

	if (parseoutflag)
	{
		fprintf(stderr, "twin mode can not be combined with -P\n");
        	exit(42);
	}

	if (jsonoutflag)
	{
		fprintf(stderr, "twin mode can not be combined with -J\n");
        	exit(42);
	}

	if (!isatty(fileno(stdout)) )	// output to pipe or file?
	{
		fprintf(stderr, "twin mode only for interactive use\n");
        	exit(42);
	}

	/*
	** create unique temporary file
	*/
	if (strlen(twindir) + sizeof TWINNAME + 1 >= RAWNAMESZ)
	{
		fprintf(stderr, "twin mode directory path too long\n");
        	exit(42);
	}

	tempname = malloc(strlen(twindir) + sizeof TWINNAME + 1);

	ptrverify(tempname, "Malloc failed for temporary twin name\n");

	snprintf(tempname, strlen(twindir) + sizeof TWINNAME + 1, "%s/%s", twindir, TWINNAME);

	if ( (tempfd = mkstemp(tempname)) == -1)
	{
		fprintf(stderr, "%s: ", tempname);
		perror("twin mode file creation");
        	exit(42);
	}

	/*
	** create lower half as child process
	*/
	switch (twinpid = fork())
	{
	   case -1:
		perror("fork twin process");
        	exit(42);

	   case 0:	// lower half: gather data and write to rawfile
		rawwriteflag++;

		handlers[0].handle_sample = rawwrite;
		break;

	   default:	// upper half: read from raw file and visualize
		rawreadflag++;

		/*
		** created inotify instance to be awoken when the lower half
		** has written a new sample to the temporary file 
		*/
		if ( (fdinotify = inotify_init()) == -1)
		{
			perror("twin mode inotify init");
        		exit(42);
		}

		(void) inotify_add_watch(fdinotify, tempname, IN_MODIFY);

		/*
		** arrange an automic kill of the lower half
		** at the moment that the upper half terminates
		*/
		atexit(twinclean);

		/*
		** wait for first sample to be written by lower half
		*/
		(void) read(fdinotify, eventbuf, sizeof eventbuf);
	}

	/*
	** define current raw file name for both parent and child
	*/
	safe_strcpy(irawname, tempname, sizeof irawname);
	safe_strcpy(orawname, tempname, sizeof orawname);
}

/*
** kill twin process that gathers data and
** remove the temporary raw file
*/
static void
twinclean(void)
{
	if (twinpid)    // kill lower half process
		kill(twinpid, SIGTERM);

	(void) unlink(tempname);
}
