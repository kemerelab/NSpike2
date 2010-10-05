#ifndef __SPIKE_USER_MAIN_CONFIG_H__
#define __SPIKE_USER_MAIN_CONFIG_H__

#include <QtGui>

class MainConfigTab : public QWidget
{
    Q_OBJECT

public:
    MainConfigTab( QWidget* parent = 0);
    ~MainConfigTab();

    QComboBox* UserProgramCombo;
    QPushButton* RunUserProgramButton;
    QLabel* UserProgramStatus;

    QButtonGroup *modeButtonGroup;
    QPushButton *outputOnlyModePushButton;
    QPushButton *realtimeFeedbackModePushButton;
    QLineEdit* CmPerPix;

public slots:
    void updateStatus(int);
    void updateCmPerPix(void);
    void runProgram(void);

protected:

protected slots:

};

#endif
