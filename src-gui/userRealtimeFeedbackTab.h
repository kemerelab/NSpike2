#ifndef __SPIKE_USER_REALTIME_FEEDBACK_H__
#define __SPIKE_USER_REALTIME_FEEDBACK_H__

#include <QtGui>
#include "spike_stimcontrol_defines.h"
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
  QLineEdit *ripCoeff1;
  QLineEdit *ripCoeff2;
  QLineEdit *ripThresh;
  QSpinBox *lockoutPeriod;
  QSpinBox *timeDelay;
  QSpinBox *timeJitter;
  QLineEdit *speedThresh;

private slots:
  void updateRippleData(void);
};

class RealtimeFeedbackTab : public QWidget
{
  Q_OBJECT

public:
  RealtimeFeedbackTab (QWidget *parent);

  QComboBox *stimulatorSelectComboBox;
  QStackedWidget *algorithmAlternativesStack;

  QLabel *status;
  QGroupBox *statusGroupBox;

  QLabel *realtimeDataStatus;
  QPushButton *startFeedbackButton;
  QPushButton *stopFeedbackButton;

public slots:
  void setFeedbackAlgorithm(int index);

private slots:
  void checkRealtimeStatus(void); 
  void updateRealtimeStatus(RippleStatusMsg status); 

protected:
};



#endif
