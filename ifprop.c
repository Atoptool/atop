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
*/
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/in.h>
#include <dirent.h>

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

static int		calcbucket(char *);
static int		getphysprop(struct ifprop *);

/*
** hash table for linked lists with *all* interfaces
** of this system (including virtual interfaces), even
** if the number of interfaces exceeds the maximum that
** is supported by atop (MAXINTF)
** when the number of interfaces in the system exceeds MAXINTF,
** preferably virtual interfaces are marked 'invalid' to ensure
** that all physical interfaces are reported
**
** the hash table is meant to be searched on interface name
*/
#define	NUMIFHASH	256	// must be power of 2!!
static struct ifprop	*ifhash[NUMIFHASH];

/*
** refresh interval of ifhash table
**
** periodic refreshing is needed because interfaces might have been
** created or removed, or the speed might have changed (e.g. with wireless)
*/
#define REFRESHTIME	60		// seconds
static time_t		lastrefreshed; 	// epoch

/*
** function that searches for the properties of a particular interface;
** the interface name should be filled in the struct ifprop before
** calling this function
**
** return value reflects true (valid interface) or false (invalid interface)
*/
int
getifprop(struct ifprop *p)
{
	register int	bucket;
	struct ifprop	*ifp;

	/*
	** search properties related to given interface name
	*/
	bucket = calcbucket(p->name);

	for (ifp=ifhash[bucket]; ifp; ifp=ifp->next)
	{
		if ( strcmp(ifp->name, p->name) == EQ )
		{
			if (ifp->type == 'i')	// invalidated interface?
				break;

			// valid interface; copy properties
			*p = *ifp;
			return 1;
		}
	}

	p->type		= '?';
	p->speed	= 0;
	p->fullduplex	= 0;

	return 0;
}

/*
** function (re)stores properties of all interfaces in a hash table
*/
void
initifprop(void)
{
	FILE 		*fp;
	char		*cp, linebuf[2048];
	struct ifprop	*ifp, *ifpsave = NULL;
	int		bucket, nrinterfaces=0, nrphysical=0;

	DIR		*dirp;
	struct dirent	*dentry;

	/*
	** verify if the interface properties have to be refreshed
	** at this moment already
	*/
	if (time(0) < lastrefreshed + REFRESHTIME)
		return;

	/*
 	** when this function has been called before, first remove
	** old entries
	*/
	if (lastrefreshed)
	{
		for (bucket=0; bucket < NUMIFHASH; bucket++)
		{
			for (ifp = ifhash[bucket]; ifp; ifp = ifpsave)
			{
				ifpsave = ifp->next;
				free(ifp);
			}
	
			ifhash[bucket] = NULL;
		}
	}

	/*
	** open /proc/net/dev and read all interface names to be able to
	** setup new entries in the hash table
	*/
	if ( (fp = fopen("/proc/net/dev", "r")) == NULL)
		return;

	while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
	{
		/*
		** skip lines containing a '|' symbol (headers)
		*/
		if ( strchr(linebuf, '|') != NULL)
			continue;

		if ( (cp = strchr(linebuf, ':')) != NULL)
			*cp = ' ';    /* subst ':' by space */

		/*
		** allocate new ifprop struct for this interface
		*/
		ifp = malloc(sizeof *ifp);

		ptrverify(ifp, "Malloc failed for ifprop struct\n");

		memset(ifp, 0, sizeof *ifp);
		sscanf(linebuf, "%30s", ifp->name);	// fill name
		ifp->type = 'i';			// initially 'invalid'

		/*
		** add ifprop struct to proper hash bucket
		*/
		bucket = calcbucket(ifp->name);

		ifp->next = ifhash[bucket];
		ifhash[bucket] = ifp;

		nrinterfaces++;
	}

	fclose(fp);

	/*
	** read /sys/devices/virtual/net/xxx to determine which
	** interfaces are virtual (xxx is subdirectory name)
	*/
	if ( (dirp = opendir("/sys/devices/virtual/net")) )
	{
		while ( (dentry = readdir(dirp)) )
		{
			if (dentry->d_name[0] == '.')
				continue;
	
			// valid name
			// search ifprop and mark it as 'virtual'
			bucket = calcbucket(dentry->d_name);

			for (ifp = ifhash[bucket]; ifp; ifp = ifp->next)
			{
				if ( strcmp(ifp->name, dentry->d_name) == EQ )
				{
					ifp->type = 'v'; // virtual interface
					break;
				}
			}
		}

		closedir(dirp);
	}


	/*
	** for physical interfaces, determine the speed and duplex mode
	*/
	for (bucket=0; bucket < NUMIFHASH; bucket++)
	{
		for (ifp=ifhash[bucket]; ifp; ifp=ifp->next)
		{
			// possible physical interface?
			if (ifp->type == 'i')
			{
				if (getphysprop(ifp))
					nrphysical++;
			}
		}
	}

	lastrefreshed = time(0);

	if (nrinterfaces < MAXINTF)
		return;

	/*
 	** when the number of interfaces exceeds the maximum,
	** invalidate the appropriate number of interfaces (preferably
	** virtual interfaces)
	*/
	for (bucket=0; bucket < NUMIFHASH && nrinterfaces >= MAXINTF; bucket++)
	{
		for (ifp=ifhash[bucket]; ifp && nrinterfaces >= MAXINTF; ifp=ifp->next)
		{
			// interface invalid already?
			if (ifp->type == 'i')
			{
				nrinterfaces--;
				continue;
			}

			// physical interface (ethernet or wireless)?
			if (ifp->type == 'e' || ifp->type == 'w')
			{
				// only invalidate when the number of physical
				// interfaces exceeds MAXINTF
				if (nrphysical >= MAXINTF)
				{
					ifp->type = 'i';

					nrphysical--;
					nrinterfaces--;
				}
				continue;
			}

			// virtual or unknown interface, invalidate anyhow
			ifp->type = 'i';
			nrinterfaces--;
		}
	}
}

static int
calcbucket(char *p)
{
	int bucket = 0;

	while (*p)
		bucket += *p++;

	return bucket & (NUMIFHASH-1);
}

/*
** function gathers the properties of a particular physical interface;
** the name of the interface should have been filled before calling
**
** return value reflects true (success) or false (unknown interface type)
*/
static int
getphysprop(struct ifprop *p)
{
	int sockfd;

#ifdef ETHTOOL_GLINKSETTINGS
	struct ethtool_link_settings 	*ethlink = NULL;	// preferred!
#endif
	struct ethtool_cmd 		ethcmd;		// deprecated	

	struct ifreq		 	ifreq;
	struct iwreq		 	iwreq;

	unsigned long			speed = 0;
	unsigned char			duplex = 0, ethernet = 0;


	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		return 0;

	/*
	** determine properties of ethernet interface
	** preferably with actual struct ethtool_link_settings,
	** otherwise with deprecated struct ethtool_cmd
	*/
	memset(&ifreq,  0, sizeof ifreq);

	strncpy((void *)&ifreq.ifr_ifrn.ifrn_name, p->name,
			sizeof ifreq.ifr_ifrn.ifrn_name-1);

#ifdef ETHTOOL_GLINKSETTINGS
	ethlink = calloc(1, sizeof *ethlink);

	ptrverify(ethlink, "Calloc failed for ethtool_link_settings\n");

	ethlink->cmd = ETHTOOL_GLINKSETTINGS;

	ifreq.ifr_ifru.ifru_data = (void *)ethlink;

	if ( ioctl(sockfd, SIOCETHTOOL, &ifreq) == 0)
	{
		if (ethlink->link_mode_masks_nwords <= 0)
		{
			/*
			** hand shaked ethlink required with added maps
			**
			** layout of link_mode_masks fields:
			** __u32 map_supported[link_mode_masks_nwords];
			** __u32 map_advertising[link_mode_masks_nwords];
			** __u32 map_lp_advertising[link_mode_masks_nwords];
			*/
			ethlink->link_mode_masks_nwords = -ethlink->link_mode_masks_nwords;

			ethlink = realloc(ethlink, sizeof *ethlink +
			              3 * sizeof(__u32) * ethlink->link_mode_masks_nwords);

			ptrverify(ethlink, "Realloc failed for ethtool_link_settings\n");

			ifreq.ifr_ifru.ifru_data = (void *)ethlink; // might have changed

			if ( ioctl(sockfd, SIOCETHTOOL, &ifreq) != 0 )
			{
				close(sockfd);
				free(ethlink);
				return 0;
			}
		}

		ethernet = 1;
		speed    = ethlink->speed;
		duplex   = ethlink->duplex;
	}
	else
#endif
	{
		memset(&ethcmd, 0, sizeof ethcmd);

		ethcmd.cmd               = ETHTOOL_GSET;
		ifreq.ifr_ifru.ifru_data = (void *)&ethcmd;

		if ( ioctl(sockfd, SIOCETHTOOL, &ifreq) == 0) 
		{
			ethernet = 1;
			speed    = ethcmd.speed;
			duplex   = ethcmd.duplex;
		}
	}

	if (ethernet)
	{
		p->type  = 'e';	// type ethernet

		if (speed == (u32)SPEED_UNKNOWN || speed == 65535)
			p->speed = 0;
		else
			p->speed = speed;

		switch (duplex)
		{
	   	   case DUPLEX_FULL:
			p->fullduplex	= 1;
			break;
	   	   default:
			p->fullduplex	= 0;
		}
	}
	else
	{
		/*
		** determine properties of wireless interface
		*/
		memset(&iwreq,  0, sizeof iwreq);

		strncpy(iwreq.ifr_ifrn.ifrn_name, p->name,
				sizeof iwreq.ifr_ifrn.ifrn_name-1);

		if ( ioctl(sockfd, SIOCGIWRATE, &iwreq) == 0) 
		{
			p->type       	= 'w';	// type wireless
			p->fullduplex	= 0;
			p->speed	= (iwreq.u.bitrate.value + 500000) / 1000000;
		}
		else
		{
			p->type       	= '?';	// type unknown
			p->fullduplex	= 0;
			p->speed 	= 0;
		}
	}

#ifdef ETHTOOL_GLINKSETTINGS
	free(ethlink);
#endif
	close(sockfd);

	return 1;
}
