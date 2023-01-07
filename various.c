/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** This source-file contains various functions to a.o. format the
** time-of-day, the cpu-time consumption and the memory-occupation. 
** ==========================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
** --------------------------------------------------------------------------
** Copyright (C) 2000-2022 Gerlof Langeveld
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
** $Log: various.c,v $
** Revision 1.21  2010/11/12 06:16:16  gerlof
** Show all parts of timestamp in header line, even when zero.
**
** Revision 1.20  2010/05/18 19:21:08  gerlof
** Introduce CPU frequency and scaling (JC van Winkel).
**
** Revision 1.19  2010/04/28 18:21:11  gerlof
** Cast value larger than 4GB to long long.
**
** Revision 1.18  2010/04/23 12:19:35  gerlof
** Modified mail-address in header.
**
** Revision 1.17  2010/03/26 11:52:45  gerlof
** Introduced unit of Tbytes for memory-usage.
**
** Revision 1.16  2009/12/17 08:28:38  gerlof
** Express CPU-time usage in days and hours for large values.
**
** Revision 1.15  2009/12/10 08:50:39  gerlof
** Introduction of a new function to convert number of seconds
** to a string indicating days, hours, minutes and seconds.
**
** Revision 1.14  2007/02/13 10:32:47  gerlof
** Removal of external declarations.
** Removal of function getpagesz().
**
** Revision 1.13  2006/02/07 08:27:21  gerlof
** Add possibility to show counters per second.
** Modify presentation of CPU-values.
**
** Revision 1.12  2005/10/31 12:26:09  gerlof
** Modified date-format to yyyy/mm/dd.
**
** Revision 1.11  2005/10/21 09:51:29  gerlof
** Per-user accumulation of resource consumption.
**
** Revision 1.10  2004/05/06 09:46:24  gerlof
** Ported to kernel-version 2.6.
**
** Revision 1.9  2003/07/07 09:27:46  gerlof
** Cleanup code (-Wall proof).
**
** Revision 1.8  2003/07/03 11:16:59  gerlof
** Minor bug solutions.
**
** Revision 1.7  2003/06/30 11:31:17  gerlof
** Enlarge counters to 'long long'.
**
** Revision 1.6  2003/06/24 06:22:24  gerlof
** Limit number of system resource lines.
**
** Revision 1.5  2002/08/30 07:49:09  gerlof
** Convert a hh:mm string into a number of seconds since 00:00.
**
** Revision 1.4  2002/08/27 12:08:37  gerlof
** Modified date format (from yyyy/mm/dd to mm/dd/yyyy).
**
** Revision 1.3  2002/07/24 11:14:05  gerlof
** Changed to ease porting to other UNIX-platforms.
**
** Revision 1.2  2002/07/11 09:43:36  root
** Modified HZ into sysconf(_SC_CLK_TCK).
**
** Revision 1.1  2001/10/02 10:43:36  gerlof
** Initial revision
**
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

#include "atop.h"
#include "acctproc.h"

/*
** Function convtime() converts a value (number of seconds since
** 1-1-1970) to an ascii-string in the format hh:mm:ss, stored in
** chartim (9 bytes long).
*/
char *
convtime(time_t utime, char *chartim)
{
	struct tm 	*tt;

	tt = localtime(&utime);

	snprintf(chartim, 9, "%02d:%02d:%02d", tt->tm_hour, tt->tm_min, tt->tm_sec);

	return chartim;
}

/*
** Function convdate() converts a value (number of seconds since
** 1-1-1970) to an ascii-string in the format yyyy/mm/dd, stored in
** chardat (11 bytes long).
*/
char *
convdate(time_t utime, char *chardat)
{
	struct tm 	*tt;

	tt = localtime(&utime);

	snprintf(chardat, 11, "%04u/%02u/%02u",
		(tt->tm_year+1900)%10000, (tt->tm_mon+1)%100, tt->tm_mday%100);

	return chardat;
}


/*
** Convert a string in format [YYYYMMDD]hh[:]mm into an epoch time value or
** when only the value hh[:]mm was given, take this time from midnight.
**
** Arguments:		String with date-time in format [YYYYMMDD]hh[:]mm
** 			or hh[:]mm.
**
**			Pointer to time_t containing 0 or current epoch time.
**
** Return-value:	0 - Wrong input-format
**			1 - Success
*/
int
getbranchtime(char *itim, time_t *newtime)
{
	register int	ilen = strlen(itim);
	int		hours, minutes;
	time_t		epoch;
	struct tm	tm;

	memset(&tm, 0, sizeof tm);

	/*
	** verify length of input string
	*/
	if (ilen != 4 && ilen != 5 && ilen != 12 && ilen != 13)
		return 0;		// wrong date-time format

	/*
	** check string syntax for absolute time specified as
	** YYYYMMDDhh:mm or YYYYMMDDhhmm
	*/
	if ( sscanf(itim, "%4d%2d%2d%2d:%2d", &tm.tm_year, &tm.tm_mon,
				&tm.tm_mday,  &tm.tm_hour, &tm.tm_min) == 5 ||
	     sscanf(itim, "%4d%2d%2d%2d%2d",  &tm.tm_year, &tm.tm_mon,
				&tm.tm_mday,  &tm.tm_hour, &tm.tm_min) == 5   )
	{
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

	/*
	** check string syntax for relative time specified as
	** hh:mm or hhmm
	*/
	if ( sscanf(itim, "%2d:%2d", &hours, &minutes) == 2	||
	     sscanf(itim, "%2d%2d",  &hours, &minutes) == 2 	  )
	{
		if ( hours < 0 || hours > 23 || minutes < 0 || minutes > 59 )
			return 0;	// wrong date-time format

		/*
		** when the new time is already filled with an epoch time,
		** the relative time will be on the same day as indicated by
		** that epoch time
		** when the new time is the time within a day or 0, the new
		** time will be stored again as the time within a day.
		*/
		if (*newtime <= SECONDSINDAY)	// time within the day?
		{
			*newtime = (hours * 3600) + (minutes * 60);

			if (*newtime >= SECONDSINDAY)
				*newtime = SECONDSINDAY-1;

			return 1;
		}
		else
		{
			*newtime = normalize_epoch(*newtime,
						(hours*3600) + (minutes*60));
			return 1;
		}
	}

	return 0;	// wrong date-time format
}


/*
** Normalize an epoch time with the number of seconds within a day
** Return-value:        Normalized epoch 
*/
time_t
normalize_epoch(time_t epoch, long secondsinday)
{
	struct tm	tm;

	localtime_r(&epoch, &tm);	// convert epoch to tm

	tm.tm_hour   = 0;	
	tm.tm_min    = 0;	
	tm.tm_sec    = secondsinday;	
	tm.tm_isdst  = -1;

	return mktime(&tm);		// convert tm to epoch
}


/*
** Function val2valstr() converts a positive value to an ascii-string of a 
** fixed number of positions; if the value does not fit, it will be formatted
** to exponent-notation (as short as possible, so not via the standard printf-
** formatters %f or %e). The offered buffer should have a length of width+1.
** The value might even be printed as an average for the interval-time.
*/
char *
val2valstr(count_t value, char *strvalue, int width, int avg, int nsecs)
{
	count_t		maxval, remain = 0;
	unsigned short	exp     = 0;
	char		*suffix = "";
 	int		strsize = width+1;

	if (avg && nsecs)
	{
		value  = (value + (nsecs/2)) / nsecs;     /* rounded value */
		width  = width - 2;	/* subtract two positions for '/s' */
		suffix = "/s";
	}

	if (value < 0)		// no negative value expected
	{
		snprintf(strvalue, strsize, "%*s%s", width, "?", suffix);
		return strvalue;
	}

	maxval = pow(10.0, width) - 1;

	if (value < maxval)
	{
		snprintf(strvalue, strsize, "%*lld%s", width, value, suffix);
	}
	else
	{
		if (width < 3)
		{
			/*
			** cannot avoid ignoring width
			*/
			snprintf(strvalue, strsize, "%lld%s", value, suffix);
		}
		else
		{
			/*
			** calculate new maximum value for the string,
			** calculating space for 'e' (exponent) + one digit
			*/
			width -= 2;
			maxval = pow(10.0, width) - 1;

			while (value > maxval)
			{
				exp++;
				remain = value % 10;
				value /= 10;
			}

			if (remain >= 5 && value < maxval)
				value++;

			snprintf(strvalue, strsize, "%*llde%hd%s",
					width%100, value, exp%100, suffix);
		}
	}

	return strvalue;
}

#define DAYSECS 	(24*60*60)
#define HOURSECS	(60*60)
#define MINSECS 	(60)

/*
** Function val2elapstr() converts a value (number of seconds)
** to an ascii-string of up to max 13 positions in NNNdNNhNNmNNs
** stored in strvalue (at least 14 positions).
** returnvalue: number of bytes stored
*/
int
val2elapstr(int value, char *strvalue)
{
        char	*p = strvalue;
	int	rv, n = 14;

        if (value >= DAYSECS) 
        {
                rv = snprintf(p, n, "%dd", value/DAYSECS);
		p += rv;
		n -= rv;
        }

        if (value >= HOURSECS) 
        {
                rv = snprintf(p, n, "%dh", (value%DAYSECS)/HOURSECS);
		p += rv;
		n -= rv;
        }

        if (value >= MINSECS) 
        {
                rv = snprintf(p, n, "%dm", (value%HOURSECS)/MINSECS);
		p += rv;
		n -= rv;
        }

        rv = snprintf(p, n, "%ds", (value%MINSECS));
	p += rv;
	n -= rv;

        return p - strvalue;
}


/*
** Function val2cpustr() converts a value (number of milliseconds)
** to an ascii-string of 6 positions in milliseconds or minute-seconds or
** hours-minutes, stored in strvalue (at least 7 positions).
*/
#define	MAXMSEC		(count_t)100000
#define	MAXSEC		(count_t)6000
#define	MAXMIN		(count_t)6000

char *
val2cpustr(count_t value, char *strvalue)
{
	if (value < MAXMSEC)
	{
		snprintf(strvalue, 7, "%2llu.%02llus",
				(value/1000)%100, value%1000/10);
	}
	else
	{
	        /*
       	 	** millisecs irrelevant; round to seconds
       	 	*/
        	value = (value + 500) / 1000;

        	if (value < MAXSEC) 
        	{
               	 	snprintf(strvalue, 7, "%2llum%02llus",
						(value/60)%100, value%60);
		}
		else
		{
			/*
			** seconds irrelevant; round to minutes
			*/
			value = (value + 30) / 60;

			if (value < MAXMIN) 
			{
				snprintf(strvalue, 7, "%2lluh%02llum",
						(value/60)%100, value%60);
			}
			else
			{
				/*
				** minutes irrelevant; round to hours
				*/
				value = (value + 30) / 60;

				snprintf(strvalue, 7, "%2llud%02lluh",
						(value/24)%100, value%24);
			}
		}
	}

	return strvalue;
}

/*
** Function val2Hzstr() converts a value (in MHz) 
** to an ascii-string.
** The result-string is placed in the area pointed to strvalue,
** which should be able to contain 7 positions plus null byte.
*/
char *
val2Hzstr(count_t value, char *strvalue)
{
	char *fformat;

        if (value < 1000)
        {
                snprintf(strvalue, 8, "%4lluMHz", value%10000);
        }
        else
        {
                double fval=value/1000.0;      // fval is double in GHz
                char prefix='G';

                if (fval >= 1000.0)            // prepare for the future
                {
                        prefix='T';        
                        fval /= 1000.0;
                }

                if (fval < 10.0)
		{
			 fformat = "%4.2f%cHz";
		}
		else
		{
			if (fval < 100.0)
				fformat = "%4.1f%cHz";
                	else
				fformat = "%4.0f%cHz";
		}

                snprintf(strvalue, 8, fformat, fval, prefix);
        }

	return strvalue;
}


/*
** Function val2memstr() converts a value (number of bytes)
** to an ascii-string in a specific format (indicated by pformat).
** The result-string is placed in the area pointed to strvalue,
** which should be able to contain at least 7 positions.
*/
#define	ONEKBYTE	1024
#define	ONEMBYTE	1048576
#define	ONEGBYTE	1073741824L
#define	ONETBYTE	1099511627776LL
#define	ONEPBYTE	1125899906842624LL

#define	MAXBYTE		999
#define	MAXKBYTE	ONEKBYTE*999L
#define	MAXKBYTE9	ONEKBYTE*9L
#define	MAXMBYTE	ONEMBYTE*999L
#define	MAXMBYTE9	ONEMBYTE*9L
#define	MAXGBYTE	ONEGBYTE*999LL
#define	MAXGBYTE9	ONEGBYTE*9LL
#define	MAXTBYTE	ONETBYTE*999LL
#define	MAXTBYTE9	ONETBYTE*9LL
#define	MAXPBYTE9	ONEPBYTE*9LL

char *
val2memstr(count_t value, char *strvalue, int pformat, int avgval, int nsecs)
{
	char 	aformat;	/* advised format		*/
	count_t	verifyval;
	char	*suffix = "";
	int	basewidth = 6;

	/*
	** notice that the value can be negative, in which case the
	** modulo-value should be evaluated and an extra position should
	** be reserved for the sign
	*/
	if (value < 0)
		verifyval = -value * 10;
	else
		verifyval =  value;

	/*
	** verify if printed value is required per second (average) or total
	*/
	if (avgval && nsecs)
	{
		value         = llround((double)((double)value/(double)nsecs));
		verifyval     = llround((double)((double)verifyval/(double)nsecs));
		basewidth    -= 2;
		suffix        = "/s";

		if (verifyval <= MAXBYTE)	/* bytes ? */
		    aformat = BFORMAT;
		else
		    if (verifyval <= MAXKBYTE9)	/* kbytes 1-9 ? */
		        aformat = KBFORMAT;
		    else
		        if (verifyval <= MAXKBYTE)    /* kbytes ? */
		            aformat = KBFORMAT_INT;
		        else
		            if (verifyval <= MAXMBYTE9)    /* mbytes 1-9 ? */
		                aformat = MBFORMAT;
		            else
		                if (verifyval <= MAXMBYTE)    /* mbytes 10-999 ? */
		                    aformat = MBFORMAT_INT;
		                else
		                    if (verifyval <= MAXGBYTE9)    /* gbytes 1-9 ? */
		                        aformat = GBFORMAT;
		                    else
		                        if (verifyval <= MAXGBYTE)    /* gbytes 10-999 ? */
		                            aformat = GBFORMAT_INT;
		                        else
		                            if (verifyval <= MAXTBYTE9)    /* tbytes 1-9 ? */
		                                aformat = TBFORMAT;
		                            else
		                                if (verifyval <= MAXTBYTE)    /* tbytes 10-999? */
		                                    aformat = TBFORMAT_INT;
		                                else
		                                    if (verifyval <= MAXPBYTE9)    /* pbytes 1-9 ? */
		                                        aformat = PBFORMAT;
		                                    else
		                                        aformat = PBFORMAT_INT;    /* pbytes! */
	} else 
	/*
	** printed value per interval (normal mode) 
	*/
	{
		/*
		** determine which format will be used on bases of the value itself
		*/
		if (verifyval <= MAXBYTE)	/* bytes ? */
			aformat = BFORMAT;
		else
			if (verifyval <= MAXKBYTE)	/* kbytes ? */
				aformat = KBFORMAT;
			else
				if (verifyval <= MAXMBYTE)	/* mbytes ? */
					aformat = MBFORMAT;
				else
					if (verifyval <= MAXGBYTE)	/* gbytes ? */
						aformat = GBFORMAT;
					else
						if (verifyval <= MAXTBYTE)    /* tbytes? */
							aformat = TBFORMAT;
						else
							aformat = PBFORMAT;   /* pbytes! */
	}


	/*
	** check if this is also the preferred format
	*/
	if (aformat <= pformat)
		aformat = pformat;

	switch (aformat)
	{
	   case	BFORMAT:
		snprintf(strvalue, 7, "%*lldB%s",
				basewidth-1, value, suffix);
		break;

	   case	KBFORMAT:
		snprintf(strvalue, 7, "%*.1lfK%s",
			basewidth-1, (double)((double)value/ONEKBYTE), suffix); 
		break;

	   case	KBFORMAT_INT:
		snprintf(strvalue, 7, "%*lldK%s",
				basewidth-1, llround((double)((double)value/ONEKBYTE)), suffix);
		break;

	   case	MBFORMAT:
		snprintf(strvalue, 7, "%*.1lfM%s",
			basewidth-1, (double)((double)value/ONEMBYTE), suffix); 
		break;

	   case	MBFORMAT_INT:
		snprintf(strvalue, 7, "%*lldM%s",
			basewidth-1, llround((double)((double)value/ONEMBYTE)), suffix); 
		break;

	   case	GBFORMAT:
		snprintf(strvalue, 7, "%*.1lfG%s",
			basewidth-1, (double)((double)value/ONEGBYTE), suffix);
		break;

	   case	GBFORMAT_INT:
		snprintf(strvalue, 7, "%*lldG%s",
			basewidth-1, llround((double)((double)value/ONEGBYTE)), suffix);
		break;

	   case	TBFORMAT:
		snprintf(strvalue, 7, "%*.1lfT%s",
			basewidth-1, (double)((double)value/ONETBYTE), suffix);
		break;

	   case	TBFORMAT_INT:
		snprintf(strvalue, 7, "%*lldT%s",
			basewidth-1, llround((double)((double)value/ONETBYTE)), suffix);
		break;

	   case	PBFORMAT:
		snprintf(strvalue, 7, "%*.1lfP%s",
			basewidth-1, (double)((double)value/ONEPBYTE), suffix);
		break;

	   case	PBFORMAT_INT:
		snprintf(strvalue, 7, "%*lldP%s",
			basewidth-1, llround((double)((double)value/ONEPBYTE)), suffix);
		break;

	   default:
		snprintf(strvalue, 7, "!TILT!");
	}

	// check if overflow occurred during the formatting
	// by checking the last byte of the formatted string
	//
	switch ( *(strvalue+5) )
	{
	   case 's':		// in case of per-second value
	   case 'B':
	   case 'K':
	   case 'M':
	   case 'G':
	   case 'T':
	   case 'P':
		break;

	   default:
		snprintf(strvalue, 7, "OVFLOW");
	}

	return strvalue;
}


/*
** Function numeric() checks if the ascii-string contains 
** a numeric (positive) value.
** Returns 1 (true) if so, or 0 (false).
*/
int
numeric(char *ns)
{
	register char *s = ns;

	while (*s)
		if (*s < '0' || *s > '9')
			return(0);		/* false */
		else
			s++;
	return(1);				/* true  */
}


/*
** Function getboot() returns the boot-time of this system 
** as number of jiffies since 1-1-1970.
*/
unsigned long long
getboot(void)
{
	static unsigned long long	boottime;
	unsigned long long		getbootlinux(long);

	if (!boottime)		/* do this only once */
		boottime = getbootlinux(hertz);

	return boottime;
}

/*
** generic pointer verification after malloc
*/
void
ptrverify(const void *ptr, const char *errormsg, ...)
{
	if (!ptr)
	{
		va_list args;

		acctswoff();
		netatop_signoff();

		if (vis.show_end)
			(vis.show_end)();

		va_start(args, errormsg);
		vfprintf(stderr, errormsg, args);
		va_end(args);

		exit(13);
	}
}

/*
** cleanup, give error message and exit
*/
void
mcleanstop(int exitcode, const char *errormsg, ...)
{
	va_list args;

	acctswoff();
	netatop_signoff();
	(vis.show_end)();

	va_start(args, errormsg);
	vfprintf(stderr, errormsg, args);
	va_end(args);

	exit(exitcode);
}

/*
** cleanup, give error message and exit
*/
void
cleanstop(int exitcode)
{
	acctswoff();
	netatop_signoff();
	(vis.show_end)();

	exit(exitcode);
}

/*
** drop the root privileges that might be obtained via setuid-bit
**
** this action may only fail with errno EPERM (normal situation when
** atop has not been started with setuid-root privs); when this
** action fails with EAGAIN or ENOMEM, atop should not continue
** without root privs being dropped...
*/
int
droprootprivs(void)
{
	if (seteuid( getuid() ) == -1 && errno != EPERM)
		return 0;	/* false */
	else
		return 1;	/* true  */
}

/*
** regain the root privileges that might be dropped earlier
*/
void
regainrootprivs(void)
{
	int liResult;

	// this will fail for non-privileged processes
	liResult = seteuid(0);

	if (liResult != 0)
	{
	}
}

/*
** try to set the highest OOM priority
*/
void
set_oom_score_adj(void)
{
	int fd;
	char val[] = "-1000";	/* suggested by Gerlof, always set -1000 */

	/*
	 ** set OOM score adj to avoid to lost necessary log of system.
	 ** ignored if not running under superuser privileges!
	 */
	fd = open("/proc/self/oom_score_adj", O_RDWR);
	if ( fd < 0 ) {
		return;
	}

	if ( write(fd, val, strlen(val)) < 0 )
		;

	close(fd);
}

/* hypervisor enum, move this into header if actually in use */
enum {
	HYPER_NONE	= 0,
	HYPER_XEN,
	HYPER_KVM,
	HYPER_MSHV,
	HYPER_VMWARE,
	HYPER_IBM,
	HYPER_VSERVER,
	HYPER_UML,
	HYPER_INNOTEK,
	HYPER_HITACHI,
	HYPER_PARALLELS,
	HYPER_VBOX,
	HYPER_OS400,
	HYPER_PHYP,
	HYPER_SPAR,
	HYPER_WSL,
};

#if defined(__x86_64__) || defined(__i386__)
#define HYPERVISOR_INFO_LEAF   0x40000000

static inline void
x86_cpuid(unsigned int op, unsigned int *eax, unsigned int *ebx,
			 unsigned int *ecx, unsigned int *edx)
{
	__asm__(
#if defined(__PIC__) && defined(__i386__)
		"xchg %%ebx, %%esi;"
		"cpuid;"
		"xchg %%esi, %%ebx;"
		: "=S" (*ebx),
#else
		"cpuid;"
		: "=b" (*ebx),
#endif
		  "=a" (*eax),
		  "=c" (*ecx),
		  "=d" (*edx)
		: "1" (op), "c"(0));
}

static int
get_hypervisor(void)
{
	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0, hyper = HYPER_NONE;
	char hyper_vendor_id[13];

	memset(hyper_vendor_id, 0, sizeof(hyper_vendor_id));

	x86_cpuid(HYPERVISOR_INFO_LEAF, &eax, &ebx, &ecx, &edx);
	memcpy(hyper_vendor_id + 0, &ebx, 4);
	memcpy(hyper_vendor_id + 4, &ecx, 4);
	memcpy(hyper_vendor_id + 8, &edx, 4);
	hyper_vendor_id[12] = '\0';

	if (!hyper_vendor_id[0])
		return hyper;

	if (!strncmp("XenVMMXenVMM", hyper_vendor_id, 12))
		hyper = HYPER_XEN;
	else if (!strncmp("KVMKVMKVM", hyper_vendor_id, 9))
		hyper = HYPER_KVM;
	else if (!strncmp("Microsoft Hv", hyper_vendor_id, 12))
		hyper = HYPER_MSHV;
	else if (!strncmp("VMwareVMware", hyper_vendor_id, 12))
		hyper = HYPER_VMWARE;
	else if (!strncmp("UnisysSpar64", hyper_vendor_id, 12))
		hyper = HYPER_SPAR;

	return hyper;
}
#else /* ! (__x86_64__ || __i386__) */
static int
get_hypervisor(void)
{
	return HYPER_NONE;
}
#endif

int
run_in_guest(void)
{
	return get_hypervisor() != HYPER_NONE;
}

/*
** return maximum number of digits for PID/TID
*/
int
getpidwidth(void)
{
	FILE 	*fp;
	char	linebuf[64];
	int	numdigits = 5;

        if ( (fp = fopen("/proc/sys/kernel/pid_max", "r")) != NULL)
        {
                if ( fgets(linebuf, sizeof(linebuf), fp) != NULL)
                {
                        numdigits = strlen(linebuf) - 1;
                }

                fclose(fp);
        }

	return numdigits;
}
