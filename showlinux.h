/*
** ATOP - System & Process Monitor 
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains the Linux-specific functions to calculate
** figures to be visualized.
** ==========================================================================
** Author:      JC van Winkel - AT Computing, Nijmegen, Holland
** E-mail:      jc@ATComputing.nl
** Date:        November 2009
** --------------------------------------------------------------------------
** Copyright (C) 2009 	JC van Winkel
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
** $Log: showlinux.h,v $
** Initial revision
**
** Initial
**
*/
#define MAXITEMS 80    /* The maximum number of items per line */

/*
 * structure for extra parameters for system related data
*/
typedef struct {
        count_t		totut;
        count_t		totst;
        int		nact; 
        int		nproc;
        int		ntrun;
        int		ntslpi;
        int		ntslpu;
        int		nzomb;
        int		nexit;
        int		avgval;
        int		nsecs;
        count_t		 mstot;
        count_t		iotot;
	struct perdsk	*perdsk;
        int		index;
        count_t		cputot;
        count_t		percputot;
} extraparam;

/***************************************************************/
/*
 * structure for system print-list
*/
typedef struct {
        char *configname;                       // name as used to 
                                                // config print line
        char* (*doconvert)(void *, void *);     // pointer to conv function
} sys_printdef;


/*
 * structure for system print-list with priority
 * in case of leck of screen space, lowest priority items will be
 * removed first 
*/
typedef struct
{
        sys_printdef    *f;
        int             prio;
} sys_printpair;



/*
** structure for process print-list
*/
typedef struct 
{
        char *head;                      // column header
        char *configname;                // name as used to config print line
        char *(*doactiveconvert)(struct pstat *,int,int); 
                                         // pointer to conv function
                                         // for active process
        char *(*doexitconvert)  (struct pstat *,int,int);   
                                         // pointer to conv function
                                         // for exited process
        int  width;                      // required width
        int  varwidth;                   // width may grow (eg cmd params)
} proc_printdef;


typedef struct 
{
        proc_printdef   *f;
        int             prio;
} proc_printpair;

void showsysline(sys_printpair* elemptr, 
                 struct sstat* sstat, extraparam *extra,
                 char *labeltext, int usecolors, unsigned int badness);


void showhdrline(proc_printpair* elemptr, int curlist, int totlist, 
                  char showorder, char autosort) ;
void showprocline(proc_printpair* elemptr, struct pstat *curstat, 
                            double perc, int nsecs, int avgval) ;
int  procsuppress(struct pstat *, struct selection *);

extern sys_printdef *prcsyspdefs[];
extern sys_printdef *cpusyspdefs[];
extern sys_printdef *cpisyspdefs[];
extern sys_printdef *cplsyspdefs[];
extern sys_printdef *memsyspdefs[];
extern sys_printdef *swpsyspdefs[];
extern sys_printdef *pagsyspdefs[];
extern sys_printdef *dsksyspdefs[];
extern sys_printdef *nettranssyspdefs[];
extern sys_printdef *netnetsyspdefs[];
extern sys_printdef *netintfsyspdefs[];

extern sys_printdef syspdef_PRCSYS;
extern sys_printdef syspdef_PRCUSER;
extern sys_printdef syspdef_PRCNPROC;
extern sys_printdef syspdef_PRCNRUNNING;
extern sys_printdef syspdef_PRCNSLEEPING;
extern sys_printdef syspdef_PRCNDSLEEPING;
extern sys_printdef syspdef_PRCNZOMBIE;
extern sys_printdef syspdef_PRCCLONES;
extern sys_printdef syspdef_PRCNNEXIT;
extern sys_printdef syspdef_CPUSYS;
extern sys_printdef syspdef_CPUUSER;
extern sys_printdef syspdef_CPUIRQ;
extern sys_printdef syspdef_CPUIDLE;
extern sys_printdef syspdef_CPUWAIT;
extern sys_printdef syspdef_CPUISYS;
extern sys_printdef syspdef_CPUIUSER;
extern sys_printdef syspdef_CPUIIRQ;
extern sys_printdef syspdef_CPUIIDLE;
extern sys_printdef syspdef_CPUIWAIT;
extern sys_printdef syspdef_CPUISTEAL;
extern sys_printdef syspdef_CPUIFREQ;
extern sys_printdef syspdef_CPUFREQ;
extern sys_printdef syspdef_CPUSCALE;
extern sys_printdef syspdef_CPUISCALE;
extern sys_printdef syspdef_CPUSTEAL;
extern sys_printdef syspdef_CPUISTEAL;
extern sys_printdef syspdef_CPUGUEST;
extern sys_printdef syspdef_CPUIGUEST;
extern sys_printdef syspdef_CPLAVG1;
extern sys_printdef syspdef_CPLAVG5;
extern sys_printdef syspdef_CPLAVG15;
extern sys_printdef syspdef_CPLCSW;
extern sys_printdef syspdef_CPLNUMCPU;
extern sys_printdef syspdef_CPLINTR;
extern sys_printdef syspdef_MEMTOT;
extern sys_printdef syspdef_MEMFREE;
extern sys_printdef syspdef_MEMCACHE;
extern sys_printdef syspdef_MEMDIRTY;
extern sys_printdef syspdef_MEMBUFFER;
extern sys_printdef syspdef_MEMSLAB;
extern sys_printdef syspdef_SWPTOT;
extern sys_printdef syspdef_SWPFREE;
extern sys_printdef syspdef_SWPCOMMITTED;
extern sys_printdef syspdef_SWPCOMMITLIM;
extern sys_printdef syspdef_PAGSCAN;
extern sys_printdef syspdef_PAGSTALL;
extern sys_printdef syspdef_PAGSWIN;
extern sys_printdef syspdef_PAGSWOUT;
extern sys_printdef syspdef_DSKNAME;
extern sys_printdef syspdef_DSKBUSY;
extern sys_printdef syspdef_DSKNREAD;
extern sys_printdef syspdef_DSKNWRITE;
extern sys_printdef syspdef_DSKMBPERSECWR;
extern sys_printdef syspdef_DSKMBPERSECRD;
extern sys_printdef syspdef_DSKKBPERWR;
extern sys_printdef syspdef_DSKKBPERRD;
extern sys_printdef syspdef_DSKAVQUEUE;
extern sys_printdef syspdef_DSKAVIO;
extern sys_printdef syspdef_NETTRANSPORT;
extern sys_printdef syspdef_NETTCPI;
extern sys_printdef syspdef_NETTCPO;
extern sys_printdef syspdef_NETTCPACTOPEN;
extern sys_printdef syspdef_NETTCPPASVOPEN;
extern sys_printdef syspdef_NETTCPRETRANS;
extern sys_printdef syspdef_NETTCPINERR;
extern sys_printdef syspdef_NETTCPORESET;
extern sys_printdef syspdef_NETUDPNOPORT;
extern sys_printdef syspdef_NETUDPINERR;
extern sys_printdef syspdef_NETUDPI;
extern sys_printdef syspdef_NETUDPO;
extern sys_printdef syspdef_NETNETWORK;
extern sys_printdef syspdef_NETIPI;
extern sys_printdef syspdef_NETIPO;
extern sys_printdef syspdef_NETIPFRW;
extern sys_printdef syspdef_NETIPDELIV;
extern sys_printdef syspdef_NETICMPIN;
extern sys_printdef syspdef_NETICMPOUT;
extern sys_printdef syspdef_NETNAME;
extern sys_printdef syspdef_NETPCKI;
extern sys_printdef syspdef_NETPCKO;
extern sys_printdef syspdef_NETSPEEDIN;
extern sys_printdef syspdef_NETSPEEDOUT;
extern sys_printdef syspdef_NETCOLLIS;
extern sys_printdef syspdef_NETMULTICASTIN;
extern sys_printdef syspdef_NETRCVERR;
extern sys_printdef syspdef_NETSNDERR;
extern sys_printdef syspdef_NETRCVDROP;
extern sys_printdef syspdef_NETSNDDROP;
extern sys_printdef syspdef_BLANKBOX;


/*
** functions that print ???? for unavailable data
*/
char *procprt_NOTAVAIL_4(struct pstat *curstat, int avgval, int nsecs);
char *procprt_NOTAVAIL_5(struct pstat *curstat, int avgval, int nsecs);
char *procprt_NOTAVAIL_6(struct pstat *curstat, int avgval, int nsecs);
char *procprt_NOTAVAIL_7(struct pstat *curstat, int avgval, int nsecs);

extern proc_printdef *allprocpdefs[];
extern proc_printdef procprt_PID;
extern proc_printdef procprt_PPID;
extern proc_printdef procprt_SYSCPU;
extern proc_printdef procprt_USRCPU;
extern proc_printdef procprt_VGROW;
extern proc_printdef procprt_RGROW;
extern proc_printdef procprt_MINFLT;
extern proc_printdef procprt_MAJFLT;
extern proc_printdef procprt_VSTEXT;
extern proc_printdef procprt_VSIZE;
extern proc_printdef procprt_RSIZE;
extern proc_printdef procprt_CMD;
extern proc_printdef procprt_RUID;
extern proc_printdef procprt_EUID;
extern proc_printdef procprt_SUID;
extern proc_printdef procprt_FSUID;
extern proc_printdef procprt_RGID;
extern proc_printdef procprt_EGID;
extern proc_printdef procprt_SGID;
extern proc_printdef procprt_FSGID;
extern proc_printdef procprt_STDATE;
extern proc_printdef procprt_STTIME;
extern proc_printdef procprt_ENDATE;
extern proc_printdef procprt_ENTIME;
extern proc_printdef procprt_THR;
extern proc_printdef procprt_TRUN;
extern proc_printdef procprt_TSLPI;
extern proc_printdef procprt_TSLPU;
extern proc_printdef procprt_POLI;
extern proc_printdef procprt_NICE;
extern proc_printdef procprt_PRI;
extern proc_printdef procprt_RTPR;
extern proc_printdef procprt_CURCPU;
extern proc_printdef procprt_ST;
extern proc_printdef procprt_EXC;
extern proc_printdef procprt_S;
extern proc_printdef procprt_COMMAND_LINE;
extern proc_printdef procprt_NPROCS;
extern proc_printdef procprt_RDDSK;
extern proc_printdef procprt_WRDSK;
extern proc_printdef procprt_NRDDSK_NOACCT;
extern proc_printdef procprt_NWRDSK_NOACCT;
extern proc_printdef procprt_RDDSK_IOSTAT;
extern proc_printdef procprt_WRDSK_IOSTAT;
extern proc_printdef procprt_WCANCEL_IOSTAT;
extern proc_printdef procprt_AVGRSZ;
extern proc_printdef procprt_AVGWSZ;
extern proc_printdef procprt_TOTRSZ;
extern proc_printdef procprt_TOTWSZ;
extern proc_printdef procprt_TOTRSZ_NOACCT;
extern proc_printdef procprt_TOTWSZ_NOACCT;
extern proc_printdef procprt_TCPRCV;
extern proc_printdef procprt_TCPRASZ;
extern proc_printdef procprt_TCPSND;
extern proc_printdef procprt_TCPSASZ;
extern proc_printdef procprt_UDPRCV;
extern proc_printdef procprt_UDPRASZ;
extern proc_printdef procprt_UDPSND;
extern proc_printdef procprt_UDPSASZ;
extern proc_printdef procprt_RNET;
extern proc_printdef procprt_SORTITEM;
extern proc_printdef procprt_SNET;
extern proc_printdef procprt_RAWRCV;
extern proc_printdef procprt_RAWSND;



extern char *procprt_NRDDSK_ae(struct pstat *, int, int);
extern char *procprt_NWRDSK_a(struct pstat *, int, int);
extern char *procprt_NRDDSK_e(struct pstat *, int, int);
extern char *procprt_NWRDSK_e(struct pstat *, int, int);
extern char *procprt_RDDSK_IOSTAT_a(struct pstat *, int, int);
extern char *procprt_RDDSK_IOSTAT_e(struct pstat *, int, int);
extern char *procprt_WRDSK_IOSTAT_a(struct pstat *, int, int);
extern char *procprt_WRDSK_IOSTAT_e(struct pstat *, int, int);

extern char *procprt_SNET_a(struct pstat *, int, int);
extern char *procprt_SNET_e(struct pstat *, int, int);
extern char *procprt_RNET_a(struct pstat *, int, int);
extern char *procprt_RNET_e(struct pstat *, int, int);
extern char *procprt_TCPSND_a(struct pstat *, int, int);
extern char *procprt_TCPRCV_a(struct pstat *, int, int);
extern char *procprt_UDPSND_a(struct pstat *, int, int);
extern char *procprt_UDPRCV_a(struct pstat *, int, int);
extern char *procprt_TCPSASZ_a(struct pstat *, int, int);
extern char *procprt_TCPRASZ_a(struct pstat *, int, int);
extern char *procprt_UDPSASZ_a(struct pstat *, int, int);
extern char *procprt_UDPRASZ_a(struct pstat *, int, int);
extern char *procprt_TCPSND_e(struct pstat *, int, int);
extern char *procprt_TCPRCV_e(struct pstat *, int, int);
extern char *procprt_UDPSND_e(struct pstat *, int, int);
extern char *procprt_UDPRCV_e(struct pstat *, int, int);
extern char *procprt_TCPSASZ_e(struct pstat *, int, int);
extern char *procprt_TCPRASZ_e(struct pstat *, int, int);
extern char *procprt_UDPSASZ_e(struct pstat *, int, int);
extern char *procprt_UDPRASZ_e(struct pstat *, int, int);
extern char *procprt_RAWSND_e(struct pstat *, int, int);
extern char *procprt_RAWRCV_e(struct pstat *, int, int);
extern char *procprt_RAWSND_a(struct pstat *, int, int);
extern char *procprt_RAWRCV_a(struct pstat *, int, int);
