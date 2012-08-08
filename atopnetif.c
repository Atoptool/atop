/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains functions to interface with the atopnet
** module in the kernel. That module keeps track of network activity
** per process and thread.
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        August 2012
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
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include "atop.h"
#include "photoproc.h"
#include "atopnet.h"

static int	netsock = -1;

void
netmodprobe(void)
{
	struct atopnetstat	ans;
	socklen_t		socklen = sizeof ans;

	/*
	** open a socket to the IP layer
	*/
	if (netsock == -1)
		if ( (netsock = socket(PF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
			return;

	/*
	** probe if the atopnet module is active
	*/
	ans.id = 1;

        regainrootprivs();

	if (getsockopt(netsock, SOL_IP, ATOP_GETCNT_TGID, &ans, &socklen)!=0) {
		if (errno == ENOPROTOOPT || errno == EPERM) {
			supportflags &= ~ATOPNET;

        		if (! droprootprivs())
				cleanstop(42);

			return;
		}
	}

        if (! droprootprivs())
        	cleanstop(42);

	supportflags |= ATOPNET;
}

/*
** read network counters for one process (type 'g' for thread group)
** or for one thread (type 't' for thread).
*/
void
netmodfill(pid_t id, char type, struct tstat *tp)
{
	struct atopnetstat	ans;
	socklen_t		socklen = sizeof ans;
	int 			cmd = (type == 'g' ?
					ATOP_GETCNT_TGID :ATOP_GETCNT_PID);

	/*
	** if kernel module atopnet not active on this system, skip call
	*/
	if (netsock == -1 || !(supportflags & ATOPNET) ) {
		memset(&tp->net, 0, sizeof tp->net);
		return;
	}

	/*
 	** get statistics of this process/thread
	*/
	ans.id	= id;

        regainrootprivs();

	if (getsockopt(netsock, SOL_IP, cmd, &ans, &socklen) != 0) {
		memset(&tp->net, 0, sizeof tp->net);

        	if (! droprootprivs())
        		cleanstop(42);

		if (errno == ENOPROTOOPT || errno == EPERM)
		{
			supportflags &= ~ATOPNET;
			close(netsock);
			netsock = -1;
		}

		return;
	}

       	if (! droprootprivs())
       		cleanstop(42);

	/*
	** statistics available: fill counters
	*/
	tp->net.tcpsnd = ans.tc.tcpsndpacks;
	tp->net.tcprcv = ans.tc.tcprcvpacks;
	tp->net.tcpssz = ans.tc.tcpsndbytes;
	tp->net.tcprsz = ans.tc.tcprcvbytes;
	tp->net.udpsnd = ans.tc.udpsndpacks;
	tp->net.udprcv = ans.tc.udprcvpacks;
	tp->net.udpssz = ans.tc.udpsndbytes;
	tp->net.udprsz = ans.tc.udprcvbytes;
	tp->net.rawsnd = 0;
	tp->net.rawrcv = 0;
}
