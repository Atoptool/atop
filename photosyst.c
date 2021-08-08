/*
** ATOP - System & Process Monitor 
** 
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
** 
** This source-file contains functions to read all relevant system-level
** figures.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
** --------------------------------------------------------------------------
** Copyright (C) 2000-2012 Gerlof Langeveld
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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <limits.h>

#define SCALINGMAXCPU	8	// threshold for scaling info per CPU


#ifndef	NOPERFEVENT
#include <linux/perf_event.h>
#include <asm/unistd.h>
#endif

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// #define	_GNU_SOURCE
#include <sys/ipc.h>
#include <sys/shm.h>

#include "atop.h"
#include "photosyst.h"

#define	MAXCNT	64

/* return value of isdisk() */
#define	NONTYPE	0
#define	DSKTYPE	1
#define	MDDTYPE	2
#define	LVMTYPE	3

/* hypervisor enum */
enum {
	HYPER_NONE	= 0,
	HYPER_XEN,
	HYPER_KVM,
	HYPER_MSHV,
	HYPER_VMWARE,
	HYPER_IBM,
	HYPER_VSERVER,
	HYPER_UML,
	HYPER_INNOTEK,
	HYPER_HITACHI,
	HYPER_PARALLELS,
	HYPER_VBOX,
	HYPER_OS400,
	HYPER_PHYP,
	HYPER_SPAR,
	HYPER_WSL,
};

#ifndef	NOPERFEVENT
enum {
	PERF_EVENTS_AUTO = 0,
	PERF_EVENTS_ENABLE,
	PERF_EVENTS_DISABLE,
};

static int	perfevents = PERF_EVENTS_AUTO;
static void	getperfevents(struct cpustat *);

static int	run_in_guest(void);
static int	get_hypervisor(void);
#endif

static int	get_infiniband(struct ifbstat *);
static int	get_ksm(struct sstat *);
static int	get_zswap(struct sstat *);

static int	isdisk(unsigned int, unsigned int,
			char *, struct perdsk *, int);

static struct ipv6_stats	ipv6_tmp;
static struct icmpv6_stats	icmpv6_tmp;
static struct udpv6_stats	udpv6_tmp;

struct v6tab {
	char 	*nam;
	count_t *val;
};

static struct v6tab 		v6tab[] = {
    {"Ip6InReceives",		&ipv6_tmp.Ip6InReceives,                     },
    {"Ip6InHdrErrors",		&ipv6_tmp.Ip6InHdrErrors,                    },
    {"Ip6InTooBigErrors",	&ipv6_tmp.Ip6InTooBigErrors,                 },
    {"Ip6InNoRoutes",		&ipv6_tmp.Ip6InNoRoutes,                     },
    {"Ip6InAddrErrors",		&ipv6_tmp.Ip6InAddrErrors,                   },
    {"Ip6InUnknownProtos",	&ipv6_tmp.Ip6InUnknownProtos,                },
    {"Ip6InTruncatedPkts",	&ipv6_tmp.Ip6InTruncatedPkts,                },
    {"Ip6InDiscards",		&ipv6_tmp.Ip6InDiscards,                     },
    {"Ip6InDelivers",		&ipv6_tmp.Ip6InDelivers,                     },
    {"Ip6OutForwDatagrams",	&ipv6_tmp.Ip6OutForwDatagrams,               },
    {"Ip6OutRequests",		&ipv6_tmp.Ip6OutRequests,                    },
    {"Ip6OutDiscards",		&ipv6_tmp.Ip6OutDiscards,                    },
    {"Ip6OutNoRoutes",		&ipv6_tmp.Ip6OutNoRoutes,                    },
    {"Ip6ReasmTimeout",		&ipv6_tmp.Ip6ReasmTimeout,                   },
    {"Ip6ReasmReqds",		&ipv6_tmp.Ip6ReasmReqds,                     },
    {"Ip6ReasmOKs",		&ipv6_tmp.Ip6ReasmOKs,                       },
    {"Ip6ReasmFails",		&ipv6_tmp.Ip6ReasmFails,                     },
    {"Ip6FragOKs",		&ipv6_tmp.Ip6FragOKs,                        },
    {"Ip6FragFails",		&ipv6_tmp.Ip6FragFails,                      },
    {"Ip6FragCreates",		&ipv6_tmp.Ip6FragCreates,                    },
    {"Ip6InMcastPkts",		&ipv6_tmp.Ip6InMcastPkts,                    },
    {"Ip6OutMcastPkts",		&ipv6_tmp.Ip6OutMcastPkts,                   },
 
    {"Icmp6InMsgs",		&icmpv6_tmp.Icmp6InMsgs,                     },
    {"Icmp6InErrors",		&icmpv6_tmp.Icmp6InErrors,                   },
    {"Icmp6InDestUnreachs",	&icmpv6_tmp.Icmp6InDestUnreachs,             },
    {"Icmp6InPktTooBigs",	&icmpv6_tmp.Icmp6InPktTooBigs,               },
    {"Icmp6InTimeExcds",	&icmpv6_tmp.Icmp6InTimeExcds,                },
    {"Icmp6InParmProblems",	&icmpv6_tmp.Icmp6InParmProblems,             },
    {"Icmp6InEchos",		&icmpv6_tmp.Icmp6InEchos,                    },
    {"Icmp6InEchoReplies",	&icmpv6_tmp.Icmp6InEchoReplies,              },
    {"Icmp6InGroupMembQueries",	&icmpv6_tmp.Icmp6InGroupMembQueries,         },
    {"Icmp6InGroupMembResponses",
				&icmpv6_tmp.Icmp6InGroupMembResponses,       },
    {"Icmp6InGroupMembReductions",
				&icmpv6_tmp.Icmp6InGroupMembReductions,      },
    {"Icmp6InRouterSolicits",	&icmpv6_tmp.Icmp6InRouterSolicits,           },
    {"Icmp6InRouterAdvertisements",
				&icmpv6_tmp.Icmp6InRouterAdvertisements,     },
    {"Icmp6InNeighborSolicits",	&icmpv6_tmp.Icmp6InNeighborSolicits,         },
    {"Icmp6InNeighborAdvertisements",
				&icmpv6_tmp.Icmp6InNeighborAdvertisements,   },
    {"Icmp6InRedirects",	&icmpv6_tmp.Icmp6InRedirects,                },
    {"Icmp6OutMsgs",		&icmpv6_tmp.Icmp6OutMsgs,                    },
    {"Icmp6OutDestUnreachs",	&icmpv6_tmp.Icmp6OutDestUnreachs,            },
    {"Icmp6OutPktTooBigs",	&icmpv6_tmp.Icmp6OutPktTooBigs,              },
    {"Icmp6OutTimeExcds",	&icmpv6_tmp.Icmp6OutTimeExcds,               },
    {"Icmp6OutParmProblems",	&icmpv6_tmp.Icmp6OutParmProblems,            },
    {"Icmp6OutEchoReplies",	&icmpv6_tmp.Icmp6OutEchoReplies,             },
    {"Icmp6OutRouterSolicits",	&icmpv6_tmp.Icmp6OutRouterSolicits,          },
    {"Icmp6OutNeighborSolicits",&icmpv6_tmp.Icmp6OutNeighborSolicits,        },
    {"Icmp6OutNeighborAdvertisements",
				&icmpv6_tmp.Icmp6OutNeighborAdvertisements,  },
    {"Icmp6OutRedirects",	&icmpv6_tmp.Icmp6OutRedirects,               },
    {"Icmp6OutGroupMembResponses",
				&icmpv6_tmp.Icmp6OutGroupMembResponses,      },
    {"Icmp6OutGroupMembReductions",
				&icmpv6_tmp.Icmp6OutGroupMembReductions,     },

    {"Udp6InDatagrams",		&udpv6_tmp.Udp6InDatagrams,                   },
    {"Udp6NoPorts",		&udpv6_tmp.Udp6NoPorts,                       },
    {"Udp6InErrors",		&udpv6_tmp.Udp6InErrors,                      },
    {"Udp6OutDatagrams",	&udpv6_tmp.Udp6OutDatagrams,                  },
};

static int	v6tab_entries = sizeof(v6tab)/sizeof(struct v6tab);

void
photosyst(struct sstat *si)
{
	static char	part_stats = 1; /* per-partition statistics ? */
	static char	ib_stats = 1; 	/* InfiniBand statistics ? */
	static char	ksm_stats = 1; 	
	static char	zswap_stats = 1;

	register int	i, nr;
	count_t		cnts[MAXCNT];
	float		lavg1, lavg5, lavg15;
	FILE 		*fp;
	char		linebuf[1024], nam[64], origdir[1024];
	unsigned int	major, minor;
	struct shm_info	shminfo;
#if	HTTPSTATS
	static int	wwwvalid = 1;
#endif

	memset(si, 0, sizeof(struct sstat));

	if ( getcwd(origdir, sizeof origdir) == NULL)
		mcleanstop(54, "failed to save current dir\n");

	if ( chdir("/proc") == -1)
		mcleanstop(54, "failed to change to /proc\n");

	/*
	** gather various general statistics from the file /proc/stat and
	** store them in binary form
	*/
	if ( (fp = fopen("stat", "r")) != NULL)
	{
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			nr = sscanf(linebuf,
			            "%s   %lld %lld %lld %lld %lld %lld %lld "
			            "%lld %lld %lld %lld %lld %lld %lld %lld ",
			  	nam,
			  	&cnts[0],  &cnts[1],  &cnts[2],  &cnts[3],
			  	&cnts[4],  &cnts[5],  &cnts[6],  &cnts[7],
			  	&cnts[8],  &cnts[9],  &cnts[10], &cnts[11],
			  	&cnts[12], &cnts[13], &cnts[14]);

			if (nr < 2)		/* headerline ? --> skip */
				continue;

			if ( strcmp("cpu", nam) == EQ)
			{
				si->cpu.all.utime	= cnts[0];
				si->cpu.all.ntime	= cnts[1];
				si->cpu.all.stime	= cnts[2];
				si->cpu.all.itime	= cnts[3];

				if (nr > 5)	/* 2.6 kernel? */
				{
					si->cpu.all.wtime	= cnts[4];
					si->cpu.all.Itime	= cnts[5];
					si->cpu.all.Stime	= cnts[6];

					if (nr > 8)	/* steal support */
						si->cpu.all.steal = cnts[7];

					if (nr > 9)	/* guest support */
						si->cpu.all.guest = cnts[8];
				}
				continue;
			}

			if ( strncmp("cpu", nam, 3) == EQ)
			{
				i = atoi(&nam[3]);

				if (i >= MAXCPU)
				{
					fprintf(stderr,
						"cpu %s exceeds maximum of %d\n",
						nam, MAXCPU);
					continue;
				}

				si->cpu.cpu[i].cpunr	= i;
				si->cpu.cpu[i].utime	= cnts[0];
				si->cpu.cpu[i].ntime	= cnts[1];
				si->cpu.cpu[i].stime	= cnts[2];
				si->cpu.cpu[i].itime	= cnts[3];

				if (nr > 5)	/* 2.6 kernel? */
				{
					si->cpu.cpu[i].wtime	= cnts[4];
					si->cpu.cpu[i].Itime	= cnts[5];
					si->cpu.cpu[i].Stime	= cnts[6];

					if (nr > 8)	/* steal support */
						si->cpu.cpu[i].steal = cnts[7];

					if (nr > 9)	/* guest support */
						si->cpu.cpu[i].guest = cnts[8];
				}

				si->cpu.nrcpu++;
				continue;
			}

			if ( strcmp("ctxt", nam) == EQ)
			{
				si->cpu.csw	= cnts[0];
				continue;
			}

			if ( strcmp("intr", nam) == EQ)
			{
				si->cpu.devint	= cnts[0];
				continue;
			}

			if ( strcmp("processes", nam) == EQ)
			{
				si->cpu.nprocs	= cnts[0];
				continue;
			}

			if ( strcmp("swap", nam) == EQ)   /* < 2.6 */
			{
				si->mem.swins	= cnts[0];
				si->mem.swouts	= cnts[1];
				continue;
			}
		}

		fclose(fp);

		if (si->cpu.nrcpu == 0)
			si->cpu.nrcpu = 1;
	}

	/*
	** gather loadaverage values from the file /proc/loadavg and
	** store them in binary form
	*/
	if ( (fp = fopen("loadavg", "r")) != NULL)
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if ( sscanf(linebuf, "%f %f %f",
				&lavg1, &lavg5, &lavg15) == 3)
			{
				si->cpu.lavg1	= lavg1;
				si->cpu.lavg5	= lavg5;
				si->cpu.lavg15	= lavg15;
			}
		}

		fclose(fp);
	}

	/*
	** gather frequency scaling info.
        ** sources (in order of preference): 
        **     /sys/devices/system/cpu/cpu0/cpufreq/stats/time_in_state
        ** or
        **     /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
        **     /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
        **
	** store them in binary form
	*/
        static char fn[256];
        int didone=0;

	if (si->cpu.nrcpu <= SCALINGMAXCPU)
	{
            for (i = 0; i < si->cpu.nrcpu; ++i)
            {
                long long f=0;

                sprintf(fn,
                   "/sys/devices/system/cpu/cpu%d/cpufreq/stats/time_in_state",
                   i);

                if ((fp=fopen(fn, "r")) != 0)
                {
                        long long hits=0;
                        long long maxfreq=0;
                        long long cnt=0;
                        long long sum=0;

                        while (fscanf(fp, "%lld %lld", &f, &cnt) == 2)
                        {
                                f	/= 1000;
                                sum 	+= (f*cnt);
                                hits	+= cnt;

                                if (f > maxfreq)
                        		maxfreq=f;
                        	didone=1;
                        }

	                si->cpu.cpu[i].freqcnt.maxfreq	= maxfreq;
	                si->cpu.cpu[i].freqcnt.cnt	= sum;
	                si->cpu.cpu[i].freqcnt.ticks	= hits;

                        fclose(fp);
                }
		else
		{    // governor statistics not available
                     sprintf(fn,  
                      "/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq",
		      i);

                        if ((fp=fopen(fn, "r")) != 0)
                        {
                                if (fscanf(fp, "%lld", &f) == 1)
                                {
  					// convert KHz to MHz
	                                si->cpu.cpu[i].freqcnt.maxfreq =f/1000; 
                                }

                                fclose(fp);
                        }
                        else 
                        {
	                        si->cpu.cpu[i].freqcnt.maxfreq=0;
                        }

                       sprintf(fn,  
                       "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
		       i);
        
                        if ((fp=fopen(fn, "r")) != 0)
                        {
                                if (fscanf(fp, "%lld", &f) == 1)
                                {
   					// convert KHz to MHz
                                        si->cpu.cpu[i].freqcnt.cnt   = f/1000;
                                        si->cpu.cpu[i].freqcnt.ticks = 0;
                                        didone=1;
                                }

                                fclose(fp);
                        }
                        else
                        {
                                si->cpu.cpu[i].freqcnt.cnt	= 0;
                                si->cpu.cpu[i].freqcnt.ticks 	= 0;
                        }
                }
            } // for all CPUs
	}

        if (!didone)     // did not get processor freq statistics.
                         // use /proc/cpuinfo
        {
	        if ( (fp = fopen("cpuinfo", "r")) != NULL)
                {
                        // get information from the lines
                        // processor\t: 0
                        // cpu MHz\t\t: 800.000
                        
                        int cpuno=-1;

		        while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
                        {
                                if (memcmp(linebuf, "processor", 9)== EQ)
					sscanf(linebuf, "%*s %*s %d", &cpuno);

                                if (memcmp(linebuf, "cpu MHz", 7) == EQ)
				{
                                        if (cpuno >= 0 && cpuno < si->cpu.nrcpu)
					{
						sscanf(linebuf,
							"%*s %*s %*s %lld",
                                	     		&(si->cpu.cpu[cpuno].freqcnt.cnt));
					}
                                }
                        }

			fclose(fp);
                }

        }

	/*
	** gather virtual memory statistics from the file /proc/vmstat and
	** store them in binary form (>= kernel 2.6)
	*/
	si->mem.oomkills     = -1;

	si->mem.allocstall   = 0;
	si->mem.numamigrate  = 0;
	si->mem.pgmigrate    = 0;

	if ( (fp = fopen("vmstat", "r")) != NULL)
	{
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			nr = sscanf(linebuf, "%s %lld", nam, &cnts[0]);

			if (nr < 2)		/* headerline ? --> skip */
				continue;

			if ( strcmp("pswpin", nam) == EQ)
			{
				si->mem.swins   = cnts[0];
				continue;
			}

			if ( strcmp("pswpout", nam) == EQ)
			{
				si->mem.swouts  = cnts[0];
				continue;
			}

			if ( strncmp("pgscan_", nam, 7) == EQ)
			{
				si->mem.pgscans += cnts[0];
				continue;
			}

			if ( strncmp("pgsteal_", nam, 8) == EQ)
			{
				si->mem.pgsteal += cnts[0];
				continue;
			}

			// more counters might start with "allocstall"
			if ( memcmp("allocstall", nam, 10) == EQ)
			{
				si->mem.allocstall += cnts[0];
				continue;
			}

			if ( strcmp("oom_kill", nam) == EQ)
			{
				si->mem.oomkills = cnts[0];
				continue;
			}
			if ( strcmp("compact_stall", nam) == EQ) {
				si->mem.compactstall = cnts[0];
				continue;
			}

			if ( strcmp("numa_pages_migrated", nam) == EQ) {
				si->mem.numamigrate = cnts[0];
				continue;
			}

			if ( strcmp("pgmigrate_success", nam) == EQ) {
				si->mem.pgmigrate = cnts[0];
				continue;
			}
		}

		fclose(fp);
	}

	/*
	** gather memory-related statistics from the file /proc/meminfo and
	** store them in binary form
	**
	** in the file /proc/meminfo a 2.4 kernel starts with two lines
	** headed by the strings "Mem:" and "Swap:" containing all required
	** fields, except proper value for page cache
        ** if these lines are present we try to skip parsing the rest
	** of the lines; if these lines are not present we should get the
	** required field from other lines
	*/
	si->mem.physmem	 	= (count_t)-1; 
	si->mem.freemem		= (count_t)-1;
	si->mem.buffermem	= (count_t)-1;
	si->mem.cachemem  	= (count_t)-1;
	si->mem.slabmem		= (count_t) 0;
	si->mem.slabreclaim	= (count_t) 0;
	si->mem.shmem 		= (count_t) 0;
	si->mem.totswap  	= (count_t)-1;
	si->mem.freeswap 	= (count_t)-1;
	si->mem.swapcached 	= (count_t) 0;
	si->mem.committed 	= (count_t) 0;

	if ( (fp = fopen("meminfo", "r")) != NULL)
	{
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			nr = sscanf(linebuf,
				"%s %lld %lld %lld %lld %lld %lld %lld "
			        "%lld %lld %lld\n",
				nam,
			  	&cnts[0],  &cnts[1],  &cnts[2],  &cnts[3],
			  	&cnts[4],  &cnts[5],  &cnts[6],  &cnts[7],
			  	&cnts[8],  &cnts[9]);

			if (nr < 2)		/* headerline ? --> skip */
				continue;

			if ( strcmp("Mem:", nam) == EQ)
			{
				si->mem.physmem	 	= cnts[0] / pagesize; 
				si->mem.freemem		= cnts[2] / pagesize;
				si->mem.buffermem	= cnts[4] / pagesize;
			}
			else	if ( strcmp("Swap:", nam) == EQ)
				{
					si->mem.totswap  = cnts[0] / pagesize;
					si->mem.freeswap = cnts[2] / pagesize;
				}
			else	if (strcmp("Cached:", nam) == EQ)
				{
					if (si->mem.cachemem  == (count_t)-1)
					{
						si->mem.cachemem  =
							cnts[0]*1024/pagesize;
					}
				}
			else	if (strcmp("Dirty:", nam) == EQ)
				{
					si->mem.cachedrt  =
							cnts[0]*1024/pagesize;
				}
			else	if (strcmp("MemTotal:", nam) == EQ)
				{
					if (si->mem.physmem  == (count_t)-1)
					{
						si->mem.physmem  =
							cnts[0]*1024/pagesize;
					}
				}
			else	if (strcmp("MemFree:", nam) == EQ)
				{
					if (si->mem.freemem  == (count_t)-1)
					{
						si->mem.freemem  =
							cnts[0]*1024/pagesize;
					}
				}
			else	if (strcmp("Buffers:", nam) == EQ)
				{
					if (si->mem.buffermem  == (count_t)-1)
					{
						si->mem.buffermem  =
							cnts[0]*1024/pagesize;
					}
				}
			else	if (strcmp("Shmem:", nam) == EQ)
				{
					si->mem.shmem = cnts[0]*1024/pagesize;
				}
			else	if (strcmp("SwapTotal:", nam) == EQ)
				{
					if (si->mem.totswap  == (count_t)-1)
					{
						si->mem.totswap  =
							cnts[0]*1024/pagesize;
					}
				}
			else	if (strcmp("SwapFree:", nam) == EQ)
				{
					if (si->mem.freeswap  == (count_t)-1)
					{
						si->mem.freeswap  =
							cnts[0]*1024/pagesize;
					}
				}
			else	if (strcmp("SwapCached:", nam) == EQ)
				{
					si->mem.swapcached  =
							cnts[0]*1024/pagesize;
				}
			else	if (strcmp("Slab:", nam) == EQ)
				{
					si->mem.slabmem = cnts[0]*1024/pagesize;
				}
			else	if (strcmp("SReclaimable:", nam) == EQ)
				{
					si->mem.slabreclaim = cnts[0]*1024/
								pagesize;
				}
			else	if (strcmp("Committed_AS:", nam) == EQ)
				{
					si->mem.committed = cnts[0]*1024/
								pagesize;
				}
			else	if (strcmp("CommitLimit:", nam) == EQ)
				{
					si->mem.commitlim = cnts[0]*1024/
								pagesize;
				}
			else	if (strcmp("HugePages_Total:", nam) == EQ)
				{
					si->mem.tothugepage = cnts[0];
				}
			else	if (strcmp("HugePages_Free:", nam) == EQ)
				{
					si->mem.freehugepage = cnts[0];
				}
			else	if (strcmp("Hugepagesize:", nam) == EQ)
				{
					si->mem.hugepagesz = cnts[0]*1024;
				}
		}

		fclose(fp);
	}

	/*
	** gather vmware-related statistics from /proc/vmmemctl
	** (only present if balloon driver enabled)
	*/ 
	si->mem.vmwballoon = (count_t) -1;

	if ( (fp = fopen("vmmemctl", "r")) != NULL)
	{
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			nr = sscanf(linebuf, "%s %lld ", nam, &cnts[0]);

			if ( strcmp("current:", nam) == EQ)
			{
				si->mem.vmwballoon = cnts[0];
				break;
			}
		}

		fclose(fp);
	}

	/*
	** ZFSonlinux: gather size of ARC cache in memory
	** searching for:
	** 	size       4    519101312
	*/ 
	si->mem.zfsarcsize = (count_t) -1;

	if ( (fp = fopen("spl/kstat/zfs/arcstats", "r")) != NULL)
	{
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			nr = sscanf(linebuf,
				"%s %lld %lld", nam, &cnts[0], &cnts[1]);

			if (nr < 3)
				continue;

			if ( strcmp("size", nam) == EQ)
			{
				si->mem.zfsarcsize = cnts[1] / pagesize;
				break;
			}
		}

		fclose(fp);
	}

	/*
	** gather network-related statistics
 	** 	- interface stats from the file /proc/net/dev
 	** 	- IPv4      stats from the file /proc/net/snmp
 	** 	- IPv6      stats from the file /proc/net/snmp6
	*/

	/*
	** interface statistics
	*/
	if ( (fp = fopen("net/dev", "r")) != NULL)
	{
		char *cp;

		i = 0;

		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if ( (cp = strchr(linebuf, ':')) != NULL)
				*cp = ' ';      /* substitute ':' by space */

			nr = sscanf(linebuf,
                                    "%15s %lld %lld %lld %lld %lld %lld %lld "
                                    "%lld %lld %lld %lld %lld %lld %lld %lld "
                                    "%lld\n",
				  si->intf.intf[i].name,
				&(si->intf.intf[i].rbyte),
				&(si->intf.intf[i].rpack),
				&(si->intf.intf[i].rerrs),
				&(si->intf.intf[i].rdrop),
				&(si->intf.intf[i].rfifo),
				&(si->intf.intf[i].rframe),
				&(si->intf.intf[i].rcompr),
				&(si->intf.intf[i].rmultic),
				&(si->intf.intf[i].sbyte),
				&(si->intf.intf[i].spack),
				&(si->intf.intf[i].serrs),
				&(si->intf.intf[i].sdrop),
				&(si->intf.intf[i].sfifo),
				&(si->intf.intf[i].scollis),
				&(si->intf.intf[i].scarrier),
				&(si->intf.intf[i].scompr));

			if (nr == 17)	/* skip header & lines without stats */
			{
				if (++i >= MAXINTF-1)
					break;
			}
		}

		si->intf.intf[i].name[0] = '\0'; /* set terminator for table */
		si->intf.nrintf = i;

		fclose(fp);
	}

	/*
	** IP version 4 statistics
	*/
	if ( (fp = fopen("net/snmp", "r")) != NULL)
	{
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			nr = sscanf(linebuf,
			 "%s   %lld %lld %lld %lld %lld %lld %lld %lld %lld "
			 "%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld "
			 "%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld "
			 "%lld %lld %lld %lld %lld %lld %lld %lld %lld %lld "
			 "%lld\n",
				nam,
				&cnts[0],  &cnts[1],  &cnts[2],  &cnts[3],
				&cnts[4],  &cnts[5],  &cnts[6],  &cnts[7],
				&cnts[8],  &cnts[9],  &cnts[10], &cnts[11],
				&cnts[12], &cnts[13], &cnts[14], &cnts[15],
				&cnts[16], &cnts[17], &cnts[18], &cnts[19],
				&cnts[20], &cnts[21], &cnts[22], &cnts[23],
				&cnts[24], &cnts[25], &cnts[26], &cnts[27],
				&cnts[28], &cnts[29], &cnts[30], &cnts[31],
				&cnts[32], &cnts[33], &cnts[34], &cnts[35],
				&cnts[36], &cnts[37], &cnts[38], &cnts[39]);

			if (nr < 2)		/* headerline ? --> skip */
				continue;

			if ( strcmp("Ip:", nam) == 0)
			{
				memcpy(&si->net.ipv4, cnts,
						sizeof si->net.ipv4);
				continue;
			}
	
			if ( strcmp("Icmp:", nam) == 0)
			{
				memcpy(&si->net.icmpv4, cnts,
						sizeof si->net.icmpv4);
				continue;
			}
	
			if ( strcmp("Tcp:", nam) == 0)
			{
				memcpy(&si->net.tcp, cnts,
						sizeof si->net.tcp);
				continue;
			}
	
			if ( strcmp("Udp:", nam) == 0)
			{
				memcpy(&si->net.udpv4, cnts,
						sizeof si->net.udpv4);
				continue;
			}
		}
	
		fclose(fp);
	}

	/*
	** IP version 6 statistics
	*/
	memset(&ipv6_tmp,   0, sizeof ipv6_tmp);
	memset(&icmpv6_tmp, 0, sizeof icmpv6_tmp);
	memset(&udpv6_tmp,  0, sizeof udpv6_tmp);

	if ( (fp = fopen("net/snmp6", "r")) != NULL)
	{
		count_t	countval;
		int	cur = 0;

		/*
		** one name-value pair per line
		*/
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
		   	nr = sscanf(linebuf, "%s %lld", nam, &countval);

			if (nr < 2)		/* unexpected line ? --> skip */
				continue;

		   	if (strcmp(v6tab[cur].nam, nam) == 0)
		   	{
		   		*(v6tab[cur].val) = countval;
		   	}
		   	else
		   	{
		   		for (cur=0; cur < v6tab_entries; cur++)
					if (strcmp(v6tab[cur].nam, nam) == 0)
						break;

				if (cur < v6tab_entries) /* found ? */
		   			*(v6tab[cur].val) = countval;
			}

			if (++cur >= v6tab_entries)
				cur = 0;
		}

		memcpy(&si->net.ipv6,   &ipv6_tmp,   sizeof ipv6_tmp);
		memcpy(&si->net.icmpv6, &icmpv6_tmp, sizeof icmpv6_tmp);
		memcpy(&si->net.udpv6,  &udpv6_tmp,  sizeof udpv6_tmp);

		fclose(fp);
	}

	/*
	** check if extended partition-statistics are provided < kernel 2.6
	*/
	if ( part_stats && (fp = fopen("partitions", "r")) != NULL)
	{
		char diskname[256];

		i = 0;

		while ( fgets(linebuf, sizeof(linebuf), fp) )
		{
			nr = sscanf(linebuf,
			      "%*d %*d %*d %255s %lld %*d %lld %*d "
			      "%lld %*d %lld %*d %*d %lld %lld",
			        diskname,
				&(si->dsk.dsk[i].nread),
				&(si->dsk.dsk[i].nrsect),
				&(si->dsk.dsk[i].nwrite),
				&(si->dsk.dsk[i].nwsect),
				&(si->dsk.dsk[i].io_ms),
				&(si->dsk.dsk[i].avque) );

			/*
			** check if this line concerns the entire disk
			** or just one of the partitions of a disk (to be
			** skipped)
			*/
			if (nr == 7)	/* full stats-line ? */
			{
				if ( isdisk(0, 0, diskname,
				                 &(si->dsk.dsk[i]),
						 MAXDKNAM) != DSKTYPE)
				       continue;
			
				if (++i >= MAXDSK-1)
					break;
			}
		}

		si->dsk.dsk[i].name[0] = '\0'; /* set terminator for table */
		si->dsk.ndsk = i;

		fclose(fp);

		if (i == 0)
			part_stats = 0;	/* do not try again for next cycles */
	}


	/*
	** check if disk-statistics are provided (kernel 2.6 onwards)
	*/
	if ( (fp = fopen("diskstats", "r")) != NULL)
	{
		char 		diskname[256];
		struct perdsk	tmpdsk;

		si->dsk.ndsk = 0;
		si->dsk.nmdd = 0;
		si->dsk.nlvm = 0;

		while ( fgets(linebuf, sizeof(linebuf), fp) )
		{
			nr = sscanf(linebuf,
			      "%d %d %255s %lld %*d %lld %*d "
			      "%lld %*d %lld %*d %*d %lld %lld",
				&major, &minor, diskname,
				&tmpdsk.nread,  &tmpdsk.nrsect,
				&tmpdsk.nwrite, &tmpdsk.nwsect,
				&tmpdsk.io_ms,  &tmpdsk.avque );

			/*
			** check if this line concerns the entire disk
			** or just one of the partitions of a disk (to be
			** skipped)
			*/
			if (nr == 9)	/* full stats-line ? */
			{
				switch ( isdisk(major, minor, diskname,
							 &tmpdsk, MAXDKNAM) )
				{
				   case NONTYPE:
				       continue;

				   case DSKTYPE:
					if (si->dsk.ndsk < MAXDSK-1)
					  si->dsk.dsk[si->dsk.ndsk++] = tmpdsk;
					break;

				   case MDDTYPE:
					if (si->dsk.nmdd < MAXMDD-1)
					  si->dsk.mdd[si->dsk.nmdd++] = tmpdsk;
					break;

				   case LVMTYPE:
					if (si->dsk.nlvm < MAXLVM-1)
					  si->dsk.lvm[si->dsk.nlvm++] = tmpdsk;
					break;
				}
			}
		}

		/*
 		** set terminator for table
 		*/
		si->dsk.dsk[si->dsk.ndsk].name[0] = '\0';
		si->dsk.mdd[si->dsk.nmdd].name[0] = '\0';
		si->dsk.lvm[si->dsk.nlvm].name[0] = '\0'; 

		fclose(fp);
	}

	/*
 	** get information about the shared memory statistics
	*/
	if ( shmctl(0, SHM_INFO, (struct shmid_ds *)&shminfo) != -1)
	{
		si->mem.shmrss = shminfo.shm_rss;
		si->mem.shmswp = shminfo.shm_swp;
	}

	/*
	** NFS server statistics
	*/
	if ( (fp = fopen("net/rpc/nfsd", "r")) != NULL)
	{
		char    label[32];
		count_t	cnt[40];

		/*
		** every line starts with a small label,
		** followed by upto 60 counters
		*/
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			memset(cnt, 0, sizeof cnt);

			nr = sscanf(linebuf, "%31s %lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld",
			            label,
			            &cnt[0],  &cnt[1],  &cnt[2],  &cnt[3],
			            &cnt[4],  &cnt[5],  &cnt[6],  &cnt[7],
			            &cnt[8],  &cnt[9],  &cnt[10], &cnt[11],
			            &cnt[12], &cnt[13], &cnt[14], &cnt[15],
			            &cnt[16], &cnt[17], &cnt[18], &cnt[19],
			            &cnt[20], &cnt[21], &cnt[22], &cnt[23],
			            &cnt[24], &cnt[25], &cnt[26], &cnt[27],
			            &cnt[28], &cnt[29], &cnt[30], &cnt[31],
			            &cnt[32], &cnt[33], &cnt[34], &cnt[35],
			            &cnt[36], &cnt[37], &cnt[38], &cnt[39]);

			if (nr < 2)		// unexpected empty line ?
				continue;

		   	if (strcmp(label, "rc") == 0)
		   	{
				si->nfs.server.rchits = cnt[0];
				si->nfs.server.rcmiss = cnt[1];
				si->nfs.server.rcnoca = cnt[2];

				continue;
			}

		   	if (strcmp(label, "io") == 0)
		   	{
				si->nfs.server.nrbytes = cnt[0];
				si->nfs.server.nwbytes = cnt[1];

				continue;
			}

		   	if (strcmp(label, "net") == 0)
		   	{
				si->nfs.server.netcnt    = cnt[0];
				si->nfs.server.netudpcnt = cnt[1];
				si->nfs.server.nettcpcnt = cnt[2];
				si->nfs.server.nettcpcon = cnt[3];

				continue;
			}

		   	if (strcmp(label, "rpc") == 0)
		   	{
				si->nfs.server.rpccnt    = cnt[0];
				si->nfs.server.rpcbadfmt = cnt[1];
				si->nfs.server.rpcbadaut = cnt[2];
				si->nfs.server.rpcbadcln = cnt[3];

				continue;
			}
			//
			// first counter behind 'proc..' is number of
			// counters that follow
		   	if (strcmp(label, "proc2") == 0)
		   	{
				si->nfs.server.rpcread  += cnt[7]; // offset+1
				si->nfs.server.rpcwrite += cnt[9]; // offset+1
				continue;
			}
		   	if (strcmp(label, "proc3") == 0)
		   	{
				si->nfs.server.rpcread  += cnt[7]; // offset+1
				si->nfs.server.rpcwrite += cnt[8]; // offset+1
				continue;
			}
		   	if (strcmp(label, "proc4ops") == 0)
		   	{
				si->nfs.server.rpcread  += cnt[26]; // offset+1
				si->nfs.server.rpcwrite += cnt[39]; // offset+1
				continue;
			}
		}

		fclose(fp);
	}

	/*
	** NFS client statistics
	*/
	if ( (fp = fopen("net/rpc/nfs", "r")) != NULL)
	{
		char    label[32];
		count_t	cnt[10];

		/*
		** every line starts with a small label,
		** followed by counters
		*/
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			memset(cnt, 0, sizeof cnt);

			nr = sscanf(linebuf, "%31s %lld %lld %lld %lld %lld"
			                          "%lld %lld %lld %lld %lld",
			            label,
			            &cnt[0], &cnt[1], &cnt[2], &cnt[3],
			            &cnt[4], &cnt[5], &cnt[6], &cnt[7],
			            &cnt[8], &cnt[9]);

			if (nr < 2)		// unexpected empty line ?
				continue;

		   	if (strcmp(label, "rpc") == 0)
		   	{
				si->nfs.client.rpccnt        = cnt[0];
				si->nfs.client.rpcretrans    = cnt[1];
				si->nfs.client.rpcautrefresh = cnt[2];
				continue;
			}

			// first counter behind 'proc..' is number of
			// counters that follow
		   	if (strcmp(label, "proc2") == 0)
		   	{
				si->nfs.client.rpcread  += cnt[7]; // offset+1
				si->nfs.client.rpcwrite += cnt[9]; // offset+1
				continue;
			}
		   	if (strcmp(label, "proc3") == 0)
		   	{
				si->nfs.client.rpcread  += cnt[7]; // offset+1
				si->nfs.client.rpcwrite += cnt[8]; // offset+1
				continue;
			}
		   	if (strcmp(label, "proc4") == 0)
		   	{
				si->nfs.client.rpcread  += cnt[2]; // offset+1
				si->nfs.client.rpcwrite += cnt[3]; // offset+1
				continue;
			}
		}

		fclose(fp);
	}

	/*
	** NFS client: per-mount statistics
	*/
	regainrootprivs();

	if ( (fp = fopen("self/mountstats", "r")) != NULL)
	{
		char 	mountdev[128], fstype[32], label[32];
                count_t	cnt[8];

		i = 0;

		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			// if 'device' line, just remember the mounted device
			if (sscanf(linebuf,
				"device %127s mounted on %*s with fstype %31s",
				mountdev, fstype) == 2)
			{
				continue;
			}

			if (memcmp(fstype, "nfs", 3) != 0)
				continue;

			// this is line with NFS client stats
			nr = sscanf(linebuf,
                                "%31s %lld %lld %lld %lld %lld %lld %lld %lld",
				label, &cnt[0], &cnt[1], &cnt[2], &cnt[3],
				       &cnt[4], &cnt[5], &cnt[6], &cnt[7]);

			if (nr >= 2 )
			{
		   		if (strcmp(label, "age:") == 0)
				{
				    strcpy(si->nfs.nfsmounts.nfsmnt[i].mountdev,
				           mountdev);

				    si->nfs.nfsmounts.nfsmnt[i].age = cnt[0];
				}

		   		if (strcmp(label, "bytes:") == 0)
				{
				    si->nfs.nfsmounts.nfsmnt[i].bytesread =
									cnt[0];
				    si->nfs.nfsmounts.nfsmnt[i].byteswrite =
									cnt[1];
				    si->nfs.nfsmounts.nfsmnt[i].bytesdread =
									cnt[2];
				    si->nfs.nfsmounts.nfsmnt[i].bytesdwrite =
									cnt[3];
				    si->nfs.nfsmounts.nfsmnt[i].bytestotread =
									cnt[4];
				    si->nfs.nfsmounts.nfsmnt[i].bytestotwrite =
									cnt[5];
				    si->nfs.nfsmounts.nfsmnt[i].pagesmread =
									cnt[6];
				    si->nfs.nfsmounts.nfsmnt[i].pagesmwrite =
									cnt[7];

				    if (++i >= MAXNFSMOUNT-1)
					break;
				}
			}
		}

		si->nfs.nfsmounts.nrmounts = i;

		fclose(fp);
	}

	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

	/*
	** pressure statistics in /proc/pressure (>= 4.20)
	**
	** cpu:      some avg10=0.00 avg60=1.37 avg300=3.73 total=30995960
	** io:       some avg10=0.00 avg60=8.83 avg300=22.86 total=141658568
	** io:       full avg10=0.00 avg60=8.33 avg300=21.56 total=133129045
	** memory:   some avg10=0.00 avg60=0.74 avg300=1.67 total=10663184
	** memory:   full avg10=0.00 avg60=0.45 avg300=0.94 total=6461782
	**
	** verify if pressure stats supported by this system
	*/
	if ( chdir("pressure") == 0)
 	{
		struct psi 	psitemp;
		char 		psitype;
		char 		psiformat[] =
				"%c%*s avg10=%f avg60=%f avg300=%f total=%llu";

		si->psi.present = 1;

		if ( (fp = fopen("cpu", "r")) != NULL)
		{
			if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
			{
				nr = sscanf(linebuf, psiformat,
			            	&psitype,
					&psitemp.avg10, &psitemp.avg60,
					&psitemp.avg300, &psitemp.total);

				if (nr == 5)	// complete line ?
					memmove(&(si->psi.cpusome), &psitemp,
								sizeof psitemp);
			}
			fclose(fp);
		}

		if ( (fp = fopen("memory", "r")) != NULL)
		{
			while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
			{
				nr = sscanf(linebuf, psiformat,
			            	&psitype,
					&psitemp.avg10, &psitemp.avg60,
					&psitemp.avg300, &psitemp.total);

				if (nr == 5)
				{
					if (psitype == 's')
						memmove(&(si->psi.memsome),
							&psitemp,
							sizeof psitemp);
					else
						memmove(&(si->psi.memfull),
							&psitemp,
							sizeof psitemp);
				}
			}
			fclose(fp);
		}

		if ( (fp = fopen("io", "r")) != NULL)
		{
			while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
			{
				nr = sscanf(linebuf, psiformat,
			            	&psitype,
					&psitemp.avg10, &psitemp.avg60,
					&psitemp.avg300, &psitemp.total);

				if (nr == 5)
				{
					if (psitype == 's')
						memmove(&(si->psi.iosome),
							&psitemp,
							sizeof psitemp);
					else
						memmove(&(si->psi.iofull),
							&psitemp,
							sizeof psitemp);
				}
			}
			fclose(fp);
		}

		if ( chdir("..") == -1)
			mcleanstop(54, "failed to return to /proc\n");
	}
	else
	{
		si->psi.present = 0;
	}

	/*
	** Container statistics (if any)
	*/
	if ( (fp = fopen("user_beancounters", "r")) != NULL)
	{
		unsigned long	ctid;
		char    	label[32];
		count_t		cnt;

		i = -1;

		/*
		** lines introducing a new container have an extra
		** field with the container id at the beginning.
		*/
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			nr = sscanf(linebuf, "%lu: %31s %lld",
			            		&ctid, label, &cnt);

			if (nr == 3)		// new container ?
			{
				if (++i >= MAXCONTAINER)
					break;

				si->cfs.cont[i].ctid = ctid;
			}
			else
			{
				nr = sscanf(linebuf, "%31s %lld", label, &cnt);

				if (nr != 2)
					continue;
			}

			if (i == -1)	// no container defined yet
				continue;

	   		if (strcmp(label, "numproc") == 0)
	   		{
				si->cfs.cont[i].numproc = cnt;
				continue;
			}

	   		if (strcmp(label, "physpages") == 0)
	   		{
				si->cfs.cont[i].physpages = cnt;
				continue;
			}
		}

		fclose(fp);

		si->cfs.nrcontainer = i+1;

		if ( (fp = fopen("vz/vestat", "r")) != NULL)
		{
			unsigned long	ctid;
			count_t		cnt[8];

			/*
			** relevant lines start with container id
			*/
			while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
			{
				nr = sscanf(linebuf, "%lu  %lld %lld %lld %lld"
			                             "%lld %lld %lld %lld %lld",
			            	&ctid,
					&cnt[0], &cnt[1], &cnt[2], &cnt[3],
			            	&cnt[4], &cnt[5], &cnt[6], &cnt[7],
					&cnt[8]);

				if (nr < 9)	// irrelevant contents
					continue;

				// relevant stats: search for containerid
				for (i=0; i < si->cfs.nrcontainer; i++)
				{
					if (si->cfs.cont[i].ctid == ctid)
						break;
				}

				if (i >= si->cfs.nrcontainer)
					continue;	// container not found

				si->cfs.cont[i].user   = cnt[0];
				si->cfs.cont[i].nice   = cnt[1];
				si->cfs.cont[i].system = cnt[2];
				si->cfs.cont[i].uptime = cnt[3];
			}

			fclose(fp);
		}
	}

	/*
 	** verify presence of InfiniBand controllers
	** warning: possibly switches to other directory
	*/
	if (ib_stats)
		ib_stats = get_infiniband(&(si->ifb));

	/*
	** get counters related to ksm
	*/
	if (ksm_stats)
		ksm_stats = get_ksm(si);

	/*
	** get counters related to zswap
	*/
	if (zswap_stats)
		zswap_stats = get_zswap(si);

	/*
 	** return to original directory
	*/
	if ( chdir(origdir) == -1)
		mcleanstop(55, "failed to change to %s\n", origdir);

#ifndef	NOPERFEVENT
	/*
	** get low-level CPU event counters
	*/
        getperfevents(&(si->cpu));
#endif

	/*
	** fetch application-specific counters
	*/
#if	HTTPSTATS
	if ( wwwvalid)
		wwwvalid = getwwwstat(80, &(si->www));
#endif
}

/*
** set of subroutines to determine which disks should be monitored
** and to translate name strings into (shorter) name strings
*/
static void
nullmodname(unsigned int major, unsigned int minor,
		char *curname, struct perdsk *px, int maxlen)
{
	strncpy(px->name, curname, maxlen-1);
	*(px->name+maxlen-1) = 0;
}

static void
abbrevname1(unsigned int major, unsigned int minor,
		char *curname, struct perdsk *px, int maxlen)
{
	char	cutype[128];
	int	hostnum, busnum, targetnum, lunnum;

	sscanf(curname, "%[^/]/host%d/bus%d/target%d/lun%d",
			cutype, &hostnum, &busnum, &targetnum, &lunnum);

	snprintf(px->name, maxlen, "%c-h%db%dt%d", 
			cutype[0], hostnum, busnum, targetnum);
}

/*
** recognize LVM logical volumes
*/
#define	NUMDMHASH	64
#define	DMHASH(x,y)	(((x)+(y))%NUMDMHASH)	
#define	MAPDIR		"/dev/mapper"

struct devmap {
	unsigned int	major;
	unsigned int	minor;
	char		name[MAXDKNAM];
	struct devmap	*next;
};

static void
lvmmapname(unsigned int major, unsigned int minor,
		char *curname, struct perdsk *px, int maxlen)
{
	static int		firstcall = 1;
	static struct devmap	*devmaps[NUMDMHASH], *dmp;
	int			hashix;

	/*
 	** setup a list of major-minor numbers of dm-devices with their
	** corresponding name
	*/
	if (firstcall)
	{
		DIR		*dirp;
		struct dirent	*dentry;
		struct stat	statbuf;
		char		path[PATH_MAX];

		if ( (dirp = opendir(MAPDIR)) )
		{
			/*
	 		** read every directory-entry and search for
			** block devices
			*/
			while ( (dentry = readdir(dirp)) )
			{
				snprintf(path, sizeof path, "%s/%s", 
						MAPDIR, dentry->d_name);

				if ( stat(path, &statbuf) == -1 )
					continue;

				if ( ! S_ISBLK(statbuf.st_mode) )
					continue;
				/*
 				** allocate struct to store name
				*/
				if ( !(dmp = malloc(sizeof (struct devmap))))
					continue;

				/*
 				** store info in hash list
				*/
				strncpy(dmp->name, dentry->d_name, MAXDKNAM);
				dmp->name[MAXDKNAM-1] = 0;
				dmp->major 	= major(statbuf.st_rdev);
				dmp->minor 	= minor(statbuf.st_rdev);

				hashix = DMHASH(dmp->major, dmp->minor);

				dmp->next	= devmaps[hashix];

				devmaps[hashix]	= dmp;
			}

			closedir(dirp);
		}

		firstcall = 0;
	}

	/*
 	** find info in hash list
	*/
	hashix  = DMHASH(major, minor);
	dmp	= devmaps[hashix];

	while (dmp)
	{
		if (dmp->major == major && dmp->minor == minor)
		{
			/*
		 	** info found in hash list; fill proper name
			*/
			strncpy(px->name, dmp->name, maxlen-1);
			*(px->name+maxlen-1) = 0;
			return;
		}

		dmp = dmp->next;
	}

	/*
	** info not found in hash list; fill original name
	*/
	strncpy(px->name, curname, maxlen-1);
	*(px->name+maxlen-1) = 0;
}

/*
** this table is used in the function isdisk()
**
** table contains the names (in regexp format) of disks
** to be recognized, together with a function to modify
** the name-strings (i.e. to abbreviate long strings);
** some frequently found names (like 'loop' and 'ram')
** are also recognized to skip them as fast as possible
*/
static struct {
	char 	*regexp;
	regex_t	compreg;
	void	(*modname)(unsigned int, unsigned int,
				char *, struct perdsk *, int);
	int	retval;
} validdisk[] = {
	{ "^ram[0-9][0-9]*$",			{0},  (void *)0,   NONTYPE, },
	{ "^loop[0-9][0-9]*$",			{0},  (void *)0,   NONTYPE, },
	{ "^sd[a-z][a-z]*$",			{0},  nullmodname, DSKTYPE, },
	{ "^dm-[0-9][0-9]*$",			{0},  lvmmapname,  LVMTYPE, },
	{ "^md[0-9][0-9]*$",			{0},  nullmodname, MDDTYPE, },
	{ "^vd[a-z][a-z]*$",                    {0},  nullmodname, DSKTYPE, },
	{ "^nvme[0-9][0-9]*n[0-9][0-9]*$",	{0},  nullmodname, DSKTYPE, },
	{ "^nvme[0-9][0-9]*c[0-9][0-9]*n[0-9][0-9]*$", {0}, nullmodname, DSKTYPE, },
	{ "^nbd[0-9][0-9]*$",			{0},  nullmodname, DSKTYPE, },
	{ "^hd[a-z]$",				{0},  nullmodname, DSKTYPE, },
	{ "^rd/c[0-9][0-9]*d[0-9][0-9]*$",	{0},  nullmodname, DSKTYPE, },
	{ "^cciss/c[0-9][0-9]*d[0-9][0-9]*$",	{0},  nullmodname, DSKTYPE, },
	{ "^fio[a-z][a-z]*$",			{0},  nullmodname, DSKTYPE, },
	{ "/host.*/bus.*/target.*/lun.*/disc",	{0},  abbrevname1, DSKTYPE, },
	{ "^xvd[a-z][a-z]*[0-9]*$",		{0},  nullmodname, DSKTYPE, },
	{ "^dasd[a-z][a-z]*$",			{0},  nullmodname, DSKTYPE, },
	{ "^mmcblk[0-9][0-9]*$",		{0},  nullmodname, DSKTYPE, },
	{ "^emcpower[a-z][a-z]*$",		{0},  nullmodname, DSKTYPE, },
};

static int
isdisk(unsigned int major, unsigned int minor,
           char *curname, struct perdsk *px, int maxlen)
{
	static int	firstcall = 1;
	register int	i;

	if (firstcall)		/* compile the regular expressions */
	{
		for (i=0; i < sizeof validdisk/sizeof validdisk[0]; i++)
			regcomp(&validdisk[i].compreg, validdisk[i].regexp,
								REG_NOSUB);
		firstcall = 0;
	}

	/*
	** try to recognize one of the compiled regular expressions
	*/
	for (i=0; i < sizeof validdisk/sizeof validdisk[0]; i++)
	{
		if (regexec(&validdisk[i].compreg, curname, 0, NULL, 0) == 0)
		{
			/*
			** name-string recognized; modify name-string
			*/
			if (validdisk[i].retval != NONTYPE)
				(*validdisk[i].modname)(major, minor,
						curname, px, maxlen);

			return validdisk[i].retval;
		}
	}

	return NONTYPE;
}

/*
** LINUX SPECIFIC:
** Determine boot-time of this system (as number of jiffies since 1-1-1970).
*/
unsigned long long
getbootlinux(long hertz)
{
	int    		 	cpid;
	char  	  		tmpbuf[1280];
	FILE    		*fp;
	unsigned long 		startticks;
	unsigned long long	bootjiffies = 0;
	struct timespec		ts;

	/*
	** dirty hack to get the boottime, since the
	** Linux 2.6 kernel (2.6.5) does not return a proper
	** boottime-value with the times() system call   :-(
	*/
	if ( (cpid = fork()) == 0 )
	{
		/*
		** child just waiting to be killed by parent
		*/
		pause();
	}
	else
	{
		/*
		** parent determines start-time (in jiffies since boot) 
		** of the child and calculates the boottime in jiffies
		** since 1-1-1970
		*/
		(void) clock_gettime(CLOCK_REALTIME, &ts);	// get current
		bootjiffies = 1LL * ts.tv_sec  * hertz +
		              1LL * ts.tv_nsec * hertz / 1000000000LL;

		snprintf(tmpbuf, sizeof tmpbuf, "/proc/%d/stat", cpid);

		if ( (fp = fopen(tmpbuf, "r")) != NULL)
		{
			if ( fscanf(fp, "%*d (%*[^)]) %*c %*d %*d %*d %*d "
			                "%*d %*d %*d %*d %*d %*d %*d %*d "
			                "%*d %*d %*d %*d %*d %*d %lu",
			                &startticks) == 1)
			{
				bootjiffies -= startticks;
			}

			fclose(fp);
		}

		/*
		** kill the child and get rid of the zombie
		*/
		kill(cpid, SIGKILL);
		(void) wait((int *)0);
	}

	return bootjiffies;
}


/*
** get stats of all InfiniBand ports below
**    /sys/class/infiniband/<controller>/ports/<port#>/....
*/
static struct ibcachent {
	char		*ibha;		// InfiniBand Host Adaptor
	unsigned short	port;		// port number
	unsigned short	lanes;		// number of lanes
	count_t 	rate;		// transfer rate in bytes/sec
	char 		*pathrcvb;	// path name for received bytes
	char 		*pathsndb;	// path name for transmitted bytes
	char 		*pathrcvp;	// path name for received packets
	char 		*pathsndp;	// path name for transmitted packets
} ibcache[MAXIBPORT];

static int	nib;			// number of IB ports in cache

static void	ibprep(struct ibcachent *);
static int	ibstat(struct ibcachent *, struct perifb *);

static int
get_infiniband(struct ifbstat *si)
{
	static int	firstcall = 1;
	int		i;

	// verify if InfiniBand used in this system
	if ( chdir("/sys/class/infiniband") == -1)
		return 0;	// no path, no IB, so don't try again

	if (firstcall)
	{
		char		path[PATH_MAX], *p;
		struct stat	statbuf;
		struct dirent	*contdent, *portdent;
		DIR		*contp, *portp;

		firstcall = 0;

		/*
		** once setup a cache with all info that is needed
		** to gather the necessary stats with every subsequent
		** call, including  path names, etcetera.
		*/
		if ( (contp = opendir(".")) )
		{
			/*
 			** read every directory-entry and search for
			** subdirectories (i.e. controllers)
			*/
			while ( (contdent = readdir(contp)) )
			{
				// skip . and ..
				if (contdent->d_name[0] == '.')
					continue;

				if ( stat(contdent->d_name, &statbuf) == -1 )
					continue;
	
				if ( ! S_ISDIR(statbuf.st_mode) )
					continue;

				// controller found
				// store controller name for cache
				//
				p = malloc( strlen(contdent->d_name)+1 );
				strcpy(p, contdent->d_name);

				if (strlen(contdent->d_name) > MAXIBNAME-1)
					p[MAXIBNAME-1] = '\0';

				// discover all ports
				//
				snprintf(path, sizeof path, "%s/ports",
							contdent->d_name);

				if ( (portp = opendir(path)) )
				{
					/*
 					** read every directory-entry and
					** search for subdirectories (i.e.
					** port numbers)
					*/
					while ( (portdent = readdir(portp)) )
					{
						int port;

						// skip . and ..
						if (portdent->d_name[0] == '.')
							continue;

						port = atoi(portdent->d_name);

						if (!port)
							continue;

						// valid port
						// fill cache info
						//
						ibcache[nib].port = port;
						ibcache[nib].ibha = p;

						ibprep(&ibcache[nib]);

						if (++nib >= MAXIBPORT)
							break;
					}

					closedir(portp);

					if (nib >= MAXIBPORT)
						break;
				}
			}

			closedir(contp);
		}
	}

	/*
	** get static and variable metrics
	*/
	for (i=0; i < nib; i++)
	{
		// static metrics from cache
		strcpy(si->ifb[i].ibname, ibcache[i].ibha);

		si->ifb[i].portnr = ibcache[i].port;
		si->ifb[i].lanes  = ibcache[i].lanes;
		si->ifb[i].rate   = ibcache[i].rate;

		// variable metrics from sysfs
		ibstat(&(ibcache[i]), &(si->ifb[i]));
	}	

	si->nrports = nib;
	return 1;
}

/*
** determine rate and number of lanes
** from <contr>/ports/<port>/rate --> e.g. "100 Gb/sec (4X EDR)"
** and assemble path names to be used for counters later on
*/
static void
ibprep(struct ibcachent *ibc)
{
	FILE	*fp;
	char	path[PATH_MAX], linebuf[64], speedunit;

	// determine port rate and number of lanes
	snprintf(path, sizeof path, "%s/ports/%d/rate", ibc->ibha, ibc->port); 

	if ( (fp = fopen(path, "r")) )
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			(void) sscanf(linebuf, "%lld %c%*s (%hdX",
				&(ibc->rate), &speedunit, &(ibc->lanes));

			// calculate megabits/second
			switch (speedunit)
			{
			   case 'M':
			   case 'm':
				break;
			   case 'G':
			   case 'g':
				ibc->rate *= 1000;
				break;
			   case 'T':
			   case 't':
				ibc->rate *= 1000000;
				break;
			}

		}
		else
		{
			ibc->lanes = 0;
			ibc->rate = 0;
		}

		fclose(fp);
	}

	// build all pathnames to obtain the counters
	// of this port later on
	snprintf(path, sizeof path, "%s/ports/%d/counters/port_rcv_data",
						    ibc->ibha, ibc->port);
	ibc->pathrcvb = malloc( strlen(path)+1 );
	strcpy(ibc->pathrcvb, path);

	snprintf(path, sizeof path, "%s/ports/%d/counters/port_xmit_data",
						    ibc->ibha, ibc->port);
	ibc->pathsndb = malloc( strlen(path)+1 );
	strcpy(ibc->pathsndb, path);

	snprintf(path, sizeof path, "%s/ports/%d/counters/port_rcv_packets",
						    ibc->ibha, ibc->port);
	ibc->pathrcvp = malloc( strlen(path)+1 );
	strcpy(ibc->pathrcvp, path);

	snprintf(path, sizeof path, "%s/ports/%d/counters/port_xmit_packets",
						    ibc->ibha, ibc->port);
	ibc->pathsndp = malloc( strlen(path)+1 );
	strcpy(ibc->pathsndp, path);
}

/*
** read necessary variable counters for this IB port
*/
static int
ibstat(struct ibcachent *ibc, struct perifb *ifb)
{
	FILE	*fp;
	char	linebuf[64];

	if ( (fp = fopen(ibc->pathrcvb, "r")) )
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if (sscanf(linebuf, "%lld", &(ifb->rcvb)) == 0)
				ifb->rcvb = 0;
		}
		else
		{
			ifb->rcvb = 0;
		}
		fclose(fp);
	}

	if ( (fp = fopen(ibc->pathsndb, "r")) )
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if (sscanf(linebuf, "%lld", &(ifb->sndb)) == 0)
				ifb->sndb = 0;
		}
		else
		{
			ifb->sndb = 0;
		}
		fclose(fp);
	}

	if ( (fp = fopen(ibc->pathrcvp, "r")) )
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if (sscanf(linebuf, "%lld", &(ifb->rcvp)) == 0)
				ifb->rcvp = 0;
		}
		else
		{
			ifb->rcvp = 0;
		}
		fclose(fp);
	}

	if ( (fp = fopen(ibc->pathsndp, "r")) )
	{
		if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			if (sscanf(linebuf, "%lld", &(ifb->sndp)) == 0)
				ifb->sndp = 0;
		}
		else
		{
			ifb->sndp = 0;
		}
		fclose(fp);
	}

	return 1;
}

/*
** retrieve ksm values (if switched on)
*/
static int
get_ksm(struct sstat *si)
{
	FILE *fp;
	int  state;

	si->mem.ksmsharing = -1;
	si->mem.ksmshared  = -1;

	if ((fp=fopen("/sys/kernel/mm/ksm/run", "r")) != 0)
	{
		if (fscanf(fp, "%d", &state) == 1)
		{
			if (state == 0)
			{
				fclose(fp);
				return 0; // no more calling
			}
		}

		fclose(fp);
	}

	if ((fp=fopen("/sys/kernel/mm/ksm/pages_sharing", "r")) != 0)
	{
		if (fscanf(fp, "%llu", &(si->mem.ksmsharing)) != 1)
			si->mem.ksmsharing = 0;

		fclose(fp);
	}

	if ((fp=fopen("/sys/kernel/mm/ksm/pages_shared", "r")) != 0)
	{
		if (fscanf(fp, "%llu", &(si->mem.ksmshared)) != 1)
			si->mem.ksmshared = 0;

		fclose(fp);
	}

	return 1;
}

/*
** retrieve zswap values (if switched on)
*/
static int
get_zswap(struct sstat *si)
{
	FILE *fp;
	char  state;

	si->mem.zswtotpool = -1;
	si->mem.zswstored  = -1;

	if ((fp=fopen("/sys/module/zswap/parameters/enabled", "r")) != 0)
	{
		if (fscanf(fp, "%c", &state) == 1)
		{
			if (state != 'Y')
			{
				fclose(fp);
				return 0; // no more calling
			}
		}

		fclose(fp);
	}

	regainrootprivs();

	if ((fp=fopen("/sys/kernel/debug/zswap/pool_total_size", "r")) != 0)
	{
		if (fscanf(fp, "%llu", &(si->mem.zswtotpool)) != 1)
			si->mem.zswtotpool = 0;
		else
			si->mem.zswtotpool /= pagesize;

		fclose(fp);
	}

	if ((fp=fopen("/sys/kernel/debug/zswap/stored_pages", "r")) != 0)
	{
		if (fscanf(fp, "%llu", &(si->mem.zswstored)) != 1)
			si->mem.zswstored = 0;

		fclose(fp);
	}

	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

	return 1;
}


#if	HTTPSTATS
/*
** retrieve statistics from local HTTP daemons
** via http://localhost/server-status?auto
*/
int
getwwwstat(unsigned short port, struct wwwstat *wp)
{
	int 			sockfd, tobefound;
	FILE			*sockfp;
	struct sockaddr_in	sockname;
	char			linebuf[4096];
	char			label[512];
	long long		value;

	memset(wp, 0, sizeof *wp);

	/*
	** allocate a socket and connect to the local HTTP daemon
	*/
	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return 0;

	sockname.sin_family		= AF_INET;
	sockname.sin_addr.s_addr	= htonl(INADDR_LOOPBACK);
	sockname.sin_port		= htons(port);

	if ( connect(sockfd, (struct sockaddr *) &sockname,
						sizeof sockname) == -1)
	{
		close(sockfd);
		return 0;
	}

	/*
	** write a GET-request for /server-status
	*/
	if ( write(sockfd, HTTPREQ, sizeof HTTPREQ) < sizeof HTTPREQ)
	{
		close(sockfd);
		return 0;
	}

	/*
	** remap socket descriptor to a stream to allow stdio calls
	*/
	sockfp = fdopen(sockfd, "r+");

	/*
	** read response line by line
	*/
	tobefound = 5;		/* number of values to be searched */

	while ( fgets(linebuf, sizeof linebuf, sockfp) && tobefound)
	{
		/*
		** handle line containing status code
		*/
		if ( strncmp(linebuf, "HTTP/", 5) == 0)
		{
			sscanf(linebuf, "%511s %lld %*s\n", label, &value);

			if (value != 200)	/* HTTP-request okay? */
			{
				fclose(sockfp);
				close(sockfd);
				return 0;
			}

			continue;
		}

		/*
		** decode line and search for the required counters
		*/
		if (sscanf(linebuf, "%511[^:]: %lld\n", label, &value) == 2)
		{
			if ( strcmp(label, "Total Accesses") == 0)
			{
				wp->accesses = value;
				tobefound--;
			}

			if ( strcmp(label, "Total kBytes") == 0)
			{
				wp->totkbytes = value;
				tobefound--;
			}

			if ( strcmp(label, "Uptime") == 0)
			{
				wp->uptime = value;
				tobefound--;
			}

			if ( strcmp(label, "BusyWorkers") == 0)
			{
				wp->bworkers = value;
				tobefound--;
			}

			if ( strcmp(label, "IdleWorkers") == 0)
			{
				wp->iworkers = value;
				tobefound--;
			}
		}
	}

	fclose(sockfp);
	close(sockfd);

	return 1;
}
#endif



/*
** retrieve low-level CPU events:
** 	instructions and cycles per CPU
*/
#ifndef	NOPERFEVENT

void
do_perfevents(char *tagname, char *tagvalue)
{
	if (!strcmp("enable", tagvalue))
		perfevents = PERF_EVENTS_ENABLE;
	else if (!strcmp("disable", tagvalue))
		perfevents = PERF_EVENTS_DISABLE;
	else
	{
		if (run_in_guest())
			perfevents = PERF_EVENTS_DISABLE;
		else
			perfevents = PERF_EVENTS_ENABLE;
	}
}

static int
enable_perfevents()
{
	if (perfevents == PERF_EVENTS_AUTO)
		do_perfevents("perfevents", "auto");

	return perfevents == PERF_EVENTS_ENABLE;
}

long
perf_event_open(struct perf_event_attr *hwevent, pid_t pid,
                int cpu, int groupfd, unsigned long flags)
{
	return syscall(__NR_perf_event_open, hwevent, pid, cpu, groupfd, flags);
}

static void
getperfevents(struct cpustat *cs)
{
	static int	firstcall = 1, cpualloced, *fdi, *fdc;
	int		i;
	int 		liResult;

	if (!enable_perfevents())
		return;

	/*
 	** once initialize perf event counter retrieval
	*/
	if (firstcall)
	{
		struct perf_event_attr  pea;
		int			success = 0;

		firstcall = 0;

		/*
		** allocate space for per-cpu file descriptors
		*/
		cpualloced = cs->nrcpu;
		fdi        = malloc(sizeof(int) * cpualloced);
		fdc        = malloc(sizeof(int) * cpualloced);

		/*
		** fill perf_event_attr struct with appropriate values
		*/
		memset(&pea, 0, sizeof(struct perf_event_attr));

		pea.type    = PERF_TYPE_HARDWARE;
		pea.size    = sizeof(struct perf_event_attr);
		pea.inherit = 1;
		pea.pinned  = 1;

	 	regainrootprivs();

		for (i=0; i < cpualloced; i++)
		{
			pea.config = PERF_COUNT_HW_INSTRUCTIONS;

			if ( (*(fdi+i) = perf_event_open(&pea, -1, i, -1,
 						PERF_FLAG_FD_CLOEXEC)) >= 0)
				success++;

                        pea.config = PERF_COUNT_HW_CPU_CYCLES;

			if ( (*(fdc+i) = perf_event_open(&pea, -1, i, -1,
						PERF_FLAG_FD_CLOEXEC)) >= 0)
				success++;
		}

		if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");


		/*
		** all failed (probably no kernel support)?
		*/
		if (success == 0)	
		{
			free(fdi);
			free(fdc);
			cpualloced = 0;
		}
		else
		{
			cs->all.instr = 1;
			cs->all.cycle = 1;
		}

		return;		// initialization finished for first sample
        }

	/*
	** every sample: check if counters available anyhow
	*/
        if (!cpualloced)
                return;

	/*
   	** retrieve counters per CPU and in total
	*/
	cs->all.instr = 0;
	cs->all.cycle = 0;

        for (i=0; i < cpualloced; i++)
        {
		if (*(fdi+i) != -1)
		{
                	liResult = read(*(fdi+i), &(cs->cpu[i].instr), sizeof(count_t));
                        cs->all.instr += cs->cpu[i].instr;
			if(liResult < 0)
			{
				char lcMessage[64];

				snprintf(lcMessage, sizeof(lcMessage) - 1,
				          "%s:%d - Error %d reading instr counters\n",
				           __FILE__, __LINE__, errno);
				fprintf(stderr, "%s", lcMessage);
			}

                	liResult = read(*(fdc+i), &(cs->cpu[i].cycle), sizeof(count_t));
                        cs->all.cycle += cs->cpu[i].cycle;
			if(liResult < 0)
			{
				char lcMessage[64];

				snprintf(lcMessage, sizeof(lcMessage) - 1,
				          "%s:%d - Error %d reading cycle counters\n",
				           __FILE__, __LINE__, errno );
				fprintf(stderr, "%s", lcMessage);
			}
		}
        }
}

static int
run_in_guest(void)
{
	return get_hypervisor() != HYPER_NONE;
}

#if defined(__x86_64__) || defined(__i386__)
#define HYPERVISOR_INFO_LEAF   0x40000000

static inline void
x86_cpuid(unsigned int op, unsigned int *eax, unsigned int *ebx,
			 unsigned int *ecx, unsigned int *edx)
{
	__asm__(
#if defined(__PIC__) && defined(__i386__)
		"xchg %%ebx, %%esi;"
		"cpuid;"
		"xchg %%esi, %%ebx;"
		: "=S" (*ebx),
#else
		"cpuid;"
		: "=b" (*ebx),
#endif
		  "=a" (*eax),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "1" (op), "c"(0));
}

static int
get_hypervisor(void)
{
	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, hyper = HYPER_NONE;
	char hyper_vendor_id[13];

	memset(hyper_vendor_id, 0, sizeof(hyper_vendor_id));

	x86_cpuid(HYPERVISOR_INFO_LEAF, &eax, &ebx, &ecx, &edx);
	memcpy(hyper_vendor_id + 0, &ebx, 4);
	memcpy(hyper_vendor_id + 4, &ecx, 4);
	memcpy(hyper_vendor_id + 8, &edx, 4);
	hyper_vendor_id[12] = '\0';

	if (!hyper_vendor_id[0])
		return hyper;

	if (!strncmp("XenVMMXenVMM", hyper_vendor_id, 12))
		hyper = HYPER_XEN;
	else if (!strncmp("KVMKVMKVM", hyper_vendor_id, 9))
		hyper = HYPER_KVM;
	else if (!strncmp("Microsoft Hv", hyper_vendor_id, 12))
		hyper = HYPER_MSHV;
	else if (!strncmp("VMwareVMware", hyper_vendor_id, 12))
		hyper = HYPER_VMWARE;
	else if (!strncmp("UnisysSpar64", hyper_vendor_id, 12))
		hyper = HYPER_SPAR;

	return hyper;
}
#else /* ! (__x86_64__ || __i386__) */
static int
get_hypervisor(void)
{
	return HYPER_NONE;
}
#endif

#else /* ! NOPERFEVENT */
void
do_perfevents(char *tagname, char *tagvalue)
{
	if (strcmp("disable", tagvalue))
		mcleanstop(1, "atop built with NOPERFEVENT, cannot use perfevents\n");
}
#endif
