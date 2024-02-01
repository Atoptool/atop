/*
** ATOP - System & Process Monitor 
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
** 
** This source-file contains functions to retrieve the hostname of a
** containerized process by associating to its UTS namespace.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        August 2023 (initial)
** --------------------------------------------------------------------------
** Copyright (C) 2023 Gerlof Langeveld
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

#define _GNU_SOURCE
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <sched.h>

#include <atop.h>
#include <photoproc.h>

#define	BASEPATH	"/proc/1/ns/uts"
#define	UTSPATH		"/proc/%d/ns/uts"

static	int		mypidfd, foreignuts;

// Function that fills the host name (container/pod name) of 
// a specific process.
// When the process has a different UTS namespace then systemd,
// the process' UTS namespace will be temporarily associated with atop
// itself to retrieve the host name (i.e. container/pod name) of that process.
// To avoid too many namespace switches, atop only reassociates with its own
// UTS namespace when the resetutsname() function is called.
//
// Return value:
// 	0 -	no container/pod detected
// 	1 -	container/pod detected
//
int
getutsname(struct tstat *curtask)
{
	static char	firstcall = 1, basepath[64], basehost[32];

	int		pidfd, offset;
	ssize_t		destlen;
	char		srcpath[64], destpath[64], tmphost[70];

	// regain root privs in case of setuid root executable
	//
	regainrootprivs();

	// function initialization
	//
	if (firstcall)
	{
		firstcall = 0;

		// determine the UTS namespace of systemd and the hostname
		// related to systemd
		//
		if ( (destlen = readlink(BASEPATH, basepath, sizeof basepath)) == -1)
			goto drop_and_return;

		basepath[destlen] = '\0';	// guarantee delimeter

		if ( (pidfd = open(BASEPATH, O_RDONLY)) == -1)
			goto drop_and_return;

		if (setns(pidfd, CLONE_NEWUTS) == -1)
		{
			close(pidfd);
			goto drop_and_return;
		}

		foreignuts = 1;
		close(pidfd);

		gethostname(basehost, sizeof basehost);

		// determine the UTS namespace of atop itself
		// and open that namespace for reassociation via
		// function resetutsname()
		//
		snprintf(srcpath, sizeof srcpath, UTSPATH, getpid());

		if ( (mypidfd = open(srcpath, O_RDONLY)) == -1)
			basepath[0] = '\0';

		resetutsname();
	}

	// no root privileges?
	//
	if (!basepath[0])
		goto drop_and_return;

	// check if this pid is related to myself (atop)
	//
	if (curtask->gen.pid == getpid())
		goto drop_and_return;

	// base path is known
	// verify if this process is native, i.e. does it share the 
 	// UTS namespace of systemd
	//
	snprintf(srcpath, sizeof srcpath, UTSPATH, curtask->gen.pid);

	destlen = readlink(srcpath, destpath, sizeof destpath);
	destpath[destlen] = '\0';

	if (strcmp(basepath, destpath) == 0)	// equal?
		goto drop_and_return;

	// UTS namespace deviates from base UTS namespace
	// 
	// get hostname related to this UTS namespace by associating
	// atop to that namespace
	//
	if ( (pidfd = open(srcpath, O_RDONLY)) == -1)
		goto drop_and_return;

	if (setns(pidfd, CLONE_NEWUTS) == -1)
	{
		close(pidfd);
		goto drop_and_return;
	}

	foreignuts = 1;		// set boolean
	close(pidfd);

	gethostname(tmphost, sizeof tmphost);

	// check if the hostname is the same as the hostname of
	// systemd (PID 1), because some systemd-related processes have
	// their own UTS namespace with the same hostname
	// also the hostname 'localhost' might be used by various daemons
	// and will be skipped as well
	//
	if ( strcmp(tmphost, basehost) == 0 || strcmp(tmphost, "localhost") == 0)
		goto drop_and_return;

	// this process really seems to be container/pod related
	//
	if ( (offset = strlen(tmphost) - UTSLEN) < 0)
		offset = 0;

	strcpy(curtask->gen.utsname, tmphost+offset);	// copy last part when overflow

	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

	return 1;

    drop_and_return:
	if (! droprootprivs())
		mcleanstop(42, "failed to drop root privs\n");

        return 0;
}

// Reassociate atop with its own UTS namespace
//
void
resetutsname(void)
{
	// switch back to my own original UTS namespace
	//
	if (foreignuts)
	{
		foreignuts = 0;

		regainrootprivs();

		if (setns(mypidfd, CLONE_NEWUTS) == -1)
			;

		if (! droprootprivs())
			mcleanstop(42, "failed to drop root privs\n");
	}
}
