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
**
** $Log: rawlog.c,v $
** Revision 1.32  2010/11/26 06:06:35  gerlof
** Cosmetic.
**
** Revision 1.31  2010/11/17 12:43:31  gerlof
** The flag -r followed by exactly 8 'y' characters is not considered
** as 8 days ago, but as a literal filename.
**
** Revision 1.30  2010/10/23 14:03:03  gerlof
** Counters for total number of running and sleep threads (JC van Winkel).
**
** Revision 1.29  2010/04/23 13:55:52  gerlof
** Get rid of all setuid-root privs before activating other program.
**
** Revision 1.28  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.27  2010/04/16 12:55:16  gerlof
** Automatically start another version of atop if the logfile to
** be read has not been created by the current version.
**
** Revision 1.26  2010/03/02 13:55:29  gerlof
** Struct stat size did not fit in short any more (modified to int).
**
** Revision 1.25  2009/12/17 08:16:06  gerlof
** Introduce branch-key to go to specific time in raw file.
**
** Revision 1.24  2009/11/27 15:26:29  gerlof
** Added possibility to specify y[y..] als filename for -r flag
** to access file of yesterday, day before yesterday, etc.
**
** Revision 1.23  2009/11/27 14:28:14  gerlof
** Rollback a "transaction" when not all parts could be
** written to the logfile (e.g. file system full) to avoid
** a corrupted logfile.
**
** Revision 1.22  2008/01/07 10:18:05  gerlof
** Implement possibility to make summaries.
**
** Revision 1.21  2007/08/16 12:01:25  gerlof
** Add support for atopsar reporting.
**
** Revision 1.20  2007/03/20 13:02:25  gerlof
** Introduction of variable supportflags.
**
** Revision 1.19  2007/03/20 11:11:57  gerlof
** Verify success of malloc's.
**
** Revision 1.18  2007/03/20 07:25:59  gerlof
** Avoid loop when incompatible raw file is read.
** Verify return code of compress/uncompress functions.
**
** Revision 1.17  2007/02/23 07:34:00  gerlof
** Changed unsigned short's into unsigned int's for process-counters
** in raw record.
**
** Revision 1.16  2007/02/13 10:33:27  gerlof
** Removal of external declarations.
** Store pagesize and hertz in raw logfile.
**
** Revision 1.15  2006/01/30 09:12:34  gerlof
** Minor bug-fix.
**
** Revision 1.14  2005/10/21 09:50:36  gerlof
** Per-user accumulation of resource consumption.
**
** Revision 1.13  2004/12/14 15:06:23  gerlof
** Implementation of patch-recognition for disk and network-statistics.
**
** Revision 1.12  2004/05/06 09:47:36  gerlof
** Ported to kernel-version 2.6.
**
** Revision 1.11  2003/07/07 09:26:48  gerlof
** Cleanup code (-Wall proof).
**
** Revision 1.10  2003/07/03 12:04:11  gerlof
** Implemented subcommand `r' (reset).
**
** Revision 1.9  2003/06/27 12:32:48  gerlof
** Removed rawlog compatibility.
**
** Revision 1.8  2003/02/06 14:08:33  gerlof
** Cosmetic changes.
**
** Revision 1.7  2003/01/17 07:33:39  gerlof
** Modified process statistics: add command-line.
** Implement compatibility for old tstat-structure read from logfiles.
**
** Revision 1.6  2002/10/30 13:46:11  gerlof
** Generate notification for statistics since boot.
** Adapt interval from ushort to ulong (bug-solution);
** this results in incompatible logfile but old logfiles can still be read.
**
** Revision 1.5  2002/10/24 12:22:25  gerlof
** In case of writing a raw logfile:
** If the logfile already exists, append new records to the existing file.
** If the logfile does not yet exist, create it.
**
** Revision 1.4  2002/10/08 11:35:22  gerlof
** Modified storage of raw filename.
**
** Revision 1.3  2002/10/03 10:41:51  gerlof
** Avoid end-less loop when using end-time specification (flag -e).
**
** Revision 1.2  2002/09/17 13:17:01  gerlof
** Allow key 'T' to be pressed to view previous sample in raw file.
**
**
*/

static const char rcsid[] = "$Id: rawlog.c,v 1.32 2010/11/26 06:06:35 gerlof Exp $";

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
	 struct sstat *ss, struct tstat *ts, struct tstat **proclist,
	 int ndeviat, int ntask, int nactproc,
         int totproc, int totrun, int totslpi, int totslpu, int totzomb, 
         int nexit, unsigned int noverflow, char flag)
{
	static int		rawfd = -1;
	struct rawrecord	rr;
	int			rv;
	struct stat		filestat;

	Byte			scompbuf[sizeof(struct sstat)], *pcompbuf;
	unsigned long		scomplen = sizeof scompbuf;
	unsigned long		pcomplen = sizeof(struct tstat) * ndeviat;

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
				(Byte *)ss, (unsigned long)sizeof *ss);

	testcompval(rv, "compress");

	pcompbuf = malloc(pcomplen);

	ptrverify(pcompbuf, "Malloc failed for compression buffer\n");

	rv = compress(pcompbuf, &pcomplen, (Byte *)ts, (unsigned long)pcomplen);

	testcompval(rv, "compress");

	/*
	** fill record header and write to file
	*/
	memset(&rr, 0, sizeof rr);

	rr.curtime	= curtime;
	rr.interval	= numsecs;
	rr.flags	= 0;
	rr.ndeviat	= ndeviat;
	rr.nactproc	= nactproc;
	rr.ntask	= ntask;
	rr.nexit	= nexit;
	rr.noverflow	= noverflow;
	rr.totproc	= totproc;
	rr.totrun	= totrun;
	rr.totslpi	= totslpi;
	rr.totslpu	= totslpu;
	rr.totzomb	= totzomb;
	rr.scomplen	= scomplen;
	rr.pcomplen	= pcomplen;

	if (flag&RRBOOT)
		rr.flags |= RRBOOT;

	if (supportflags & NETATOP)
		rr.flags |= RRNETATOP;

	if (supportflags & NETATOPD)
		rr.flags |= RRNETATOPD;

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
		     rh.rawreclen	!= sizeof(struct rawrecord)	||
		     rh.supportflags	!= (supportflags & ~(NETATOP|NETATOPD)))
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
	rh.supportflags	= (supportflags & ~(NETATOP|NETATOPD));
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
	struct sstat		devsstat;
	struct tstat		*devtstat;
	struct tstat		**devpstat;

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
			if ( !getrawsstat(rawfd, &devsstat, rr.scomplen) )
				cleanstop(7);

			/*
			** allocate space, read compressed process-level
			** statistics and decompress
			*/
			devtstat = malloc(sizeof(struct tstat) * rr.ndeviat);

			ptrverify(devtstat,
			          "Malloc failed for %d stored tasks\n",
			          rr.ndeviat);

			devpstat = malloc(sizeof(struct tstat *) * rr.nactproc);

			ptrverify(devpstat,
			          "Malloc failed for %d stored processes\n",
			          rr.nactproc);

			if ( !getrawtstat(rawfd, devtstat,
						rr.pcomplen, rr.ndeviat) )
				cleanstop(7);

			/*
			** activate the installed print-function to visualize
			** the system- and process-level statistics
			*/
			sampcnt++;

			if (rr.flags & RRNETATOP)
				supportflags |=  NETATOP;
			else
				supportflags &= ~NETATOP;

			if (rr.flags & RRNETATOPD)
				supportflags |=  NETATOPD;
			else
				supportflags &= ~NETATOPD;

			flags = rr.flags & RRBOOT;

			(void) fstat(rawfd, &filestat);

			if ( filestat.st_size -
			     lseek(rawfd, (off_t)0, SEEK_CUR) <= rh.rawreclen)
				flags |= RRLAST;

			for (i=j=0; i < rr.ndeviat; i++)
			{
				if ( (devtstat+i)->gen.isproc)
					devpstat[j++] = devtstat+i;
			}

			lastcmd = (vis.show_samp)(rr.curtime, rr.interval,
			             &devsstat, devtstat, devpstat,
		 	             rr.ndeviat,  rr.ntask, rr.nactproc,
			             rr.totproc, rr.totrun, rr.totslpi,
			             rr.totslpu, rr.totzomb, rr.nexit,
			             rr.noverflow, flags);

			free(devtstat);
			free(devpstat);
	
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
