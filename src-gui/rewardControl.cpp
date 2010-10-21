/* spikeMainWindow.cpp: the main qt object for nspike's display
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "spikecommon.h"
#include "rewardControl.h"
#include "spikeGLPane.h"
#include "spike_main.h"


#include <string>
#include <sstream>

extern DisplayInfo dispinfo;
extern SysInfo sysinfo;
extern NetworkInfo netinfo;
extern DigIOInfo digioinfo;
extern UserDataInfo userdatainfo;


/* Reward control gui */
void rewardControl::reject() {
  emit finished();
  QDialog::reject();
}

rewardControl::rewardControl(QWidget* parent, const char* name, bool modal, 
	Qt::WFlags fl) : QDialog( parent, name, modal, fl)
{
    this->setCaption("Reward Control");

    Q3GridLayout *mainGrid = new Q3GridLayout(this, 10, 10, 0, 0, "maingrid");

    // Create a multitabbed window
    qtab = new QTabWidget(this, 0, 0);
    this->ntabs = 3;
    QWidget *w = new QWidget(this, 0, 0);
    qtab->setTabPosition(QTabWidget::South);

    QRect r(0, 0, 600, 400);
    this->setGeometry(r);

    /* initialze the random number generator */
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    srand48((long) tv.tv_sec);


    /* Tab 1 */
    w = new QWidget(qtab, "Well Information");


    Q3GridLayout *grid0 = new Q3GridLayout(w, 4, 2, 0, 0, "grid 0");

    /* I don't know if the following is necessary */
    QSizePolicy es(QSizePolicy::Expanding, QSizePolicy::Expanding, 1, 1);
    w->setSizePolicy(es); 

    fileLoad = new QPushButton(QString("Load File"), w, "load button");
    connect(fileLoad, SIGNAL(clicked(void)), this, SLOT(load(void)));
    grid0->addMultiCellWidget(fileLoad, 0, 0, 0, 0);
    fileSave = new QPushButton(QString("Save File"), w, "save button");
    connect(fileSave, SIGNAL(clicked(void)), this, SLOT(save(void)));
    grid0->addMultiCellWidget(fileSave, 0, 0, 1, 1);

    nWellsLabel = new QLabel("Number of Wells", w, "nwells label", 0);
    grid0->addMultiCellWidget(nWellsLabel, 1, 1, 0, 0);
    nWells = new QSpinBox(1, MAX_WELLS, 1, w, "Number of Wells");
    grid0->addMultiCellWidget(nWells, 1, 1, 1, 1);
    nWells->setValue(3);

    /* when the create tabs button is pressed we recreate the second and third
     * tabs with the appropriate number of wells */
    createTabsButton = new QPushButton("Update Logic / Status", w, "CreateTabsButton");
    connect(createTabsButton, SIGNAL( clicked() ), this, SLOT( createTabs() ) );
    grid0->addMultiCellWidget(createTabsButton, 2, 2, 1, 1);

    close = new QPushButton("Close", w, "CloseDialog");
    connect( close, SIGNAL( clicked() ), this, SLOT( dialogClosed() ) );
    grid0->addMultiCellWidget(close, 3, 3, 1, 1);

    tablabel = QString("Well Info"); 
    qtab->addTab(w, tablabel); 

    mainGrid->addMultiCellWidget(qtab, 0, 9, 0, 9);
    show();
    /* set the number of wells to 3 to create the second set of tabs */
}


rewardControl::~rewardControl() 
{
}

void rewardControl::dialogClosed()
{
    emit finished();
    accept();
}

void rewardControl::loadFile(void) 
{
    QString fileName = Q3FileDialog::getOpenFileName(".",
                    "All Files (*)",
                    this,
                    "Load Reward Config",
                    "Choose a file to load" );
    if (!fileName.isEmpty()) {
	readRewardConfig(fileName);
    }
}

void rewardControl::saveFile(void) 
{
  Q3FileDialog *fd = new Q3FileDialog(this, "Save Reward Configuration", 
      TRUE );
  fd->setMode( Q3FileDialog::AnyFile );
  QString fileName;
  if ( fd->exec() == QDialog::Accepted ) {
    fileName = fd->selectedFile();
    /* make sure the user wants to overwrite the file if it exits*/
    if (QFile::exists(fileName)  && QMessageBox::question(
          this,
          tr("Overwrite File?"),
          tr("A file called %1 already exists."
            "Do you want to overwrite it?")
          .arg(fileName),
          tr("&Yes"), tr("&No"),
          QString::null, 1, 1 ) ) {
      return;
    }
    else {
      /* write out the configuration to the file */
      writeRewardConfig(fileName);
    }
  }
}

void rewardControl::writeRewardConfig(QString fileName) 
{
  QFile file(fileName);
  int i, j;
  if (file.open(QIODevice::WriteOnly) ) {
    Q3TextStream stream(&file);
	/* number of wells */
	stream << QString("nWells\t\t%1\n").arg(nWells->value());

	/* Well Information */
	for (i = 0; i < nWells->value(); i++) {
	    stream << QString("well\t%1\t\n").arg(i);
	    stream << QString("\tprev\t");
	    for (j = 0; j < nWells->value(); j++) {
		stream << QString("%1  ").arg(prev[i]->isSelected(j));
	    }
	    stream << QString("\n");
	    stream << QString("\tcurr\t");
	    for (j = 0; j < nWells->value(); j++) {
		stream << QString("%1  ").arg(curr[i]->isSelected(j));
	    }
	    stream << QString("\n");
	    stream << QString("\tinputBit\t%1\t\n").arg(inputBit[i]->value());
	    stream << QString("\ttriggerHigh\t%1\t\n").arg(triggerHigh[i]->isOn());
	    stream << QString("\toutputBit\t%1\t\n").arg(outputBit[i]->value());
	    stream << QString("\trewardLength\t%1\t\n").arg(rewardLength[i]->value());
	    stream << QString("\trewardPercent\t%1\t\n").arg(rewardPercent[i]->value());
	}
	stream << QString("firstReward\t%1\t\n").arg(firstRewardWell->value());
        file.close();
    }
    else {
	fprintf(stderr, "Unable to write to file %s\n", fileName.ascii());
    }
}


void rewardControl::readRewardConfig(QString fileName) 
{
    QFile file(fileName);
    int i, well, t1;
    int nwells;
    const char delim[] = " "; 

    QString s;
    const char *str;
    char *tmpstr;
    if (file.open(QIODevice::ReadOnly) ) {
        Q3TextStream stream(&file);
	while (!stream.atEnd()) {
	    s = stream.readLine();
       fprintf(stderr,"%s\n",s.ascii());
	    /* Parse the string */
	    /* Skip comments */
	    if (s.contains("%")) {
		continue;
	    }
	    str = s.ascii();
	    /* skip spaces */
	    while (isspace(*str) && (*str != '\0')) str++;
	    if (strncmp(str, "nWells", 6) == 0) {
		str += 6;
		sscanf(str, "%d", &nwells);
		nWells->setValue(nwells);
      createTabs();
	    }
	    else if (strncmp(str, "well", 4) == 0) {
		str += 4;
		sscanf(str, "%d", &well);
		/* create the prev and curr list */
		createRewardListBox(prev[well]);
		createRewardListBox(curr[well]);
	    }
	    else if (strncmp(str, "prev", 4) == 0) {
		tmpstr = (char *) str + 4;
		tmpstr = strtok(tmpstr, delim);
		/* read in the list of previous wells */
		for (i = 0; i < nwells; i++) {
		    sscanf(tmpstr, "%d", &t1);
		    prev[well]->setSelected(i, (bool) t1);
		    tmpstr = strtok(NULL, delim);
		}
	    }
	    else if (strncmp(str, "curr", 4) == 0) {
		tmpstr = (char *) str + 4;
		/* read in the list of current wells */
		tmpstr = strtok(tmpstr, delim);
		for (i = 0; i < nwells; i++) {
		    sscanf(tmpstr, "%d", &t1);
		    curr[well]->setSelected(i, (bool) t1);
		    tmpstr = strtok(NULL, delim);
		}
	    }
	    else if (strncmp(str, "inputBit", 8) == 0) {
		str += 8;
		sscanf(str, "%d", &t1);
		inputBit[well]->setValue(t1);
	    }
	    else if (strncmp(str, "triggerHigh", 11) == 0) {
		str += 11;
		sscanf(str, "%d", &t1);
		triggerHigh[well]->setChecked((bool) t1);
	    }
	    else if (strncmp(str, "outputBit", 9) == 0) {
		str += 9;
		sscanf(str, "%d", &t1);
		outputBit[well]->setValue(t1);
	    }
	    else if (strncmp(str, "rewardLength", 12) == 0) {
		str += 12;
		sscanf(str, "%d", &t1);
		rewardLength[well]->setValue(t1);
	    }
	    else if (strncmp(str, "rewardPercent", 13) == 0) {
		str += 13;
		sscanf(str, "%d", &t1);
		rewardPercent[well]->setValue(t1);
	    }
	    else if (strncmp(str, "firstReward", 11) == 0) {
		str += 11;
		sscanf(str, "%d", &t1);
		firstRewardWell->setValue(t1);
	    }
	}
    }
    else {
	fprintf(stderr, "Unable to read file %s\n", fileName.ascii());
    }
}


void rewardControl::createLogicTab(int n) 
{
    int i, col;
    /* create the logic tab with the correct number of wells. We first get rid
     * of the current tab if it exists */
    /* see if the first page has been created */
    QWidget *w = qtab->page(1);
    if (w) {
	qtab->removePage(w);
	delete w;
    }

    prevWell = -1;

    w = new QWidget(qtab, 0, 0);
    QString tablabel2 = QString("Logic"); 
    qtab->insertTab(w, tablabel2, 1); 
    Q3GridLayout *grid1 = new Q3GridLayout(w, 8, n+1, 0, 0, "grid 1");

    /* I don't know if the following is necessary */
    QSizePolicy es(QSizePolicy::Expanding, QSizePolicy::Expanding, 1, 1);
    w->setSizePolicy(es); 

    /* create the well labels and the logic */
    prevLabel = new QLabel(QString("Previous Well"), w, "prev label", 0);
    grid1->addMultiCellWidget(prevLabel, 1, 1, 0, 0);
    currLabel = new QLabel(QString("Current Well"), w, "curr label", 0);
    grid1->addMultiCellWidget(currLabel, 2, 2, 0, 0);
    inputBitLabel = new QLabel(QString("Input Bit"), w, "input label", 0);
    grid1->addMultiCellWidget(inputBitLabel, 3, 3, 0, 0);
    outputBitLabel = new QLabel(QString("Output Bit"), w, "output label", 0);
    grid1->addMultiCellWidget(outputBitLabel, 5, 5, 0, 0);
    rewardLengthLabel = new QLabel(QString("Reward Length (100 usec)"), w, 
	    "reward len label", 0);
    grid1->addMultiCellWidget(rewardLengthLabel, 6, 6, 0, 0);
    rewardPercentLabel = new QLabel(QString("Reward Percentage"), w, 
	    "reward percentage label", 0);
    grid1->addMultiCellWidget(rewardPercentLabel, 7, 7, 0, 0);

    wellLabel = new QLabel* [n];
    prev = new Q3ListBox* [n];
    curr = new Q3ListBox* [n];
    outputBit = new QSpinBox* [n];
    inputBit = new QSpinBox* [n];
    triggerHigh = new QRadioButton* [n];
    rewardLength = new QSpinBox* [n];
    rewardPercent = new QSpinBox* [n];
    for (i = 0; i < n; i++) {
	col = i+1;
	wellLabel[i] = new QLabel(QString("Reward Well %1").arg(i), w, 
		"well label", 0);
	wellLabel[i]->setAlignment(Qt::AlignHCenter);
	grid1->addMultiCellWidget(wellLabel[i], 0, 0, col, col);
	prev[i] = new Q3ListBox(w, "prev list box", 0);
	createRewardListBox(prev[i]);
	grid1->addMultiCellWidget(prev[i], 1, 1, col, col);
	curr[i] = new Q3ListBox(w, "curr list box", 0);
	createRewardListBox(curr[i]);
	grid1->addMultiCellWidget(curr[i], 2, 2, col, col);

	inputBit[i] = new QSpinBox(0, MAX_BITS, 1, w, "Input Bit");
	grid1->addMultiCellWidget(inputBit[i], 3, 3, col, col);
	triggerHigh[i] = new QRadioButton("Trigger High", w, 0);
  triggerHigh[i]->setAutoExclusive(false);
	grid1->addMultiCellWidget(triggerHigh[i], 4, 4, col, col);
	outputBit[i] = new QSpinBox(0, MAX_BITS, 1, w, "Output Bit");
	grid1->addMultiCellWidget(outputBit[i], 5, 5, col, col);
	rewardLength[i] = new QSpinBox(0, 30000, 1, w, "Reward Length");
	grid1->addMultiCellWidget(rewardLength[i], 6, 6, col, col);
	rewardPercent[i] = new QSpinBox(0, 100, 1, w, "Reward Prob");
	rewardPercent[i]->setValue(100);
	grid1->addMultiCellWidget(rewardPercent[i], 7, 7, col, col);
    }
}


void rewardControl::createRewardListBox(Q3ListBox *lb)
{
    int i;

    QString s;
    QStringList l;

    lb->clear();
    s.truncate(0);
    for (i = 0; i < nWells->value(); i++) {
      l << QString("%1").arg(i);
    }
    lb->insertStringList(l, -1);
    /* allow multiple selections */
    lb->setSelectionMode(Q3ListBox::Multi);
}
			

void rewardControl::createStatusTab(int n) 
{
  int i;

  /* create the logic tab with the correct number of wells. We first get rid
   * of the current tab if it exists */
  QWidget *w = qtab->page(2);
  if (w) {
    qtab->removePage(w);
    delete w;
  }
  w = new QWidget(qtab, "Status");
  /* create the status tab with the correct number of wells */
  QString tablabel2 = QString("Status"); 
  qtab->insertTab(w, tablabel2, 2); 
  Q3GridLayout *grid2 = new Q3GridLayout(w, 6, MIN(n,3), 0, 0, "grid 2");

  /* I don't know if the following is necessary */
  QSizePolicy es(QSizePolicy::Expanding, QSizePolicy::Expanding, 1, 1);
  w->setSizePolicy(es); 

  /* create the well labels and the logic  */
  firstTrial = new QRadioButton("First Trial", w, 0);
  firstTrial->setChecked(true);
  grid2->addMultiCellWidget(firstTrial, 1, 1, 0, 0);
  firstRewardLabel = new QLabel("First Reward Well", w, "first reward label",
      0);
  grid2->addMultiCellWidget(firstRewardLabel, 1, 1, 1, 1); 
  firstRewardWell = new QSpinBox(0, nWells->value()-1, 1, w, 
      "first reward well");
  grid2->addMultiCellWidget(firstRewardWell, 1, 1, 2, 2);
  connect(firstRewardWell, SIGNAL(valueChanged(int)), this, 
      SLOT(setFirstReward(int)));


  /* create the well labels and the logic */
  reward = new QPushButton *[n];
  rewardButtonGroup = new Q3ButtonGroup(this);
  next = new QRadioButton *[n];
  status = new QLabel *[n];
  rewardCounter.resize(n);
  for (i = 0; i < n; i++) {
    reward[i] = new QPushButton(QString("Well %1").arg(i), w, 
        "reward button");
    grid2->addMultiCellWidget(reward[i], 2, 2, i, i);
    rewardButtonGroup->insert(reward[i], i);
    next[i] = new QRadioButton("Next", w, 0);
    grid2->addMultiCellWidget(next[i], 3, 3, i, i);
    status[i] = new QLabel("status", w, 0);
    status[i]->setAlignment(Qt::AlignHCenter);
    grid2->addMultiCellWidget(status[i], 4, 4, i, i);

    rewardCounter[i] = 0;
  }
  connect(rewardButtonGroup, SIGNAL(clicked(int)), this, 
      SLOT(setReward(int)));
  nextReward = new QPushButton("Next Reward", w, "next reward button");
  grid2->addMultiCellWidget(nextReward, 5, 5, n-1, n-1);
  connect(nextReward, SIGNAL(clicked()), this, SLOT(setNextReward()));

  QPushButton *resetRewardCounterButton = new QPushButton("Reset Counters", w, "reset counters button");
  grid2->addMultiCellWidget(resetRewardCounterButton, 5, 5, 0, 0);
  connect(resetRewardCounterButton, SIGNAL(clicked()), this, SLOT(resetRewardCounters()));

  /* assume well 0 unless W track */
  if (n != 3) {
    firstRewardWell->setValue(0);
  }
  else {
    /* assume W track, so well 1 is the first to be rewarded */
    firstRewardWell->setValue(1);
  }
}

void rewardControl::setStatus()
{
    /* set the status labels correctly */
    int i, nNext = 0;

    for (i = 0; i < nWells->value(); i++) {
	if (next[i]->isChecked()) {
      status[i]->setText(QString("\n %1").arg(rewardCounter[i]));
	    nNext++;
	}
	else if (prevWell == i) {
	    if (prevRewarded) {
    rewardCounter[i]++;
    status[i]->setText(QString("Rewarded\n %1").arg(rewardCounter[i]));
	    }
	    else {
		status[i]->setText("Reward Omitted");
	    }
	} 
	else {
      status[i]->setText(QString("\n %1").arg(rewardCounter[i]));
	}
    }
    if (nNext == 1) {
	nextReward->setEnabled(true); 
    }
    else {
	nextReward->setEnabled(false); 
    }
}

void rewardControl::resetRewardCounters()
{
  /* reset the reward counters */

  for (int i = 0; i < nWells->value(); i++) {
    rewardCounter[i] = 0;
    status[i]->setText(QString("\n %1").arg(rewardCounter[i]));
  }
}


void rewardControl::rewardWell(int well, bool reward) 
{
    int i;
    /* put a reward at the well if we should and use the logic to update the nextwell
     * variable */

    if (well < 0) {
        return;
    }
    //if (reward && (well >= 0)) {
    if (reward) {
	/* set the reward length */
	digioinfo.rewardlength[outputBit[well]->value()] = 
		rewardLength[well]->value();
        
	/* use the random number generator to determine whether we issue a
	 * reward */
	if ((drand48() * 100) <=  rewardPercent[well]->value()) {
	    TriggerOutput(outputBit[well]->value());
	    prevRewarded = 1;
	}
	else {
	    prevRewarded = 0;
	}
	/* this is the end of the first trial */
	firstTrial->setChecked(false);
    }

    for (i = 0; i < nWells->value(); i++) {
	/* we can't reward the same well twice, so we skip the current well's
	 * entry */
	if (i != well) {
	    if (((prevWell == -1) || prev[i]->isSelected(prevWell)) &&
		curr[i]->isSelected(well)) {
		/* we reward this well next. Note that this will update wellStatus*/
		next[i]->setChecked(true);
		fprintf(stderr, "next[%d] true\n",  i);
	    }
	    else {
		next[i]->setChecked(false);
		fprintf(stderr, "next[%d] false\n", i);
	    }
	}
    }
    next[well]->setChecked(false);
    prevWell = well;
    this->setStatus();
}


int rewardControl::getNextWell(void) 
{
    int i;

    for (i = 0; i < nWells->value(); i++) {
	if (next[i]->isChecked()) {
	    return i;
	}
    }
    return -1;
}



     
void rewardControl::DIOInput(DIOBuffer *diobuf)
{
    int i, port, bit;
    /* check to see if the next well's bits have been set, update the status
     * and deliver reward if appropriate */
    for (i = 0; i < nWells->value(); i++) {
	port = inputBit[i]->value() / 16;
	bit = 1 << (inputBit[i]->value() % 16);
	if ((triggerHigh[i]->isChecked() && (diobuf->status[port] & bit)) || 
	    ((!triggerHigh[i]->isChecked() && !(diobuf->status[port] & bit)))) {
	    /* we ignore this if the animal was just rewarded at this well*/
	    if (prevWell != i) {
		if (next[i]->isChecked()) {
		    rewardWell(i, true);
		    break;
		}
		else {
		    /* this is an error */
		    rewardWell(i, false);
		    break;
		}
	    }
	}
    }
}


void setRewardsDialog::reject() {
  emit finished();
  QDialog::reject();
}

setRewardsDialog::setRewardsDialog(QWidget* parent, 
	const char* name, bool modal, Qt::WFlags fl, int port)
    : QDialog( parent, name, modal, fl), port(port)
{

    int ncols = 7;
    int nrows = BITS_PER_PORT + 1;
    int currentbit;
    int i;
    bool enabled;

    QString s;

    Q3GridLayout *grid = new Q3GridLayout(this, nrows, ncols, 0, 0, "CalibrateRewardsDialogLayout");

    QSizePolicy p(QSizePolicy::Preferred, QSizePolicy::Preferred, FALSE);
    QSizePolicy p2(QSizePolicy::Maximum, QSizePolicy::Maximum, FALSE);

    QFont fs( "SansSerif", 10, QFont::Normal );

    outputNumberLabel = new QLabel *[BITS_PER_PORT];
    outputLengthLabel = new QLabel *[BITS_PER_PORT];
    outputLength = new SpikeLineEdit *[BITS_PER_PORT];
    pulse = new QPushButton *[BITS_PER_PORT];
    change = new QRadioButton *[BITS_PER_PORT];

    pulseButtonGroup = new Q3ButtonGroup(this);
    changeButtonGroup = new QButtonGroup(this);

    /* set the changeButtonGroup to be non exclusive */
    changeButtonGroup->setExclusive(false);

    enabled = false;
    /* if this an output port enable it */
    if (digioinfo.porttype[port] == OUTPUT_PORT) {
	enabled = true;
    }

    currentbit = port * BITS_PER_PORT;
    for (i = 0; i < BITS_PER_PORT; i++, currentbit++) {
	/* create the output number label */
	s = QString("Bit %1").arg(currentbit);
	outputNumberLabel[i] = new QLabel("outputNumberLabel", this, 0);
	outputNumberLabel[i]->setText(s);
	outputNumberLabel[i]->setFont(fs);
	outputNumberLabel[i]->setSizePolicy(p);
	grid->addMultiCellWidget(outputNumberLabel[i], i, i, 0, 1);

	/* create the output length label */
	s = QString("Output Length (100 usec units)");
	outputLengthLabel[i] = new QLabel("outputLengthLabel", this, 0);
	outputLengthLabel[i]->setText(s);
	outputLengthLabel[i]->setFont(fs);
	outputLengthLabel[i]->setSizePolicy(p);
	grid->addMultiCellWidget(outputLengthLabel[i], i, i, 2, 2);

	/* create the outputLength input */
	outputLength[i] = new SpikeLineEdit(this, currentbit);
	/* we set the depth here so that we can update it below  */
	s = QString("%1").arg(digioinfo.rewardlength[currentbit]);
	outputLength[i]->setText(s);
	outputLength[i]->setFont(fs);
	QIntValidator *valid = new QIntValidator(0, MAX_OUTPUT_PULSE_LENGTH, 
		this, "outputLengthValidator");
	outputLength[i]->setValidator(valid);
	grid->addMultiCellWidget(outputLength[i], i, i, 3, 3);
	/* connect the update signal to the depth update function */
	connect(outputLength[i], SIGNAL(updateVal(int, unsigned short)), this, 
		SLOT(changePulseLength(int, unsigned short)));

	/* create the test button */
	pulse[i] = new QPushButton("Pulse", this, "pulse");
	pulse[i]->setSizePolicy(p);
	pulse[i]->setFont(fs);
	grid->addMultiCellWidget(pulse[i], i, i, 4, 4);
	pulseButtonGroup->insert(pulse[i], i);

	/* create the raise radio */
	change[i] = new QRadioButton("  Raise", this, "raise");
	change[i]->setSizePolicy(p);
	change[i]->setFont(fs);
	grid->addMultiCellWidget(change[i], i, i, 6, 6);
	changeButtonGroup->addButton(change[i], i);

	/* check to see if we need to set this button to be down */
	if (digioinfo.raised[currentbit]) {
	    change[i]->setChecked(true);
	}
	/* disable this line if this is an input port */
	outputLength[i]->setEnabled(enabled);
	pulse[i]->setEnabled(enabled);
	change[i]->setEnabled(enabled);
    }
    connect(pulseButtonGroup, SIGNAL(clicked(int)), this, SLOT(pulseBit(int)));
    connect(changeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(changeBit(int)));


    /* create the next and prev buttons */
    next = new QPushButton("Next Port", this, "NextPort");
    connect(next, SIGNAL( clicked() ), this, SLOT( gotoNextPort()));
    grid->addMultiCellWidget(next, nrows-1, nrows-1, 1, 1, 0); 
    prev = new QPushButton("Previous Port", this, "NextPort");
    connect(prev, SIGNAL( clicked() ), this, SLOT( gotoPrevPort()));
    grid->addMultiCellWidget(prev, nrows-1, nrows-1, 2, 2, 0); 

    /* create a close button at the bottom */
    close = new QPushButton("Close", this, "CloseDialog");
    connect( close, SIGNAL( clicked() ), this, SLOT( dialogClosed() ) );
    grid->addMultiCellWidget(close, nrows-1, nrows-1, ncols-1, ncols-1, 0); 

    setAttribute(Qt::WA_DeleteOnClose,true);
    show();
}

	
setRewardsDialog::~setRewardsDialog() 
{
}

void setRewardsDialog::dialogClosed()
{
    emit finished();
    accept();
}


void setRewardsDialog::nextPort() {
    /* create a new Dialog for the next tetrode (if available) */
    if (this->port < NUM_PORTS - 1) {
	new setRewardsDialog(dispinfo.spikeInfo, "rewarddialog", FALSE, 
		0, this->port+1);
	/* now close the current dialog */
	accept();
    }
}

void setRewardsDialog::prevPort() {
    /* create a new Dialog for the next tetrode (if available) */
    if (this->port > 0) {
	new setRewardsDialog(dispinfo.spikeInfo, "rewarddialog", FALSE, 
		0, this->port-1);
	/* now close the current dialog */
	accept();
    }
}

