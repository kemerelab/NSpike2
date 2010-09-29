

#include "spikecommon.h"
#include "userOutputOnlyTab.h"


StimOutputOnlyTab::StimOutputOnlyTab (QWidget *parent)
  : QWidget(parent)
{
  QGridLayout *layout = new QGridLayout;


  QComboBox *stimulatorSelectComboBox = new QComboBox;
  stimulatorSelectComboBox->addItem("Stimulator A");
  stimulatorSelectComboBox->addItem("Stimulator B");
  stimulatorSelectComboBox->addItem("Alternate Stimulators");
  //stimulatorSelectComboBox->setSizePolicy(QSizePolicy::Maximum,
      //QSizePolicy::Preferred);
  layout->addWidget(stimulatorSelectComboBox,0,0,1,3);

  layout->addWidget(new QLabel("Number of pulse trains"), 1, 0,
      Qt::AlignRight | Qt::AlignVCenter);

  QSpinBox *nTrainsSpinBox = new QSpinBox();
  nTrainsSpinBox->setAlignment(Qt::AlignRight);
  nTrainsSpinBox->setRange(1,200);
  layout->addWidget(nTrainsSpinBox, 1, 1);

  QPushButton *continuousButton = new QPushButton("Contiuous");
  layout->addWidget(continuousButton,1,2);

  layout->addWidget(new QLabel("Inter-train Interval"), 2, 0,
      Qt::AlignRight | Qt::AlignVCenter);
  QSpinBox *trainIntervalSpinBox = new QSpinBox();
  trainIntervalSpinBox->setAlignment(Qt::AlignRight);
  trainIntervalSpinBox->setSuffix(" ms");
  trainIntervalSpinBox->setRange(500, 50000);
  layout->addWidget(trainIntervalSpinBox, 2, 1);

  QPushButton *stimSingleButton = new QPushButton("Trigger Single Pulse Sequence");
  layout->addWidget(stimSingleButton,4,0);

  QPushButton *startStimButton = new QPushButton("Start Pulse Sequence");
  layout->addWidget(startStimButton,4,1);

  QPushButton *abortStimButton = new QPushButton("Abort Pulse Sequence");
  layout->addWidget(abortStimButton,4,2);

  setLayout(layout);

}

