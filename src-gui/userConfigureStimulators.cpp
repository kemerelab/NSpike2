
#include "spikecommon.h"
#include "spikeUserGUI.h"
#include "userConfigureStimulators.h"


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
    stimulatorAButton->setChecked(true);
    stimulatorAButton->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    stimulatorAButton->setStyleSheet("QPushButton::checked{color: green;}");
    layout->addWidget(stimConfigA,1,1,Qt::AlignLeft);

    stimulatorBButton = new QPushButton("B");
    layout->addWidget(stimulatorBButton,2,0,Qt::AlignRight);
    stimulatorBButton->setCheckable(true);
    stimulatorBButton->setChecked(false);
    stimulatorBButton->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
    stimulatorBButton->setStyleSheet("QPushButton::checked{color: green;}");
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

  stimulatorAButton->setChecked(which & 0x01);
  stimulatorBButton->setChecked(which & 0x02);

  stimConfigA->setEnabled(stimulatorAButton->isChecked());
  stimConfigB->setEnabled(stimulatorBButton->isChecked());
  return;
}



StimConfigureWidget::StimConfigureWidget(const QString &title, QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *groupBox = new QGroupBox(title);

  pulseLengthSpinBox = new QDoubleSpinBox();
  pulseLengthSpinBox->setSuffix(" ms");
  pulseLengthSpinBox->setAlignment(Qt::AlignRight);
  pulseLengthSpinBox->setDecimals(1);
  pulseLengthSpinBox->setRange(0.1,5);
  //connect(pulseLengthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(periodChanged(void)));

  nPulsesSpinBox = new QSpinBox();
  nPulsesSpinBox->setRange(1,10);
  nPulsesSpinBox->setAlignment(Qt::AlignRight);
  connect(nPulsesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ablePulseSequence(void)));

  sequencePeriodSpinBox = new QDoubleSpinBox();
  sequencePeriodSpinBox->setAlignment(Qt::AlignRight);
  sequencePeriodSpinBox->setSuffix(" ms");
  sequencePeriodSpinBox->setRange(1,5000);
  connect(sequencePeriodSpinBox, SIGNAL(valueChanged(double)), this, SLOT(periodChanged(void)));
  sequenceFrequencySpinBox = new QSpinBox();
  sequenceFrequencySpinBox->setAlignment(Qt::AlignRight);
  sequenceFrequencySpinBox->setRange(0.2,200);
  sequenceFrequencySpinBox->setSuffix(" Hz");
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
  QLabel *primaryStimPinLabel = new QLabel("Primary");

  biphasicCheckBox = new QCheckBox("Enable Biphasic Stim");
  connect(biphasicCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ableBiphasicStimulation(int)));

  secondaryStimPinSpinBox = new QSpinBox();
  secondaryStimPinSpinBox->setAlignment(Qt::AlignRight);
  secondaryStimPinSpinBox->setRange(0,63);
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
  //updateStimData();
    return;
}

void StimConfigureWidget::periodChanged(void)
{
  sequenceFrequencySpinBox->blockSignals(true);
  sequenceFrequencySpinBox->setValue(1000/sequencePeriodSpinBox->value());
  sequenceFrequencySpinBox->blockSignals(false);
  //updateStimData();
    return;
}

void StimConfigureWidget::ablePulseSequence(void)
{
  if (nPulsesSpinBox->value() > 1) {
    multiPulseGroup->setEnabled(true);
  }
  else {
    multiPulseGroup->setEnabled(false);
  }
    return;
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
    return;
}

/*
void ConfigForm::updateStimPins(void)
{
  int data[2];

  int stimPin1 = PortSpinBox->value();
  int stimPin2 = Port2SpinBox->value();

  if (digioinfo.outputfd) {
    if (BiphasicButton->isOn()) {
      // set biphasic and 2 pins
      data[0] = stimPin1;
      data[1] = stimPin2;
      if ((data[0] / 16) != (data[1] / 16))
QMessageBox::warning(this,"Invalid Stimulation Ports","Stimulation ports for stim 1 and 2 must be the same. Stim Pin 1 port assumed.");
      SendDAQUserMessage(DIO_SET_BIPHASIC_STIM_PINS, (char *) data, 2*sizeof(int));
      fprintf(stderr,"Sent pins\n");
    }
    else {
      // set single activation pin
      data[0] = stimPin1;
      SendDAQUserMessage(DIO_SET_SINGLE_STIM_PIN, (char *) data, sizeof(int));
      fprintf(stderr,"Sent pin\n");
    }

  }
  else {
    QMessageBox::warning(this,"No User Program","No user program is currently running");
  }
}

*/
