#ifndef __SPIKE_FS_CONFIGURE_AOUT_H__
#define __SPIKE_FS_CONFIGURE_AOUT_H__

#include <QtGui>
#include "spike_stimcontrol_defines.h"

#define DEFAULT_TMP_PULSE_COMMANDS_FILE "/tmp/tmp_pulse_commands"

class AOutContinuousMode : public QWidget {
  Q_OBJECT

public:
  AOutContinuousMode(QWidget *parent = 0);

  QSpinBox *contPercentSpinBox;
};


class AOutPulseMode : public QWidget {
  Q_OBJECT

public:
  AOutPulseMode(QWidget *parent = 0);

  QDoubleSpinBox *pulseLengthSpinBox;
  QSpinBox *nPulsesSpinBox;
  QGroupBox *multiPulseGroup;
  QSpinBox *sequenceFrequencySpinBox;
  QDoubleSpinBox *sequencePeriodSpinBox;
  QSpinBox *pulsePercentSpinBox;

  PulseCommand aOutPulseCmd;

public slots:
  void pulseChanged(void);
  void frequencyChanged(void);
  void periodChanged(void);
  void ablePulseSequence(void);

signals:
  void aOutPulseCmdChanged(void);
};


/*
class AOutRampMode : public QWidget {
  Q_OBJECT

public:
  AOutRampMode(QWidget *parent = 0);
  QRadioButton *single;
  QRadioButton *continuous;
  QSpinBox     *min;
  QSpinBox     *max;
  QSpinBox     *length;

public slots:
  void rampParmChanged(void);
};

class AOutSineMode : public QWidget {
  Q_OBJECT

public:
  AOutSineMode(QWidget *parent = 0);
  QRadioButton *single;
  QRadioButton *continuous;
  QSpinBox     *min;
  QSpinBox     *max;
  QSpinBox     *freq;

public slots:
  void sinParmChanged(void);
};
*/


class AOutConfigureWidget : public QWidget
{
  Q_OBJECT

public:
  AOutConfigureWidget(QWidget *parent = 0);
  PulseCommand aOutPulseCmd;

public slots:
  void setAOutMode(int);
  void setAOutRange(int);
  void updateAOutPulseCmd(void);

private:
  QComboBox *aOutRangeSelectBox;
  QDoubleSpinBox *aOutRangeMinSpinBox;
  QDoubleSpinBox *aOutRangeMaxSpinBox;
  QSpinBox *aOutTriggerBitSpinBox;
  QComboBox *aOutModeComboBox;
  QStackedWidget *aOutModeStack;

  AOutContinuousMode *aOutContinuousMode;
  AOutPulseMode *aOutPulseMode;
  //AOutRampMode *aOutRampMode;
  //AOutSineMode *aOutSineMode;
  
signals:
  void aOutModeChangedSignal(void);
};


class AOutConfigTab : public QWidget
{
  Q_OBJECT

public:
  AOutConfigTab(QWidget *parent = 0);

  int activeAOut;
  AOutConfigureWidget *aOut1Config;
  AOutConfigureWidget *aOut2Config;

public slots:
  void selectAOut(void);
  void setActiveAOut(int);
  void aOutModeChanged(void);

signals:
  void activeAOutChanged(int, int, int);

private:
  QPushButton *aOut1Button;
  QPushButton *aOut2Button;

protected:
};
#endif
