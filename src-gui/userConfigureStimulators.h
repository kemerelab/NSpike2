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

private:
  QGroupBox *groupBox;
  QGroupBox *multiPulseGroup;
  QDoubleSpinBox *pulseLengthSpinBox;
  QDoubleSpinBox *nPulsesSpinBox;
  QDoubleSpinBox *sequenceFrequencySpinBox;
  QDoubleSpinBox *sequencePeriodSpinBox;

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
