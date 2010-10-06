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

# SOURCES += spikeUserThetaGUI.cpp 
# SOURCES += userRippleGUI.cpp
# SOURCES += spikeUserLatencyGUI.cpp 
# SOURCES += spikeUserPulseGUI.cpp
# SOURCES += spikeUserStimForm.cpp 

SOURCES += spikeUserGUI.cpp
HEADERS += spikeUserGUI.h 
SOURCES += userMainConfig.cpp
HEADERS += userMainConfig.h
SOURCES += userConfigureStimulators.cpp
HEADERS += userConfigureStimulators.h 
SOURCES += userOutputOnlyTab.cpp
HEADERS += userOutputOnlyTab.h 
SOURCES += userRealtimeFeedbackTab.cpp
HEADERS += userRealtimeFeedbackTab.h 

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


