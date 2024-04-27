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

#ifndef __RAWLOG__
#define __RAWLOG__

/*
** structure describing the raw file contents
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
#define READAHEADOFF	22
#define READAHEADSIZE	(1 << READAHEADOFF)

struct rawheader {
	unsigned int	magic;

	unsigned short	aversion;	/* creator atop version with MSB */
	unsigned short	future1;	/* can be reused 		 */
	unsigned short	future2;	/* can be reused 		 */
	unsigned short	rawheadlen;	/* length of struct rawheader    */
	unsigned short	rawreclen;	/* length of struct rawrecord    */
	unsigned short	hertz;		/* clock interrupts per second   */
	unsigned short	pidwidth;	/* number of digits for PID/TID  */
	unsigned short	sfuture[5];	/* future use                    */
	unsigned int	sstatlen;	/* length of struct sstat        */
	unsigned int	tstatlen;	/* length of struct tstat        */
	struct utsname	utsname;	/* info about this system        */
	char		cfuture[8];	/* future use                    */

	unsigned int	pagesize;	/* size of memory page (bytes)   */
	int		supportflags;  	/* used features                 */
	int		osrel;		/* OS release number             */
	int		osvers;		/* OS version number             */
	int		ossub;		/* OS version subnumber          */
	int		cstatlen;	/* length of struct cstat        */
	int		ifuture[5];	/* future use                    */
};

struct rawrecord {
	time_t		curtime;	/* current time (epoch)         */

	unsigned short	flags;		/* various flags                */
	unsigned short	ncgroups;	/* number of cgroups 		*/
	unsigned short	sfuture[2];	/* future use                   */

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
	unsigned int    totidle;	/* number of idle     threads(I)*/
	unsigned int	ccomplen;	/* length of compressed cstats  */
	unsigned int	coriglen;	/* length of original   cstats	*/
	unsigned int	ncgpids;	/* number of cgroups pidlist 	*/
	unsigned int	icomplen;	/* length of compressed pidlist */
	unsigned int	ifuture;	/* future use                   */
};
#endif
