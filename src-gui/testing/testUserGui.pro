TEMPLATE = app
TARGET = testFSGui

INCLUDEPATH += ../../include
SOURCES += main.cpp
SOURCES += ../spikeFSGUI.cpp
# SOURCES += ../spikeFSThetaGUI.cpp 
# SOURCES += ../userRippleGUI.cpp
# SOURCES += ../spikeFSLatencyGUI.cpp 
# SOURCES += ../spikeFSPulseGUI.cpp
# SOURCES += ../spikeFSStimForm.cpp 
HEADERS += ../spikeFSGUI.h 
SOURCES += ../userMainConfig.cpp
HEADERS += ../userMainConfig.h
SOURCES += ../userConfigureStimulators.cpp
HEADERS += ../userConfigureStimulators.h 
SOURCES += ../userOutputOnlyTab.cpp
HEADERS += ../userOutputOnlyTab.h 
SOURCES += ../userRealtimeFeedbackTab.cpp
HEADERS += ../userRealtimeFeedbackTab.h 
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
