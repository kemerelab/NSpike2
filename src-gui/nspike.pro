SOURCES += ../src-modules/spike_message.cpp
SOURCES += ../src-modules/spike_config.cpp
INCLUDEPATH += ../include
HEADERS += ../include/sqcompat.h
HEADERS += ../include/spike_main.h
HEADERS	+= spikeGLPane.h 
SOURCES += spikeGLPane.cpp 
SOURCES += spikeMainWindow.cpp 
HEADERS += spikeMainWindow.h 
SOURCES += spikeInput.cpp 
HEADERS += spikeInput.h 
SOURCES += spikeStatusbar.cpp
HEADERS += spikeStatusbar.h

SOURCES += rewardControl.cpp
HEADERS += rewardControl.h

SOURCES += spikeFSData.cpp
HEADERS += spikeFSData.h

# SOURCES += spikeFSThetaGUI.cpp 
# SOURCES += fsRippleGUI.cpp
# SOURCES += spikeFSLatencyGUI.cpp 
# SOURCES += spikeFSPulseGUI.cpp
# SOURCES += spikeFSStimForm.cpp 

SOURCES += spikeFSGUI.cpp
HEADERS += spikeFSGUI.h 
SOURCES += fsMainConfig.cpp
HEADERS += fsMainConfig.h
SOURCES += fsConfigureStimulators.cpp
HEADERS += fsConfigureStimulators.h 
SOURCES += fsOutputOnlyTab.cpp
HEADERS += fsOutputOnlyTab.h 
SOURCES += fsRealtimeFeedbackTab.cpp
HEADERS += fsRealtimeFeedbackTab.h 

RESOURCES += guiresources.qrc
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


