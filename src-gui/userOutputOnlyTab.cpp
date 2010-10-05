

#include "spikecommon.h"
#include "userOutputOnlyTab.h"


StimOutputOnlyTab::StimOutputOnlyTab (QWidget *parent)
  : QWidget(parent)
{
  QGridLayout *layout = new QGridLayout;

  stimulatorSelectComboBox = new QComboBox;
  stimulatorSelectComboBox->addItem("None");
  stimulatorSelectComboBox->addItem("A");
  stimulatorSelectComboBox->addItem("B");
  stimulatorSelectComboBox->addItem("Both (Alternating)");

  // Signals/Slots for stimulatorSelectComboBox are connected in
  //  main GUI code to StimConfigTab.
  layout->addWidget(new QLabel("Active Stimulator"),0,0, 
      Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(stimulatorSelectComboBox,0,1);

  layout->addWidget(new QLabel("Number of pulse trains"), 1, 0,
      1,1, Qt::AlignRight | Qt::AlignTop);

  QVBoxLayout *nTrainsLayout = new QVBoxLayout;

  QSpinBox *nTrainsSpinBox = new QSpinBox();
  nTrainsSpinBox->setAlignment(Qt::AlignRight);
  nTrainsSpinBox->setRange(1,200);
  nTrainsLayout->addWidget(nTrainsSpinBox);
  QPushButton *continuousButton = new QPushButton("Continuous");
  nTrainsLayout->addWidget(continuousButton);

  layout->addLayout(nTrainsLayout,1,1);

  layout->addWidget(new QLabel("Inter-train Interval"), 3, 0,
      Qt::AlignRight | Qt::AlignVCenter);
  QSpinBox *trainIntervalSpinBox = new QSpinBox();
  trainIntervalSpinBox->setAlignment(Qt::AlignRight);
  trainIntervalSpinBox->setSuffix(" ms");
  trainIntervalSpinBox->setRange(500, 50000);
  layout->addWidget(trainIntervalSpinBox, 3, 1);

  QPushButton *stimSingleButton = new QPushButton("Trigger Single Pulse Sequence");
  layout->addWidget(stimSingleButton,5,0);

  QPushButton *startStimButton = new QPushButton("Start Pulse Sequence");
  layout->addWidget(startStimButton,5,1);

  QPushButton *abortStimButton = new QPushButton("Abort Pulse Sequence");
  layout->addWidget(abortStimButton,5,2);

  setLayout(layout);

}
