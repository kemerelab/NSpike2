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

#include "spikeGLPane.h"
#include "spikeUserGUI.h"
#include "spike_dsp.h"
#include "spike_dio.h"
#include "spike_main.h"
#define MAX_WELLS	8

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

  signals:
    void finished(void);

  private slots:
    void 	load() { loadFile(); };
    void 	save() { saveFile(); };
    void	createTabs(void) { 
      createLogicTab(nWells->value());
      createStatusTab(nWells->value()); }; 
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

  protected slots:
    void reject();
    void resetRewardCounters(void); 


  protected:
    void 		loadFile(void);
    void 	  saveFile(void);
    void		createLogicTab(int n);
    void		createStatusTab(int n);
    void		createListBoxItems(int nwells);
    void		createRewardListBox(Q3ListBox *lb);

    void		rewardWell(int n, bool reward);
    int 		getPrevWell(void);
    int 		getNextWell(void);
    void		setStatus(void);

    void 		writeRewardConfig(QString fileName);
    void 		readRewardConfig(QString fileName);

    QVector<int>    rewardCounter;

    int		  prevWell;
    bool		prevRewarded;

    /* tab widgets */
    QTabWidget     	*qtab;
    QString		tablabel;
    int         	ntabs;

    /* first tab: number of wells */
    QPushButton	*fileLoad;
    QPushButton	*fileSave;
    QLabel		*nWellsLabel;
    QSpinBox 	*nWells;
    QPushButton	*createTabsButton;
    QPushButton	*close;

    /* second tab - logic */
    QLabel		**wellLabel;
    QLabel		*prevLabel;
    Q3ListBox	**prev;
    QLabel		*currLabel;
    Q3ListBox	**curr;
    QLabel		*inputBitLabel;
    QSpinBox	**inputBit;
    QRadioButton	**triggerHigh;
    QLabel		*outputBitLabel;
    QSpinBox	**outputBit;
    QLabel		*rewardLengthLabel;
    QSpinBox	**rewardLength;
    QLabel		*rewardPercentLabel;
    QSpinBox	**rewardPercent;

    /* third tab */
    QRadioButton	*firstTrial;
    QLabel		*firstRewardLabel;
    QSpinBox	*firstRewardWell;
    Q3ButtonGroup	*rewardButtonGroup;
    QPushButton	**reward;
    QRadioButton	**next;
    QLabel		**status;
    QPushButton	*nextReward;
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
			digioinfo.rewardlength[bit] = len; };
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
	Q3ButtonGroup	*changeButtonGroup;
	QPushButton	*next;
	QPushButton	*prev;
	QPushButton	*close;

};

class matlabDialog : public QDialog {
	Q_OBJECT

    public:
	    matlabDialog(QWidget *parent = 0, 
		    const char *name = 0, bool model = FALSE, 
		    Qt::WFlags fl = 0);
	    ~matlabDialog();
	    void setMatlabInfo();

    public slots:
	    void acceptSettings(void) { setMatlabInfo(); };

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

#endif // spikeMainWindow.h
