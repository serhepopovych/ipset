
%define _topdir %(pwd)
%define _specdir %{_topdir}/rpm
%define _sourcedir %{_topdir}
%define _buildrootdir %{_topdir}
%define _builddir %{_topdir}
%define _rpmdir %{_topdir}/..
%define _srcrpmdir %{_topdir}/..

Summary:	Manage Linux IP sets
Name:		ipset
Version:	7.21
License:	GPLv2
Group:		System Environment/Base
URL:		https://github.com/serhepopovych/ipset
Packager:	Serhey Popovych <serhe.popovych@gmail.com>
Release:	1%{?dist}
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root/

BuildRequires:	automake autoconf pkgconfig libtool make libtool-ltdl-devel libmnl-devel

# An explicit requirement is needed here, to avoid cases where a user would
# explicitly update only one of the two (e.g 'yum update ipset')
Requires:	%{name}-libs%{?_isa} = %{version}-%{release}

%define _srcdir %{_prefix}/src/%{name}-%{version}
%define _dkmsdir %{_prefix}/lib/dkms
%define __find_provides  %{_dkmsdir}/find-provides
%define _legacy_actions %{_libexecdir}/initscripts/legacy-actions
%{!?_unitdir:%define _unitdir /usr/lib/systemd/system}

%description
IP sets are a framework inside the Linux 2.4.x and 2.6.x kernel which can be
administered by the ipset(8) utility. Depending on the type, currently an
IP set may store IP addresses, (TCP/UDP) port numbers or IP addresses with
MAC addresses in a way  which ensures lightning speed when matching an
entry against a set.

If you want to

 * store multiple IP addresses or port numbers and match against the
   entire collection using a single iptables rule.
 * dynamically update iptables rules against IP addresses or ports without
   performance penalty.
 * express complex IP address and ports based rulesets with a single
   iptables rule and benefit from the speed of IP sets.

then IP sets may be the proper tool for you.

%package libs
Summary:	Shared library providing the IP sets functionality

%description libs
IP sets are a framework inside the Linux 2.4.x and 2.6.x kernel which can be
administered by the ipset(8) utility. Depending on the type, currently an
IP set may store IP addresses, (TCP/UDP) port numbers or IP addresses with
MAC addresses in a way  which ensures lightning speed when matching an
entry against a set.

If you want to

 * store multiple IP addresses or port numbers and match against the
   entire collection using a single iptables rule.
 * dynamically update iptables rules against IP addresses or ports without
   performance penalty.
 * express complex IP address and ports based rulesets with a single
   iptables rule and benefit from the speed of IP sets.

then IP sets may be the proper tool for you.

This package contains the shared libraries needed to run programs that use
the IP sets library.

%package devel
Summary:	Development files for %{name}
Requires:	%{name}-libs%{?_isa} == %{version}-%{release}
Requires:	kernel-devel

%description devel
IP sets are a framework inside the Linux 2.4.x and 2.6.x kernel which can be
administered by the ipset(8) utility. Depending on the type, currently an
IP set may store IP addresses, (TCP/UDP) port numbers or IP addresses with
MAC addresses in a way  which ensures lightning speed when matching an
entry against a set.

If you want to

 * store multiple IP addresses or port numbers and match against the
   entire collection using a single iptables rule.
 * dynamically update iptables rules against IP addresses or ports without
   performance penalty.
 * express complex IP address and ports based rulesets with a single
   iptables rule and benefit from the speed of IP sets.

then IP sets may be the proper tool for you.

This package contains the development libraries and header files you need to
develop your programs using the IP sets library.

%package dkms
Summary:	DKMS source for ipset kernel part
Requires:	dkms >= 1.95
Requires:	automake, autoconf, pkgconfig, libtool, make, libtool-ltdl-devel, libmnl-devel, kernel-devel

%description dkms
IP sets are a framework inside the Linux 2.4.x and 2.6.x kernel which can be
administered by the ipset(8) utility. Depending on the type, currently an
IP set may store IP addresses, (TCP/UDP) port numbers or IP addresses with
MAC addresses in a way  which ensures lightning speed when matching an
entry against a set.

If you want to

 * store multiple IP addresses or port numbers and match against the
   entire collection using a single iptables rule.
 * dynamically update iptables rules against IP addresses or ports without
   performance penalty.
 * express complex IP address and ports based rulesets with a single
   iptables rule and benefit from the speed of IP sets.

then IP sets may be the proper tool for you.

This package provides the dkms source code for the ipset kernel modules.
Kernel source or headers are required to compile these modules.

%package services
Summary:		ipset service to save/restore ipsets
Requires:		%{name} = %{version}-%{release}
Conflicts:		%{name}-service
Requires:		coreutils
Requires:		diffutils
BuildArch:		noarch

%description services
IP sets are a framework inside the Linux 2.4.x and 2.6.x kernel which can be
administered by the ipset(8) utility. Depending on the type, currently an
IP set may store IP addresses, (TCP/UDP) port numbers or IP addresses with
MAC addresses in a way  which ensures lightning speed when matching an
entry against a set.

If you want to

 * store multiple IP addresses or port numbers and match against the
   entire collection using a single iptables rule.
 * dynamically update iptables rules against IP addresses or ports without
   performance penalty.
 * express complex IP address and ports based rulesets with a single
   iptables rule and benefit from the speed of IP sets.

then IP sets may be the proper tool for you.

This package provides the service ipset that is split out of the base package
since it is not active by default.

%prep
if [ "$RPM_BUILD_ROOT" != "/" ]; then
	rm -rf $RPM_BUILD_ROOT
fi

%build
autoreconf -fi
%configure --enable-static=no --with-kmod=no

# Prevent libtool from defining rpath
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool

make %{?_smp_mflags}

%install
## ipset tools, libs and devel
make install DESTDIR=$RPM_BUILD_ROOT
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f '{}' \;

## dkms
mkdir -p $RPM_BUILD_ROOT/%{_srcdir}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/%{name}-dkms
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/depmod.d
mkdir -p $RPM_BUILD_ROOT/%{_sysconfdir}/modprobe.d

for f in %{_sourcedir}/COPYING %{_sourcedir}/README \
	 %{_sourcedir}/Make_global.am %{_sourcedir}/Makefile.am \
	 %{_sourcedir}/autogen.sh %{_sourcedir}/configure.ac %{_sourcedir}/m4 \
	 %{_sourcedir}/kernel %{_sourcedir}/include %{_sourcedir}/lib \
	 %{_sourcedir}/src %{_sourcedir}/utils; do
	cp -a ${f} $RPM_BUILD_ROOT/%{_srcdir}/
done

sed -e "s|updates/dkms|extra|g" \
    -e "s|#MODULE_NAME#|%{name}|g" \
    -e "s|#MODULE_VERSION#|%{version}|g" \
    %{_sourcedir}/dkms/conf > $RPM_BUILD_ROOT/%{_srcdir}/dkms.conf

sed -e "s|updates/dkms|extra|g" \
    %{_sourcedir}/dkms/depmod > $RPM_BUILD_ROOT/%{_sysconfdir}/depmod.d/%{name}-dkms.conf

sed -e "s|updates/dkms|extra|g" \
    %{_sourcedir}/dkms/modprobe > $RPM_BUILD_ROOT/%{_sysconfdir}/modprobe.d/%{name}-dkms.conf

if [ -f %{_sourcedir}/common.postinst ]; then
	install -m 755 %{_sourcedir}/common.postinst $RPM_BUILD_ROOT/%{_datadir}/%{name}-dkms/postinst
fi

# install directories
install -d $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig
install -d $RPM_BUILD_ROOT/%{_sysconfdir}/%{name}

# install systemd unit file
install -D -m 644 %{_sourcedir}/rpm/%{name}.service \
    $RPM_BUILD_ROOT/%{_unitdir}/%{name}.service

# install supporting script
install -D -m 755 %{_sourcedir}/rpm/%{name}.init \
    $RPM_BUILD_ROOT/%{_sysconfdir}/init.d/%{name}

# install ipset-config
install -D -m 644 %{_sourcedir}/rpm/%{name}.default \
    $RPM_BUILD_ROOT/%{_sysconfdir}/sysconfig/%{name}-config

# install legacy actions for service command
install -D -m 755 %{_sourcedir}/rpm/%{name}.save-legacy \
    $RPM_BUILD_ROOT/%{_legacy_actions}/%{name}/save
install -D -m 755 %{_sourcedir}/rpm/%{name}.flush-legacy \
    $RPM_BUILD_ROOT/%{_legacy_actions}/%{name}/flush

%clean
if [ "$RPM_BUILD_ROOT" != "/" ]; then
	rm -rf $RPM_BUILD_ROOT
fi

%files
%doc COPYING ChangeLog
%doc %{_mandir}/man8/%{name}.8.gz
%doc %{_mandir}/man8/%{name}-translate.8.gz
%{_sbindir}/%{name}
%{_sbindir}/%{name}-translate

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%files libs
%doc COPYING
%{_libdir}/lib%{name}.so.*

%files devel
%doc %{_mandir}/man3/lib%{name}.3.gz
%{_includedir}/lib%{name}
%{_libdir}/lib%{name}.so
%{_libdir}/pkgconfig/lib%{name}.pc

%post dkms
for POSTINST in %{_dkmsdir}/common.postinst %{_datadir}/%{name}-dkms/postinst; do
	if [ -f $POSTINST ]; then
		$POSTINST %{name} %{version} %{_datadir}/%{name}-dkms
		exit $?
	fi
	echo "WARNING: $POSTINST does not exist."
done
echo -e "ERROR: DKMS version is too old and %{name}-dkms was not"
echo -e "built with legacy DKMS support."
echo -e "You must either rebuild %{name}-dkms with legacy postinst"
echo -e "support or upgrade DKMS to a more current version."
exit 1

%preun dkms
echo -e
echo -e "Uninstall of %{name} module (version %{version}) beginning:"
dkms remove -m %{name} -v %{version} --all --rpm_safe_upgrade
exit 0

%files dkms
%defattr(-,root,root)
%config(noreplace) %{_sysconfdir}/depmod.d/%{name}-dkms.conf
%config(noreplace) %{_sysconfdir}/modprobe.d/%{name}-dkms.conf
%{_srcdir}
%{_datadir}/%{name}-dkms
%doc README

%files services
%{_unitdir}/%{name}.service
%attr(0755,root,root) %{_sysconfdir}/init.d/%{name}
%config(noreplace) %attr(0644,root,root) %{_sysconfdir}/sysconfig/%{name}-config
%dir %{_legacy_actions}/%{name}
%{_legacy_actions}/%{name}/save
%{_legacy_actions}/%{name}/flush
%dir %{_sysconfdir}/%{name}
%ghost %config(noreplace) %attr(0600,root,root) %{_sysconfdir}/%{name}/%{name}

%changelog
* %(date "+%a %b %d %Y") %{packager} %{version}-%{release}
- Initial release. (Closes: #123456)
