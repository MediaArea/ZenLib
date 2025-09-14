%global libzen_version            0.4.41
%global libzen_version_major      0
%global libzen_version_minor      4
%global libzen_version_release    41

%if 0%{?fedora} || 0%{?rhel}
%global package_with_0_ending 0
%global libzen_name libzen
%else
%global package_with_0_ending 1
%global libzen_name libzen0
%endif

%global name_without_0_ending libzen

%global libzen_suffix %{libzen_version_major}%{libzen_version_minor}%{libzen_version_release}

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
%if 0%{?rhel} > 8
BuildRequires:  alternatives
%endif

%if 0%{?rhel}
%package        -n %{libzen_name}%{libzen_suffix}
Summary:        C++ utility library -- slot version
%endif

%global libzen_description ZenLib is a C++ utility library. It includes classes for handling strings,\
configuration, bit streams, threading, translation, and cross-platform\
operating system functions.\
\
This package contains the headers required for compiling applications/libraries\
which use this library.

%description
%{libzen_description}

%if 0%{?rhel}
%description -n %{libzen_name}%{libzen_suffix}
%{libzen_description}
%endif

%package        -n %{name_without_0_ending}-doc
Summary:        C++ utility library -- documentation
Group:          Development/Libraries
Requires:       %{name} = %{version}
%if !0%{?suse_version} || 0%{?suse_version} >= 1200
BuildArch:      noarch
%endif

%if 0%{?rhel}
%package        -n %{name_without_0_ending}%{libzen_suffix}-doc
Summary:        C++ utility library -- documentation
Group:          Development/Libraries
Requires:       %{libzen_name}%{libzen_suffix} = %{version}
BuildArch:      noarch
%endif

%global doc_description ZenLib is a C++ utility library. It includes classes for handling strings, \
configuration, bit streams, threading, translation, and cross-platform \
operating system functions. \
\
This package contains the documentation

%description    -n %{name_without_0_ending}-doc
%{doc_description}

%if 0%{?rhel}
%description    -n %{name_without_0_ending}%{libzen_suffix}-doc
%{doc_description}
%endif

%package        -n %{name_without_0_ending}-devel
Summary:        C++ utility library -- development
Group:          Development/Libraries
Requires:       %{name}%{?_isa} = %{version}
Requires:       glibc-devel

%if 0%{?rhel}
%package        -n %{name_without_0_ending}%{libzen_suffix}-devel
Summary:        C++ utility library -- development
Group:          Development/Libraries
Requires:       %{libzen_name}%{libzen_suffix}%{?_isa} = %{version}
Requires:       glibc-devel
%endif

%global devel_description ZenLib is a C++ utility library. It includes classes for handling strings,\
configuration, bit streams, threading, translation, and cross-platform\
operating system functions.\
\
This package contains the include files and mandatory libraries\
for development.

%description    -n %{name_without_0_ending}-devel
%{devel_description}

%if 0%{?rhel}
%description    -n %{name_without_0_ending}%{libzen_suffix}-devel
%{devel_description}
%endif

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
export CFLAGS="-g %{optflags}"
export CPPFLAGS="-g %{optflags}"
export CXXFLAGS="%{optflags}"

#Make documentation
pushd Source/Doc/
    doxygen -u Doxyfile
    doxygen Doxyfile
popd
cp Source/Doc/*.html ./

pushd Project/GNU/Library
    %configure --disable-static --enable-shared
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

%define libzen_files %defattr(-,root,root,-)\
%doc History.txt ReadMe.txt\
%if 0%{?fedora} || 0%{?rhel}\
%license License.txt\
%else\
%doc License.txt\
%endif\
%{_libdir}/%{name_without_0_ending}.so.*

%files
%{libzen_files}

%%if 0%{?rhel}
%files -n %{libzen_name}%{libzen_suffix}
%{libzen_files}
%endif

%define doc_files %defattr(-,root,root,-)\
%doc Documentation.html\
%doc Doc

%files -n %{name_without_0_ending}-doc
%{doc_files}

%if 0%{?rhel}
%files -n %{name_without_0_ending}%{libzen_suffix}-doc
%{doc_files}
%endif

%define devel_files %defattr(-,root,root,-)\
%{_includedir}/ZenLib\
%{_libdir}/%{name_without_0_ending}.so\
%if 0%{?fedora} < 37 && 0%{?mageia} < 9 && 0%{?rhel} < 10 \
%{_libdir}/%{name_without_0_ending}.la\
%endif \
%{_libdir}/pkgconfig/*.pc

%files -n %{name_without_0_ending}-devel
%{devel_files}

%if 0%{?rhel}
%files -n %{name_without_0_ending}%{libzen_suffix}-devel
%{devel_files}
%endif

%changelog
* Wed Dec 21 2022 MediaArea.net SARL <info@mediaarea.net> - 0.4.41-0
- See History.txt for more info and real dates
- Previous packages made by Toni Graffy <toni@links2linux.de>
- Fedora style made by Vasiliy N. Glazov <vascom2@gmail.com>
