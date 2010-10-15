#ifndef __SPIKE_USER_GUI_H__
#define __SPIKE_USER_GUI_H__

#include "spikeGLPane.h"
#include "spike_dsp.h"
#include "spike_main.h"

#include "spike_stimcontrol_defines.h"
#include "userMainConfig.h"
#include "userOutputOnlyTab.h"
#include "userRealtimeFeedbackTab.h"

#include <QtGui>
#include <q3textedit.h>

#define DEFAULT_TMP_PULSE_COMMANDS_FILE "/tmp/tmp_pulse_commands"
#define MAIN_CONFIG_TAB 0
#define CONFIG_STIMULATORS_TAB 1
#define OUTPUT_ONLY_TAB 2
#define REALTIME_FEEDBACK_TAB 3

extern void  SendDAQUserMessage(int message, char *data, int datalen);

extern void StartDigIOProgram(int prognum);

class DAQ_IO : public QWidget {
	Q_OBJECT
public:
  DAQ_IO(QWidget *parent = 0);
  QStringList *ChannelStrings;
  int StimChan;
  int StimChanChanged;
  int UserProgramRunning;

signals:
  void changedUserProgramStatus(int);
  void updateChanDisplay(int);
  void userProgramRunning(bool);
  void pulseFileLineExecuted(int);
  void rippleStatusUpdate(RippleStatusMsg);
  void pulseFileFinished(void);

public slots:
  void updateChan(int);
  void checkUserProgramStatus(void);
  void msgFromUser(int msg, char *data);

};

extern DAQ_IO *daq_io_widget; // global, but created in DIOInterface


class DIOInterface : public QDialog {
	Q_OBJECT

public:
  DIOInterface(QWidget *parent = 0, 
   const char *name = 0, bool model = FALSE, 
   Qt::WFlags fl = 0);
  ~DIOInterface();
  void msgFromUser(int msg, char *data) { daq_io_widget->msgFromUser(msg,data); }

public slots:
  void  enableTabs(bool);
  void  changeOperatingMode(int);
  void  loadSettings(void);
  void  saveSettings(void);

protected:
  int         	ntabs;

  /* Control tabs */
  Q3ButtonGroup *indexGroup;
  // Q3WidgetStack     	*qtab;
  QTabWidget *qtab;

  MainConfigTab *mainConfigTab;
  //PulseFileTab *pulseFileTabWidget;
  StimOutputOnlyTab *stimOutputOnlyTab;
  RealtimeFeedbackTab *realtimeFeedbackTab;
signals:

};

/*
class PulseFileTab : public QWidget
{
  Q_OBJECT

public:
  PulseFileTab(QWidget *parent = 0);
  int  currentLine;

public slots:
  void  open() { openFile(); };
  void  close() { closeFile(); };
  void  save() { saveFile(); };
  void  send() { sendFileToUser(); };
	void 	startPulseSeq(bool);
	void 	stopPulseSeq(bool);
	void	disableStart() { pulseStart->setEnabled(false); };
  void  lineExecuted(int);

protected:
	QPushButton  *pulseStart;
	QPushButton  *pulseStop;

  QMenuBar     *menuBar;
  Q3PopupMenu   *fileMenu;
  QWidget      *PulseFileWidget;
	Q3TextEdit	   *textEdit;

  int   openFileID;
  int   saveFileID;
  int   closeFileID;
  int   sendFileID;

  void  openFile();
	int		fileOpen;
  void  saveFile();
  void  closeFile();
  void  sendFileToUser();
};

class ThetaTab : public QWidget
{
  Q_OBJECT

public:
  ThetaTab(QWidget *parent = 0);

public slots:
  void  updateThetaData(void);
	void 	startThetaStim(bool);
	void 	stopThetaStim(bool);
	void	disableStart() { triggeredStart->setEnabled(false); };
  void  changeStimChanDisplay(int ch) {StimChan->setCurrentItem(ch);};


protected:
  QComboBox *StimChan;
  QSpinBox *pulse_len;
  QSpinBox *vel_thresh;
  QSpinBox *filt_delay;
  QSpinBox *theta_phase;

	QPushButton  	*triggeredStart;
	QPushButton  	*triggeredStop;
};


class RippleTab : public QWidget
{
  Q_OBJECT

public:
  RippleTab(QWidget *parent = 0);

public slots:
  void  updateRippleData(void);
	void 	startRTData(bool);
	void 	stopRTData(bool);
  void  enableRipStim(bool);
  void  changeStimChanDisplay(int ch) {StimChan->setCurrentItem(ch);};
  void  checkRippleStatus(void);
  void  updateRippleStatus(RippleStatusMsg status);

protected:
  QComboBox *StimChan;
  QSpinBox *pulse_len;
  QSpinBox *vel_thresh;
  QSpinBox *filt_delay;
  QSpinBox *theta_phase;
  QLineEdit *ripCoeff1;
  QLineEdit *ripCoeff2;
  QSpinBox *timeDelay;
  QSpinBox *timeJitter;
  QLineEdit *ripThresh;
  QSpinBox *lockoutPeriod;
  QLineEdit *speedThresh;

  QLabel *statusBox;

	QPushButton  	*enableButton;

	QPushButton  	*startButton;
	QPushButton  	*stopButton;

  QTimer *timer;

  void showEvent(QShowEvent *e);
};

class LatencyTab : public QWidget
{
  Q_OBJECT

public:
  LatencyTab(QWidget *parent = 0);

public slots:
  void  updateLatencyData(void);
	void 	startTest(bool);
	void 	stopTest(bool);
	void	disableStart() { triggeredStart->setEnabled(false); };
  void  changeStimChanDisplay(int ch) {StimChan->setCurrentItem(ch);};

protected:
  QComboBox *StimChan;
  QSpinBox *thresh;

	QPushButton  	*triggeredStart;
	QPushButton  	*triggeredStop;
};


class StimForm : public QWidget
{
    Q_OBJECT

public:
    StimForm( QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0 );
    ~StimForm();

    QSpinBox* PulseLengthSpinBox;
    QSpinBox* FrequencySpinBox;
    QSpinBox* PeriodSpinBox;
    QSpinBox* InterPulseIntervalSpinBox;
    QSpinBox* NPulsesSpinBox;
    QSpinBox* NTrainsSpinBox;
    QPushButton* ContinuousButton;

    QPushButton* pulseStart;
    QPushButton* pulseStop;

    int generateSingleStim(char *command);

public slots:
    void updateStimData(void);
    void ableMultiplePulses(void);
    void ablePulseTrain(void);
    void periodChanged(void);
    void frequencyChanged(void);
    void startPulseSeq(bool on);
    void stopPulseSeq(bool on);
    void stopPulseSeq(void);

protected:

protected slots:

};
*/

#endif
