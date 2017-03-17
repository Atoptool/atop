/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of 
** the system on system-level as well as process-level.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        September 2002
** --------------------------------------------------------------------------
** Copyright (C) 2000-2010 Gerlof Langeveld
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
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <string.h>
#include <regex.h>
#include <zlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "atop.h"
#include "showgeneric.h"
#include "photoproc.h"
#include "photosyst.h"

#define	BASEPATH	"/var/log/atop"  

/*
** structure which describes the raw file contents
**
** layout raw file:    rawheader
**
**                     rawrecord                           \
**                     compressed system-level statistics   | sample 1
**                     compressed process-level statistics /
**
**                     rawrecord                           \
**                     compressed system-level statistics   | sample 2
**                     compressed process-level statistics /
**
** etcetera .....
*/
#define	MYMAGIC		(unsigned int) 0xfeedbeef

struct rawheader {
	unsigned int	magic;

	unsigned short	aversion;	/* creator atop version with MSB */
	unsigned short	future1;	/* can be reused 		 */
	unsigned short	future2;	/* can be reused 		 */
	unsigned short	rawheadlen;	/* length of struct rawheader    */
	unsigned short	rawreclen;	/* length of struct rawrecord    */
	unsigned short	hertz;		/* clock interrupts per second   */
	unsigned short	sfuture[6];	/* future use                    */
	unsigned int	sstatlen;	/* length of struct sstat        */
	unsigned int	tstatlen;	/* length of struct tstat        */
	struct utsname	utsname;	/* info about this system        */
	char		cfuture[8];	/* future use                    */

	unsigned int	pagesize;	/* size of memory page (bytes)   */
	int		supportflags;  	/* used features                 */
	int		osrel;		/* OS release number             */
	int		osvers;		/* OS version number             */
	int		ossub;		/* OS version subnumber          */
	int		ifuture[6];	/* future use                    */
};

struct rawrecord {
	time_t		curtime;	/* current time (epoch)         */

	unsigned short	flags;		/* various flags                */
	unsigned short	sfuture[3];	/* future use                   */

	unsigned int	scomplen;	/* length of compressed sstat   */
	unsigned int	pcomplen;	/* length of compressed tstat's */
	unsigned int	interval;	/* interval (number of seconds) */
	unsigned int	ndeviat;	/* number of tasks in list      */
	unsigned int	nactproc;	/* number of processes in list  */
	unsigned int	ntask;		/* total number of tasks        */
	unsigned int    totproc;	/* total number of processes	*/
	unsigned int    totrun;		/* number of running  threads	*/
	unsigned int    totslpi;	/* number of sleeping threads(S)*/
	unsigned int    totslpu;	/* number of sleeping threads(D)*/
	unsigned int	totzomb;	/* number of zombie processes   */
	unsigned int	nexit;		/* number of exited processes   */
	unsigned int	noverflow;	/* number of overflow processes */
	unsigned int	ifuture[6];	/* future use                   */
};

static int	getrawrec  (int, struct rawrecord *, int);
static int	getrawsstat(int, struct sstat *, int);
static int	getrawtstat(int, struct tstat *, int, int);
static int	rawwopen(void);
static int	lookslikedatetome(char *);
static void	testcompval(int, char *);
static void	try_other_version(int, int);

/*
** write a raw record to file
** (file is opened/created during the first call)
*/
char
rawwrite(time_t curtime, int numsecs, 
         struct devtstat *devtstat, struct sstat *sstat,
         int nexit, unsigned int noverflow, char flag)
{
	static int		rawfd = -1;
	struct rawrecord	rr;
	int			rv;
	struct stat		filestat;

	Byte			scompbuf[sizeof(struct sstat)], *pcompbuf;
	unsigned long		scomplen = sizeof scompbuf;
	unsigned long		pcomplen = sizeof(struct tstat) *
						devtstat->ntaskall;

	/*
	** first call:
	**	take care that the log file is opened
	*/
	if (rawfd == -1)
		rawfd = rawwopen();

	/*
 	** register current size of file in order to "roll back"
	** writes that have been done while not *all* writes could
	** succeed, e.g. when file system full
	*/
	(void) fstat(rawfd, &filestat);

	/*
	** compress system- and process-level statistics
	*/
	rv = compress(scompbuf, &scomplen,
				(Byte *)sstat, (unsigned long)sizeof *sstat);

	testcompval(rv, "compress");

	pcompbuf = malloc(pcomplen);

	ptrverify(pcompbuf, "Malloc failed for compression buffer\n");

	rv = compress(pcompbuf, &pcomplen, (Byte *)devtstat->taskall,
						(unsigned long)pcomplen);

	testcompval(rv, "compress");

	/*
	** fill record header and write to file
	*/
	memset(&rr, 0, sizeof rr);

	rr.curtime	= curtime;
	rr.interval	= numsecs;
	rr.flags	= 0;
	rr.ndeviat	= devtstat->ntaskall;
	rr.nactproc	= devtstat->nprocactive;
	rr.ntask	= devtstat->ntaskall;
	rr.nexit	= nexit;
	rr.noverflow	= noverflow;
	rr.totproc	= devtstat->nprocall;
	rr.totrun	= devtstat->totrun;
	rr.totslpi	= devtstat->totslpi;
	rr.totslpu	= devtstat->totslpu;
	rr.totzomb	= devtstat->totzombie;
	rr.scomplen	= scomplen;
	rr.pcomplen	= pcomplen;

	if (flag&RRBOOT)
		rr.flags |= RRBOOT;

	if (supportflags & ACCTACTIVE)
		rr.flags |= RRACCTACTIVE;

	if (supportflags & IOSTAT)
		rr.flags |= RRIOSTAT;

	if (supportflags & NETATOP)
		rr.flags |= RRNETATOP;

	if (supportflags & NETATOPD)
		rr.flags |= RRNETATOPD;

	if (supportflags & DOCKSTAT)
		rr.flags |= RRDOCKSTAT;

	if ( write(rawfd, &rr, sizeof rr) == -1)
	{
		fprintf(stderr, "%s - ", rawname);
		perror("write raw record");
		if ( ftruncate(rawfd, filestat.st_size) == -1)
			cleanstop(8);
		cleanstop(7);
	}

	/*
	** write compressed system status structure to file
	*/
	if ( write(rawfd, scompbuf, scomplen) == -1)
	{
		fprintf(stderr, "%s - ", rawname);
		perror("write raw status record");
		if ( ftruncate(rawfd, filestat.st_size) == -1)
			cleanstop(8);
		cleanstop(7);
	}

	/*
	** write compressed list of process status structures to file
	*/
	if ( write(rawfd, pcompbuf, pcomplen) == -1)
	{
		fprintf(stderr, "%s - ", rawname);
		perror("write raw process record");
		if ( ftruncate(rawfd, filestat.st_size) == -1)
			cleanstop(8);
		cleanstop(7);
	}

	free(pcompbuf);

	return '\0';
}


/*
** open a raw file for writing
**
** if the raw file exists already:
**    - read and validate the header record (be sure it is an atop-file)
**    - seek to the end of the file
**
** if the raw file does not yet exist:
**    - create the raw file
**    - write a header record
**
** return the filedescriptor of the raw file
*/
static int
rawwopen()
{
	struct rawheader	rh;
	int			fd;

	/*
	** check if the file exists already
	*/
	if ( (fd = open(rawname, O_RDWR)) >= 0)
	{
		/*
		** read and verify header record
		*/
		if ( read(fd, &rh, sizeof rh) < sizeof rh)
		{
			fprintf(stderr, "%s - cannot read header\n", rawname);
			cleanstop(7);
		}

		if (rh.magic != MYMAGIC)
		{
			fprintf(stderr,
				"file %s exists but does not contain raw "
				"atop output (wrong magic number)\n", rawname);

			cleanstop(7);
		}

		if ( rh.sstatlen	!= sizeof(struct sstat)		||
		     rh.tstatlen	!= sizeof(struct tstat)		||
	    	     rh.rawheadlen	!= sizeof(struct rawheader)	||
		     rh.rawreclen	!= sizeof(struct rawrecord)	  )
		{
			fprintf(stderr,
				"existing file %s has incompatible header\n",
				rawname);

			if (rh.aversion & 0x8000 &&
			   (rh.aversion & 0x7fff) != getnumvers())
			{
				fprintf(stderr,
					"(created by version %d.%d - "
					"current version %d.%d)\n",
					(rh.aversion >> 8) & 0x7f,
					 rh.aversion & 0xff,
					 getnumvers() >> 8,
					 getnumvers() & 0x7f);
			}

			cleanstop(7);
		}

		(void) lseek(fd, (off_t) 0, SEEK_END);

		return fd;
	}

	/*
	** file does not exist (or can not be opened)
	*/
	if ( (fd = creat(rawname, 0666)) == -1)
	{
		fprintf(stderr, "%s - ", rawname);
		perror("create raw file");
		cleanstop(7);
	}

	memset(&rh, 0, sizeof rh);

	rh.magic	= MYMAGIC;
	rh.aversion	= getnumvers() | 0x8000;
	rh.sstatlen	= sizeof(struct sstat);
	rh.tstatlen	= sizeof(struct tstat);
	rh.rawheadlen	= sizeof(struct rawheader);
	rh.rawreclen	= sizeof(struct rawrecord);
	rh.supportflags	= supportflags | RAWLOGNG;
	rh.osrel	= osrel;
	rh.osvers	= osvers;
	rh.ossub	= ossub;
	rh.hertz	= hertz;
	rh.pagesize	= pagesize;

	memcpy(&rh.utsname, &utsname, sizeof rh.utsname);

	if ( write(fd, &rh, sizeof rh) == -1)
	{
		fprintf(stderr, "%s - ", rawname);
		perror("write raw header");
		cleanstop(7);
	}

	return fd;
}

/*
** read the contents of a raw file
*/
#define	OFFCHUNK	256

void
rawread(void)
{
	int			i, j, rawfd, len;
	char			*py;
	struct rawheader	rh;
	struct rawrecord	rr;
	struct sstat		sstat;
	static struct devtstat	devtstat;

	struct stat		filestat;

	/*
	** variables to maintain the offsets of the raw records
	** to be able to see previous samples again
	*/
	off_t			*offlist;
	unsigned int		offsize = 0;
	unsigned int		offcur  = 0;
	char			lastcmd = 'X', flags;

	time_t			timenow;
	struct tm		*tp;

	switch ( len = strlen(rawname) )
	{
	   /*
	   ** if no filename is specified, assemble the name of the raw file
	   */
	   case 0:
		timenow	= time(0);
		tp	= localtime(&timenow);

		snprintf(rawname, RAWNAMESZ, "%s/atop_%04d%02d%02d",
			BASEPATH, 
			tp->tm_year+1900,
			tp->tm_mon+1,
			tp->tm_mday);

		break;

	   /*
	   ** if date specified as filename in format YYYYMMDD, assemble
	   ** the full pathname of the raw file
	   */
	   case 8:
		if ( access(rawname, F_OK) == 0) 
			break;		/* existing file */

		if (lookslikedatetome(rawname))
		{
			char	savedname[RAWNAMESZ];

			strncpy(savedname, rawname, RAWNAMESZ-1);

			snprintf(rawname, RAWNAMESZ, "%s/atop_%s",
				BASEPATH, 
				savedname);
			break;
		}

	   /*
	   ** if one or more 'y' (yesterday) characters are used and that
	   ** string is not known as an existing file, the standard logfile
	   ** is shown from N days ago (N is determined by the number
	   ** of y's).
	   */
	   default:
		if ( access(rawname, F_OK) == 0) 
			break;		/* existing file */

		/*
		** make a string existing of y's to compare with
		*/
		py = malloc(len+1);

		ptrverify(py, "Malloc failed for 'yes' sequence\n");

		memset(py, 'y', len);
		*(py+len) = '\0';

		if ( strcmp(rawname, py) == 0 )
		{
			timenow	 = time(0);
			timenow -= len*3600*24;
			tp	 = localtime(&timenow);

			snprintf(rawname, RAWNAMESZ, "%s/atop_%04d%02d%02d",
				BASEPATH, 
				tp->tm_year+1900,
				tp->tm_mon+1,
				tp->tm_mday);
		}

		free(py);
	}

	/*
	** open raw file
	*/
	if ( (rawfd = open(rawname, O_RDONLY)) == -1)
	{
		char	command[512], tmpname1[RAWNAMESZ], tmpname2[RAWNAMESZ];

		/*
		** check if a compressed raw file is present
		*/
		snprintf(tmpname1, sizeof tmpname1, "%s.gz", rawname);

		if ( access(tmpname1, F_OK|R_OK) == -1)
		{
			fprintf(stderr, "%s - ", rawname);
			perror("open raw file");
			cleanstop(7);
		}

		/*
		** compressed raw file to be decompressed via gunzip
		*/
		fprintf(stderr, "Decompressing logfile ....\n");
		snprintf(tmpname2, sizeof tmpname2, "/tmp/atopwrkXXXXXX");
		rawfd = mkstemp(tmpname2);
		if (rawfd == -1)
		{
			fprintf(stderr, "%s - ", rawname);
			perror("creating decompression temp file");
			cleanstop(7);
		}

		snprintf(command,  sizeof command, "gunzip -c %s > %s",
							tmpname1, tmpname2);
		const int system_res = system (command);
		unlink(tmpname2);

		if (system_res)
		{
			fprintf(stderr, "%s - gunzip failed", rawname);
			cleanstop(7);
		}
	}

	/*
	** read the raw header and verify the magic
	*/
	if ( read(rawfd, &rh, sizeof rh) < sizeof rh)
	{
		fprintf(stderr, "can not read raw file header\n");
		cleanstop(7);
	}

	if (rh.magic != MYMAGIC)
	{
		fprintf(stderr, "file %s does not contain raw atop/atopsar "
				"output (wrong magic number)\n", rawname);
		cleanstop(7);
	}

	/*
	** magic okay, but file-layout might have been modified
	*/
	if (rh.sstatlen   != sizeof(struct sstat)		||
	    rh.tstatlen   != sizeof(struct tstat)		||
	    rh.rawheadlen != sizeof(struct rawheader)		||
	    rh.rawreclen  != sizeof(struct rawrecord)		  )
	{
		fprintf(stderr,
			"raw file %s has incompatible format\n", rawname);

		if (rh.aversion & 0x8000 &&
       		   (rh.aversion & 0x7fff) != getnumvers())
		{
			fprintf(stderr,
				"(created by version %d.%d - "
				"current version %d.%d)\n",
				(rh.aversion >> 8) & 0x7f,
				 rh.aversion       & 0xff,
				 getnumvers() >> 8,
				 getnumvers() & 0x7f);
		}
		else
		{
			fprintf(stderr,
				"(files from other system architectures might"
				" be binary incompatible)\n");
		}

		close(rawfd);

		if (((rh.aversion >> 8) & 0x7f) != (getnumvers()   >> 8) ||
		     (rh.aversion       & 0xff) != (getnumvers() & 0x7f)   )
		{
			try_other_version((rh.aversion >> 8) & 0x7f,
			                   rh.aversion       & 0xff);
		}

		cleanstop(7);
	}

	memcpy(&utsname, &rh.utsname, sizeof utsname);
	utsnodenamelen = strlen(utsname.nodename);

	supportflags = rh.supportflags;
	osrel        = rh.osrel;
	osvers       = rh.osvers;
	ossub        = rh.ossub;
	interval     = 0;

	if (rh.hertz)
		hertz    = rh.hertz;

	if (rh.pagesize)
		pagesize = rh.pagesize;

	/*
	** allocate a list for backtracking of rawrecord-offsets
	*/
	offlist = malloc(sizeof(off_t) * OFFCHUNK);

	ptrverify(offlist, "Malloc failed for backtrack list\n");

	offsize = OFFCHUNK;

	*offlist = lseek(rawfd, 0, SEEK_CUR);
	offcur   = 1;

	/*
	** read a raw record header until end-of-file
	*/
	sampcnt = 0;

	while (lastcmd && lastcmd != 'q')
	{
		while ( getrawrec(rawfd, &rr, rh.rawreclen) == rh.rawreclen)
		{
			unsigned int	secsinday = daysecs(rr.curtime);
			unsigned int	k, l;

			/*
			** store the offset of the raw record in the offset list
			** in case of offset list overflow, extend the list
			*/
			*(offlist+offcur) = lseek(rawfd, 0, SEEK_CUR) -
								rh.rawreclen;

			if ( ++offcur >= offsize )
			{
				offlist = realloc(offlist,
				             (offsize+OFFCHUNK)*sizeof(off_t));

				ptrverify(offlist,
				        "Realloc failed for backtrack list\n");

				offsize+= OFFCHUNK;
			}
	
			/*
			** check if this sample is within the time-range
			** specified with the -b and -e flags (if any)
			*/
			if ( (begintime && begintime > secsinday) )
			{
				lastcmd = 1;
				lseek(rawfd, rr.scomplen+rr.pcomplen, SEEK_CUR);
				continue;
			}

			begintime = 0;	// allow earlier times from now on

			if ( (endtime && endtime < secsinday) )
			{
				free(offlist);
				close(rawfd);
				return;
			}

			/*
			** allocate space, read compressed system-level
			** statistics and decompress
			*/
			if ( !getrawsstat(rawfd, &sstat, rr.scomplen) )
				cleanstop(7);

			/*
			** allocate space, read compressed process-level
			** statistics and decompress
			*/
			devtstat.taskall    = malloc(sizeof(struct tstat  )
								* rr.ndeviat);

			if (rr.totproc < rr.nactproc)	// compat old raw files
				devtstat.procall = malloc(sizeof(struct tstat *)
								* rr.nactproc);
			else
				devtstat.procall = malloc(sizeof(struct tstat *)
								* rr.totproc);

			devtstat.procactive = malloc(sizeof(struct tstat *) * rr.nactproc);

			ptrverify(devtstat.taskall,
			          "Malloc failed for %d stored tasks\n",
			          rr.ndeviat);

			ptrverify(devtstat.procall,
			          "Malloc failed for total %d processes\n",
			          rr.totproc);

			ptrverify(devtstat.procactive,
			          "Malloc failed for %d active processes\n",
			          rr.nactproc);

			if ( !getrawtstat(rawfd, devtstat.taskall,
						rr.pcomplen, rr.ndeviat) )
				cleanstop(7);


			for (i=j=k=l=0; i < rr.ndeviat; i++)
			{
				if ( (devtstat.taskall+i)->gen.isproc)
				{
					devtstat.procall[j++] = devtstat.taskall+i;

					if (! (devtstat.taskall+i)->gen.wasinactive)
						devtstat.procactive[k++] = devtstat.taskall+i;
				}

				if (! (devtstat.taskall+i)->gen.wasinactive)
					l++;
			}

			devtstat.ntaskall	= i;
			devtstat.nprocall	= j;
			devtstat.nprocactive	= k;
			devtstat.ntaskactive	= l;

 			devtstat.totrun		= rr.totrun;
 			devtstat.totslpi	= rr.totslpi;
 			devtstat.totslpu	= rr.totslpu;
 			devtstat.totzombie	= rr.totzomb;

			/*
			** activate the installed print-function to visualize
			** the system- and process-level statistics
			*/
			sampcnt++;

			if ( (rh.supportflags & RAWLOGNG) == RAWLOGNG)
			{
				if (rr.flags & RRACCTACTIVE)
					supportflags |=  ACCTACTIVE;
				else
					supportflags &= ~ACCTACTIVE;

				if (rr.flags & RRIOSTAT)
					supportflags |=  IOSTAT;
				else
					supportflags &= ~IOSTAT;
			}

			if (rr.flags & RRNETATOP)
				supportflags |=  NETATOP;
			else
				supportflags &= ~NETATOP;

			if (rr.flags & RRNETATOPD)
				supportflags |=  NETATOPD;
			else
				supportflags &= ~NETATOPD;

			if (rr.flags & RRDOCKSTAT)
				supportflags |=  DOCKSTAT;
			else
				supportflags &= ~DOCKSTAT;

			flags = rr.flags & RRBOOT;

			(void) fstat(rawfd, &filestat);

			if ( filestat.st_size -
			     lseek(rawfd, (off_t)0, SEEK_CUR) <= rh.rawreclen)
				flags |= RRLAST;

			lastcmd = (vis.show_samp)(rr.curtime, rr.interval,
			             &devtstat, &sstat,
			             rr.nexit, rr.noverflow, flags);

			free(devtstat.taskall);
			free(devtstat.procall);
			free(devtstat.procactive);
	
			switch (lastcmd)
			{
			   case MSAMPPREV:
				if (offcur >= 2)
					offcur-= 2;
				else
					offcur = 0;
	
				lseek(rawfd, *(offlist+offcur), SEEK_SET);
				break;

			   case MRESET:
				lseek(rawfd, *offlist, SEEK_SET);
				offcur = 1;
				break;

			   case MSAMPBRANCH:
				if (begintime && begintime < secsinday)
				{
					lseek(rawfd, *offlist, SEEK_SET);
					offcur = 1;
				}
			}
		}

		begintime = 0;	// allow earlier times from now on

		if (offcur >= 1)
			offcur--;

		lseek(rawfd, *(offlist+offcur), SEEK_SET);
	}

	free(offlist);

	close(rawfd);
}

/*
** read the next raw record from the raw logfile
*/
static int
getrawrec(int rawfd, struct rawrecord *prr, int rrlen)
{
	return read(rawfd, prr, rrlen);
}

/*
** read the system-level statistics from the current offset
*/
static int
getrawsstat(int rawfd, struct sstat *sp, int complen)
{
	Byte		*compbuf;
	unsigned long	uncomplen = sizeof(struct sstat);
	int		rv;

	if ( (compbuf = malloc(complen)) == NULL)

	ptrverify(compbuf, "Malloc failed for reading compressed sysstats\n");

	if ( read(rawfd, compbuf, complen) < complen)
	{
		free(compbuf);
		return 0;
	}

	rv = uncompress((Byte *)sp, &uncomplen, compbuf, complen);

	testcompval(rv, "uncompress");

	free(compbuf);

	return 1;
}

/*
** read the process-level statistics from the current offset
*/
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
		return 0;
	}

	rv = uncompress((Byte *)pp, &uncomplen, compbuf, complen);

	testcompval(rv, "uncompress");

	free(compbuf);

	return 1;
}

/* 
** verify if a particular ascii-string is in the format yyyymmdd
*/
static int
lookslikedatetome(char *p)
{
	register int 	i;

	for (i=0; i < 8; i++)
		if ( !isdigit(*(p+i)) )
			return 0;

	if (*p != '2')
		return 0;	/* adapt this in the year 3000 */

	if ( *(p+4) > '1')
		return 0;

	if ( *(p+6) > '3')
		return 0;

	return 1;	/* yes, looks like a date to me */
}

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
		fprintf(stderr, "atop/atopsar - "
		        "%s: failed due to lack of memory\n", func);
		cleanstop(7);

	   case Z_BUF_ERROR:
		fprintf(stderr, "atop/atopsar - "
			"%s: failed due to lack of room in buffer\n", func);
		cleanstop(7);

   	   case Z_DATA_ERROR:
		fprintf(stderr, "atop/atopsar - "
		        "%s: failed due to corrupted/incomplete data\n", func);
		cleanstop(7);

	   default:
		fprintf(stderr, "atop/atopsar - "
		        "%s: unexpected error %d\n", func, rv);
		cleanstop(7);
	}
}

/*
** try to activate another atop- or atopsar-version
** to read this logfile
*/
static void
try_other_version(int majorversion, int minorversion)
{
	char		tmpbuf[1024], *p;
	extern char	**argvp;
	int		fds;
	struct rlimit	rlimit;
	int 		setresuid(uid_t, uid_t, uid_t);

	/*
 	** prepare name of executable file
	** the current pathname (if any) is stripped off
	*/
	snprintf(tmpbuf, sizeof tmpbuf, "%s-%d.%d",
		(p = strrchr(*argvp, '/')) ? p+1 : *argvp,
			majorversion, minorversion);

	fprintf(stderr, "trying to activate %s....\n", tmpbuf);

	/*
	** be sure no open file descriptors are passed
	** except stdin, stdout en stderr
	*/
	(void) getrlimit(RLIMIT_NOFILE, &rlimit);

	for (fds=3; fds < rlimit.rlim_cur; fds++)
		close(fds);

	/*
	** be absolutely sure not to pass setuid-root privileges
	** to the loaded program; errno EAGAIN and ENOMEM are not
	** acceptable!
	*/
	if ( setresuid(getuid(), getuid(), getuid()) == -1 && errno != EPERM)
	{
		fprintf(stderr, "not possible to drop root-privileges!\n");
		exit(1);
	}

	/*
 	** load alternative executable image
	** at this moment the saved-uid might still be set
	** to 'root' but this is reset at the moment of exec
	*/
	(void) execvp(tmpbuf, argvp);

	/*
	** point of no return, except when exec failed
	*/
	fprintf(stderr, "activation of %s failed!\n", tmpbuf);
}
