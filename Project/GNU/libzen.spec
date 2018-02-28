%define libzen_version            0.4.37

%if 0%{?fedora_version} || 0%{?centos_version} >= 600 || 0%{?rhel_version} >= 600
%define package_with_0_ending 0
%define libzen_name libzen
%else
%define package_with_0_ending 1
%define libzen_name libzen0
%endif

%define name_without_0_ending libzen

Name:           %{libzen_name}
Version:        %{libzen_version}
Release:        1
Summary:        C++ utility library -- runtime

License:        Zlib
Group:          System/Libraries
URL:            http://sourceforge.net/projects/zenlib
Packager:       MediaArea.net SARL <info@mediaarea.net>
Source:         %{name_without_0_ending}_%{version}.tar.gz
Requires:       glibc
%if !%{package_with_0_ending}
Provides:       %{name_without_0_ending}0 = %{version}
Obsoletes:      %{name_without_0_ending}0 < %{version}
%endif

Prefix:         %{_prefix}
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  glibc-devel
BuildRequires:  doxygen
BuildRequires:  libtool
BuildRequires:  automake
BuildRequires:  autoconf

%description
ZenLib is a C++ utility library. It includes classes for handling strings,
configuration, bit streams, threading, translation, and cross-platform
operating system functions.

This package contains the headers required for compiling applications/libraries
which use this library.

%package        -n %{name_without_0_ending}-doc
Summary:        C++ utility library -- documentation
Group:          Development/Libraries
Requires:       %{name} = %{version}
%if !0%{?suse_version} || 0%{?suse_version} >= 1200
BuildArch:      noarch
%endif

%description    -n %{name_without_0_ending}-doc
ZenLib is a C++ utility library. It includes classes for handling strings,
configuration, bit streams, threading, translation, and cross-platform
operating system functions.

This package contains the documentation

%package        -n %{name_without_0_ending}-devel
Summary:        C++ utility library -- development
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}
Requires:       glibc-devel

%description    -n %{name_without_0_ending}-devel
ZenLib is a C++ utility library. It includes classes for handling strings,
configuration, bit streams, threading, translation, and cross-platform
operating system functions.

This package contains the include files and mandatory libraries
for development.

%prep
%setup -q -n ZenLib
#Correct documentation encoding and permissions
sed -i 's/.$//' *.txt
chmod 644 *.txt Source/Doc/Documentation.html

chmod 644 Source/ZenLib/*.h Source/ZenLib/*.cpp \
    Source/ZenLib/Format/Html/*.h Source/ZenLib/Format/Html/*.cpp \
    Source/ZenLib/Format/Http/*.h Source/ZenLib/Format/Http/*.cpp

pushd Project/GNU/Library
    autoreconf -i
popd

%build
export CFLAGS="%{optflags}"
export CPPFLAGS="%{optflags}"
export CXXFLAGS="%{optflags}"

#Make documentation
pushd Source/Doc/
    doxygen -u Doxyfile
    doxygen Doxyfile
popd
cp Source/Doc/*.html ./

pushd Project/GNU/Library
    %if 0%{?mageia} > 5
        %configure --disable-static --enable-shared --disable-dependency-tracking
    %else
        %configure --disable-static --enable-shared
    %endif

    make clean
    make %{?_smp_mflags}
popd

%install
pushd Project/GNU/Library
    make install DESTDIR=%{buildroot}
popd

#Install headers
install -dm 755 %{buildroot}%{_includedir}/ZenLib
install -m 644 Source/ZenLib/*.h \
    %{buildroot}%{_includedir}/ZenLib
for i in HTTP_Client Format/Html Format/Http; do
    install -dm 755 %{buildroot}%{_includedir}/ZenLib/$i
    install -m 644 Source/ZenLib/$i/*.h \
        %{buildroot}%{_includedir}/ZenLib/$i
done

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc History.txt ReadMe.txt
%if 0%{?fedora_version} || 0%{?centos_version} >= 700 || 0%{?rhel_version} >= 700
%license License.txt
%else
%doc License.txt
%endif
%{_libdir}/%{name_without_0_ending}.so.*

%if 0%{?rhel} == 5
%exclude %{_usr}/lib/debug
%exclude %{_usr}/src/debug
%endif

%files -n %{name_without_0_ending}-doc
%defattr(-,root,root,-)
%doc Documentation.html
%doc Doc

%files -n %{name_without_0_ending}-devel
%defattr(-,root,root,-)
%{_includedir}/ZenLib
%{_libdir}/%{name_without_0_ending}.so
%{_libdir}/%{name_without_0_ending}.la
%{_libdir}/pkgconfig/*.pc

%changelog
* Thu Jan 01 2009 MediaArea.net SARL <info@mediaarea.net> - 0.4.37-0
- See History.txt for more info and real dates
- Previous packages made by Toni Graffy <toni@links2linux.de>
- Fedora style made by Vasiliy N. Glazov <vascom2@gmail.com>
