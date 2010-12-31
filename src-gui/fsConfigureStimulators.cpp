
#include "spikecommon.h"
#include "spikeFSGUI.h"
#include "fsConfigureStimulators.h"


StimConfigTab::StimConfigTab (QWidget *parent)
  : QWidget(parent)
{
    QString s;

    stimConfigA = new StimConfigureWidget("");
    stimConfigB = new StimConfigureWidget("");

    QLabel *explanation = new QLabel(
        "Configuration parameters for indvidual pulses or " \
        "pulse sequences. <br>These parameters are used for "
        "<b>either</b> <i>Output-only Experiments</i> or "\
        "<i>Real-time Feedback Experiments.</i>");
    //explanation->setAlignment(Qt::AlignHCenter);
    explanation->setWordWrap(true);
    explanation->setIndent(30);

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(explanation,0,0,1,2);
    
    stimulatorAButton = new QPushButton("A");
    layout->addWidget(stimulatorAButton,1,0,Qt::AlignRight);
    stimulatorAButton->setCheckable(true);
    stimulatorAButton->setChecked(false);
    stimulatorAButton->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    stimulatorAButton->setStyleSheet("QPushButton::checked{color: green;}");
    stimulatorAButton->setStyle("Windows");
    layout->addWidget(stimConfigA,1,1,Qt::AlignLeft);
    activeStimulator = 0;

    stimulatorBButton = new QPushButton("B");
    layout->addWidget(stimulatorBButton,2,0,Qt::AlignRight);
    stimulatorBButton->setCheckable(true);
    stimulatorBButton->setChecked(false);
    stimulatorBButton->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    stimulatorBButton->setStyleSheet("QPushButton::checked{color: green;}");
    stimulatorBButton->setStyle("Windows");
    layout->addWidget(stimConfigB,2,1,Qt::AlignLeft);
    stimConfigB->setEnabled(false);

    connect(stimulatorAButton, SIGNAL(clicked(bool)), this, SLOT(selectStimulator(void)));
    connect(stimulatorBButton, SIGNAL(clicked(bool)), this, SLOT(selectStimulator(void)));

    QFont font;
    font.setPointSize(32);
    font.setBold(true);
    stimulatorAButton->setFont(font);
    stimulatorBButton->setFont(font);

    layout->setColumnStretch(1,1);

    setLayout(layout);

}

void StimConfigTab::selectStimulator(void)
{
  stimConfigA->setEnabled(stimulatorAButton->isChecked());
  stimConfigB->setEnabled(stimulatorBButton->isChecked());

  emit activeStimulatorChanged(stimulatorAButton->isChecked() +
      2*((int)stimulatorBButton->isChecked()));

  qDebug("Emitting activeStimulatorChanged signal");
  return;
}

void StimConfigTab::setActiveStimulator(int which)
{
  qDebug("Received setActiveStimulator signal %d",which);

  activeStimulator = which;

  stimulatorAButton->setChecked(which & 0x01);
  stimulatorBButton->setChecked(which & 0x02);

  stimConfigA->setEnabled(stimulatorAButton->isChecked());
  stimConfigB->setEnabled(stimulatorBButton->isChecked());
  return;
}



StimConfigureWidget::StimConfigureWidget(const QString &title, QWidget *parent)
  : QWidget(parent)
{
  stimPulseCmd.digital_only = 1;
  stimPulseCmd.line = 0;
  stimPulseCmd.pre_delay = 0;
  stimPulseCmd.pulse_width = 1; // default value - should set this below?
  stimPulseCmd.inter_pulse_delay = 1249;
  stimPulseCmd.n_pulses = 1;
  stimPulseCmd.is_biphasic = 0;
  stimPulseCmd.pin1 = 1;
  stimPulseCmd.pin2 = 1;
  stimPulseCmd.n_repeats = 0;
  stimPulseCmd.inter_frame_delay = 0;

  QGroupBox *groupBox = new QGroupBox(title);

  pulseLengthSpinBox = new QDoubleSpinBox();
  pulseLengthSpinBox->setSuffix(" ms");
  pulseLengthSpinBox->setAlignment(Qt::AlignRight);
  pulseLengthSpinBox->setDecimals(1);
  pulseLengthSpinBox->setRange(0.1,500);
  pulseLengthSpinBox->setToolTip("Length in milliseconds of each pulse in pulse sequence.");
  connect(pulseLengthSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateStimParameters(void)));

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

  parametersLayout->addWidget(new QLabel("# of Pulses"),2,0,
      Qt::AlignRight | Qt::AlignVCenter);
  parametersLayout->addWidget(nPulsesSpinBox, 2, 1);
  parametersLayout->addWidget(nPulsesGraphic, 2, 2);

  /* For multi-pulse sequences, control frequency or period of
   * pulses in stimulation train. */
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

  /* For each stimulator, we need one and potentially two (if biphasic) pin masks. */
  primaryStimPinSpinBox = new QSpinBox();
  primaryStimPinSpinBox->setAlignment(Qt::AlignRight);
  primaryStimPinSpinBox->setRange(0,63);
  primaryStimPinSpinBox->setToolTip("Output pin (range 0 - 63) to stimulate.\nFor biphasic triggering, this is the first pin triggered.");
  connect(primaryStimPinSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimParameters(void)));
  QLabel *primaryStimPinLabel = new QLabel("Primary");

  biphasicCheckBox = new QCheckBox("Enable Biphasic Stim");
  connect(biphasicCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ableBiphasicStimulation(int)));

  secondaryStimPinSpinBox = new QSpinBox();
  secondaryStimPinSpinBox->setAlignment(Qt::AlignRight);
  secondaryStimPinSpinBox->setRange(0,63);
  secondaryStimPinSpinBox->setToolTip("For biphasic triggering, this is the second pin triggered.\nThe value must lie in the same bank of 16 as the primary pin.");
  connect(secondaryStimPinSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimParameters(void)));
  secondaryStimPinLabel = new QLabel("Secondary");

  QGridLayout *stimPinControlsLayout = new QGridLayout;
  stimPinControlsLayout->addWidget(primaryStimPinLabel,0,0,
      Qt::AlignRight | Qt::AlignVCenter);
  stimPinControlsLayout->addWidget(primaryStimPinSpinBox,0,1);
  stimPinControlsLayout->addWidget(biphasicCheckBox,1,0,1,2);
  stimPinControlsLayout->addWidget(secondaryStimPinLabel,2,0,
      Qt::AlignRight | Qt::AlignVCenter);
  stimPinControlsLayout->addWidget(secondaryStimPinSpinBox,2,1);
  ableBiphasicStimulation(false);

  QGroupBox *stimPinControlsGroup = new QGroupBox("Stimulation Pins");
  stimPinControlsGroup->setLayout(stimPinControlsLayout);
  stimPinControlsGroup->setStyleSheet("QGroupBox::enabled{border: 2px solid navy;border-radius: 5px; margin-top: 1ex;}" \
                "QGroupBox::title{subcontrol-origin: margin; subcontrol-position:top center; padding: 0 3px;}");
  parametersLayout->addWidget(stimPinControlsGroup,0,3,4,1,
      Qt::AlignHCenter | Qt::AlignTop);

  groupBox->setLayout(parametersLayout);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(groupBox,0,0);
  setLayout(layout);

  // Load defaults
  // Update state (number of pulses->period, biphasic->secondary stim)
}


void StimConfigureWidget::frequencyChanged(void)
{
  sequencePeriodSpinBox->blockSignals(true);
  sequencePeriodSpinBox->setValue(10000/sequenceFrequencySpinBox->value());
  sequencePeriodSpinBox->blockSignals(false);
  updateStimParameters();
    return;
}

void StimConfigureWidget::periodChanged(void)
{
  sequenceFrequencySpinBox->blockSignals(true);
  sequenceFrequencySpinBox->setValue(1000/sequencePeriodSpinBox->value());
  sequenceFrequencySpinBox->blockSignals(false);
  updateStimParameters();
}

void StimConfigureWidget::ablePulseSequence(void)
{
  if (nPulsesSpinBox->value() > 1) {
    multiPulseGroup->setEnabled(true);
  }
  else {
    multiPulseGroup->setEnabled(false);
  }
  updateStimParameters();
}

void StimConfigureWidget::ableBiphasicStimulation(int state)
{
  if (state) {
    secondaryStimPinSpinBox->setEnabled(true);
    secondaryStimPinLabel->setEnabled(true);
  }
  else {
    secondaryStimPinSpinBox->setEnabled(false);
    secondaryStimPinLabel->setEnabled(false);
  }
  updateStimParameters();
}

void StimConfigureWidget::updateStimParameters(void)
{
  stimPulseCmd.pulse_width = pulseLengthSpinBox->value()*10; // conver to 0.1 ms ticks
  stimPulseCmd.inter_pulse_delay = sequencePeriodSpinBox->value()*10 - stimPulseCmd.pulse_width;
  stimPulseCmd.n_pulses = nPulsesSpinBox->value();

  stimPulseCmd.is_biphasic = biphasicCheckBox->isChecked();
  stimPulseCmd.pin1 = primaryStimPinSpinBox->value();
  stimPulseCmd.pin2 = secondaryStimPinSpinBox->value();

  if (stimPulseCmd.pin2 / 16 != stimPulseCmd.pin1 / 16) 
    secondaryStimPinSpinBox->setStyleSheet("color: red");
  else
    secondaryStimPinSpinBox->setStyleSheet("color: black");

  qDebug("Updating stim parameters.");
}
