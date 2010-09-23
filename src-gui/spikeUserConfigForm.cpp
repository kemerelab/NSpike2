/****************************************************************************
** Form implementation generated from reading ui file 'stimform.ui'
**
** Created: Tue Apr 7 22:58:19 2009
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include <string.h>
#include <stdio.h>
#include "spikeUserGUI.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qmessagebox.h>
//Added by qt3to4:
#include <Q3GridLayout>

/*
 *  Constructs a ConfigForm as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
ConfigForm::ConfigForm( QWidget* parent, const char* name, Qt::WFlags fl )
    : QWidget( parent, name, fl )
{
  int i;


    Q3GridLayout *grid = new Q3GridLayout(this, 13, 8, 10, 5);

    UserProgramStatus  = new QLabel("User Program [Not Running]",this);
    grid->addMultiCellWidget(UserProgramStatus, 2, 2, 1, 2);
    UserProgramCombo = new QComboBox(FALSE, this);
    grid->addMultiCellWidget(UserProgramCombo, 2, 2, 3, 4);
    for (i = 0; i < digioinfo.nprograms; i++) {
      UserProgramCombo->insertItem(QString(digioinfo.progname[i]), i);
    }
    RunUserProgramButton = new QPushButton("Start", this);
    grid->addMultiCellWidget(RunUserProgramButton, 2, 2, 5, 5);
    connect(RunUserProgramButton, SIGNAL(clicked()), this, SLOT(runProgram(void)));
    connect(daq_io_widget, SIGNAL(changedUserProgramStatus(int)), this, SLOT(updateStatus(int)));
    
    grid->addMultiCellWidget(new QLabel("Timing/RT IO Tetrode / channel:", this), 4, 4, 1, 2);
    StimChan = new QComboBox( FALSE, this, "Timer Channel Combo Box" );
    StimChan->insertStringList(*(daq_io_widget->ChannelStrings));
    grid->addMultiCellWidget(StimChan, 4, 4, 3, 3);
    connect(StimChan, SIGNAL(activated( int )), daq_io_widget, SLOT(updateChan(int)));
    connect(daq_io_widget, SIGNAL(updateChanDisplay(int)), this, SLOT(changeStimChanDisplay(int)));

    grid->addMultiCellWidget(new QLabel("Camera cm/pixel:", this), 6, 6, 2, 3);
    CmPerPix = new QLineEdit(QString::number(DIO_DEFAULT_CM_PER_PIX), this);
    CmPerPix->setValidator(new QDoubleValidator(this));
    CmPerPix->setAlignment(Qt::AlignRight);
    grid->addMultiCellWidget(CmPerPix, 6, 6, 4, 4);
    connect(CmPerPix, SIGNAL(textChanged(const QString &)), this, SLOT(updateCmPerPix(void)));

    grid->addMultiCellWidget(new QLabel("Activation Pin",this), 9, 9, 2, 3);
    PortSpinBox = new QSpinBox( 0, 127, 1, this );
    grid->addMultiCellWidget(PortSpinBox, 9, 9, 4, 4);
    connect(PortSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimPins(void)));

    BiphasicButton = new QPushButton("Biphasic", this );
    BiphasicButton->setToggleButton(TRUE);
    grid->addMultiCellWidget(BiphasicButton, 10, 10, 1, 1);
    connect(BiphasicButton, SIGNAL(toggled(bool)), this, SLOT(updateStimPins(void)));

    grid->addMultiCellWidget(new QLabel("Activation Pin 2",this), 10, 10, 2, 3);
    Port2SpinBox = new QSpinBox( 0, 127, 1, this );
    grid->addMultiCellWidget(Port2SpinBox, 10, 10, 4, 4);
    Port2SpinBox->setEnabled(FALSE);
    connect(Port2SpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimPins(void)));
    connect(BiphasicButton, SIGNAL(toggled(bool)), Port2SpinBox, SLOT(setEnabled(bool)));

}

/*
 *  Destroys the object and frees any allocated resources
 */
ConfigForm::~ConfigForm()
{
    // no need to delete child widgets, Qt does it all for us
}

void ConfigForm::updateStatus(int whichProgram)
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

void ConfigForm::updateStimPins(void)
{
  int data[2];

  int stimPin1 = PortSpinBox->value();
  int stimPin2 = Port2SpinBox->value();

  if (digioinfo.outputfd) {
    if (BiphasicButton->isOn()) {
      // set biphasic and 2 pins
      data[0] = stimPin1;
      data[1] = stimPin2;
      if ((data[0] / 16) != (data[1] / 16))
QMessageBox::warning(this,"Invalid Stimulation Ports","Stimulation ports for stim 1 and 2 must be the same. Stim Pin 1 port assumed.");
      SendDAQUserMessage(DIO_SET_BIPHASIC_STIM_PINS, (char *) data, 2*sizeof(int));
      fprintf(stderr,"Sent pins\n");
    }
    else {
      // set single activation pin
      data[0] = stimPin1;
      SendDAQUserMessage(DIO_SET_SINGLE_STIM_PIN, (char *) data, sizeof(int));
      fprintf(stderr,"Sent pin\n");
    }

  }
  else {
    QMessageBox::warning(this,"No User Program","No user program is currently running");
  }
}


void ConfigForm::updateCmPerPix(void)
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

