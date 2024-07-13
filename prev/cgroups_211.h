// structure containing general info and metrics per cgroup (directory)
//
struct cstat_211 {
	// GENERAL INFO
	struct cggen_211 {
		int	structlen;	// struct length including rounded name
		int	sequence;	// sequence number in chain/array
		int	parentseq;	// parent sequence number in chain/array
		int	depth;		// cgroup tree depth starting from 0
		int	nprocs;		// number of processes in cgroup
		int	procsbelow;	// number of processes in cgroups below
		int	namelen;	// cgroup name length (at end of struct)
		int	fullnamelen;	// cgroup path length
		int	ifuture[4];

		long	namehash;	// cgroup name hash of
					// full path name excluding slashes
		long	lfuture[4];
	} gen;

	// CONFIGURATION INFO
	struct cgconf_211 {
		int	cpuweight;	// -1=max, -2=undefined
		int	cpumax;		// -1=max, -2=undefined (perc)

		count_t	memmax;		// -1=max, -2=undefined (pages)
		count_t	swpmax;		// -1=max, -2=undefined (pages)

		int	dskweight;	// -1=max, -2=undefined

		int	ifuture[5];
		count_t	cfuture[5];
	} conf;

	// CPU STATISTICS
	struct cgcpu_211 {
		count_t	utime;		// time user   text (usec) -1=undefined
		count_t	stime;		// time system text (usec) -1=undefined

		count_t	somepres;	// some pressure (microsec)
		count_t	fullpres;	// full pressure (microsec)

		count_t	cfuture[5];
	} cpu;

	// MEMORY STATISTICS
	struct cgmem_211 {
		count_t	current;	// current memory (pages)   -1=undefined
		count_t	anon;		// anonymous memory (pages) -1=undefined
		count_t	file;		// file memory (pages)      -1=undefined
		count_t	kernel;		// kernel memory (pages)    -1=undefined
		count_t	shmem;		// shared memory (pages)    -1=undefined

		count_t	somepres;	// some pressure (microsec)
		count_t	fullpres;	// full pressure (microsec)

		count_t	cfuture[5];
	} mem;

	// DISK I/O STATISTICS
	struct cgdsk_211 {
		count_t	rbytes;		// total bytes read on all physical disks
		count_t	wbytes;		// total bytes written on all physical disks
		count_t	rios;		// total read I/Os on all physical disks
		count_t	wios;		// total write I/Os on all physical disks

		count_t	somepres;	// some pressure (microsec)
		count_t	fullpres;	// full pressure (microsec)

		count_t	cfuture[5];
	} dsk;

	// cgroup name with variable length
	char	cgname[];
};
