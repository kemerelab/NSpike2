#ifndef __SPIKE_USER_OUTPUT_ONLY_H__
#define __SPIKE_USER_OUTPUT_ONLY_H__

#include <QtGui>

#define DEFAULT_TMP_PULSE_COMMANDS_FILE "/tmp/tmp_pulse_commands"

class StimOutputOnlyTab : public QWidget
{
  Q_OBJECT

public:
  StimOutputOnlyTab (QWidget *parent);

  QSpinBox *trainIntervalSpinBox;
  QSpinBox *nTrainsSpinBox;
  QPushButton *continuousButton;

  QComboBox *stimulatorSelectComboBox;
  QPushButton *stimSingleButton;
  QPushButton *startStimButton;
  QPushButton *abortStimButton;

public slots:
  void toggleContinuousMode(bool isToggled) {nTrainsSpinBox->setEnabled(!isToggled);}


private:

protected:
};



#endif
