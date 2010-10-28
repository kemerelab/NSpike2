
#include "spikecommon.h"
#include "userRealtimeFeedbackTab.h"
#include "spikeUserGUI.h"

extern DAQ_IO *daq_io_widget; // global
extern SysInfo sysinfo; // global

RealtimeFeedbackTab::RealtimeFeedbackTab (QWidget *parent)
  : QWidget(parent)
{
  QGridLayout *layout = new QGridLayout;

  stimulatorSelectComboBox = new QComboBox;
  stimulatorSelectComboBox->addItem("None");
  stimulatorSelectComboBox->addItem("A");
  stimulatorSelectComboBox->addItem("B");
  stimulatorSelectComboBox->addItem("Both");
  // Signals/Slots for stimulatorSelectComboBox are connected in
  //  main GUI code to StimConfigTab.
  layout->addWidget(new QLabel("Active Stimulator"),0,0, 
      Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(stimulatorSelectComboBox,0,1);

  QComboBox *feedbackAlgorithmComboBox = new QComboBox;
  feedbackAlgorithmComboBox->addItem("None");
  feedbackAlgorithmComboBox->addItem("Latency Test");
  feedbackAlgorithmComboBox->addItem("Theta Phase");
  feedbackAlgorithmComboBox->addItem("Ripple Disruption");
  layout->addWidget(new QLabel("Realtime Feedback Program"),1,0, Qt::AlignRight);
  layout->addWidget(feedbackAlgorithmComboBox,1,1);

  realtimeDataStatus = new QLabel();
  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(checkRealtimeStatus()));
  timer->start(500);
 
  layout->addWidget(realtimeDataStatus,2,0,1,2,Qt::AlignCenter);
  // click signal connected in spikeUserGUI

  status = new QLabel("waiting...");
  statusGroupBox = new QGroupBox("Status");
  statusGroupBox->setStyleSheet("QGroupBox{border: 1px solid black;border-radius: 5px; margin-top: 1ex;}" \
                "QGroupBox::title{subcontrol-origin: margin; subcontrol-position:top center; padding: 0 3px;}");
  QVBoxLayout *statusBoxLayout = new QVBoxLayout;
  statusBoxLayout->addWidget(status,Qt::AlignCenter);
  statusGroupBox->setLayout(statusBoxLayout);
  layout->addWidget(statusGroupBox,3,0,2,2);

  resetFeedbackButton = new QPushButton("Reset Feedback");
  layout->addWidget(resetFeedbackButton,5,0,1,2, Qt::AlignCenter);
  // click signal connected in spikeUserGUI

  startFeedbackButton = new QPushButton("Start Feedback");
  layout->addWidget(startFeedbackButton,6,0, Qt::AlignCenter);
  // click signal connected in spikeUserGUI

  stopFeedbackButton = new QPushButton("Stop Feedback");
  layout->addWidget(stopFeedbackButton,6,1, Qt::AlignCenter);
  // click signal connected in spikeUserGUI

  algorithmAlternativesStack = new QStackedWidget;
  QLabel *noAlgorithm = new QLabel("Select a Feedback Control Algorithm");
  algorithmAlternativesStack->addWidget(noAlgorithm);

  algorithmAlternativesStack->addWidget(new LatencyTest(this));
  algorithmAlternativesStack->addWidget(new ThetaPhaseStim(this));
  algorithmAlternativesStack->addWidget(new RippleDisruption(this));

  layout->addWidget(algorithmAlternativesStack,0,2,4,5);

  setLayout(layout);

  connect(feedbackAlgorithmComboBox, SIGNAL(currentIndexChanged(int)), 
      this, SLOT(setFeedbackAlgorithm(int)));
}

void RealtimeFeedbackTab::setFeedbackAlgorithm (int index)
{
  int mode;

  algorithmAlternativesStack->setCurrentIndex(index);

  switch(index) {
    case 0:
      mode = DIO_RTMODE_DEFAULT;
      break;
    case 1:
      mode = DIO_RTMODE_LATENCY_TEST;
      break;
    case 2:
      mode = DIO_RTMODE_THETA;
      break;
    case 3:
      mode = DIO_RTMODE_RIPPLE_DISRUPT;
      break;
  }

  qDebug("sending mode: %d\n", mode);
  SendUserDataMessage(DIO_STIMCONTROL_MODE, (char *)&mode, sizeof(int));
}

void RealtimeFeedbackTab::checkRealtimeStatus (void)
/* check the status of real time processing */
{
  /* first see if the userdata is being sent */
  if (sysinfo.userdataon) {
    this->realtimeDataStatus->setText("Realtime Data Enabled");

  }
  else {
    this->realtimeDataStatus->setText("Realtime Data Disabled");
  }

  /* now query spike_userdata to get the status of ripple disruption */
  SendUserDataMessage(DIO_QUERY_RT_FEEDBACK_STATUS, NULL, 0);
  return;
}

void RealtimeFeedbackTab::updateRealtimeStatus(RippleStatusMsg r)
{
  QString str;
  str.sprintf("Ripple Mean (Std): %f (%f)<br>Time since last: %d <br>Speed: %f", 
        r.mean, r.std, r.sincelast, r.ratSpeed);
  status->setText(str);
}

LatencyTest::LatencyTest(QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *algorithmParametersGroupBox = new QGroupBox("Latency Test Parameters");
  QFormLayout *parametersLayout = new QFormLayout;

  QComboBox *StimChan = new QComboBox( false, this, "Channel Combo Box" );
  StimChan->insertStringList(*(daq_io_widget->ChannelStrings));
  parametersLayout->addRow("Tetrode / channel", StimChan);

  QSpinBox *thresh = new QSpinBox;
  thresh->setRange(0,65535);
  thresh->setValue(65535);
  thresh->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Threshold", thresh);

  //connect(StimChan, SIGNAL(activated( int )), daq_io_widget, SLOT(updateChan(int)));
  //connect(daq_io_widget, SIGNAL(updateChanDisplay(int)), this, SLOT(changeStimChanDisplay(int)));
  //connect(thresh, SIGNAL(valueChanged(int)), this, SLOT(updateLatencyData(void)));
  
  algorithmParametersGroupBox->setLayout(parametersLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(algorithmParametersGroupBox,Qt::AlignCenter);
  setLayout(layout);
}

ThetaPhaseStim::ThetaPhaseStim(QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *algorithmParametersGroupBox = new QGroupBox("Theta Phase Stimulation Parameters");
  QFormLayout *parametersLayout = new QFormLayout;

  QComboBox *StimChan = new QComboBox( false, this, "Channel Combo Box" );
  StimChan->insertStringList(*(daq_io_widget->ChannelStrings));
  parametersLayout->addRow("Tetrode / channel", StimChan);

  QLineEdit *speedThresh = new QLineEdit(QString::number(DIO_RT_DEFAULT_THETA_VEL), this);
  speedThresh->setValidator(new QDoubleValidator(0.0,100.0,2,this));
  speedThresh->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Speed threshold (cm/s)", speedThresh);

  QSpinBox *filterDelay = new QSpinBox;
  filterDelay->setRange(0, 20000);
  filterDelay->setValue(DIO_RT_DEFAULT_THETA_FILTER_DELAY);
  filterDelay->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Filter Delay (msec)", filterDelay);

  QSpinBox *thetaPhase = new QSpinBox;
  thetaPhase->setRange(0,360*4);
  thetaPhase->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Desired phase of stimulation (deg)", thetaPhase);

  algorithmParametersGroupBox->setLayout(parametersLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(algorithmParametersGroupBox,Qt::AlignCenter);
  setLayout(layout);
}


RippleDisruption::RippleDisruption(QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *algorithmParametersGroupBox = new QGroupBox("Ripple Disruption Parameters");
  QFormLayout *parametersLayout = new QFormLayout;

/*  QComboBox *StimChan = new QComboBox( false, this, "Channel Combo Box" );
  StimChan->insertStringList(*(daq_io_widget->ChannelStrings));
  parametersLayout->addRow("Tetrode / channel", StimChan); */

  ripCoeff1 = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_COEFF1));
  ripCoeff1->setValidator(new QDoubleValidator(0.0,2.0,3,this));
  ripCoeff1->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Rip. Coeff 1", ripCoeff1);

  ripCoeff2 = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_COEFF2));
  ripCoeff2->setValidator(new QDoubleValidator(0.0,2.0,3,this));
  ripCoeff2->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Rip. Coeff 2", ripCoeff2);

  ripThresh = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_THRESHOLD));
  ripThresh->setValidator(new QDoubleValidator(0.0,20.0,2,this));
  ripThresh->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Ripple Threshold (sd)", ripThresh);

  lockoutPeriod = new QSpinBox;
  lockoutPeriod->setRange(0,20000);
  lockoutPeriod->setValue(DIO_RT_DEFAULT_RIPPLE_LOCKOUT);
  lockoutPeriod->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Lockout period (100 usec)", lockoutPeriod);

  timeDelay = new QSpinBox;
  timeDelay->setRange(0,10000);
  timeDelay->setValue(DIO_RT_DEFAULT_RIPPLE_TIME_DELAY);
  timeDelay->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Time delay (msec)", timeDelay);

  timeJitter = new QSpinBox;
  timeJitter->setRange(0,10000);
  timeJitter->setValue(DIO_RT_DEFAULT_RIPPLE_JITTER);
  timeJitter->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Jitter (msec)", timeJitter);

  speedThresh = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_SPEED_THRESH), this);
  speedThresh->setValidator(new QDoubleValidator(0.0,100.0,2,this));
  speedThresh->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Maximum speed (cm/s)", speedThresh);

  // think about using a qsignalmapper 
  // file:///usr/share/qt4/doc/html/qsignalmapper.html

  connect(ripCoeff1, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  connect(ripCoeff2, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  connect(ripThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  connect(lockoutPeriod, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(timeDelay, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(timeJitter, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(speedThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

  connect(daq_io_widget, SIGNAL(rippleStatusUpdate(RippleStatusMsg)), parentWidget(), SLOT(updateRealtimeStatus(RippleStatusMsg)));
  
  algorithmParametersGroupBox->setLayout(parametersLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(algorithmParametersGroupBox,Qt::AlignCenter);
  setLayout(layout);
}


/*
    grid->addMultiCellWidget(new QLabel("Timing/RT IO Tetrode / channel:", this), 4, 4, 1, 2);
    StimChan = new QComboBox( FALSE, this, "Timer Channel Combo Box" );
    StimChan->insertStringList(*(daq_io_widget->ChannelStrings));
    grid->addMultiCellWidget(StimChan, 4, 4, 3, 3);
    connect(StimChan, SIGNAL(activated( int )), daq_io_widget, SLOT(updateChan(int)));
    connect(daq_io_widget, SIGNAL(updateChanDisplay(int)), this, SLOT(changeStimChanDisplay(int)));
*/


void RippleDisruption::updateRippleData(void)
{
  RippleStimParameters data;

  fprintf(stderr, "updating ripple data\n");

  data.ripCoeff1 = ripCoeff1->text().toDouble();
  data.ripCoeff2 = ripCoeff2->text().toDouble();
  data.time_delay = timeDelay->value();
  data.jitter = timeJitter->value();
  data.ripple_threshold = ripThresh->text().toDouble();
  data.lockout = lockoutPeriod->value();
  data.speed_threshold = speedThresh->text().toDouble();

  SendUserDataMessage(DIO_SET_RIPPLE_STIM_PARAMS, (char *) &data, sizeof(RippleStimParameters));
}

/*void RippleDisruption::startRTData(bool on)
{
  if (on) {
    updateRippleData();
    SendUserDataMessage(DIO_START_RT_FEEDBACK), NULL, 0);
    startButton->setPaletteForegroundColor("green");
    startButton->setText("Running");
    stopButton->setPaletteForegroundColor("black");
    stopButton->setText("Stop");
    stopButton->setOn(false);
    enableButton->setEnabled(true);
    timer->start(500);
  }
}

void RippleDisruption::stopRTData(bool on)
{
  if (on) {
    SendUserDataMessage(DIO_STOP_RT_FEEDBACK, NULL, 0);
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
} */

/*void RippleDisruption::enableRipStim(bool on)
{
  if (on) {
    SendMessage(digioinfo.outputfd, DIO_RIPPLE_STIM_START, NULL, 0);
    enableButton->setText("Disable Stim");
  }
  else {
    SendMessage(digioinfo.outputfd, DIO_RIPPLE_STIM_STOP, NULL, 0);
    enableButton->setText("Enable Stim");
  }
}



void RippleDisruption::showEvent(QShowEvent *e) {
  QWidget::showEvent(e);
  updateRippleData();
  stopRTData(TRUE);
} */

