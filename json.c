/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** ==========================================================================
** Author:      Fei Li & zhenwei pi
** E-mail:      lifei.shirley@bytedance.com, pizhenwei@bytedance.com
** Date:        August 2019
** --------------------------------------------------------------------------
** Copyright (C) 2019 bytedance.com
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
** Revision 1.1  2019/08/08 14:02:19
** Initial revision
** Add support for json style output, basing on the parseable.c file.
**
** Currently, we support three types of output:
** 1. atop -O stdio
** 2. atop -O only
** 3. atop -O unixsock -w /path/to/file 10
**
** Usage examples:
**  ./atop
**  ./atop -P ALL
**  ./atop -O only // overwrite parseout, show json to stdio only
**  ./atop -O stdio -P ALL // both parseout and json stdio
**  ./atop -O stdio -w atop.log // print to stdio, as well as file
**  ./atop -O unixsock // overwrite parseout, show json to unixsock
**  ./atop -O unixsock -P ALL // both parseout and json unixsock
**  ./atop -O unixsock -w atop.log // write json to unixsock and file
**
*/
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>

#include "atop.h"
#include "photosyst.h"
#include "photoproc.h"
#include "json.h"

int 	json_print_CPU();
int 	json_print_cpu();
int 	json_print_CPL();
int 	json_print_GPU();
int 	json_print_MEM();
int 	json_print_SWP();
int 	json_print_PAG();
int 	json_print_PSI();
int 	json_print_LVM();
int 	json_print_MDD();
int 	json_print_DSK();
int 	json_print_NFM();
int 	json_print_NFC();
int 	json_print_NFS();
int 	json_print_NET();
int 	json_print_IFB();

int 	json_print_PRG();
int 	json_print_PRC();
int 	json_print_PRM();
int 	json_print_PRD();
int 	json_print_PRN();
int 	json_print_PRE();

/*
** table with possible labels and the corresponding
** print-function for json style output
*/
struct labeldef {
	char	*label;
	int	(*prifunc)(char *, struct sstat *, struct tstat *, int, int);
};

static struct labeldef	labeldef[] = {
	{ "CPU",	json_print_CPU },
	{ "cpu",	json_print_cpu },
	{ "CPL",	json_print_CPL },
	{ "GPU",	json_print_GPU },
	{ "MEM",	json_print_MEM },
	{ "SWP",	json_print_SWP },
	{ "PAG",	json_print_PAG },
	{ "PSI",	json_print_PSI },
	{ "LVM",	json_print_LVM },
	{ "MDD",	json_print_MDD },
	{ "DSK",	json_print_DSK },
	{ "NFM",	json_print_NFM },
	{ "NFC",	json_print_NFC },
	{ "NFS",	json_print_NFS },
	{ "NET",	json_print_NET },
	{ "IFB",	json_print_IFB },

	{ "PRG",	json_print_PRG },
	{ "PRC",	json_print_PRC },
	{ "PRM",	json_print_PRM },
	{ "PRD",	json_print_PRD },
	{ "PRN",	json_print_PRN },
	{ "PRE",	json_print_PRE },
};

static int	numlabels = sizeof labeldef/sizeof(struct labeldef);


#define OUTPUT_SOCKPATH	"/run/atopunix.sock"
#define OUTPUT_STDIO    0
#define OUTPUT_UNIXSOCK 1
#define WRITE_RETRY_DELAY 80000
static int		output = OUTPUT_STDIO;
static time_t		write_begin_ts = 0;
static suseconds_t	write_begin_usec = 0;

/*
** analyse the json-definition string that has been passed as
** argument with the flag -O, currently only supports:
** "only", "stdio" and "unixsock".
*/
int
jsondef(char *jd)
{
	/*
	** check if string passed bahind -O is not another flag
	*/
	if (!jd || *jd == '-') {
		fprintf(stderr, "flag -O should be followed by out path, \
			like 'stdio', 'only' or 'unixsock'\n");
		return 0;
	}

	if (!strcmp(jd, "stdio") || !strcmp(jd, "only")) {
		output = OUTPUT_STDIO;
		setbuf(stdout, (char *)0);
	} else if (!strcmp(jd, "unixsock")) {
		output = OUTPUT_UNIXSOCK;
	} else {
		fprintf(stderr, "flag -O should be followed by out path, \
			like 'stdio', 'only' or 'unixsock'\n");
		return 0;
	}

	return 1;
}

static inline suseconds_t
json_now_usec()
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) < 0)
		return -1;
	else
		return tv.tv_sec*1000000UL + tv.tv_usec;
}

static inline int
json_write_timeout(suseconds_t usec)
{
	time_t now;

	time(&now);
	if (now - write_begin_ts >= interval)
		return 1;

	if (json_now_usec() - write_begin_usec + usec >= interval * 1000000UL)
		return 1;

	return 0;
}

/*
** establish a new connection to a unix socket
*/
int
json_unix_sock(int reconnect)
{
	static struct sockaddr_un c_addr;
	static int json_unix_sock = -1;
	socklen_t buflen = 256 * 1024;	/* set unix sock buf as 256K */
	int flag;

	if (reconnect == 1 && json_unix_sock != -1) {
		close(json_unix_sock);
		json_unix_sock = -1;
	}

	if (reconnect == -1) {
		if (json_unix_sock != -1) {
			close(json_unix_sock);
			json_unix_sock = -1;
		}
		return -1;
	}

	if (json_unix_sock > 0)
		return json_unix_sock;

	json_unix_sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (json_unix_sock < 0) {
		return -1;
	}

	c_addr.sun_family = AF_UNIX;
	strcpy(c_addr.sun_path, OUTPUT_SOCKPATH);
	if (connect(json_unix_sock, (struct sockaddr *)&c_addr, sizeof(c_addr)) == -1) {
		close(json_unix_sock);
		json_unix_sock = -1;
		return -1;
	}


	/* just set buflen for unix sock, maybe fail, not fatal */
	setsockopt(json_unix_sock, SOL_SOCKET, SO_SNDBUF, &buflen, sizeof(buflen));

	/* this should not block the main engine */
	flag = fcntl(json_unix_sock, F_GETFL, 0);
	fcntl(json_unix_sock, F_SETFL, flag | O_NONBLOCK);

	return json_unix_sock;
}

/*
** common function to write buffer to unix socket,
** reconnect the socket if the connection is off
*/
ssize_t json_unix_sock_write(int conn_fd, char *buf, int len)
{
	ssize_t ret = 0;
	int i = 0, orig_len = len;

	if (json_write_timeout(0))
		return -ETIME;
	/*
	** Let's try 5 times to make sure the buf has been sent,
	** especially when meeting EAGAIN.
	*/
	while (len) {
		ret = write(conn_fd, buf, len);
		if (ret < 0) {
			switch (errno) {
			case EINTR:
			case EAGAIN:
				if (json_write_timeout(WRITE_RETRY_DELAY) || i > 4)
					return -ETIME;

				i++;
				usleep(WRITE_RETRY_DELAY);
				continue;
			case EBADF:
			case EPIPE:
				conn_fd = json_unix_sock(1);
				if (conn_fd < 0 || json_write_timeout(0))
					return -ETIME;

				continue;
			default:
				return -errno;
			}
		}
		len -= ret;
		buf += ret;
	}

	return orig_len;
}

/*
** produce json output for an interval
*/

char
jsonout(time_t curtime, int numsecs,
         struct devtstat *devtstat, struct sstat *sstat,
         int nexit, unsigned int noverflow, char flag)
{
	register int	i;
	char		datestr[32], timestr[32], header[256], general[256];
	int buflen = 0;
	int conn_fd = 0;
	struct tstat *tmp = devtstat->taskall;

	convdate(curtime, datestr);
	convtime(curtime, timestr);
	write_begin_ts = curtime;
	write_begin_usec = json_now_usec();
	buflen = snprintf(general, sizeof general,
		"{\"ip\": \"%s\", "
		"\"timestamp\": %ld, "
		"\"date\": \"%s\", "
		"\"time_hms\": \"%s\"",
		utsname.nodename,
		curtime,
		datestr,
		timestr
		);

	/* Replace " with # in case json can not parse this out */
	for (int k = 0; k < devtstat->ntaskall; k++, (tmp)++) {
		for (int j = 0; j < strlen((tmp)->gen.name); j++)
			if ((tmp)->gen.name[j] == '\"') {
				(tmp)->gen.name[j] = '#';
			}
		for (int j = 0; j < strlen((tmp)->gen.cmdline); j++)
			if ((tmp)->gen.cmdline[j] == '\"') {
				(tmp)->gen.cmdline[j] = '#';
			}
	}

	if (output == OUTPUT_STDIO) {
		printf("%s", general);
	} else if (output == OUTPUT_UNIXSOCK) {
		conn_fd = json_unix_sock(0);
		if (conn_fd < 0)
			return '\0';

		if (json_unix_sock_write(conn_fd, general, buflen) < 0)
			return '\0';

	} else {
		fprintf(stderr, "unknow json output path\n");
		return '\0';
	}

	/*
	** iterate all labels defined in labeldef[]
	*/
	for (i=0; i < numlabels; i++) {
		/* prepare generic columns */
		snprintf(header, sizeof header, "\"%s\"",
			labeldef[i].label);
		/* call all print-functions */
		if ( (labeldef[i].prifunc)(header, sstat, devtstat->taskall,
				      devtstat->ntaskall, conn_fd) < 0 ) {
			json_unix_sock(-1);
			break;
		}

	}

	return '\0';
}

/*
** print functions for system-level statistics
*/
void
json_calc_freqscale(count_t maxfreq, count_t cnt, count_t ticks,
               count_t *freq, int *freqperc)
{
	// if ticks != 0,do full calcs
	if (maxfreq && ticks) {
		*freq=cnt/ticks;
		*freqperc=100* *freq / maxfreq;
	} else if (maxfreq) { // max frequency is known so % can be calculated
		*freq=cnt;
		*freqperc=100*cnt/maxfreq;
	} else if (cnt) {   // no max known, set % to 100
		*freq=cnt;
		*freqperc=100;
	} else {            // nothing is known: set freq to 0, % to 100
		*freq=0;
		*freqperc=100;
	}
}


int
json_print_CPU(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	count_t maxfreq=0;
	count_t cnt=0;
	count_t ticks=0;
	count_t freq;
	int freqperc;
	int i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	// calculate average clock frequency
	for (i=0; i < ss->cpu.nrcpu; i++) {
		cnt    += ss->cpu.cpu[i].freqcnt.cnt;
		ticks  += ss->cpu.cpu[i].freqcnt.ticks;
	}
	maxfreq = ss->cpu.cpu[0].freqcnt.maxfreq;
	json_calc_freqscale(maxfreq, cnt, ticks, &freq, &freqperc);

	if (ss->cpu.all.instr == 1) {
		ss->cpu.all.instr = 0;
		ss->cpu.all.cycle = 0;
	}

	buflen = snprintf(tmp, len, ", %s: {"
		"\"hertz\": %u, "
		"\"nrcpu\": %lld, "
		"\"stime\": %lld, "
		"\"utime\": %lld, "
		"\"ntime\": %lld, "
		"\"itime\": %lld, "
		"\"wtime\": %lld, "
		"\"Itime\": %lld, "
		"\"Stime\": %lld, "
		"\"steal\": %lld, "
		"\"guest\": %lld, "
		"\"freq\": %lld, "
		"\"freqperc\": %d, "
		"\"instr\": %lld, "
		"\"cycle\": %lld}",
		hp,
		hertz,
		ss->cpu.nrcpu,
		ss->cpu.all.stime,
		ss->cpu.all.utime,
		ss->cpu.all.ntime,
		ss->cpu.all.itime,
		ss->cpu.all.wtime,
		ss->cpu.all.Itime,
		ss->cpu.all.Stime,
		ss->cpu.all.steal,
		ss->cpu.all.guest,
		freq,
		freqperc,
		ss->cpu.all.instr,
		ss->cpu.all.cycle
		);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_cpu(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int i;
	count_t maxfreq=0;
	count_t cnt=0;
	count_t ticks=0;
	count_t freq;
	int freqperc;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; i < ss->cpu.nrcpu; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		cnt    = ss->cpu.cpu[i].freqcnt.cnt;
		ticks  = ss->cpu.cpu[i].freqcnt.ticks;
		maxfreq= ss->cpu.cpu[0].freqcnt.maxfreq;

		json_calc_freqscale(maxfreq, cnt, ticks, &freq, &freqperc);

		buflen = snprintf(tmp, len, "{\"cpuid\": %d, "
			"\"stime\": %lld, "
			"\"utime\": %lld, "
			"\"ntime\": %lld, "
			"\"itime\": %lld, "
			"\"wtime\": %lld, "
			"\"Itime\": %lld, "
			"\"Stime\": %lld, "
			"\"steal\": %lld, "
			"\"guest\": %lld, "
			"\"freq\": %lld, "
			"\"freqperc\": %d, "
			"\"instr\": %lld, "
			"\"cycle\": %lld}",
			i,
			ss->cpu.cpu[i].stime,
			ss->cpu.cpu[i].utime,
			ss->cpu.cpu[i].ntime,
			ss->cpu.cpu[i].itime,
			ss->cpu.cpu[i].wtime,
			ss->cpu.cpu[i].Itime,
			ss->cpu.cpu[i].Stime,
			ss->cpu.cpu[i].steal,
			ss->cpu.cpu[i].guest,
			freq,
			freqperc,
			ss->cpu.cpu[i].instr,
			ss->cpu.cpu[i].cycle
			);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_CPL(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	int buflen = 0;
	int len = 256;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", %s: {"
		"\"lavg1\": %.2f, "
		"\"lavg5\": %.2f, "
		"\"lavg15\": %.2f, "
		"\"csw\": %lld, "
		"\"devint\": %lld}",
		hp,
		ss->cpu.lavg1,
		ss->cpu.lavg5,
		ss->cpu.lavg15,
		ss->cpu.csw,
		ss->cpu.devint);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_GPU(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	int	i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; i < ss->gpu.nrgpus; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"gpuid\": %d, "
			"\"busid\": \"%.19s\", "
			"\"type\": \"%.19s\", "
			"\"gpupercnow\": %d, "
			"\"mempercnow\": %d, "
			"\"memtotnow\": %lld, "
			"\"memusenow\": %lld, "
			"\"samples\": %lld, "
			"\"gpuperccum\": %lld, "
			"\"memperccum\": %lld, "
			"\"memusecum\": %lld}",
			i,
			ss->gpu.gpu[i].busid,
			ss->gpu.gpu[i].type,
			ss->gpu.gpu[i].gpupercnow,
			ss->gpu.gpu[i].mempercnow,
			ss->gpu.gpu[i].memtotnow,
			ss->gpu.gpu[i].memusenow,
			ss->gpu.gpu[i].samples,
			ss->gpu.gpu[i].gpuperccum,
			ss->gpu.gpu[i].memperccum,
			ss->gpu.gpu[i].memusecum);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_MEM(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", %s: {"
		"\"physmem\": %lld, "
		"\"freemem\": %lld, "
		"\"cachemem\": %lld, "
		"\"buffermem\": %lld, "
		"\"slabmem\": %lld, "
		"\"cachedrt\": %lld, "
		"\"slabreclaim\": %lld, "
		"\"vmwballoon\": %lld, "
		"\"shmem\": %lld, "
		"\"shmrss\": %lld, "
		"\"shmswp\": %lld, "
		"\"hugepagesz\": %lld, "
		"\"tothugepage\": %lld, "
		"\"freehugepage\": %lld}",
		hp,
		ss->mem.physmem,
		ss->mem.freemem,
		ss->mem.cachemem,
		ss->mem.buffermem,
		ss->mem.slabmem,
		ss->mem.cachedrt,
		ss->mem.slabreclaim,
		ss->mem.vmwballoon,
		ss->mem.shmem,
		ss->mem.shmrss,
		ss->mem.shmswp,
		ss->mem.hugepagesz,
		ss->mem.tothugepage,
		ss->mem.freehugepage);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_SWP(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", %s: {"
		"\"totswap\": %lld, "
		"\"freeswap\": %lld, "
		"\"committed\": %lld, "
		"\"commitlim\": %lld}",
		hp,
		ss->mem.totswap,
		ss->mem.freeswap,
		ss->mem.committed,
		ss->mem.commitlim);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_PAG(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", %s: {"
		"\"pgscans\": %lld, "
		"\"allocstall\": %lld, "
		"\"swins\": %lld, "
		"\"swouts\": %lld}",
		hp,
		ss->mem.pgscans,
		ss->mem.allocstall,
		ss->mem.swins,
		ss->mem.swouts);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_PSI(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	if ( !(ss->psi.present) )
		return 0;

	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", %s: {"
		"\"psi\": \"%c\", "
		"\"CPU_some_avg10\": %.1f, "
		"\"CPU_some_avg60\": %.1f, "
		"\"CPU_some_avg300\": %.1f, "
		"\"CPU_some_avg_total\": %llu, "
		"\"mem_some_avg10\": %.1f, "
		"\"mem_some_avg60\": %.1f, "
		"\"mem_some_avg300\": %.1f, "
		"\"mem_some_avg_total\": %llu, "
		"\"mem_full_avg10\": %.1f, "
		"\"mem_full_avg60\": %.1f, "
		"\"mem_full_avg300\": %.1f, "
		"\"mem_full_avg_total\": %llu, "
		"\"io_some_avg10\": %.1f, "
		"\"io_some_avg60\": %.1f, "
		"\"io_some_avg300\": %.1f, "
		"\"io_some_avg_total\": %llu, "
		"\"io_full_avg10\": %.1f, "
		"\"io_full_avg60\": %.1f, "
		"\"io_full_avg300\": %.1f, "
		"\"io_full_avg_total\": %llu}",
		hp, ss->psi.present ? 'y' : 'n',
		ss->psi.cpusome.avg10, ss->psi.cpusome.avg60,
		ss->psi.cpusome.avg300, ss->psi.cpusome.total,
		ss->psi.memsome.avg10, ss->psi.memsome.avg60,
		ss->psi.memsome.avg300, ss->psi.memsome.total,
		ss->psi.memfull.avg10, ss->psi.memfull.avg60,
		ss->psi.memfull.avg300, ss->psi.memfull.total,
		ss->psi.iosome.avg10, ss->psi.iosome.avg60,
		ss->psi.iosome.avg300, ss->psi.iosome.total,
		ss->psi.iofull.avg10, ss->psi.iofull.avg60,
		ss->psi.iofull.avg300, ss->psi.iofull.total);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_LVM(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int	i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; ss->dsk.lvm[i].name[0]; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"lvmname\": \"%.19s\", "
			"\"io_ms\": %lld, "
			"\"nread\": %lld, "
			"\"nrsect\": %lld, "
			"\"nwrite\": %lld, "
			"\"nwsect\": %lld}",
			ss->dsk.lvm[i].name,
			ss->dsk.lvm[i].io_ms,
			ss->dsk.lvm[i].nread,
			ss->dsk.lvm[i].nrsect,
			ss->dsk.lvm[i].nwrite,
			ss->dsk.lvm[i].nwsect);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_MDD(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int	i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; ss->dsk.mdd[i].name[0]; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"mddname\": \"%.19s\", "
			"\"io_ms\": %lld, "
			"\"nread\": %lld, "
			"\"nrsect\": %lld, "
			"\"nwrite\": %lld, "
			"\"nwsect\": %lld}",
			ss->dsk.mdd[i].name,
			ss->dsk.mdd[i].io_ms,
			ss->dsk.mdd[i].nread,
			ss->dsk.mdd[i].nrsect,
			ss->dsk.mdd[i].nwrite,
			ss->dsk.mdd[i].nwsect);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_DSK(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int	i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; ss->dsk.dsk[i].name[0]; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"dskname\": \"%.19s\", "
			"\"io_ms\": %lld, "
			"\"nread\": %lld, "
			"\"nrsect\": %lld, "
			"\"nwrite\": %lld, "
			"\"nwsect\": %lld, "
			"\"avque\": %lld}",
			ss->dsk.dsk[i].name,
			ss->dsk.dsk[i].io_ms,
			ss->dsk.dsk[i].nread,
			ss->dsk.dsk[i].nrsect,
			ss->dsk.dsk[i].nwrite,
			ss->dsk.dsk[i].nwsect,
			ss->dsk.dsk[i].avque);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_NFM(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int	i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; i < ss->nfs.nfsmounts.nrmounts; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"mountdev\": \"%.19s\", "
			"\"bytestotread\": %lld, "
			"\"bytestotwrite\": %lld, "
			"\"bytesread\": %lld, "
			"\"byteswrite\": %lld, "
			"\"bytes_directIO_read\": %lld, "
			"\"bytes_directIO_write\": %lld, "
			"\"pages_mmio_read\": %lld, "
			"\"pages_mmio_write\": %lld}",
			ss->nfs.nfsmounts.nfsmnt[i].mountdev,
			ss->nfs.nfsmounts.nfsmnt[i].bytestotread,
			ss->nfs.nfsmounts.nfsmnt[i].bytestotwrite,
			ss->nfs.nfsmounts.nfsmnt[i].bytesread,
			ss->nfs.nfsmounts.nfsmnt[i].byteswrite,
			ss->nfs.nfsmounts.nfsmnt[i].bytesdread,
			ss->nfs.nfsmounts.nfsmnt[i].bytesdwrite,
			ss->nfs.nfsmounts.nfsmnt[i].pagesmread,
			ss->nfs.nfsmounts.nfsmnt[i].pagesmwrite);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_NFC(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	int buflen = 0;
	int len = 256;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", %s: {"
		"\"rpccnt\": %lld, "
		"\"rpcread\": %lld, "
		"\"rpcwrite\": %lld, "
		"\"rpcretrans\": %lld, "
		"\"rpcautrefresh\": %lld}",
		hp,
		ss->nfs.client.rpccnt,
		ss->nfs.client.rpcread,
		ss->nfs.client.rpcwrite,
		ss->nfs.client.rpcretrans,
		ss->nfs.client.rpcautrefresh);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_NFS(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", %s: {"
		"\"rpccnt\": %lld, "
		"\"rpcread\": %lld, "
		"\"rpcwrite\": %lld, "
		"\"nrbytes\": %lld, "
		"\"nwbytes\": %lld, "
		"\"rpcbadfmt\": %lld, "
		"\"rpcbadaut\": %lld, "
		"\"rpcbadcln\": %lld, "
		"\"netcnt\": %lld, "
		"\"nettcpcnt\": %lld, "
		"\"netudpcnt\": %lld, "
		"\"nettcpcon\": %lld, "
		"\"rchits\": %lld, "
		"\"rcmiss\": %lld, "
		"\"rcnocache\": %lld}",
		hp,
		ss->nfs.server.rpccnt,
		ss->nfs.server.rpcread,
		ss->nfs.server.rpcwrite,
		ss->nfs.server.nrbytes,
		ss->nfs.server.nwbytes,
		ss->nfs.server.rpcbadfmt,
		ss->nfs.server.rpcbadaut,
		ss->nfs.server.rpcbadcln,
		ss->nfs.server.netcnt,
		ss->nfs.server.nettcpcnt,
		ss->nfs.server.netudpcnt,
		ss->nfs.server.nettcpcon,
		ss->nfs.server.rchits,
		ss->nfs.server.rcmiss,
		ss->nfs.server.rcnoca);

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
	else
		printf("%s", tmp);

	free(tmp);
	return ret;
}

int
json_print_NET(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int 	i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	buflen = snprintf(tmp, len, ", \"NET_GENERAL\": {"
		"\"rpacketsTCP\": %lld, "
		"\"spacketsTCP\": %lld, "
		"\"rpacketsUDP\": %lld, "
		"\"spacketsUDP\": %lld, "
		"\"rpacketsIP\": %lld, "
		"\"spacketsIP\": %lld, "
		"\"dpacketsIP\": %lld, "
		"\"fpacketsIP\": %lld, "
		"\"tcpao\": %lld, "
		"\"tcppo\": %lld, "
		"\"tcprs\": %lld, "
		"\"tcpie\": %lld, "
		"\"tcpor\": %lld, "
		"\"udpnp\": %lld, "
		"\"udpie\": %lld}",
		ss->net.tcp.InSegs,
		ss->net.tcp.OutSegs,
		ss->net.udpv4.InDatagrams +
		ss->net.udpv6.Udp6InDatagrams,
		ss->net.udpv4.OutDatagrams +
		ss->net.udpv6.Udp6OutDatagrams,
		ss->net.ipv4.InReceives  +
		ss->net.ipv6.Ip6InReceives,
		ss->net.ipv4.OutRequests +
		ss->net.ipv6.Ip6OutRequests,
		ss->net.ipv4.InDelivers +
		ss->net.ipv6.Ip6InDelivers,
		ss->net.ipv4.ForwDatagrams +
		ss->net.ipv6.Ip6OutForwDatagrams,
		ss->net.tcp.ActiveOpens,
		ss->net.tcp.PassiveOpens,
		ss->net.tcp.RetransSegs,
		ss->net.tcp.InErrs,
		ss->net.tcp.OutRsts,
		ss->net.udpv4.NoPorts,
		ss->net.udpv4.InErrors);

	if (conn_fd) {
		ret = json_unix_sock_write(conn_fd, tmp, buflen);
		if (ret < 0)
			goto out;
	} else
		printf("%s", tmp);

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; ss->intf.intf[i].name[0]; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"netdev\": \"%.19s\", "
			"\"rpack\": %lld, "
			"\"rbyte\": %lld, "
			"\"spack\": %lld, "
			"\"sbyte\": %lld, "
			"\"speed\": \"%ld\", "
			"\"speedp\": \"%ld\", "
			"\"rerrs\": \"%lld\", "
			"\"serrs\": \"%lld\", "
			"\"rdrop\": \"%lld\", "
			"\"sdrop\": \"%lld\", "
			"\"duplex\": %d}",
			ss->intf.intf[i].name,
			ss->intf.intf[i].rpack,
			ss->intf.intf[i].rbyte,
			ss->intf.intf[i].spack,
			ss->intf.intf[i].sbyte,
			ss->intf.intf[i].speed,
			ss->intf.intf[i].speedp,
			ss->intf.intf[i].rerrs,
			ss->intf.intf[i].serrs,
			ss->intf.intf[i].rdrop,
			ss->intf.intf[i].sdrop,
			ss->intf.intf[i].duplex);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_IFB(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int 	i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; i < ss->ifb.nrports; i++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"ifbname\": \"%.19s\", "
			"\"port_id\": \"%hd\", "
			"\"lane_id\": \"%hd\", "
			"\"max_rate\": %lld, "
			"\"rcvb\": %lld, "
			"\"sndb\": %lld, "
			"\"rcvp\": %lld, "
			"\"sndp\": %lld}",
			ss->ifb.ifb[i].ibname,
			ss->ifb.ifb[i].portnr,
			ss->ifb.ifb[i].lanes,
			ss->ifb.ifb[i].rate,
			ss->ifb.ifb[i].rcvb,
			ss->ifb.ifb[i].sndb,
			ss->ifb.ifb[i].rcvp,
			ss->ifb.ifb[i].sndp);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}


/*
** print functions for process-level statistics
*/
int
json_print_PRG(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int i;
	int buflen = 0;
	int len = 1024;
	char *tmp;
	int ret = 0;
	char ruidbuf[9], euidbuf[9];
	static char st[3];
	struct passwd *pwd;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			return ret;
	} else
		printf(", %s: [", hp);

	tmp = malloc(len * sizeof(char));
	for (i=0; i < nact; i++, ps++) {
		// Filter out pid==tgid and isproc=n, as it has the same value with pid==tgid and isproc=y
		if (ps->gen.tgid == ps->gen.pid && !ps->gen.isproc)
			continue;

		if (ps->gen.excode & ~(INT_MAX))
			st[0] = 'N';
		else
			st[0] = '-';
		if (ps->gen.excode & 0xff)      // killed by signal?
			if (ps->gen.excode & 0x80)
				st[1] = 'C';
			else
				st[1] = 'S';
		else
			st[1] = 'E';

		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}

		/* let's show the straightforward username instead of implicit userid */
		if ( (pwd = getpwuid(ps->gen.ruid)) )
			sprintf(ruidbuf, "%8.8s", pwd->pw_name);
		else
			snprintf(ruidbuf, sizeof(ruidbuf), "%8.8d", ps->gen.ruid);
		if ( (pwd = getpwuid(ps->gen.euid)) )
			sprintf(euidbuf, "%8.8s", pwd->pw_name);
		else
			snprintf(euidbuf, sizeof(euidbuf), "%8.8d", ps->gen.euid);

		buflen = snprintf(tmp, len, "{\"pid\": %d, "
			"\"name\": \"(%.19s)\", "
			"\"state\": \"%c\", "
			"\"ruid\": \"%s\", "
			"\"rgid\": %d, "
			"\"tgid\": %d, "
			"\"nthr\": %d, "
			"\"st\": \"%s\", "
			"\"btime\": \"%ld\", "
			"\"cmdline\": \"(%.50s)\", "
			"\"ppid\": %d, "
			"\"nthrrun\": %d, "
			"\"nthrslpi\": %d, "
			"\"nthrslpu\": %d, "
			"\"euid\": \"%s\", "
			"\"egid\": %d, "
			"\"suid\": %d, "
			"\"sgid\": %d, "
			"\"fsuid\": %d, "
			"\"fsgid\": %d, "
			"\"elaps\": \"%ld\", "
			"\"isproc\": \"%c\", "
			"\"vpid\": \"%d\", "
			"\"ctid\": \"%d\", "
			"\"cid\": \"%.19s\"}",
			ps->gen.pid,
			ps->gen.name,
			ps->gen.state,
			ruidbuf,
			ps->gen.rgid,
			ps->gen.tgid,
			ps->gen.nthr,
			st,
			ps->gen.btime,
			ps->gen.cmdline[0] ? ps->gen.cmdline : ps->gen.name,
			ps->gen.ppid,
			ps->gen.nthrrun,
			ps->gen.nthrslpi,
			ps->gen.nthrslpu,
			euidbuf,
			ps->gen.egid,
			ps->gen.suid,
			ps->gen.sgid,
			ps->gen.fsuid,
			ps->gen.fsgid,
			ps->gen.elaps,
			ps->gen.isproc ? 'y':'n',
			ps->gen.vpid,
			ps->gen.ctid,
			ps->gen.container[0] ? ps->gen.container:"-");
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_PRC(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; i < nact; i++, ps++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"pid\": %d, "
			"\"utime\": %lld, "
			"\"stime\": %lld, "
			"\"nice\": %d, "
			"\"priority\": %d, "
			"\"current_cpu\": %d, "
			"\"sleep_average\": %d}",
			ps->gen.pid,
			ps->cpu.utime,
			ps->cpu.stime,
			ps->cpu.nice,
			ps->cpu.prio,
			ps->cpu.curcpu,
			ps->cpu.sleepavg);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_PRM(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; i < nact; i++, ps++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"pid\": %d, "
			"\"vmem\": %lld, "
			"\"rmem\": %lld, "
			"\"vexec\": %lld, "
			"\"vgrow\": %lld, "
			"\"rgrow\": %lld, "
			"\"minflt\": %lld, "
			"\"majflt\": %lld, "
			"\"vlibs\": %lld, "
			"\"vdata\": %lld, "
			"\"vstack\": %lld, "
			"\"vswap\": %lld, "
			"\"pmem\": %lld}",
			ps->gen.pid,
			ps->mem.vmem,
			ps->mem.rmem,
			ps->mem.vexec,
			ps->mem.vgrow,
			ps->mem.rgrow,
			ps->mem.minflt,
			ps->mem.majflt,
			ps->mem.vlibs,
			ps->mem.vdata,
			ps->mem.vstack,
			ps->mem.vswap,
			ps->mem.pmem == (unsigned long long)-1LL ?
			0:ps->mem.pmem);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_PRD(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"pid\": %d, "
			"\"rio\": %lld, "
			"\"rsz\": %lld, "
			"\"wio\": %lld, "
			"\"wsz\": %lld, "
			"\"cwsz\": %lld}",
			ps->gen.pid,
			ps->dsk.rio, ps->dsk.rsz,
			ps->dsk.wio, ps->dsk.wsz, ps->dsk.cwsz);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_PRN(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	if ( !(supportflags & NETATOP) )
		return 0;

	register int i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i = 0; i < nact; i++, ps++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"pid\": %d, "
			"\"tcpsnd\": \"%lld\", "
			"\"tcpssz\": \"%lld\", "
			"\"tcprcv\": \"%lld\", "
			"\"tcprsz\": \"%lld\", "
			"\"udpsnd\": \"%lld\", "
			"\"udpssz\": \"%lld\", "
			"\"udprcv\": \"%lld\", "
			"\"udprsz\": \"%lld\"}",
			ps->gen.pid,
			ps->net.tcpsnd, ps->net.tcpssz,
			ps->net.tcprcv, ps->net.tcprsz,
			ps->net.udpsnd, ps->net.udpssz,
			ps->net.udprcv, ps->net.udprsz);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd)
		ret = json_unix_sock_write(conn_fd, "]", 1);
	else
		printf("]");

out:
	free(tmp);
	return ret;
}

int
json_print_PRE(char *hp, struct sstat *ss, struct tstat *ps, int nact, int conn_fd)
{
	register int i;
	int buflen = 0;
	int len = 1024;
	char *tmp = malloc(len * sizeof(char));
	int ret = 0;

	if (conn_fd) {
		char *br = malloc(10 * sizeof(char));
		sprintf(br, ", %s: [", hp);
		ret = json_unix_sock_write(conn_fd, br, strlen(br));
		free(br);
		if (ret < 0)
			goto out;
	} else
		printf(", %s: [", hp);

	for (i=0; i < nact; i++, ps++) {
		if (i > 0) {
			if (conn_fd) {
				ret = json_unix_sock_write(conn_fd, ", ", 2);
				if (ret < 0)
					goto out;
			} else
				printf(", ");
		}
		buflen = snprintf(tmp, len, "{\"pid\": %d, "
			"\"gpu_state\": \"%c\", "
			"\"gpu_used\": %d, "
			"\"gpu_used_bitlist\": \"%x\", "
			"\"gpu_busy_perctg\": %d, "
			"\"mem_busy_perctg\": %d, "
			"\"mem_now\": %lld, "
			"\"cumulative_mem_occupation\": %lld, "
			"\"gpu_sample_nums\": %lld}",
			ps->gen.pid,
			ps->gpu.state == '\0' ? 'N':ps->gpu.state,
			ps->gpu.nrgpus,
			ps->gpu.gpulist,
			ps->gpu.gpubusy,
			ps->gpu.membusy,
			ps->gpu.memnow,
			ps->gpu.memcum,
			ps->gpu.sample);
		if (conn_fd) {
			ret = json_unix_sock_write(conn_fd, tmp, buflen);
			if (ret < 0)
				goto out;
		} else
			printf("%s", tmp);
	}

	if (conn_fd) {
		ret = json_unix_sock_write(conn_fd, "]}\n", 3);
        } else
		printf("]}\n");

out:
	free(tmp);
	return ret;
}
