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

#include "spikecommon.h"

#include <QtGui>
#include <Q3GridLayout>


/*
 *  Constructs a StimForm as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
StimForm::StimForm( QWidget* parent, const char* name, Qt::WFlags fl )
    : QWidget( parent, name, fl )
{
    Q3GridLayout *grid = new Q3GridLayout(this, 13, 8, 10, 5);

    grid->addMultiCellWidget(new QLabel("Pulse Length (100 us)",this), 1, 1, 1, 2);
    PulseLengthSpinBox = new QSpinBox( 1, 20000, 1, this );
    grid->addMultiCellWidget(PulseLengthSpinBox, 1, 1, 3, 3);
    connect(PulseLengthSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimData(void)));


    grid->addMultiCellWidget(new QLabel("# Pulses in Train",this), 3, 3, 1, 1);
    NPulsesSpinBox = new QSpinBox(1, 10, 1, this );
    grid->addMultiCellWidget(NPulsesSpinBox, 3, 3, 2, 2);
    connect(NPulsesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimData(void)));
    connect(NPulsesSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ablePulseTrain(void)));

    grid->addMultiCellWidget(new QLabel("Frequency (Hz)",this), 4, 4, 2, 2);
    FrequencySpinBox = new QSpinBox( 1, 200, 1, this );
    grid->addMultiCellWidget(FrequencySpinBox, 4, 4, 3, 3);
    FrequencySpinBox->setEnabled(FALSE);
    connect(FrequencySpinBox, SIGNAL(valueChanged(int)), this, SLOT(frequencyChanged(void)));

    grid->addMultiCellWidget(new QLabel("Period (100 us)",this), 4, 4, 4, 4);
    PeriodSpinBox = new QSpinBox( 1, 10000, 1, this );
    grid->addMultiCellWidget(PeriodSpinBox, 4, 4, 5, 5);
    PeriodSpinBox->setEnabled(FALSE);
    connect(PeriodSpinBox, SIGNAL(valueChanged(int)), this, SLOT(periodChanged(void)));


    grid->addMultiCellWidget(new QLabel("# Pulse Trains",this), 6, 6, 1, 1);
    NTrainsSpinBox = new QSpinBox( 1, 50, 1, this );
    grid->addMultiCellWidget(NTrainsSpinBox, 6, 6, 2, 2);
    connect(NTrainsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimData(void)));
    connect(NTrainsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(ableMultiplePulses(void)));

    ContinuousButton = new QPushButton("Continuous", this );
    ContinuousButton->setToggleButton(TRUE);
    grid->addMultiCellWidget(ContinuousButton, 6, 6, 4, 4);
    connect(ContinuousButton, SIGNAL(toggled(bool)), this, SLOT(ableMultiplePulses(void)));
    connect(ContinuousButton, SIGNAL(toggled(bool)), NTrainsSpinBox, SLOT(setDisabled(bool)));
    connect(ContinuousButton, SIGNAL(toggled(bool)), this, SLOT(updateStimData(void)));

    grid->addMultiCellWidget(new QLabel("Inter-Pulse Interval (100 us)",this), 7, 7, 2, 3);
    InterPulseIntervalSpinBox = new QSpinBox( 1, 600000, 100, this );
    InterPulseIntervalSpinBox->setValue(50000);
    grid->addMultiCellWidget(InterPulseIntervalSpinBox, 7, 7, 4, 4);
    InterPulseIntervalSpinBox->setEnabled(FALSE);
    connect(InterPulseIntervalSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateStimData(void)));

    pulseStart = new QPushButton( "Start", this );
    pulseStart->setToggleButton(TRUE);
    grid->addMultiCellWidget(pulseStart, 12, 12, 1, 2);
    connect(pulseStart, SIGNAL(toggled(bool)), this, SLOT(startPulseSeq(bool)));

    pulseStop = new QPushButton( "Stop", this );
    pulseStop->setToggleButton(TRUE);
    grid->addMultiCellWidget(pulseStop, 12, 12, 4, 5);
    connect(pulseStop, SIGNAL(toggled(bool)), this, SLOT(stopPulseSeq(bool)));

    connect(daq_io_widget, SIGNAL(pulseFileFinished(void)), this, SLOT(stopPulseSeq(void)));


}

/*
 *  Destroys the object and frees any allocated resources
 */
StimForm::~StimForm()
{
    // no need to delete child widgets, Qt does it all for us
}

int StimForm::generateSingleStim(char *stimCommand)
{
  int len;

  if (NPulsesSpinBox->value() == 1) {
    len = sprintf(stimCommand, "p %d\n", PulseLengthSpinBox->value());
  }
  else {
    len = sprintf(stimCommand, "s %d %d %d\n", PulseLengthSpinBox->value(),
        FrequencySpinBox->value(), NPulsesSpinBox->value());
  }

  return len;
}

void StimForm::updateStimData(void)
{
  // create pulse array
  int commandLength = 0;
  int len;
  int i;
  char stimCommand[100];
  char pulseCommands[2000];

  memset(stimCommand,0,100);
  memset(pulseCommands,0,2000);

  len = generateSingleStim(stimCommand);

  len = sprintf(pulseCommands,"%s",stimCommand);
  commandLength += len;
  if (ContinuousButton->isOn()) {
      len = sprintf(pulseCommands+commandLength, "d %d\n", InterPulseIntervalSpinBox->value());
      commandLength += len;
      len = sprintf(pulseCommands+commandLength, "r\n");
      commandLength += len;
  }
  else if (NTrainsSpinBox->value() > 1) {
    for (i = 0; i < NTrainsSpinBox->value()-1; i++) {
      len = sprintf(pulseCommands+commandLength, "d %d\n", InterPulseIntervalSpinBox->value());
      commandLength += len;
      len = sprintf(pulseCommands+commandLength, "%s",stimCommand);
      commandLength += len;
    }
  }

  fprintf(stderr, "This is file:(%d)\n%s", commandLength, pulseCommands);

  SendMessage(digioinfo.outputfd, DIO_PULSE_SEQ, (char *) pulseCommands, 
    commandLength * sizeof(char));
  
  return;
}

void StimForm::ableMultiplePulses(void)
{
  if ((NTrainsSpinBox->value() > 1) || ContinuousButton->isOn())
    InterPulseIntervalSpinBox->setEnabled(TRUE);
  else
    InterPulseIntervalSpinBox->setEnabled(FALSE);
    return;
}

void StimForm::ablePulseTrain(void)
{
  if (NPulsesSpinBox->value() > 1) {
    FrequencySpinBox->setEnabled(TRUE);
    PeriodSpinBox->setEnabled(TRUE);
  }
  else {
    FrequencySpinBox->setEnabled(FALSE);
    PeriodSpinBox->setEnabled(FALSE);
  }
    return;
}

void StimForm::frequencyChanged(void)
{
  PeriodSpinBox->blockSignals(TRUE);
  PeriodSpinBox->setValue(10000/FrequencySpinBox->value());
  PeriodSpinBox->blockSignals(FALSE);
  updateStimData();
    return;
}

void StimForm::periodChanged(void)
{
  FrequencySpinBox->blockSignals(TRUE);
  FrequencySpinBox->setValue(10000/PeriodSpinBox->value());
  FrequencySpinBox->blockSignals(FALSE);
  updateStimData();
    return;
}

void StimForm::startPulseSeq(bool on)
{
    if (on) {
	if (digioinfo.outputfd) {
            fprintf(stderr,"Sending file\n");
      updateStimData();
            fprintf(stderr,"Sending enable\n");
	    SendDAQUserMessage(DIO_RT_ENABLE, NULL, 0);
	    SendMessage(digioinfo.outputfd, DIO_PULSE_SEQ_START, NULL, 0);
	    pulseStart->setPaletteForegroundColor("green");
	    pulseStart->setText("Running");
	    pulseStop->setPaletteForegroundColor("black");
	    pulseStop->setText("Stop");
	    /* Move the cursor to the beginning of the file and highlight the
	     * first line */
	    pulseStop->setOn(false);
	}
	else {
	    QMessageBox::warning(this,"No User Program","No user program is currently running");
	    pulseStart->setOn(false);
	}
    }
}

void StimForm::stopPulseSeq(bool on)
{
    if (on) {
	if (digioinfo.outputfd) {
	    SendMessage(digioinfo.outputfd, DIO_PULSE_SEQ_STOP, NULL, 0);
	    pulseStart->setPaletteForegroundColor("black");
	    pulseStart->setText("Start");
	    pulseStop->setPaletteForegroundColor("red");
	    pulseStop->setText("Stopped");
	    pulseStart->setOn(false);
      SendDAQUserMessage(DIO_RT_DISABLE, NULL, 0);
	}
	else {
	    QMessageBox::warning(this, "No User Program", 
		    "No user program is currently running");
	    pulseStop->setOn(false);
	}
    }
}

void StimForm::stopPulseSeq(void)
{
  stopPulseSeq(true);
}
