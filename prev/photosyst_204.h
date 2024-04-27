#define	MAXCPU_24	2048
#define	MAXDSK_24	1024
#define	MAXLVM_24	2048
#define	MAXMDD_24	256
#define	MAXINTF_24	128
#define	MAXCONTAINER_24	128
#define	MAXNFSMOUNT_24	64
#define	MAXIBPORT_24	32
#define	MAXGPU_24	32

/************************************************************************/
struct	memstat_24 {
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

	count_t	commitlim;	// commit limit in pages
	count_t	committed;	// number of reserved pages

	count_t	shmem;		// tot shmem incl. tmpfs (pages)
	count_t	shmrss;		// resident shared memory (pages)
	count_t	shmswp;		// swapped shared memory (pages)

	count_t	slabreclaim;	// reclaimable slab (pages)

	count_t	tothugepage;	// total huge pages (huge pages)
	count_t	freehugepage;	// free  huge pages (huge pages)
	count_t	hugepagesz;	// huge page size (bytes)

	count_t	vmwballoon;	// vmware claimed balloon pages
	count_t	cfuture[8];	// reserved for future use
};

/************************************************************************/

struct	netstat_24 {
	struct ipv4_stats	ipv4;
	struct icmpv4_stats_without_InCsumErrors	icmpv4;
	struct udpv4_stats	udpv4;

	struct ipv6_stats	ipv6;
	struct icmpv6_stats	icmpv6;
	struct udpv6_stats	udpv6;

	struct tcp_stats_without_InCsumErrors	tcp;
};

/************************************************************************/

struct freqcnt_24 {
        count_t maxfreq;/* frequency in MHz                    */
        count_t cnt;    /* number of clock ticks times state   */
        count_t ticks;  /* number of total clock ticks         */
                        /* if zero, cnt is actul freq          */
};

struct percpu_24 {
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
        struct freqcnt_24	freqcnt;/* frequency scaling info  		*/
	count_t		instr;	/* CPU instructions 			*/
	count_t		cycle;	/* CPU cycles 				*/
	count_t		cfuture[2];	/* reserved for future use	*/
};

struct	cpustat_24 {
	count_t	nrcpu;	/* number of cpu's 			*/
	count_t	devint;	/* number of device interrupts 		*/
	count_t	csw;	/* number of context switches		*/
	count_t	nprocs;	/* number of processes started          */
	float	lavg1;	/* load average last    minute          */
	float	lavg5;	/* load average last  5 minutes         */
	float	lavg15;	/* load average last 15 minutes         */
	count_t	cfuture[4];	/* reserved for future use	*/

	struct percpu_24   all;
	struct percpu_24   cpu[MAXCPU_24];
};

/************************************************************************/

struct	perdsk_24 {
        char	name[MAXDKNAM];	/* empty string for last        */
        count_t	nread;	/* number of read  transfers            */
        count_t	nrsect;	/* number of sectors read               */
        count_t	nwrite;	/* number of write transfers            */
        count_t	nwsect;	/* number of sectors written            */
        count_t	io_ms;	/* number of millisecs spent for I/O    */
        count_t	avque;	/* average queue length                 */
	count_t	cfuture[4];	/* reserved for future use	*/
};

struct dskstat_24 {
	int		ndsk;	/* number of physical disks	*/
	int		nmdd;	/* number of md volumes		*/
	int		nlvm;	/* number of logical volumes	*/
	struct perdsk_24	dsk[MAXDSK_24];
	struct perdsk_24	mdd[MAXMDD_24];
	struct perdsk_24	lvm[MAXLVM_24];
};

/************************************************************************/

struct	perintf_24 {
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

	char 	type;	/* interface type ('e'/'w'/'?')  	*/
	long 	speed;	/* interface speed in megabits/second	*/
	long 	speedp;	/* previous interface speed 		*/
	char	duplex;	/* full duplex (boolean) 		*/
	count_t	cfuture[4];	/* reserved for future use	*/
};

struct intfstat_24 {
	int		nrintf;
	struct perintf_24	intf[MAXINTF_24];
};

/************************************************************************/

struct  pernfsmount_24 {
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

struct nfsstat_24 {
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
       		struct pernfsmount_24	nfsmnt[MAXNFSMOUNT_24];
	} nfsmounts;
};

/************************************************************************/
struct	psi_24 {
	float	avg10;		// average pressure last 10 seconds
	float	avg60;		// average pressure last 60 seconds
	float	avg300;		// average pressure last 300 seconds
	count_t	total;		// total number of milliseconds
};

struct	pressure_24 {
	char	   present;	/* pressure stats supported?	*/
	char       future[3];
	struct psi_24 cpusome;	/* pressure stall info 'some'   */
	struct psi_24 memsome;	/* pressure stall info 'some'   */
	struct psi_24 memfull;	/* pressure stall info 'full'   */
	struct psi_24 iosome;	/* pressure stall info 'some'   */
	struct psi_24 iofull;	/* pressure stall info 'full'   */
};

/************************************************************************/

struct  percontainer_24 {
        unsigned long	ctid;		/* container id			*/
        unsigned long	numproc;	/* number of processes		*/

        count_t system;  	/* */
        count_t user;  		/* */
        count_t nice;  		/* */
        count_t uptime; 	/* */

        count_t physpages; 	/* */
};

struct contstat_24 {
        int             	nrcontainer;
        struct percontainer_24	cont[MAXCONTAINER_24];
};

/************************************************************************/
/*
** experimental stuff for access to local HTTP daemons
*/
struct wwwstat_24 {
	count_t	accesses;	/* total number of HTTP-requests	*/
	count_t	totkbytes;	/* total kbytes transfer for HTTP-req   */
	count_t	uptime;		/* number of seconds since startup	*/
	int	bworkers;	/* number of busy httpd-daemons		*/
	int	iworkers;	/* number of idle httpd-daemons		*/
};

/************************************************************************/

struct pergpu_24 {
	char	taskstats;		// GPU task statistics supported?
	unsigned char   nrprocs;	// number of processes using GPU
	char	type[MAXGPUTYPE+1];	// GPU type
	char	busid[MAXGPUBUS+1];	// GPU bus identification
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

struct gpustat_24 {
	int		nrgpus;		// total number of GPUs
	struct pergpu_24   gpu[MAXGPU_24];
};

/************************************************************************/

struct perifb_24 {
	char	ibname[MAXIBNAME];	// InfiniBand controller
	short	portnr;			// InfiniBand controller port

	short	lanes;			// number of lanes (traffic factor)
	count_t	rate;			// transfer rate in megabits/sec
	count_t	rcvb;   	    	// bytes received
	count_t	sndb;       		// bytes transmitted
	count_t	rcvp;   	    	// packets received
	count_t	sndp;       		// packets transmitted
};

struct ifbstat_24 {
	int		nrports;	// total number of IB ports
	struct perifb_24   ifb[MAXIBPORT_24];
};
/************************************************************************/

struct	sstat_24 {
	struct cpustat_24	cpu;
	struct memstat_24	mem;
	struct netstat_24	net;
	struct intfstat_24	intf;
	struct dskstat_24  	dsk;
	struct nfsstat_24  	nfs;
	struct contstat_24 	cfs;
	struct pressure_24	psi;
	struct gpustat_24 	gpu;
	struct ifbstat_24 	ifb;

	struct wwwstat_24	www;
};
