/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of 
** the system on system-level as well as process-level.
**
** This program copies an input raw logfile to an output raw logfile,
** while offering the possibility to select a subset of the samples 
** (begin time and/or end time) and to anonymize the samples
** (subsitute command names/arguments, host name, logical volume names,
** etcetera by place holders).
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Initial:     July 2023
** --------------------------------------------------------------------------
** Copyright (C) 2023 Gerlof Langeveld
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
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <regex.h>
#include <zlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdarg.h>

#include "atop.h"
#include "photosyst.h"
#include "photoproc.h"
#include "rawlog.h"

// struct to register fakenames that are assigned
// to the original names
//
struct standin {
	char		*origname;
	char		*fakename;
	struct standin	*next;
};

// function prototypes
//
static int	openin(char *);
static void	readin(int, void *, int);
static int	openout(char *);
static void	writeout(int, void *, int);
static void	writesamp(int, struct rawrecord *,
			void *, int, void *, int, int,
			void *, int, void *, int);
static int	getrawsstat(int, struct sstat *, int);
static int	getrawtstat(int, struct tstat *, int, int);

static void	testcompval(int, char *);
static void	anonymize(struct sstat *, struct tstat *, int);
static char 	*findstandin(struct standin **, unsigned long *, char *, char *);

// command names that will not be anonymized (in alphabetical order)
//
static char	*allowedcoms[] = {
	"^0anacron$",
	"^agetty$", "^anacron$", "^atd$", "^atop", "^auditd$",
	"^avahi-", "^awk$",
	"^basename$", "^bash$", "^bc$", "^bunzip2$", "^bzip2$",
	"^cat$", "^chmod$", "^chmod$", "^chown$", "^chromium",
	"^chronyc$", "^chronyd$", "^cp$", "^cpio$", "^crond$",
	"^csh$", "^cut$",
	"^date$", "^dbus", "^dd$", "^df$", "^diff$",
	"^dig$", "^dircolors$", "^dirname$", "^dnf$",
	"^echo$", "^expr$",
	"^file$", "^find$", "^firefox$", "^firewalld$",
	"^gawk$", "^git$", "^grep$", "^grepconf.sh$",
	"^gunzip$", "^gzip$",
	"^head$", "^head$", "^host$", "^hostname$", "^hostnamectl$",
	"^id$", "^ip$", "^iptables$", "^irqbalance$",
	"^kill$", "^ksh$",
	"^ldconfig$", "^less$", "^ln$", "^locale$", "^locate$",
	"^logger$", "^logrotate$", "^ls$", "^lsmd$",
	"^man$", "^make$", "^mcelog$", "^mkdate$", "^mkdir$", "^mktemp$",
	"^modprobe$", "^more$", "^mount$", "^mv$",
	"^netatop", "^NetworkManager$", "^nice$", "^nl$",
	"^oom_",
	"^pr$", "^ps$", "^pwd$", "^python$", "^python3$",
	"^qemu-kvm$",
	"^readlink$", "^rm$", "^rmdir$", "^rpcbind$", "^rpc.imapd$",
	"^rpm$", "^rsyslogd$",
	"^scp$", "^sed$", "^sh$", "^sleep$", "^smartd$", "^sort$",
	"^ss$", "^ssh$", "^sshd$", "^stat$", "^su$", "^sudo$",
	"^systemctl$", "^systemd",
	"^tail$", "^tar$", "^tclsh$", "^tee$", "^thunderbird$",
	"^top$", "^touch$", "^tr$", "^tuned$",
	"^udevd", "^uname$", "^uniq$", "^unxz$", "^updatedb$",
	"^usecpu$", "^usemem$",
	"^vi$", "^vim$", "^vmtoolsd$",
	"^wc$", "^which$",
	"^xargs$", "^xz$", "^xzcat$",
	"^yum$",
	"^zcat$", "^zgrep$",
};

static regex_t *compreg;	// compiled REs of allowed command names


int
main(int argc, char *argv[])
{
	struct rawheader	rh;
	struct rawrecord        rr;

	struct sstat		sstat;
	struct tstat		*tstatp;
	struct cstat		*cstatp;
	char			*istatp;

	int			ifd, ofd=0, writecnt = 0, anonflag = 0;
	int			i, c, numallowedcoms = sizeof allowedcoms/sizeof(char *);
	char			*infile, *outfile;
	time_t			begintime = 0, endtime = 0;

	// verify the command line arguments:
	// 	mandatory input and output filename
	//
	if (argc < 3)
		prusage(argv[0]);


	while ((c = getopt(argc, argv, "ab:e:")) != EOF)
	{
		switch (c)
		{

		   case 'a':		// anonymize
			anonflag = 1;
			break;

		   case 'b':		// begin time
			if ( !getbranchtime(optarg, &begintime) )
				prusage(argv[0]);
			break;

		   case 'e':		// end time 
			if ( !getbranchtime(optarg, &endtime) )
				prusage(argv[0]);
			break;

		   default:
			prusage(argv[0]);
			break;
		}
	}

	if (optind >= argc)
		prusage(argv[0]);

	infile  = argv[optind++];

	if (optind >= argc)
		prusage(argv[0]);

	outfile = argv[optind++];

	// open the input file and verify magic number
	//
	if ( (ifd = openin(infile)) == -1)
	{
		prusage(argv[0]);
		exit(2);
	}

	readin(ifd, &rh, sizeof rh);

	if (rh.magic != MYMAGIC)
	{
		fprintf(stderr,
			"File %s does not contain atop/atopsar data "
			"(wrong magic number)\n", infile);
		exit(3);
	}

	if (rh.sstatlen   != sizeof(struct sstat)               ||
            rh.tstatlen   != sizeof(struct tstat)               ||
            rh.rawheadlen != sizeof(struct rawheader)           ||
            rh.rawreclen  != sizeof(struct rawrecord)             )
	{
		fprintf(stderr,
			"File %s created with incompatible version of atop "
			"or created on other CPU architecture\n", infile);
		exit(3);
	}

	// handle the output file 
	//
	if (strcmp(infile, outfile) == 0)
	{
		fprintf(stderr,
	 	    "Input file and output file should not be identical!\n");
		exit(12);
	}

	if (anonflag)	// anonymize wanted?
	{
		// allocate space for compiled command REs and compile all REs
		//
		compreg = malloc(sizeof(regex_t) * numallowedcoms);
		ptrverify(compreg, "Malloc failed for regex\n");

		for (i=0; i < numallowedcoms; i++)
			regcomp(&compreg[i], allowedcoms[i], REG_NOSUB);

		// anonymize host name
		//
		memset(rh.utsname.nodename, '\0', sizeof rh.utsname.nodename);
		strcpy(rh.utsname.nodename, "anonymized");
	}

	// read recorded samples and copy to output file
	//
	while ( read(ifd, &rr, rh.rawreclen) == rh.rawreclen)
        {
		// skip records that are recorded before specified begin time
		//
		if (begintime && begintime > rr.curtime)
		{
			(void) lseek(ifd, rr.scomplen, SEEK_CUR);
			(void) lseek(ifd, rr.pcomplen, SEEK_CUR);
			(void) lseek(ifd, rr.ccomplen, SEEK_CUR);
			(void) lseek(ifd, rr.icomplen, SEEK_CUR);
			continue;
		}

		// skip records that are recorded after specified end time
		//
		if (endtime && endtime < rr.curtime)
			break;

		// open the output file and write the rawheader once
		//
		if (writecnt++ == 0)
		{
			if ( (ofd = openout(outfile)) == -1)
			{
				prusage(argv[0]);
				exit(4);
			}

			writeout(ofd, &rh, sizeof rh);
		}

                // read compressed system-level statistics and decompress
                //
                if ( !getrawsstat(ifd, &sstat, rr.scomplen) )
                        exit(7);

                // read compressed process-level statistics and decompress
                //
                tstatp = malloc(sizeof(struct tstat) * rr.ndeviat);

                ptrverify(tstatp,
                        "Malloc failed for %d stored tasks\n", rr.ndeviat);

                if ( !getrawtstat(ifd, tstatp, rr.pcomplen, rr.ndeviat) )
                        exit(7);

                // get compressed cgroup-level statistics
                //
                cstatp = malloc(rr.ccomplen);

                ptrverify(cstatp, "Malloc failed for compressed pidlist\n");

		readin(ifd, cstatp, rr.ccomplen);

                // get compressed pidlist
                //
                istatp = malloc(rr.icomplen);

                ptrverify(istatp, "Malloc failed for compressed pidlist\n");

		readin(ifd, istatp, rr.icomplen);

		// anonymize command lines and hostname
		//
		if (anonflag)
			anonymize(&sstat, tstatp, rr.ndeviat);

		// write record header, system-level stats, process-level stats,
		// cgroup-level stats and pidlist
		//
		writesamp(ofd, &rr, &sstat, sizeof sstat,
		                    tstatp, sizeof *tstatp, rr.ndeviat,
				    cstatp, rr.ccomplen,
				    istatp, rr.icomplen);

                // cleanup
                // 
                free(tstatp);
                free(cstatp);
                free(istatp);
        }

	// close files
	//
	close(ifd);

	printf("Samples written: %d", writecnt);

	if (writecnt == 0)
		printf(" -- no output file created!\n");
	else
		printf("\n");

	return 0;
}


// Funtion to anonymize the command lines and host name
//
static	struct standin	*lvmhead;
static	unsigned long	lvmsequence;

static	struct standin	*nfshead;
static	unsigned long	nfssequence;

static	struct standin	*cmdhead;
static	unsigned long	cmdsequence;

static void
anonymize(struct sstat *ssp, struct tstat *tsp, int ntask)
{
	int	i, r, numallowedcoms = sizeof allowedcoms/sizeof(char *);
	char	*standin, *p;

	// anonimize system-level stats
	// - logical volume names
	//
	for (i=0; i < ssp->dsk.nlvm; i++)
	{
		standin = findstandin(&lvmhead, &lvmsequence,
		                      "logvol", ssp->dsk.lvm[i].name);

		memset(ssp->dsk.lvm[i].name, '\0', MAXDKNAM); 
		strncpy(ssp->dsk.lvm[i].name, standin, MAXDKNAM-1);
	}
	
	// anonimize system-level stats
	// - NFS mounted shares
	//
	for (i=0; i < ssp->nfs.nfsmounts.nrmounts; i++)
	{
		standin = findstandin(&nfshead, &nfssequence,
		                      "nfsmnt", ssp->nfs.nfsmounts.nfsmnt[i].mountdev);

		memset(ssp->nfs.nfsmounts.nfsmnt[i].mountdev, '\0',
					sizeof ssp->nfs.nfsmounts.nfsmnt[i].mountdev); 
		strncpy(ssp->nfs.nfsmounts.nfsmnt[i].mountdev, standin,
					sizeof ssp->nfs.nfsmounts.nfsmnt[i].mountdev - 1); 
	}
	
	// anonymize process-level stats
	// - preserve all kernel processes (i.e. processes without memory)
	// - all command line arguments will be removed
	// - selected command names will be kept
	//   (mainly standard commands and daemons)
	//
	for (i=0; i < ntask; i++, tsp++)
	{
		// preserve kernel processes
		//
		if (tsp->mem.vmem == 0 && tsp->gen.state != 'E')
			continue;

		// remove command line arguments
		//
		if ( (p = strchr(tsp->gen.cmdline, ' ')) )
			memset(p, '\0', CMDLEN-(p-tsp->gen.cmdline));

		// check all allowed names
		//
		for (r=0; r < numallowedcoms; r++)
		{
			// allowed name recognized: leave loop
			//
			if (regexec(&compreg[r], tsp->gen.name, 0, NULL, 0) == 0)
				break;
		}

		// when command name does not appear to be allowed,
		// replace command name by fake name
		//
		if (r == numallowedcoms)
		{
			standin = findstandin(&cmdhead, &cmdsequence,
		                      		"prog", tsp->gen.name);

			memset(tsp->gen.name, '\0', PNAMLEN+1);
			strncpy(tsp->gen.name, standin, PNAMLEN); 

			memset(tsp->gen.cmdline, '\0', CMDLEN+1);
			strncpy(tsp->gen.cmdline, standin, CMDLEN); 
		}
	}
}


// Function that searches for the original name and returns
// an anonymized replacement string.
// When the original string does not exist yet, a replacement
// string will be generated by using the prefix followed by a
// 5-digit sequence number. That replacement string is stored
// together with the original string for a subsequent search.
//
static char *
findstandin(struct standin **head, unsigned long *sequence,
            char *prefix, char *origp)
{
	struct standin	*sp;

	// first check if the original string can be found in the linked list
	//
	for (sp = *head; sp; sp = sp->next)
	{
		if ( strcmp(sp->origname, origp) == 0)
			return sp->fakename;	// found!
	}

	// original name not known yet
	// create a new entry in the linked list
	//
	sp = malloc(sizeof *sp);
	ptrverify(sp, "Malloc failed for standin struct\n");

	sp->origname = malloc(strlen(origp)+1);
	sp->fakename = malloc(strlen(prefix)+6);

	ptrverify(sp->origname, "Malloc failed for standin orig\n");
	ptrverify(sp->fakename, "Malloc failed for standin fake\n");

	strcpy(sp->origname, origp);
	snprintf(sp->fakename, strlen(prefix)+6, "%s%05lu", prefix, (*sequence)++);
	sp->next = *head;
	*head = sp;

	return sp->fakename;
}


// Function that opens an existing raw file and
// verifies the magic number
//
static int
openin(char *infile)
{
	int	rawfd;

	/*
	** open raw file for reading
	*/
	if ( (rawfd = open(infile, O_RDONLY)) == -1)
	{
		fprintf(stderr, "%s - ", infile);
		perror("open for reading");
		return -1;
	}

	return rawfd;
}


// Function that reads a chunk of bytes from
// the input raw file
//
static void
readin(int rawfd, void *buf, int size)
{
	/*
	** read the requested chunk and verify
	*/
	if ( read(rawfd, buf, size) < size)
	{
		fprintf(stderr, "can not read raw file\n");
		close(rawfd);
		exit(9);
	}
}



// Function that creates a raw log for output
//
static int
openout(char *outfile)
{
	int	rawfd;

	/*
	** create new output file
	*/
	if ( (rawfd = creat(outfile, 0666)) == -1)
	{
		fprintf(stderr, "%s - ", outfile);
		perror("create raw output file");
		return -1;
	}

	return rawfd;
}


// Function that reads a chunk of bytes from
// the input raw file
//
static void
writeout(int rawfd, void *buf, int size)
{
	/*
	** write the provided chunk and verify
	*/
	if ( write(rawfd, buf, size) < size)
	{
		fprintf(stderr, "can not write raw file\n");
		close(rawfd);
		exit(10);
	}
}


// Function that shows the usage message
//
void
prusage(char *name)
{
	fprintf(stderr,
		"Usage: %s [-a] [-b YYYYMMDDhhmm] [-e YYYYMMDDhhmm] "
		"rawin rawout\n\n", name);
	fprintf(stderr,
		"\t-a\tanonymize command names, host name, "
		"logical volume names, etc\n");
	fprintf(stderr,
		"\t-b\twrite output from specified begin time\n");
	fprintf(stderr,
		"\t-e\twrite output until specified end time\n");
	exit(1);
}



// Function to read the system-level statistics from the current offset
//
static int
getrawsstat(int rawfd, struct sstat *sp, int complen)
{
	Byte		*compbuf;
	unsigned long	uncomplen = sizeof(struct sstat);
	int		rv;

	compbuf = malloc(complen);

	ptrverify(compbuf, "Malloc failed for reading compressed sysstats\n");

	if ( read(rawfd, compbuf, complen) < complen)
	{
		free(compbuf);
		fprintf(stderr,
			"Failed to read %d bytes for system\n", complen);
		return 0;
	}

	rv = uncompress((Byte *)sp, &uncomplen, compbuf, complen);

	testcompval(rv, "uncompress");

	free(compbuf);

	return 1;
}


// Function to read the process-level statistics from the current offset
//
static int
getrawtstat(int rawfd, struct tstat *pp, int complen, int ndeviat)
{
	Byte		*compbuf;
	unsigned long	uncomplen = sizeof(struct tstat) * ndeviat;
	int		rv;

	compbuf = malloc(complen);

	ptrverify(compbuf, "Malloc failed for reading compressed procstats\n");

	if ( read(rawfd, compbuf, complen) < complen)
	{
		free(compbuf);
		fprintf(stderr,
			"Failed to read %d bytes for tasks\n", complen);
		return 0;
	}

	rv = uncompress((Byte *)pp, &uncomplen, compbuf, complen);

	testcompval(rv, "uncompress");

	free(compbuf);

	return 1;
}


#if 0
// Function to read the cgroup-level statistics
// from the current offset
//
static struct cstat *
getrawcstat(int rawfd, unsigned long ccomplen, unsigned long coriglen)
{
	Byte		*ccompbuf, *corigbuf;
	int		rv;

	/*
	** read all cstat structs
	*/
	ccompbuf = malloc(ccomplen);
	corigbuf = malloc(coriglen);

	ptrverify(ccompbuf, "Malloc failed for reading compressed cgroups\n");
	ptrverify(corigbuf, "Malloc failed for decompressing cgroups\n");

	readin(rawfd, ccompbuf, ccomplen);

	rv = uncompress((Byte *)corigbuf, &coriglen, ccompbuf, ccomplen);

	testcompval(rv, "uncompress cgroups");

	free(ccompbuf);

	return (struct cstat *)corigbuf;
}
#endif

// Function to write a new output sample from the current offset
//
static void
writesamp(int ofd, struct rawrecord *rr,
	 void *sstat, int sstatlen,
	 void *tstat, int tstatlen, int ntask,
	 void *cstat, int cstatlen,
	 void *istat, int istatlen)
{
	int			rv;
	Byte			scompbuf[sstatlen], *pcompbuf;
	unsigned long		scomplen = sizeof scompbuf;
	unsigned long		pcomplen = tstatlen * ntask;

	/*
	** compress system- and process-level statistics
	*/
	rv = compress(scompbuf, &scomplen,
				(Byte *)sstat, (unsigned long)sstatlen);

	testcompval(rv, "compress");

	pcompbuf = malloc(pcomplen);

	ptrverify(pcompbuf, "Malloc failed for compression buffer\n");

	rv = compress(pcompbuf, &pcomplen, (Byte *)tstat,
						(unsigned long)pcomplen);

	testcompval(rv, "compress");

	rr->scomplen	= scomplen;
	rr->pcomplen	= pcomplen;

	if ( write(ofd, rr, sizeof *rr) == -1)
	{
		perror("write raw record");
		exit(7);
	}

	/*
	** write compressed system status structure to file
	*/
	if ( write(ofd, scompbuf, scomplen) == -1)
	{
		perror("write raw status record");
		exit(7);
	}

	/*
	** write compressed list of process status structures to file
	*/
	if ( write(ofd, pcompbuf, pcomplen) == -1)
	{
		perror("write raw process records");
		exit(7);
	}

	free(pcompbuf);

	/*
	** write compressed cgroup status structures to file
	*/
	if ( write(ofd, cstat, cstatlen) == -1)
	{
		perror("write raw cgroup records");
		exit(7);
	}

	/*
	** write compressed PID list
	*/
	if ( write(ofd, istat, istatlen) == -1)
	{
		perror("write raw pidlist");
		exit(7);
	}
}


// check success of (de)compression
//
static void
testcompval(int rv, char *func)
{
	switch (rv)
	{
	   case Z_OK:
	   case Z_STREAM_END:
	   case Z_NEED_DICT:
		break;

	   case Z_MEM_ERROR:
		fprintf(stderr, "%s: failed due to lack of memory\n", func);
		exit(7);

	   case Z_BUF_ERROR:
		fprintf(stderr, "%s: failed due to lack of room in buffer\n",
								func);
		exit(7);

   	   case Z_DATA_ERROR:
		fprintf(stderr, 
		        "%s: failed due to corrupted/incomplete data\n", func);
		exit(7);

	   default:
		fprintf(stderr,
		        "%s: unexpected error %d\n", func, rv);
		exit(7);
	}
}


// generic pointer verification after malloc
//
void
ptrverify(const void *ptr, const char *errormsg, ...)
{
        if (!ptr)
        {
                va_list args;
                va_start(args, errormsg);
                vfprintf(stderr, errormsg, args);
                va_end  (args);

                exit(13);
        }
}


// Convert a string in format YYYYMMDDhhmm into an epoch time value.
//
// Arguments:		String with date-time in format YYYYMMDDhhmm
//			Pointer to time_t containing 0 or current epoch time.
//
// Return-value:	0 - Wrong input-format
//			1 - Success
//
int
getbranchtime(char *itim, time_t *newtime)
{
	register int	ilen = strlen(itim);
	time_t		epoch;
	struct tm	tm;

	memset(&tm, 0, sizeof tm);

	/*
	** verify length of input string
	*/
	if (ilen != 12)
		return 0;		// wrong date-time format

	/*
	** check string syntax for absolute time specified as
	** YYYYMMDDhhmm
	*/
	if ( sscanf(itim, "%4d%2d%2d%2d%2d", &tm.tm_year, &tm.tm_mon,
			       &tm.tm_mday,  &tm.tm_hour, &tm.tm_min) != 5)
		return 0;

	tm.tm_year -= 1900;
	tm.tm_mon  -= 1;

	if (tm.tm_year < 100 || tm.tm_mon  < 0  || tm.tm_mon > 11 ||
            tm.tm_mday < 1   || tm.tm_mday > 31 || 
	    tm.tm_hour < 0   || tm.tm_hour > 23 ||
	    tm.tm_min  < 0   || tm.tm_min  > 59   )
	{
		return 0;	// wrong date-time format
	}

	tm.tm_isdst = -1;

	if ((epoch = mktime(&tm)) == -1)
		return 0;	// wrong date-time format

	// correct date-time format
	*newtime = epoch;
	return 1;
}
