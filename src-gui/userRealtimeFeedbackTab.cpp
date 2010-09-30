


#include "spikecommon.h"
#include "userRealtimeFeedbackTab.h"

RealtimeFeedbackTab::RealtimeFeedbackTab (QWidget *parent)
  : QWidget(parent)
{
  QGridLayout *layout = new QGridLayout;

  stimulatorSelectComboBox = new QComboBox;
  stimulatorSelectComboBox->addItem("None");
  stimulatorSelectComboBox->addItem("A");
  stimulatorSelectComboBox->addItem("B");
  stimulatorSelectComboBox->addItem("Both");
  // Signals/Slots for stimulatorSelectComboBox are connected in
  //  main GUI code to StimConfigTab.
  layout->addWidget(new QLabel("Active Stimulator"),0,0, 
      Qt::AlignRight | Qt::AlignVCenter);
  layout->addWidget(stimulatorSelectComboBox,0,1);

  QComboBox *feedbackAlgorithmComboBox = new QComboBox;
  feedbackAlgorithmComboBox->addItem("None");
  feedbackAlgorithmComboBox->addItem("Latency Test");
  feedbackAlgorithmComboBox->addItem("Theta Phase");
  feedbackAlgorithmComboBox->addItem("Ripple Disruption");
  layout->addWidget(new QLabel("Realtime Feedback Program"),1,0, Qt::AlignRight);
  layout->addWidget(feedbackAlgorithmComboBox,1,1);

  QPushButton *realtimeEnableButton = new QPushButton("Enable Realtime");
  layout->addWidget(realtimeEnableButton,2,0,1,2,Qt::AlignCenter);

  status = new QLabel("waiting...");
  QGroupBox *statusGroupBox = new QGroupBox("Status");
  statusGroupBox->setStyleSheet("QGroupBox{border: 1px solid black;border-radius: 5px; margin-top: 1ex;}" \
                "QGroupBox::title{subcontrol-origin: margin; subcontrol-position:top center; padding: 0 3px;}");
  QVBoxLayout *statusBoxLayout = new QVBoxLayout;
  statusBoxLayout->addWidget(status,Qt::AlignCenter);
  layout->addWidget(statusGroupBox,3,0,2,2);

  QPushButton *startFeedbackButton = new QPushButton("Start Feedback");
  layout->addWidget(startFeedbackButton,5,0, Qt::AlignCenter);

  QPushButton *stopFeedbackButton = new QPushButton("Stop Feedback");
  layout->addWidget(stopFeedbackButton,5,1, Qt::AlignCenter);

  QStackedWidget *algorithmAlternativesStack = new QStackedWidget;
  QLabel *noAlgorithm = new QLabel("Select a Feedback Control Algorithm");
  algorithmAlternativesStack->addWidget(noAlgorithm);

  algorithmAlternativesStack->addWidget(new LatencyTest(this));
  algorithmAlternativesStack->addWidget(new ThetaPhaseStim(this));
  algorithmAlternativesStack->addWidget(new RippleDisruption(this));

  layout->addWidget(algorithmAlternativesStack,0,2,4,5);

  setLayout(layout);

  connect(feedbackAlgorithmComboBox, SIGNAL(currentIndexChanged(int)), 
      algorithmAlternativesStack, SLOT(setCurrentIndex(int)));
}

LatencyTest::LatencyTest(QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *algorithmParametersGroupBox = new QGroupBox("Latency Test Parameters");
  QFormLayout *parametersLayout = new QFormLayout;
  
  algorithmParametersGroupBox->setLayout(parametersLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(algorithmParametersGroupBox,Qt::AlignCenter);
  setLayout(layout);
}

ThetaPhaseStim::ThetaPhaseStim(QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *algorithmParametersGroupBox = new QGroupBox("Theta Phase Stimulation Parameters");
  QFormLayout *parametersLayout = new QFormLayout;
  
  algorithmParametersGroupBox->setLayout(parametersLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(algorithmParametersGroupBox,Qt::AlignCenter);
  setLayout(layout);
}


RippleDisruption::RippleDisruption(QWidget *parent)
  : QWidget(parent)
{
  QGroupBox *algorithmParametersGroupBox = new QGroupBox("Ripple Disruption Parameters");
  QFormLayout *parametersLayout = new QFormLayout;
  
  algorithmParametersGroupBox->setLayout(parametersLayout);

  QVBoxLayout *layout = new QVBoxLayout;
  layout->addWidget(algorithmParametersGroupBox,Qt::AlignCenter);
  setLayout(layout);
}
