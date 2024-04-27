/* 
** structure containing only relevant process-info extracted 
** from kernel's process-administration
*/
struct tstat_23 {
	/* GENERAL TASK INFO 					*/
	struct gen_23 {
		int	tgid;		/* threadgroup identification 	*/
		int	pid;		/* process identification 	*/
		int	ppid;           /* parent process identification*/
		int	ruid;		/* real  user  identification 	*/
		int	euid;		/* eff.  user  identification 	*/
		int	suid;		/* saved user  identification 	*/
		int	fsuid;		/* fs    user  identification 	*/
		int	rgid;		/* real  group identification 	*/
		int	egid;		/* eff.  group identification 	*/
		int	sgid;		/* saved group identification 	*/
		int	fsgid;		/* fs    group identification 	*/
		int	nthr;		/* number of threads in tgroup 	*/
		char	name[PNAMLEN+1];/* process name string       	*/
		char 	isproc;		/* boolean: process level?      */
		char 	state;		/* process state ('E' = exited)	*/
		int	excode;		/* process exit status		*/
		time_t 	btime;		/* process start time (epoch)	*/
		time_t 	elaps;		/* process elaps time (hertz)	*/
		char	cmdline[CMDLEN+1];/* command-line string       	*/
		int	nthrslpi;	/* # threads in state 'S'       */
		int	nthrslpu;	/* # threads in state 'D'       */
		int	nthrrun;	/* # threads in state 'R'       */

		int	ctid;		/* OpenVZ container ID		*/
		int	vpid;		/* OpenVZ virtual PID		*/

		int	wasinactive;	/* boolean: task inactive	*/

		char	container[16];	/* Docker container id (12 pos)	*/
	} gen;

	/* CPU STATISTICS						*/
	struct cpu_23 {
		count_t	utime;		/* time user   text (ticks) 	*/
		count_t	stime;		/* time system text (ticks) 	*/
		int	nice;		/* nice value                   */
		int	prio;		/* priority                     */
		int	rtprio;		/* realtime priority            */
		int	policy;		/* scheduling policy            */
		int	curcpu;		/* current processor            */
		int	sleepavg;       /* sleep average percentage     */
		int	ifuture[4];	/* reserved for future use	*/
		count_t	cfuture[4];	/* reserved for future use	*/
	} cpu;

	/* DISK STATISTICS						*/
	struct dsk_23 {
		count_t	rio;		/* number of read requests 	*/
		count_t	rsz;		/* cumulative # sectors read	*/
		count_t	wio;		/* number of write requests 	*/
		count_t	wsz;		/* cumulative # sectors written	*/
		count_t	cwsz;		/* cumulative # written sectors */
					/* being cancelled              */
		count_t	cfuture[4];	/* reserved for future use	*/
	} dsk;

	/* MEMORY STATISTICS						*/
	struct mem_23 {
		count_t	minflt;		/* number of page-reclaims 	*/
		count_t	majflt;		/* number of page-faults 	*/
		count_t	vexec;		/* virtmem execfile (Kb)        */
		count_t	vmem;		/* virtual  memory  (Kb)	*/
		count_t	rmem;		/* resident memory  (Kb)	*/
		count_t	pmem;		/* resident memory  (Kb)	*/
		count_t vgrow;		/* virtual  growth  (Kb)    	*/
		count_t rgrow;		/* resident growth  (Kb)     	*/
		count_t vdata;		/* virtmem data     (Kb)     	*/
		count_t vstack;		/* virtmem stack    (Kb)     	*/
		count_t vlibs;		/* virtmem libexec  (Kb)     	*/
		count_t vswap;		/* swap space used  (Kb)     	*/
		count_t	cfuture[4];	/* reserved for future use	*/
	} mem;

	/* NETWORK STATISTICS						*/
	struct net_23 {
		count_t tcpsnd;		/* number of TCP-packets sent	*/
		count_t tcpssz;		/* cumulative size packets sent	*/
		count_t	tcprcv;		/* number of TCP-packets recved	*/
		count_t tcprsz;		/* cumulative size packets rcvd	*/
		count_t	udpsnd;		/* number of UDP-packets sent	*/
		count_t udpssz;		/* cumulative size packets sent	*/
		count_t	udprcv;		/* number of UDP-packets recved	*/
		count_t udprsz;		/* cumulative size packets sent	*/
		count_t	avail1;		/* */
		count_t	avail2;		/* */
		count_t	cfuture[4];	/* reserved for future use	*/
	} net;
};
