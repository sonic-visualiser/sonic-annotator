
SV_INCLUDEPATH = \
        . \
	bqvec \
	bqvec/bqvec \
	bqfft \
	bqresample \
	piper-cpp \
        checker \
	dataquay \
	dataquay/dataquay \
	svcore \
	svcore/data \
	svcore/plugin/api/alsa \
	vamp-plugin-sdk

DEPENDPATH += $$SV_INCLUDEPATH
INCLUDEPATH += $$SV_INCLUDEPATH

# Platform defines for RtMidi
linux*:   DEFINES += __LINUX_ALSASEQ__
macx*:    DEFINES += __MACOSX_CORE__
win*:     DEFINES += __WINDOWS_MM__
solaris*: DEFINES += __RTMIDI_DUMMY_ONLY__

# Defines for Dataquay
DEFINES += USE_SORD

DEFINES += NO_HIT_COUNTS

CONFIG += qt thread warn_on stl rtti exceptions c++11
    
