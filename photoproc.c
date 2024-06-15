/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-/thread-level.
**
** This source-file contains functions to read the process-administration
** of every running process from kernel-space and extract the required
** activity-counters.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
** --------------------------------------------------------------------------
** Copyright (C) 2000-2022 Gerlof Langeveld
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
#include <glib.h>

#include "atop.h"
#include "photoproc.h"
#include "netatop.h"

#define	SCANSTAT 	"%c   %d   %*d  %*d  %*d %*d  "	\
			"%*d  %lld %*d  %lld %*d %lld "	\
			"%lld %*d  %*d  %d   %d  %*d  "	\
			"%*d  %ld  %lld %lld %*d %*d  "	\
			"%*d  %*d  %*d  %*d  %*d %*d  " \
			"%*d  %*d  %*d  %*d  %*d %*d  "	\
			"%d   %d   %d   %lld"

static int	procstat(struct tstat *, unsigned long long, char);
static int	procstatus(struct tstat *);
static int	procio(struct tstat *);
static void	proccmd(struct tstat *);
static void	procsmaps(struct tstat *);
static void	procwchan(struct tstat *);
static count_t	procschedstat(struct tstat *);

extern GHashTable *ghash_net;

extern char	prependenv;
extern regex_t  envregex;


unsigned long
photoproc(struct tstat *tasklist, int maxtask)
{
	static int			firstcall = 1;
	static unsigned long long	bootepoch;

	register struct tstat	*curtask;

	FILE		*fp;
	DIR		*dirp;
	struct dirent	*entp;
	char		origdir[4096], dockstat=0;
	unsigned long	tval=0;

	/*
	** one-time initialization stuff
	*/
	if (firstcall)
	{
		/*
		** check if this kernel offers io-statistics per task
		*/
		regainrootprivs();

		if ( (fp = fopen("/proc/1/io", "r")) )
		{
			supportflags |= IOSTAT;
			fclose(fp);
		}

		if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");

		/*
 		** find epoch time of boot moment
		*/
		bootepoch = getboot();

		firstcall = 0;
	}

	/*
	** probe if the netatop module and (optionally) the
	** netatopd daemon are active
	*/
	regainrootprivs();

	/*
	** if kernel module is  not active on this system,
	** netatop-bpf will try tp run;
	*/
	if (!(supportflags & NETATOPD)) {
		netatop_bpf_probe();
	}
	/*
	** if netatop-bpf is  not active on this system,
	** kernel module will try to run;
	*/
	if (!(supportflags & NETATOPBPF)) {
		netatop_probe();
	}

	/*
	** if netatop-bpf is active on this system, skip call
	*/
	if (supportflags & NETATOPBPF) {
		netatop_bpf_gettask();
	}

	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

	/*
	** read all subdirectory-names below the /proc directory
	*/
	if ( getcwd(origdir, sizeof origdir) == NULL)
		mcleanstop(53, "failed to save current dir\n");

	if ( chdir("/proc") == -1)
		mcleanstop(54, "failed to change to /proc\n");

	dirp = opendir(".");

	while ( (entp = readdir(dirp)) && tval < maxtask )
	{
		/*
		** skip non-numerical names
		*/
		if (!isdigit(entp->d_name[0]))
			continue;

		/*
		** change to the process' subdirectory
		*/
		if ( chdir(entp->d_name) != 0 )
			continue;

		/*
 		** gather process-level information
		*/
		curtask	= tasklist+tval;

		if ( !procstat(curtask, bootepoch, 1)) /* from /proc/pid/stat */
		{
			if ( chdir("..") == -1)
				;
			continue;
		}

		if ( !procstatus(curtask) )	/* from /proc/pid/status  */
		{
			if ( chdir("..") == -1)
				;
			continue;
		}

		if ( !procio(curtask) )		/* from /proc/pid/io      */
		{
			if ( chdir("..") == -1)
				;
			continue;
		}

		procschedstat(curtask);			/* from /proc/pid/schedstat */
		proccmd(curtask);			/* from /proc/pid/cmdline */
		dockstat += getutsname(curtask);	/* retrieve container/pod name */

		/*
		** reading the smaps file for every process with every sample
		** is a really 'expensive' from a CPU consumption point-of-view,
		** so gathering this info is optional
		*/
		if (calcpss)
			procsmaps(curtask);	/* from /proc/pid/smaps */

		/*
		** determine thread's wchan, if wanted ('expensive' from
		** a CPU consumption point-of-view)
		*/
                if (getwchan)
                	procwchan(curtask);

		if (supportflags & NETATOPBPF) {
			struct taskcount *tc = g_hash_table_lookup(ghash_net, &(curtask->gen.tgid));
			if (tc) {
				// printf("%d %d %d %d %d\n",curtask->gen.tgid, tc->tcpsndpacks,  tc->tcprcvpacks, tc->udpsndpacks, tc->udprcvpacks);
				curtask->net.tcpsnd = tc->tcpsndpacks;
				curtask->net.tcprcv = tc->tcprcvpacks;
				curtask->net.tcpssz = tc->tcpsndbytes;
				curtask->net.tcprsz = tc->tcprcvbytes;

				curtask->net.udpsnd = tc->udpsndpacks;
				curtask->net.udprcv = tc->udprcvpacks;
				curtask->net.udpssz = tc->udpsndbytes;
				curtask->net.udprsz = tc->udprcvbytes;
			}
		} else {
			// read network stats from netatop
			netatop_gettask(curtask->gen.tgid, 'g', curtask);
		}

		tval++;		/* increment for process-level info */

		/*
 		** if needed (when number of threads is larger than 1):
		**   read and fill new entries with thread-level info
		*/
		if (curtask->gen.nthr > 1)
		{
			DIR		*dirtask;
			struct dirent	*tent;

			curtask->gen.nthrrun  = 0;
			curtask->gen.nthrslpi = 0;
			curtask->gen.nthrslpu = 0;
			curtask->gen.nthridle = 0;

			/*
 			** rundelay and blkdelay on process level only
			** concerns the delays of the main thread;
			** totalize the delays of all threads
			*/
			curtask->cpu.rundelay = 0;
			curtask->cpu.blkdelay = 0;

			/*
 			** nvcsw and nivcsw on process level only
			** concerns the delays of the main thread;
			** totalize the delays of all threads
			*/
			curtask->cpu.nvcsw  = 0;
			curtask->cpu.nivcsw = 0;

			/*
			** open underlying task directory
			*/
			if ( chdir("task") == 0 )
			{
				unsigned long cur_nth = 0;

				dirtask = opendir(".");

				/*
				** due to race condition, opendir() might
				** have failed (leave task and process-level
				** directories)
				*/
				if( dirtask == NULL )
				{
					if(chdir("../..") == -1)
						;
					continue;
				}

				while ((tent=readdir(dirtask)) && tval<maxtask)
				{
					struct tstat *curthr = tasklist+tval;

					/*
					** change to the thread's subdirectory
					*/
					if ( tent->d_name[0] == '.'  ||
					     chdir(tent->d_name) != 0 )
						continue;

					if ( !procstat(curthr, bootepoch, 0))
					{
						if ( chdir("..") == -1)
							;
						continue;
					}

					if ( !procstatus(curthr) )
					{
						if ( chdir("..") == -1)
							;
						continue;
					}

					if ( !procio(curthr) )
					{
						if ( chdir("..") == -1)
							;
						continue;
					}

					/*
					** determine thread's wchan, if wanted
 					** ('expensive' from a CPU consumption
					** point-of-view)
					*/
                			if (getwchan)
                        			procwchan(curthr);

					// totalize values of all threads
					curtask->cpu.rundelay +=
						procschedstat(curthr);

					curtask->cpu.blkdelay +=
						curthr->cpu.blkdelay;

					curtask->cpu.nvcsw +=
						curthr->cpu.nvcsw;

					curtask->cpu.nivcsw +=
						curthr->cpu.nivcsw;

					// continue gathering
					strcpy(curthr->gen.utsname,
						curtask->gen.utsname);

					switch (curthr->gen.state)
					{
	   		   		   case 'R':
						curtask->gen.nthrrun  += 1;
						break;
	   		   		   case 'S':
						curtask->gen.nthrslpi += 1;
						break;
	   		   		   case 'D':
						curtask->gen.nthrslpu += 1;
						break;
	   		   		   case 'I':
						curtask->gen.nthridle += 1;
						break;
					}

					curthr->gen.nthr = 1;

					// try to read network stats from netatop's module
					if (!(supportflags & NETATOPBPF)) {
						netatop_gettask(curthr->gen.pid, 't',
										curthr);
					}
					// all stats read now
					tval++;	    /* increment thread-level */
					cur_nth++;  /* increment # threads    */

					if ( chdir("..") == -1)
						; /* thread */
				}

				closedir(dirtask);
				if ( chdir("..") == -1)
					; /* leave task */

				// calibrate number of threads
				curtask->gen.nthr = cur_nth;
			}
		}

		if ( chdir("..") == -1)
			; /* leave process-level directry */
	}

	closedir(dirp);

	if ( chdir(origdir) == -1)
		mcleanstop(55, "cannot change to %s\n", origdir);

	if (dockstat)
		supportflags |= CONTAINERSTAT;
	else
		supportflags &= ~CONTAINERSTAT;

	resetutsname();		// reassociate atop with own UTS namespace

	return tval;
}

/*
** count number of tasks in the system, i.e.
** the number of processes plus the total number of threads
*/
unsigned long
counttasks(void)
{
	unsigned long	nrproc=0, nrthread=0;
	char		linebuf[256];
	FILE		*fp;
	DIR             *dirp;
	struct dirent   *entp;
	char            origdir[1024];

	/*
	** determine total number of threads
	*/
	if ( (fp = fopen("/proc/loadavg", "r")) != NULL)
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if ( sscanf(linebuf, "%*f %*f %*f %*d/%lu", &nrthread) < 1)
				mcleanstop(53, "wrong /proc/loadavg\n");
		}
		else
			mcleanstop(53, "unreadable /proc/loadavg\n");

		fclose(fp);
	}
	else
		mcleanstop(53, "can not open /proc/loadavg\n");


	/*
	** add total number of processes
	*/
	if ( getcwd(origdir, sizeof origdir) == NULL)
		mcleanstop(53, "cannot determine cwd\n");

	if ( chdir("/proc") == -1)
		mcleanstop(53, "cannot change to /proc\n");

	dirp = opendir(".");

	while ( (entp = readdir(dirp)) )
	{
		/*
		** count subdirectory names under /proc starting with a digit
		*/
		if (isdigit(entp->d_name[0]))
			nrproc++;
	}

	closedir(dirp);

	if ( chdir(origdir) == -1)
		mcleanstop(53, "cannot change to %s\n", origdir);

	if (nrthread < nrproc)
		mcleanstop(53, "#threads (%ld) < #procs (%ld)\n",
					nrthread, nrproc);

	return nrproc + nrthread;
}

/*
** open file "stat" and obtain required info
*/
static int
procstat(struct tstat *curtask, unsigned long long bootepoch, char isproc)
{
	FILE	*fp;
	int	nr;
	char	line[4096], *p, *cmdhead, *cmdtail;

	if ( (fp = fopen("stat", "r")) == NULL)
		return 0;

	if ( (nr = fread(line, 1, sizeof line-1, fp)) == 0)
	{
		fclose(fp);
		return 0;
	}

	line[nr] = '\0';	// terminate string

	/*
    	** fetch command name
	*/
	cmdhead = strchr (line, '(');
	cmdtail = strrchr(line, ')');

	if (!cmdhead || !cmdtail || cmdtail < cmdhead) // parsing failed?
	{
		fclose(fp);
		return 0;
	}

	if ( (nr = cmdtail-cmdhead-1) > PNAMLEN)
		nr = PNAMLEN;

	p = curtask->gen.name;

	memcpy(p, cmdhead+1, nr);
	*(p+nr) = 0;

	while ( (p = strchr(p, '\n')) != NULL)
	{
		*p = '?';
		p++;
	}

	/*
  	** fetch other values
  	*/
	curtask->gen.isproc  = isproc;
	curtask->cpu.rtprio  = 0;
	curtask->cpu.policy  = 0;
	curtask->gen.excode  = 0;

	sscanf(line, "%d", &(curtask->gen.pid));  /* fetch pid */

	nr = sscanf(cmdtail+2, SCANSTAT,
		&(curtask->gen.state), 	&(curtask->gen.ppid),
		&(curtask->mem.minflt),	&(curtask->mem.majflt),
		&(curtask->cpu.utime),	&(curtask->cpu.stime),
		&(curtask->cpu.prio),	&(curtask->cpu.nice),
		&(curtask->gen.btime),
		&(curtask->mem.vmem),	&(curtask->mem.rmem),
		&(curtask->cpu.curcpu),	&(curtask->cpu.rtprio),
		&(curtask->cpu.policy), &(curtask->cpu.blkdelay));

	if (nr < 12)		/* parsing failed? */
	{
		fclose(fp);
		return 0;
	}

	/*
 	** normalization
	*/
	curtask->gen.btime   = (curtask->gen.btime+bootepoch)/hertz;
	curtask->cpu.prio   += 100; 	/* was subtracted by kernel */
	curtask->mem.vmem   /= 1024;
	curtask->mem.rmem   *= pagesize/1024;

	fclose(fp);

	switch (curtask->gen.state)
	{
  	   case 'R':
		curtask->gen.nthrrun  = 1;
		break;
  	   case 'S':
		curtask->gen.nthrslpi = 1;
		break;
  	   case 'D':
		curtask->gen.nthrslpu = 1;
		break;
	   case 'I':
		curtask->gen.nthridle = 1;
		break;
	}

	return 1;
}

/*
** open file "status" and obtain required info
*/
static int
procstatus(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];

	if ( (fp = fopen("status", "r")) == NULL)
		return 0;

	curtask->gen.nthr     = 1;	/* for compat with 2.4 */
	curtask->cpu.sleepavg = 0;	/* for compat with 2.4 */
	curtask->mem.vgrow    = 0;	/* calculated later */
	curtask->mem.rgrow    = 0;	/* calculated later */

	while (fgets(line, sizeof line, fp))
	{
		if (memcmp(line, "Tgid:", 5) ==0)
		{
			sscanf(line, "Tgid: %d", &(curtask->gen.tgid));
			continue;
		}

		if (memcmp(line, "Pid:", 4) ==0)
		{
			sscanf(line, "Pid: %d", &(curtask->gen.pid));
			continue;
		}

		if (memcmp(line, "SleepAVG:", 9)==0)
		{
			sscanf(line, "SleepAVG: %d%%",
				&(curtask->cpu.sleepavg));
			continue;
		}

		if (memcmp(line, "Uid:", 4)==0)
		{
			sscanf(line, "Uid: %d %d %d %d",
				&(curtask->gen.ruid), &(curtask->gen.euid),
				&(curtask->gen.suid), &(curtask->gen.fsuid));
			continue;
		}

		if (memcmp(line, "Gid:", 4)==0)
		{
			sscanf(line, "Gid: %d %d %d %d",
				&(curtask->gen.rgid), &(curtask->gen.egid),
				&(curtask->gen.sgid), &(curtask->gen.fsgid));
			continue;
		}

		if (memcmp(line, "envID:", 6) ==0)
		{
			sscanf(line, "envID: %d", &(curtask->gen.ctid));
			continue;
		}

		if (memcmp(line, "VPid:", 5) ==0)
		{
			sscanf(line, "VPid: %d", &(curtask->gen.vpid));
			continue;
		}

		if (memcmp(line, "Threads:", 8)==0)
		{
			sscanf(line, "Threads: %d", &(curtask->gen.nthr));
			continue;
		}

		if (memcmp(line, "VmData:", 7)==0)
		{
			sscanf(line, "VmData: %lld", &(curtask->mem.vdata));
			continue;
		}

		if (memcmp(line, "VmStk:", 6)==0)
		{
			sscanf(line, "VmStk: %lld", &(curtask->mem.vstack));
			continue;
		}

		if (memcmp(line, "VmExe:", 6)==0)
		{
			sscanf(line, "VmExe: %lld", &(curtask->mem.vexec));
			continue;
		}

		if (memcmp(line, "VmLib:", 6)==0)
		{
			sscanf(line, "VmLib: %lld", &(curtask->mem.vlibs));
			continue;
		}

		if (memcmp(line, "VmSwap:", 7)==0)
		{
			sscanf(line, "VmSwap: %lld", &(curtask->mem.vswap));
			continue;
		}

		if (memcmp(line, "VmLck:", 6)==0)
		{
			sscanf(line, "VmLck: %lld", &(curtask->mem.vlock));
			continue;
		}

		if (memcmp(line, "voluntary_ctxt_switches:", 24)==0)
		{
			sscanf(line, "voluntary_ctxt_switches: %lld", &(curtask->cpu.nvcsw));
			continue;
		}

		if (memcmp(line, "nonvoluntary_ctxt_switches:", 27)==0)
		{
			sscanf(line, "nonvoluntary_ctxt_switches: %lld", &(curtask->cpu.nivcsw));
			continue;
		}
	}

	fclose(fp);
	return 1;
}

/*
** open file "io" (>= 2.6.20) and obtain required info
*/
#define	IO_READ		"read_bytes:"
#define	IO_WRITE	"write_bytes:"
#define	IO_CWRITE	"cancelled_write_bytes:"
static int
procio(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];
	count_t	dskrsz=0, dskwsz=0, dskcwsz=0;

	if (supportflags & IOSTAT)
	{
		regainrootprivs();

		if ( (fp = fopen("io", "r")) )
		{
			while (fgets(line, sizeof line, fp))
			{
				if (memcmp(line, IO_READ,
						sizeof IO_READ -1) == 0)
				{
					sscanf(line, "%*s %llu", &dskrsz);
					dskrsz /= 512;		// in sectors
					continue;
				}

				if (memcmp(line, IO_WRITE,
						sizeof IO_WRITE -1) == 0)
				{
					sscanf(line, "%*s %llu", &dskwsz);
					dskwsz /= 512;		// in sectors
					continue;
				}

				if (memcmp(line, IO_CWRITE,
						sizeof IO_CWRITE -1) == 0)
				{
					sscanf(line, "%*s %llu", &dskcwsz);
					dskcwsz /= 512;		// in sectors
					continue;
				}
			}

			fclose(fp);

			curtask->dsk.rsz	= dskrsz;
			curtask->dsk.rio	= dskrsz;  // to enable sort
			curtask->dsk.wsz	= dskwsz;
			curtask->dsk.wio	= dskwsz;  // to enable sort
			curtask->dsk.cwsz	= dskcwsz;
		}

		if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");
	}

	return 1;
}

/*
** store the full command line
**
** the command-line may contain:
**    - null-bytes as a separator between the arguments
**    - newlines (e.g. arguments for awk or sed)
**    - tabs (e.g. arguments for awk or sed)
** these special bytes will be converted to spaces
**
** the command line may be prepended by environment variables
*/

#define	ABBENVLEN	16

static void
proccmd(struct tstat *curtask)
{
	FILE		*fp, *fpe;
	register int 	i, nr;
	ssize_t		env_len = 0;
	register char	*pc = curtask->gen.cmdline;

	memset(curtask->gen.cmdline, 0, CMDLEN+1); // initialize command line

	// prepend by environment variables (if required)
	//
	if ( prependenv && (fpe = fopen("environ", "r")) != NULL)
	{
		char *line = NULL;
		ssize_t nread;
		size_t len = 0;

		while ((nread = getdelim(&line, &len, '\0', fpe)) != -1)
		{
			if (nread > 0 && !regexec(&envregex, line, 0, NULL, 0))
			{
				if (env_len + nread >= CMDLEN)
				{
					// try to add abbreviated env string
					//
					if (env_len + ABBENVLEN + 1 >= CMDLEN)
					{
						break;
					}
					else
					{
						line[ABBENVLEN-4] = '.';
						line[ABBENVLEN-3] = '.';
						line[ABBENVLEN-2] = '.';
						line[ABBENVLEN-1] = '\0';
						line[ABBENVLEN]   = '\0';
						nread = ABBENVLEN;
					}
				}

				env_len += nread;

				*(line+nread-1) = ' ';	// modify NULL byte to space

				strcpy(pc, line);
				pc += nread;
			}
		}

		/* line has been (re)allocated within the first call to getdelim */
                /* even if the call fails, see getline(3) */
		free(line);
		fclose(fpe);
	}

	// add command line and parameters
	//
	if ( (fp = fopen("cmdline", "r")) != NULL)
	{
		nr = fread(pc, 1, CMDLEN-env_len, fp);
		fclose(fp);

		if (nr > 0)	/* anything read? */
		{
			for (i=0; i < nr-1; i++, pc++)
			{
				switch (*pc)
				{
				   case '\0':
				   case '\n':
				   case '\t':
					*pc = ' ';
				}
			}
		}
		else
		{
			// nothing read (usually for kernel processes)
			// wipe prepended environment vars not to disturb
			// checks on an empty command line in other places
			//
			curtask->gen.cmdline[0] = '\0';
		}
	}
}


/*
** determine the wait channel of a sleeping thread
** i.e. the name of the kernel function in which the thread
** has been put in sleep state)
*/
static void
procwchan(struct tstat *curtask)
{
        FILE            *fp;
        register int    nr = 0;

        if ( (fp = fopen("wchan", "r")) != NULL)
        {

                nr = fread(curtask->cpu.wchan, 1,
			sizeof(curtask->cpu.wchan)-1, fp);
                if (nr < 0)
                        nr = 0;
                fclose(fp);
        }

        curtask->cpu.wchan[nr] = 0;
}


/*
** open file "smaps" and obtain required info
** since Linux-4.14, kernel supports "smaps_rollup" which has better
** performence. check "smaps_rollup" in first call
** if kernel supports "smaps_rollup", use "smaps_rollup" instead
*/
static void
procsmaps(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];
	count_t	pssval;
	static int procsmaps_firstcall = 1;
	static char *smapsfile = "smaps";

	if (procsmaps_firstcall)
	{
		regainrootprivs();
		if ( (fp = fopen("/proc/1/smaps_rollup", "r")) )
		{
			smapsfile = "smaps_rollup";
			fclose(fp);
		}

		procsmaps_firstcall = 0;
	}

	/*
 	** open the file (always succeeds, even if no root privs)
	*/
	regainrootprivs();

	if ( (fp = fopen(smapsfile, "r")) )
	{
		curtask->mem.pmem = 0;

		while (fgets(line, sizeof line, fp))
		{
			if (memcmp(line, "Pss:", 4) != 0)
				continue;

			// PSS line found to be accumulated
			sscanf(line, "Pss: %llu", &pssval);
			curtask->mem.pmem += pssval;
		}

		/*
		** verify if fgets returned NULL due to error i.s.o. EOF
		*/
		if (ferror(fp))
			curtask->mem.pmem = (unsigned long long)-1LL;

		fclose(fp);
	}
	else
	{
		curtask->mem.pmem = (unsigned long long)-1LL;
	}

	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");
}

/*
** get run_delay from /proc/<pid>/schedstat
** ref: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/scheduler/sched-stats.rst?h=v5.7-rc6
*/
static count_t
procschedstat(struct tstat *curtask)
{
	FILE	*fp;
	char	line[4096];
	count_t	runtime, rundelay = 0;
	unsigned long pcount;
	static char *schedstatfile = "schedstat";

	/*
 	** open the schedstat file
	*/
	if ( (fp = fopen(schedstatfile, "r")) )
	{
		curtask->cpu.rundelay = 0;

		if (fgets(line, sizeof line, fp))
		{
			sscanf(line, "%llu %llu %lu\n",
					&runtime, &rundelay, &pcount);

			curtask->cpu.rundelay = rundelay;
		}

		/*
		** verify if fgets returned NULL due to error i.s.o. EOF
		*/
		if (ferror(fp))
			curtask->cpu.rundelay = 0;

		fclose(fp);
	}
	else
	{
		curtask->cpu.rundelay = 0;
	}

	return curtask->cpu.rundelay;
}
