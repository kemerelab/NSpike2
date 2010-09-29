
#include "spikecommon.h"
#include "userConfigureStimulators.h"


StimConfigTab::StimConfigTab (QWidget *parent)
  : QWidget(parent)
{
  QGridLayout *layout = new QGridLayout;
  layout->addWidget(new QLabel("Number of pulse trains"), 0, 0,
      Qt::AlignRight | Qt::AlignVCenter);

  QSpinBox *nTrainsSpinBox = new QSpinBox();
  nTrainsSpinBox->setAlignment(Qt::AlignRight);
  nTrainsSpinBox->setDecimals(1);
  nTrainsSpinBox->setRange(1,200);
  layout->addWidget(nTrainsSpinBox, 0, 1);

  QPushButton *continuousButton = new QPushButton("Contiuous");
  layout->addWidget(continuousButton,0,2);

  layout->addWidget(new QLabel("Inter-train Interval"), 1, 0,
      Qt::AlignRight | Qt::AlignVCenter);
  QSpinBox *trainIntervalSpinBox = new QSpinBox();
  trainIntervalSpinBox->setAlignment(Qt::AlignRight);
  trainIntervalSpinBox->setSuffix(" ms");
  trainIntervalSpinBox->setRange(500, 50000);
  layout->addWidget(trainIntervalSpinBox, 1, 1);

  QPushButton *stimSingleButton = new QPushButton("Trigger Single Pulse Sequence");
  layout->addWidget(stimSingleButton,2,0);

  QPushButton *startStimButton = new QPushButton("Start Pulse Sequence");
  layout->addWidget(stimSingleButton,2,1);

  QPushButton *abortStimButton = new QPushButton("Abort");
  layout->addWidget(abortSingleButton,2,2);

  setLayout(layout);

}

