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
  groupBox = new QGroupBox(title);
  //groupBox->setStyleSheet("border: 2px solid gray; border-radius: 5px;"); 

  pulseLengthSpinBox = new QDoubleSpinBox();
  nPulsesSpinBox = new QDoubleSpinBox();
  sequenceFrequencySpinBox = new QDoubleSpinBox();
  sequencePeriodSpinBox = new QDoubleSpinBox();

  QGridLayout *parametersLayout = new QGridLayout;
  parametersLayout->addWidget(new QLabel("Pulse Length"), 0, 0);
  parametersLayout->addWidget(pulseLengthSpinBox, 0, 1);
  QLabel *pulseLengthGraphic = new QLabel;
  pulseLengthGraphic->setPixmap(QPixmap(":/images/pulselength.png"));
  pulseLengthGraphic->setAlignment(Qt::AlignHCenter);
  parametersLayout->addWidget(pulseLengthGraphic, 0, 2);

  parametersLayout->addWidget(new QLabel("# of Pulses"),1,0);
  parametersLayout->addWidget(nPulsesSpinBox, 1, 1);
  QLabel *nPulsesGraphic = new QLabel;
  nPulsesGraphic->setPixmap(QPixmap(":/images/npulses.png"));
  nPulsesGraphic->setAlignment(Qt::AlignHCenter);
  parametersLayout->addWidget(nPulsesGraphic, 1, 2);


  multiPulseGroup = new QGroupBox;
  QGridLayout *multiPulseLayout = new QGridLayout;
  multiPulseLayout->addWidget(new QLabel("Period"),0,0);
  multiPulseLayout->addWidget(sequencePeriodSpinBox, 0, 1);

  multiPulseLayout->addWidget(new QLabel("Frequency"),1,0);
  multiPulseLayout->addWidget(sequenceFrequencySpinBox,1,1);
  multiPulseGroup->setLayout(multiPulseLayout);

  parametersLayout->addWidget(multiPulseGroup,2,0,1,2);
  QLabel *pulsePeriodGraphic = new QLabel;
  pulsePeriodGraphic->setPixmap(QPixmap(":/images/period.png"));
  pulsePeriodGraphic->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  parametersLayout->addWidget(pulsePeriodGraphic, 2,2);

  groupBox->setLayout(parametersLayout);

  QHBoxLayout *layout = new QHBoxLayout;
  layout->addWidget(groupBox);

  setLayout(layout);
}

