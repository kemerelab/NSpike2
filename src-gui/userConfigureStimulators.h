#ifndef __SPIKE_USER_CONFIGURE_STIMULATORS_H__
#define __SPIKE_USER_CONFIGURE_STIMULATORS_H__

#include <QtGui>

#define DEFAULT_TMP_PULSE_COMMANDS_FILE "/tmp/tmp_pulse_commands"


class StimConfigTab : public QWidget
{
  Q_OBJECT

public:
  StimConfigTab(QWidget *parent = 0);

public slots:

private:
  QGroupBox *createStimAGroup();
  QGroupBox *createStimBGroup();

protected:
};


#endif
