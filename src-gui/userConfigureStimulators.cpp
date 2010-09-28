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

    QVBoxLayout *vMainLayout = new QVBoxLayout();

    vMainLayout->addWidget(createStimAGroup());
    vMainLayout->addWidget(createStimBGroup());

    setLayout(vMainLayout);

}

QGroupBox *StimConfigTab::createStimAGroup()
{
  QGroupBox *groupBox = new QGroupBox("Stimulator A");
  //groupBox->setStyleSheet("border: 2px solid gray; border-radius: 5px;"); 

  QDoubleSpinBox *pulseLengthSpinBox = new QDoubleSpinBox();
  QDoubleSpinBox *nPulsesSpinBox = new QDoubleSpinBox();
  QDoubleSpinBox *sequenceFrequencySpinBox = new QDoubleSpinBox();
  QDoubleSpinBox *sequencePeriodSpinBox = new QDoubleSpinBox();

  QFormLayout *ParametersLayout = new QFormLayout();
  ParametersLayout->addRow("Pulse &Length",pulseLengthSpinBox);
  ParametersLayout->addRow("# of &Pulses",nPulsesSpinBox);
  ParametersLayout->addRow("Frequency",sequenceFrequencySpinBox);
  ParametersLayout->addRow("Period",sequencePeriodSpinBox);

  groupBox->setLayout(ParametersLayout);
  groupBox->setPalette(QPalette(Qt::darkBlue));
  QPalette p = groupBox->palette()
  p.setColor(grpControls->backgroundRole(),
      QColor(0,255,255));

  return groupBox;
}

QGroupBox *StimConfigTab::createStimBGroup()
{
  QGroupBox *groupBox = new QGroupBox("Stimulator B");

  return groupBox;
}

