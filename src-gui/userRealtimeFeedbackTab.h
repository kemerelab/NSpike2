#ifndef __SPIKE_USER_REALTIME_FEEDBACK_H__
#define __SPIKE_USER_REALTIME_FEEDBACK_H__

#include <QtGui>

#define DEFAULT_TMP_PULSE_COMMANDS_FILE "/tmp/tmp_pulse_commands"

class LatencyTest : public QWidget
{
  Q_OBJECT

public:
  LatencyTest (QWidget *parent);
};

class ThetaPhaseStim : public QWidget
{
  Q_OBJECT

public:
  ThetaPhaseStim (QWidget *parent);
};

class RippleDisruption : public QWidget
{
  Q_OBJECT

public:
  RippleDisruption (QWidget *parent);
};

class RealtimeFeedbackTab : public QWidget
{
  Q_OBJECT

public:
  RealtimeFeedbackTab (QWidget *parent);

  QComboBox *stimulatorSelectComboBox;
  QStackedWidget *algorithmAlternativesStack;

  QLabel *status;

  QPushButton *realtimeEnableButton;
  QPushButton *startFeedbackButton;
  QPushButton *stopFeedbackButton;
  //LatencyTest *latencyTest;
  //ThetaPhaseStim *thetaPhaseStim;
  //RippleDisruption *rippleDisruption;

public slots:

private:

protected:
};



#endif
