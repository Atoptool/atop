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

#ifndef __ATOPACCTD__
#define __ATOPACCTD__

/*
** keys to access the semaphores
*/
#define PACCTPUBKEY	1071980		// # clients using shadow files
#define PACCTPRVKEY	(PACCTPUBKEY-1)	// # atopacctd daemons busy (max. 1)

/*
** name of the PID file
*/
#define	PIDFILE		"/run/atopacctd.pid"

/*
** directory containing the source accounting file and
** the subdirectory (PACCTSHADOWD) containing the shadow file(s)
** this directory can be overruled by a command line parameter (atopacctd)
** or by a keyword in the /etc/atoprc file (atop)
*/
#define	PACCTDIR	"/run"

/*
** accounting file (source file to which kernel writes records)
*/
#define PACCTORIG	"pacct_source"		// file name in PACCTDIR

#define MAXORIGSZ	(1024*1024)		// maximum size of source file

/*
** file and directory names for shadow files
*/
#define PACCTSHADOWD	"pacct_shadow.d"	// directory name in PACCTDIR
#define PACCTSHADOWF	"%s/%s/%010ld.paf"	// file name of shadow file
#define PACCTSHADOWC	"current"		// file containining current
						// sequence and MAXSHADOWREC

#define MAXSHADOWREC	10000 	// number of accounting records per shadow file

#endif
