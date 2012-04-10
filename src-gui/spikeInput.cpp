/*
 * spikeInput.cpp:  All buttons and input related objects for nspike
 *
 * Copyright 2005 Loren M. Frank
 *
 * This program is part of the nspike data acquisition package.
 * nspike is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * nspike is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
    spikeTetInput = new SpikeTetInput* [nElect];
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "spikeInput.h"
#include "spike_main.h"
#include "spike_functions.h"
#include "spikeMainWindow.h"
#include "audio.h"

#include <string>
#include <sstream>

extern DisplayInfo dispinfo;
extern SysInfo sysinfo;
extern CommonDSPInfo cdspinfo;
extern SpikeMainWindow *spikeMainWindow;
extern char *tmpstring;

SpikeLineEdit::SpikeLineEdit(QWidget *parent, int chanNum, bool textIsNumeric) : 
	QLineEdit(parent), chanNum(chanNum), textIsNumeric(textIsNumeric)
{
    setStyle("Windows");
    // Attach the checkinput slot to a timer so that we can determine if the
    // input has been changed
    editTimer = new QTimer(this);
    connect(editTimer, SIGNAL(timeout()), this, SLOT(check()));
    editTimer->start(200, FALSE);

    /* we also want to execute the valuechanged signal when the user has
     * changed the text */
    connect(this, SIGNAL(lostFocus()), 
	    this, SLOT(changed()));
    //this->setPaletteBackgroundColor("grey98");
  setAutoFillBackground(true);
}


SpikeLineEdit::~SpikeLineEdit() {
}

void SpikeLineEdit::checkinput(void) {
    if (this->isModified()) {
	/* change the text color to red */
	this->setPaletteForegroundColor("red");
    }
    else {
	this->setPaletteForegroundColor("black");
    }
}

void SpikeLineEdit::valueChanged(void) {
    /* the value has been changed, so we reset the modified flag and pass a
     * message back to our parent */
    this->clearModified();
    if (textIsNumeric) {
	emit updateVal(this->chanNum, (unsigned short) this->text().toInt());
    }
    else {
	emit updateText(this->chanNum, this->text());
    }
}


SpikeFiltSpinBox::SpikeFiltSpinBox(QWidget *parent, bool highfilter, int chanNum) : QSpinBox(parent), highfilter(highfilter), chanNum(chanNum) {
    //this->setPaletteBackgroundColor("lightgrey");

    /* check the values for the filter and set the displayed text to the
     * correct number */
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(updateValue(int)));
    
    /* set the range */
    this->setRange(cdspinfo.lowFilt[0], cdspinfo.highFilt[NDSP_HIGH_FILTERS-1]);

    setStyle("Windows");
}

QValidator::State SpikeFiltSpinBox::validate(QString &input, int &pos) const {
    /* check the input string against the possible filter values to see if it
     * is valid */
    int v, i;
    bool ok;

    v = input.toInt(&ok);
    pos = 0;
    if (!ok)
	return QValidator::Invalid;
    /* The user should only use the up and down arrows, so we only allow
     * exact matches */
    if (this->highfilter) {
	for (i = 0; i < cdspinfo.nHighFilters; i++) {
	    if (v == cdspinfo.highFilt[i]) {
		return QValidator::Acceptable;
	    }
	    else {
		return QValidator::Invalid;
	    }
	}
    }
    else {
	for (i = 0; i < cdspinfo.nLowFilters; i++) {
	    if (v == cdspinfo.lowFilt[i]) {
		return QValidator::Acceptable;
	    }
	    else {
		return QValidator::Invalid;
	    }
	}
    }
    return QValidator::Invalid;
}

SpikeFiltSpinBox::~SpikeFiltSpinBox() {
}

void SpikeFiltSpinBox::updateFilter(int val) {
    /* set the value to the next legitimate value */
    /* as the value will change by 1, we need to figure out what the
     * corresponding filter setting is and set the value appropriately */
    int i;
    if (this->highfilter) {
	 /* go through the high filter values and find the closest one.*/
	for (i = 0; i < cdspinfo.nHighFilters; i++) {
	    if ((i < cdspinfo.nHighFilters - 1) && 
		(val - cdspinfo.highFilt[i] == 1)) {
		/* go to the next filter if possible */
		this->setValue(cdspinfo.highFilt[i+1]);
		break;
	    }
	    else if ((i == cdspinfo.nHighFilters - 1) && 
		      (val - cdspinfo.highFilt[i] == 1)) {
		this->setValue(this->value() - 1);
		break;
	    }
	    else if ((i > 0) && (val - cdspinfo.highFilt[i] == -1)) {
		/* go to the next higher filter if possible */
		this->setValue(cdspinfo.highFilt[i-1]);
		break;
	    }
	    else if ((i == 0) && (val - cdspinfo.highFilt[i] == -1)) {
		this->setValue(this->value() + 1);
		break;
	    }
	}
    }
    else {
	for (i = 0; i < cdspinfo.nLowFilters; i++) {
	    if ((i < cdspinfo.nLowFilters - 1) && 
		(val - cdspinfo.lowFilt[i] == 1)) {
		/* go to the next filter if possible */
		this->setValue(cdspinfo.lowFilt[i+1]);
		break;
	    }
	    else if ((i == cdspinfo.nLowFilters - 1) && 
		      (val - cdspinfo.lowFilt[i] == 1)) {
		this->setValue(this->value() - 1);
		break;
	    }
	    else if ((i > 0) && (val - cdspinfo.lowFilt[i] == -1)) {
		/* go to the next lower filter if possible */
		this->setValue(cdspinfo.lowFilt[i-1]);
		break;
	    }
	    else if ((i == 0) && (val - cdspinfo.lowFilt[i] == -1)) {
		this->setValue(this->value() + 1);
		break;
	    }
	}
    }
    this->lastVal = this->value();
    emit updateVal(this->chanNum, this->value());
}



SpikeAudioButton::SpikeAudioButton(QWidget *parent, int chan, int output, bool fullScreenElect) :QPushButton(parent), chan(chan), output(output), 
	fullScreenElect(fullScreenElect) {
    /* create an audio selection button. This button turns yellow when active 
     * and calls UpdateAudio(chan) to update the audio output of the system */
    /* set this to be a toggle button */
    setToggleButton(true);

    if (this->fullScreenElect) {
      /* add this button the fsaudioButton button group */
      dispinfo.fsaudioGroup[output]->insert(this);
    }
    else {
      /* add this button the fsaudioButton button group */
      dispinfo.audioGroup[output]->insert(this);
    }

    /* connect the stateChanged signal to the color and the output */
    connect(this, SIGNAL(clicked(bool)), this, SLOT(setAudio())); 
    //connect(this, SIGNAL(toggled(bool)), this, SLOT(changeStatus(bool))); 

    if (output == 0)
      setStyleSheet("QPushButton::checked{color: yellow;}");
    if (output == 1)
      setStyleSheet("QPushButton::checked{color: green;}");

    setStyle("Windows");
}

SpikeAudioButton::~SpikeAudioButton() {
}

void SpikeAudioButton::setAudio() {
    ChannelInfo *ch;

    if (dispinfo.fullscreenelect != -1) { // fullscreen mode
      /* if we are in full screen mode, the channel number refers to the
       * number within the tetrode, not the absolute channel, so we need to
       * calculate the actual channel number */
      ch = sysinfo.channelinfo[sysinfo.machinenum] + 
        dispinfo.fullscreenelect * NCHAN_PER_ELECTRODE + this->chan;
    }
    else {
      ch = sysinfo.channelinfo[sysinfo.machinenum] + this->chan;
    }

    if (this->isChecked()) {
      cdspinfo.audiochan[this->output].dspchan = ch->dspchan;
      SetAudio(this->output, ch, 1);
    }
    else {
      /* the button was just toggled off. This only happens when the user has
       * clicked on another audio button on this machine */
      //cdspinfo.audiochan[this->output].dspchan = -1;
    }
    //this->updateButton();
}

SpikeTetInput::SpikeTetInput(QWidget *parent, int electNum, 
	bool fullScreenElect) 
	: QWidget(parent), electNum(electNum), fullScreenElect(fullScreenElect)
{
  ChannelInfo *ch;
  int currentchan, i;
  int ndiv= 10;

  setAutoFillBackground(true);

  /* Get the appropriate channelinfo structure */
  currentchan = electNum * NCHAN_PER_ELECTRODE;
  ch = sysinfo.channelinfo[sysinfo.machinenum] + currentchan;

  /* create a grid layout for the spike waveform windows */
  Q3GridLayout *grid = new Q3GridLayout(this, 4, 
      NCHAN_PER_ELECTRODE * ndiv, 0, 0, "spikeTetInputLayout");

  QSizePolicy p(QSizePolicy::Preferred, QSizePolicy::Preferred, FALSE);
  QSizePolicy p2(QSizePolicy::Maximum, QSizePolicy::Maximum, FALSE);

  QFont f( "SansSerif", 8, QFont::Normal );
  QFont fs( "SansSerif", 7, QFont::Normal );
  if (fullScreenElect) {
    f = QFont( "SansSerif", 16, QFont::Normal );
    fs = QFont( "SansSerif", 12, QFont::Normal );
  }
  /* create the push buttons that select whether all of the channels are
   * changed at once */
  mdvSelectAll = new QPushButton("M", this, "mdv_all");
  mdvSelectAll->setToggleButton(TRUE);
  mdvSelectAll->setSizePolicy(p2);
  mdvSelectAll->setFont(f);
  mdvSelectAll->setStyle("Windows");
  tSelectAll = new QPushButton("T", this, "f_all");
  tSelectAll->setToggleButton(TRUE);
  tSelectAll->setSizePolicy(p2);
  tSelectAll->setFont(f);
  tSelectAll->setStyle("Windows");
  fSelectAll = new QPushButton("F", this, "t_all");
  fSelectAll->setToggleButton(TRUE);
  fSelectAll->setSizePolicy(p2);
  fSelectAll->setFont(f);
  fSelectAll->setStyle("Windows");

  mdvSelectAll->setOn(TRUE);
  tSelectAll->setOn(TRUE);
  fSelectAll->setOn(TRUE);

  /* put the buttons in the grid */
  grid->addMultiCellWidget(mdvSelectAll, 0, 0, 0, 0);
  grid->addMultiCellWidget(tSelectAll, 1, 1, 0, 0);
  grid->addMultiCellWidget(fSelectAll, 2, 2, 0, 0);

  /* create the pointers to the line input boxes */
  maxdispval = new SpikeLineEdit* [NCHAN_PER_ELECTRODE]; 
  thresh = new SpikeLineEdit* [NCHAN_PER_ELECTRODE]; 

  /* create the pointers to the filter spin boxes */
  lowfilt = new SpikeFiltSpinBox* [NCHAN_PER_ELECTRODE]; 
  highfilt = new SpikeFiltSpinBox* [NCHAN_PER_ELECTRODE]; 

  /* create the pointer to the audio push buttons */
  audio1 = new SpikeAudioButton* [NCHAN_PER_ELECTRODE];
  audio2 = new SpikeAudioButton* [NCHAN_PER_ELECTRODE];

  QString ss;

  /* set up the validators to check the input */
  QIntValidator *valid = new QIntValidator(0, MAX_DISP_VAL, this, 
      "dataValValidator");
  for(i = 0; i < NCHAN_PER_ELECTRODE; i++, currentchan++) {
    maxdispval[i] = new SpikeLineEdit(this, currentchan);
    maxdispval[i]->setFont(f);
    ss = QString("%1").arg(ch[i].maxdispval);
    maxdispval[i]->setText(QString(ss));
    /* set up a validator for the value */
    maxdispval[i]->setValidator(valid);
    maxdispval[i]->setSizePolicy(p);
    /* put the label and the text window into the local layout */
    grid->addMultiCellWidget(maxdispval[i], 0, 0, (i*ndiv)+1, (i*ndiv)+9);

    /* connect the update message to the update slot */
    connect(maxdispval[i], SIGNAL(updateVal(int, unsigned short)), this, SLOT(mdvChanged(int, unsigned short)));

    thresh[i] = new SpikeLineEdit(this, currentchan);
    thresh[i]->setFont(f);
    ss = QString("%1").arg(ch[i].thresh);
    thresh[i]->setText(QString(ss));
    thresh[i]->setValidator(valid);
    thresh[i]->setSizePolicy(p);
    /* put the label and the text window into the local layout */
    grid->addMultiCellWidget(thresh[i], 1, 1, (i*ndiv)+1, (i*ndiv)+9);
    connect(thresh[i], SIGNAL(updateVal(int, unsigned short)), this, SLOT(threshChanged(int, unsigned short)));

    lowfilt[i] = new SpikeFiltSpinBox(this, FALSE, currentchan);
    lowfilt[i]->setFont(fs);
    lowfilt[i]->setValue(ch[i].lowfilter);
    lowfilt[i]->setSizePolicy(p);
    /* put the label and the text window into the local layout */
    grid->addMultiCellWidget(lowfilt[i], 2, 2, (i*ndiv)+1, (i*ndiv)+4);
    connect(lowfilt[i], SIGNAL(updateVal(int, unsigned short)), this, SLOT(lowFiltChanged(int, unsigned short)));

    highfilt[i] = new SpikeFiltSpinBox(this, TRUE, currentchan);
    highfilt[i]->setFont(fs);
    highfilt[i]->setValue(ch[i].highfilter);
    highfilt[i]->setSizePolicy(p);
    /* put the label and the text window into the local layout */
    grid->addMultiCellWidget(highfilt[i], 2, 2, (i*ndiv)+5, (i*ndiv)+9);
    connect(highfilt[i], SIGNAL(updateVal(int, unsigned short)), this, SLOT(highFiltChanged(int, unsigned short)));

    /* Create the audio buttons */
    audio1[i] = new SpikeAudioButton(this, currentchan, 0, fullScreenElect);
    audio1[i]->setText("Audio1");
    audio1[i]->setSizePolicy(p);
    audio1[i]->setFont(fs);
    grid->addMultiCellWidget(audio1[i], 3, 3, (i*ndiv)+1, (i*ndiv)+4);

    /* Create the audio buttons */
    audio2[i] = new SpikeAudioButton(this, currentchan, 1, fullScreenElect);
    audio2[i]->setText("Audio2");
    audio2[i]->setSizePolicy(p);
    audio2[i]->setFont(fs);
    grid->addMultiCellWidget(audio2[i], 3, 3, (i*ndiv)+5, (i*ndiv)+8);
  }
  for(i = 0; i < NCHAN_PER_ELECTRODE * ndiv; i++, currentchan++) {
    grid->setColStretch(i, 2);
  }
  grid->setColStretch(0, 3);
  grid->setColStretch(ndiv-1, 1);
}

SpikeTetInput::~SpikeTetInput() {
}

void SpikeTetInput::mdvUpdate(int chanNum, unsigned short newVal) {
    int i;
    if (sysinfo.commonmdv) {
	/* update all channels on this machine */
	for (i = 0; i < sysinfo.nelectrodes; i++) {
	    UpdateTetrodeMaxDispVal(i, newVal);
	    dispinfo.spikeTetInput[i]->updateTetInput();
	}
    }
    else if (this->mdvSelectAll->isOn()) {
	UpdateTetrodeMaxDispVal(this->electNum, newVal);
    }
    else {
	UpdateChanMaxDispVal(this->electNum, chanNum, newVal);
    }
    updateTetInput();
    /* we need to redraw the screen to correctly display the tetrode's
     * projections */
    DrawInitialScreen();

}

void SpikeTetInput::threshUpdate(int chanNum, unsigned short newVal) {
    int i;
    if (sysinfo.commonthresh) {
	/* update all channels on this machine */
	for (i = 0; i < sysinfo.nelectrodes; i++) {
	    UpdateTetrodeThresh(i, newVal);
	    dispinfo.spikeTetInput[i]->updateTetInput();
	}
    }
    else if (this->tSelectAll->isOn()) {
	UpdateTetrodeThresh(this->electNum, newVal);
	updateTetInput();
    }
    else {
	UpdateChanThresh(this->electNum, chanNum, newVal);
	updateTetInput();
    } 
}


void SpikeTetInput::lowFiltUpdate(int chanNum, unsigned short newVal) {
    int i, dspacq = 0;
    if (sysinfo.commonfilt) {
	/* update all channels on this machine */
	if (sysinfo.dspacq) {
	    StopLocalAcq();
	    dspacq = 1;
	}
	for (i = 0; i < sysinfo.nelectrodes; i++) {
	    UpdateTetrodeLowFilt(i, newVal);
	    dispinfo.spikeTetInput[i]->updateTetInput();
	}
	if (dspacq) {
	    StartLocalAcq();
	}
    }
    else if (this->fSelectAll->isOn()) {
	UpdateTetrodeLowFilt(this->electNum, newVal);
    }
    else {
	UpdateChanLowFilt(chanNum, newVal);
    }
    updateTetInput();
}


void SpikeTetInput::highFiltUpdate(int chanNum, unsigned short newVal) {
    int i, dspacq = 0;
    if (sysinfo.commonfilt) {
	if (sysinfo.dspacq) {
	    StopLocalAcq();
	    dspacq = 1;
	}
	/* update all channels on this machine */
	for (i = 0; i < sysinfo.nelectrodes; i++) {
	    UpdateTetrodeHighFilt(i, newVal);
	    dispinfo.spikeTetInput[i]->updateTetInput();
	}
	if (dspacq) {
	    StartLocalAcq();
	}
    }
    else if (this->fSelectAll->isOn()) {
	UpdateTetrodeHighFilt(this->electNum, newVal);
    }
    else {
	UpdateChanHighFilt(chanNum, newVal);
    }
    updateTetInput();
}


void SpikeTetInput::updateTetInput(void) 
{
  int i, chnum ;
  bool enabled = 1;
  ChannelInfo *ch;
  QString s;

  chnum = electNum * NCHAN_PER_ELECTRODE;
  ch = sysinfo.channelinfo[sysinfo.machinenum] + electNum * 
    NCHAN_PER_ELECTRODE;
  /* copy the values from the channelinfo structure to this structure */
  for (i = 0; i < NCHAN_PER_ELECTRODE; i++, ch++, chnum++) {
    /* if this is a full screen electrode, we need to update the channel
     * numbers of the buttons */
    if (this->fullScreenElect) {
      maxdispval[i]->chanNum = chnum;
      thresh[i]->chanNum = chnum;
      lowfilt[i]->chanNum = chnum;
      highfilt[i]->chanNum = chnum;
    }
    s = QString("%1").arg(ch->maxdispval);
    maxdispval[i]->setText(s);
    s = QString("%1").arg(ch->thresh);
    thresh[i]->setText(s);
    this->lowfilt[i]->setValue(ch->lowfilter);
    highfilt[i]->setValue(ch->highfilter);
    if (cdspinfo.audiochan[0].dspchan == ch->dspchan) {
      //this->audio1[i]->setOn(TRUE);
      this->audio1[i]->setChecked(true);
    }
    else {
      //this->audio1[i]->setOn(FALSE);
      this->audio1[i]->setChecked(false);
    }
    if (cdspinfo.audiochan[1].dspchan == ch->dspchan) {
      //this->audio2[i]->setOn(TRUE);
      this->audio2[i]->setChecked(true);
    }
    else {
      //this->audio2[i]->setOn(FALSE);
      this->audio2[i]->setChecked(false);
    }
    //this->audio1[i]->updateButton();
    //this->audio2[i]->updateButton();
  }
  /* check to see if the file is open, and if so disable the filters */
  if (sysinfo.fileopen) {
    enabled = 0;
  }
  for (i = 0; i < NCHAN_PER_ELECTRODE; i++) {
    lowfilt[i]->setEnabled(enabled);
    highfilt[i]->setEnabled(enabled);
  }
  /* if this is a full screen electrode, we need to copy the status of the
   * buttons from the multiple tetrode window */
  if (this->fullScreenElect) {
    mdvSelectAll->setOn(dispinfo.spikeTetInput[electNum]->mdvSelectAll->isOn());
    tSelectAll->setOn(dispinfo.spikeTetInput[electNum]->tSelectAll->isOn());
    fSelectAll->setOn(dispinfo.spikeTetInput[electNum]->fSelectAll->isOn());
  }
  return;
}

void SpikeTetInput::copyValues(SpikeTetInput *src) {
    int i;
    /* copy the values from the src structure to this structure */
    for (i = 0; i < NCHAN_PER_ELECTRODE; i++) {
	if (src->mdvSelectAll->isOn()) this->mdvSelectAll->setOn(TRUE); 
	if (src->tSelectAll->isOn()) this->tSelectAll->setOn(TRUE); 
	if (src->mdvSelectAll->isOn()) this->mdvSelectAll->setOn(TRUE); 
	this->maxdispval[i]->setText(src->maxdispval[i]->text());
	this->thresh[i]->setText(src->thresh[i]->text());
	this->lowfilt[i]->setValue(src->lowfilt[i]->value());
	this->highfilt[i]->setValue(src->highfilt[i]->value());
	if (src->audio1[i]->isOn()) {
	    this->audio1[i]->setOn(TRUE);
	}
	if (src->audio1[i]->isOn()) {
	    this->audio1[i]->setOn(TRUE);
	}
	else {
	    this->audio1[i]->setOn(FALSE);
	}
	if (src->audio2[i]->isOn()) {
	    this->audio2[i]->setOn(TRUE);
	}
	else {
	    this->audio2[i]->setOn(FALSE);
	}
    }
}

SpikeTetInfo::SpikeTetInfo(QWidget *parent, int electNum, 
	bool fullScreenElect) 
	: QWidget(parent), electNum(electNum), fullScreenElect(fullScreenElect)
{
  ChannelInfo *ch;
  int currentchan;

  /* Get the appropriate channelinfo structure */
  currentchan = electNum * NCHAN_PER_ELECTRODE;
  ch = sysinfo.channelinfo[sysinfo.machinenum] + currentchan;

  /* create a grid layout for the spike waveform windows */
  QGridLayout *grid = new Q3GridLayout(this);
  QSizePolicy p2(QSizePolicy::Maximum, QSizePolicy::Maximum, FALSE);

  QFont f( "SansSerif", 8, QFont::Normal );
  QFont fs( "SansSerif", 7, QFont::Normal );
  QFont fb( "TypeWriter", 10, QFont::Normal );
  if (fullScreenElect) {
    f = QFont( "SansSerif", 16, QFont::Normal );
    fs = QFont( "SansSerif", 12, QFont::Normal );
    fb = QFont( "TypeWriter", 20, QFont::Normal );
  }

  /* create the tetrode number label */
  QString s = QString("Tetrode %1").arg((int) ch->number, 2);
  tetNumLabel = new QLabel("tetNumLabel", this, 0);
  tetNumLabel->setText(s);
  tetNumLabel->setFont(fb);
  tetNumLabel->setSizePolicy(p2);
  tetNumLabel->setAutoFillBackground(true);
  grid->addWidget(tetNumLabel, 0, 0, 1, 5);

  /* Create the Depth Label */
  depthLabel = new QLabel("Depth", this, "DepthLabel");
  depthLabel->setFont(f);
  depthLabel->setSizePolicy(p2);
  grid->addWidget(depthLabel, 0, 11, 1, 4);

  /* create the Depth input */
  depth = new SpikeLineEdit(this, currentchan);
  /* we set the depth here so that we can update it below  */
  s = QString("%1").arg(ch->depth);
  depth->setText(s);
  depth->setFont(f);
  QIntValidator *valid = new QIntValidator(0, SHRT_MAX, this, "depthValidator");
  depth->setValidator(valid);
  depth->setSizePolicy(p2);
  grid->addWidget(depth, 0, 14, 1, 2);
  /* connect the update signal to the depth update function */
  connect(depth, SIGNAL(updateVal(int, unsigned short)), this, SLOT(depthChanged(int, unsigned short)));
  depthLabel->setBuddy(depth);


  /* create the depth conversion label */ 
  depthmmLabel = new QLabel(this);
  depthmmLabel->setFont(f);
  depthmmLabel->setSizePolicy(p2);
  grid->addWidget(depthmmLabel, 0, 16, 1, 3);

  /* create the reference label*/
  refLabel = new QLabel("Ref", this, "Ref", 0);
  refLabel->setFont(f);
  refLabel->setSizePolicy(p2);
  grid->addWidget(refLabel, 1, 0, 1, 3);

  /* creat the reference spin box */
  refElect = new QSpinBox(0, sysinfo.maxelectnum, 1, this, "refspinbox");
  refElect->setSpecialValueText("Gnd");
  refElect->setFont(f);
  refElect->setValue(ch->refelect);
  refElect->setSizePolicy(p2);
  refElect->setStyle("Windows");
  connect(refElect, SIGNAL(valueChanged(int)), this, SLOT(refElectChanged(int)));
  grid->addWidget(refElect, 1, 3, 1, 4);

  /* create the reference channel label*/
  refChLabel = new QLabel("ch", this, "ch", 0);
  refChLabel->setFont(f);
  refChLabel->setSizePolicy(p2);
  grid->addWidget(refChLabel, 1, 7, 1, 1);

  /* create the reference spin box */
  /* note that the maximum value will be set in the update function */
  refChan = new QSpinBox(0, 0, 1, this, "refchspinbox");
  refChan->setFont(f);
  refChan->setValue(ch->refchan);
  refChan->setSizePolicy(p2);
  refChan->setStyle("Windows");
  connect(refChan, SIGNAL(valueChanged(int)), this,SLOT(refChanChanged(int)));
  grid->addWidget(refChan, 1, 8, 1, 1);

  /* create the overlay button */
  overlay = new QPushButton("Overlay", this, "Overlaybutton");
  overlay->setToggleButton(TRUE);
  overlay->setFont(f);
  overlay->setStyle("Windows");
  grid->addWidget(overlay, 1, 10, 1, 3); 
  connect(overlay, SIGNAL(pressed()), this, SLOT(overlayChanged()));


  /* create the full screen button  */
  if (!fullScreenElect) {
    fullScreen = new QPushButton("Full Screen", this, "fullscreenbutton");
  }
  else { 
    fullScreen = new QPushButton("Multiple Window", this, "fullscreenbutton");
  }
  /* connect the button's signal to the appropriate change page command  */
  fullScreen->setFont(f);
  fullScreen->setStyle("Windows");
  //fullScreen->setSizePolicy(p2);
  connect(fullScreen, SIGNAL(pressed()), this, SLOT(fullScreenChanged()));
  grid->addWidget(fullScreen, 1, 13, 1, 4); 

  /* create the clear projections button */
  clearProj = new QPushButton("Clear Proj.", this, "clear proj. button");
  clearProj->setFont(f);
  clearProj->setStyle("Windows");
  connect(clearProj, SIGNAL(pressed()), this, SLOT(clearProjections()));
  grid->addWidget(clearProj, 1, 17, 1, 1);

  grid->setColStretch(10, 3);
  grid->setColStretch(11, 3);

  setAutoFillBackground(true);
  setLayout(grid);
}

SpikeTetInfo::~SpikeTetInfo() {
}

void SpikeTetInfo::depthUpdate(int chanNum, unsigned short newVal) {
    ChannelInfo *ch;

    ch = sysinfo.channelinfo[sysinfo.machinenum]+chanNum;
    ch->depth = newVal;
    UpdateChannel(ch, 0);
    SetDepth(ch->number, ch->depth);
}

void SpikeTetInfo::refElectUpdate(int electrode) {
    ChannelInfo *ch;
    int chanNum;
    chanNum = this->electNum * NCHAN_PER_ELECTRODE;
    ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum; 
    if (ch->refelect != electrode) {
	this->refUpdate(electrode, 0);
    }
}

void SpikeTetInfo::refChanUpdate(int channel) {
    this->refUpdate(this->refElect->value(), channel);
}

void SpikeTetInfo::refUpdate(int electrode, int channel) 
{
  ChannelInfo *ch;
  int chanNum, i;
  int updated = 0;


  /* check to see if we are supposed to reference to ground, in which case we
   * set the channel to 0 */
  if (electrode == 0) {
    channel = 0;
  }
  /* update the internal variables */
  if (sysinfo.commonref) {
    /* if we want a common reference, we set sysinfo.commonref to 0, update 
     * values of each channels refelect and refchan, call the tetrode update routine
     * and then reset sysinfo.commonref.  This will call this routine once for 
     * each channel */
    sysinfo.commonref = 0;
    /* Stop acquisition if it is on */
    StopLocalAcq();
    ch = sysinfo.channelinfo[sysinfo.machinenum]; 
    for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++, ch++) {
      ch->refelect = electrode; 
      ch->refchan = channel;
      UpdateChannel(ch, 0);
    }
    for (i = 0; i < sysinfo.nelectrodes; i++) {
      /* We need to update the displays */
      dispinfo.spikeTetInfo[i]->updateTetInfo();
    }
    sysinfo.commonref = 1;
    StartLocalAcq();

  }
  else {
    chanNum = this->electNum * NCHAN_PER_ELECTRODE;
    ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum;
    updated = 0;
    for (i = 0; i < NCHAN_PER_ELECTRODE; i++, chanNum++, ch++) {
      if ((ch->refelect != electrode) || (ch->refchan != channel))  {
        ch->refelect = electrode;
        ch->refchan = channel;
        if (ch->refelect > 0) {
          /* get the dsp number for this reference channel */
          ch->dsprefchan = sysinfo.electinfo[ch->refelect].dspchan[ch->refchan];
        }
        else {
          /* set the channel to be 127, which will be interpreted as 
           * setting the reference to ground in spike_dsp.cpp */
          ch->dsprefchan = 127;
        }
        UpdateChannel(ch, 0);
        updated = 1;
      }
    }
    if (updated) {
      updateTetInfo();
    }
  }
}

void SpikeTetInfo::changeFullScreen(void) {
    int currentPage = dispinfo.qtab->currentPageIndex();
    if (this->fullScreenElect) {
	/* change back to the previous page  */
        dispinfo.qtab->setCurrentPage(spikeMainWindow->prevPage);
    }
    else {
	dispinfo.fullscreenelect = this->electNum;
	dispinfo.currentdispelect = dispinfo.fullscreenelect;
	dispinfo.nelectperscreen = 1;
        dispinfo.qtab->setCurrentPage(spikeMainWindow->ntabs-2);
	spikeMainWindow->prevPage = currentPage;
	spikeMainWindow->updateAllInfo();
    } 
}

void SpikeTetInfo::toggleOverlay(void) {
    dispinfo.overlay[electNum] = !dispinfo.overlay[electNum];
}



void SpikeTetInfo::copyValues(SpikeTetInfo *src) {
    /* copy the values from the src structure to this structure  */
    this->tetNumLabel->setText(src->tetNumLabel->text());
    this->depth->setText(src->depth->text());
    this->depthmmLabel->setText(src->depthmmLabel->text());
} 

void SpikeTetInfo::updateTetInfo(void) 
{
    int startchan;
    bool enabled = 1;
    ChannelInfo *ch;
    QString s;


    startchan = electNum * NCHAN_PER_ELECTRODE;
    ch = sysinfo.channelinfo[sysinfo.machinenum] + startchan;

    s = QString("%1").arg(ch->number);
    tetNumLabel->setText(s);

    s = QString("%1").arg(ch->depth);
    depth->setText(s);
    depthmmLabel->setText(QString("%1 mm").arg(ch->depth * 
		DEPTH_CONVERSION, 0, 'f', 3));

    refElect->setValue(ch->refelect);
    refChan->setValue(ch->refchan);
    
    /* set the validator for the reference channels to be the number of
     * channels on this electrode minus one (zero based)*/
    if (ch->refelect > 0) {
	refChan->setMaxValue(sysinfo.electinfo[ch->refelect].nchan-1);
    }
    else {
	refChan->setMaxValue(0);
    }

    /* if the file is open we need to disable the reference and depth selectors
     * */
    if (sysinfo.fileopen) {
	enabled = 0;
    }
    depth->setEnabled(enabled);
    refElect->setEnabled(enabled);
    refChan->setEnabled(enabled);
}



void SpikeTetInfo::clearTetProjections(void) {
    dispinfo.projdata[this->electNum].end = 
		dispinfo.projdata[this->electNum].lastdrawn = 
		dispinfo.projdata[this->electNum].point;
    dispinfo.projdata[this->electNum].endindex = 0;
    DrawInitialScreen();
}

SpikeEEGButton::SpikeEEGButton(QWidget *parent, int chanNum):
    QWidget(parent), chanNum(chanNum)
{
  ChannelInfo *ch;

  int ncols = 6;
  int nrows = 5;

  setAutoFillBackground(true);
  setStyle("Windows");

  ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum;

  QSizePolicy p(QSizePolicy::Preferred, QSizePolicy::Preferred, FALSE);
  QSizePolicy p2(QSizePolicy::Ignored, QSizePolicy::Ignored, FALSE);

  QFont fs( "TypeWriter", 8, QFont::Normal );
  QFont fvs( "TypeWriter", 6, QFont::Normal );
  QFont fl( "TypeWriter", 14, QFont::Normal );

  /* create the tetrode number label */
  QString s = QString("%1").arg((int)ch->number,2,10,QLatin1Char('0'));
  tetNumLabel = new QLabel("tetNumLabel", this);
  tetNumLabel->setText(s);
  if (sysinfo.nchannels[sysinfo.machinenum] > 64) {
    tetNumLabel->setFont(fvs);
  }
  else if (sysinfo.nchannels[sysinfo.machinenum] > 32) {
    tetNumLabel->setFont(fs);
  }
  else {
    tetNumLabel->setFont(fl);
  }
  tetNumLabel->setSizePolicy(p);

  Q3GridLayout *grid = new Q3GridLayout(this, nrows, ncols, 0, 0, "SpikeEEGButtonLayout");

  grid->addMultiCellWidget(tetNumLabel, 1, 3, 0, 1);

  /* create the button */
  chInfo = new QPushButton("test", this, "channelbutton");
  chInfo->setSizePolicy(p);
  chInfo->setStyle("Windows");

  /* add the button to the global button group */
  grid->addMultiCellWidget(chInfo, 1, 3, 2, 5);
  dispinfo.EEGButtonGroup->addButton(chInfo, chanNum);
}

SpikeEEGButton::~SpikeEEGButton() {
}


void SpikeEEGButton::updateButton(void) 
{
    ChannelInfo *ch;
    QString s;

    ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum;

    if (ch->refelect > 0) {
	s = QString("d %1 %2 mm\nR %3")
	    .arg(ch->depth)
	    .arg(ch->depth * DEPTH_CONVERSION, 0, 'f', 2)
	    .arg(ch->refelect);
    }
    else {
	s = QString("d %1 %2 mm\nR Gnd")
	    .arg(ch->depth)
	    .arg(ch->depth * DEPTH_CONVERSION, 0, 'f', 2);
    }
    chInfo->setText(tr(s));
}


SpikeEEGInfo::SpikeEEGInfo(QWidget *parent, int colNum):
    QWidget(parent), colNum(colNum)
{
  /* Set up the EEG traces for the first or second column of traces */
  int i, nrows, ncols;
  int currentchan;

  /* create a grid layout for the spike waveform windows */
  if (colNum == 0) {
    nChannels = dispinfo.neegchan1;
    currentchan = 0;
  }
  else {
    nChannels = dispinfo.neegchan2;
    currentchan = dispinfo.neegchan1;
  }
  nrows = nChannels * 3;
  ncols = 1;

  Q3GridLayout *grid = new Q3GridLayout(this, nrows, ncols, 0, 0, "SpikeEEGInfoLayout");

  QSizePolicy p2(QSizePolicy::Maximum, QSizePolicy::Ignored, FALSE);

  /* create the array of EEG Buttons */
  EEGButton = new SpikeEEGButton * [nChannels];
  for (i = 0; i < nChannels; i++, currentchan++) {
    EEGButton[i] = new SpikeEEGButton(this, currentchan);
    EEGButton[i]->setSizePolicy(p2);
    grid->addMultiCellWidget(EEGButton[i], i*3, i*3 + 2, 0, 1);
  }
}

SpikeEEGInfo::~SpikeEEGInfo() {
}

void SpikeEEGInfo::updateAll(void) 
{
    /* tell the channels to update themselves */
    int i;

    for (i = 0; i < nChannels; i++) {
	EEGButton[i]->updateButton();
    }
}


SpikeEEGDialog::SpikeEEGDialog(QWidget* parent, const char* name, bool modal, 
	Qt::WFlags fl, int chanNum)
    : QDialog( parent, name, modal, fl ), chanNum(chanNum)
{
  bool enabled; 
  ChannelInfo *ch;

 //  AudioThread *audThread = new AudioThread();
//   audThread->sound = new QAlsaSound("test", NULL);
//   audThread->start();
//   sleep(2);
//   audThread->stopSound();

  enabled = !sysinfo.fileopen;

  ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum;
  /* check to see if this is the position sync signal and if so whether we
   * should be allowed to change it */
  if (enabled && (ch->dspchan == DSP_POS_SYNC_CHAN) && 
      !sysinfo.allowsyncchanchange) {
    enabled = 0;
  }

  //Q3GridLayout *grid = new Q3GridLayout(this, nrows, ncols, 0, 0, "SpikeEEGDialogLayout");
  QGridLayout *grid = new QGridLayout;

  QSizePolicy p(QSizePolicy::Preferred, QSizePolicy::Preferred, FALSE);
  QSizePolicy p2(QSizePolicy::Maximum, QSizePolicy::Maximum, FALSE);

  QFont fs( "SansSerif", 10, QFont::Normal );
  QFont fb( "TypeWriter", 18, QFont::Normal );

  /* create the tetrode number label */
  QString s = QString("%1").arg((int)ch->number,2);
  tetNumLabel = new QLabel("tetNumLabel", this, 0);
  tetNumLabel->setText(s);
  tetNumLabel->setFont(fb);
  tetNumLabel->setSizePolicy(p);
  grid->addWidget(tetNumLabel, 0, 0);
  //grid->addMultiCellWidget(tetNumLabel, 0, 0, 0, 0);

  /* create the tetrode channel label */
  tetChanLabel = new QLabel("tetNumLabel", this, 0);
  tetChanLabel->setText("channel");
  tetChanLabel->setFont(fs);
  tetChanLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter | Qt::TextExpandTabs);
  tetChanLabel->setSizePolicy(p);
  grid->addWidget(tetChanLabel, 0, 1);
  //grid->addMultiCellWidget(tetChanLabel, 0, 0, 1, 1);

  /* create the tetrode channel number spin box */
  tetChanNum = new QSpinBox(0, sysinfo.electinfo[ch->number].nchan - 1, 1,
      this, "tetchspinbox");
  tetChanNum->setValue(ch->electchan);
  tetChanNum->setEnabled(enabled);
  connect(tetChanNum, SIGNAL(valueChanged(int)), this, 
      SLOT(tetChanChanged(int)));
  grid->addWidget(tetChanNum, 0, 2);
  //grid->addMultiCellWidget(tetChanNum, 0, 0, 2, 2);
  /* disable this if the default datatype is SPIKE */

  /* Create the Depth Label */
  depthLabel = new QLabel("Depth", this, "DepthLabel");
  depthLabel->setFont(fs);
  depthLabel->setSizePolicy(p);
  depthLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter | Qt::TextExpandTabs);
  grid->addWidget(depthLabel, 0, 5);
  //grid->addMultiCellWidget(depthLabel, 0, 0, 5, 5);

  /* create the Depth input */
  depth = new SpikeLineEdit(this, chanNum);
  /* we set the depth here so that we can update it below  */
  s = QString("%1").arg(ch->depth);
  depth->setText(s);
  depth->setFont(fs);
  depth->setEnabled(enabled);
  QIntValidator *dvalid = new QIntValidator(0, SHRT_MAX, this, 
      "depthValidator");
  depth->setValidator(dvalid);
  depth->setSizePolicy(p);
  grid->addWidget(depth, 0, 6);
  //grid->addMultiCellWidget(depth, 0, 0, 6, 6);
  /* connect the update signal to the depth update function */
  connect(depth, SIGNAL(updateVal(int, unsigned short)), this, SLOT(depthChanged(int, unsigned short)));
  /* disable this if the default datatype is SPIKE */

  /* create the depth conversion label */ 
  depthmmLabel = new QLabel(this);
  depthmmLabel->setFont(fs);
  depthmmLabel->setSizePolicy(p2);
  depthmmLabel->setText(QString("%1 mm").arg(ch->depth * 
        DEPTH_CONVERSION, 0, 'f', 3));
  grid->addWidget(depthmmLabel, 0, 7);
  //grid->addMultiCellWidget(depthmmLabel, 0, 0, 7, 7);

  /* create the reference selector button */

  /* create the reference selector */
  refSelectAll = new QPushButton("Reference", this, "ref_all");
  refSelectAll->setToggleButton(TRUE);
  refSelectAll->setSizePolicy(p);
  refSelectAll->setFont(fs);
  grid->addWidget(refSelectAll, 1, 0, 1, 4);
  //grid->addMultiCellWidget(refSelectAll, 1, 1, 0, 3);

  refElect = new QSpinBox(0, sysinfo.maxelectnum, 1, this, "refelectspinbox");
  refElect->setSpecialValueText("Gnd");
  refElect->setValue(ch->refelect);
  refElect->setEnabled(enabled);
  connect(refElect, SIGNAL(valueChanged(int)), this, 
      SLOT(refElectChanged(int)));
  grid->addWidget(refElect, 1, 4, 1, 2);
  //grid->addMultiCellWidget(refElect, 1, 1, 4, 5);
  /* disable this if the default datatype is SPIKE */

  /* create the reference channel label */
  refChanLabel = new QLabel("chan", this, "RefLabel");
  refChanLabel->setFont(fs);
  refChanLabel->setSizePolicy(p);
  grid->addWidget(refChanLabel, 1, 6);
  //grid->addMultiCellWidget(refChanLabel, 1, 1, 6, 6);

  /* create the reference channel spin box */
  refChan = new QSpinBox(0, sysinfo.electinfo[ch->refelect].nchan - 1, 1,
      this, "refchspinbox");
  refChan->setFont(fs);
  refChan->setValue(ch->refchan);
  refChan->setEnabled(enabled);
  connect(refChan, SIGNAL(valueChanged(int)), this,SLOT(refChanChanged(int)));
  grid->addWidget(refChan, 1, 7);
  //grid->addMultiCellWidget(refChan, 1, 1, 7, 7);
  /* disable this if the default datatype is SPIKE */

  /* create the push buttons that select whether all of the channels are
   * changed at once  */
  mdvSelectAll = new QPushButton("Max Disp", this, "mdv_all");
  mdvSelectAll->setToggleButton(TRUE);
  mdvSelectAll->setSizePolicy(p);
  mdvSelectAll->setFont(fs);
  fSelectAll = new QPushButton("Filters", this, "t_all");
  fSelectAll->setToggleButton(TRUE);
  fSelectAll->setSizePolicy(p);
  fSelectAll->setFont(fs);
  /* put the buttons in the grid  */
  grid->addWidget(mdvSelectAll, 2, 0, 1, 4);
  grid->addWidget(fSelectAll, 3, 0, 1, 4);
  //grid->addMultiCellWidget(mdvSelectAll, 2, 2, 0, 3);
  //grid->addMultiCellWidget(fSelectAll, 3, 3, 0, 3);

  /* create the max display value label  */
  maxdispval = new SpikeLineEdit(this, chanNum);
  maxdispval->setFont(fs);
  s = QString("%1").arg(ch->maxdispval);
  maxdispval->setText(s);
  /* set up a validator for the value */
  QIntValidator *valid = new QIntValidator(0, MAX_DISP_VAL, this, 
      "dataValValidator");
  maxdispval->setValidator(valid);
  maxdispval->setSizePolicy(p);
  /* put the label and the text window into the local layout */
  grid->addWidget(maxdispval, 2, 4, 1, 2);
  //grid->addMultiCellWidget(maxdispval, 2, 2, 4, 7);
  /* connect the update message to the update slot */
  connect(maxdispval, SIGNAL(updateVal(int, unsigned short)), this, SLOT(mdvChanged(int, unsigned short)));


  /* create the filter value label  */

  lowfilt = new SpikeFiltSpinBox(this, FALSE, chanNum);
  lowfilt->setFont(fs);
  lowfilt->setValue(ch->lowfilter);
  lowfilt->setSizePolicy(p);
  lowfilt->setEnabled(enabled);
  /* put the label and the text window into the local layout */
  grid->addWidget(lowfilt, 3, 4, 1, 2);
  //grid->addMultiCellWidget(lowfilt, 3, 3, 4, 5);
  connect(lowfilt, SIGNAL(updateVal(int, unsigned short)), this, SLOT(lowFiltChanged(int, unsigned short)));

  highfilt = new SpikeFiltSpinBox(this, TRUE, chanNum);
  highfilt->setFont(fs);
  highfilt->setValue(ch->highfilter);
  highfilt->setSizePolicy(p);
  highfilt->setEnabled(enabled);
  /* put the label and the text window into the local layout */
  grid->addWidget(highfilt, 3, 6, 1, 2);
  //grid->addMultiCellWidget(highfilt, 3, 3, 6, 7);
  connect(highfilt, SIGNAL(updateVal(int, unsigned short)), this, SLOT(highFiltChanged(int, unsigned short)));

  /* create the next and prev buttons */
  next = new QPushButton("Next", this, "NextTetrode");
  connect(next, SIGNAL( clicked() ), this, SLOT( gotoNextTet() ) );
  prev = new QPushButton("Previous", this, "NextTetrode");
  connect(prev, SIGNAL( clicked() ), this, SLOT( gotoPrevTet() ) );
  /* create a close button at the bottom */
  close = new QPushButton("Close", this, "CloseEEGDialog");
  connect( close, SIGNAL( clicked() ), this, SLOT( accept() ) );

  grid->addWidget(next, 4, 3); 
  grid->addWidget(prev, 4, 4); 
  grid->addWidget(close, 4, 5); 


  /* set things up so that we can make changes to all tetrodes if this is
   * a SPIKE machine */
  if (sysinfo.defaultdatatype == SPIKE) {
    /* set the selectors to be for all channels */
    mdvSelectAll->setOn(TRUE);
    refSelectAll->setOn(TRUE);
    fSelectAll->setOn(TRUE);
    tetChanNum->setEnabled(FALSE);
    depth->setEnabled(FALSE);
  }

  setLayout(grid);
  show();

}

SpikeEEGDialog::~SpikeEEGDialog() {
}


void SpikeEEGDialog::nextTet() {
    /* create a new Dialog for the next tetrode (if available) */
    if (this->chanNum < sysinfo.nchannels[sysinfo.machinenum] - 1) {
	new SpikeEEGDialog(dispinfo.spikeEEGInfo[0], "EEGDialog", FALSE, 0, chanNum+1);
	/* now close the current dialog */
	accept();
    }
}

void SpikeEEGDialog::prevTet() {
    /* create a new Dialog for the next tetrode (if available) */
    if (this->chanNum > 0) {
	new SpikeEEGDialog(dispinfo.spikeEEGInfo[0], "EEGDialog", FALSE, 0, chanNum-1);
	/* now close the current dialog */
	accept();
    }
}

void SpikeEEGDialog::tetChanUpdate(int newVal) {
    /* set the channel to the selected channel */
    ChannelInfo *ch;
    int		electnum;
    short	olddspchan;

    ch = sysinfo.channelinfo[sysinfo.machinenum]+chanNum;
    electnum = ch->number;
    olddspchan = ch->dspchan;
    ch->dspchan = sysinfo.electinfo[electnum].dspchan[newVal];
    ch->electchan = newVal;
    /* update this channel on the DSPS */
    UpdateChannel(ch, 0);
    return;
}


void SpikeEEGDialog::depthUpdate(int chanNum, short newVal) {
    ChannelInfo *ch;

    ch = sysinfo.channelinfo[sysinfo.machinenum]+chanNum;
    ch->depth = newVal;
    UpdateChannel(ch, 0);
    /* we also need to update the depth mm converstion */
    depthmmLabel->setText(QString("%1 mm").arg(ch->depth * 
		DEPTH_CONVERSION, 0, 'f', 3));
    SetDepth(ch->number, ch->depth);
}

void SpikeEEGDialog::refElectUpdate(int electrode) {
    /* we update the reference of the current tetrode to channel 0 of the
     * selected electrode */

    this->refUpdate(electrode, 0);
    /* we also need to update the channel number for the dialog  and the
     * maximum channel number */
    refChan->setValue(0);
    if (electrode > 0) {
	refChan->setMaxValue(sysinfo.electinfo[electrode].nchan-1);
    }
    else {
	refChan->setMaxValue(0);
    }
}

void SpikeEEGDialog::refChanUpdate(int channel) {
    /* we update the reference of the current tetrode to the selected channel 
     * of the currently selected electrode */

    ChannelInfo *ch;
    ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum;

    this->refUpdate(this->refElect->value(), channel);
}

void SpikeEEGDialog::refUpdate(int electrode, int channel) {
    ChannelInfo *ch;
    int i;

    if (this->refSelectAll->isOn()) {
	/* we need to temporarily stop ecquisition to update all channels */
	StopLocalAcq();
	for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
	    if (AllowChanChange(i)) {
		ch = sysinfo.channelinfo[sysinfo.machinenum] + i;

		/* update the internal variables */
		if ((ch->refelect != electrode) || (ch->refchan != channel)) { 
		    ch->refelect = electrode;
		    ch->refchan = channel;
		    UpdateChannel(ch, 0);
		}
	    }
	}
	StartLocalAcq();
    }
    else if (!(sysinfo.defaultdatatype & SPIKE)) {
	ch = sysinfo.channelinfo[sysinfo.machinenum] + this->chanNum;
	/* update the internal variables */
	if ((ch->refelect != electrode) || (ch->refchan != channel)) { 
	    ch->refelect = electrode;
	    ch->refchan = channel;
	    UpdateChannel(ch, 0);
	}
    }
    else {
	DisplayStatusMessage("Error: all channels must be selected to change reference");
	ch = sysinfo.channelinfo[sysinfo.machinenum] + this->chanNum;
	refElect->setValue(ch->refelect);
	refChan->setValue(ch->refchan);
    }
    dispinfo.spikeEEGInfo[0]->updateAll();
    dispinfo.spikeEEGInfo[1]->updateAll();
}


void SpikeEEGDialog::mdvUpdate(int chanNum, short newVal) {
    int i;
    ChannelInfo *ch;
    /* check the status of the update all filter button */
    if (this->mdvSelectAll->isOn()) {
	ch = sysinfo.channelinfo[sysinfo.machinenum];
	for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
	    if (AllowChanChange(i)) {
		UpdateChanMaxDispVal(i % NCHAN_PER_ELECTRODE, i, newVal);
	    }
	}

    }
    else {
	UpdateChanMaxDispVal(chanNum % NCHAN_PER_ELECTRODE, chanNum, newVal);
    }
    dispinfo.spikeEEGInfo[0]->updateAll();
    dispinfo.spikeEEGInfo[1]->updateAll();
}

void SpikeEEGDialog::lowFiltUpdate(int chanNum, short newVal) {
    int i;
    ChannelInfo *ch;
    /* check the status of the update all filter button */
    if (this->fSelectAll->isOn()) {
	/* we need to temporarily stop acquisition to update all channels */
	StopLocalAcq();
	ch = sysinfo.channelinfo[sysinfo.machinenum];
	for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
	    if (AllowChanChange(i)) {
		UpdateChanLowFilt(i, newVal);
	    }
	}
	StartLocalAcq();

    }
    else {
	UpdateChanLowFilt(chanNum, newVal);
	ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum;
    }
    dispinfo.spikeEEGInfo[0]->updateAll();
    dispinfo.spikeEEGInfo[1]->updateAll();
}

void SpikeEEGDialog::highFiltUpdate(int chanNum, short newVal) {
    /* if the mdvSelectAll button is down we update the entire electrode,
     * otherwise we only update the current channel */
    int i;
    ChannelInfo *ch;
    /* check the status of the update all filter button */
    if (this->fSelectAll->isOn()) {
	/* we need to temporarily stop acquisition to update all channels */
	StopLocalAcq();
	ch = sysinfo.channelinfo[sysinfo.machinenum];
	for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
	    if (AllowChanChange(i)) {
		UpdateChanHighFilt(i, newVal);
	    }

	}
	StartLocalAcq();
    }
    else {
	UpdateChanHighFilt(chanNum, newVal);
	ch = sysinfo.channelinfo[sysinfo.machinenum] + chanNum;
    }
    dispinfo.spikeEEGInfo[0]->updateAll();
    dispinfo.spikeEEGInfo[1]->updateAll();
}

SpikePosInfo::SpikePosInfo(QWidget* parent) : QWidget(parent)
{
    QString s;

    setAutoFillBackground(true);
    setFixedHeight(25);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setContentsMargins(0,0,0,0);

    QFont f( "SansSerif", 12, QFont::Normal );

    posThreshLabel = new QLabel("Bright Level Threshold", this, 0);
    posThreshLabel->setFont(f);
    posThreshLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    posThreshLabel->setMargin(0);
    hbox->addWidget(posThreshLabel);


    posThresh = new QSpinBox(MIN_POS_THRESH, MAX_POS_THRESH, 1, this, "posthreshspinbox");
    posThresh->setFont(f);
    posThresh->setContentsMargins(0,0,0,0);
    hbox->addWidget(posThresh);
    connect(posThresh, SIGNAL(valueChanged(int)), this, SLOT(posThreshChanged(int)));

    setLayout(hbox);
}


SpikePosInfo::~SpikePosInfo() {
}

void SpikePosInfo::posThreshChanged(int newVal) { 
  UpdatePosInfo((unsigned char) newVal);
} 

void SpikePosInfo::updatePosInfo() 
{
    /* update the values */
    this->posThresh->setValue((int)sysinfo.posthresh);
}
	    

SpikeAudio::SpikeAudio(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : QDialog( parent, name, modal, fl )
{
    
    int i, ncols = 12;

    Q3GridLayout *grid = new Q3GridLayout(this, 5, ncols, 0, 0, "spikeAudioLayout");

    QSizePolicy p(QSizePolicy::Preferred, QSizePolicy::Preferred, FALSE);
    QSizePolicy p2(QSizePolicy::Maximum, QSizePolicy::Maximum, FALSE);

    QFont f( "SansSerif", 14, QFont::Normal );
    QFont f2( "SansSerif", 12, QFont::Normal );

    label0 = new QLabel("AOutLabel0", this, 0);
    label0->setText("Analog Ouput 0");
    label0->setFont(f);
    //label0->setSizePolicy(p2);
    grid->addMultiCellWidget(label0, 0, 0, 1, 5);

    label1 = new QLabel("AOutLabel1", this, 0);
    label1->setText("Analog Ouput 1");
    label1->setFont(f);
    //label1->setSizePolicy(p2);
    grid->addMultiCellWidget(label1, 0, 0, 7, 11);

    volLabel = new QLabel("volumeLabel", this, 0);
    volLabel->setText("Volume");
    volLabel->setFont(f2);
    //volLabel->setSizePolicy(p2);
    grid->addMultiCellWidget(volLabel, 1, 1, 0, 0);

    cutoffLabel = new QLabel("cutoffLabel", this, 0);
    cutoffLabel->setText("Cutoff");
    cutoffLabel->setFont(f2);
    //cutoffLabel->setSizePolicy(p2);
    grid->addMultiCellWidget(cutoffLabel, 2, 2, 0, 0);

    delayLabel = new QLabel("delayLabel", this, 0);
    delayLabel->setText("Delay (ms)");
    delayLabel->setFont(f2);
    //delayLabel->setSizePolicy(p2);
    grid->addMultiCellWidget(delayLabel, 3, 3, 0, 0);

    /* DAC 0 */

    /* volume slider */
    gain0 = new QSlider(0, MAX_DSP_AUDIO_GAIN, 16, cdspinfo.audiogain[0], 
	    Qt::Horizontal, this, "gain0slider");
    gain0->setTickmarks(QSlider::TicksBelow );
    gain0->setTickInterval(16);
    grid->addMultiCellWidget(gain0, 1, 1, 1, 4);
    /* volume spin box */
    gain0spin = new QSpinBox(0, MAX_DSP_AUDIO_GAIN, 1, this, "gain0spinbox");
    gain0spin->setValue(cdspinfo.audiogain[0]);
    gain0spin->setFont(f2);
    grid->addMultiCellWidget(gain0spin, 1, 1, 5, 5);
    /* connections */
    connect(gain0, SIGNAL( valueChanged(int)), this, SLOT(gain0Changed(int)));
    connect(gain0, SIGNAL( valueChanged(int)), gain0spin, SLOT(setValue(int)));
    connect(gain0spin, SIGNAL( valueChanged(int)), gain0, SLOT(setValue(int)));

    /* cutoff slider */
    cutoff0 = new QSlider(0, MAX_DSP_AUDIO_CUTOFF, 256, 
	    cdspinfo.audiocutoff[0], Qt::Horizontal, this, "cutoff0slider");
    cutoff0->setTickmarks(QSlider::TicksBelow );
    cutoff0->setTickInterval(2048);
    grid->addMultiCellWidget(cutoff0, 2, 2, 1, 4);
    /* cutoff spin box */
    cutoff0spin = new QSpinBox(0, MAX_DSP_AUDIO_CUTOFF, 1, this, 
	    "cutoff0spinbox");
    cutoff0spin->setValue(cdspinfo.audiocutoff[0]);
    cutoff0spin->setFont(f2);
    grid->addMultiCellWidget(cutoff0spin, 2, 2, 5, 5);
    /* connections */
    connect(cutoff0, SIGNAL( valueChanged(int)), this, 
	    SLOT(cutoff0Changed(int)));
    connect(cutoff0, SIGNAL( valueChanged(int)), cutoff0spin, 
	    SLOT(setValue(int)));
    connect(cutoff0spin, SIGNAL( valueChanged(int)), cutoff0, 
	    SLOT(setValue(int)));


    /* delay slider */
    delay0 = new QSlider(0, MAX_DSP_AUDIO_DELAY, 50, cdspinfo.audiodelay[0], Qt::Horizontal, 
	    this, "delay0slider");
    delay0->setTickmarks(QSlider::TicksBelow );
    delay0->setTickInterval(100);
    grid->addMultiCellWidget(delay0, 3, 3, 1, 4);
    /* delay spin box */
    delay0spin = new QSpinBox(0, MAX_DSP_AUDIO_DELAY, 1, this, 
	    "delay0spinbox");
    delay0spin->setValue(cdspinfo.audiodelay[0]);
    delay0spin->setFont(f2);
    grid->addMultiCellWidget(delay0spin, 3, 3, 5, 5);
    /* connections */
    connect(delay0, SIGNAL( valueChanged(int)), this, 
	    SLOT(delay0Changed(int)));
    connect(delay0, SIGNAL( valueChanged(int)), delay0spin, 
	    SLOT(setValue(int)));
    connect(delay0spin, SIGNAL( valueChanged(int)), delay0, 
	    SLOT(setValue(int)));


    /* DAC 1 */

    /* volume slider */
    gain1 = new QSlider(0, MAX_DSP_AUDIO_GAIN, 16, cdspinfo.audiogain[1], 
	    Qt::Horizontal, this, "gain1slider");
    gain1->setTickmarks(QSlider::TicksBelow );
    gain1->setTickInterval(16);
    grid->addMultiCellWidget(gain1, 1, 1, 7, 10);
    /* volume spin box */
    gain1spin = new QSpinBox(0, MAX_DSP_AUDIO_GAIN, 1, this, "gain1spinbox");
    gain1spin->setValue(cdspinfo.audiogain[1]);
    gain1spin->setFont(f2);
    grid->addMultiCellWidget(gain1spin, 1, 1, 11, 11);
    /* connections */
    connect(gain1, SIGNAL( valueChanged(int)), this, SLOT(gain1Changed(int)));
    connect(gain1, SIGNAL( valueChanged(int)), gain1spin, SLOT(setValue(int)));
    connect(gain1spin, SIGNAL( valueChanged(int)), gain1, SLOT(setValue(int)));

    /* cutoff slider */
    cutoff1 = new QSlider(0, MAX_DSP_AUDIO_CUTOFF, 256, 
	    cdspinfo.audiocutoff[1], Qt::Horizontal, this, "cutoff1slider");
    cutoff1->setTickmarks(QSlider::TicksBelow );
    cutoff1->setTickInterval(2048);
    grid->addMultiCellWidget(cutoff1, 2, 2, 7, 10);
    /* cutoff spin box */
    cutoff1spin = new QSpinBox(0, MAX_DSP_AUDIO_CUTOFF, 1, this, 
	    "cutoff1spinbox");
    cutoff1spin->setValue(cdspinfo.audiocutoff[1]);
    cutoff1spin->setFont(f2);
    grid->addMultiCellWidget(cutoff1spin, 2, 2, 11, 11);
    /* connections */
    connect(cutoff1, SIGNAL( valueChanged(int)), this, 
	    SLOT(cutoff1Changed(int)));
    connect(cutoff1, SIGNAL( valueChanged(int)), cutoff1spin, 
	    SLOT(setValue(int)));
    connect(cutoff1spin, SIGNAL( valueChanged(int)), cutoff1, 
	    SLOT(setValue(int)));


    /* delay slider */
    delay1 = new QSlider(0, MAX_DSP_AUDIO_DELAY, 50, cdspinfo.audiodelay[1], Qt::Horizontal, 
	    this, "delay1slider");
    delay1->setTickmarks(QSlider::TicksBelow );
    delay1->setTickInterval(100);
    grid->addMultiCellWidget(delay1, 3, 3, 7, 10);
    /* delay spin box */
    delay1spin = new QSpinBox(0, MAX_DSP_AUDIO_DELAY, 1, this, 
	    "delay1spinbox");
    delay1spin->setValue(cdspinfo.audiodelay[1]);
    delay1spin->setFont(f2);
    grid->addMultiCellWidget(delay1spin, 3, 3, 11, 11);
    /* connections */
    connect(delay1, SIGNAL( valueChanged(int)), this, 
	    SLOT(delay1Changed(int)));
    connect(delay1, SIGNAL( valueChanged(int)), delay1spin, 
	    SLOT(setValue(int)));
    connect(delay1spin, SIGNAL( valueChanged(int)), delay1, 
	    SLOT(setValue(int)));


    linked = new QRadioButton("Linked", this, "Linked");
    connect(linked, SIGNAL(toggled(bool) ), this, SLOT(audioLinked(bool)) );
    linked->setFont(f2);
    grid->addMultiCellWidget(linked, 4, 4, 7, 7);


    mute = new QPushButton("Mute", this, "Mute");
    mute->setToggleButton(TRUE);
    mute->setOn((bool)cdspinfo.mute);
    connect(mute, SIGNAL( pressed() ), this, SLOT(audioMute()) );
    mute->setFont(f2);
    grid->addMultiCellWidget(mute, 4, 4, 8, 9);

    close = new QPushButton("Close", this, "CloseAudioDialog");
    close->setFont(f2);
    connect( close, SIGNAL( clicked() ), this, SLOT( accept() ) );
    grid->addMultiCellWidget(close, 4, 4, 10, 11);

    for (i = 0; i < ncols; i++) {
	grid->setColStretch(i, 0);
	grid->setColSpacing(i, 40);
    }

    show();
}


SpikeAudio::~SpikeAudio() {
}

void SpikeAudio::audioMute(void) { 
  DSPMuteAudio(!mute->isOn()); 
}


void SpikeAudio::linkAudio(bool linked) {
    if (linked) {
	/* set the value of the gain, cutoff and delay for audio channel 1 
	 * equal to that of audio channel 0 */
	gain1->setValue(gain0->value());
	cutoff1->setValue(cutoff0->value());
	delay1->setValue(delay0->value());
    }
}

void SpikeAudio::changeGain(int chan, int newVal) {
    SetDSPAudioGain(chan, (short) newVal);
    if (linked->isChecked()) {
	/* update the channel that wasn't selected */
	if (chan == 0) {
	    gain1->setValue(newVal);
	}
	else {
	    gain0->setValue(newVal);
	}
    }
}

void SpikeAudio::changeCutoff(int chan, int newVal) {
    SetDSPAudioCutoff(chan, (short) newVal);
    if (linked->isChecked()) {
	/* update the channel that wasn't selected */
	if (chan == 0) {
	    cutoff1->setValue(newVal);
	}
	else {
	    cutoff0->setValue(newVal);
	}
    }
}


void SpikeAudio::changeDelay(int chan, int newVal) {
    SetDSPAudioDelay(chan, (short) newVal);
    if (linked->isChecked()) {
	if (chan == 0) {
	    delay1->setValue(newVal);
	}
	else {
	    delay0->setValue(newVal);
	}
    }
    else {
    }
}
