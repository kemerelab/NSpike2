#ifndef __REWARD_CONTROL_H__
#define __REWARD_CONTROL_H__

#include <QtGui>

#include <q3textedit.h>
#include <q3listbox.h>
#include <QKeyEvent>
#include <Q3GridLayout>
#include <Q3PopupMenu>
#include <q3buttongroup.h>
#include <q3filedialog.h>
#include <Q3TextStream>
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3BoxLayout>

#include "audio.h"
#include "spikeGLPane.h"
#include "spikeFSGUI.h"
#include "spike_dsp.h"
#include "spike_dio.h"
#include "spike_main.h"
#define MAX_WELLS	8
#define MAX_REST	90000	
#define MAX_REWARD_BITS	8	

extern DigIOInfo digioinfo;
extern DisplayInfo dispinfo;

/* Reward GUI */
class rewardControl : public QDialog {
  Q_OBJECT

  public:
    rewardControl(QWidget *parent = 0, 
        const char *name = 0, bool model = FALSE, 
        Qt::WFlags fl = 0);
    ~rewardControl();
    void 	DIOInput(DIOBuffer *diobuf);
    AudioThread *audThread;

  signals:
    void finished(void);

  private slots:
  //  void        timedOut(void){ audThread->stopSound();};

    void 	load() { loadFile(); };
    void 	save() { saveFile(); };
    void	createTabs(void) { 
      createLogicTab(nWells->value());
      createStatusTab(nWells->value()); 
      if(avoidButton->isChecked()) { 
        createAvoidTab(); 
      }
    };
    void  	setNextReward() { rewardWell(getNextWell(), true); };
    void  	setReward(int well) { rewardWell(well, true); };
    void  	setFirstReward(int well) { 
    int i;
    if (firstTrial->isOn()) {
        next[well]->setChecked(true);
        /* get rid of any other checked wells */
        for (i = 0; i < nWells->value(); i++) {
          if (i != well) {
            next[i]->setChecked(false);
          }
        }
      }
      setStatus();
    };
    void 	dialogClosed(void);

    void enableEditSequence(void);
    void loadSequence(void);

    void setSoundFileName(QString fileName, int bit);

  protected slots:
    void reject();
    void resetRewardCounters(void);
    void setAirTimer(void); 
    void	warnOff() { TriggerOutput(outputBit[1][0]->value()); };
    void	airOn() { TriggerOutput(outputBitAir->value()); };

  protected:
    void 		loadFile(void);
    void 	  	saveFile(void);
    void		createLogicTab(int n);
    void		createStatusTab(int n);
    void		createAvoidTab(void);
    void		createListBoxItems(int nwells);
    void		createRewardListBox(Q3ListBox *lb);

    void		rewardWell(int n, bool reward);
    int 		getPrevWell(void);
    int 		getNextWell(void);
    void		setStatus(void);

    void 		writeRewardConfig(QString fileName);
    void 		readRewardConfig(QString fileName);

    QVector<int>    	rewardCounter;

    int		  	prevWell;
    bool		prevRewarded;

    /* tab widgets */
    QTabWidget     	*qtab;
    QString		tablabel;
    int         	ntabs;

    /* first tab: number of wells */
    QPushButton	*fileLoad;
    QPushButton	*fileSave;
    QLabel	*nWellsLabel;
    QSpinBox 	*nWells;
    QLabel	*nOutputBitsLabel;
    QSpinBox	*nOutputBits;
    QPushButton	*createTabsButton;
    QPushButton	*close;
    QRadioButton *avoidButton;
    QRadioButton *audioButton;

    /* second tab - logic */
    QLabel		**wellLabel;
    QLabel		*prevLabel;
    Q3ListBox		**prev;
    QLabel		*currLabel;
    Q3ListBox		**curr;
    QLabel		*inputBitLabel;
    QSpinBox		**inputBit;
    QRadioButton	**triggerHigh;
    QLabel		**outputBitLabel;
    QSpinBox		***outputBit;
    QLabel		**outputBitLengthLabel;
    QSpinBox		***outputBitLength;
    QLabel		**outputBitPercentLabel;
    QSpinBox		***outputBitPercent;
    QLabel		**outputBitDelayLabel;
    QSpinBox		***outputBitDelay;
    QLabel		*output0BitLabel;
    QSpinBox		**output0Bit;
    QLabel		*output0BitLengthLabel;
    QSpinBox		**output0BitLength;
    QLabel		*output0SoundFileLabel;
    SpikeLineEdit	**output0SoundFile;

    /* third tab */
    QRadioButton	*firstTrial;
    QLabel		*firstRewardLabel;
    QSpinBox		*firstRewardWell;
    Q3ButtonGroup	*rewardButtonGroup;
    QPushButton		**reward;
    QRadioButton	**next;
    QLabel		**status;
    QPushButton		*nextReward;

    QPushButton		*useSequenceButton;
    QPushButton		*nextWellOnErrorButton;
    QPushButton		*loadSequenceButton;
    QPushButton		*editSequenceButton;
    QListWidget		*wellSequenceListWidget;

    /* fourth tab */
    QLabel		*restLengthLabel;
    QSpinBox		*restLength;
    QLabel		*warnLengthLabel;
    QSpinBox		*warnLength;
    QLabel		*warnPulseLabel;
    QSpinBox		*warnPulse;
    QLabel		*outputBitAirLabel;
    QSpinBox		*outputBitAir;
    QTimer		*timerRest;
    QTimer		*timerWarn;
    QTimer		*timerWarnOff;
};

class setRewardsDialog : public QDialog {
	Q_OBJECT

    public:
	    setRewardsDialog(QWidget *parent = 0, 
		    const char *name = 0, bool model = FALSE, 
		    Qt::WFlags fl = 0, int port = 0);
	    ~setRewardsDialog();
	    int	port;
    signals:
	    void finished(void);

    public slots:
	void	changePulseLength(int bit, unsigned short len) { 
			digioinfo.length[bit] = len; };
	void	changePulseDelay(int bit, u32 delay) { 
			digioinfo.delay[bit] = delay; };
	void	pulseBit(int bit) { TriggerOutput(bit + port * 16); };
	void	changeBit(int bit) { 
		       if (digioinfo.raised[bit+port*16]) {
			    ChangeOutput(bit + port * 16, 0); 
		       }
		       else {
			    ChangeOutput(bit + port * 16, 1); 
		       }
		};
	void  	gotoNextPort(void) { nextPort(); };
	void  	gotoPrevPort(void) { prevPort(); };
	void 	dialogClosed(void);

  protected slots:
    void reject();

    protected:
	void		nextPort();
	void		prevPort();
	QLabel		**outputNumberLabel;
	QLabel		**outputLengthLabel;
	SpikeLineEdit 	**outputLength;
	QPushButton  	**pulse;
	QRadioButton  	**change;
	Q3ButtonGroup	*pulseButtonGroup;
	QButtonGroup	*changeButtonGroup;
	QPushButton	*next;
	QPushButton	*prev;
	QPushButton	*close;

};

class fsDataDialog : public QDialog {
	Q_OBJECT

    public:
	    fsDataDialog(QWidget *parent = 0, 
		    const char *name = 0, bool model = FALSE, 
		    Qt::WFlags fl = 0);
	    ~fsDataDialog();
	    void setFSDataInfo();

    public slots:
	    void acceptSettings(void) { setFSDataInfo(); };

    signals:
	    void finished(void);

    protected:
	QRadioButton  	*posSelect;
	QRadioButton  	*digioSelect;
	QLabel		*spikeLabel;
	QLabel		*contLabel;
	QRadioButton  	**tetSpikeSelect;
	QRadioButton  	**tetContSelect;
	QPushButton	*accept;
};


#endif // rewardControl.h
