

TEMPLATE = app
TARGET = nspike
LIBS += ../src-gui/libnspike.a

SOURCES += ../src-modules/spike_message.cpp
SOURCES += ../src-modules/spike_config.cpp
SOURCES += spike_main.cpp
SOURCES += spike_dsp.cpp
INCLUDEPATH += ../include
INCLUDEPATH += ../src-gui
HEADERS += ../include/spikecommon.h ../include/spike_main.h

QT += qt3support 
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

DEPENDPATH = gui
CONFIG   += debug qt warn_on release opengl thread
LANGUAGE = C++
#The following line was inserted by qt3to4
QT +=  opengl 
