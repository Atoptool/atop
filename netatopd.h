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

#ifndef __NETATOPD__
#define __NETATOPD__

#define SEMAKEY         1541961

#define NETEXITFILE     "/run/netatop.log"
#define MYMAGIC         (unsigned int) 0xfeedb0b0

struct naheader {
        u_int32_t	magic;	// magic number MYMAGIC
        u_int32_t	curseq;	// sequence number of last netpertask
        u_int16_t	hdrlen;	// length of this header
        u_int16_t	ntplen;	// length of netpertask structure
        pid_t    	mypid;	// PID of netatopd itself
};

#endif
