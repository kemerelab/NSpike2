bin_PROGRAMS = nspike

nspike_SOURCES = spike_main.cpp spike_dsp.cpp \
    ../src-modules/spike_message.cpp \
    ../src-modules/spike_config.cpp
nspike_LDFLAGS = -L @QTDIR@/lib 
nspike_LDADD = @top_srcdir@/src-gui/libnspike.a -lpthread -lGLU -lGL -lQt3Support -lQtOpenGL -lQtGui -lQtCore

AM_CPPFLAGS = -pipe -D_REENTRANT -Wall -W -DQT_NO_DEBUG -DQT_QT3SUPPORT_LIB -DQT3_SUPPORT -DQT_OPENGL_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED $(all_includes) 

INCLUDES = -I@top_srcdir@/include -I@top_srcdir@/src-gui \
-I@QTDIR@/mkspecs/linux-g++ -I. -I@QTDIR@/include/QtCore \
-I@QTDIR@/include/QtGui -I@QTDIR@/include/QtOpenGL \
-I@QTDIR@/include/Qt3Support -I@QTDIR@/include
# -I/usr/share/qt4/mkspecs/linux-g++ -I. -I/usr/include/qt4/QtCore \
# -I/usr/include/qt4/QtGui -I/usr/include/qt4/QtOpenGL \
# -I/usr/include/qt4/Qt3Support -I/usr/include/qt4

EXTRA_DIST = spike_main_shared.cpp


