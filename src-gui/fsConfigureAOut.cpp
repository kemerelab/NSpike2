#include "spikecommon.h"
#include "spikeFSGUI.h"
#include "fsConfigureAOut.h"
#include "laser.h"


AOutConfigTab::AOutConfigTab (QWidget *parent)
  : QWidget(parent)
{
    QString s;


    QGridLayout *layout = new QGridLayout;

    QLabel *explanation = new QLabel(
        "Configuration parameters for Analog Outputs " \
        "<br>These parameters are used for "
        "<b>either</b> <i>Output-only Experiments</i> or "\
        "<i>Real-time Feedback Experiments.</i>");
    //explanation->setAlignment(Qt::AlignHCenter);
    explanation->setWordWrap(true);
    explanation->setIndent(30);
    layout->addWidget(explanation,0,0,1,2);

    
    activeAOut = 0;
    aOut1Button = new QPushButton("AOut 1");
    layout->addWidget(aOut1Button,1,0,Qt::AlignRight | Qt::AlignVCenter);
    aOut1Button->setCheckable(true);
    aOut1Button->setChecked(false);
    aOut1Button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    aOut1Button->setStyleSheet("QPushButton::checked{color: green;}");
    aOut1Button->setStyle("Windows");
    connect(aOut1Button, SIGNAL(clicked(bool)), this, SLOT(selectAOut(void)));

    aOut1Config = new AOutConfigureWidget(this);
    aOut1Config->aOutPulseCmd.aout = 1;
    connect(aOut1Config, SIGNAL(aOutModeChangedSignal(void)), this, 
	    SLOT(aOutModeChanged(void)));

    layout->addWidget(aOut1Config,1,1,Qt::AlignLeft | Qt::AlignVCenter);

    aOut2Button = new QPushButton("AOut 2");
    layout->addWidget(aOut2Button,2,0,Qt::AlignRight | Qt::AlignVCenter);
    aOut2Button->setCheckable(true);
    aOut2Button->setChecked(false);
    aOut2Button->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    aOut2Button->setStyleSheet("QPushButton::checked{color: green;}");
    aOut2Button->setStyle("Windows");
    //aOut2Button->setEnabled(false);
    connect(aOut2Button, SIGNAL(clicked(bool)), this, SLOT(selectAOut(void)));

    aOut2Config = new AOutConfigureWidget(this);
    aOut2Config->aOutPulseCmd.aout = 2;
    connect(aOut2Config, SIGNAL(aOutModeChangedSignal(void)), this, 
	    SLOT(aOutModeChanged(void)));
    layout->addWidget(aOut2Config,2,1,Qt::AlignLeft | Qt::AlignVCenter);

    QFont font;
    font.setPointSize(12);
    font.setBold(true);
    aOut1Button->setFont(font);
    aOut2Button->setFont(font);

    layout->setColumnStretch(1,1);

    setLayout(layout);
    setActiveAOut(0);

}

void AOutConfigTab::selectAOut(void)
{
  aOut1Config->setEnabled(aOut1Button->isChecked());
  aOut2Config->setEnabled(aOut2Button->isChecked());

  emit activeAOutChanged(aOut1Button->isChecked() +
      2*((int)aOut2Button->isChecked()), aOut1Config->aOutPulseCmd.aout_mode, 
      aOut2Config->aOutPulseCmd.aout_mode);

  qDebug("Emitting activeAOutChanged signal");
  return;
}

void AOutConfigTab::setActiveAOut(int which)
{
  qDebug("Received setActiveAOut signal %d",which);

  activeAOut = which;

  aOut1Button->setChecked(which & 0x01);
  aOut2Button->setChecked(which & 0x02);

  selectAOut();

  return;
}

void AOutConfigTab::aOutModeChanged(void)
{
  /* we need to send the signal out to the tabs to tell them there has been a change
   * */
  selectAOut();
  return;
}



AOutConfigureWidget::AOutConfigureWidget(QWidget *parent)
  : QWidget(parent)
{
  QGridLayout *layout = new QGridLayout;
  
  aOutRangeSelectBox = new QComboBox(); 
  aOutRangeSelectBox->addItem("Mambo 594nm Laser");
  aOutRangeSelectBox->addItem("Adjustable Range");
  layout->addWidget(aOutRangeSelectBox, 0, 0, 1, 4, Qt::AlignBottom);
  connect(aOutRangeSelectBox, SIGNAL(currentIndexChanged(int)), this, 
      	  SLOT(setAOutRange(int)));

  layout->addWidget(new QLabel("Min."), 1, 0, 1, 1, Qt::AlignVCenter);
  aOutRangeMinSpinBox = new QDoubleSpinBox();
  aOutRangeMinSpinBox->setRange(0,1);
  aOutRangeMinSpinBox->setDecimals(2);
  aOutRangeMinSpinBox->setSuffix(" Volts");
  layout->addWidget(aOutRangeMinSpinBox, 1, 1, 1, 1, Qt::AlignVCenter);
  connect(aOutRangeMinSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateAOutPulseCmd(void)));

  layout->addWidget(new QLabel("Max"), 1, 2, 1, 1, Qt::AlignVCenter);
  aOutRangeMaxSpinBox = new QDoubleSpinBox();
  aOutRangeMaxSpinBox->setRange(0,1);
  aOutRangeMaxSpinBox->setDecimals(2);
  aOutRangeMaxSpinBox->setSuffix(" Volts");
  layout->addWidget(aOutRangeMaxSpinBox, 1, 3, 1, 1, Qt::AlignVCenter);
  connect(aOutRangeMinSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateAOutPulseCmd(void)));

  layout->addWidget(new QLabel("Trigger Bit"), 2, 0, 1, 1, Qt::AlignTop);
  aOutTriggerBitSpinBox = new QSpinBox();
  aOutTriggerBitSpinBox->setRange(0,63);
  layout->addWidget(aOutTriggerBitSpinBox, 2, 1, 1, 1, Qt::AlignTop);
  connect(aOutTriggerBitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateAOutPulseCmd(void))); 

  aOutModeComboBox = new QComboBox();
  aOutModeComboBox->addItem("No Output");
  aOutModeComboBox->addItem("Continuous");
  aOutModeComboBox->addItem("Pulse");
  //aOutModeComboBox->addItem("Ramp");
  //aOutModeComboBox->addItem("Sine Wave");
  layout->addWidget(aOutModeComboBox, 0, 4, 1, 1, Qt::AlignBottom);
  connect(aOutModeComboBox, SIGNAL(currentIndexChanged(int)), this, 
      	  SLOT(setAOutMode(int)));

  aOutContinuousMode = new AOutContinuousMode(this);
  connect(aOutContinuousMode->contPercentSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateAOutPulseCmd(void)));

  aOutPulseMode = new AOutPulseMode(this);
  connect(aOutPulseMode, SIGNAL(aOutPulseCmdChanged(void)), this, SLOT(updateAOutPulseCmd(void)));

  //aOutRampMode = new AOutRampMode(this);
  //aOutSineMode = new AOutSineMode(this);

  aOutModeStack = new QStackedWidget;
  QLabel *noMode = new QLabel("\tSelect Analog Output Mode");
  aOutModeStack->addWidget(noMode);
  aOutModeStack->addWidget(aOutContinuousMode);
  aOutModeStack->addWidget(aOutPulseMode);
//  aOutModeStack->addWidget(aOutRampMode);
//  aOutModeStack->addWidget(aOutSineMode);
//
  layout->addWidget(aOutModeStack, 0, 5, 3, 1, Qt::AlignVCenter);
  setLayout(layout);


  aOutRangeSelectBox->setCurrentIndex(0);
  setAOutRange(0);
}
	  

void AOutConfigureWidget::setAOutMode(int index)
{
  int mode = 0;
  aOutModeStack->setCurrentIndex(index);

  fprintf(stderr, "index = %d\n", index);
  switch (index) {
    case 0:
      mode = 0;
      break;
    case 1:
      mode = DIO_AO_MODE_CONTINUOUS;
      break;
    case 2:
      mode = DIO_AO_MODE_PULSE;
      break;
    case 3:
      mode = DIO_AO_MODE_RAMP;
      break;
    case 4:
      mode = DIO_AO_MODE_SINE;
      break;
  }
  aOutPulseCmd.aout_mode = mode;
  emit aOutModeChangedSignal();
  fprintf(stderr, "aOutMode = %d, %d\n", mode, DIO_AO_MODE_CONTINUOUS);
}

void AOutConfigureWidget::setAOutRange(int index)
{

  aOutRangeMinSpinBox->setEnabled(false);
  aOutRangeMaxSpinBox->setEnabled(false);

  switch (index) {
    case 0:
      // Mambo 594 nm laser with AOM
      aOutRangeMinSpinBox->setValue(MAMBO_594_MIN_VOLTAGE);
      aOutRangeMaxSpinBox->setValue(MAMBO_594_MAX_VOLTAGE);
      break;
    case 1:
      // full range
      aOutRangeMinSpinBox->setValue(0);
      aOutRangeMaxSpinBox->setValue(1);
      aOutRangeMinSpinBox->setEnabled(true);
      aOutRangeMaxSpinBox->setEnabled(true);
      break;
  }

}


void AOutConfigureWidget::updateAOutPulseCmd(void)
{
  /* go through all the objects and update the pulse command */

  // we assume statemachine 0; we probably want to make this user selectable
  aOutPulseCmd.statemachine = 0;
  aOutPulseCmd.digital_only = false;
  aOutPulseCmd.pin1 = aOutTriggerBitSpinBox->value();
  aOutPulseCmd.minv = aOutRangeMinSpinBox->value();
  aOutPulseCmd.maxv = aOutRangeMaxSpinBox->value();

  aOutPulseCmd.cont_percent = aOutContinuousMode->contPercentSpinBox->value();

  aOutPulseCmd.pulse_percent = aOutPulseMode->pulsePercentSpinBox->value();
  aOutPulseCmd.pulse_width = aOutPulseMode->pulseLengthSpinBox->value()*10; // conver to 0.1 ms ticks
  aOutPulseCmd.inter_pulse_delay = aOutPulseMode->sequencePeriodSpinBox->value()*10 - aOutPulseCmd.pulse_width;
  aOutPulseCmd.n_pulses = aOutPulseMode->nPulsesSpinBox->value();

  /* send the updated pulse command to SPIKE_FS_DATA */
}



AOutContinuousMode::AOutContinuousMode(QWidget *parent) : QWidget(parent)
{

  QGridLayout *layout = new QGridLayout;

  layout->addWidget(new QLabel("Continuous Level: "), 0, 0, 1, 1, Qt::AlignRight);

  contPercentSpinBox = new QSpinBox();
  contPercentSpinBox->setRange(0,100);
  contPercentSpinBox->setSuffix(" Percent");
  contPercentSpinBox->setValue(0);

  layout->addWidget(contPercentSpinBox, 0, 1, 1, 1);
  setLayout(layout);
  qDebug("set up continuous mode");

}

AOutPulseMode::AOutPulseMode(QWidget *parent) : QWidget(parent)
{
  aOutPulseCmd.line = 0;
  aOutPulseCmd.pre_delay = 0;
  aOutPulseCmd.pulse_width = 1; // default value - should set this below?
  aOutPulseCmd.pulse_percent = 0; // default value - should set this below?
  aOutPulseCmd.inter_pulse_delay = 1249;
  aOutPulseCmd.n_pulses = 1;
  aOutPulseCmd.is_biphasic = 0;
  aOutPulseCmd.pin1 = 1;
  aOutPulseCmd.pin2 = 1;
  aOutPulseCmd.n_repeats = 0;
  aOutPulseCmd.inter_frame_delay = 0;

  QGroupBox *groupBox = new QGroupBox();

  pulseLengthSpinBox = new QDoubleSpinBox();
  pulseLengthSpinBox->setSuffix(" ms");
  pulseLengthSpinBox->setAlignment(Qt::AlignRight);
  pulseLengthSpinBox->setDecimals(1);
  pulseLengthSpinBox->setRange(0.1,2000);
  pulseLengthSpinBox->setToolTip("Length in milliseconds of each pulse in pulse sequence.");
  connect(pulseLengthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(pulseChanged(void)));

  pulsePercentSpinBox = new QSpinBox();
  pulsePercentSpinBox->setSuffix(" percent");
  pulsePercentSpinBox->setAlignment(Qt::AlignRight);
  pulsePercentSpinBox->setRange(0,100);
  pulsePercentSpinBox->setToolTip("Percent of maximum for pulse amplitude.");
  connect(pulsePercentSpinBox, SIGNAL(valueChanged(int)), this, SLOT(pulseChanged(void)));


  nPulsesSpinBox = new QSpinBox();
  nPulsesSpinBox->setRange(1,10);
  nPulsesSpinBox->setAlignment(Qt::AlignRight);
  nPulsesSpinBox->setToolTip("Number of pulses in pulse sequence.");
  connect(nPulsesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ablePulseSequence(void)));

  sequencePeriodSpinBox = new QDoubleSpinBox();
  sequencePeriodSpinBox->setAlignment(Qt::AlignRight);
  sequencePeriodSpinBox->setSuffix(" ms");
  sequencePeriodSpinBox->setRange(125,5000);
  sequencePeriodSpinBox->setToolTip("Period of pulses in pulse sequence.");
  connect(sequencePeriodSpinBox, SIGNAL(valueChanged(double)), this, SLOT(periodChanged(void)));
  sequenceFrequencySpinBox = new QSpinBox();
  sequenceFrequencySpinBox->setAlignment(Qt::AlignRight);
  sequenceFrequencySpinBox->setRange(0.2,1000);
  sequenceFrequencySpinBox->setSuffix(" Hz");
  sequenceFrequencySpinBox->setToolTip("Frequency of pulses in pulse sequence.");
  connect(sequenceFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(frequencyChanged(void)));

  QLabel *pulseLengthGraphic = new QLabel;
  pulseLengthGraphic->setPixmap(QPixmap(":/images/pulselength.png"));
  pulseLengthGraphic->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  QGridLayout *parametersLayout = new QGridLayout;

  QLabel *nPulsesGraphic = new QLabel;
  nPulsesGraphic->setPixmap(QPixmap(":/images/npulses.png"));
  nPulsesGraphic->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  parametersLayout->addWidget(new QLabel("Pulse Length"), 0, 0,
      Qt::AlignRight | Qt::AlignVCenter);
  parametersLayout->addWidget(pulseLengthSpinBox, 0, 1);
  parametersLayout->addWidget(pulseLengthGraphic, 0, 2);
  parametersLayout->setRowMinimumHeight(1, 5);

  parametersLayout->addWidget(new QLabel("Pulse amplitude"), 1, 0,
      Qt::AlignRight | Qt::AlignVCenter);
  parametersLayout->addWidget(pulsePercentSpinBox, 1, 1);

  parametersLayout->addWidget(new QLabel("# of Pulses"),2,0,
      Qt::AlignRight | Qt::AlignVCenter);
  parametersLayout->addWidget(nPulsesSpinBox, 2, 1);
  parametersLayout->addWidget(nPulsesGraphic, 2, 2);

  /* For multi-pulse sequences, control frequency or period of
   * pulses in train. */
  multiPulseGroup = new QGroupBox;
  QGridLayout *multiPulseLayout = new QGridLayout;
  multiPulseLayout->addWidget(new QLabel("Period"),0,0,
      Qt::AlignRight | Qt::AlignVCenter);
  multiPulseLayout->addWidget(sequencePeriodSpinBox, 0, 1);

  multiPulseLayout->addWidget(new QLabel("Frequency"),1,0,
      Qt::AlignRight | Qt::AlignVCenter);
  multiPulseLayout->addWidget(sequenceFrequencySpinBox,1,1);

  QLabel *pulsePeriodGraphic = new QLabel;
  pulsePeriodGraphic->setPixmap(QPixmap(":/images/period.png"));
  multiPulseLayout->addWidget(pulsePeriodGraphic,0,2,2,1, Qt::AlignCenter);
  multiPulseGroup->setLayout(multiPulseLayout);
  multiPulseGroup->setEnabled(false);

  parametersLayout->addWidget(multiPulseGroup,3,0,1,3,Qt::AlignRight);

  groupBox->setLayout(parametersLayout);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(groupBox,0,0);
  setLayout(layout);

  // Load defaults
  // Update state (number of pulses->period, biphasic->secondary aOut)
}

void AOutPulseMode::pulseChanged(void)
{
  emit aOutPulseCmdChanged();
  return;
}

void AOutPulseMode::frequencyChanged(void)
{
  sequencePeriodSpinBox->blockSignals(true);
  sequencePeriodSpinBox->setValue(10000/sequenceFrequencySpinBox->value());
  sequencePeriodSpinBox->blockSignals(false);
  emit aOutPulseCmdChanged();
  return;
}

void AOutPulseMode::periodChanged(void)
{
  sequenceFrequencySpinBox->blockSignals(true);
  sequenceFrequencySpinBox->setValue(1000/sequencePeriodSpinBox->value());
  sequenceFrequencySpinBox->blockSignals(false);
  emit aOutPulseCmdChanged();
}

void AOutPulseMode::ablePulseSequence(void)
{
  if (nPulsesSpinBox->value() > 1) {
    multiPulseGroup->setEnabled(true);
  }
  else {
    multiPulseGroup->setEnabled(false);
  }
  emit aOutPulseCmdChanged();
}
