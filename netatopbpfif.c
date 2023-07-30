/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains functions to interface with the netatop-bpf 
** in the kernel. This keeps track of network activity per process 
** and thread, and exited process.
** ================================================================
** Author:      Ting Liu
** E-mail:      liuting.0xffff@bytedance.com
** Date:        2023
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <zlib.h>
#include <sys/mman.h>
#include <glib.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "atop.h"
#include "photoproc.h"
#include "netatop.h"

static int		netsock   = -1;

static void	fill_networkcnt(struct tstat *, struct tstat *,
        	                struct taskcount *);
void my_handler(int);

int len;
struct sockaddr_un    un;
/*
** create a UNIX domain stream socket and 
** connect the netatop-bpf userspace program
*/
void
netatop_bpf_ipopen(void)
{
    char *name = NETATOPBPF_SOCKET;
    
	/* create a UNIX domain stream socket */
    if((netsock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return;

    /* fill socket address structure with server's address */
    memset(&un, 0, sizeof(un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, name);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(name);
    
    if(connect(netsock, (struct sockaddr *)&un, len) < 0)
    {
        close(netsock);
		return;
    }
	supportflags |= NETATOPBPF;
	return;
}
/*
** check if at this moment the netatop-bpf is loaded
*/
void
netatop_bpf_probe(void)
{
	/*
 	** check if netsock is not connect.
	** if not, try to connect again.
	*/
	if (!(supportflags & NETATOPBPF)) {
		char *name = NETATOPBPF_SOCKET;
		if (!access(name,F_OK)) {
			netatop_bpf_ipopen();
		}
	} 
}

void my_handler (int param)
{
	supportflags &= ~NETATOPBPF;
	close(netsock);
	netsock = -1;
}

/* the NET hash function */
GHashTable *ghash_net;

void
netatop_bpf_gettask()
{
	int ret;
	struct netpertask	npt;

	ghash_net = g_hash_table_new_full(g_int_hash, g_int_equal, g_free, g_free);

	/*
 	** get statistics of this process/thread
	*/
	signal(SIGPIPE, my_handler);
	if (send(netsock, &npt, sizeof(npt), 0) < 0) {
		supportflags &= ~NETATOPBPF;
		close(netsock);
		netsock = -1;
		return;
	}
	while ((ret = recv(netsock, &npt, sizeof(npt), 0)) > 0) {
		if (npt.id == 0) {
			break;
		}
		gint *key = g_new(gint, 1);
		*key  = npt.id;
		struct taskcount *value = g_new(struct taskcount, 1);
		*value = npt.tc;
		g_hash_table_insert(ghash_net, key, value);
	}
	if (ret <= 0) {
		supportflags &= ~NETATOPBPF;
		close(netsock);
		netsock = -1;
		return ;
	}
}

/*
** search for relevant exited network task and
** update counters in tstat struct
*/
void
netatop_bpf_exitfind(unsigned long key, struct tstat *dev, struct tstat *pre)
{
	struct taskcount *tc = g_hash_table_lookup(ghash_net, &key);
	/*
	** correct PID found
	*/
	if (tc) {
		if (tc->tcpsndpacks < pre->net.tcpsnd 	||
			tc->tcpsndbytes < pre->net.tcpssz 	||
			tc->tcprcvpacks < pre->net.tcprcv 	||
			tc->tcprcvbytes < pre->net.tcprsz 	||
			tc->udpsndpacks < pre->net.udpsnd 	||
			tc->udpsndbytes < pre->net.udpssz 	||
			tc->udprcvpacks < pre->net.udprcv 	||
			tc->udprcvbytes < pre->net.udprsz 	  )
				return;
		fill_networkcnt(dev, pre, tc);
	}
}

static void
fill_networkcnt(struct tstat *dev, struct tstat *pre, struct taskcount *tc)
{
	dev->net.tcpsnd	= tc->tcpsndpacks - pre->net.tcpsnd;
	dev->net.tcpssz	= tc->tcpsndbytes - pre->net.tcpssz;
	dev->net.tcprcv	= tc->tcprcvpacks - pre->net.tcprcv;
	dev->net.tcprsz	= tc->tcprcvbytes - pre->net.tcprsz;
	dev->net.udpsnd	= tc->udpsndpacks - pre->net.udpsnd;
	dev->net.udpssz	= tc->udpsndbytes - pre->net.udpssz;
	dev->net.udprcv	= tc->udprcvpacks - pre->net.udprcv;
	dev->net.udprsz	= tc->udprcvbytes - pre->net.udprsz;
}
