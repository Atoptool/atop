/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
** ==========================================================================
** Author:      Fei Li & zhenwei pi
** E-mail:      lifei.shirley@bytedance.com, pizhenwei@bytedance.com
** Date:        August 2019
** --------------------------------------------------------------------------
** Copyright (C) Copyright (C) 2019-2022 bytedance.com
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
int 	jsondef(char *);
char	jsonout(time_t, int,
                 struct devtstat *, struct sstat *,
		 struct cgchainer *, int, int,
                 int, unsigned int, char);
