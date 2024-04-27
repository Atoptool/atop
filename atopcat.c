/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of 
** the system on system-level as well as process-level.
**
** This program concatenates several raw logfiles into one output stream,
** to be stored as new file or to be passed via a pipe to atop/atopsar directly.
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Initial:     March 2020
** --------------------------------------------------------------------------
** Copyright (C) 2020 Gerlof Langeveld
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
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "atop.h"
#include "rawlog.h"

char	*convepoch(time_t);
void	prusage(char *);

int
main(int argc, char *argv[])
{
	int			i, fd, n, c;
	int			firstfile, beverbose=0, dryrun=0;
	struct rawheader	rh;
	struct rawrecord	rr;
	char			*infile, *sstat, *pstat, *cstat, *istat;
	unsigned short		aversion;

	// verify the command line arguments: input filename(s)
	//
	if (argc < 2)
		prusage(argv[0]);

	while ((c = getopt(argc, argv, "?hvd")) != EOF)
	{
		switch (c)
		{
		   case '?':            	// usage wanted?
		   case 'h':            	// usage wanted?
			prusage(argv[0]);
			break;

		   case 'v': 			// verbosity wanted?
			beverbose = 1;
			break;

		   case 'd': 			// dry run wanted?
			dryrun = 1;
			break;

		   default:
			prusage(argv[0]);
		}
	}

	if ( isatty(fileno(stdout)) && !dryrun)
	{
		fprintf(stderr,
			"this program produces binary output on stdout "
			"that should be redirected\nto a file or pipe!\n");
		exit(1);
	}

	// open all input files one-by-one
	//
	for (i=optind, firstfile=1; i < argc; i++, firstfile=0)
	{
		infile = argv[i];

		// open raw file for reading
		//
		if ( (fd = open(infile, O_RDONLY)) == -1)
		{
			fprintf(stderr, "%s - ", infile);
			perror("open for reading");
			exit(2);
		}

		// read the raw header
		//
		if ( read(fd, &rh, sizeof rh) < sizeof rh)
		{
			fprintf(stderr, "%s: cannot read raw header\n", infile);
			close(fd);
			exit(3);
		}

		// verify if this is a correct rawlog file
		//
		if (rh.magic != MYMAGIC)
		{
			fprintf(stderr, "%s: not a valid rawlog file "
				"(wrong magic number)\n", infile);
			close(fd);
			exit(4);
		}

		// only for the first file, store the version number and write
		// the raw header for the entire stream
		//
		// for the next files, be sure that the version is the same
		// as the first file
		//
		if (firstfile)
		{
			aversion = rh.aversion;

			if (!dryrun)
			{
				if ( write(1, &rh, sizeof rh) < sizeof rh)
				{
					fprintf(stderr,
						"can not write raw header\n");
					exit(10);
				}
			}

			if (beverbose)
			{
				fprintf(stderr,
				     "Logs created by atop version %d.%d\n\n", 
                        	     (rh.aversion >> 8) & 0x7f,
				      rh.aversion & 0xff);

				fprintf(stderr, "%-10s %-8s %12s  %8s  %9s  %8s %8s\n",
					"date", "time", "interval",
					"comprsys", "comprproc", "comprcgr", "comppids");
			}
					
		}
		else	// subsequent file
		{
			if (aversion != rh.aversion)
			{
				fprintf(stderr,
					"Version of file %s is unequal to "
					"version of first file\n", infile);
				close(fd);
				exit(5);
			}
		}

		// read every raw record followed by the compressed
		// system-level stats, process-level stats,
		// cgroup-level stats and pidlist.
		//
		while ( read(fd, &rr, sizeof rr) == sizeof rr )
		{
			if (beverbose)
			{
				fprintf(stderr, "%19s %12u  %8u  %9u  %8u %8u  %s\n",
					convepoch(rr.curtime),
					rr.interval, rr.scomplen, rr.pcomplen,
					rr.ccomplen, rr.icomplen,
					rr.flags&RRBOOT ? "boot" : "");
			}

			// dynamically allocate space to read stats
			// 
			if ( (sstat = malloc(rr.scomplen)) == NULL)
			{
				fprintf(stderr, "malloc failed for sstat\n");
				exit(7);
			}

			if ( (pstat = malloc(rr.pcomplen)) == NULL)
			{
				fprintf(stderr, "malloc failed for pstat\n");
				exit(7);
			}

			if ( (cstat = malloc(rr.ccomplen)) == NULL)
			{
				fprintf(stderr, "malloc failed for cstat\n");
				exit(7);
			}

			if ( (istat = malloc(rr.icomplen)) == NULL)
			{
				fprintf(stderr, "malloc failed for istat\n");
				exit(7);
			}

			// read system-level stats
			// 
			if ((n = read(fd, sstat, rr.scomplen)) != rr.scomplen)
			{
				if (n == -1)
				{
					fprintf(stderr, "read file %s", infile);
					perror("");
					exit(8);
				}
				else
				{
					fprintf(stderr,
					     "file %s incomplete!\n", infile);

					free(sstat);
					free(pstat);
					free(cstat);
					free(istat);
					break;
				}
			}

			// read process-level stats
			// 
			if ((n = read(fd, pstat, rr.pcomplen)) != rr.pcomplen)
			{
				if (n == -1)
				{
					fprintf(stderr, "read file %s", infile);
					perror("");
					exit(8);
				}
				else
				{
					fprintf(stderr,
					     "file %s incomplete!\n", infile);

					free(sstat);
					free(pstat);
					free(cstat);
					free(istat);
					break;
				}
			}

			// read cgroup-level stats
			// 
			if ((n = read(fd, cstat, rr.ccomplen)) != rr.ccomplen)
			{
				if (n == -1)
				{
					fprintf(stderr, "read file %s", infile);
					perror("");
					exit(8);
				}
				else
				{
					fprintf(stderr,
					     "file %s incomplete!\n", infile);

					free(sstat);
					free(pstat);
					free(cstat);
					free(istat);
					break;
				}
			}

			// read compressed pidlist
			// 
			if ((n = read(fd, istat, rr.icomplen)) != rr.icomplen)
			{
				if (n == -1)
				{
					fprintf(stderr, "read file %s", infile);
					perror("");
					exit(8);
				}
				else
				{
					fprintf(stderr,
					     "file %s incomplete!\n", infile);

					free(sstat);
					free(pstat);
					free(cstat);
					free(istat);
					break;
				}
			}


			if (!dryrun)
			{
				// write raw record followed by the compressed
				// system-level stats, process-level stats,
				// cgroup-level stats and pidlist
				//
				if ( write(1, &rr, sizeof rr) < sizeof rr)
				{
					fprintf(stderr, "can not write raw record\n");
					exit(11);
				}

				if ( write(1, sstat, rr.scomplen) < rr.scomplen)
				{
					fprintf(stderr, "can not write sstat\n");
					exit(11);
				}

				if ( write(1, pstat, rr.pcomplen) < rr.pcomplen)
				{
					fprintf(stderr, "can not write pstat\n");
					exit(11);
				}

				if ( write(1, cstat, rr.ccomplen) < rr.ccomplen)
				{
					fprintf(stderr, "can not write cstat\n");
					exit(11);
				}

				if ( write(1, istat, rr.icomplen) < rr.icomplen)
				{
					fprintf(stderr, "can not write istat\n");
					exit(11);
				}
			}

			// free dynamically allocated buffers
			//
			free(sstat);
			free(pstat);
			free(cstat);
			free(istat);
		}

		close(fd);
	}

	return 0;
}

// Function to convert an epoch time to date-time format
//
char *
convepoch(time_t utime)
{
	struct tm       *tt;
	static char	datetime[64];
	

	tt = localtime(&utime);

	sprintf(datetime, "%04d/%02d/%02d %02d:%02d:%02d",
                tt->tm_year+1900, tt->tm_mon+1, tt->tm_mday,
		tt->tm_hour, tt->tm_min, tt->tm_sec);

        return datetime;
}

// Function that shows the usage message
//
void
prusage(char *name)
{
	fprintf(stderr, "Usage: %s [-dv] rawfile [rawfile]...\n", name);
	fprintf(stderr, "\t-d\tdry run (no raw output generated)\n");
	fprintf(stderr, "\t-v\tbe verbose\n");
	exit(1);
}
