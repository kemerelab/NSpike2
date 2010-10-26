
CXXFLAGS+=-g

AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = configure include/*.h
EXTRA_DIST += src-gui/nspike.pro src-gui/Makefile.am \
              src-gui/*.h src-gui/*.cpp src-gui/*.qrc \
              src-gui/testing/*.h src-gui/testing/*.cpp \
              src-gui/testing/testUserGui.pro src-gui/testing/main.cpp 
EXTRA_DIST += src-gui/images/*
EXTRA_DIST += src-gui/guiresources.qrc
EXTRA_DIST += src-gui/testing/*.cpp src-gui/testing/*.pro
EXTRA_DIST += Doc DSPLowLevelCode
#EXTRA_DIST += kspike setupnet
EXTRA_DIST += autogen.sh

dist_bin_SCRIPTS = setupnet kspike

SUBDIRS = src-modules src-fixpos src-extract src-gui src-main data

