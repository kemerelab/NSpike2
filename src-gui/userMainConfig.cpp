/****************************************************************************
** Form implementation generated from reading ui file 'stimform.ui'
**
** Created: Tue Apr 7 22:58:19 2009
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

//#include <string.h>
//#include <stdio.h>
#include "spikeUserGUI.h"
#include "userMainConfig.h"

extern DigIOInfo digioinfo;

/*
 *  Constructs a MainConfigTab as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
MainConfigTab::MainConfigTab( QWidget* parent )
    : QWidget( parent )
{
  int i;

    QGridLayout *grid = new QGridLayout;

    UserProgramStatus  = new QLabel("User Program [Not Running]",this);
    grid->addWidget(UserProgramStatus, 1,0, Qt::AlignRight);
    UserProgramCombo = new QComboBox(FALSE, this);
    grid->addWidget(UserProgramCombo, 1, 1);
    for (i = 0; i < digioinfo.nprograms; i++) {
      UserProgramCombo->insertItem(QString(digioinfo.progname[i]), i);
    }
    RunUserProgramButton = new QPushButton("Start", this);
    //RunUserProgramButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    grid->addWidget(RunUserProgramButton, 1, 2);
    connect(RunUserProgramButton, SIGNAL(clicked()), this, SLOT(runProgram(void)));
    connect(daq_io_widget, SIGNAL(changedUserProgramStatus(int)), this, SLOT(updateStatus(int)));
    
    grid->addWidget(new QLabel("Camera cm/pixel:", this), 2, 0,
        Qt::AlignRight);
    CmPerPix = new QLineEdit;
    CmPerPix->setValidator(new QDoubleValidator(this));
    CmPerPix->setAlignment(Qt::AlignRight);
    //CmPerPix->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    grid->addWidget(CmPerPix, 2, 1);
    connect(CmPerPix, SIGNAL(textChanged(const QString &)), this, SLOT(updateCmPerPix(void)));

    outputOnlyModePushButton =       new QPushButton("\nOutput Only Mode\n");
    realtimeFeedbackModePushButton = new QPushButton("\nRealtime Feedback Mode\n");
    outputOnlyModePushButton->setCheckable(true);
    realtimeFeedbackModePushButton->setCheckable(true);
    outputOnlyModePushButton->setStyleSheet("QPushButton::checked{color: green;}");
    realtimeFeedbackModePushButton->setStyleSheet("QPushButton::checked{color: green;}");
    outputOnlyModePushButton->setStyle("Windows");
    realtimeFeedbackModePushButton->setStyle("Windows");

    modeButtonGroup = new QButtonGroup;
    modeButtonGroup->addButton(outputOnlyModePushButton,OUTPUT_ONLY_MODE);
    modeButtonGroup->addButton(realtimeFeedbackModePushButton,REALTIME_FEEDBACK_MODE);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(outputOnlyModePushButton);
    hlayout->addWidget(realtimeFeedbackModePushButton);
    grid->addLayout(hlayout,4,0,1,3);

    loadSettingsButton = new QPushButton("Load Settings");
    saveSettingsButton = new QPushButton("Save Settings");
    QHBoxLayout *hlayout2 = new QHBoxLayout;
    hlayout2->addWidget(loadSettingsButton);
    hlayout2->addWidget(saveSettingsButton);
    grid->addLayout(hlayout2,6,0,1,3);

    grid->setAlignment(Qt::AlignHCenter);
    grid->setRowStretch(0,1);
    grid->setRowStretch(3,1);
    grid->setRowStretch(5,1);
    grid->setRowStretch(7,1);

    //QHBoxLayout *marginsLayout = new QHBoxLayout;
    //marginsLayout->addLayout(grid,1,0);

    setLayout(grid);

}

/*
 *  Destroys the object and frees any allocated resources
 */
MainConfigTab::~MainConfigTab()
{
    // no need to delete child widgets, Qt does it all for us
}

void MainConfigTab::initializeValues(void) {
  CmPerPix->setText(QString::number(DIO_DEFAULT_CM_PER_PIX));
}

void MainConfigTab::updateStatus(int whichProgram)
{

  if (whichProgram < 0) { // no user program running
    UserProgramStatus->setText(QString("User Program [Not Running]"));
    UserProgramCombo->setCurrentItem(0);
    RunUserProgramButton->setText(QString("Run"));
  }
  else {
    UserProgramStatus->setText(QString("User Program [Running]"));
    UserProgramCombo->setCurrentItem(whichProgram);
    RunUserProgramButton->setText(QString("Restart"));
  }
}
void MainConfigTab::updateCmPerPix(void)
{
  double data;

  if (digioinfo.outputfd) {
    // set single activation pin
    data = CmPerPix->text().toDouble();
    SendDAQUserMessage(DIO_SET_CM_PER_PIX, (char *) &data, sizeof(double));
    fprintf(stderr,"Sent pin\n");
  }
  else {
    QMessageBox::warning(this,"No User Program","No user program is currently running");
  }
}

void MainConfigTab::runProgram(void)
{
  StartDigIOProgram(UserProgramCombo->currentItem());
};

