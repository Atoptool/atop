/*
** keys to access the semaphores
*/
#define PACCTPUBKEY	1071980		// # clients using shadow files
#define PACCTPRVKEY	(PACCTPUBKEY-1)	// # atopacctd daemons busy (max. 1)

/*
** name of the PID file
*/
#define	PIDFILE		"/var/run/atopacctd.pid"

/*
** directory containing the source accounting file and
** the subdirectory (PACCTSHADOWD) containing the shadow file(s)
** this directory can be overruled by a command line parameter (atopacctd)
** or by a keyword in the /etc/atoprc file (atop)
*/
#define	PACCTDIR	"/var/run"

/*
** accounting file (source file to which kernel writes records)
*/
#define PACCTORIG	"pacct_source"		// file name in PACCTDIR

#define MAXORIGSZ	(1024*1024)		// maximum size of source file

/*
** file and directory names for shadow files
*/
#define PACCTSHADOWD	"pacct_shadow.d"	// directory name in PACCTDIR
#define PACCTSHADOWF	"%s/%s/%010ld.paf"	// file name of shadow file
#define PACCTSHADOWC	"current"		// file containining current
						// sequence and MAXSHADOWREC

#define MAXSHADOWREC	10000 	// number of accounting records per shadow file
