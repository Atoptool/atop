/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains functions to read bpf-porgram counters.
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

#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <curses.h>
#include <regex.h>

#include "atop.h"
#include "photobpf.h"
#include "showgeneric.h"

#define BPF_STATS_MIN_ALLOC 8

/*
** check configurations:
**   bpfsampleinterval < interval
*/
void
photo_bpf_check(void)
{
	if (bpfsampleinterval >= interval)
		mcleanstop(1, "bpfsampleinterval must be smaller than interval");
}

/* boolean */
int
system_support_bpf(void)
{
	int bpf_stats_fd = bpf_enable_stats(BPF_STATS_RUN_TIME);

	/* check bpf_enable_stats() returns valid fd */
	if (bpf_stats_fd < 0)
		return 0;
	close(bpf_stats_fd);
	return 1;
}

/*
** Get a snapshot of all bpf program stats
*/
static struct bstats *
get_allbstats(void)
{
	int		err;
	__u32		id = 0;
	unsigned int	bufsize = 0;
	struct bstats	*curbstats = malloc(sizeof(struct bstats));

	ptrverify(curbstats, "Malloc failed for bstats\n");

	memset(curbstats, 0, sizeof(struct bstats));

	while (true) {
		int			fd;
		struct bpf_prog_info	info = {};
		__u32			len = sizeof(struct bpf_prog_info);
		struct bstat		*bstat;

		err = bpf_prog_get_next_id(id, &id);
		if (err)
			break;
		if (curbstats->nbpfall >= bufsize) {
			bufsize += BPF_STATS_MIN_ALLOC;
			curbstats->bpfall = realloc(
				curbstats->bpfall,
				bufsize * sizeof(struct bstat));
			ptrverify(curbstats->bpfall,
				  "Malloc failed for %u bstats\n",
				  bufsize);
		}

		fd = bpf_prog_get_fd_by_id(id);
		if (fd < 0) {
			if (errno == ENOENT)
				continue;
			break;
		}

		err = bpf_obj_get_info_by_fd(fd, &info, &len);
		close(fd);
		if (err)
			break;

		bstat = curbstats->bpfall + curbstats->nbpfall;

		bstat->type = info.type;
		bstat->id = info.id;
		memcpy(bstat->name, info.name, BPF_OBJ_NAME_LEN);
		bstat->run_time_ns = info.run_time_ns;
		bstat->run_cnt = info.run_cnt;

		curbstats->nbpfall++;
	}

	return curbstats;
}

static int
bstatcompar(const void *a, const void *b)
{
	struct bstat	*sa = (struct bstat *)a;
	struct bstat	*sb = (struct bstat *)b;

	if (sa->run_time_ns < sb->run_time_ns)
		return 1;
	if (sa->run_time_ns > sb->run_time_ns)
		return -1;

	return  0;
}

/*
** return deviate bpf program stats, sorted by run_time_ns.
** programs with zero run_time_ns are ingored.
*/
struct bstats *
get_devbstats(void)
{
	int			i, j;
	struct bstats		*prebstats;
	struct bstats		*curbstats;
	int			bpf_stats_fd;

	if (!(supportflags & BPFSTAT)) {
		printf("NO support bpf\n");
		return NULL;
	}

	/* bpf_enable_stats() enables stats and returns a valid fd */
	bpf_stats_fd = bpf_enable_stats(BPF_STATS_RUN_TIME);

	if (bpf_stats_fd < 0) {
		supportflags &= ~BPFSTAT;
		return NULL;
	}

	prebstats = get_allbstats();
	sleep(bpfsampleinterval);
	/* close the fd to disable stats */
	close(bpf_stats_fd);
	curbstats = get_allbstats();

	/*
	** deviate, save result in curbstats
	*/
	i = 0;
	j = 0;
	while (i < prebstats->nbpfall && j < curbstats->nbpfall) {
		struct bstat	*prebstat = prebstats->bpfall + i;
		struct bstat	*curbstat = curbstats->bpfall + j;

		if (prebstat->id < curbstat->id) {
			i++;
		} else if (prebstat->id > curbstat->id) {
			j++;
		} else {
			curbstat->run_cnt -= prebstat->run_cnt;
			curbstat->run_time_ns -= prebstat->run_time_ns;
			i++;
			j++;
		}
	}

	free(prebstats->bpfall);

	/*
	** sort result based on run_time_ns
	*/
	qsort(curbstats->bpfall, curbstats->nbpfall,
	      sizeof(curbstats->bpfall[0]),
	      bstatcompar);

	/*
	** drop stats with zero run time
	*/
	for (i = 0; i < curbstats->nbpfall; i++)
		if (curbstats->bpfall[i].run_time_ns == 0) {
			curbstats->bpfall = realloc(curbstats->bpfall,
						    i * sizeof(struct bstat));
			curbstats->nbpfall = i;
			break;
		}

	return curbstats;
}

/*
** Print bpf stats.
** For less than 76 columns, skip.
** For 76 or more columens, show
**   BPF_PROG_ID	11 bytes
**   NAME		17+ bytes
**   TOTAL_TIME_NS	15 bytes
**   RUN_CNT		13 bytes
**   CPU %		8 bytes
**   AVG_TIME_NS	12 bytes
*/
int
pribpf(struct bstats *devbstat, int curline)
{
	int	i;
	int	maxw = screen ? COLS : linelen;
	int	namewidth = maxw - 59;

	if ((supportflags & BPFSTAT) == 0 || maxw < 76 || !devbstat)
		return curline;

	curline += 1;
	if (screen) {
		move(curline, 0);
		attron(A_REVERSE);
	} else {
		printg("\n\n");
	}
	printg("%11s%*s%15s%13s%8s%12s", "BPF_PROG_ID", namewidth, "NAME", "TOTAL_TIME_NS",
	       "RUN_CNT", "CPU", "AVG_TIME_NS");

	if (screen)
		attroff(A_REVERSE);
	else
		printg("\n");

	for (i = 0; i < devbstat->nbpfall; i++) {
		float		avgtime;
		struct bstat	*bstat = devbstat->bpfall + i;

		/*
		** show at most bpflines on screen
		*/
  		if ((screen && i >= bpflines))
			break;

		curline += 1;
		if (screen)
			move(curline, 0);
		else
			printg("\n");

		avgtime = (bstat->run_cnt == 0) ? 0.0 :
			(float)(bstat->run_time_ns) / bstat->run_cnt;

		printg("%11d%*s%15llu%13llu%7llu%%%12.2f", bstat->id, namewidth,
		       bstat->name, bstat->run_time_ns, bstat->run_cnt,
		       bstat->run_time_ns / bpfsampleinterval / 10000000ULL,
		       avgtime);
	}

	return curline;
}
