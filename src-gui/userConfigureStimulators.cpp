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


StimConfigTab::StimConfigTab (QWidget *parent)
  : QWidget(parent)
{
    QString s;

    stimConfigA = new StimConfigureWidget("Stimulator A");
    stimConfigB = new StimConfigureWidget("Stimulator B");

    QLabel *explanation = new QLabel(
        "Configuration parameters for indvidual pulses or " \
        "pulse sequences. <br>These parameters are used for "
        "<b>either</b> <i>Output-only Experiments</i> or "\
        "<i>Real-time Feedback Experiments.</i>");
    //explanation->setAlignment(Qt::AlignHCenter);
    explanation->setWordWrap(true);
    explanation->setIndent(30);

    QVBoxLayout *vMainLayout = new QVBoxLayout;
    vMainLayout->addWidget(explanation);
    vMainLayout->addWidget(stimConfigA);
    vMainLayout->addWidget(stimConfigB);

    setLayout(vMainLayout);

}

StimConfigureWidget::StimConfigureWidget(const QString &title, QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *groupBox = new QGroupBox(title);
  //groupBox->setStyleSheet("border: 2px solid gray; border-radius: 5px;"); 

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
  connect(sequencePeriodSpinBox, SIGNAL(valueChanged(double)), this, SLOT(periodChanged(void)));
  sequenceFrequencySpinBox = new QSpinBox();
  sequenceFrequencySpinBox->setAlignment(Qt::AlignRight);
  sequenceFrequencySpinBox->setSuffix(" Hz");
  connect(sequenceFrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(frequencyChanged(void)));

  QLabel *pulseLengthLabel = new QLabel("Pulse Length");
  pulseLengthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *pulseLengthGraphic = new QLabel;
  pulseLengthGraphic->setPixmap(QPixmap(":/images/pulselength.png"));
  pulseLengthGraphic->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  QLabel *nPulsesLabel = new QLabel("# of Pulses");
  nPulsesLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QGridLayout *parametersLayout = new QGridLayout;

  QLabel *nPulsesGraphic = new QLabel;
  nPulsesGraphic->setPixmap(QPixmap(":/images/npulses.png"));
  nPulsesGraphic->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

  parametersLayout->setColumnStretch(0,1);
  parametersLayout->setColumnStretch(3,1);
  parametersLayout->setColumnStretch(4,1);
  parametersLayout->addWidget(pulseLengthLabel, 0, 1);
  parametersLayout->addWidget(pulseLengthSpinBox, 0, 2);
  parametersLayout->addWidget(pulseLengthGraphic, 0, 3);
  parametersLayout->setRowMinimumHeight(1, 10);

  parametersLayout->addWidget(nPulsesLabel,2,1);
  parametersLayout->addWidget(nPulsesSpinBox, 2, 2);
  parametersLayout->addWidget(nPulsesGraphic, 2, 3);

  QLabel *sequencePeriodLabel = new QLabel("Period");
  sequencePeriodLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QLabel *sequenceFrequencyLabel = new QLabel("Frequency");
  sequenceFrequencyLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  
  /* For multi-pulse sequences, control frequency or period of
   * pulses in stimulation train. */
  QGroupBox *multiPulseGroup = new QGroupBox;
  QGridLayout *multiPulseLayout = new QGridLayout;
  multiPulseLayout->addWidget(sequencePeriodLabel,0,0);
  multiPulseLayout->addWidget(sequencePeriodSpinBox, 0, 1);

  multiPulseLayout->addWidget(sequenceFrequencyLabel,1,0);
  multiPulseLayout->addWidget(sequenceFrequencySpinBox,1,1);
  multiPulseGroup->setLayout(multiPulseLayout);

  parametersLayout->addWidget(multiPulseGroup,3,1,1,2);
  QLabel *pulsePeriodGraphic = new QLabel;
  pulsePeriodGraphic->setPixmap(QPixmap(":/images/period.png"));
  pulsePeriodGraphic->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  parametersLayout->addWidget(pulsePeriodGraphic, 3,3);

  /* For each stimulator, we need one and potentially two (if
   * biphasic) pin masks. */
  primaryStimPinSpinBox = new QSpinBox();
  primaryStimPinSpinBox->setAlignment(Qt::AlignRight);
  primaryStimPinSpinBox->setRange(0,63);
  //primaryStimPinMask->setPrefix("0x");
  QLabel *primaryStimPinLabel = new QLabel("Primary");
  primaryStimPinLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  biphasicCheckBox = new QCheckBox("Biphasic Stimulation");
  connect(biphasicCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ableBiphasicStimulation(int)));

  secondaryStimPinSpinBox = new QSpinBox();
  secondaryStimPinSpinBox->setAlignment(Qt::AlignRight);
  secondaryStimPinSpinBox->setRange(0,63);
  secondaryStimPinSpinBox->setEnabled(false);
  QLabel *secondaryStimPinLabel = new QLabel("Second");
  secondaryStimPinLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

  QGridLayout *stimPinControlsLayout = new QGridLayout;
  stimPinControlsLayout->addWidget(primaryStimPinLabel,0,0);
  stimPinControlsLayout->addWidget(primaryStimPinSpinBox,0,1);
  stimPinControlsLayout->addWidget(biphasicCheckBox,1,1);
  stimPinControlsLayout->addWidget(secondaryStimPinLabel,2,0);
  stimPinControlsLayout->addWidget(secondaryStimPinSpinBox,2,1);
  QGroupBox *portControlsGroup = new QGroupBox("Stimulation Pins");
  portControlsGroup->setLayout(stimPinControlsLayout);
  portControlsGroup->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  portControlsGroup->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Maximum);
  parametersLayout->addWidget(portControlsGroup,0,5,4,1);


  groupBox->setLayout(parametersLayout);

  QGridLayout *layout = new QGridLayout;
  layout->addWidget(groupBox,0,1);
  layout->setColumnStretch(0,1);
  layout->setColumnStretch(1,10);
  layout->setColumnStretch(2,1);

  setLayout(layout);
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
    sequenceFrequencySpinBox->setEnabled(true);
    sequencePeriodSpinBox->setEnabled(true);
  }
  else {
    sequenceFrequencySpinBox->setEnabled(false);
    sequencePeriodSpinBox->setEnabled(false);
  }
    return;
}

void StimConfigureWidget::ableBiphasicStimulation(int state)
{
  if (state) {
    secondaryStimPinSpinBox->setEnabled(true);
  }
  else {
    secondaryStimPinSpinBox->setEnabled(false);
  }
    return;
}

