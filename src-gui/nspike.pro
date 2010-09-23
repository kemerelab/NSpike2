SOURCES += ../src-modules/spike_message.cpp
SOURCES += ../src-modules/spike_config.cpp
INCLUDEPATH += ../include
HEADERS += ../include/sqcompat.h
HEADERS += ../include/spike_main.h
HEADERS	+= spikeGLPane.h 
SOURCES += spikeGLPane.cpp 
SOURCES += spikeMainWindow.cpp 
SOURCES += rewardControl.cpp
HEADERS += rewardControl.h
HEADERS += spikeMainWindow.h 
SOURCES += spikeInput.cpp 
HEADERS += spikeInput.h 
SOURCES += spikeUserGUI.cpp
SOURCES += spikeUserThetaGUI.cpp 
SOURCES += spikeUserRippleGUI.cpp
SOURCES += spikeUserLatencyGUI.cpp 
SOURCES += spikeUserPulseGUI.cpp
SOURCES += spikeUserStimForm.cpp 
SOURCES += spikeUserConfigForm.cpp
HEADERS += spikeUserGUI.h 
QT += qt3support

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
DEPENDPATH = gui
TEMPLATE	= lib
CONFIG	+= debug qt warn_on release opengl thread staticlib
LANGUAGE	= C++
#The following line was inserted by qt3to4
QT +=  opengl 
