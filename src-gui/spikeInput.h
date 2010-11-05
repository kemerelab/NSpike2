#ifndef __SPIKE_INPUT_H__
#define __SPIKE_INPUT_H__

//#include "sqcompat.h"
//#include "spike_defines.h"

#include <QtGui>

class SpikeLineEdit : public QLineEdit {
  Q_OBJECT

  public:
    SpikeLineEdit(QWidget *parent = 0, int chanNum = 0);
    ~SpikeLineEdit();    


    public slots:
      void      check() { checkinput(); }
    void       changed() { valueChanged(); };

signals:
    void  updateVal(int, unsigned short);

  public:
    void    checkinput(void);
    void    valueChanged(void);
    int chanNum;

  protected:
    QTimer *editTimer;
    int type;
};

class SpikeFiltSpinBox : public QSpinBox {
  Q_OBJECT

  public:
    SpikeFiltSpinBox(QWidget *parent = 0, bool highfilter = FALSE, int chanNum = 0);
    ~SpikeFiltSpinBox();    

    public slots:
      void      updateValue(int newval) { updateFilter(newval); }


signals:
    void  updateVal(int, unsigned short);

  protected:
    const bool  highfilter;
    int   lastVal;

  public: 
    void  updateFilter(int newval);
    int chanNum;

  private:
    QValidator::State validate(QString &, int &) const;
};

class SpikeAudioButton : public QPushButton {
  Q_OBJECT

  public:
    SpikeAudioButton(QWidget *parent = 0, int chan = 0, int output = 0, 
        bool fullScreenElect = FALSE);
    ~SpikeAudioButton();    
    int chan;
    int output;
    bool fullScreenElect;

  public slots:
    //void  changeStatus(bool on) { setAudio(); on = 0;}
    void  setAudio(void);

  public:
};



class SpikeTetInput: public QWidget {
  Q_OBJECT

  public:
    SpikeTetInput(QWidget *parent = 0, int electNum = 0, bool fullScreenElect = FALSE);
    ~SpikeTetInput();
    //void copyValues(SpikeTetInput *);
    void updateTetInput(void);
    void copyValues(SpikeTetInput *);
    int electNum;


    public slots:
      void      mdvChanged(int chanNum, unsigned short newVal) { 
        mdvUpdate(chanNum, newVal); }
    void      threshChanged(int chanNum, unsigned short newVal) { 
      threshUpdate(chanNum, newVal); } 
    void      lowFiltChanged(int chanNum, unsigned short newVal) { 
      lowFiltUpdate(chanNum, newVal); }
    void      highFiltChanged(int chanNum, unsigned short newVal) { 
      highFiltUpdate(chanNum, newVal); } 

  public:
    void mdvUpdate(int chanNum, unsigned short newVal);
    void threshUpdate(int chanNum, unsigned short newVal);
    void lowFiltUpdate(int chanNum, unsigned short newVal);
    void highFiltUpdate(int chanNum, unsigned short newVal);

  protected:
    bool fullScreenElect;
    //QWidget  *tetButtonWidget;
    SpikeLineEdit **maxdispval;
    SpikeLineEdit **thresh;
    SpikeFiltSpinBox   **lowfilt, **highfilt;
    QPushButton  *mdvSelectAll;
    QPushButton  *tSelectAll;
    QPushButton  *fSelectAll;
    QLabel *mdvlabel;
    QLabel *tlabel;
    QLabel *flabel;
    SpikeAudioButton **audio1;
    SpikeAudioButton **audio2;
};


class SpikeTetInfo: public QWidget {
  Q_OBJECT

  public:
    SpikeTetInfo(QWidget *parent = 0, int electNum = 0, bool fullScreenElect = FALSE);
    ~SpikeTetInfo();
    void updateTetInfo(void);
    void copyValues(SpikeTetInfo *);
    int electNum;

    public slots:
      void  depthChanged(int chanNum, unsigned short newVal) { 
        depthUpdate(chanNum, (short) newVal); }
    void  refElectChanged(int newVal) { 
      refElectUpdate(newVal); }
    void  refChanChanged(int newVal) { 
      refChanUpdate(newVal); }
    void  fullScreenChanged() { changeFullScreen(); }
    void  clearProjections() { clearTetProjections();}
    void  overlayChanged() { toggleOverlay(); }

  public:
    void depthUpdate(int chanNum, unsigned short newVal);
    void refElectUpdate(int newVal);
    void refChanUpdate(int newVal);
    void refUpdate(int refElect, int refChan);
    void changeFullScreen(void);
    void toggleOverlay(void);
    void clearTetProjections(void);

  protected:
    bool    fullScreenElect;
    int   dsp;  // the number of the dsp that handles this tetrode
    QLabel    *tetNumLabel;
    QLabel    *depthLabel;
    QLabel    *depthmmLabel;
    QLabel    *refLabel;
    QLabel    *refChLabel;
    QSpinBox  *refElect;
    QSpinBox  *refChan;
    QIntValidator *refChanValid;
    QPushButton     *overlay;
    QPushButton     *clearProj;
    QPushButton     *fullScreen;
    SpikeLineEdit   *depth;

};


class SpikeEEGButton: public QWidget {
  Q_OBJECT

  public:
    SpikeEEGButton(QWidget *parent = 0, int chanNum = 0);
    ~SpikeEEGButton();

  public:
    QPushButton   *chInfo;
    void updateButton(void);
    int  colNum;
    int  chanNum;

  protected:
    QLabel    *tetNumLabel;
};

class SpikeEEGInfo: public QWidget {
  Q_OBJECT

  public:
    SpikeEEGInfo(QWidget *parent = 0, int rowNum = 0);
    ~SpikeEEGInfo();

    public slots:
      void updateAllChannels() { updateAll(); };

  public:
    void updateAll();
    int  colNum;
    int  nChannels;

  protected:
    SpikeEEGButton **EEGButton;
};

class SpikeEEGDialog: public QDialog {
  Q_OBJECT

  public:
    SpikeEEGDialog(QWidget* parent, const char* name, 
        bool modal, Qt::WFlags fl, int chanNum);
    ~SpikeEEGDialog();

    public slots:
      void  tetChanChanged(int newVal) { 
        tetChanUpdate(newVal); }
    void  depthChanged(int chanNum, unsigned short newVal) { 
      depthUpdate(chanNum, (short) newVal); }
    void      lowFiltChanged(int chanNum, unsigned short newVal) { 
      lowFiltUpdate(chanNum, newVal); }
    void      highFiltChanged(int chanNum, unsigned short newVal) { 
      highFiltUpdate(chanNum, newVal); } 
    void  mdvChanged(int chanNum, unsigned short newVal) { 
      mdvUpdate(chanNum, newVal); }
    void  refElectChanged(int newVal) { 
      refElectUpdate(newVal); }
    void  refChanChanged(int newVal) { 
      refChanUpdate(newVal); }
    void    gotoNextTet(void) { nextTet(); };
    void    gotoPrevTet(void) { prevTet(); };

  public:
    void tetNumUpdate(int newVal);
    void tetChanUpdate(int newVal);
    void depthUpdate(int chanNum, short newVal);
    void lowFiltUpdate(int chanNum, short newVal);
    void highFiltUpdate(int chanNum, short newVal);
    void mdvUpdate(int chanNum, short newVal);
    void refElectUpdate(int newVal);
    void refChanUpdate(int newVal);
    void refUpdate(int refelect, int refchan);
    void nextTet(void);
    void prevTet(void);
    int  colNum;
    int  chanNum;

  protected:
    QLabel    *tetNumLabel;
    QLabel    *tetChanLabel;
    QSpinBox  *tetChanNum;
    QLabel    *depthLabel;
    SpikeLineEdit   *depth;
    QLabel    *depthmmLabel;
    QPushButton   *refSelectAll;
    QSpinBox  *refElect;
    QLabel    *refChanLabel;
    QSpinBox  *refChan;
    QIntValidator *refChanValid;
    SpikeLineEdit   *maxdispval;
    SpikeFiltSpinBox   *lowfilt, *highfilt;
    QPushButton  *mdvSelectAll;
    QPushButton  *fSelectAll;
    QPushButton *close;
    QPushButton *next;
    QPushButton *prev;

};

class SpikePosInfo: public QWidget {
  Q_OBJECT

  public:
    SpikePosInfo(QWidget* parent);
    ~SpikePosInfo();

    QSpinBox  *posThresh;
    void    updatePosInfo();

    public slots:
      void  posThreshChanged(int newVal); 

  protected:
    QLabel    *posThreshLabel;
    QSpacerItem *qspace;
};


class SpikeAudio: public QDialog {
  Q_OBJECT

  public:
    SpikeAudio(QWidget* parent, const char* name, bool modal, 
        Qt::WFlags fl);
    ~SpikeAudio();

    void updateAudio(void);
    QLabel    *label0;
    QLabel    *label1;
    QLabel    *volLabel;
    QLabel    *cutoffLabel;
    QLabel    *delayLabel;
    QSlider   *gain0;
    QSlider   *gain1;
    QSpinBox  *gain0spin;
    QSpinBox  *gain1spin;
    QSlider   *cutoff0;
    QSlider   *cutoff1;
    QSpinBox  *cutoff0spin;
    QSpinBox  *cutoff1spin;
    QSlider   *delay0;
    QSlider   *delay1;
    QSpinBox  *delay0spin;
    QSpinBox  *delay1spin;
    QRadioButton  *linked;
    QPushButton *mute;
    QPushButton *close;

    void  linkAudio(bool linked);
    void  changeGain(int chan, int newVal);
    void  changeCutoff(int chan, int newVal);
    void  changeDelay(int chan, int newVal);
    public slots:
      void  gain0Changed(int newVal) { changeGain(0, newVal);};
    void  gain1Changed(int newVal) { changeGain(1, newVal);};
    void  cutoff0Changed(int newVal) { changeCutoff(0, newVal);};
    void  cutoff1Changed(int newVal) { changeCutoff(1, newVal);};
    void  delay0Changed(int newVal) { changeDelay(0, newVal);};
    void  delay1Changed(int newVal) { changeDelay(1, newVal);};
    void  audioLinked(bool linked) { linkAudio(linked); };
    void  audioMute(void);
};


#endif
