
#include "spikecommon.h"
#include "userRealtimeFeedbackTab.h"
#include "spikeUserGUI.h"

extern DAQ_IO *daq_io_widget; // global

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

  QPushButton *realtimeEnableButton = new QPushButton("Enable Realtime");
  layout->addWidget(realtimeEnableButton,2,0,1,2,Qt::AlignCenter);
  connect(realtimeEnableButton, SIGNAL(clicked()), daq_io_widget, SLOT(enableRealtimeStim()));

  status = new QLabel("waiting...");
  QGroupBox *statusGroupBox = new QGroupBox("Status");
  statusGroupBox->setStyleSheet("QGroupBox{border: 1px solid black;border-radius: 5px; margin-top: 1ex;}" \
                "QGroupBox::title{subcontrol-origin: margin; subcontrol-position:top center; padding: 0 3px;}");
  QVBoxLayout *statusBoxLayout = new QVBoxLayout;
  statusBoxLayout->addWidget(status,Qt::AlignCenter);
  layout->addWidget(statusGroupBox,3,0,2,2);

  QPushButton *startFeedbackButton = new QPushButton("Start Feedback");
  layout->addWidget(startFeedbackButton,5,0, Qt::AlignCenter);
  connect(realtimeEnableButton, SIGNAL(clicked()), daq_io_widget, SLOT(startRealtimeStim()));

  QPushButton *stopFeedbackButton = new QPushButton("Stop Feedback");
  layout->addWidget(stopFeedbackButton,5,1, Qt::AlignCenter);
  connect(realtimeEnableButton, SIGNAL(clicked()), daq_io_widget, SLOT(stopRealtimeStim()));

  QStackedWidget *algorithmAlternativesStack = new QStackedWidget;
  QLabel *noAlgorithm = new QLabel("Select a Feedback Control Algorithm");
  algorithmAlternativesStack->addWidget(noAlgorithm);

  algorithmAlternativesStack->addWidget(new LatencyTest(this));
  algorithmAlternativesStack->addWidget(new ThetaPhaseStim(this));
  algorithmAlternativesStack->addWidget(new RippleDisruption(this));

  layout->addWidget(algorithmAlternativesStack,0,2,4,5);

  setLayout(layout);

  connect(feedbackAlgorithmComboBox, SIGNAL(currentIndexChanged(int)), 
      algorithmAlternativesStack, SLOT(setCurrentIndex(int)));
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

  QComboBox *StimChan = new QComboBox( false, this, "Channel Combo Box" );
  StimChan->insertStringList(*(daq_io_widget->ChannelStrings));
  parametersLayout->addRow("Tetrode / channel", StimChan);

  QLineEdit *ripCoeff1 = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_COEFF1));
  ripCoeff1->setValidator(new QDoubleValidator(0.0,2.0,3,this));
  ripCoeff1->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Rip. Coeff 1", ripCoeff1);

  QLineEdit *ripCoeff2 = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_COEFF2));
  ripCoeff2->setValidator(new QDoubleValidator(0.0,2.0,3,this));
  ripCoeff2->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Rip. Coeff 2", ripCoeff2);

  QLineEdit *ripThresh = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_THRESHOLD));
  ripThresh->setValidator(new QDoubleValidator(0.0,20.0,2,this));
  ripThresh->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Ripple Threshold (sd)", ripThresh);

  QSpinBox *lockoutPeriod = new QSpinBox;
  lockoutPeriod->setRange(0,20000);
  lockoutPeriod->setValue(DIO_RT_DEFAULT_RIPPLE_LOCKOUT);
  lockoutPeriod->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Lockout period (usec)", lockoutPeriod);

  QSpinBox *timeDelay = new QSpinBox;
  timeDelay->setRange(0,10000);
  timeDelay->setValue(DIO_RT_DEFAULT_RIPPLE_TIME_DELAY);
  timeDelay->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Time delay (msec)", timeDelay);

  QSpinBox *timeJitter = new QSpinBox;
  timeJitter->setRange(0,10000);
  timeJitter->setValue(DIO_RT_DEFAULT_RIPPLE_JITTER);
  timeJitter->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Jitter (msec)", timeJitter);

  QLineEdit *speedThresh = new QLineEdit(QString::number(DIO_RT_DEFAULT_RIPPLE_SPEED_THRESH), this);
  speedThresh->setValidator(new QDoubleValidator(0.0,100.0,2,this));
  speedThresh->setAlignment(Qt::AlignRight);
  parametersLayout->addRow("Maximum speed (cm/s)", speedThresh);

  // think about using a qsignalmapper 
  // file:///usr/share/qt4/doc/html/qsignalmapper.html

  // connect(StimChan, SIGNAL(activated( int )), daq_io_widget, SLOT(updateChan(int)));
  // connect(daq_io_widget, SIGNAL(updateChanDisplay(int)), this, SLOT(changeStimChanDisplay(int)));

  // connect(StimChan, SIGNAL(activated(int)), this, SLOT(updateRippleData(void)));
  //connect(pulse_len, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  //connect(ripCoeff2, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  //connect(ripCoeff1, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  //connect(ripThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));
  //connect(lockoutPeriod, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  //connect(timeDelay, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  //connect(timeJitter, SIGNAL(valueChanged(int)), this, SLOT(updateRippleData(void)));
  //connect(speedThresh, SIGNAL(textChanged(const QString &)), this, SLOT(updateRippleData(void)));

  //connect(daq_io_widget, SIGNAL(rippleStatusUpdate(RippleStatusMsg)), this, SLOT(updateRippleStatus(RippleStatusMsg)));
  //connect(enableButton, SIGNAL(toggled(bool)), this, SLOT(enableRipStim(bool)));
  //connect(startButton, SIGNAL(toggled(bool)), this, SLOT(startRTData(bool)));
  //connect(stopButton, SIGNAL(toggled(bool)), this, SLOT(stopRTData(bool)));

  
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
