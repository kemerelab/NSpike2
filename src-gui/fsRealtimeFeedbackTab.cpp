
#include "spikecommon.h"
#include "fsRealtimeFeedbackTab.h"
#include "spikeFSGUI.h"

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

  aOutSelectComboBox = new QComboBox;
  aOutSelectComboBox->addItem("None");
  aOutSelectComboBox->addItem("A");
  aOutSelectComboBox->addItem("B");
  // Signals/Slots for aOutSelectComboBox are connected in
  //  main GUI code to StimConfigTab.
  layout->addWidget(new QLabel("Active AOut"),1,0, 
      Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(aOutSelectComboBox,1,1);


  QComboBox *feedbackAlgorithmComboBox = new QComboBox;
  feedbackAlgorithmComboBox->addItem("None");
  feedbackAlgorithmComboBox->addItem("Latency Test");
  feedbackAlgorithmComboBox->addItem("Theta Phase");
  feedbackAlgorithmComboBox->addItem("Ripple Disruption");
  layout->addWidget(new QLabel("Realtime Feedback Program"),2,0, Qt::AlignRight);
  layout->addWidget(feedbackAlgorithmComboBox,2,1);

  QTimer *timer = new QTimer(this);
  connect(timer, SIGNAL(timeout()), this, SLOT(checkRealtimeStatus()));
  timer->start(500);

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
  // click signal connected in spikeFSGUI

  startFeedbackButton = new QPushButton("Start Feedback");
  layout->addWidget(startFeedbackButton,6,0, Qt::AlignCenter);
  // click signal connected in spikeFSGUI

  stopFeedbackButton = new QPushButton("Stop Feedback");
  layout->addWidget(stopFeedbackButton,6,1, Qt::AlignCenter);
  // click signal connected in spikeFSGUI

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

void RealtimeFeedbackTab::updateActiveAOut(int aOutIndex, int aOut1Mode, 
    int aOut2Mode)
{
  /* set the current analog out; at the moment we don't do anything different for
   * the modes */
  aOutSelectComboBox->setCurrentIndex(aOutIndex);
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
  SendFSDataMessage(DIO_STIMCONTROL_MODE, (char *)&mode, sizeof(int));
}

void RealtimeFeedbackTab::checkRealtimeStatus (void)
/* check the status of real time processing */
{
  /* query spike_fsdata to get the status of ripple disruption */
  SendFSDataMessage(DIO_QUERY_RT_FEEDBACK_STATUS, NULL, 0);
  return;
}

void RealtimeFeedbackTab::updateRealtimeStatus(char *s)
{
  status->setText(QString(s));
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

  sampDivisor = new QSpinBox;
  sampDivisor->setRange(0,1000000);
  sampDivisor->setValue(DIO_RT_DEFAULT_SAMP_DIVISOR);
  sampDivisor->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Sample Divisor", sampDivisor);

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

  nAboveThreshold = new QSpinBox;
  nAboveThreshold->setRange(0,MAX_ELECTRODES);
  nAboveThreshold->setValue(DIO_RT_DEFAULT_RIPPLE_N_ABOVE_THRESH);
  nAboveThreshold->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Num Above Threshold", nAboveThreshold);

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

  connect(sampDivisor, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(ripCoeff1, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  connect(ripCoeff2, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  connect(ripThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

  connect(lockoutPeriod, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(nAboveThreshold, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(timeDelay, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(timeJitter, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  connect(speedThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

  connect(daq_io_widget, SIGNAL(rippleStatusUpdate(char *)), parentWidget(), SLOT(updateRealtimeStatus(char *)));
  

  algorithmParametersGroupBox->setLayout(parametersLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(algorithmParametersGroupBox,Qt::AlignCenter);
  setLayout(layout);
}


void RippleDisruption::updateRippleData(void)
{
  RippleStimParameters data;

  data.sampDivisor = sampDivisor->value();
  data.ripCoeff1 = ripCoeff1->text().toDouble();
  data.ripCoeff2 = ripCoeff2->text().toDouble();
  data.ripple_threshold = ripThresh->text().toDouble();
  data.n_above_thresh = nAboveThreshold->value();
  data.time_delay = timeDelay->value();
  data.jitter = timeJitter->value();
  data.lockout = lockoutPeriod->value();
  data.speed_threshold = speedThresh->text().toDouble();

  SendFSDataMessage(DIO_SET_RIPPLE_STIM_PARAMS, (char *) &data, sizeof(RippleStimParameters));
}

/*

void RippleDisruption::showEvent(QShowEvent *e) {
  QWidget::showEvent(e);
  updateRippleData();
  stopRTData(TRUE);
} */

