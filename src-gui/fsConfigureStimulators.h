#ifndef __SPIKE_FS_CONFIGURE_STIMULATORS_H__
#define __SPIKE_FS_CONFIGURE_STIMULATORS_H__

#include <QtGui>
#include "spike_stimcontrol_defines.h"

#define DEFAULT_TMP_PULSE_COMMANDS_FILE "/tmp/tmp_pulse_commands"

class StimConfigureWidget : public QWidget
{
  Q_OBJECT

public:
  StimConfigureWidget(const QString &title = "Group Box",QWidget *parent = 0);

  PulseCommand stimPulseCmd;

public slots:
  void frequencyChanged(void);
  void periodChanged(void);
  void ablePulseSequence(void);

  void ableBiphasicStimulation(int);

  void updateStimParameters(void);

private:
  QDoubleSpinBox *pulseLengthSpinBox;
  QSpinBox *nPulsesSpinBox;
  QGroupBox *multiPulseGroup;
  QSpinBox *sequenceFrequencySpinBox;
  QDoubleSpinBox *sequencePeriodSpinBox;

  QSpinBox *primaryStimPinSpinBox;
  QSpinBox *secondaryStimPinSpinBox;
  QLabel *secondaryStimPinLabel;
  QCheckBox *biphasicCheckBox;

protected:
};


class StimConfigTab : public QWidget
{
  Q_OBJECT

public:
  StimConfigTab(QWidget *parent = 0);
  int activeStimulator;
  StimConfigureWidget *stimConfigA;
  StimConfigureWidget *stimConfigB;

public slots:
  void selectStimulator(void);

  void setActiveStimulator(int);

signals:
  void activeStimulatorChanged(int);

private:
  QPushButton *stimulatorAButton;
  QPushButton *stimulatorBButton;

protected:
};



#endif
