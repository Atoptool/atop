#define	MAXCPU_21	2048
#define	MAXDSK_21	1024
#define	MAXLVM_21	2048
#define	MAXMDD_21	256
#define	MAXINTF_21	128

#define	MAXDKNAM	32

/************************************************************************/

struct	memstat_21 {
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

struct	netstat_21 {
	struct ipv4_stats	ipv4;
	struct icmpv4_stats_without_InCsumErrors	icmpv4;
	struct udpv4_stats	udpv4;

	struct ipv6_stats	ipv6;
	struct icmpv6_stats	icmpv6;
	struct udpv6_stats	udpv6;

	struct tcp_stats_without_InCsumErrors	tcp;
};

/************************************************************************/

struct freqcnt_21 {
        count_t maxfreq;/* frequency in MHz                    */
        count_t cnt;    /* number of clock ticks times state   */
        count_t ticks;  /* number of total clock ticks         */
                        /* if zero, cnt is actul freq          */
};

struct percpu_21 {
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
        struct freqcnt_21	freqcnt;/* frequency scaling info  	*/
	count_t		cfuture[4];	/* reserved for future use	*/
};

struct	cpustat_21 {
	count_t	nrcpu;	/* number of cpu's 			*/
	count_t	devint;	/* number of device interrupts 		*/
	count_t	csw;	/* number of context switches		*/
	count_t	nprocs;	/* number of processes started          */
	float	lavg1;	/* load average last    minute          */
	float	lavg5;	/* load average last  5 minutes         */
	float	lavg15;	/* load average last 15 minutes         */
	count_t	cfuture[4];	/* reserved for future use	*/

	struct percpu_21   all;
	struct percpu_21   cpu[MAXCPU_21];
};

/************************************************************************/

struct	perdsk_21 {
        char	name[MAXDKNAM];	/* empty string for last        */
        count_t	nread;	/* number of read  transfers            */
        count_t	nrsect;	/* number of sectors read               */
        count_t	nwrite;	/* number of write transfers            */
        count_t	nwsect;	/* number of sectors written            */
        count_t	io_ms;	/* number of millisecs spent for I/O    */
        count_t	avque;	/* average queue length                 */
	count_t	cfuture[4];	/* reserved for future use	*/
};

struct dskstat_21 {
	int		ndsk;	/* number of physical disks	*/
	int		nmdd;	/* number of md volumes		*/
	int		nlvm;	/* number of logical volumes	*/
	struct perdsk_21	dsk[MAXDSK_21];
	struct perdsk_21	mdd[MAXMDD_21];
	struct perdsk_21	lvm[MAXLVM_21];
};

/************************************************************************/

struct	perintf_21 {
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

	long 	speed;	/* interface speed in megabits/second	*/
	char	duplex;	/* full duplex (boolean) 		*/
	count_t	cfuture[4];	/* reserved for future use	*/
};

struct intfstat_21 {
	int		 	nrintf;
	struct perintf_21	intf[MAXINTF_21];
};

/************************************************************************/
/*
** experimental stuff for access to local HTTP daemons
*/
struct wwwstat_21 {
	count_t	accesses;	/* total number of HTTP-requests	*/
	count_t	totkbytes;	/* total kbytes transfer for HTTP-req   */
	count_t	uptime;		/* number of seconds since startup	*/
	int	bworkers;	/* number of busy httpd-daemons		*/
	int	iworkers;	/* number of idle httpd-daemons		*/
};
/************************************************************************/

struct	sstat_21 {
	struct cpustat_21	cpu;
	struct memstat_21	mem;
	struct netstat_21	net;
	struct intfstat_21	intf;
	struct dskstat_21	dsk;

	struct wwwstat_21	www;
};
