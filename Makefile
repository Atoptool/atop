# Makefile for System & Process Monitor ATOP (Linux version)
#
# Gerlof Langeveld - gerlof.langeveld@atoptool.nl
#
DESTDIR  =

BINPATH  = /usr/bin
SCRPATH  = /etc/atop
LOGPATH  = /var/log/atop
MAN1PATH = /usr/share/man/man1
MAN5PATH = /usr/share/man/man5
INIPATH  = /etc/init.d
CRNPATH  = /etc/cron.d
ROTPATH  = /etc/logrotate.d
PMPATH1  = /usr/lib/pm-utils/sleep.d
PMPATH2  = /usr/lib64/pm-utils/sleep.d

CFLAGS  += -O2 -I. -Wall 	 # -DHTTPSTATS
OBJMOD0  = version.o
OBJMOD1  = various.o  deviate.o   procdbase.o
OBJMOD2  = acctproc.o photoproc.o photosyst.o  rawlog.o ifprop.o parseable.o
OBJMOD3  = showgeneric.o          showlinux.o  showsys.o showprocs.o
OBJMOD4  = atopsar.o  netatopif.o
ALLMODS  = $(OBJMOD0) $(OBJMOD1) $(OBJMOD2) $(OBJMOD3) $(OBJMOD4)

VERS     = $(shell ./atop -V 2>/dev/null| sed -e 's/^[^ ]* //' -e 's/ .*//')

all: 		atop atopsar

atop:		atop.o    $(ALLMODS) Makefile
		$(CC) atop.o $(ALLMODS) -o atop -lncurses -lm -lz -lrt

atopsar:	atop
		ln -sf atop atopsar

clean:
		rm -f *.o

install:	atop
		if [ ! -d $(DESTDIR)$(LOGPATH) ]; 	\
			then mkdir -p $(DESTDIR)$(LOGPATH); fi
		if [ ! -d $(DESTDIR)$(BINPATH) ]; 	\
			then mkdir -p $(DESTDIR)$(BINPATH); fi
		if [ ! -d $(DESTDIR)$(SCRPATH) ]; 	\
			then mkdir -p $(DESTDIR)$(SCRPATH); fi
		if [ ! -d $(DESTDIR)$(MAN1PATH) ]; 	\
			then mkdir -p $(DESTDIR)$(MAN1PATH); fi
		if [ ! -d $(DESTDIR)$(MAN5PATH) ]; 	\
			then mkdir -p $(DESTDIR)$(MAN5PATH); fi
		if [ ! -d $(DESTDIR)$(INIPATH) ]; 	\
			then mkdir -p $(DESTDIR)$(INIPATH); fi
		if [ ! -d $(DESTDIR)$(CRNPATH) ]; 	\
			then mkdir -p $(DESTDIR)$(CRNPATH); fi
		if [ ! -d $(DESTDIR)$(ROTPATH) ]; 	\
			then mkdir -p $(DESTDIR)$(ROTPATH); fi
		if [ -d $(DESTDIR)$(PMPATH1) ]; 	\
			then cp 45atoppm $(DESTDIR)$(PMPATH1); 	\
			chmod 0711 $(DESTDIR)$(PMPATH1)/45atoppm; fi
		if [ -d $(DESTDIR)$(PMPATH2) ]; 	\
			then cp 45atoppm $(DESTDIR)$(PMPATH2); 	\
			chmod 0711 $(DESTDIR)$(PMPATH2)/45atoppm; fi
		#
		cp atop   		$(DESTDIR)$(BINPATH)/atop
		chown root		$(DESTDIR)$(BINPATH)/atop
		chmod 04711 		$(DESTDIR)$(BINPATH)/atop
		ln -sf atop             $(DESTDIR)$(BINPATH)/atopsar
		cp atop   		$(DESTDIR)$(BINPATH)/atop-$(VERS)
		ln -sf atop-$(VERS)     $(DESTDIR)$(BINPATH)/atopsar-$(VERS)
		cp atop.daily    	$(DESTDIR)$(SCRPATH)
		chmod 0711 	 	$(DESTDIR)$(SCRPATH)/atop.daily
		cp man/atop.1    	$(DESTDIR)$(MAN1PATH)
		cp man/atopsar.1 	$(DESTDIR)$(MAN1PATH)
		cp man/atoprc.5  	$(DESTDIR)$(MAN5PATH)
		cp atop.init     	$(DESTDIR)$(INIPATH)/atop
		cp atop.cron     	$(DESTDIR)$(CRNPATH)/atop
		cp psaccs_atop   	$(DESTDIR)$(ROTPATH)/psaccs_atop
		cp psaccu_atop  	$(DESTDIR)$(ROTPATH)/psaccu_atop
		touch          	  	$(DESTDIR)$(LOGPATH)/dummy_before
		touch            	$(DESTDIR)$(LOGPATH)/dummy_after
		if [ -z "$(DESTDIR)" -a -f /sbin/chkconfig ]; then /sbin/chkconfig --add atop; fi
		if [ -z "$(DESTDIR)" -a -f /usr/sbin/update-rc.d ]; then update-rc.d atop defaults; fi

distr: rm -f *.o atop
		tar czvf /tmp/atop.tar.gz *
##########################################################################

atop.o:		atop.h	photoproc.h photosyst.h  acctproc.h showgeneric.h
atopsar.o:	atop.h	photoproc.h photosyst.h                           
rawlog.o:	atop.h	photoproc.h photosyst.h             showgeneric.h
various.o:	atop.h                           acctproc.h
ifprop.o:	atop.h	            photosyst.h             ifprop.h
parseable.o:	atop.h	photoproc.h photosyst.h             parseable.h
deviate.o:	atop.h	photoproc.h photosyst.h
procdbase.o:	atop.h	photoproc.h
acctproc.o:	atop.h	photoproc.h              acctproc.h netatop.h
netatopif.o:	atop.h	photoproc.h              netatopd.h netatop.h
photoproc.o:	atop.h	photoproc.h
photosyst.o:	atop.h	            photosyst.h
showgeneric.o:	atop.h	photoproc.h photosyst.h  showgeneric.h showlinux.h
showlinux.o:	atop.h	photoproc.h photosyst.h  showgeneric.h showlinux.h
showsys.o:	atop.h  photoproc.h photosyst.h  showgeneric.h 
showprocs.o:	atop.h	photoproc.h photosyst.h  showgeneric.h showlinux.h
