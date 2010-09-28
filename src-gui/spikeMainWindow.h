#ifndef __SPIKE_MAIN_WINDOW_H__
#define __SPIKE_MAIN_WINDOW_H__

#include "spikeGLPane.h"
#include "spikeUserGUI.h"
#include "spike_dsp.h"
#include "spike_dio.h"

#include "rewardControl.h"
#include "spikeInput.h"
#include "spikeStatusbar.h"

#include <QtGui>
#include <q3textedit.h>

#define MAX_WELLS	8

extern DigIOInfo digioinfo;
extern DisplayInfo dispinfo;

class SpikeMainWindow : public QMainWindow {
  Q_OBJECT

  public:
    SpikeMainWindow(QWidget *parent = 0, const char *name = 0, 
        Qt::WFlags f = Qt::WType_TopLevel );
    virtual ~SpikeMainWindow();
    SpikeGLPane     **spikeGLPane;
    QButtonGroup     *audioGroup1;
    QButtonGroup     *audioGroup2;
    QButtonGroup     *fsaudioGroup1;
    QButtonGroup     *fsaudioGroup2;
    QButtonGroup     *EEGButtonGroup;
    QLabel         *timeLabel;
    QTabWidget     *qtab;
    setRewardsDialog  *setOut;
    rewardControl	*rewardCont;
    int         ntabs;
    int         prevPage;
    QWidget     **w;

    public slots:
      void      pageChanged(QWidget *w) { changeGLPage(w); }
    void       createEEGDialog(int ch) { 
      new SpikeEEGDialog(this, "EEGDialog", FALSE, 0, ch);};
    void      key(QKeyEvent *e) { keyPressEvent(e); }
    /* Master menu slots */
    void       masterAcq() { ToggleAcq(); };
    void       masterClear() { MasterClearAll(); };
    void       masterDiskOn() { MasterStartSave(); };
    void       masterDiskOff() { MasterStopSave(); };
    void       masterOpenFiles() { masterOpenDataFiles(); };
    void       masterCloseFiles() { masterCloseDataFiles(); };
    void       masterAudioSettings() { new SpikeAudio(this, "AudioDialog", 
        FALSE, 0);};
    void       masterMatlabSaveOn() { MasterMatlabStartSave(); };
    void       masterMatlabSaveOff() { MasterMatlabStopSave(); };
    void       masterMatlabSettings() { new matlabDialog(this, 
        "MatlabDialog", FALSE, 0);};
    void       masterResetClock() { ResetClock(); };
    void       masterReprogramMasterDSP() { reprogramDSPDialog(1); };
    void       masterReprogramAuxDSPs() { reprogramDSPDialog(0); };
    void       masterShowDSPCodeRev() { showDSPCodeRev(); };
    void       masterQuit() { Quit(0); };
    /* File menu slots */
    void      open() { openDataFile(); };
    void      close() { closeDataFile(); };
    void      saveConfig() { saveConfigFile(); };
    void      diskOn() { StartSave(); };
    void      diskOff() { StopSave(); };
    void      clear() { ClearAll(); };
    void      redraw() { DrawInitialScreen(); };
    /* Display menu slots */        
    void      eegTraceLength() { setEEGTraceLength(); };
    /* Tetrode settings menu slots */
    void 	  commonRef() { updateCommonRef();};
    void 	  commonThresh() { updateCommonThresh();};
    void 	  commonMDV() { updateCommonMDV();};
    void 	  commonFilt() { updateCommonFilt(); };
    /* Position Menu slots */
    void 	  allowSyncChange() { updateAllowSyncChange(); };
    void      outputPos() { updatePositionOutput();};
    /* Digital IO menu slots */
    void       digioOutput() { triggerOutput(); };
    void       runProgram(int prognum) {StartDigIOProgram(prognum);};
    void       setOutputs() { 
      if (!rewardGUIOn) {
        setOut = new setRewardsDialog(
            this, "Set Rewards", FALSE, 0);
        rewardGUIOn = true;
        connect(setOut, SIGNAL(finished() ), 
            this, SLOT(rewardGUIOff() ) );
      }
      else 
        QMessageBox::information(this, "Reward GUI Warning", "Only one Reward GUI can be open at a time.");

    };
    void       startRewardControl() { 
      if (!rewardGUIOn) {
        rewardCont = new rewardControl(this, "Set Rewards",
            FALSE, 0);
        rewardGUIOn = true;
        connect(rewardCont, SIGNAL(finished() ), 
            this, SLOT(rewardGUIOff() ) );
      }
      else
        QMessageBox::information(this, "Reward GUI Warning", "Only one Reward GUI can be open at a time.");
    }
    void       outputToUserProgram() { getOutputToUserProgram(); };
    void       userProgramGUI() { launchUserGUI() ; };
    void    toggleUsesCompression();
    void    doCompressionSettingsDialog();
    void 	warnCompSettingsTakeEffect();
    void	rewardGUIOff(void) { rewardGUIOn = false; };

  public: 
    // menu actions
    QAction *crAction;
    QAction *ctAction;
    QAction *cmdvAction;
    QAction *cfAction;
    QAction	*masterAcqAction;
    QAction	*masterStartSaveAction;
    QAction	*masterStopSaveAction;
    QAction	*masterOpenFilesAction;
    QAction	*masterCloseFilesAction;
    QAction	*masterResetClockAction;
    QAction	*masterReprogramDSPSAction;
    QAction	*matlabSettingsAction;
    QAction	*startSaveAction;
    QAction	*stopSaveAction;
    QAction	*openFileAction;
    QAction	*closeFileAction;
    QAction *usecompAction, *compsetAction; // for file->usescompression
    QAction *posOutputAction;
    QAction *posAllowSCAction;

    void    changeGLPage(QWidget *w);
    void    updateAllInfo();
    void    updateInfo();
    void    keyPressEvent(QKeyEvent *);
    /* Master menu items */
    void    masterOpenDataFiles();
    void    masterCloseDataFiles();
    void	reprogramDSPDialog(int master);
    void	showDSPCodeRev();
    /* file menu items */
    void    openDataFile();
    void    closeDataFile();
    void    saveConfigFile();
    void 	updateUsesCompression();
    /* display menu items */
    void    setEEGTraceLength();
    /* tetrode settings menu items */
    void 	updateCommonRef();
    void 	updateCommonThresh();
    void 	updateCommonMDV();
    void 	updateCommonFilt();
    /* position menu items */
    void 	updatePositionOutput();
    void 	updateAllowSyncChange();
    /* digioior menu items */
    void    triggerOutput();
    void    getOutputToUserProgram();
    void    launchUserGUI();
    QMenu    *masterMenu;
    QMenu    *fileMenu;

  protected:
    QPushButton   *b1, *b2, *b3;  
    //Q3GridLayout   *mainGrid;
    QMenuBar      *menuBar;
    QMenu    *displayMenu;
    QMenu    *posMenu;
    QMenu    *digioMenu;
    QMenu    *digioProgMenu;
    QMenu    *tetrodeSettingsMenu;

    SpikeInfo *spikeInfo;
    DIOInterface *laserC;

    int        	eegtab;
    bool		rewardGUIOn;

};

#endif // spikeMainWindow.h
