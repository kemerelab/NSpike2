#ifndef __SPIKE_STATUSBAR_H__
#define __SPIKE_STATUSBAR_H__

#include <QtGui>

class SpikeInfo: public QStatusBar {
  Q_OBJECT

  public:
    SpikeInfo(QWidget* parent);
    ~SpikeInfo();
    void updateInfo(void);

    QPushButton *clear;
    QLabel    *message;
    QLabel    *fileStatus;
    QLabel    *fileSize;
    QLabel    *diskStatus;
    QLabel    *diskFree;

    public slots:
      void  clearStatus();
};

#endif
