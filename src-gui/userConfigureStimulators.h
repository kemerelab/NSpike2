#ifndef __SPIKE_USER_CONFIGURE_STIMULATORS_H__
#define __SPIKE_USER_CONFIGURE_STIMULATORS_H__

#include <QtGui>

#define DEFAULT_TMP_PULSE_COMMANDS_FILE "/tmp/tmp_pulse_commands"

class StimConfigureWidget : public QWidget
{
  Q_OBJECT

public:
  StimConfigureWidget(const QString &title = "Group Box",QWidget *parent = 0);

public slots:
  void frequencyChanged(void);
  void periodChanged(void);
  void ablePulseSequence(void);

  void ableBiphasicStimulation(int);

private:
  QDoubleSpinBox *pulseLengthSpinBox;
  QSpinBox *nPulsesSpinBox;
  QSpinBox *sequenceFrequencySpinBox;
  QDoubleSpinBox *sequencePeriodSpinBox;

  QSpinBox *primaryStimPinSpinBox;
  QSpinBox *secondaryStimPinSpinBox;
  QCheckBox *biphasicCheckBox;

protected:
};


class StimConfigTab : public QWidget
{
  Q_OBJECT

public:
  StimConfigTab(QWidget *parent = 0);

public slots:

private:
  StimConfigureWidget *stimConfigA;
  StimConfigureWidget *stimConfigB;

protected:
};



#endif
