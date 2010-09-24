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

#ifndef GUI_TESTING
#include "spikecommon.h"
#endif

#include "spikeUserGUI.h"
//#include "spike_main.h"

#include <QtGui>
#include <q3listbox.h>
#include <q3buttongroup.h>
#include <Q3PopupMenu>
#include <Q3GridLayout>

#ifndef GUI_TESTING
extern DisplayInfo dispinfo;
extern SysInfo sysinfo;
extern NetworkInfo netinfo;
extern DigIOInfo digioinfo;
extern MatlabInfo matlabinfo;
extern void  SendDAQUserMessage(int message, char *data, int datalen);
#else
DigIOInfo digioinfo;
SysInfo sysinfo;
void  SendDAQUserMessage(int message, char *data, int datalen) {
  qDebug("Sending DAQ User message %d",message);
  return;
}

int SendMessage(int a, int messageID, const char *c, int d) {
  qDebug("Sending message %d",messageID);
  return 0;
}
  
#endif

DAQ_IO *daq_io_widget; // global, but created in laserControl

laserControl::laserControl(QWidget* parent, 
	const char* name, bool modal, Qt::WFlags fl)
    : QDialog( parent, name, modal, fl)
{
    int i;

    this->setCaption("Stimulation Control");

    QRect r(0, 0, 600, 400);
    this->setGeometry(r);

    daq_io_widget = new DAQ_IO(this);

    //c QGridLayout *mainGrid = new QGridLayout(this, 100, 100, 0, 0, "maingrid");
    Q3GridLayout *mainGrid = new Q3GridLayout(this, 1, 3, 0, 5, "maingrid");

    Q3GridLayout *indexGrid = new Q3GridLayout(this, 6, 1, 0, 0, "indexgrid");
    mainGrid->addLayout(indexGrid,0,0);

    indexGroup = new Q3ButtonGroup(this, "name");
    indexGroup->setExclusive(TRUE);

    QPushButton *IndexConfig = new QPushButton("System Config", this, "name");
    IndexConfig->setToggleButton(TRUE);
    IndexConfig->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
    indexGrid->addWidget(IndexConfig, 0, 0); 
    indexGroup->insert(IndexConfig,0);

    QPushButton *IndexTriggerPulses = new QPushButton("Trigger Pulses", this, "name");
    IndexTriggerPulses->setToggleButton(TRUE);
    IndexTriggerPulses->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
    indexGrid->addWidget(IndexTriggerPulses, 1, 0); 
    indexGroup->insert(IndexTriggerPulses,1);

    QPushButton *IndexPulsesFromFile = new QPushButton("Pulses From File", this, "name");
    IndexPulsesFromFile->setToggleButton(TRUE);
    IndexPulsesFromFile->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
    indexGrid->addWidget(IndexPulsesFromFile, 2, 0); 
    indexGroup->insert(IndexPulsesFromFile,2);

    QPushButton *IndexThetaPhaseStim = new QPushButton("Theta Phase", this, "name");
    IndexThetaPhaseStim->setToggleButton(TRUE);
    IndexThetaPhaseStim->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
    indexGrid->addWidget(IndexThetaPhaseStim, 3, 0); 
    indexGroup->insert(IndexThetaPhaseStim,3);

    QPushButton *IndexRippleDisruption = new QPushButton("Ripple Disruption", this, "name");
    IndexRippleDisruption->setToggleButton(TRUE);
    IndexRippleDisruption->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
    indexGrid->addWidget(IndexRippleDisruption, 4, 0); 
    indexGroup->insert(IndexRippleDisruption,4);

    QPushButton *IndexTestLatency = new QPushButton("Test Latency", this, "name");
    IndexTestLatency->setToggleButton(TRUE);
    IndexTestLatency->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);
    indexGrid->addWidget(IndexTestLatency, 5, 0); 
    indexGroup->insert(IndexTestLatency,5);

    for (i = 1; i <= 5; i++)
      indexGroup->find(i)->setEnabled(FALSE);
    connect(daq_io_widget,SIGNAL(userProgramRunning(bool)),this,SLOT(enableTabs(bool)));

    mainGrid->setColStretch(0,2);
    mainGrid->setColStretch(1,0);
    mainGrid->setColStretch(2,10);

    // Create a multitabbed window
    qtab = new Q3WidgetStack(this, "name");
    mainGrid->addWidget(qtab, 0, 2);

    connect(indexGroup, SIGNAL(clicked(int)), this, SLOT(switchFunction(int)));

    /*
     * =========================================================
     * Tab 1: Trigger laser using the pulse specified 
     * =========================================================
     */

    ConfigWidget = new ConfigForm(this);
    qtab->addWidget(ConfigWidget,0);

    qtab->addWidget(new StimForm(this),1);

    pulseFileTabWidget = new PulseFileTab(this);
    qtab->addWidget(pulseFileTabWidget,2);

    qtab->addWidget(new ThetaTab(this),3);

    qtab->addWidget(new RippleTab(this),4);
    //connect(this, SIGNAL(switchToTab(int)), qtab->widget(4), SLOT(tabShown(int)));

    qtab->addWidget(new LatencyTab(this),5);

    show();
}

laserControl::~laserControl() 
{
}

void laserControl::switchFunction(int whichProgram)
{

  if (whichProgram == 0) {
      qtab->raiseWidget(whichProgram);
  }
  else {
    if (!digioinfo.outputfd) {
      QMessageBox::warning(this,"No User Program","No user program is currently running");
#ifndef GUI_TESTING
      return;
#endif
    }

    if (qtab->id(qtab->visibleWidget()) == 0) { // current widget is config tab
      ((ConfigForm *)(qtab->visibleWidget()))->updateStimPins(); // update stim pins before we leave
      daq_io_widget->updateChan(daq_io_widget->StimChan);
    }

    qtab->raiseWidget(whichProgram);

    switch (whichProgram) {
      case 1:
        SendDAQUserMessage(DIO_REQUEST_SIMPLE_STIMS, NULL, 0);
        break;
      case 2:
        SendDAQUserMessage(DIO_REQUEST_PULSE_FILE, NULL, 0);
        break;
      case 3: //theta stim
        SendDAQUserMessage(DIO_REQUEST_THETA_STIM, NULL, 0);
        break;
      case 4: //ripple disrupt
        SendDAQUserMessage(DIO_REQUEST_RIPPLE_DISRUPT, NULL, 0);
        break;
      case 5: //test latency
        SendDAQUserMessage(DIO_REQUEST_LATENCY_TEST, NULL, 0);
        break;
      default:
        break;
    }
  }

}

void laserControl::enableTabs(bool enable)
{
  if (enable) {
    for (int i = 1; i <= 5; i++)
      indexGroup->find(i)->setEnabled(TRUE);
  }
  else {
    for (int i = 1; i <= 5; i++)
      indexGroup->find(i)->setEnabled(FALSE);
  }
}


DAQ_IO::DAQ_IO (QWidget *parent)
  : QWidget(parent)
{
    QString s;

    ChannelStrings = new QStringList;

#ifdef GUI_TESTING
    sysinfo.machinenum = 0;
    sysinfo.nchannels[0] = 1;
    sysinfo.channelinfo[0][1].number = 1;
    sysinfo.channelinfo[0][1].electchan = 0;
#endif
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
    case DIO_RT_RIPPLE_STATUS :
      emit rippleStatusUpdate(*((RippleStatusMsg *)data));
      break;
    default:
      fprintf(stderr,"Unknown message from user program (%d)\n",msg);
      break;
  }
}

