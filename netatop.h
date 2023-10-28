/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of 
** the system on system-level as well as process-level.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        September 2002
** --------------------------------------------------------------------------
** Copyright (C) 2000-2010 Gerlof Langeveld
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

#ifndef __NETATOP__
#define __NETATOP__

#define	COMLEN	16

struct taskcount {
	unsigned long long	tcpsndpacks;
	unsigned long long	tcpsndbytes;
	unsigned long long	tcprcvpacks;
	unsigned long long	tcprcvbytes;

	unsigned long long	udpsndpacks;
	unsigned long long	udpsndbytes;
	unsigned long long	udprcvpacks;
	unsigned long long	udprcvbytes;

	/* space for future extensions */
};

struct netpertask {
	pid_t			id;	// tgid or tid (depending on command)
	unsigned long		btime;
	char			command[COMLEN];

	struct taskcount	tc;
};


/*
** getsocktop commands
*/
#define NETATOP_BASE_CTL   	15661

// just probe if the netatop module is active
#define NETATOP_PROBE		(NETATOP_BASE_CTL)

// force garbage collection to make finished processes available
#define NETATOP_FORCE_GC	(NETATOP_BASE_CTL+1)

// wait until all finished processes are read (blocks until done)
#define NETATOP_EMPTY_EXIT	(NETATOP_BASE_CTL+2)

// get info for finished process (blocks until available)
#define NETATOP_GETCNT_EXIT	(NETATOP_BASE_CTL+3)

// get counters for thread group (i.e. process):  input is 'id' (pid)
#define NETATOP_GETCNT_TGID	(NETATOP_BASE_CTL+4)

// get counters for thread:  input is 'id' (tid)
#define NETATOP_GETCNT_PID 	(NETATOP_BASE_CTL+5)

#define NETATOPBPF_SOCKET "/run/netatop-bpf-socket"

#endif
