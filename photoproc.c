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
#define _POSIX_C_SOURCE
#define _XOPEN_SOURCE
#define _GNU_SOURCE
#define _DEFAULT_SOURCE

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
#include <sys/mman.h> // For mmap
#include <sys/stat.h> // For fstat
#include <fcntl.h>    // For open

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
	** if netatop kernel module is not active on this system,
	** try to connect to netatop-bpf 
	*/
	if (connectnetatop && !(supportflags & NETATOP)) {
		netatop_bpf_probe();
	}

	/*
	** if netatop-bpf is not active on this system,
	** try to connect to kernel module
	*/
	if (connectnetatop && !(supportflags & NETATOPBPF)) {
		netatop_probe();
	}

	/*
	** if netatop-bpf is active on this system, fetch data
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
			// read network stats from netatop (if active)
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
#define	TPTOLERANCE	2

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

	/*
	** In a normal situation the number of threads will be far more
	** than the number of processes since every process consists of
	** at least one thread (even zombie processes) while many processes
	** consist of multiple threads. Some malicious kernel versions in
	** the past did not provide correct information about the number of
	** threads for which a sanity check was added at this point.
	**
	** However, in the early boot phase (when the atop daemon is started)
	** only single-threaded processes might run. Since the gathering of
	** the number of threads and the number of processes is not one atomic
	** operation, the number of threads might be 1 or 2 less than the number
	** of processes. This should not lead to a preliminary termination.
	*/
	if (nrthread < nrproc)
	{
		if (nrproc - nrthread > TPTOLERANCE)
			mcleanstop(53, "#threads (%ld) < #procs (%ld)\n",
					nrthread, nrproc);

		nrthread = nrproc;	// correct number of threads
	}

	return nrproc + nrthread;
}

/*
** open file "stat" and obtain required info
*/
static int
procstat(struct tstat *curtask, unsigned long long bootepoch, char isproc)
{
	int	fd;
	struct stat st;
	char	*map;
	char	line[4096]; // Use a buffer for sscanf
	int	nr;
	char	*p, *cmdhead, *cmdtail;

	if ( (fd = open("stat", O_RDONLY)) == -1)
		return 0;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return 0;
	}

	if (st.st_size == 0) {
		close(fd);
		return 0;
	}

	map = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		return 0;
	}

	// Copy to a null-terminated buffer for sscanf and string operations
	nr = (st.st_size < sizeof(line) - 1) ? st.st_size : sizeof(line) - 1;
	memcpy(line, map, nr);
	line[nr] = '\0';

	munmap(map, st.st_size);
	close(fd);

	/*
    	** fetch command name
	*/
	cmdhead = strchr (line, '(');
	cmdtail = strrchr(line, ')');

	if (!cmdhead || !cmdtail || cmdtail < cmdhead) // parsing failed?
	{
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
		return 0;
	}

	/*
 	** normalization
	*/
	curtask->gen.btime   = (curtask->gen.btime+bootepoch)/hertz;
	curtask->cpu.prio   += 100; 	/* was subtracted by kernel */
	curtask->mem.vmem   /= 1024;
	curtask->mem.rmem   *= pagesize/1024;


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
	int	fd;
	struct stat st;
	char	*map;
	char	*p, *end;

	if ( (fd = open("status", O_RDONLY)) == -1)
		return 0;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return 0;
	}

	if (st.st_size == 0) {
		close(fd);
		return 0;
	}

	map = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		return 0;
	}

	close(fd); // File descriptor can be closed after mmap

	curtask->gen.nthr     = 1;	/* for compat with 2.4 */
	curtask->cpu.sleepavg = 0;	/* for compat with 2.4 */
	curtask->mem.vgrow    = 0;	/* calculated later */
	curtask->mem.rgrow    = 0;	/* calculated later */

	p = map;
	end = map + st.st_size;

	while (p < end)
	{
		char *line_end = strchr(p, '\n');
		size_t len;
		if (line_end == NULL) {
			len = end - p;
		} else {
			len = line_end - p;
		}

		// Create a temporary null-terminated string for sscanf
		char temp_line[256];
		if (len >= sizeof(temp_line)) {
			len = sizeof(temp_line) - 1;
		}
		memcpy(temp_line, p, len);
		temp_line[len] = '\0';

		if (memcmp(temp_line, "Tgid:", 5) ==0)
		{
			sscanf(temp_line, "Tgid: %d", &(curtask->gen.tgid));
		}
		else if (memcmp(temp_line, "Pid:", 4) ==0)
		{
			sscanf(temp_line, "Pid: %d", &(curtask->gen.pid));
		}
		else if (memcmp(temp_line, "SleepAVG:", 9)==0)
		{
			sscanf(temp_line, "SleepAVG: %d%%",
				&(curtask->cpu.sleepavg));
		}
		else if (memcmp(temp_line, "Uid:", 4)==0)
		{
			sscanf(temp_line, "Uid: %d %d %d %d",
				&(curtask->gen.ruid), &(curtask->gen.euid),
				&(curtask->gen.suid), &(curtask->gen.fsuid));
		}
		else if (memcmp(temp_line, "Gid:", 4)==0)
		{
			sscanf(temp_line, "Gid: %d %d %d %d",
				&(curtask->gen.rgid), &(curtask->gen.egid),
				&(curtask->gen.sgid), &(curtask->gen.fsgid));
		}
		else if (memcmp(temp_line, "envID:", 6) ==0)
		{
			sscanf(temp_line, "envID: %d", &(curtask->gen.ctid));
		}
		else if (memcmp(temp_line, "VPid:", 5) ==0)
		{
			sscanf(temp_line, "VPid: %d", &(curtask->gen.vpid));
		}
		else if (memcmp(temp_line, "Threads:", 8)==0)
		{
			sscanf(temp_line, "Threads: %d", &(curtask->gen.nthr));
		}
		else if (memcmp(temp_line, "VmData:", 7)==0)
		{
			sscanf(temp_line, "VmData: %lld", &(curtask->mem.vdata));
		}
		else if (memcmp(temp_line, "VmStk:", 6)==0)
		{
			sscanf(temp_line, "VmStk: %lld", &(curtask->mem.vstack));
		}
		else if (memcmp(temp_line, "VmExe:", 6)==0)
		{
			sscanf(temp_line, "VmExe: %lld", &(curtask->mem.vexec));
		}
		else if (memcmp(temp_line, "VmLib:", 6)==0)
		{
			sscanf(temp_line, "VmLib: %lld", &(curtask->mem.vlibs));
		}
		else if (memcmp(temp_line, "VmSwap:", 7)==0)
		{
			sscanf(temp_line, "VmSwap: %lld", &(curtask->mem.vswap));
		}
		else if (memcmp(temp_line, "VmLck:", 6)==0)
		{
			sscanf(temp_line, "VmLck: %lld", &(curtask->mem.vlock));
		}
		else if (memcmp(temp_line, "voluntary_ctxt_switches:", 24)==0)
		{
			sscanf(temp_line, "voluntary_ctxt_switches: %lld", &(curtask->cpu.nvcsw));
		}
		else if (memcmp(temp_line, "nonvoluntary_ctxt_switches:", 27)==0)
		{
			sscanf(temp_line, "nonvoluntary_ctxt_switches: %lld", &(curtask->cpu.nivcsw));
		}

		if (line_end == NULL) {
			break; // End of file
		}
		p = line_end + 1; // Move to the next line
	}

	munmap(map, st.st_size);
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
	int	fd;
	struct stat st;
	char	*map;
	char	*p, *end;
	count_t	dskrsz=0, dskwsz=0, dskcwsz=0;

	if (supportflags & IOSTAT)
	{
		regainrootprivs();

		if ( (fd = open("io", O_RDONLY)) != -1)
		{
			if (fstat(fd, &st) == -1) {
				close(fd);
				droprootprivs();
				return 0;
			}

			if (st.st_size == 0) {
				close(fd);
				droprootprivs();
				return 0;
			}

			map = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
			if (map == MAP_FAILED) {
				close(fd);
				droprootprivs();
				return 0;
			}
			close(fd); // File descriptor can be closed after mmap

			p = map;
			end = map + st.st_size;

			while (p < end)
			{
				char *line_end = strchr(p, '\n');
				size_t len;
				if (line_end == NULL) {
					len = end - p;
				} else {
					len = line_end - p;
				}

				char temp_line[256];
				if (len >= sizeof(temp_line)) {
					len = sizeof(temp_line) - 1;
				}
				memcpy(temp_line, p, len);
				temp_line[len] = '\0';

				if (memcmp(temp_line, IO_READ,
						sizeof IO_READ -1) == 0)
				{
					sscanf(temp_line, "%*s %llu", &dskrsz);
					dskrsz /= 512;		// in sectors
				}
				else if (memcmp(temp_line, IO_WRITE,
						sizeof IO_WRITE -1) == 0)
				{
					sscanf(temp_line, "%*s %llu", &dskwsz);
					dskwsz /= 512;		// in sectors
				}
				else if (memcmp(temp_line, IO_CWRITE,
						sizeof IO_CWRITE -1) == 0)
				{
					sscanf(temp_line, "%*s %llu", &dskcwsz);
					dskcwsz /= 512;		// in sectors
				}

				if (line_end == NULL) {
					break;
				}
				p = line_end + 1;
			}

			munmap(map, st.st_size);

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
	int		fd_cmd, fd_env;
	struct stat	st_cmd, st_env;
	char		*map_cmd = NULL, *map_env = NULL;
	register int 	i;
	ssize_t		env_len = 0;
	register char	*pc = curtask->gen.cmdline;

	memset(curtask->gen.cmdline, 0, CMDLEN+1); // initialize command line

	// prepend by environment variables (if required)
	//
	if ( prependenv && (fd_env = open("environ", O_RDONLY)) != -1)
	{
		if (fstat(fd_env, &st_env) != -1 && st_env.st_size > 0) {
			map_env = mmap(0, st_env.st_size, PROT_READ, MAP_PRIVATE, fd_env, 0);
			if (map_env != MAP_FAILED) {
				char *line = map_env;
				char *end = map_env + st_env.st_size;
				while (line < end)
				{
					size_t nread = 0;
					char *null_byte = memchr(line, '\0', end - line);
					if (null_byte) {
						nread = null_byte - line + 1;
					} else {
						nread = end - line;
					}

					if (nread > 0 && !regexec(&envregex, line, 0, NULL, 0))
					{
						if (env_len + nread >= CMDLEN)
						{
							if (env_len + ABBENVLEN + 1 >= CMDLEN)
							{
								break;
							}
							else
							{
								// Create a temporary buffer for abbreviated string
								char abbrev_line[ABBENVLEN + 1];
								size_t copy_len = (nread - 1 < ABBENVLEN - 4) ? (nread - 1) : (ABBENVLEN - 4);
								memcpy(abbrev_line, line, copy_len);
								abbrev_line[copy_len] = '.';
								abbrev_line[copy_len + 1] = '.';
								abbrev_line[copy_len + 2] = '.';
								abbrev_line[copy_len + 3] = '\0';
								abbrev_line[ABBENVLEN] = '\0'; // Ensure null termination
								nread = ABBENVLEN; // Update nread to new length

								if (env_len + nread < CMDLEN) {
									strcpy(pc, abbrev_line);
									pc += nread;
									env_len += nread;
								} else {
									break;
								}
							}
						} else {
							// Copy the environment variable, replacing null with space
							size_t copy_len = nread - 1;
							if (env_len + copy_len + 1 < CMDLEN) { // +1 for space
								memcpy(pc, line, copy_len);
								pc[copy_len] = ' ';
								pc += copy_len + 1;
								env_len += copy_len + 1;
							} else {
								break;
							}
						}
					}
					line += nread;
				}
				munmap(map_env, st_env.st_size);
			}
		}
		close(fd_env);
	}

	// add command line and parameters
	//
	if ( (fd_cmd = open("cmdline", O_RDONLY)) != -1)
	{
		if (fstat(fd_cmd, &st_cmd) != -1 && st_cmd.st_size > 0) {
			map_cmd = mmap(0, st_cmd.st_size, PROT_READ, MAP_PRIVATE, fd_cmd, 0);
			if (map_cmd != MAP_FAILED) {
				int nr_read = (st_cmd.st_size < CMDLEN - env_len) ? st_cmd.st_size : (CMDLEN - env_len);
				memcpy(pc, map_cmd, nr_read);
				munmap(map_cmd, st_cmd.st_size);

				if (nr_read > 0)	/* anything read? */
				{
					for (i=0; i < nr_read; i++, pc++)
					{
						switch (*pc)
						{
						   case '\0':
						   case '\n':
						   case '\r':
						   case '\t':
							*pc = ' ';
						}
					}
					*pc = '\0'; // Null-terminate the command line
				}
				else
				{
					// nothing read (usually for kernel processes)
					// wipe prepended environment vars not to disturb
					// checks on an empty command line in other places
					//
					curtask->gen.cmdline[0] = '\0';
				}
			} else {
				curtask->gen.cmdline[0] = '\0';
			}
		} else {
			curtask->gen.cmdline[0] = '\0';
		}
		close(fd_cmd);
	} else {
		curtask->gen.cmdline[0] = '\0';
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
        int            fd;
        struct stat    st;
        char           *map;
        register int   nr = 0;

        if ( (fd = open("wchan", O_RDONLY)) != -1)
        {
            if (fstat(fd, &st) != -1 && st.st_size > 0) {
                map = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
                if (map != MAP_FAILED) {
                    nr = (st.st_size < sizeof(curtask->cpu.wchan)-1) ? st.st_size : sizeof(curtask->cpu.wchan)-1;
                    memcpy(curtask->cpu.wchan, map, nr);
                    munmap(map, st.st_size);
                }
            }
            close(fd);
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
	int	fd;
	struct stat st;
	char	*map;
	char	*p, *end;
	count_t	pssval;
	static int procsmaps_firstcall = 1;
	static char *smapsfile = "smaps";

	if (procsmaps_firstcall)
	{
		regainrootprivs();
		if ( (fd = open("/proc/1/smaps_rollup", O_RDONLY)) != -1)
		{
			smapsfile = "smaps_rollup";
			close(fd);
		}

		procsmaps_firstcall = 0;
	}

	/*
 	** open the file (always succeeds, even if no root privs)
	*/
	regainrootprivs();

	if ( (fd = open(smapsfile, O_RDONLY)) != -1)
	{
		if (fstat(fd, &st) == -1) {
			close(fd);
			droprootprivs();
			return;
		}

		if (st.st_size == 0) {
			close(fd);
			droprootprivs();
			return;
		}

		map = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (map == MAP_FAILED) {
			close(fd);
			droprootprivs();
			return;
		}
		close(fd); // File descriptor can be closed after mmap

		curtask->mem.pmem = 0;
		p = map;
		end = map + st.st_size;

		while (p < end)
		{
			char *line_end = strchr(p, '\n');
			size_t len;
			if (line_end == NULL) {
				len = end - p;
			} else {
				len = line_end - p;
			}

			char temp_line[256];
			if (len >= sizeof(temp_line)) {
				len = sizeof(temp_line) - 1;
			}
			memcpy(temp_line, p, len);
			temp_line[len] = '\0';

			if (memcmp(temp_line, "Pss:", 4) == 0)
			{
				// PSS line found to be accumulated
				sscanf(temp_line, "Pss: %llu", &pssval);
				curtask->mem.pmem += pssval;
			}

			if (line_end == NULL) {
				break;
			}
			p = line_end + 1;
		}

		munmap(map, st.st_size);
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
	int	fd;
	struct stat st;
	char	*map;
	char	temp_line[256]; // Buffer for sscanf
	count_t	runtime, rundelay = 0;
	unsigned long pcount;
	static char *schedstatfile = "schedstat";

	/*
 	** open the schedstat file
	*/
	if ( (fd = open(schedstatfile, O_RDONLY)) != -1)
	{
		if (fstat(fd, &st) == -1) {
			close(fd);
			curtask->cpu.rundelay = 0;
			return 0;
		}

		if (st.st_size == 0) {
			close(fd);
			curtask->cpu.rundelay = 0;
			return 0;
		}

		map = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		if (map == MAP_FAILED) {
			close(fd);
			curtask->cpu.rundelay = 0;
			return 0;
		}
		close(fd); // File descriptor can be closed after mmap

		curtask->cpu.rundelay = 0;

		// Copy the first line to a null-terminated buffer
		char *line_end = strchr(map, '\n');
		size_t len;
		if (line_end == NULL) {
			len = st.st_size;
		} else {
			len = line_end - map;
		}

		if (len >= sizeof(temp_line)) {
			len = sizeof(temp_line) - 1;
		}
		memcpy(temp_line, map, len);
		temp_line[len] = '\0';

		if (sscanf(temp_line, "%llu %llu %lu",
				&runtime, &rundelay, &pcount) == 3)
		{
			curtask->cpu.rundelay = rundelay;
		}

		munmap(map, st.st_size);
	}
	else
	{
		curtask->cpu.rundelay = 0;
	}

	return curtask->cpu.rundelay;
}
