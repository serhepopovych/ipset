%define module_name ipset
%define module_version 7.1

%define _srcdir %{_prefix}/src/%{module_name}-%{module_version}
%define _dkmsdir %{_prefix}/lib/dkms

%define _topdir %(pwd)
%define _specdir %{_topdir}/rpm
%define _sourcedir %{_topdir}
%define _buildrootdir %{_topdir}
%define _builddir %{_topdir}
%define _rpmdir %{_topdir}/..
%define _srcrpmdir %{_topdir}/..

%define __find_provides  %{_dkmsdir}/find-provides

Summary:	DKMS source for ipset kernel part
Name:		%{module_name}-dkms
Version:	%{module_version}
License:	GPLv2
URL:		https://github.com/serhepopovych/ipset
Packager:	Serhey Popovych <serhe.popovych@gmail.com>
Release:	1%{?dist}
BuildArch:	noarch
Group:		System/Kernel
Requires:	dkms >= 1.95
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root/

Requires:	automake, autoconf, pkgconfig, libtool, make, libtool-ltdl-devel, libmnl-devel, kernel-devel

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

%prep
if [ "$RPM_BUILD_ROOT" != "/" ]; then
	rm -rf $RPM_BUILD_ROOT
fi

%install
mkdir -p $RPM_BUILD_ROOT/%{_srcdir}
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/%{name}
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
    -e "s|#MODULE_NAME#|%{module_name}|g" \
    -e "s|#MODULE_VERSION#|%{module_version}|g" \
    %{_sourcedir}/dkms/conf > $RPM_BUILD_ROOT/%{_srcdir}/dkms.conf

sed -e "s|updates/dkms|extra|g" \
    %{_sourcedir}/dkms/depmod > $RPM_BUILD_ROOT/%{_sysconfdir}/depmod.d/%{name}.conf

sed -e "s|updates/dkms|extra|g" \
    %{_sourcedir}/dkms/modprobe > $RPM_BUILD_ROOT/%{_sysconfdir}/modprobe.d/%{name}.conf

if [ -f %{_sourcedir}/common.postinst ]; then
	install -m 755 %{_sourcedir}/common.postinst $RPM_BUILD_ROOT/%{_datadir}/%{name}/postinst
fi

%clean
if [ "$RPM_BUILD_ROOT" != "/" ]; then
	rm -rf $RPM_BUILD_ROOT
fi

%post
for POSTINST in %{_dkmsdir}/common.postinst %{_datadir}/%{name}/postinst; do
	if [ -f $POSTINST ]; then
		$POSTINST %{module_name} %{module_version} %{_datadir}/%{name}
		exit $?
	fi
	echo "WARNING: $POSTINST does not exist."
done
echo -e "ERROR: DKMS version is too old and %{name} was not"
echo -e "built with legacy DKMS support."
echo -e "You must either rebuild %{name} with legacy postinst"
echo -e "support or upgrade DKMS to a more current version."
exit 1

%preun
echo -e
echo -e "Uninstall of %{module_name} module (version %{module_version}) beginning:"
dkms remove -m %{module_name} -v %{module_version} --all --rpm_safe_upgrade
exit 0

%files
%defattr(-,root,root)
%config(noreplace) %{_sysconfdir}/depmod.d/%{name}.conf
%config(noreplace) %{_sysconfdir}/modprobe.d/%{name}.conf
%{_srcdir}
%{_datadir}/%{name}
%doc README

%changelog
* Wed Jan 23 2019 Serhey Popovych <serhe.popovych@gmail.com> - 7.1-1
- Initial release. (Closes: #123456)
