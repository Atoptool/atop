#define	MAXCPU_210		2048
#define	MAXDSK_210		1024
#define	MAXNUMA_210		1024
#define	MAXLVM_210		2048
#define	MAXMDD_210		256
#define	MAXINTF_210		128
#define	MAXCONTAINER_210	128
#define	MAXNFSMOUNT_210	64
#define	MAXIBPORT_210	32
#define	MAXGPU_210		32
#define	MAXGPUBUS_210	12
#define	MAXGPUTYPE_210	12
#define	MAXLLC_210		256

#define	MAXDKNAM_210	32
#define	MAXIBNAME_210	12

/************************************************************************/
struct	memstat_210 {
	count_t	physmem;	// number of physical pages
	count_t	freemem;	// number of free     pages
	count_t	buffermem;	// number of buffer   pages
	count_t	slabmem;	// number of slab     pages
	count_t	cachemem;	// number of cache    pages
	count_t	cachedrt;	// number of cache    pages (dirty)

	count_t	totswap;	// number of pages in swap
	count_t	freeswap;	// number of free swap pages

	count_t	pgscans;	// number of page scans
	count_t	pgsteal;	// number of page steals
	count_t	allocstall;	// try to free pages forced
	count_t	swouts;		// number of pages swapped out
	count_t	swins;		// number of pages swapped in
	count_t	tcpsock;	// number of pages allocated by TCP sockets
	count_t	udpsock;	// number of pages allocated by UDP sockets

	count_t	commitlim;	// commit limit in pages
	count_t	committed;	// number of reserved pages

	count_t	shmem;		// tot shmem incl. tmpfs (pages)
	count_t	shmrss;		// resident shared memory (pages)
	count_t	shmswp;		// swapped shared memory (pages)

	count_t	slabreclaim;	// reclaimable slab (pages)

	count_t	stothugepage;	// total huge pages (huge pages) - small
	count_t	sfreehugepage;	// free  huge pages (huge pages) - small
	count_t	shugepagesz;	// huge page size (bytes) - small

	count_t	vmwballoon;	// vmware claimed balloon pages
	count_t	zfsarcsize;	// zfsonlinux ARC size (pages)
	count_t swapcached;	// swap cache (pages)
	count_t	ksmsharing;	// saved i.e. deduped memory (pages)
	count_t	ksmshared;	// current size shared pages (pages)
	count_t	zswapped;	// zswap stored pages decompressed (pages)
	count_t	zswap;		// zswap current pool size compressed (pages)
	count_t	oomkills;	// number of oom killings
	count_t	compactstall;	// counter for process stalls
	count_t	pgmigrate;	// counter for migrated successfully (pages)
	count_t	numamigrate;	// counter for numa migrated (pages)
	count_t	pgouts;		// total number of pages written to block device
	count_t	pgins;		// total number of pages read from block device
	count_t	pagetables;	// page tables of processes (pages)

	count_t zswouts;	// number of pages swapped out to zswap
	count_t zswins;		// number of pages swapped in from zswap

	count_t	ltothugepage;	// total huge pages (huge pages) - large
	count_t	lfreehugepage;	// free  huge pages (huge pages) - large
	count_t	lhugepagesz;	// huge page size (bytes) - large

	count_t availablemem;	// available memory (pages)

	count_t anonhugepage;   // anonymous transparent huge pages
				// (in units of 'normal' pages)

	count_t	cfuture[4];	// reserved for future use
};

/************************************************************************/

struct	mempernuma_210 {
	int	numanr;
	float	frag;		// fragmentation level for this numa
	count_t	totmem;		// number of physical pages for this numa
	count_t	freemem;	// number of free     pages for this numa
	count_t	filepage;	// number of file     pages for this numa
	count_t	dirtymem;	// number of cache    pages (dirty) for this numa
	count_t	slabmem;	// number of slab     pages for this numa
	count_t	slabreclaim;	// reclaimable slab (pages) for this numa

	count_t	active;		// number of pages used more recently for this numa
	count_t	inactive;	// number of pages less recently used for this numa

	count_t	shmem;		// tot shmem incl. tmpfs (pages) for this numa
	count_t	tothp;		// total huge pages (huge pages) for this numa
	count_t	freehp;		// total free pages (huge pages) for this numa
};

struct	memnuma_210 {
	count_t           nrnuma;		/* the counts of numa		*/
	struct mempernuma_210 numa[MAXNUMA_210];
};

struct	cpupernuma_210 {
	int	numanr;
	count_t	nrcpu;		// number of cpu's
	count_t	stime;		// accumulate system  time in clock ticks for per numa
	count_t	utime;		// accumulate user    time in clock ticks for per numa
	count_t	ntime;		// accumulate nice    time in clock ticks for per numa
	count_t	itime;		// accumulate idle    time in clock ticks for per numa
	count_t	wtime;		// accumulate iowait  time in clock ticks for per numa
	count_t	Itime;		// accumulate irq     time in clock ticks for per numa
	count_t	Stime;		// accumulate softirq time in clock ticks for per numa
	count_t	steal;		// accumulate steal   time in clock ticks for per numa
	count_t	guest;		// accumulate guest   time in clock ticks for per numa
};

struct	cpunuma_210 {
	count_t           nrnuma;		/* the counts of numa		*/
	struct cpupernuma_210 numa[MAXNUMA_210];
};

/************************************************************************/

struct	netstat_210 {
	struct ipv4_stats	ipv4;
	struct icmpv4_stats	icmpv4;
	struct udpv4_stats	udpv4;

	struct ipv6_stats	ipv6;
	struct icmpv6_stats	icmpv6;
	struct udpv6_stats	udpv6;

	struct tcp_stats	tcp;
};

/************************************************************************/

struct freqcnt_210 {
        count_t maxfreq;/* frequency in MHz                    */
        count_t cnt;    /* number of clock ticks times state   */
        count_t ticks;  /* number of total clock ticks         */
                        /* if zero, cnt is actual freq         */
};

struct percpu_210 {
	int		cpunr;
	count_t		stime;	/* system  time in clock ticks		*/
	count_t		utime;	/* user    time in clock ticks		*/
	count_t		ntime;	/* nice    time in clock ticks		*/
	count_t		itime;	/* idle    time in clock ticks		*/
	count_t		wtime;	/* iowait  time in clock ticks		*/
	count_t		Itime;	/* irq     time in clock ticks		*/
	count_t		Stime;	/* softirq time in clock ticks		*/
	count_t		steal;	/* steal   time in clock ticks		*/
	count_t		guest;	/* guest   time in clock ticks		*/
        struct freqcnt_210	freqcnt;/* frequency scaling info  		*/
	count_t		instr;	/* CPU instructions 			*/
	count_t		cycle;	/* CPU cycles 				*/
	count_t		cfuture[6];	/* reserved for future use	*/
};

struct	cpustat_210 {
	count_t	nrcpu;	/* number of cpu's 			*/
	count_t	devint;	/* number of device interrupts 		*/
	count_t	csw;	/* number of context switches		*/
	count_t	nprocs;	/* number of processes started          */
	float	lavg1;	/* load average last    minute          */
	float	lavg5;	/* load average last  5 minutes         */
	float	lavg15;	/* load average last 15 minutes         */
	count_t	cfuture[4];	/* reserved for future use	*/

	struct percpu_210   all;
	struct percpu_210   cpu[MAXCPU_210];
};

/************************************************************************/

struct	perdsk_210 {
        char	name[MAXDKNAM_210];	/* empty string for last		*/
        count_t	nread;		/* number of read  transfers		*/
        count_t	nrsect;		/* number of sectors read		*/
        count_t	nwrite;		/* number of write transfers		*/
        count_t	nwsect;		/* number of sectors written		*/
        count_t	io_ms;		/* number of millisecs spent for I/O	*/
        count_t	avque;		/* average queue length			*/
        count_t	ndisc;		/* number of discards (-1 = unavailable)*/
        count_t	ndsect;		/* number of sectors discarded		*/
        count_t	inflight;	/* number of inflight I/O		*/
        count_t	cfuture[3];	/* reserved for future use		*/
};

struct dskstat_210 {
	int		ndsk;	/* number of physical disks	*/
	int		nmdd;	/* number of md volumes		*/
	int		nlvm;	/* number of logical volumes	*/
	struct perdsk_210	dsk[MAXDSK_210];
	struct perdsk_210	mdd[MAXMDD_210];
	struct perdsk_210	lvm[MAXLVM_210];
};

/************************************************************************/

struct	perintf_210 {
        char	name[16];	/* empty string for last        */

        count_t	rbyte;	/* number of read bytes                 */
        count_t	rpack;	/* number of read packets               */
	count_t rerrs;  /* receive errors                       */
	count_t rdrop;  /* receive drops                        */
	count_t rfifo;  /* receive fifo                         */
	count_t rframe; /* receive framing errors               */
	count_t rcompr; /* receive compressed                   */
	count_t rmultic;/* receive multicast                    */
	count_t	rfuture[4];	/* reserved for future use	*/

        count_t	sbyte;	/* number of written bytes              */
        count_t	spack;	/* number of written packets            */
	count_t serrs;  /* transmit errors                      */
	count_t sdrop;  /* transmit drops                       */
	count_t sfifo;  /* transmit fifo                        */
	count_t scollis;/* collisions                           */
	count_t scarrier;/* transmit carrier                    */
	count_t scompr; /* transmit compressed                  */
	count_t	sfuture[4];	/* reserved for future use	*/

	char 	type;	/* interface type ('e'/'w'/'v'/'?')  	*/
	long 	speed;	/* interface speed in megabits/second	*/
	long 	speedp;	/* previous interface speed 		*/
	char	duplex;	/* full duplex (boolean) 		*/
	count_t	cfuture[4];	/* reserved for future use	*/
};

struct intfstat_210 {
	int		nrintf;
	struct perintf_210	intf[MAXINTF_210];
};

/************************************************************************/

struct  pernfsmount_210 {
        char 	mountdev[128];		/* mountdevice 			*/
        count_t	age;			/* number of seconds mounted	*/
	
	count_t	bytesread;		/* via normal reads		*/
	count_t	byteswrite;		/* via normal writes		*/
	count_t	bytesdread;		/* via direct reads		*/
	count_t	bytesdwrite;		/* via direct writes		*/
	count_t	bytestotread;		/* via reads			*/
	count_t	bytestotwrite;		/* via writes			*/
	count_t	pagesmread;		/* via mmap  reads		*/
	count_t	pagesmwrite;		/* via mmap  writes		*/

	count_t	future[8];
};

struct nfsstat_210 {
	struct {
        	count_t	netcnt;
		count_t netudpcnt;
		count_t nettcpcnt;
		count_t nettcpcon;

		count_t rpccnt;
		count_t rpcbadfmt;
		count_t rpcbadaut;
		count_t rpcbadcln;

		count_t rpcread;
		count_t rpcwrite;

	   	count_t	rchits;		/* repcache hits	*/
	   	count_t	rcmiss;		/* repcache misses	*/
	   	count_t	rcnoca;		/* uncached requests	*/

	   	count_t	nrbytes;	/* read bytes		*/
	   	count_t	nwbytes;	/* written bytes	*/

		count_t	future[8];
	} server;

	struct {
		count_t	rpccnt;
		count_t rpcretrans;
		count_t rpcautrefresh;

		count_t rpcread;
		count_t rpcwrite;

		count_t	future[8];
	} client;

	struct {
        	int             	nrmounts;
       		struct pernfsmount_210	nfsmnt[MAXNFSMOUNT_210];
	} nfsmounts;
};

/************************************************************************/
struct	psi_210 {
	float	avg10;		// average pressure last 10 seconds
	float	avg60;		// average pressure last 60 seconds
	float	avg300;		// average pressure last 300 seconds
	count_t	total;		// total number of milliseconds
};

struct	pressure_210 {
	char	   present;	/* pressure stats supported?	*/
	char       future[3];
	struct psi_210 cpusome;	/* pressure stall info 'some'   */
	struct psi_210 memsome;	/* pressure stall info 'some'   */
	struct psi_210 memfull;	/* pressure stall info 'full'   */
	struct psi_210 iosome;	/* pressure stall info 'some'   */
	struct psi_210 iofull;	/* pressure stall info 'full'   */
};

/************************************************************************/

struct  percontainer_210 {
        unsigned long	ctid;		/* container id			*/
        unsigned long	numproc;	/* number of processes		*/

        count_t system;  	/* */
        count_t user;  		/* */
        count_t nice;  		/* */
        count_t uptime; 	/* */

        count_t physpages; 	/* */
};

struct contstat_210 {
        int             	nrcontainer;
        struct percontainer_210	cont[MAXCONTAINER_210];
};

/************************************************************************/
/*
** experimental stuff for access to local HTTP daemons
*/
#define	HTTPREQ	"GET /server-status?auto HTTP/1.1\nHost: localhost\n\n"

struct wwwstat_210 {
	count_t	accesses;	/* total number of HTTP-requests	*/
	count_t	totkbytes;	/* total kbytes transfer for HTTP-req   */
	count_t	uptime;		/* number of seconds since startup	*/
	int	bworkers;	/* number of busy httpd-daemons		*/
	int	iworkers;	/* number of idle httpd-daemons		*/
};

/************************************************************************/
struct pergpu_210 {
	char	taskstats;		// GPU task statistics supported?
	unsigned char   nrprocs;	// number of processes using GPU
	char	type[MAXGPUTYPE_210+1];	// GPU type
	char	busid[MAXGPUBUS_210+1];	// GPU bus identification
	int	gpunr;			// GPU number
	int	gpupercnow;		// processor percentage last second
					// -1 if not supported
	int	mempercnow;		// memory    percentage last second
					// -1 if not supported
	count_t	memtotnow;		// total memory in KiB
	count_t	memusenow;		// used  memory in KiB
	count_t	samples;		// number of samples
	count_t	gpuperccum;		// cumulative processor busy percentage
					// -1 if not supported
	count_t	memperccum;		// cumulative memory percentage 
					// -1 if not supported
	count_t	memusecum;		// cumulative used memory in KiB
};

struct gpustat_210 {
	int		nrgpus;		// total number of GPUs
	struct pergpu_210   gpu[MAXGPU_210];
};

/************************************************************************/
struct perifb_210 {
	char	ibname[MAXIBNAME_210];	// InfiniBand controller
	short	portnr;			// InfiniBand controller port

	short	lanes;			// number of lanes (traffic factor)
	count_t	rate;			// transfer rate in megabits/sec
	count_t	rcvb;   	    	// bytes received
	count_t	sndb;       		// bytes transmitted
	count_t	rcvp;   	    	// packets received
	count_t	sndp;       		// packets transmitted
};

struct ifbstat_210 {
	int		nrports;	// total number of IB ports
	struct perifb_210   ifb[MAXIBPORT_210];
};

/************************************************************************/
struct perllc_210 {
	unsigned char	id;
	float		occupancy;
	count_t		mbm_local;
	count_t		mbm_total;
};

struct llcstat_210 {
	int		nrllcs;	        // total number of LLC
	struct perllc_210   perllc[MAXLLC_210];
};

/************************************************************************/

struct	sstat_210 {
	struct cpustat_210	cpu;
	struct memstat_210	mem;
	struct netstat_210	net;
	struct intfstat_210	intf;
	struct memnuma_210	memnuma;
	struct cpunuma_210	cpunuma;
	struct dskstat_210  dsk;
	struct nfsstat_210  nfs;
	struct contstat_210 cfs;
	struct pressure_210	psi;
	struct gpustat_210 	gpu;
	struct ifbstat_210 	ifb;
	struct llcstat_210  llc;

	struct wwwstat_210	www;
};
