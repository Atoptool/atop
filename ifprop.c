/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        January 2007
** --------------------------------------------------------------------------
** Copyright (C) 2007-2010 Gerlof Langeveld
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
** $Id: ifprop.c,v 1.5 2010/04/23 12:19:35 gerlof Exp $
** $Log: ifprop.c,v $
** Revision 1.5  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.4  2007/02/13 10:34:06  gerlof
** Removal of external declarations.
**
*/
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/in.h>

typedef __u64	u64;
typedef __u32	u32;
typedef __u16	u16;
typedef __u8	u8;
#include <linux/ethtool.h>
#include <linux/wireless.h>

#ifndef	SPEED_UNKNOWN
#define	SPEED_UNKNOWN	-1
#endif

#include "atop.h"
#include "ifprop.h"
#include "photosyst.h"

static struct ifprop	ifprops[MAXINTF];

/*
** function searches for the properties of a particular interface
** the interface name should be filled in the struct ifprop before
** calling this function
**
** return value reflects true or false
*/
int
getifprop(struct ifprop *ifp)
{
	register int	i;

	for (i=0; ifprops[i].name[0]; i++)
	{
		if (strcmp(ifp->name, ifprops[i].name) == 0)
		{
			*ifp = ifprops[i];
			return 1;
		}
	}

	ifp->type	= '?';
	ifp->speed	= 0;
	ifp->fullduplex	= 0;

	return 0;
}

/*
** function stores properties of all interfaces in a static
** table to be queried later on
**
** this function should be called with superuser privileges!
*/
void
initifprop(void)
{
	FILE 			*fp;
	char 			*cp, linebuf[2048];
	int			i=0, sockfd;
	struct ethtool_cmd 	ethcmd;
	struct ifreq	 	ifreq;
	struct iwreq	 	iwreq;

	/*
	** open /proc/net/dev to obtain all interface names and open
  	** a socket to determine the properties for each interface
	*/
	if ( (fp = fopen("/proc/net/dev", "r")) == NULL)
		return;

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		fclose(fp);
		return;
	}

	/*
	** read every name and obtain properties
	*/
	while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
	{
		/*
		** skip lines containing a '|' symbol (headers)
		*/
		if ( strchr(linebuf, '|') != NULL)
			continue;

		if ( (cp = strchr(linebuf, ':')) != NULL)
			*cp = ' ';    /* subst ':' by space */

		sscanf(linebuf, "%15s", ifprops[i].name);

		/*
		** determine properties of ethernet interface
		*/
		memset(&ifreq,  0, sizeof ifreq);
		memset(&ethcmd, 0, sizeof ethcmd);

		strncpy((void *)&ifreq.ifr_ifrn.ifrn_name, ifprops[i].name,
				sizeof ifreq.ifr_ifrn.ifrn_name-1);

		ifreq.ifr_ifru.ifru_data = (void *)&ethcmd;

		ethcmd.cmd = ETHTOOL_GSET;

		if ( ioctl(sockfd, SIOCETHTOOL, &ifreq) == 0) 
		{
			ifprops[i].type  = 'e';	// type ethernet
			ifprops[i].speed = ethtool_cmd_speed(&ethcmd);

			if (ifprops[i].speed == (u32)SPEED_UNKNOWN)
				ifprops[i].speed = 0;

			switch (ethcmd.duplex)
			{
		   	   case DUPLEX_FULL:
				ifprops[i].fullduplex	= 1;
				break;
		   	   default:
				ifprops[i].fullduplex	= 0;
			}

			if (++i >= MAXINTF-1)
				break;
			else
				continue;
		}

		/*
		** determine properties of wireless interface
		*/
		memset(&iwreq,  0, sizeof iwreq);

		strncpy(iwreq.ifr_ifrn.ifrn_name, ifprops[i].name,
				sizeof iwreq.ifr_ifrn.ifrn_name-1);

		if ( ioctl(sockfd, SIOCGIWRATE, &iwreq) == 0) 
		{
			ifprops[i].type       = 'w';	// type wireless
			ifprops[i].fullduplex = 0;

			ifprops[i].speed =
				(iwreq.u.bitrate.value + 500000) / 1000000;

			if (++i >= MAXINTF-1)
				break;
			else
				continue;
		}

		ifprops[i].type       = '?';	// type unknown
		ifprops[i].fullduplex = 0;
		ifprops[i].speed      = 0;

		if (++i >= MAXINTF-1)
			break;
	}

	close(sockfd);
	fclose(fp);
}
