/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** Include-file describing bpf-porgram-level counters maintained and
** functions to access the bpf-program-database.
** ================================================================
** Author:      Song Liu
** E-mail:      song@kernel.org
** Date:        Dec 2019
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
#include <linux/bpf.h>

/*
** structure containing information about a bpf program
*/
struct bstat {
	__u32 type;
	__u32 id;
	char name[BPF_OBJ_NAME_LEN];
	__u64 run_time_ns;
	__u64 run_cnt;
} __attribute__((aligned(8)));

/*
** structure containing information about all bpf programs, or a deviate of
** two snapshots.
*/
struct bstats {
	struct bstat *bpfall;

	unsigned int nbpfall;
};

#ifdef ATOP_BPF_SUPPORT
void	photo_bpf_check();
int	system_support_bpf(void);

struct bstats	*get_devbstats(void);
int		pribpf(struct bstats *devbstat, int curline);
#else
static inline void
photo_bpf_check(void) {}

static inline int
system_support_bpf(void)
{
	return 0;
}

static inline struct bstats	*
get_devbstats(void)
{
	return NULL;
}

static inline int
pribpf(struct bstats *devbstat, int curline)
{
	return curline;
}

#endif
