struct ifprop	{
	char		type;		/* type: 'e' (ethernet)		*/
					/*       'w' (wireless)  	*/
	char		name[31];	/* name of interface  		*/
	long int	speed;		/* in megabits per second	*/
	char		fullduplex;	/* boolean			*/
};

int 	getifprop(struct ifprop *);
void 	initifprop(void);
