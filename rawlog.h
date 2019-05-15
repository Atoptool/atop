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
#define DROPCACHEOFF	24
#define DROPCACHESIZE	(1 << DROPCACHEOFF)
#define DROPFULLCACHEOFF	26

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
