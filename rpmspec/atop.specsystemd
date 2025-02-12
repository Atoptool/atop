Name:           atop
Version:        XVERSX
Release:        XRELX
Source0:        %{name}-%{version}.tar.gz
URL:            https://www.atoptool.nl
Packager:       Gerlof Langeveld <gerlof.langeveld@atoptool.nl>
Summary:        Advanced System and Process Monitor
License:        GPL
Group: 	        System Environment
Requires:       zlib, ncurses, python3
BuildRequires:  zlib-devel, ncurses-devel, glib2-devel
BuildRoot:      /var/tmp/rpm-buildroot-atop

%description
The program atop is an interactive monitor to view the load on
a Linux-system. It shows the occupation of the most critical
hardware-resources (from a performance point of view) on system-level,
i.e. cpu, memory, disk and network. It also shows which processess
(and threads) are responsible for the indicated load (again cpu-,
memory-, disk- and network-load on process-level).
The program atop can also be used to log system- and process-level
information in raw format for long-term analysis.

The program atopsar can be used to view system-level statistics
as reports.

%prep
%setup -q

%build
make

%install
rm    -rf 			  $RPM_BUILD_ROOT

# generic build
install -Dp -m 0711 atop 	  $RPM_BUILD_ROOT/usr/bin/atop
ln -s atop                        $RPM_BUILD_ROOT/usr/bin/atopsar
install -Dp -m 0711 atopconvert	  $RPM_BUILD_ROOT/usr/bin/atopconvert
install -Dp -m 0711 atopcat	  $RPM_BUILD_ROOT/usr/bin/atopcat
install -Dp -m 0711 atophide	  $RPM_BUILD_ROOT/usr/bin/atophide
install -Dp -m 0700 atopacctd 	  $RPM_BUILD_ROOT/usr/sbin/atopacctd
install -Dp -m 0700 atopgpud 	  $RPM_BUILD_ROOT/usr/sbin/atopgpud
install -Dp -m 0644 atop.default  $RPM_BUILD_ROOT/etc/default/atop

install -Dp -m 0644 man/atop.1 	      $RPM_BUILD_ROOT/usr/share/man/man1/atop.1
install -Dp -m 0644 man/atopsar.1     $RPM_BUILD_ROOT/usr/share/man/man1/atopsar.1
install -Dp -m 0644 man/atopconvert.1 $RPM_BUILD_ROOT/usr/share/man/man1/atopconvert.1
install -Dp -m 0644 man/atopcat.1     $RPM_BUILD_ROOT/usr/share/man/man1/atopcat.1
install -Dp -m 0644 man/atophide.1    $RPM_BUILD_ROOT/usr/share/man/man1/atophide.1
install -Dp -m 0644 man/atoprc.5      $RPM_BUILD_ROOT/usr/share/man/man5/atoprc.5
install -Dp -m 0644 man/atopacctd.8   $RPM_BUILD_ROOT/usr/share/man/man8/atopacctd.8
install -Dp -m 0644 man/atopgpud.8    $RPM_BUILD_ROOT/usr/share/man/man8/atopgpud.8

install  -d -m 0755 		  $RPM_BUILD_ROOT/var/log/atop

# systemd-specific build
install -Dp -m 0644 atop.service     $RPM_BUILD_ROOT/usr/lib/systemd/system/atop.service
install -Dp -m 0644 atop-rotate.service     $RPM_BUILD_ROOT/usr/lib/systemd/system/atop-rotate.service
install -Dp -m 0644 atop-rotate.timer     $RPM_BUILD_ROOT/usr/lib/systemd/system/atop-rotate.timer
install -Dp -m 0644 atopacct.service $RPM_BUILD_ROOT/usr/lib/systemd/system/atopacct.service
install -Dp -m 0644 atopgpu.service  $RPM_BUILD_ROOT/usr/lib/systemd/system/atopgpu.service
install -Dp -m 0711 atop-pm.sh	     $RPM_BUILD_ROOT/usr/lib/systemd/system-sleep/atop-pm.sh

%clean
rm -rf $RPM_BUILD_ROOT

%post
# save today's logfile (format might be incompatible)
mv /var/log/atop/atop_`date +%Y%m%d` /var/log/atop/atop_`date +%Y%m%d`.save \
					2> /dev/null || :

/bin/systemctl daemon-reload
/bin/systemctl enable atopacct
/bin/systemctl enable atop
/bin/systemctl enable atop-rotate.timer

%preun
if [ $1 -eq 0 ]
then
	/bin/systemctl disable --now atop
	/bin/systemctl disable --now atopacct
	/bin/systemctl disable --now atopgpu
	/bin/systemctl disable --now atop-rotate.timer
fi

# atop-XVERSX and atopsar-XVERSX will remain for old logfiles
cp -f  /usr/bin/atop            /usr/bin/atop-XVERSX
ln -sf /usr/bin/atop-XVERSX     /usr/bin/atopsar-XVERSX

%files
%defattr(-,root,root)
%doc README COPYING AUTHORS ChangeLog
/usr/bin/atop
/usr/bin/atopconvert
/usr/bin/atopcat
/usr/bin/atophide
/usr/bin/atopsar
/usr/sbin/atopacctd
/usr/sbin/atopgpud
/usr/share/man/man1/atop.1*
/usr/share/man/man1/atopsar.1*
/usr/share/man/man1/atopconvert.1*
/usr/share/man/man1/atopcat.1*
/usr/share/man/man1/atophide.1*
/usr/share/man/man5/atoprc.5*
/usr/share/man/man8/atopacctd.8*
/usr/share/man/man8/atopgpud.8*
/usr/lib/systemd/system/atop.service
/usr/lib/systemd/system/atop-rotate.service
/usr/lib/systemd/system/atop-rotate.timer
/usr/lib/systemd/system/atopacct.service
/usr/lib/systemd/system/atopgpu.service
/usr/lib/systemd/system-sleep/atop-pm.sh
%config(noreplace) /etc/default/atop
%dir /var/log/atop/
