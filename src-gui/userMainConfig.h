#ifndef __SPIKE_USER_MAIN_CONFIG_H__
#define __SPIKE_USER_MAIN_CONFIG_H__

#include <QtGui>

class MainConfigTab : public QWidget
{
    Q_OBJECT

public:
    MainConfigTab( QWidget* parent = 0);
    ~MainConfigTab();

    QLabel* UserDataStatus;

    QButtonGroup *modeButtonGroup;
    QPushButton *outputOnlyModeButton;
    QPushButton *realtimeFeedbackModeButton;
    QLineEdit* CmPerPix;

    QPushButton *loadSettingsButton;
    QPushButton *saveSettingsButton;

public slots:
    void updateStatus(void);
    void updateCmPerPix(void);
    void initializeValues(void);

protected:

protected slots:

};

#endif
