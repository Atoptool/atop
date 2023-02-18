/*
** Copyright (C) 2014 Gerlof Langeveld
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>

#include <linux/genetlink.h>
#include <linux/taskstats.h>

#include "atop.h"

/*
** generic macro's
*/
#define GENLMSG_DATA(glh)	((void *)(NLMSG_DATA(glh) + GENL_HDRLEN))
#define GENLMSG_PAYLOAD(glh)	(NLMSG_PAYLOAD(glh, 0)    - GENL_HDRLEN)
#define NLA_DATA(na)		((void *)((char*)(na)     + NLA_HDRLEN))
#define NLA_PAYLOAD(len)	(len                      - NLA_HDRLEN)

/*
** function prototypes
*/
static int	nlsock_open(void);
static int	nlsock_sendcmd(int, __u16, __u32, __u8, __u16, void *, int);
static int	nlsock_getfam(int);
static int	getnumcpu(void);

/*
** message format to communicate with NETLINK
*/
struct msgtemplate {
	struct nlmsghdr		n;
	struct genlmsghdr	g;
	char			buf[2048];
};

int
netlink_open(void)
{
	int	nlsock, famid;
	char	cpudef[64];

	/*
	** open the netlink socket
	*/
	nlsock = nlsock_open();

	/*
	** get the family id for the TASKSTATS family
	*/
	famid = nlsock_getfam(nlsock);

	/*
	** determine maximum number of CPU's for this system
	** and specify mask to register all cpu's
	*/
	sprintf(cpudef, "0-%d", getnumcpu() -1);

	/*
	** indicate to listen for processes from all CPU's
	*/
	if (nlsock_sendcmd(nlsock, famid, getpid(), TASKSTATS_CMD_GET,
		TASKSTATS_CMD_ATTR_REGISTER_CPUMASK,
		&cpudef, strlen(cpudef)+1) == -1)
	{
		fprintf(stderr, "register cpumask failed\n");
		return -1;
	}

	return nlsock;
}


int
netlink_recv(int nlsock, int flags)
{
	int			len;
        struct msgtemplate	msg;

        if ( (len = recv(nlsock, &msg, sizeof msg, flags)) == -1)
		return -errno;		// negative: errno

	if  (msg.n.nlmsg_type == NLMSG_ERROR || !NLMSG_OK(&msg.n, len))
	{
		struct nlmsgerr *err = NLMSG_DATA(&msg);

		return err->error;	// negative: errno
	}

	return len;			// 0 or positive value
}

static int
nlsock_getfam(int nlsock)
{
	int			len;
        struct nlattr		*nlattr;
        struct msgtemplate	msg;

        nlsock_sendcmd(nlsock, GENL_ID_CTRL, getpid(),
			CTRL_CMD_GETFAMILY,
                        CTRL_ATTR_FAMILY_NAME,
			TASKSTATS_GENL_NAME, sizeof TASKSTATS_GENL_NAME);

        if ( (len = recv(nlsock, &msg, sizeof msg, 0)) == -1)
	{
		perror("receive NETLINK family");
		exit(1);
	}

	if  (msg.n.nlmsg_type == NLMSG_ERROR || !NLMSG_OK(&msg.n, len))
	{
		struct nlmsgerr *err = NLMSG_DATA(&msg);

		fprintf(stderr, "receive NETLINK family, errno %d\n",
                                err->error);

		exit(1);
	}

        nlattr = (struct nlattr *) GENLMSG_DATA(&msg);
	nlattr = (struct nlattr *) ((char *) nlattr +
					NLA_ALIGN(nlattr->nla_len));

	if (nlattr->nla_type != CTRL_ATTR_FAMILY_ID)
	{
		fprintf(stderr, "unexpected family id\n");
		exit(1);
	}

	return *(__u16 *) NLA_DATA(nlattr);
}


static int
nlsock_open(void)
{
	int 			nlsock, rcvsz = 256*1024;
	struct sockaddr_nl	nlsockaddr;

	if ( (nlsock = socket(AF_NETLINK, SOCK_RAW, NETLINK_GENERIC) ) == -1)
	{
		perror("open NETLINK socket");
		exit(1);
	}

	if (setsockopt(nlsock, SOL_SOCKET, SO_RCVBUF, &rcvsz, sizeof rcvsz)
									== -1)
	{
		perror("set length receive buffer");
		exit(1);
	}

	memset(&nlsockaddr, 0, sizeof nlsockaddr);
	nlsockaddr.nl_family = AF_NETLINK;

	if (bind(nlsock, (struct sockaddr *) &nlsockaddr, sizeof nlsockaddr)
									== -1)
	{
		perror("bind NETLINK socket");
		close(nlsock);
		exit(1);
	}

	return nlsock;
}

static int
nlsock_sendcmd(int nlsock, __u16 nlmsg_type,
	__u32 nlmsg_pid, __u8 genl_cmd,
	__u16 nla_type, void *nla_data, int nla_len)
{
	struct nlattr		*na;
	struct sockaddr_nl	nlsockaddr;
	int 			rv, buflen;
	char 			*buf;
	struct msgtemplate	msg;

	msg.n.nlmsg_len 	= NLMSG_LENGTH(GENL_HDRLEN);
	msg.n.nlmsg_type 	= nlmsg_type;
	msg.n.nlmsg_flags 	= NLM_F_REQUEST;
	msg.n.nlmsg_seq 	= 0;
	msg.n.nlmsg_pid 	= nlmsg_pid;
	msg.g.cmd 		= genl_cmd;
	msg.g.version 		= 0x1;
	na 			= (struct nlattr *) GENLMSG_DATA(&msg);
	na->nla_type 		= nla_type;
	na->nla_len 		= nla_len + 1 + NLA_HDRLEN;

	memcpy(NLA_DATA(na), nla_data, nla_len);
	msg.n.nlmsg_len 	+= NLMSG_ALIGN(na->nla_len);

	buf 			= (char *) &msg;
	buflen 			= msg.n.nlmsg_len ;

	memset(&nlsockaddr, 0, sizeof(nlsockaddr));
	nlsockaddr.nl_family = AF_NETLINK;

	while ((rv = sendto(nlsock, buf, buflen, 0,
		(struct sockaddr *) &nlsockaddr, sizeof(nlsockaddr))) < buflen)
	{
		if (rv == -1)
		{
			if (errno != EAGAIN)
			{
				perror("sendto NETLINK");
				return -1;
			}
		}
		else
		{
			buf 	+= rv;
			buflen 	-= rv;
		}
	}

	return 0;
}

static int
getnumcpu(void)
{
	FILE	*fp;
	char	linebuf[4096], label[256];
	int	cpunum, maxcpu = 0;

	if ( (fp = fopen("/proc/stat", "r")) != NULL)
	{
		while ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
		{
			sscanf(linebuf, "%255s", label);

			if ( strncmp("cpu", label, 3) == 0 && strlen(label) >3)
			{
				cpunum = atoi(&label[3]);

				if (maxcpu < cpunum)
					maxcpu = cpunum;
			}

			if ( strncmp("int", label, 3) == 0)
				break;
		}

		fclose(fp);
	}

	return maxcpu+1;
}
