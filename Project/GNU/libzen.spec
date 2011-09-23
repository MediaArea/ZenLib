# Copyright (c) 2008 oc2pus
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.
#
# Please submit bugfixes or comments to toni@links2linux.de

# norootforbuild

%define libzen_version			0.4.23

Name:			libzen0
Version:		%libzen_version
Release:		1
Summary:		Shared library for libmediainfo and medianfo*
Group:			System/Libraries
License:		BSD
URL:			http://zenlib.sourceforge.net/
Packager:		MediaArea.net <info@mediaarea.net>
Source:			libzen_%{version}-1.tar.gz
BuildRoot:		%{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:	dos2unix
BuildRequires:	doxygen
BuildRequires:	gcc-c++

%description
Shared library for libmediainfo and medianfo-*.

%package -n libzen0-devel
Summary:	Include files and mandatory libraries for development
Group:		Development/Libraries/C and C++
Requires:	libzen0 = %{version}

%description -n libzen0-devel
Include files and mandatory libraries for development.

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
	%configure --enable-shared

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
for i in Base64 HTTP_Client Format/Html Format/Http TinyXml; do
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

%files
%defattr(-,root,root,-)
%doc History.txt License.txt ReadMe.txt
%{_libdir}/libzen.so.*

%files -n libzen0-devel
%defattr(-,root,root,-)
%doc Documentation.html
%doc Doc/*
%dir %{_includedir}/ZenLib
%{_includedir}/ZenLib/*
%{_libdir}/libzen.a
%{_libdir}/libzen.la
%{_libdir}/libzen.so
%{_libdir}/pkgconfig/*.pc

%changelog
* Tue Jan 01 2009 MediaArea.net <info@mediaarea.net> - 0.4.23-0
- See History.txt for more info and real dates
- Previous packages made by Toni Graffy <toni@links2linux.de>
