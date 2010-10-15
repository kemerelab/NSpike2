/*
 * spikeUserGUI.cpp: the qt object for user program interfaces
 *
 * Copyright 2008 Loren M. Frank
 *
 * This program is part of the nspike data acquisition package.
 * nspike is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nspike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "spikecommon.h"

#include "spikeUserGUI.h"
#include "userMainConfig.h"
#include "userConfigureStimulators.h"
#include "userOutputOnlyTab.h"
#include "userRealtimeFeedbackTab.h"

#include <QtGui>
#include <q3listbox.h>
#include <q3buttongroup.h>
#include <Q3PopupMenu>
#include <Q3GridLayout>

extern SysInfo sysinfo;
extern DigIOInfo digioinfo;
extern void  SendDAQUserMessage(int message, char *data, int datalen);

DAQ_IO *daq_io_widget; // global, but created in DIOInterface

DIOInterface::DIOInterface(QWidget* parent, 
	const char* name, bool modal, Qt::WFlags fl)
    : QDialog( parent, name, modal, fl)
{
    int i;

    this->setCaption("Stimulation Control");

    QRect r(0, 0, 600, 400);
    this->setGeometry(r);

    daq_io_widget = new DAQ_IO(this);

    connect(daq_io_widget,SIGNAL(userProgramRunning(bool)),this,SLOT(enableTabs(bool)));

    qtab = new QTabWidget(this);
    qtab->setUsesScrollButtons(false);

    mainConfigTab = new MainConfigTab(this);
    qtab->insertTab(mainConfigTab,"Global Settings", MAIN_CONFIG_TAB);
    connect(mainConfigTab->modeButtonGroup, SIGNAL(buttonClicked(int)), this,
        SLOT(changeOperatingMode(int)));

    connect(mainConfigTab->loadSettingsButton, SIGNAL(clicked(void)), this, SLOT(loadSettings(void)));
    connect(mainConfigTab->saveSettingsButton, SIGNAL(clicked(void)), this, SLOT(saveSettings(void)));

    StimConfigTab *stimConfigTab = new StimConfigTab(this);
    qtab->insertTab(stimConfigTab, "Configure Stimulators",
        CONFIG_STIMULATORS_TAB);

    stimOutputOnlyTab = new StimOutputOnlyTab(this);
    qtab->insertTab(stimOutputOnlyTab, "Output-Only Experiments",
        OUTPUT_ONLY_TAB);
    connect(stimConfigTab, SIGNAL(activeStimulatorChanged(int)), 
        stimOutputOnlyTab->stimulatorSelectComboBox,SLOT(setCurrentIndex(int)));
    connect(stimOutputOnlyTab->stimulatorSelectComboBox, 
        SIGNAL(currentIndexChanged(int)), stimConfigTab, SLOT(setActiveStimulator(int)));

    realtimeFeedbackTab = new RealtimeFeedbackTab(this);
    qtab->insertTab(realtimeFeedbackTab, "Real-time Feedback Experiments", 
        REALTIME_FEEDBACK_TAB);
    connect(stimConfigTab, SIGNAL(activeStimulatorChanged(int)), 
        realtimeFeedbackTab->stimulatorSelectComboBox,SLOT(setCurrentIndex(int)));
    connect(realtimeFeedbackTab->stimulatorSelectComboBox, 
        SIGNAL(currentIndexChanged(int)), stimConfigTab, SLOT(setActiveStimulator(int)));

    /*
    pulseFileTabWidget = new PulseFileTab(this);
    qtab->addTab(pulseFileTabWidget,"Pulses From File");
    */

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(qtab);

    setLayout(layout);

    // Can we auto start user program here?

    mainConfigTab->initializeValues();

    stimConfigTab->selectStimulator(); // synchronize initial stim display

    changeOperatingMode(DEFAULT_MODE);

    show();
}

DIOInterface::~DIOInterface() 
{
}

void DIOInterface::changeOperatingMode(int mode)
{
  switch (mode) {
  case OUTPUT_ONLY_MODE: 
    qtab->setTabEnabled(OUTPUT_ONLY_TAB,true);
    qtab->setTabEnabled(REALTIME_FEEDBACK_TAB,false);
    // enable output only mode tab
    break;
  case REALTIME_FEEDBACK_MODE: 
    // enable realtime feedback mode tab
    qtab->setTabEnabled(REALTIME_FEEDBACK_TAB,true);
    qtab->setTabEnabled(OUTPUT_ONLY_TAB,false);
    break;
  case DEFAULT_MODE:
  default:
    // disable all mode tabs
    qtab->setTabEnabled(REALTIME_FEEDBACK_TAB,false);
    qtab->setTabEnabled(OUTPUT_ONLY_TAB,false);
    break;
  }
}

void DIOInterface::loadSettings(void) {
  QString settingsFilename = QFileDialog::getOpenFileName(this, 
      QString("Open Stimulator Settings"));
  qDebug() << "User looking for settings in" << settingsFilename;
}

void DIOInterface::saveSettings(void) {
  QString settingsFilename = QFileDialog::getSaveFileName(this, 
      QString("Save Stimulator Settings"));
  qDebug() << "User  wanting to save settings in" << settingsFilename;
}

void DIOInterface::enableTabs(bool enable)
{
}

DAQ_IO::DAQ_IO (QWidget *parent)
  : QWidget(parent)
{
    QString s;

    ChannelStrings = new QStringList;

    /* go through all of the channels on this machine and add them one by one */
    for (int i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
        s = QString("%1 / %2")
        .arg(sysinfo.channelinfo[sysinfo.machinenum][i].number)
        .arg(sysinfo.channelinfo[sysinfo.machinenum][i].electchan);
      ChannelStrings->append(s);
    }

    StimChan = 0;

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkUserProgramStatus()));
    timer->start(500);
}

void DAQ_IO::updateChan(int chnum)
{
  StimChan = chnum;

    memset(sysinfo.daq_to_user.dsps, 0, sizeof(int) * MAX_DSPS);
    memset(sysinfo.daq_to_user.channels, 0, sizeof(int) * MAX_CHANNELS);
    if (digioinfo.outputfd) {
	/* construct the DaqToUserInfo structure */
	/* get the dsp number for this channel */
	sysinfo.daq_to_user.dsps[sysinfo.channelinfo[sysinfo.machinenum][chnum].dspnum] = 1;
	sysinfo.daq_to_user.channels[sysinfo.channelinfo[sysinfo.machinenum][chnum].dspchan] = 1;
	SendDAQUserMessage(SETUP_DAQ_TO_USER, (char *) &sysinfo.daq_to_user, sizeof(DaqToUserInfo));

  emit updateChanDisplay(StimChan);
    }
    else {
	QMessageBox::warning(this,"No User Program","No user program is currently running");
    }
}

void DAQ_IO::checkUserProgramStatus(void)
{
  int oldUserProgram = UserProgramRunning;

#ifdef GUI_TESTING
  UserProgramRunning = 1;
#else
  if (digioinfo.inputfd)  // user program is running
    UserProgramRunning = digioinfo.currentprogram;
  else
    UserProgramRunning = -1;
#endif

  if (UserProgramRunning != oldUserProgram)
    emit changedUserProgramStatus(UserProgramRunning);

  if (UserProgramRunning >= 0) 
    emit userProgramRunning(TRUE);
}

void DAQ_IO::msgFromUser (int msg, char *data) {
  switch (msg) {
    case DIO_PULSE_SEQ_STEP :
      emit pulseFileLineExecuted(*((int *)data));
      break;
    case DIO_PULSE_SEQ_EXECUTED :
      emit pulseFileFinished();
      fprintf(stderr,"Got DIO_PULSE_SEQ_EXECUTED\n");
      break;
    case DIO_RT_STATUS_RIPPLE_DISRUPT:
      emit rippleStatusUpdate(*((RippleStatusMsg *)data));
      break;
    default:
      fprintf(stderr,"Unknown message from user program (%d)\n",msg);
      break;
  }
}

