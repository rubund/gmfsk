# gmfsk.spec
#
%define name    	gmfsk
%define version 	0.7pre1
%define release 	1
%define skreq		0.3.5

Summary:	Gnome MFSK, RTTY, THROB, PSK31, MT63 and HELLSCHREIBER terminal
Name:		%{name}
Version:	%{version}
Release:	%{release}
License:	GPL
Group:		Applications/Communications
Source:		ftp://ftp.hes.iki.fi/pub/ham/unix/linux/hfmodems/%{name}-%{version}.tar.gz
BuildRoot:	/var/tmp/%{name}-root
BuildRequires:	scrollkeeper >= %skreq

%description

gMFSK is a terminal program for MFSK, RTTY, THROB, PSK31, MT63
and HELLSCHREIBER.

%prep
%setup -q

%build
%configure --enable-hamlib
make

%install
rm -rf $RPM_BUILD_ROOT

export GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1
%makeinstall
unset GCONF_DISABLE_MAKEFILE_SCHEMA_INSTALL

rm -rf $RPM_BUILD_ROOT/var/lib/scrollkeeper/

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc	AUTHORS COPYING COPYING-DOCS ChangeLog INSTALL NEWS README
%{_bindir}/%{name}
%{_datadir}/pixmaps/%{name}
%{_datadir}/fonts/feldhell
%{_datadir}/omf/%{name}
%{_datadir}/gnome/help/%{name}
%{_datadir}/locale/*/LC_MESSAGES/%{name}.mo
%{_sysconfdir}/gconf/schemas/%{name}.schemas

%post
if which scrollkeeper-update >/dev/null 2>&1; then
	echo "Doing scrollkeeper update."
	scrollkeeper-update -q -o %{_datadir}/omf/%{name}
fi
if which gconftool-2 >/dev/null 2>&1; then
	echo "Installing gconf schema."
	export GCONF_CONFIG_SOURCE=`gconftool-2 --get-default-source`
	gconftool-2 --makefile-install-rule %{_sysconfdir}/gconf/schemas/%{name}.schemas > /dev/null
fi

%postun
if which scrollkeeper-update >/dev/null 2>&1; then
	scrollkeeper-update -q
fi

%changelog
* Mon Jul 14 2003 Tomi Manninen <oh2bns@sral.fi>
- Added gconf stuff

%changelog
* Fri May 30 2003 Tomi Manninen <oh2bns@sral.fi>
- Updated for version 0.5.1

* Sat Apr 28 2001 Tomi Manninen <oh2bns@sral.fi>
- Create.
