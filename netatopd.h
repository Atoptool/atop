#define SEMAKEY         1541961

#define NETEXITFILE     "/var/run/netatop.log"
#define MYMAGIC         (unsigned int) 0xfeedb0b0

struct naheader {
        u_int32_t	magic;	// magic number MYMAGIC
        u_int32_t	curseq;	// sequence number of last netpertask
        u_int16_t	hdrlen;	// length of this header
        u_int16_t	ntplen;	// length of netpertask structure
        pid_t    	mypid;	// PID of netatopd itself
};
