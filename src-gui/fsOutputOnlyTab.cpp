

#include "spikecommon.h"
#include "fsOutputOnlyTab.h"
#include "spikeFSGUI.h"

extern DAQ_IO *daq_io_widget; // global

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

  nTrainsSpinBox = new QSpinBox();
  nTrainsSpinBox->setAlignment(Qt::AlignRight);
  nTrainsSpinBox->setRange(1,200);
  nTrainsSpinBox->setToolTip("Number of pulse/pulse sequences to trigger before returning.");
  nTrainsLayout->addWidget(nTrainsSpinBox);
  continuousButton = new QPushButton("Continuous");
  continuousButton->setCheckable(true);
  continuousButton->setStyle("Windows");
  continuousButton->setToolTip("Execute pulse/pulse sequences until Abort button is pressed.");
  connect(continuousButton, SIGNAL(toggled(bool)), this, SLOT(toggleContinuousMode(bool)));

  nTrainsLayout->addWidget(continuousButton);

  layout->addLayout(nTrainsLayout,1,1);

  layout->addWidget(new QLabel("Inter-train Interval"), 3, 0,
      Qt::AlignRight | Qt::AlignVCenter);
  trainIntervalSpinBox = new QSpinBox();
  trainIntervalSpinBox->setAlignment(Qt::AlignRight);
  trainIntervalSpinBox->setSuffix(" ms");
  trainIntervalSpinBox->setRange(500, 60000);
  trainIntervalSpinBox->setToolTip("Time in milliseconds from the onset of one \npulse/pulse sequence to the onset of the next.");
  layout->addWidget(trainIntervalSpinBox, 3, 1);
  layout->addWidget(new QLabel("[pulse onset to pulse onset]"), 3, 2,
      Qt::AlignRight | Qt::AlignVCenter);

  stimSingleButton = new QPushButton("Trigger Single Pulse Sequence");
  layout->addWidget(stimSingleButton,5,0);
  stimSingleButton->setToolTip("Trigger a single stimulation.\n (ignore value set of 'Number of pulse trains'/Continuous).");

  startStimButton = new QPushButton("Start Pulse Sequence");
  layout->addWidget(startStimButton,5,1);
  startStimButton->setToolTip("Start stimulation program.");

  abortStimButton = new QPushButton("Abort Pulse Sequence");
  layout->addWidget(abortStimButton,5,2);
  abortStimButton->setToolTip("End stimulation program.");
  abortStimButton->setEnabled(false);

  /* Signals from the trigger, start, and abort buttons are
   * connected to slots in  spikeFSGUI */

  trainCounter = 0;
  nTrains = 0;

  setLayout(layout);

}

void StimOutputOnlyTab::startStimulation(int count)
{
  abortStimButton->setEnabled(true);
  stimSingleButton->setEnabled(false);
  startStimButton->setEnabled(false);

  nTrainsSpinBox->setEnabled(false);
  continuousButton->setEnabled(false);
  trainIntervalSpinBox->setEnabled(false);

  nTrains = nTrainsSpinBox->value();
  qDebug("\nnTrains is %d\n\n",nTrains);
  if (count == -1)
    nTrainsSpinBox->setSuffix(" (continuous)");
  if (count >= 0)
    nTrainsSpinBox->setSuffix(QString(" of %1").arg(count+1));
  trainCounter = 0;
  nTrainsSpinBox->setValue(trainCounter);
}

void StimOutputOnlyTab::endStimulation(int flag)
{
  continuousButton->setEnabled(true);
  if (!continuousButton->isChecked()) {
    nTrainsSpinBox->setEnabled(true);
  }
  trainIntervalSpinBox->setEnabled(true);

  nTrainsSpinBox->setValue(nTrains);
  nTrainsSpinBox->setSuffix("");

  stimSingleButton->setEnabled(true);
  startStimButton->setEnabled(true);
  abortStimButton->setEnabled(false);
}

void StimOutputOnlyTab::stepStimulation(int count)
{
  trainCounter++;
  nTrainsSpinBox->setValue(trainCounter);
  qDebug("Got step: %d\n",count);
}

void StimOutputOnlyTab::writeStateToFile(QFile *stateFile)
{
}

void StimOutputOnlyTab::readStateFromFile(QFile *stateFile)
{
}


