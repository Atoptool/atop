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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
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

#define	allflags  "ab:cde:fghijklmnopqrstuvwxyz:123456789ABCDEFGHIJ:KL:MNOP:QRSTUVWXYZ"
#define	MAXFL		84      /* maximum number of command-line flags  */

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
char		rawname[RAWNAMESZ];
char		rawreadflag;
time_t		begintime, endtime, cursortime;	// epoch or time in day
char		flaglist[MAXFL];
char		deviatonly = 1;
char      	usecolors  = 1;  /* boolean: colors for high occupation  */
char		threadview = 0;	 /* boolean: show individual threads     */
char      	calcpss    = 0;  /* boolean: read/calculate process PSS  */
char      	getwchan   = 0;  /* boolean: obtain wchan string         */
char      	rmspaces   = 0;  /* boolean: remove spaces from command  */
		                 /* name in case of parsable output      */

char            displaymode = 'T';      /* 'T' = text, 'D' = draw        */
char            barmono     = 0; /* boolean: bar without categories?     */
		                 /* name in case of parseable output     */

char		prependenv = 0;  /* boolean: prepend selected            */
				 /* environment variables to cmdline     */
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
char		**argvp;


struct visualize vis = {generic_samp, generic_error,
			generic_end,  generic_usage};

/*
** argument values
*/
static char		awaittrigger;	/* boolean: awaiting trigger */
static unsigned int 	nsamples = 0xffffffff;
static char		midnightflag;
static char		rawwriteflag;

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

int
main(int argc, char *argv[])
{
	register int	i;
	int		c;
	char		*p;
	struct rlimit	rlim;

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
	** preserve command arguments to allow restart of other version
	*/
	argvp = argv;

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

	if ( memcmp(p, "atopsar", 7) == 0)
		return atopsar(argc, argv);

	/* 
	** interpret command-line arguments & flags 
	*/
	if (argc > 1)
	{
		/* 
		** gather all flags for visualization-functions
		**
		** generic flags will be handled here;
		** unrecognized flags are passed to the print-routines
		*/
		i = 0;

		while (i < MAXFL-1 && (c=getopt(argc, argv, allflags)) != EOF)
		{
			switch (c)
			{
			   case '?':		/* usage wanted ?             */
				prusage(argv[0]);
				break;

			   case 'V':		/* version wanted ?           */
				printf("%s\n", getstrvers());
				exit(0);

			   case 'w':		/* writing of raw data ?      */
				rawwriteflag++;
				if (optind >= argc)
					prusage(argv[0]);

				strncpy(rawname, argv[optind++], RAWNAMESZ-1);
				vis.show_samp = rawwrite;
				break;

			   case 'r':		/* reading of raw data ?      */
				if (optind < argc)
				{
					if (*(argv[optind]) == '-')
					{
						if (strlen(argv[optind]) == 1)
						{
							strcpy(rawname, "/dev/stdin");
							optind++;
						}
					}
					else
					{
						strncpy(rawname, argv[optind],
								RAWNAMESZ-1);
						optind++;
					}
				}

				rawreadflag++;
				break;

			   case 't':		/* twin mode ?		      */
				// optional absolute path name of directory?
				if (optind < argc)
				{
					if (*(argv[optind]) == '/')
					{
						strncpy(twindir, argv[optind],
								RAWNAMESZ-1);
						optind++;
					}
				}

				twinmodeflag++;
				break;

			   case 'B':		/* bar graphs ?               */
				displaymode = 'D';
				break;

			   case 'H':		/* bar graphs ?               */
				barmono = 1;
				break;

			   case 'S':		/* midnight limit ?           */
				midnightflag++;
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

				vis.show_samp = parseout;
				break;

                           case 'J':		/* json output?          */
				if ( !jsondef(optarg) )
					prusage(argv[0]);

				vis.show_samp = jsonout;
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
					fprintf(stderr,
					        "PSIZE gathering depends on rawfile\n");
					sleep(3);
					break;
				}

                                calcpss    = 1;

				if (!rootprivs())
				{
					fprintf(stderr,
						"PSIZE gathering only for own "
						"processes\n");
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

			   default:		/* gather other flags */
				flaglist[i++] = c;
			}
		}

		/*
		** get optional interval-value and optional number of samples	
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
	}

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
	*/
        nrgpus = gpud_init();

	if (nrgpus)
		supportflags |= GPUSTAT;

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
		char	lastcmd;

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
			gpupending = gpud_statrequest();

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
				int ng;

				// try to reconnect
        			ng = gpud_init();

				if (ng != nrgpus)	// no success
					nrgpus = 0;

				if (nrgpus)
				{
					// request for stats again
					if (gpud_statrequest())
					{
						// receive stats response
						nrgpuproc = gpud_statresponse(nrgpus,
						     cursstat->gpu.gpu, &gp);

						// persistent failure?
						if (nrgpuproc == -1)
							nrgpus = 0;
					}
				}
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
		if (nrgpus && nrgpuproc)
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
		lastcmd = (vis.show_samp)(curtime,
				     curtime-pretime > 0 ? curtime-pretime : 1,
		           	     &devtstat, devsstat,
				     devcstat, ncgroups, npids,
		                     nprocexit, noverflow, sampcnt==0);

		/*
		** release dynamically allocated memory
		*/
		if (nprocexit > 0)
			free(curpexit);

		free(curtpres);

		if ((supportflags & NETATOPD) && (nprocexitnet > 0))
			netatop_exiterase();

		if (gp)
			free(gp);

		if (lastcmd == MRESET)	/* reset requested ? */
		{
			sampcnt = -1;

			curtime = getboot() / hertz;	// reset current time

			/* set current (will be 'previous') counters to 0 */
			memset(cursstat, 0,           sizeof(struct sstat));

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
	printf("Usage: %s [-t [absdir]] [-flags] [interval [samples]]\n",
					myname);
	printf("\t\tor\n");
	printf("Usage: %s -w  file  [-S] [-%c] [interval [samples]]\n",
					myname, MALLACTIVE);
	printf("       %s -r [file] [-b [YYYYMMDD]hhmm[ss]] [-e [YYYYMMDD]hhmm[ss]] [-flags]\n",
					myname);
	printf("\n");
	printf("\tgeneric flags:\n");
	printf("\t  -t   twin mode: live measurement with possibility to review earlier samples\n");
	printf("\t                  (raw file created in /tmp or in specific directory path)\n");
	printf("\t  -%c  show bar graphs for system statistics\n", MBARGRAPH);
	printf("\t  -%c  show bar graphs without categories\n", MBARMONO);
	printf("\t  -%c  show cgroup v2 metrics\n", MCGROUPS);
	printf("\t  -7  define cgroup v2 depth level -2 till -9 (default: -7)\n");
	printf("\t  -%c  show version information\n", MVERSION);
	printf("\t  -%c  show all processes and cgroups (i.s.o. active only)\n",
			MALLACTIVE);
	printf("\t  -%c  calculate proportional set size (PSS) per process\n", 
	                MCALCPSS);
	printf("\t  -%c  determine WCHAN (string) per thread\n", MGETWCHAN);
	printf("\t  -P  generate parsable output for specified label(s)\n");
	printf("\t  -J  generate JSON output for specified label(s)\n");
	printf("\t  -%c  no spaces in parsable output for command (line)\n",
			MRMSPACES);
	printf("\t  -L  alternate line length (default 80) in case of "
			"non-screen output\n");
	printf("\t  -z  prepend regex matching environment variables to "
                        "command line\n");

	if (vis.show_usage)
		(*vis.show_usage)();

	printf("\n");
	printf("\tspecific flags for raw logfiles:\n");
	printf("\t  -w  write raw data to   file (compressed)\n");
	printf("\t  -r  read  raw data from file (compressed)\n");
	printf("\t      symbolic file: y[y...] for yesterday (repeated)\n");
	printf("\t      file name '-': read raw data from stdin\n");
	printf("\t  -S  finish atop automatically before midnight "
	                "(i.s.o. #samples)\n");
	printf("\t  -b  begin showing data from specified date/time\n");
	printf("\t  -e  finish showing data after specified date/time\n");
	printf("\n");
	printf("\tinterval: number of seconds   (minimum 0)\n");
	printf("\tsamples:  number of intervals (minimum 1)\n");
	printf("\n");
	printf("If the interval-value is zero, a new sample can be\n");
	printf("forced manually by sending signal USR1"
			" (kill -USR1 pid_atop)\n");
	printf("or with the keystroke '%c' in interactive mode.\n", MSAMPNEXT);
	printf("\n");
	printf("Please refer to the man-page of 'atop' for more details.\n");

	cleanstop(1);
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
extern char		twindir[];
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

	if (!isatty(fileno(stdout)) )	// output to pipe or file?
	{
		fprintf(stderr, "twin mode only for interactive use\n");
        	exit(42);
	}

	/*
	** created unique temporary file
	*/
	if (strlen(twindir) + sizeof TWINNAME + 1 >= RAWNAMESZ)
	{
		fprintf(stderr, "twin mode directoy path too long\n");
        	exit(42);
	}

	tempname = malloc(strlen(twindir) + sizeof TWINNAME + 1);

	ptrverify(tempname, "Malloc failed for temporary twin name\n");

	sprintf(tempname, "%s/%s", twindir, TWINNAME);

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

		vis.show_samp = rawwrite;
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
	strncpy(rawname, tempname, RAWNAMESZ-1);
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
