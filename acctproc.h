/*
** ATOP - System & Process Monitor
**
** The program 'atop' offers the possibility to view the activity of
** the system on system-level as well as process-level.
**
** Include-file for process-accounting functions.
** ================================================================
** Author:      Gerlof Langeveld
** E-mail:      gerlof.langeveld@atoptool.nl
** Date:        November 1996
** LINUX-port:  June 2000
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
*/

#ifndef __ACCTPROC__
#define __ACCTPROC__

int 		acctswon(void);
void		acctswoff(void);
unsigned long 	acctprocnt(void);
unsigned long	acctphotoproc(struct tstat *, int);
void 		acctrepos(unsigned int);

/*
** maximum number of records to be read from process accounting file
** for one sample, to avoid that atop explodes and introduces OOM killing ....
**
** the maximum is based on a limit of 50 MiB extra memory (approx. 70000 procs)
*/
#define MAXACCTPROCS	(50*1024*1024/sizeof(struct tstat))

/*
** preferred maximum size of process accounting file (200 MiB)
*/
#define ACCTMAXFILESZ	(200*1024*1024)

/*
** alternative layout of accounting record if kernel-patch
** has been installed
*/
#include <linux/types.h>
typedef __u16   comp_t;
typedef __u32   comp2_t;
#define ACCT_COMM	16

struct acct_atop
{
	char		ac_flag;		/* Flags */
	char		ac_version;		/* Always set to ACCT_VERSION */
	__u32		ac_pid;			/* Process ID */
	__u32		ac_ppid;		/* Parent Process ID */
	__u16		ac_uid16;		/* LSB of Real User ID */
	__u16		ac_gid16;		/* LSB of Real Group ID */
	__u16		ac_tty;			/* Control Terminal */
	__u32		ac_btime;		/* Process Creation Time */
	comp_t		ac_utime;		/* User Time */
	comp_t		ac_stime;		/* System Time */
	comp_t		ac_etime;		/* Elapsed Time */
	comp_t		ac_mem;			/* Virtual  Memory */
	comp_t		ac_rss;			/* Resident Memory */
	comp_t		ac_io;			/* Chars Transferred */
	comp_t		ac_rw;			/* Blocks Read or Written */
	comp_t		ac_bread;		/* Blocks Read */
	comp_t		ac_bwrite;		/* Blocks Written */
	comp2_t		ac_dskrsz;		/* Cum. blocks read */
	comp2_t		ac_dskwsz;		/* Cum. blocks written */
	comp_t		ac_tcpsnd;		/* TCP send requests */
	comp_t		ac_tcprcv;		/* TCP recv requests */
	comp2_t		ac_tcpssz;		/* TCP cum. length   */
	comp2_t		ac_tcprsz;		/* TCP cum. length   */
	comp_t		ac_udpsnd;		/* UDP send requests */
	comp_t		ac_udprcv;		/* UDP recv requests */
	comp2_t		ac_udpssz;		/* UDP cum. length   */
	comp2_t		ac_udprsz;		/* UDP cum. length   */
	comp_t		ac_rawsnd;		/* RAW send requests */
	comp_t		ac_rawrcv;		/* RAW recv requests */
	comp_t		ac_minflt;		/* Minor Pagefaults */
	comp_t		ac_majflt;		/* Major Pagefaults */
	comp_t		ac_swaps;		/* Number of Swaps */
/* m68k had no padding here. */
#if !defined(CONFIG_M68K) || !defined(__KERNEL__)
	__u16		ac_ahz;			/* AHZ */
#endif
	__u32		ac_exitcode;		/* Exitcode */
	char		ac_comm[ACCT_COMM + 1];	/* Command Name */
	__u8		ac_etime_hi;		/* Elapsed Time MSB */
	__u16		ac_etime_lo;		/* Elapsed Time LSB */
	__u32		ac_uid;			/* Real User ID */
	__u32		ac_gid;			/* Real Group ID */
};

/*
** default layout of accounting record
** (copied from /usr/src/linux/include/linux/acct.h)
*/

struct acct
{
	char		ac_flag;		/* Flags */
	char		ac_version;		/* Always set to ACCT_VERSION */
	/* for binary compatibility back until 2.0 */
	__u16		ac_uid16;		/* LSB of Real User ID */
	__u16		ac_gid16;		/* LSB of Real Group ID */
	__u16		ac_tty;			/* Control Terminal */
	__u32		ac_btime;		/* Process Creation Time */
	comp_t		ac_utime;		/* User Time */
	comp_t		ac_stime;		/* System Time */
	comp_t		ac_etime;		/* Elapsed Time */
	comp_t		ac_mem;			/* Average Memory Usage */
	comp_t		ac_io;			/* Chars Transferred */
	comp_t		ac_rw;			/* Blocks Read or Written */
	comp_t		ac_minflt;		/* Minor Pagefaults */
	comp_t		ac_majflt;		/* Major Pagefaults */
	comp_t		ac_swaps;		/* Number of Swaps */
/* m68k had no padding here. */
#if !defined(CONFIG_M68K) || !defined(__KERNEL__)
	__u16		ac_ahz;			/* AHZ */
#endif
	__u32		ac_exitcode;		/* Exitcode */
	char		ac_comm[ACCT_COMM + 1];	/* Command Name */
	__u8		ac_etime_hi;		/* Elapsed Time MSB */
	__u16		ac_etime_lo;		/* Elapsed Time LSB */
	__u32		ac_uid;			/* Real User ID */
	__u32		ac_gid;			/* Real Group ID */
};

struct acct_v3
{
	char		ac_flag;		/* Flags */
	char		ac_version;		/* Always set to ACCT_VERSION */
	__u16		ac_tty;			/* Control Terminal */
	__u32		ac_exitcode;		/* Exitcode */
	__u32		ac_uid;			/* Real User ID */
	__u32		ac_gid;			/* Real Group ID */
	__u32		ac_pid;			/* Process ID */
	__u32		ac_ppid;		/* Parent Process ID */
	__u32		ac_btime;		/* Process Creation Time */
#ifdef __KERNEL__
	__u32		ac_etime;		/* Elapsed Time */
#else
	float		ac_etime;		/* Elapsed Time */
#endif
	comp_t		ac_utime;		/* User Time */
	comp_t		ac_stime;		/* System Time */
	comp_t		ac_mem;			/* Average Memory Usage */
	comp_t		ac_io;			/* Chars Transferred */
	comp_t		ac_rw;			/* Blocks Read or Written */
	comp_t		ac_minflt;		/* Minor Pagefaults */
	comp_t		ac_majflt;		/* Major Pagefaults */
	comp_t		ac_swaps;		/* Number of Swaps */
	char		ac_comm[ACCT_COMM];	/* Command Name */
};

#endif
