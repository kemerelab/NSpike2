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
#include "userConfigureStimulators.h"
//#include "spike_main.h"

#include <QtGui>
#include <q3listbox.h>
#include <q3buttongroup.h>
#include <Q3PopupMenu>
#include <Q3GridLayout>

// extern DisplayInfo dispinfo;
// extern NetworkInfo netinfo;
// extern MatlabInfo matlabinfo;
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

    connect(qtab, SIGNAL(currentChanged(int)), this, SLOT(switchFunction(int)));

    ConfigWidget = new ConfigForm(this);
    qtab->addTab(ConfigWidget,"Global Settings");

    qtab->addTab(new StimConfigTab(this), "Configure Stimulators");

    qtab->addTab(new QWidget(this), "Output-Only Experiments");
    qtab->addTab(new StimForm(this),"Trigger Pulses");

    qtab->addTab(new RippleTab(this), "Real-time Feedback Experiments");

    /*
    qtab->addTab(new StimForm(this),"Trigger Pulses");

    pulseFileTabWidget = new PulseFileTab(this);
    qtab->addTab(pulseFileTabWidget,"Pulses From File");

    qtab->addTab(new ThetaTab(this),"Theta Phase");

    qtab->addTab(new RippleTab(this),"Ripple Disruption");

    qtab->addTab(new LatencyTab(this),"Test Latency");
    */

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(qtab);

    setLayout(layout);

    show();
}

DIOInterface::~DIOInterface() 
{
}

void DIOInterface::switchFunction(int whichProgram)
{

  if (whichProgram == 0) {
  }
  else {
    if (!digioinfo.outputfd) {
      QMessageBox::warning(this,"No User Program","No user program appears to be running (messaging established)");
#ifndef GUI_TESTING
      return;
#endif
    }

    /*
    if (qtab->id(qtab->visibleWidget()) == 0) { // current widget is config tab
      ((ConfigForm *)(qtab->visibleWidget()))->updateStimPins(); // update stim pins before we leave
      daq_io_widget->updateChan(daq_io_widget->StimChan);
    }
    */

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

void DIOInterface::enableTabs(bool enable)
{
  /*if (enable) {
    for (int i = 1; i <= 5; i++)
      indexGroup->find(i)->setEnabled(TRUE);
  }
  else {
    for (int i = 1; i <= 5; i++)
      indexGroup->find(i)->setEnabled(FALSE);
  }
  */
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
    case DIO_RT_RIPPLE_STATUS :
      emit rippleStatusUpdate(*((RippleStatusMsg *)data));
      break;
    default:
      fprintf(stderr,"Unknown message from user program (%d)\n",msg);
      break;
  }
}

