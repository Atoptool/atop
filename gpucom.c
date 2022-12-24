/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains functions to interface with the atopgpud
** daemon that maintains statistics about the processor and memory
** utilization of the GPUs.
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Initial:     April/August 2018
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>

#include "atop.h"
#include "photosyst.h"
#include "photoproc.h"
#include "gpucom.h"

#define	DUMMY		' '
#define	GPUDELIM	'@'
#define	PIDDELIM	'#'

#define	GPUDPORT	59123

static void	gputype_parse(char *);

static void	gpustat_parse(int, char *, int,
		                      struct pergpu *, struct gpupidstat *);
static void	gpuparse(int, char *, struct pergpu *);
static void	pidparse(int, char *, struct gpupidstat *);
static int	rcvuntil(int, char *, int);

static int	actsock = -1;

static int	numgpus;
static char	**gpubusid;	// array with char* to busid strings
static char	**gputypes;	// array with char* to type strings
static char	*gputasks;	// array with chars with tasksupport booleans

/*
** Open TCP connection to port of atopgpud and
** obtain type information of every GPU.
**
** Return value:
**	number of GPUs
*/
int
gpud_init(void)
{
	struct sockaddr_in	name;
	socklen_t		namelen = sizeof name;
	char			typereq[] = {'T', APIVERSION};
	uint32_t		prelude;
	char			*buf;
	int			version, length;

	struct timeval		rcvtimeout = {2, 0};	// 2 seconds

	/*
	** get local socket
	*/
	if ( (actsock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket creation");
		return 0;
	}

	/*
	** connect to server port
	*/
	memset(&name, 0, sizeof name);
	name.sin_family      = AF_INET;
	name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	name.sin_port        = htons(GPUDPORT);

	if (connect(actsock, (struct sockaddr *)&name, namelen) == -1)
		goto close_and_return;

	/*
	** set receive timeout, not to block atop forever
	** in case something fails in the commmunication
	*/
	(void) setsockopt(actsock, SOL_SOCKET, SO_RCVTIMEO,
					&rcvtimeout, sizeof rcvtimeout);

	/*
	** send request: GPU types
	*/
	if ( write(actsock, typereq, sizeof typereq) < sizeof typereq)
	{
		perror("send type request to atopgpud");
		goto close_and_return;
	}

	/*
	** receive response: GPU types
	*/
	if (rcvuntil(actsock, (char *)&prelude, sizeof prelude) == -1)
	{
		perror("receive prelude from atopgpud");
		goto close_and_return;
	}

	prelude = ntohl(prelude);	// big endian to native endianess

	version = (prelude >> 24) & 0xff;
	length  =  prelude        & 0xffffff;

	if (version != APIVERSION)
	{
		fprintf(stderr,
			"wrong API version from atopgpud: %d %d\n",
			version, APIVERSION);

		goto close_and_return;
	}
 
	if (length > 8192)	// sanity check
	{
		fprintf(stderr,
			"unexpected response length atopgpud: %d\n", length);

		goto close_and_return;
	}

	buf = malloc(length+1);
	ptrverify(buf, "Malloc failed for gpu rcvbuf\n");

	if ( rcvuntil(actsock, buf, length) == -1)
	{
		perror("receive type request from atopgpud");
		goto close_and_return;
	}

	buf[length] = '\0';

	gputype_parse(buf);

        numgpus = numgpus <= MAXGPU ? numgpus : MAXGPU;

	return numgpus;

    close_and_return:
	close(actsock);
	actsock = -1;
	return 0;
}


/*
** Transmit status request for all GPUs.
**
** Calling parameters:
** 	void
**
** Return value:
** 	0 in case of failure
** 	1 in case of success
*/
int
gpud_statrequest(void)
{
	char	statreq[] = {'S', APIVERSION};

	if (actsock == -1)
		return 0;

	if ( write(actsock, statreq, sizeof statreq) < sizeof statreq)
	{
		close(actsock);
		actsock = -1;
		return 0;
	}

	return 1;
}


/*
** Receive status response for all GPUs.
**
** Calling parameters:
** 	*ggs	pointer to allocated array of pergpu structs
** 	**gps	pointer to pointer in which addresses to gpupidstat structs
**		are returned
**		can be NULL pointer is caller is not interested in proc stats
**
** Return value:
** 	number of gpupidstat addresses (i.e. per-process info)
**	-1 in case of failure
*/
int
gpud_statresponse(int maxgpu, struct pergpu *ggs, struct gpupidstat **gps)
{
	uint32_t	prelude;
	char		*buf = NULL, *p;
	int		version, length;
	int		pids = 0;

	if (actsock == -1)
		return -1;

	/*
	** receive 4-bytes introducer:
	**	first byte:		API version
	**	next three bytes:	length of string that follows
	*/
	if ( rcvuntil(actsock, (char *)&prelude, sizeof prelude) == -1)
	{
		perror("receive 4-byte prelude from atopgpud");
		goto close_and_return;
	}

	prelude = ntohl(prelude);	// big endian to native endianess

	version = (prelude >> 24) & 0xff;
	length  =  prelude        & 0xffffff;

	if (version != APIVERSION)
	{
		fprintf(stderr,
			"wrong API version from atopgpud: %d %d\n",
			version, APIVERSION);

		goto close_and_return;
	}
 
	if (length > 8192)	// sanity check
	{
		fprintf(stderr,
			"unexpected response length atopgpud: %d\n", length);

		goto close_and_return;
	}

	buf = malloc(length+1);
	ptrverify(buf, "Malloc failed for gpu rcvbuf\n");

	/*
	** receive statistics string
	*/
	if ( rcvuntil(actsock, buf, length) == -1)
	{
		perror("receive stats string from atopgpud");
		goto close_and_return;
	}

	*(buf+length) = '\0';

	/*
	** determine number of per-process stats
	** and malloc space to parse these stats
	*/
	for (p=buf; *p; p++)
	{
		if (*p == PIDDELIM)
			pids++;
	}

	if (gps)
	{
		if (pids)
		{
			*gps = malloc(pids * sizeof(struct gpupidstat));
			ptrverify(gps, "Malloc failed for gpu pidstats\n");
			memset(*gps, 0, pids * sizeof(struct gpupidstat));
		}
		else
		{
			*gps = NULL;
		}
	}

	/*
	** parse stats string for per-gpu stats
	*/
	gpustat_parse(version, buf, maxgpu, ggs, gps ? *gps : NULL);

	free(buf);

	return pids;

    close_and_return:
	if (buf)
		free(buf);

	close(actsock);
	actsock = -1;
	return -1;
}


/*
** Receive given number of bytes from given socket
** into given buffer address
*/
static int
rcvuntil(int sock, char *buf, int size)
{
	int	remain = size, n;

	while (remain)
	{
		n = read(sock, buf, remain);

		if (n <= 0)
			return -1;

		buf 	+= n;
		remain	-= n;
	}

	return size;
}

/*
** Parse response string from server on 'T' request
**
** Store the type, busid and tasksupport of every GPU in
** static pointer tables
*/
static void
gputype_parse(char *buf)
{
	char	*p, *start, **bp, **tp, *cp;

	/*
	** determine number of GPUs
	*/
	if ( sscanf(buf, "%d@", &numgpus) != 1)
	{
		close(actsock);
		actsock = -1;
		return;
	}

	for (p=buf; *p; p++)	// search for first delimiter
	{
		if (*p == GPUDELIM)
		{
			p++;
			break;
		}
	}

	/*
	** parse GPU info and build arrays of pointers to the
	** busid strings, type strings and tasksupport strings.
	*/
	if (numgpus)			// GPUs present anyhow?
	{
		int field;

		gpubusid = bp = malloc((numgpus+1) * sizeof(char *));
		gputypes = tp = malloc((numgpus+1) * sizeof(char *));
		gputasks = cp = malloc((numgpus)   * sizeof(char  ));

		ptrverify(gpubusid, "Malloc failed for gpu busids\n");
		ptrverify(gputypes, "Malloc failed for gpu types\n");
		ptrverify(gputasks, "Malloc failed for gpu tasksup\n");

		for (field=0, start=p; ; p++)
		{
			if (*p == ' ' || *p == '\0' || *p == GPUDELIM)
			{
				switch(field)
				{
				   case 0:
					if (p-start <= MAXGPUBUS)
						*bp++ = start;
					else
						*bp++ = p - MAXGPUBUS;
					break;
				   case 1:
					if (p-start <= MAXGPUTYPE)
						*tp++ = start;
					else
						*tp++ = p - MAXGPUTYPE;
					break;
				   case 2:
					*cp++ = *start;
					break;
				}

				field++;
				start = p+1;

				if (*p == '\0')
					break;

				if (*p == GPUDELIM)
					field = 0;

				*p = '\0';
			}
		}

		*bp = NULL;
		*tp = NULL;
	}
}


/*
** Parse entire response string from server.
**
** Every series with counters on GPU level is introduced
** with a '@' delimiter.
** Every series with counters on process level is introduced
** with a '#' delimiter (last part of the GPU level data).
*/
static void
gpustat_parse(int version, char *buf, int maxgpu, 
		struct pergpu *gg, struct gpupidstat *gp)
{
	char	*p, *start, delimlast;
	int	gpunum = 0;

	/*
	** parse stats string
	*/
	for (p=start=buf, delimlast=DUMMY; gpunum <= maxgpu; p++)
	{
		char delimnow;

		if (*p != '\0' && *p != GPUDELIM && *p != PIDDELIM)
			continue;

		/*
		** next delimiter or end-of-string found
		*/
		delimnow = *p;
		*p       = 0;

 		switch (delimlast)
		{
		   case DUMMY:
			break;

		   case GPUDELIM:
			gpuparse(version, start, gg);

			strcpy(gg->type,  gputypes[gpunum]);
			strcpy(gg->busid, gpubusid[gpunum]);

			gpunum++;
			gg++;
			break;

		   case PIDDELIM:
			if (gp)
			{
				pidparse(version, start, gp);

				gp->gpu.nrgpus++;
				gp->gpu.gpulist = 1<<(gpunum-1);
				gp++;

				(gg-1)->nrprocs++;
			}
		}

		if (delimnow == 0 || *(p+1) == 0)
			break;

		start     = p+1;
		delimlast = delimnow;
	}
}


/*
** Parse GPU statistics string
*/
static void
gpuparse(int version, char *p, struct pergpu *gg)
{
	switch (version)
	{
	   case 1:
		(void) sscanf(p, "%d %d %lld %lld %lld %lld %lld %lld", 
			&(gg->gpupercnow), &(gg->mempercnow),
			&(gg->memtotnow),  &(gg->memusenow),
			&(gg->samples),    &(gg->gpuperccum),
			&(gg->memperccum), &(gg->memusecum));

		gg->nrprocs = 0;

		break;
	}
}


/*
** Parse PID statistics string
*/
static void
pidparse(int version, char *p, struct gpupidstat *gp)
{
	switch (version)
	{
	   case 1:
		(void) sscanf(p, "%c %ld %d %d %lld %lld %lld %lld",
			&(gp->gpu.state),   &(gp->pid),    
			&(gp->gpu.gpubusy), &(gp->gpu.membusy),
			&(gp->gpu.timems),
			&(gp->gpu.memnow), &(gp->gpu.memcum),
		        &(gp->gpu.sample));
		break;
	}
}


/*
** Merge the GPU per-process counters with the other
** per-process counters
*/
static int compgpupid(const void *, const void *);

void
gpumergeproc(struct tstat      *curtpres, int ntaskpres,
             struct tstat      *curpexit, int nprocexit,
             struct gpupidstat *gpuproc,  int nrgpuproc)
{
	struct gpupidstat	**gpp;
	int 			t, g, gpuleft = nrgpuproc;

	/*
 	** make pointer list for elements in gpuproc
	*/
	gpp = malloc(nrgpuproc * sizeof(struct gpupidstat *));

	if (!gpp)
		ptrverify(gpp, "Malloc failed for process list\n");

	for (g=0; g < nrgpuproc; g++)
		gpp[g] = gpuproc + g;

	/*
   	** sort the list with pointers in order of pid
	*/
	if (nrgpuproc > 1)
        	qsort(gpp, nrgpuproc, sizeof(struct gpupidstat *), compgpupid);

	/*
	** accumulate entries that contain stats from same PID
	** on different GPUs
	*/
	for (g=1; g < nrgpuproc; g++)
	{
		if (gpp[g-1]->pid == gpp[g]->pid)
		{
			struct gpupidstat *p = gpp[g-1], *q = gpp[g];

			p->gpu.nrgpus  += q->gpu.nrgpus;
			p->gpu.gpulist |= q->gpu.gpulist;

			if (p->gpu.gpubusy != -1)
				p->gpu.gpubusy += q->gpu.gpubusy;

			if (p->gpu.membusy != -1)
				p->gpu.membusy += q->gpu.membusy;

			if (p->gpu.timems != -1)
				p->gpu.timems += q->gpu.timems;

			p->gpu.memnow += q->gpu.memnow;
			p->gpu.memcum += q->gpu.memcum;
			p->gpu.sample += q->gpu.sample;

			if (nrgpuproc-g-1 > 0)
				memmove(&(gpp[g]), &(gpp[g+1]),
					(nrgpuproc-g-1) * sizeof p);

			nrgpuproc--;
			g--;
		}
	}

	/*
 	** merge gpu stats with sorted task list of active processes
	*/
	for (t=g=0; t < ntaskpres && g < nrgpuproc; t++)
	{
		if (curtpres[t].gen.isproc)
		{
			if (curtpres[t].gen.pid == gpp[g]->pid)
			{
				curtpres[t].gpu = gpp[g]->gpu;
				gpp[g++] = NULL;

				if (--gpuleft == 0 || g >= nrgpuproc)
					break;
			}

			// anyhow resync
			while ( curtpres[t].gen.pid > gpp[g]->pid)
			{
				if (++g >= nrgpuproc)
					break;
			}
		}
	}

	if (gpuleft == 0)
	{
		free(gpp);
		return;
	}

	/*
 	** compact list with pointers to remaining pids
	*/
	for (g=t=0; g < nrgpuproc; g++)
	{
		if (gpp[g] == NULL)
		{
			for (; t < nrgpuproc; t++)
			{
				if (gpp[t])
				{
					gpp[g] = gpp[t];
					gpp[t] = NULL;
					break;
				}
			}
		}
	}

	/*
 	** merge remaining gpu stats with task list of exited processes
	*/
	for (t=0; t < nprocexit && gpuleft; t++)
	{
		if (curpexit[t].gen.isproc)
		{
			for (g=0; g < gpuleft; g++)
			{
				if (gpp[g] == NULL)
					continue;

				if (curpexit[t].gen.pid == gpp[g]->pid)
				{
					curpexit[t].gpu = gpp[g]->gpu;
					gpp[g] = NULL;
					gpuleft--;
				}
			}
		}
	}

	free(gpp);
}

static int
compgpupid(const void *a, const void *b)
{
	return (*((struct gpupidstat **)a))->pid - (*((struct gpupidstat **)b))->pid;
}
