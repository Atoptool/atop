# Makefile for System & Process Monitor ATOP (Linux version)
#
# Gerlof Langeveld - gerlof.langeveld@atoptool.nl
#
DESTDIR  =

BINPATH  = /usr/bin
SBINPATH = /usr/sbin
SCRPATH  = /usr/share/atop
LOGPATH  = /var/log/atop
MAN1PATH = /usr/share/man/man1
MAN5PATH = /usr/share/man/man5
MAN8PATH = /usr/share/man/man8
INIPATH  = /etc/init.d
SYSDPATH = /usr/lib/systemd/system
CRNPATH  = /etc/cron.d
ROTPATH  = /etc/logrotate.d
PMPATH1  = /usr/lib/pm-utils/sleep.d
PMPATH2  = /usr/lib64/pm-utils/sleep.d
PMPATHD  = /usr/lib/systemd/system-sleep

CFLAGS  += -O2 -I. -Wall  # -DNOPERFEVENT   # -DHTTPSTATS
OBJMOD0  = version.o
OBJMOD1  = various.o  deviate.o   procdbase.o
OBJMOD2  = acctproc.o photoproc.o photosyst.o  rawlog.o ifprop.o parseable.o
OBJMOD3  = showgeneric.o          showlinux.o  showsys.o showprocs.o
OBJMOD4  = atopsar.o  netatopif.o gpucom.o
ALLMODS  = $(OBJMOD0) $(OBJMOD1) $(OBJMOD2) $(OBJMOD3) $(OBJMOD4)

VERS     = $(shell ./atop -V 2>/dev/null| sed -e 's/^[^ ]* //' -e 's/ .*//')

all: 		atop atopsar atopacctd atopconvert

atop:		atop.o    $(ALLMODS) Makefile
		$(CC) -c version.c
		$(CC) atop.o $(ALLMODS) -o atop -lncurses -lz -lm -lrt $(LDFLAGS)

atopsar:	atop
		ln -sf atop atopsar

atopacctd:	atopacctd.o netlink.o
		$(CC) atopacctd.o netlink.o -o atopacctd $(LDFLAGS)

atopconvert:	atopconvert.o
		$(CC) atopconvert.o -o atopconvert -lz $(LDFLAGS)

netlink.o:	netlink.c
		$(CC) -I. -Wall -c netlink.c

clean:
		rm -f *.o atop atopacctd atopconvert

distr:
		rm -f *.o atop
		tar czvf /tmp/atop.tar.gz *


install:
		@echo Choose either \'make systemdinstall\' or \'make sysvinstall\'

systemdinstall:	genericinstall
		if [ ! -d $(DESTDIR)$(SYSDPATH) ]; 			\
		then	mkdir -p $(DESTDIR)$(SYSDPATH); fi
		if [ ! -d $(DESTDIR)$(PMPATHD) ]; 			\
		then	mkdir -p $(DESTDIR)$(PMPATHD); fi
		#
		cp atop.service       $(DESTDIR)$(SYSDPATH)
		chmod 0644            $(DESTDIR)$(SYSDPATH)/atop.service
		cp atopgpu.service    $(DESTDIR)$(SYSDPATH)
		chmod 0644            $(DESTDIR)$(SYSDPATH)/atopgpu.service
		cp atopacct.service   $(DESTDIR)$(SYSDPATH)
		chmod 0644            $(DESTDIR)$(SYSDPATH)/atopacct.service
		cp atop.cronsystemd   $(DESTDIR)$(CRNPATH)/atop
		cp atop-pm.sh         $(DESTDIR)$(PMPATHD)
		chmod 0711            $(DESTDIR)$(PMPATHD)/atop-pm.sh
		#
		# only when making on target system:
		#
		if [ -z "$(DESTDIR)" -a -f /bin/systemctl ]; 		\
		then	/bin/systemctl stop    atop     2> /dev/null;	\
			/bin/systemctl disable atop     2> /dev/null;	\
			/bin/systemctl stop    atopacct 2> /dev/null;	\
			/bin/systemctl disable atopacct 2> /dev/null;	\
			/bin/systemctl enable  atopacct;		\
			/bin/systemctl start   atopacct;		\
			/bin/systemctl enable  atop;			\
			/bin/systemctl start   atop;			\
		fi

sysvinstall:	genericinstall
		if [ ! -d $(DESTDIR)$(INIPATH) ]; 			\
		then	mkdir -p  $(DESTDIR)$(INIPATH); fi
		#
		cp atop.init      $(DESTDIR)$(INIPATH)/atop
		cp atopacct.init  $(DESTDIR)$(INIPATH)/atopacct
		cp atop.cronsysv  $(DESTDIR)$(CRNPATH)/atop
		#
		if [   -d $(DESTDIR)$(PMPATH1) ]; 			\
		then	cp 45atoppm $(DESTDIR)$(PMPATH1); 		\
			chmod 0711  $(DESTDIR)$(PMPATH1)/45atoppm;	\
		fi
		if [ -d $(DESTDIR)$(PMPATH2) ]; 			\
		then	cp 45atoppm $(DESTDIR)$(PMPATH2);		\
			chmod 0711  $(DESTDIR)$(PMPATH2)/45atoppm;	\
		fi
		#
		#
		# only when making on target system:
		#
		if [ -z "$(DESTDIR)" -a -f /sbin/chkconfig ];		\
		then 	/sbin/chkconfig --del atop     2> /dev/null;	\
			/sbin/chkconfig --add atop;			\
			/sbin/chkconfig --del atopacct 2> /dev/null;	\
			/sbin/chkconfig --add atopacct;			\
		fi
		if [ -z "$(DESTDIR)" -a -f /usr/sbin/update-rc.d ];	\
		then	update-rc.d atop defaults;			\
			update-rc.d atopacct defaults;			\
		fi
		if [ -z "$(DESTDIR)" -a -f /sbin/service ];		\
		then	/sbin/service atopacct start;			\
			sleep 2;					\
			/sbin/service atop     start;			\
		fi

genericinstall:	atop atopacctd atopconvert
		if [ ! -d $(DESTDIR)$(LOGPATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(LOGPATH); fi
		if [ ! -d $(DESTDIR)$(BINPATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(BINPATH); fi
		if [ ! -d $(DESTDIR)$(SBINPATH) ]; 		\
		then mkdir -p $(DESTDIR)$(SBINPATH); fi
		if [ ! -d $(DESTDIR)$(SCRPATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(SCRPATH); fi	
		if [ ! -d $(DESTDIR)$(MAN1PATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(MAN1PATH);	fi
		if [ ! -d $(DESTDIR)$(MAN5PATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(MAN5PATH);	fi
		if [ ! -d $(DESTDIR)$(MAN8PATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(MAN8PATH);	fi
		if [ ! -d $(DESTDIR)$(CRNPATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(CRNPATH);	fi
		if [ ! -d $(DESTDIR)$(ROTPATH) ]; 		\
		then	mkdir -p $(DESTDIR)$(ROTPATH);	fi
		#
		cp atop   		$(DESTDIR)$(BINPATH)/atop
		chown root		$(DESTDIR)$(BINPATH)/atop
		chmod 04711 		$(DESTDIR)$(BINPATH)/atop
		ln -sf atop             $(DESTDIR)$(BINPATH)/atopsar
		cp atopacctd  		$(DESTDIR)$(SBINPATH)/atopacctd
		chown root		$(DESTDIR)$(SBINPATH)/atopacctd
		chmod 0700 		$(DESTDIR)$(SBINPATH)/atopacctd
		cp atopgpud  		$(DESTDIR)$(SBINPATH)/atopgpud
		chown root		$(DESTDIR)$(SBINPATH)/atopgpud
		chmod 0700 		$(DESTDIR)$(SBINPATH)/atopgpud
		cp atop   		$(DESTDIR)$(BINPATH)/atop-$(VERS)
		ln -sf atop-$(VERS)     $(DESTDIR)$(BINPATH)/atopsar-$(VERS)
		cp atopconvert 		$(DESTDIR)$(BINPATH)/atopconvert
		chown root		$(DESTDIR)$(BINPATH)/atopconvert
		chmod 0711 		$(DESTDIR)$(BINPATH)/atopconvert
		cp atop.daily    	$(DESTDIR)$(SCRPATH)
		chmod 0711 	 	$(DESTDIR)$(SCRPATH)/atop.daily
		cp man/atop.1    	$(DESTDIR)$(MAN1PATH)
		cp man/atopsar.1 	$(DESTDIR)$(MAN1PATH)
		cp man/atopconvert.1 	$(DESTDIR)$(MAN1PATH)
		cp man/atoprc.5  	$(DESTDIR)$(MAN5PATH)
		cp man/atopacctd.8  	$(DESTDIR)$(MAN8PATH)
		cp man/atopgpud.8  	$(DESTDIR)$(MAN8PATH)
		cp psaccs_atop   	$(DESTDIR)$(ROTPATH)/psaccs_atop
		cp psaccu_atop  	$(DESTDIR)$(ROTPATH)/psaccu_atop
		touch          	  	$(DESTDIR)$(LOGPATH)/dummy_before
		touch            	$(DESTDIR)$(LOGPATH)/dummy_after

##########################################################################

versdate.h:
		./mkdate

atop.o:		atop.h	photoproc.h photosyst.h  acctproc.h showgeneric.h
atopsar.o:	atop.h	photoproc.h photosyst.h                           
rawlog.o:	atop.h	photoproc.h photosyst.h             showgeneric.h
various.o:	atop.h                           acctproc.h
ifprop.o:	atop.h	            photosyst.h             ifprop.h
parseable.o:	atop.h	photoproc.h photosyst.h             parseable.h
deviate.o:	atop.h	photoproc.h photosyst.h
procdbase.o:	atop.h	photoproc.h
acctproc.o:	atop.h	photoproc.h atopacctd.h  acctproc.h netatop.h
netatopif.o:	atop.h	photoproc.h              netatopd.h netatop.h
photoproc.o:	atop.h	photoproc.h
photosyst.o:	atop.h	            photosyst.h
showgeneric.o:	atop.h	photoproc.h photosyst.h  showgeneric.h showlinux.h
showlinux.o:	atop.h	photoproc.h photosyst.h  showgeneric.h showlinux.h
showsys.o:	atop.h  photoproc.h photosyst.h  showgeneric.h 
showprocs.o:	atop.h	photoproc.h photosyst.h  showgeneric.h showlinux.h
version.o:	version.c version.h versdate.h
gpucom.o:	atop.h	photoproc.h photosyst.h

atopacctd.o:	atop.h  photoproc.h acctproc.h   atopacctd.h   version.h versdate.h

atopconvert.o:	atop.h  photoproc.h photosyst.h
