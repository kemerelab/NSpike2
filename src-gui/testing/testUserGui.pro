TEMPLATE = app
TARGET = testUserGui

INCLUDEPATH += ../../include
SOURCES += main.cpp
SOURCES += ../spikeUserGUI.cpp
SOURCES += ../spikeUserThetaGUI.cpp 
SOURCES += ../spikeUserRippleGUI.cpp
SOURCES += ../spikeUserLatencyGUI.cpp 
SOURCES += ../spikeUserPulseGUI.cpp
SOURCES += ../spikeUserStimForm.cpp 
SOURCES += ../spikeUserConfigForm.cpp
HEADERS += ../spikeUserGUI.h 
SOURCES += ../userConfigureStimulators.cpp
HEADERS += ../userConfigureStimulators.h 
RESOURCES += ../guiresources.qrc
QT += qt3support

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
DEPENDPATH = gui
CONFIG	+= debug qt warn_on release opengl thread 
LANGUAGE	= C++
#The following line was inserted by qt3to4
QT +=  opengl 
DEFINES += GUI_TESTING
