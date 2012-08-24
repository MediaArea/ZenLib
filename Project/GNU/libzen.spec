# Copyright (c) 2008 oc2pus
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments to toni@links2linux.de

# norootforbuild

%define libzen_version			0.4.28

Name:			libzen
Version:		%libzen_version
Release:		1
Summary:		ZenLib C++ utility library
Group:			System/Libraries
License:		zlib
URL:			http://sourceforge.net/projects/zenlib
Packager:		MediaArea.net SARL <info@mediaarea.net>
Source:			libzen_%{version}-1.tar.gz
BuildRoot:		%{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:	pkgconfig
BuildRequires:	dos2unix
BuildRequires:	doxygen
BuildRequires:	gcc-c++
BuildRequires:	glibc-devel
BuildRequires:  libtool
BuildRequires:  automake
BuildRequires:  autoconf

%description
ZenLib is a C++ utility library. It includes classes for handling strings,
configuration, bit streams, threading, translation, and cross-platform
operating system functions.

This package contains the headers required for compiling applications/libraries
which use this library.

%package -n libzen0
Summary:		ZenLib C++ utility library -- shared library
Group:			System/Libraries
Requires:		glibc

%description -n libzen0
ZenLib is a C++ utility library. It includes classes for handling strings,
configuration, bit streams, threading, translation, and cross-platform
operating system functions.

This package contains the headers required for compiling applications/libraries
which use this library.

%package -n libzen-devel
Summary:	ZenLib C++ utility library -- development files
Group:		Development/Libraries/C and C++
Requires:	libzen0%{?_isa} = %{version}
Requires:	glibc-devel
Obsoletes:	libzen0-devel

%description -n libzen-devel
ZenLib is a C++ utility library. It includes classes for handling strings,
configuration, bit streams, threading, translation, and cross-platform
operating system functions.

This package contains the headers and other development support files needed
for compiling and linking applications and libraries which use this library.

%prep
%setup -q -n ZenLib
dos2unix     *.txt Source/Doc/*.html
%__chmod 644 *.txt Source/Doc/*.html

%build
export CFLAGS="$RPM_OPT_FLAGS"
export CPPFLAGS="$RPM_OPT_FLAGS"
export CXXFLAGS="$RPM_OPT_FLAGS"

pushd Source/Doc/
	doxygen Doxyfile
popd
cp Source/Doc/*.html ./

pushd Project/GNU/Library
	%__chmod +x autogen
	./autogen
	%configure --disable-static --enable-shared

	%__make clean
	%__make %{?jobs:-j%{jobs}}
popd

%install
pushd Project/GNU/Library
	%__make install-strip DESTDIR=%{buildroot}
popd

# Zenlib headers and ZenLib-config
%__install -dm 755 %{buildroot}%{_includedir}/ZenLib
%__install -m 644 Source/ZenLib/*.h \
	%{buildroot}%{_includedir}/ZenLib
for i in HTTP_Client Format/Html Format/Http; do
	%__install -dm 755 %{buildroot}%{_includedir}/ZenLib/$i
	%__install -m 644 Source/ZenLib/$i/*.h \
		%{buildroot}%{_includedir}/ZenLib/$i
done

%__sed -i -e 's|Version: |Version: %{version}|g' \
	Project/GNU/Library/libzen.pc
%__install -dm 755 %{buildroot}%{_libdir}/pkgconfig
%__install -m 644 Project/GNU/Library/libzen.pc \
	%{buildroot}%{_libdir}/pkgconfig

%clean
[ -d "%{buildroot}" -a "%{buildroot}" != "" ] && %__rm -rf "%{buildroot}"

%post -n libzen0 -p /sbin/ldconfig

%postun -n libzen0 -p /sbin/ldconfig

%files -n libzen0
%defattr(-,root,root,-)
%doc History.txt License.txt ReadMe.txt
%{_libdir}/libzen.so.*

%files -n libzen-devel
%defattr(-,root,root,-)
%doc Documentation.html
%doc Doc/*
%{_bindir}/libzen-config
%dir %{_includedir}/ZenLib
%{_includedir}/ZenLib/*
%{_libdir}/libzen.la
%{_libdir}/libzen.so
%{_libdir}/pkgconfig/*.pc

%changelog
* Tue Jan 01 2009 MediaArea.net SARL <info@mediaarea.net> - 0.4.28-0
- See History.txt for more info and real dates
- Previous packages made by Toni Graffy <toni@links2linux.de>
