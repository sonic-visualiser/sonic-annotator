%define _topdir /home/cannam/rpm/sonic-annotator
%define name sonic-annotator
%define version 0.6
%define release svn20100629

BuildRoot: %{_tmppath}/%{name}-buildroot
Summary: Sonic Annotator
License: GPL
Name: %{name}
Group: Sound
Version: %{version}
Release: %{release}

%description
Sonic Annotator is a utility program for batch feature extraction
from audio files.  It runs Vamp audio analysis plugins on audio files,
and can write the result features in a selection of formats including
CSV and RDF/Turtle.

%prep
svn co http://sv1.svn.sourceforge.net/svnroot/sv1/sonic-annotator/trunk .
#%{name}-%{version}-%{release}

%build
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig
#cd %{name}-%{version}-%{release}
/home/cannam/qt-4.6.2-static/bin/qmake -r "LIBS += -Wl,-Bstatic" "DEFINES += BUILD_STATIC"
make

%install
#cd %{name}-%{version}-%{release}
mkdir -p $RPM_BUILD_ROOT/usr/bin
install runner/sonic-annotator $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT

%files 
%defattr(-,root,root)
%{_bindir}/sonic-annotator
%doc README COPYING CHANGELOG

