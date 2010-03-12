
TEMPLATE = app

SV_UNIT_PACKAGES = vamp vamp-hostsdk samplerate mad id3tag oggz fishsound sndfile lrdf redland rasqal raptor

#linux-g++:LIBS += -Wl,-Bstatic
#linux-g++:DEFINES += BUILD_STATIC

load(../prf/sv.prf)

CONFIG += sv qt thread warn_on stl rtti exceptions console
QT += xml network
QT -= gui

# Using the "console" CONFIG flag above should ensure this happens for
# normal Windows builds, but the console feature doesn't get picked up
# in my local cross-compile setup because qmake itself doesn't know to
# look for win32 features
win32-x-g++:QMAKE_LFLAGS += -Wl,-subsystem,console

# If you have compiled your Vamp plugin SDK with FFTW (using its
# HAVE_FFTW3 flag), you can define the same flag here to ensure the
# program saves and restores FFTW wisdom in its configuration properly
#
#DEFINES += HAVE_FFTW3

TARGET = sonic-annotator

DEPENDPATH += . .. i18n main
INCLUDEPATH += . .. main
LIBPATH = ../data ../plugin ../rdf ../transform ../base ../system $$LIBPATH

QMAKE_CXXFLAGS_RELEASE += -fmessage-length=80 -fdiagnostics-show-location=every-line

contains(DEFINES, BUILD_STATIC):LIBS -= -ljack

LIBS = -lsvdata -lsvtransform -lsvplugin -lsvrdf -lsvdata -lsvbase -lsvsystem  $$LIBS

PRE_TARGETDEPS += ../data/libsvdata.a \
                  ../transform/libsvtransform.a \
                  ../plugin/libsvplugin.a \
                  ../rdf/libsvrdf.a \
                  ../base/libsvbase.a \
                  ../system/libsvsystem.a

OBJECTS_DIR = tmp_obj
MOC_DIR = tmp_moc

# Input
HEADERS += \
	AudioDBFeatureWriter.h \
        FeatureWriterFactory.h  \
        DefaultFeatureWriter.h \
        FeatureExtractionManager.h

SOURCES += \
	main.cpp \
	DefaultFeatureWriter.cpp \
	FeatureExtractionManager.cpp \
        AudioDBFeatureWriter.cpp \
        FeatureWriterFactory.cpp




# Restore dynamic linkage, in case we went static earlier
linux-g++:LIBS += -Wl,-Bdynamic -lpthread -ldl -lz
