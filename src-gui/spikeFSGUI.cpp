/*
 * spikeFSGUI.cpp: the qt object for feedback stimulation (fs) program interfaces
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

#include "spikeFSGUI.h"
#include "spike_main.h"

#include <QtGui>
#include <q3listbox.h>
#include <q3buttongroup.h>

extern SysInfo sysinfo;
extern DigIOInfo digioinfo;
extern SocketInfo *server_message;

DAQ_IO *daq_io_widget; // global, but created in DIOInterface

DIOInterface::DIOInterface(QWidget* parent, 
	const char* name, bool modal, Qt::WFlags fl)
    : QDialog( parent, name, modal, fl)
{
    /* check to make sure that the fsdata program is running */
    if (!sysinfo.fsdataoutput) {
      DisplayErrorMessage("Error: FSDATA not enabled in config file");
      daq_io_widget->close();
    }

    this->setCaption("Stimulation Control");

    QRect r(0, 0, 600, 400);
    this->setGeometry(r);

    daq_io_widget = new DAQ_IO(this);

    qtab = new QTabWidget(this);
    qtab->setUsesScrollButtons(false);

    mainConfigTab = new MainConfigTab(this);
    qtab->insertTab(mainConfigTab,"Global Settings", MAIN_CONFIG_TAB);
    connect(mainConfigTab->modeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(changeOperatingMode(int)));

    connect(mainConfigTab->loadSettingsButton, SIGNAL(clicked(void)), this, SLOT(loadSettings(void)));
    connect(mainConfigTab->saveSettingsButton, SIGNAL(clicked(void)), this, SLOT(saveSettings(void)));

    stimConfigTab = new StimConfigTab(this);
    qtab->insertTab(stimConfigTab, "Configure Digital Stimulators", CONFIG_STIMULATORS_TAB);

#ifndef DIO_ON_MASTER_DSP
    aOutConfigTab = new AOutConfigTab(this);
    qtab->insertTab(aOutConfigTab, "Configure Analog Out", CONFIG_ANALOG_OUT_TAB);
#endif

    stimOutputOnlyTab = new StimOutputOnlyTab(this);
    qtab->insertTab(stimOutputOnlyTab, "Output-Only Experiments", OUTPUT_ONLY_TAB);
    
    /* connections to stimConfig for digital stimulation */
    connect(stimConfigTab, SIGNAL(activeStimulatorChanged(int)), stimOutputOnlyTab->stimulatorSelectComboBox,SLOT(setCurrentIndex(int)));
    connect(stimOutputOnlyTab->stimulatorSelectComboBox, SIGNAL(currentIndexChanged(int)), stimConfigTab, SLOT(setActiveStimulator(int)));


#ifndef DIO_ON_MASTER_DSP
    /* connections to aOutConfig for analog stimulation */
    connect(aOutConfigTab, SIGNAL(activeAOutChanged(int, int, int)), stimOutputOnlyTab, SLOT(updateActiveAOut(int, int, int)));
    connect(stimOutputOnlyTab->aOutSelectComboBox, SIGNAL(currentIndexChanged(int)), aOutConfigTab, SLOT(setActiveAOut(int)));
#endif

    connect(stimOutputOnlyTab->stimSingleButton, SIGNAL(clicked()), this, SLOT(triggerSingleStim()));
    connect(stimOutputOnlyTab->startStimButton, SIGNAL(clicked()), this, SLOT(startOutputOnlyStim()));
    connect(stimOutputOnlyTab->abortStimButton, SIGNAL(clicked()), this, SLOT(abortOutputOnlyStim()));

    connect(daq_io_widget, SIGNAL(pulseSeqFinished(int)), stimOutputOnlyTab, SLOT(endStimulation(int)));
    connect(daq_io_widget, SIGNAL(pulseSeqLineExecuted(int)), stimOutputOnlyTab, SLOT(stepStimulation(int)));

    realtimeFeedbackTab = new RealtimeFeedbackTab(this);
    qtab->insertTab(realtimeFeedbackTab, "Real-time Feedback Experiments", 
        REALTIME_FEEDBACK_TAB);
    /* connections to stimConfig for digital stimulation */
    connect(stimConfigTab, SIGNAL(activeStimulatorChanged(int)), 
        realtimeFeedbackTab->stimulatorSelectComboBox,SLOT(setCurrentIndex(int)));
    connect(realtimeFeedbackTab->stimulatorSelectComboBox, 
        SIGNAL(currentIndexChanged(int)), stimConfigTab, SLOT(setActiveStimulator(int)));
#ifndef DIO_ON_MASTER_DSP
    /* connections to aOutConfig for analog stimulation */
    connect(aOutConfigTab, SIGNAL(activeAOutChanged(int, int, int)), 
	    realtimeFeedbackTab, SLOT(updateActiveAOut(int, int, int)));
    connect(realtimeFeedbackTab->aOutSelectComboBox, 
	    SIGNAL(currentIndexChanged(int)), aOutConfigTab, 
	    SLOT(setActiveAOut(int)));
#endif

    connect(realtimeFeedbackTab->resetFeedbackButton, SIGNAL(clicked()), this, SLOT(resetRealtimeStim()));
    connect(realtimeFeedbackTab->startFeedbackButton, SIGNAL(clicked()), this, SLOT(startRealtimeStim()));
    connect(realtimeFeedbackTab->stopFeedbackButton, SIGNAL(clicked()), this, SLOT(stopRealtimeStim()));

    /*
    pulseFileTabWidget = new PulseFileTab(this);
    qtab->addTab(pulseFileTabWidget,"Pulses From File");
    */

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(qtab);

    setLayout(layout);
    setWindowFlags(fl | Qt::Window);

    mainConfigTab->initializeValues();

    stimConfigTab->selectStimulator(); // synchronize initial stim display
    qtab->setTabEnabled(CONFIG_STIMULATORS_TAB,false);

    changeOperatingMode(DEFAULT_MODE);

    //setAttribute(Qt::WA_DeleteOnClose,true);
    this->enableTabs(true);
    show();

}

DIOInterface::~DIOInterface() 
{
  qDebug("Got destructor!");
  daq_io_widget->close();
}

void DIOInterface::changeOperatingMode(int mode)
{
  int diomode;
  switch (mode) {
  case OUTPUT_ONLY_MODE: 
    // enable output only mode tab
    qtab->setTabEnabled(OUTPUT_ONLY_TAB,true);
    qtab->setTabEnabled(REALTIME_FEEDBACK_TAB,false);
    diomode = DIO_RTMODE_OUTPUT_ONLY;
    SendFSDataMessage(DIO_STIMCONTROL_MODE, (char *)&diomode, sizeof(int));
    break;
  case REALTIME_FEEDBACK_MODE: 
    // enable realtime feedback mode tab
    qtab->setTabEnabled(REALTIME_FEEDBACK_TAB,true);
    qtab->setTabEnabled(OUTPUT_ONLY_TAB,false);
    /* set to the correct mode */
    realtimeFeedbackTab->setFeedbackAlgorithm(
          realtimeFeedbackTab->algorithmAlternativesStack->currentIndex());
    break;
  case DEFAULT_MODE:
  default:
    // disable all mode tabs
    qtab->setTabEnabled(REALTIME_FEEDBACK_TAB,false);
    qtab->setTabEnabled(OUTPUT_ONLY_TAB,false);
    fprintf(stderr, "disabled tabs\n");
    diomode = DIO_RTMODE_DEFAULT;
    SendFSDataMessage(DIO_STIMCONTROL_MODE, (char *)&diomode, sizeof(int));
    break;
  }
  return;
}

void DIOInterface::loadSettings(void) {
  QString settingsFilename = QFileDialog::getOpenFileName(this, 
      QString("Open Stimulator Settings"));
  qDebug() << "FS looking for settings in" << settingsFilename;
}

void DIOInterface::saveSettings(void) {
  QString settingsFilename = QFileDialog::getSaveFileName(this, 
      QString("Save Stimulator Settings"));
  qDebug() << "FS  wanting to save settings in" << settingsFilename;
}

void DIOInterface::enableTabs(bool enable)
{

  if (qtab->isTabEnabled(CONFIG_STIMULATORS_TAB) != enable)
    qtab->setTabEnabled(CONFIG_STIMULATORS_TAB,enable);

  //qDebug("Enabled == %d\n", enable);
}

void DIOInterface::triggerSingleStim(void)
{
  PulseCommand pCmd[3]; // at most 3 pulse commands are needed
#ifdef DIO_ON_MASTER_DSP
  qDebug("triggerSingleStim signal received.\nCurrent digital stimulator is: %d\n", 
      stimConfigTab->activeStimulator);
#else
  qDebug("triggerSingleStim signal received.\nCurrent digital stimulator is: %d\nCurrent Analog Output is %d", 
      stimConfigTab->activeStimulator, aOutConfigTab->activeAOut);
#endif
  /* first handle digital stimulation events */
  switch (stimConfigTab->activeStimulator) {
  case 1:
    pCmd[0] = stimConfigTab->stimConfigA->stimPulseCmd;
    pCmd[0].n_repeats = 0;
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *)pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);
    stimOutputOnlyTab->startStimulation(1);
    break;
  case 2:
    pCmd[0] = stimConfigTab->stimConfigB->stimPulseCmd;
    pCmd[0].n_repeats = 0;
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *)pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);
    stimOutputOnlyTab->startStimulation(1);
    break;
  case 3: // A then B will always be A first then B (or we can do two...)
    pCmd[0] = stimConfigTab->stimConfigA->stimPulseCmd;
    pCmd[0].n_repeats = 0;
    pCmd[1] = stimConfigTab->stimConfigB->stimPulseCmd;
    pCmd[1].pre_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    pCmd[1].n_repeats = 0;
    pCmd[2].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *)pCmd, 3*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);
    stimOutputOnlyTab->startStimulation(1);
    break;
  default:
  case 0:
    qDebug("No active digital stimulator set.");
    break;
  }

#ifndef DIO_ON_MASTER_DSP
  switch (aOutConfigTab->activeAOut) {
  case 1:
    pCmd[0] = aOutConfigTab->aOut1Config->aOutPulseCmd;
    pCmd[0].n_repeats = 0;
    pCmd[0].pre_delay = 0;
    /* copy the information to the next command */
    memcpy(pCmd+1, pCmd, sizeof(PulseCommand));
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *)pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);
    stimOutputOnlyTab->startStimulation(1);
    break;
  case 2:
    pCmd[0] = aOutConfigTab->aOut2Config->aOutPulseCmd;
    pCmd[0].n_repeats = 0;
    pCmd[0].pre_delay = 0;
    /* copy the information to the next command */
    memcpy(pCmd+1, pCmd, sizeof(PulseCommand));
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *)pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);
    stimOutputOnlyTab->startStimulation(1);
    break;
  default:
  case 0:
    qDebug("No active Analog Output set.");
    break;
  }
#endif

}

void DIOInterface::startOutputOnlyStim(void)
{
  PulseCommand pCmd[4]; // at most 3 pulse commands are needed
  qDebug("startOutputOnlyStim signal received");
  // Generate sequence of pulse commands
  // Start with the digital stimulator if selected
  switch (stimConfigTab->activeStimulator) {
  case 1:
    pCmd[0] = stimConfigTab->stimConfigA->stimPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *) pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);

    stimOutputOnlyTab->startStimulation(pCmd[0].n_repeats);
    break;
  case 2:
    pCmd[0] = stimConfigTab->stimConfigB->stimPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() 
	* 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *) pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);

    stimOutputOnlyTab->startStimulation(pCmd[0].n_repeats);
    break;
  case 3: 
    pCmd[0] = stimConfigTab->stimConfigA->stimPulseCmd;
    pCmd[0].pre_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    pCmd[0].n_repeats = 0;
    pCmd[1] = stimConfigTab->stimConfigB->stimPulseCmd;
    pCmd[1].pre_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    pCmd[1].n_repeats = 0;
    pCmd[2].pulse_width = DIO_PULSE_COMMAND_REPEAT;
    //pCmd[2].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;;
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[2].n_repeats = -1;
    else
      pCmd[2].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    pCmd[3].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *) pCmd, 4*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);

    stimOutputOnlyTab->startStimulation(pCmd[2].n_repeats);
    break;
  default:
  case 0:
    qDebug("No active stimulator set.");
    break;
  }

#ifdef DIO_ON_MASTER_DSP
  // Trigger the analog output if selected
  switch (aOutConfigTab->activeAOut) {
  case 1:
    pCmd[0] = aOutConfigTab->aOut1Config->aOutPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    /* copy the information to the next command */
    memcpy(pCmd+1, pCmd, sizeof(PulseCommand));
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *) pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);

    stimOutputOnlyTab->startStimulation(pCmd[0].n_repeats);
    break;
  case 2:
    pCmd[0] = aOutConfigTab->aOut2Config->aOutPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    /* copy the information to the next command */
    memcpy(pCmd+1, pCmd, sizeof(PulseCommand));
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_PULSE_SEQ, (char *) pCmd, 2*sizeof(PulseCommand));
    SendFSDataMessage(DIO_PULSE_SEQ_START,NULL,0);

    stimOutputOnlyTab->startStimulation(pCmd[0].n_repeats);
    break;
  default:
  case 0:
    qDebug("No active analog output set.");
    break;
  }
#endif
}

void DIOInterface::abortOutputOnlyStim(void)
{
  qDebug("abortOutputOnlyStim signal received");
  SendFSDataMessage(DIO_PULSE_SEQ_STOP,NULL,0);
}

void DIOInterface::resetRealtimeStim(void)
{
  qDebug("resetRealtimeStim signal received");
  SendFSDataMessage(DIO_RESET_RT_FEEDBACK, NULL, 0);
}

void DIOInterface::startRealtimeStim(void)
{
  PulseCommand pCmd[4]; // at most 3 pulse commands are needed
  
#ifndef DIO_ON_MASTER_DSP
  /* Check to see that only one analog or one digital output is selected */
  if ((aOutConfigTab->activeAOut > 0) && (stimConfigTab->activeStimulator > 0)) {
    qDebug("Error: both analog and digital stimulation selected");
    return;
  }
#endif

  qDebug("startRealtimeStim signal received");

  // Generate pulse commands
  // Start with the digital stimulator if selected
  switch (stimConfigTab->activeStimulator) {
  case 1:
    pCmd[0] = stimConfigTab->stimConfigA->stimPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_SET_RT_STIM_PULSE_PARAMS, (char *) pCmd, 2*sizeof(PulseCommand));
    break;
  case 2:
    pCmd[0] = stimConfigTab->stimConfigB->stimPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() 
	* 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_SET_RT_STIM_PULSE_PARAMS, (char *) pCmd, 2*sizeof(PulseCommand));
    break;
  case 3: 
    pCmd[0] = stimConfigTab->stimConfigA->stimPulseCmd;
    pCmd[0].pre_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    pCmd[0].n_repeats = 0;
    pCmd[1] = stimConfigTab->stimConfigB->stimPulseCmd;
    pCmd[1].pre_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    pCmd[1].n_repeats = 0;
    pCmd[2].pulse_width = DIO_PULSE_COMMAND_REPEAT;
    //pCmd[2].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;;
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[2].n_repeats = -1;
    else
      pCmd[2].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    pCmd[3].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_SET_RT_STIM_PULSE_PARAMS, (char *) pCmd, 4*sizeof(PulseCommand));
    break;
  default:
  case 0:
    qDebug("No active stimulator set.");
    break;
  }

#ifndef DIO_ON_MASTER_DSP
  // Trigger the analog output if selected
  switch (aOutConfigTab->activeAOut) {
  case 1:
    pCmd[0] = aOutConfigTab->aOut1Config->aOutPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    /* copy the information to the next command */
    memcpy(pCmd+1, pCmd, sizeof(PulseCommand));
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_SET_RT_STIM_PULSE_PARAMS, (char *) pCmd, 2*sizeof(PulseCommand));
    break;
  case 2:
    pCmd[0] = aOutConfigTab->aOut2Config->aOutPulseCmd;
    pCmd[0].pre_delay = 0;
    pCmd[0].inter_frame_delay = stimOutputOnlyTab->trainIntervalSpinBox->value() * 10.0; // convert to ticks
    if (stimOutputOnlyTab->continuousButton->isChecked())
      pCmd[0].n_repeats = -1;
    else
      pCmd[0].n_repeats = stimOutputOnlyTab->nTrainsSpinBox->value() - 1;
    /* copy the information to the next command */
    memcpy(pCmd+1, pCmd, sizeof(PulseCommand));
    pCmd[1].pulse_width = DIO_PULSE_COMMAND_END;
    SendFSDataMessage(DIO_SET_RT_STIM_PULSE_PARAMS, (char *) pCmd, 2*sizeof(PulseCommand));
    break;
  default:
  case 0:
    qDebug("No active analog output set.");
    break;
  }
#endif

  SendFSDataMessage(DIO_START_RT_FEEDBACK, NULL, 0);
  /* Now disable the start button and enable the stop button*/
  realtimeFeedbackTab->startFeedbackButton->setEnabled(false);
  realtimeFeedbackTab->stopFeedbackButton->setEnabled(true);
}


void DIOInterface::stopRealtimeStim(void)
{
  qDebug("stopRealtimeStim signal received");
  SendFSDataMessage(DIO_STOP_RT_FEEDBACK, NULL, 0);
  /* Now disable the stop button and enable the start button*/
  realtimeFeedbackTab->startFeedbackButton->setEnabled(true);
  realtimeFeedbackTab->stopFeedbackButton->setEnabled(false);
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

    setAttribute(Qt::WA_DeleteOnClose,true);

}


void DAQ_IO::msgFromFS (int msg, char *data) {
  switch (msg) {
    case DIO_PULSE_SEQ_STEP :
      emit pulseSeqLineExecuted(*((int *)data));
      break;
    case DIO_PULSE_SEQ_EXECUTED :
      emit pulseSeqFinished(*((int *)data));
      fprintf(stderr,"Got DIO_PULSE_SEQ_EXECUTED\n");
      break;
    case DIO_RT_STATUS_RIPPLE_DISRUPT:
      emit rippleStatusUpdate(data);
      break;
    case DIO_RT_STATUS_SPATIAL_STIM:
      emit spatialStatusUpdate(data);
      break;
    default:
      fprintf(stderr,"Unknown message from user program (%d)\n",msg);
      break;
  }
}


