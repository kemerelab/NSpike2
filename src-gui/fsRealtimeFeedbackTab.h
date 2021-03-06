#ifndef __SPIKE_FS_REALTIME_FEEDBACK_H__
#define __SPIKE_FS_REALTIME_FEEDBACK_H__

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
  QSpinBox *sampDivisor;
  QLineEdit *ripCoeff1;
  QLineEdit *ripCoeff2;
  QLineEdit *ripThresh;
  QSpinBox *nAboveThreshold;
  QSpinBox *lockoutPeriod;
  QSpinBox *timeDelay;
  QSpinBox *timeJitter;
  QLineEdit *speedThresh;

private slots:
  void updateRippleData(void);
};

class SpatialStimulation : public QWidget
{
  Q_OBJECT

public:
  SpatialStimulation (QWidget *parent);
  QSpinBox *lowerLeftX;
  QSpinBox *lowerLeftY;
  QSpinBox *upperRightX;
  QSpinBox *upperRightY;
  QLineEdit *minSpeedThresh;
  QLineEdit *maxSpeedThresh;
  QSpinBox *lockoutTime;

private slots:
  void updateSpatialData(void);
};


class RealtimeFeedbackTab : public QWidget
{
  Q_OBJECT

public:
  RealtimeFeedbackTab (QWidget *parent);

  QComboBox *stimulatorSelectComboBox;
  QComboBox *aOutSelectComboBox;
  QStackedWidget *algorithmAlternativesStack;

  QLabel *status;
  QGroupBox *statusGroupBox;

  QPushButton *resetFeedbackButton;
  QPushButton *startFeedbackButton;
  QPushButton *stopFeedbackButton;

public slots:
  void setFeedbackAlgorithm(int index);
  void updateActiveAOut(int aOutIndex, int aOut1Mode, int aOut2Mode);

private slots:
  void checkRealtimeStatus(void); 
  void updateRealtimeStatus(char *s); 

protected:
};



#endif
