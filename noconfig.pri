
CONFIG += release

#CONFIG -= release
#CONFIG += debug

DEFINES += NDEBUG BUILD_RELEASE
DEFINES += NO_TIMING

# Full set of defines expected for all platforms when we have the
# sv-dependency-builds subrepo available to provide the dependencies.

DEFINES += \
        HAVE_BZ2 \
	HAVE_FFTW3 \
	HAVE_FFTW3F \
	HAVE_SNDFILE \
	HAVE_SAMPLERATE \
	HAVE_MAD \
	HAVE_ID3TAG
        
# Default set of libs for the above. Config sections below may update
# these.

LIBS += \
        -lbz2 \
	-lrubberband \
	-lfftw3 \
	-lfftw3f \
	-lsndfile \
	-lFLAC \
	-logg \
	-lvorbis \
	-lvorbisenc \
	-lvorbisfile \
	-logg \
	-lmad \
	-lid3tag \
	-lsamplerate \
	-lz \
	-lsord-0 \
	-lserd-0

win32-g++ {

    # This config is currently used for 32-bit Windows builds.

    INCLUDEPATH += sv-dependency-builds/win32-mingw/include

    LIBS += -Lrelease -Lsv-dependency-builds/win32-mingw/lib -L../sonic-annotator/sv-dependency-builds/win32-mingw/lib

    DEFINES += NOMINMAX _USE_MATH_DEFINES USE_OWN_ALIGNED_MALLOC CAPNP_LITE

    QMAKE_CXXFLAGS_RELEASE += -ffast-math
    
    LIBS += -lwinmm -lws2_32
}

win32-msvc* {

    # This config is actually used only for 64-bit Windows builds.
    # even though the qmake spec is still called win32-msvc*. If
    # we want to do 32-bit builds with MSVC as well, then we'll
    # need to add a way to distinguish the two.
    
    INCLUDEPATH += sv-dependency-builds/win64-msvc/include

    CONFIG(release) {
        LIBS += -Lrelease \
            -L../sonic-annotator/sv-dependency-builds/win64-msvc/lib
    }

    DEFINES += NOMINMAX _USE_MATH_DEFINES USE_OWN_ALIGNED_MALLOC

    QMAKE_CXXFLAGS_RELEASE += -fp:fast

    # No Ogg/FLAC support in the sndfile build on this platform yet
    LIBS -= -lFLAC -logg -lvorbis -lvorbisenc -lvorbisfile

    # These have different names
    LIBS -= -lsord-0 -lserd-0
    LIBS += -lsord -lserd
    
    LIBS += -ladvapi32 -lwinmm -lws2_32
}

macx* {

    # All Mac builds are 64-bit these days.

    INCLUDEPATH += sv-dependency-builds/osx/include
    LIBS += -Lsv-dependency-builds/osx/lib

    QMAKE_CXXFLAGS_RELEASE += -ffast-math

    DEFINES += MALLOC_IS_ALIGNED HAVE_VDSP
    LIBS += \
	-framework CoreFoundation \
	-framework CoreServices \
	-framework Accelerate
}

linux* {

    message("Building without ./configure on Linux is unlikely to work")
    message("If you really want to try it, remove this from noconfig.pri")
    error("Refusing to build without ./configure first")
}

