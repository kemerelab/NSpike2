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
//#include "spike_main.h"
#include <q3buttongroup.h>
#include <qsizepolicy.h>
//Added by qt3to4:
#include <Q3GridLayout>
#include <QShowEvent>
#include <QLabel>
#include <string>
#include <sstream>
#include <qmessagebox.h>

extern DigIOInfo digioinfo;

RippleTab::RippleTab (QWidget *parent)
  : QWidget(parent)
{
    QString s;

    QGridLayout *layout = new QGridLayout;

    // QComboBox *stimulatorSelectComboBox = new QComboBox;
    // stimulatorSelectComboBox->addItem("None");
    // stimulatorSelectComboBox->addItem("A");
    // stimulatorSelectComboBox->addItem("B");
    // stimulatorSelectComboBox->addItem("Both (Alternating)");

    layout->addWidget(new QLabel("Tetrode / channel", this), 0,
        0, 1, 2, Qt::AlignRight);
    StimChan = new QComboBox( false, this, "Channel Combo Box" );
    StimChan->insertStringList(*(daq_io_widget->ChannelStrings));
    layout->addWidget(StimChan, 0, 2);
    connect(StimChan, SIGNAL(activated( int )), daq_io_widget, SLOT(updateChan(int)));
    connect(daq_io_widget, SIGNAL(updateChanDisplay(int)), this, SLOT(changeStimChanDisplay(int)));
    connect(StimChan, SIGNAL(activated(int)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Pulse Length (100 us units)",
          this), 1, 0, 1, 2, Qt::AlignRight);
    pulse_len = new QSpinBox (0, 10000, 1, this, "Pulse Length");
    pulse_len->setValue(DIO_RT_DEFAULT_PULSE_LEN);
    layout->addWidget(pulse_len, 1, 2);
    connect(pulse_len, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Rip. Coeff 1", this), 2, 0, 1,
        2, Qt::AlignRight);
    ripCoeff1 = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_COEFF1), this);
    ripCoeff1->setValidator(new QDoubleValidator(this));
    ripCoeff1->setAlignment(Qt::AlignRight);
    layout->addWidget(ripCoeff1, 2, 2);
    connect(ripCoeff1, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Rip. Coeff 2", this),
        2, 3, 1, 2, Qt::AlignRight);
    ripCoeff2 = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_COEFF2), this);
    ripCoeff2->setValidator(new QDoubleValidator(this));
    ripCoeff2->setAlignment(Qt::AlignRight);
    layout->addWidget(ripCoeff2, 2, 5);
    connect(ripCoeff2, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Ripple Threshold (sd)", this),
        3, 0, 1, 2, Qt::AlignRight);
    ripThresh = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_THRESHOLD), this);
    ripThresh->setValidator(new QDoubleValidator(this));
    ripThresh->setAlignment(Qt::AlignRight);
    layout->addWidget(ripThresh, 3, 2);
    connect(ripThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Lockout period (usec)", this),
        3, 3, 1, 2, Qt::AlignRight);
    lockoutPeriod = new QSpinBox (0, 20000, 1, this);
    lockoutPeriod->setValue(DIO_RT_DEFAULT_RIPPLE_LOCKOUT);
    layout->addWidget(lockoutPeriod, 3, 5);
    connect(lockoutPeriod, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Time Delay (msec)", this), 4,
        0, 1, 2, Qt::AlignRight);
    timeDelay = new QSpinBox (0, 10000, DIO_RT_DEFAULT_RIPPLE_TIME_DELAY, this, "Time Delay");
    timeDelay->setValue(0);
    layout->addWidget(timeDelay, 4, 2);
    connect(timeDelay, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Jitter (msec)", this), 4, 4,
        Qt::AlignRight);
    timeJitter = new QSpinBox (0, 10000, DIO_RT_DEFAULT_RIPPLE_JITTER, this, "Time Jitter");
    timeJitter->setValue(0);
    layout->addWidget(timeJitter, 4, 5);
    connect(timeJitter, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));

    layout->addWidget(new QLabel("Maximum speed (cm/s)", this),
        5, 0, 1, 2, Qt::AlignRight);
    speedThresh = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_SPEED_THRESH), this);
    speedThresh->setValidator(new QDoubleValidator(this));
    speedThresh->setAlignment(Qt::AlignRight);
    layout->addWidget(speedThresh, 5, 2);
    connect(speedThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

    statusBox = new QLabel("<b>Status:</b>", this);
    //statusBox->setFrameStyle(QFrame::Box | QFrame::Plain);
    layout->addWidget(statusBox, 6, 1, 2, 2);
    connect(daq_io_widget, SIGNAL(rippleStatusUpdate(RippleStatusMsg)), this, SLOT(updateRippleStatus(RippleStatusMsg)));

    enableButton = new QPushButton("Enable Stimulation", this);
    enableButton->setToggleButton(TRUE);
    enableButton->setEnabled(false);
    layout->addWidget(enableButton, 6, 5);
    connect(enableButton, SIGNAL(toggled(bool)), this, SLOT(enableRipStim(bool)));

    startButton = new QPushButton("Start", this, "start");
    startButton->setToggleButton(TRUE);
    startButton->setEnabled(false);
    layout->addWidget(startButton, 9, 1);
    connect(startButton, SIGNAL(toggled(bool)), this, 
	    SLOT(startRTData(bool)));

    stopButton = new QPushButton("Stop", this, "stop");
    stopButton->setToggleButton(TRUE);
    layout->addWidget(stopButton, 9, 4);
    connect(stopButton, SIGNAL(toggled(bool)), this, 
	    SLOT(stopRTData(bool)));


    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(checkRippleStatus()));

    setLayout(layout);
}

void RippleTab::updateRippleData(void)
{
  RippleStimParameters data;

  data.pulse_length = pulse_len->value();
  data.ripCoeff1 = ripCoeff1->text().toDouble();
  data.ripCoeff2 = ripCoeff2->text().toDouble();
  data.time_delay = timeDelay->value();
  data.jitter = timeJitter->value();
  data.ripple_threshold = ripThresh->text().toDouble();
  data.lockout = lockoutPeriod->value();
  data.speed_threshold = speedThresh->text().toDouble();

  if (digioinfo.outputfd) {
    SendDAQUserMessage(DIO_SET_RT_RIPPLE_PARAMS, (char *) &data, sizeof(RippleStimParameters));
    startButton->setEnabled(TRUE);
  }
  else {
    QMessageBox::warning(this,"No User Program","No user program is currently running");
  }
}

void RippleTab::startRTData(bool on)
{
    if (on) {
	if (digioinfo.outputfd) {
    updateRippleData();
    SendDAQUserMessage(DIO_RT_ENABLE, NULL, 0);
    //SendMessage(digioinfo.outputfd, DIO_RIPPLE_STIM_START, NULL, 0);
	  startButton->setPaletteForegroundColor("green");
	  startButton->setText("Running");
	  stopButton->setPaletteForegroundColor("black");
	  stopButton->setText("Stop");
	  stopButton->setOn(false);

    enableButton->setEnabled(true);

    timer->start(500);
	}
	else {
	  QMessageBox::warning(this,"No User Program","No user program is currently running");
	  startButton->setOn(false);
	}
    }
}

void RippleTab::stopRTData(bool on)
{
    if (on) {
	if (digioinfo.outputfd) {
	    SendMessage(digioinfo.outputfd, DIO_RIPPLE_STIM_STOP, NULL, 0);
	    SendDAQUserMessage(DIO_RT_DISABLE, NULL, 0);
	    startButton->setPaletteForegroundColor("black");
	    startButton->setText("Start");
	    stopButton->setPaletteForegroundColor("red");
	    stopButton->setText("Stopped");
	    startButton->setOn(false);

      if (enableButton->isOn()) {
        enableRipStim(false);
        enableButton->setOn(false);
        enableButton->setEnabled(false);
      }
      else
        enableButton->setEnabled(false);

      timer->stop();
	}
	else {
	    QMessageBox::warning(this,"No User Program","No user program is currently running");
	    stopButton->setOn(false);
	}
    }
}

void RippleTab::enableRipStim(bool on)
{
	if (digioinfo.outputfd) {
    if (on) {
      SendMessage(digioinfo.outputfd, DIO_RIPPLE_STIM_START, NULL, 0);
      enableButton->setText("Disable Stim");
    }
    else {
      SendMessage(digioinfo.outputfd, DIO_RIPPLE_STIM_STOP, NULL, 0);
      enableButton->setText("Enable Stim");
    }
  }
	else {
	  QMessageBox::warning(this,"No User Program","No user program is currently running");
	}
}


void RippleTab::checkRippleStatus(void)
{
	if (digioinfo.outputfd) {
	    SendMessage(digioinfo.outputfd, DIO_QUERY_RT_RIPPLE_STATUS, NULL, 0);
	}
	else {
	    QMessageBox::warning(this,"No User Program","No user program is currently running");
	}
}

void RippleTab::updateRippleStatus(RippleStatusMsg status)
{
  QString str;
  str.sprintf("<b>Status:</b><p>Mean (Std): %f (%f)<br>Time since last: %d <br>Speed: %f", 
        status.mean, status.std, status.sincelast, status.ratSpeed);
  statusBox->setText(str);
}

void RippleTab::showEvent(QShowEvent *e) {
  QWidget::showEvent(e);
  updateRippleData();
  stopRTData(TRUE);
}

