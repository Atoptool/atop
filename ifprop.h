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

#ifndef __IFPROP__
#define __IFPROP__

struct ifprop	{
	char		type;		/* type: 'e' - ethernet		*/
					/*       'w' - wireless  	*/
					/*       'v' - virtual  	*/
	char		name[31];	/* name of interface  		*/
	long int	speed;		/* in megabits per second	*/
	char		fullduplex;	/* boolean			*/

	struct ifprop	*next;		/* next in hash list		*/
};

int 	getifprop(struct ifprop *);
void 	initifprop(void);

#endif
