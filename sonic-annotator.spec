%define _topdir /home/cannam/rpm/sonic-annotator
%define name sonic-annotator
%define version 0.6
%define release svn

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
svn co http://sv1.svn.sourceforge.net/svnroot/sv1/sonic-annotator/trunk %{name}-%{version}-%{release}

%build
cd %{name}-%{version}-%{release}
/home/cannam/qt-4.6.3-static-release-nogui/bin/qmake -r
make

%install
cd %{name}-%{version}-%{release}
mkdir -p $RPM_BUILD_ROOT/usr/bin
install runner/sonic-annotator $RPM_BUILD_ROOT/usr/bin

%files
%defattr(-,root,root)
%{_bindir}/sonic-annotator

