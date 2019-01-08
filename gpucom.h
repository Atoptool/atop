#ifndef	__GPUCOM__
#define	__GPUCOM__

#define APIVERSION	1

struct gpupidstat {
	long		pid;
	struct gpu	gpu;
};

int	gpud_init(void);
int	gpud_statrequest(void);
int	gpud_statresponse(int, struct pergpu *, struct gpupidstat **);

void	gpumergeproc(struct tstat      *, int,
                     struct tstat      *, int,
                     struct gpupidstat *, int);
#endif
