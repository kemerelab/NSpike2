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
#include "spikeMainWindow.h"
#include "spike_main.h"
#include "spikeGLPane.h"

#include <q3listbox.h>
#include <QKeyEvent>
#include <Q3GridLayout>
#include <Q3PopupMenu>
#include <q3buttongroup.h>
#include <q3filedialog.h>
#include <Q3TextStream>
#include <Q3HBoxLayout>
#include <Q3BoxLayout>
#include <string>
#include <sstream>

extern DisplayInfo dispinfo;
extern SysInfo sysinfo;
extern NetworkInfo netinfo;
extern DigIOInfo digioinfo;
extern FSDataInfo fsdatainfo;

SpikeMainWindow::SpikeMainWindow(QWidget *parent, const char *name, Qt::WFlags f ) 
: QMainWindow(parent, name, f)
{
  int i, j;
  int nelect;

  /* set up the focus so that this widget get keyboard events */
  setFocus();
  setFocusPolicy(Qt::StrongFocus);
  setEnabled(1);

  rewardCont = NULL;

  // Create a multitabbed window
  QTabWidget *qtab = new QTabWidget(this, 0, 0);
  qtab->setContentsMargins(0,0,2,0);

  /* set a pointer so we can access this later */
  dispinfo.qtab = qtab;

  QSizePolicy es(QSizePolicy::Expanding, QSizePolicy::Expanding, 1, 1);
  qtab->setSizePolicy(es);
  QBoxLayout *hbox;

  /* we create one tab if this is a continuous + position system, and several
   * tabs if this is a spike or continuous system */
  //QRect r(0, 0, dispinfo.screen_width, dispinfo.screen_height);
  QString tablabel;
  for (i = 0; i < NAUDIO_OUTPUTS; i++) {
    dispinfo.audioGroup[i] = NULL;
    dispinfo.fsaudioGroup[i] = NULL;
  }

  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    this->ntabs = 1;
    w = new QWidget* [this->ntabs];
    spikeGLPane = new SpikeGLPane* [this->ntabs];
    w[0] = new QWidget();
    hbox = new QHBoxLayout(w[0]);
    if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
      /* create the EEG Button group */
      EEGButtonGroup = new QButtonGroup(this);
      /* put the group outside the window */
      dispinfo.EEGButtonGroup = EEGButtonGroup;

      eegtab = 0;
      spikeGLPane[0] = new SpikeGLPane(0, w[0], "EEGPANE", 
          dispinfo.neegchan1, dispinfo.neegchan2, TRUE);
      /* connect the button group's clicked signal to the function
       * that launches an eeg dialog */
      connect(EEGButtonGroup, SIGNAL(buttonClicked(int)), this, 
          SLOT(createEEGDialog(int)));
      tablabel = QString("EEG / Position");
    }
    else {
      /* create a position only window */
      spikeGLPane[0] = new SpikeGLPane(0, w[0], 0, 0, TRUE);
      tablabel = QString("Position");
    }
    spikeGLPane[0]->setSizePolicy(es);
    QSize s(dispinfo.screen_width, dispinfo.screen_height);
    spikeGLPane[0]->setBaseSize(s);
    hbox->addWidget(spikeGLPane[0],1,0);
    hbox->setContentsMargins(0,0,0,0);
    w[0]->setLayout(hbox);
    qtab->addTab(w[0], tablabel);
  } 
  else if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
    this->ntabs = 1;
    w = new QWidget* [this->ntabs];
    spikeGLPane = new SpikeGLPane* [this->ntabs];
    w[0] = new QWidget();
    hbox = new QHBoxLayout(w[0]);
    /* create the EEG Button group  */
    EEGButtonGroup = new QButtonGroup(this);
    dispinfo.EEGButtonGroup = EEGButtonGroup;

    eegtab = 0;
    spikeGLPane[0] = new SpikeGLPane(0, w[0], "EEGPANE", 
        dispinfo.neegchan1, dispinfo.neegchan2, FALSE);
    /* connect the button group's clicked signal to the function
     * that launches an eeg dialog  */
    connect(EEGButtonGroup, SIGNAL(buttonClicked(int)), this, 
        SLOT(createEEGDialog(int)));
    tablabel = QString("EEG");
    spikeGLPane[0]->setSizePolicy(es);
    QSize s(dispinfo.screen_width, dispinfo.screen_height);
    spikeGLPane[0]->setBaseSize(s);
    hbox->addWidget(spikeGLPane[0],1,0);
    hbox->setContentsMargins(0,0,0,0);
    w[0]->setLayout(hbox);
    qtab->addTab(w[0], tablabel);
  } 
  else if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
    /* create the appropriate number of SpikeGLPanes, assuming
     * NELECT_PER_SCREEN, one screen for the full screen tetrode 
     * and one extra screen for the continuous data */
    this->ntabs = (int) ceil((float) sysinfo.nelectrodes / NELECT_PER_SCREEN) + 2; 
    /* create a button group for the audio buttons */
    /* remember - the NSpike master has TWO audio outs, so two
     * audio groups! */ 
    audioGroup1 = new QButtonGroup(this);
    audioGroup1->setExclusive(TRUE); // only one can be on at a time
    dispinfo.audioGroup[0] = audioGroup1;

    audioGroup2 = new QButtonGroup(this);
    audioGroup2->setExclusive(TRUE); // only one on at a time
    dispinfo.audioGroup[1] = audioGroup2;

    /* create a button group for the full screen audio buttons */
    fsaudioGroup1 = new QButtonGroup(this);
    fsaudioGroup1->setExclusive(TRUE); // only one on at a time
    dispinfo.fsaudioGroup[0] = fsaudioGroup1;

    fsaudioGroup2 = new QButtonGroup(this);
    fsaudioGroup2->setExclusive(TRUE); // only one on at a time
    dispinfo.fsaudioGroup[1] = fsaudioGroup2;

    /* create the EEG Button group */
    EEGButtonGroup = new QButtonGroup(this);
    dispinfo.EEGButtonGroup = EEGButtonGroup;

    /* create a pointer list for the tetrode input widgets */
    dispinfo.spikeTetInput = new SpikeTetInput* [sysinfo.nelectrodes+1];

    /* create a pointer list for the tetrode info widgets */
    dispinfo.spikeTetInfo = new SpikeTetInfo* [sysinfo.nelectrodes+1];

    w = new QWidget* [this->ntabs];
    spikeGLPane = new SpikeGLPane* [this->ntabs];
    for (i = 0; i < this->ntabs; i++) {
      w[i] = new QWidget(qtab, "tetwindow");
      hbox = new QHBoxLayout(w[i]);
      if (i < this->ntabs - 2) {
        /* this is a multipanel spike pane */
        nelect = MIN((sysinfo.nelectrodes - (i * NELECT_PER_SCREEN)),
            NELECT_PER_SCREEN);
        tablabel = QString("Tetrodes ");
        for (j = 0; j < nelect; j++) {
          tablabel += QString("%1 ")
            .arg(sysinfo.channelinfo[sysinfo.machinenum]
                [(i*NELECT_PER_SCREEN+j)*NCHAN_PER_ELECTRODE].number);
        }
        spikeGLPane[i] = new SpikeGLPane(w[i], i, nelect);
      }
      else if (i == this->ntabs - 2) {
        /* this is a single panel pane */
        nelect = 1;
        spikeGLPane[i] = new SpikeGLPane(w[i], i, TRUE);
        tablabel = QString("Full Screen Tetrode");
      }
      else {
        eegtab = i;
        spikeGLPane[i] = new SpikeGLPane(i, w[i], "EEGPANE", 
            dispinfo.neegchan1, dispinfo.neegchan2);
        /* connect the button group's clicked signal to the function
         * that launches an eeg dialog */
        connect(EEGButtonGroup, SIGNAL(buttonClicked(int)), this, 
            SLOT(createEEGDialog(int)));
        tablabel = QString("EEG");

      }
      spikeGLPane[i]->setSizePolicy(es);
      QSize s(dispinfo.screen_width, dispinfo.screen_height);
      spikeGLPane[i]->setBaseSize(s);
      hbox->addWidget(spikeGLPane[i],1,0);
      hbox->setContentsMargins(0,0,0,0);
      w[i]->setLayout(hbox);
      qtab->addTab(w[i], tablabel);
    }
    /* connect up the tab widgets current changed signal to the Main Form's
     * changePage slot */
    connect(qtab, SIGNAL(currentChanged(QWidget *)), this, 
        SLOT(pageChanged(QWidget *)));
  }
  qtab->setTabPosition(QTabWidget::Bottom);

  /* now add a widget to display the time */
  dispinfo.timeString = QString("");
  timeLabel = new QLabel(this);
  dispinfo.timeLabel = timeLabel;
  timeLabel->setText(dispinfo.timeString);
  /* unfortunatly, adding timeLabel to the main grid causes frequent redraws
   * of the EEG screen for reasons I don't understand, so we have to stick it
   * in the upper right hand corner for the moment */
  timeLabel->setFixedHeight(20);
  timeLabel->setFont(QFont( "TypeWriter", 12, QFont::Normal ));
  qtab->setCornerWidget(timeLabel,Qt::BottomRightCorner);
  setCentralWidget(qtab);


  /* create a menu bar */
  menuBar = new QMenuBar(this);

  if (sysinfo.system_type[sysinfo.machinenum] == MASTER) {
    /* create the master menu */
    masterMenu = new QMenu("&Master",this);
    masterAcqAction = masterMenu->addAction("Toggle Acquisition", this, SLOT(masterAcq()), Qt::CTRL+Qt::Key_A );
    masterMenu->insertSeparator();
    masterMenu->addAction("&Clear All", this, SLOT(masterClear()), Qt::CTRL+Qt::Key_C );
    masterMenu->insertSeparator();

    masterStartSaveAction = masterMenu->addAction( "Start All Save", this, SLOT(masterDiskOn()));
    masterStopSaveAction = masterMenu->addAction( "Stop All Save", this, SLOT(masterDiskOff()));
    masterMenu->insertSeparator();

    masterOpenFilesAction = masterMenu->addAction( "&Open All Files", this, SLOT(masterOpenFiles()));
    masterCloseFilesAction = masterMenu->addAction( "&Close All Files", this, SLOT(masterCloseFiles()));
    masterMenu->insertSeparator();

    masterMenu->addAction( "&Save Config File", this, SLOT(saveConfig()), Qt::CTRL+Qt::Key_S );
    masterMenu->insertSeparator();

    masterResetClockAction = masterMenu->addAction( "&Reset Clocks", this, SLOT(masterResetClock()));
    masterMenu->insertSeparator();

    masterMenu->addAction( "&Audio Settings", this, SLOT(masterAudioSettings()));
    masterMenu->insertSeparator();

    masterMenu->addAction( "Reprogram Master DSP", this, SLOT(masterReprogramMasterDSP()));
    masterMenu->addAction( "Reprogram Aux DSPs", this, SLOT(masterReprogramAuxDSPs()));
    masterMenu->addAction( "Display DSP Code Revision", this, SLOT(masterShowDSPCodeRev()));
    masterMenu->insertSeparator();

    masterMenu->addAction( "&Quit", this, SLOT(masterQuit()), Qt::CTRL+Qt::Key_Q);
    menuBar->addMenu(masterMenu );
  }

  fileMenu = new QMenu("&File", this);
  startSaveAction = fileMenu->addAction( "Local Start Save", this, SLOT(diskOn()));
  stopSaveAction = fileMenu->addAction( "Local Stop Save", this, SLOT(diskOff()));
  fileMenu->insertSeparator();
  openFileAction = fileMenu->addAction( "&Open Local File", this, SLOT(open()));
  closeFileAction = fileMenu->addAction( "&Close Local File", this, SLOT(close())); 
  fileMenu->insertSeparator();
  usecompAction = fileMenu->addAction("Compress Data File", this, SLOT(toggleUsesCompression()));
  compsetAction = fileMenu->addAction("Compression Settings...", this, SLOT(doCompressionSettingsDialog()));
    updateUsesCompression();
  menuBar->addMenu( fileMenu );

  /* create a display menu */
  displayMenu = new QMenu("&Display", this);
  displayMenu->addAction( "Redraw", this, SLOT(redraw()), Qt::Key_R );
  displayMenu->addAction( "Clear", this, SLOT(clear()), Qt::Key_C );
  displayMenu->addAction( "EEG Trace Length", this, SLOT(eegTraceLength()), 
      Qt::CTRL+Qt::Key_E );
  menuBar->addMenu( displayMenu );

  if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
    /* add a menu for couping all of the references, thresholds, and
     * filters */
    tetrodeSettingsMenu = new QMenu("&Tetrode Settings", this);
    crAction = tetrodeSettingsMenu->addAction("Common Reference", this, SLOT(commonRef()));
    crAction->setCheckable(true);
    ctAction = tetrodeSettingsMenu->addAction("Common Threshold", this, SLOT(commonThresh()));
    ctAction->setCheckable(true);
    cmdvAction = tetrodeSettingsMenu->addAction("Common Max. Disp. Val.", this, SLOT(commonMDV()));
    cmdvAction->setCheckable(true);
    cfAction = tetrodeSettingsMenu->addAction("Common Filters", this, SLOT(commonFilt()));
    cfAction->setCheckable(true);
    menuBar->addMenu( tetrodeSettingsMenu );
  }


  if (sysinfo.datatype[sysinfo.machinenum] & POSITION) {
    /* create the position menu */
    posMenu = new QMenu("&Position", this);
    posMenu->setCheckable(TRUE);
    posOutputAction = posMenu->addAction( "Position Output On" , this, SLOT(outputPos()), Qt::Key_V); 
    posAllowSCAction = posMenu->addAction( "Allow Sync Channel Changes" , this, SLOT(allowSyncChange())), 
    menuBar->addMenu( posMenu);
  }
  if (sysinfo.datatype[sysinfo.machinenum] & DIGITALIO) {
    /* create the digital IO menu */
    digioMenu = new QMenu("Digital / Analog &IO", this);
    digioMenu->addAction( "Trigger Output", this, SLOT(digioOutput()), Qt::Key_O );
    digioMenu->addAction( "Set &Outputs", this, SLOT(setOutputs()), Qt::Key_R );
    digioMenu->addSeparator();

    digioMenu->addAction( "Reward Control GUI", this, SLOT(startRewardControl()));
    if (sysinfo.fsdataoutput) {
      digioMenu->addSeparator();
      digioMenu->addAction( "Feedback / Stim GUI", this, SLOT(fsGUI()), Qt::CTRL+Qt::Key_G );
      digioFSDataMenu = new QMenu("Feedback / Stim Data", this);
      fsDataStartAction = digioFSDataMenu->addAction( "Send Data", this, SLOT(fsDataStart()));
      fsDataStopAction = digioFSDataMenu->addAction( "Stop Sending Data", this, SLOT(fsDataStop()));
      fsDataSettingsAction = digioFSDataMenu->addAction( "Data Settings", this, SLOT(fsDataSettings()));
    }
    digioMenu->addMenu(digioFSDataMenu);

    digioMenu->addSeparator();

    digioProgMenu = new QMenu("Digital/Analog IO User Programs", this);
    for (i = 0; i < digioinfo.nprograms; i++) {
      digioProgMenu->addAction(QString(digioinfo.progname[i]));
    }
    connect(digioProgMenu, SIGNAL(activated(int)), this, SLOT(runProgram(int)));
    digioMenu->addMenu( digioProgMenu);
    digioMenu->addAction( "Send Message to User Program", this, SLOT(outputToFSProgram()), Qt::CTRL+Qt::Key_U );
    digioMenu->addSeparator();
    digioMenu->addAction( "Reset State Machines", this, SLOT(resetStateMachines()));
    menuBar->addMenu( digioMenu);
  }

  setMenuBar(menuBar);

  spikeInfo = new SpikeInfo(this);
  setStatusBar(spikeInfo);

  /* start off with the appropriate screen */
  if (sysinfo.defaultdatatype & SPIKE) {
    qtab->setCurrentPage(0);
  }
  else if (sysinfo.defaultdatatype & CONTINUOUS) {
    qtab->setCurrentPage(this->eegtab);
  }

  /* set the dispinfo variables so we can access this later */
  dispinfo.w = w;
  dispinfo.ntabs = ntabs;

}

SpikeMainWindow::~SpikeMainWindow() {
    if (spikeGLPane != NULL)
        delete spikeGLPane;
}

void SpikeMainWindow::updateAllInfo(void) {
    int i;
     
    this->spikeInfo->updateInfo();
    i = dispinfo.qtab->currentPageIndex();
    this->spikeGLPane[i]->updateAllInfo();
}

void SpikeMainWindow::updateInfo(void) {
    int i;

    this->spikeInfo->updateInfo();
    /* update the bottom information on the current tab */
    this->spikeGLPane[dispinfo.qtab->currentPageIndex()]->updateInfo();
    /* if a file is open we need to disable the tab(s) that are not relevant
     * for our datatype */
    if (sysinfo.fileopen) {
  for (i = 0; i < dispinfo.ntabs; i++) {
      if ((sysinfo.defaultdatatype & SPIKE) && 
        (!spikeGLPane[i]->spikePane) && 
        (!spikeGLPane[i]->singleSpikePane)) {
    dispinfo.qtab->setTabEnabled(dispinfo.w[i], FALSE);
      }
      else if ((sysinfo.defaultdatatype & CONTINUOUS) && 
        (!spikeGLPane[i]->EEGPane)) {
    dispinfo.qtab->setTabEnabled(dispinfo.w[i], FALSE);
      }
      else if ((sysinfo.defaultdatatype & POSITION) && 
        (!spikeGLPane[i]->posPane)) {
    dispinfo.qtab->setTabEnabled(dispinfo.w[i], FALSE);
      }
  }
    }
    else {
  for (i = 0; i < dispinfo.ntabs; i++) {
      dispinfo.qtab->setTabEnabled(dispinfo.w[i], TRUE);
  }
    }

    return;
}


void SpikeMainWindow::changeGLPage(QWidget *w) {
    /* change to the appropriate page */
    int index;

    if (w == NULL) return;

    index = dispinfo.qtab->currentPageIndex();
    if ((sysinfo.datatype[sysinfo.machinenum] & SPIKE) || 
             (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS)) {
  if (spikeGLPane[index]->EEGPane) {
      /* we are now on the continuous page, so we need to change display
       * modes if we are in SPIKE mode */
      if (sysinfo.datatype[sysinfo.machinenum] & SPIKE) {
    SwitchDisplayModes();
      }
  }
  else if (spikeGLPane[index]->singleSpikePane) {
      /* we are now on a tetrode  page, so we need to change display
       * modes if we are in CONTINUOUS mode */
      if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
    SwitchDisplayModes();
      }
      /* we moved to the full screen electrode page, so we have to set
       * the fullscreen electrode number if it has not been set */
      if (dispinfo.fullscreenelect == -1) {
    dispinfo.currentdispelect = 0;
    dispinfo.fullscreenelect = 0;
    dispinfo.nelectperscreen = 1;
    this->prevPage = 0;
      }
  }
  else  {
      /* we are now on a tetrode  page, so we need to change display
       * modes if we are in CONTINUOUS mode */
      if (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS) {
    SwitchDisplayModes();
      }
      /* we moved to one of the tetrode tabs, so we set
       * dispinfo.currentdispelect appropriately */
      dispinfo.currentdispelect = index * NELECT_PER_SCREEN;
      dispinfo.fullscreenelect = -1;
      /* set the number of electrodes on this screen */
      dispinfo.nelectperscreen = MIN(sysinfo.nelectrodes - dispinfo.currentdispelect, NELECT_PER_SCREEN);
  }
    }
    updateAllInfo();
}
               

void SpikeMainWindow::openDataFile(void)
{
    if (!sysinfo.fileopen) {
  //QFileDialog *fd = new QFileDialog( this, "Open Data Files", TRUE );
  Q3FileDialog *fd = new Q3FileDialog(
    QString(sysinfo.datadir[sysinfo.machinenum]), 
    NULL, this, "Open Data File", TRUE );
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
    /* copy the file name into the sysinfo.datafilename and 
     * sysinfo.origdatafilename variables */
    strcpy(sysinfo.datafilename, fileName.ascii());
    OpenFile();
      }
  }
    }
    else {
      DisplayStatusMessage("Error: a file is currently open\n");
    }
}

void SpikeMainWindow::closeDataFile(void)
{
   if (sysinfo.fileopen) {
      if  (QMessageBox::question(this, tr("Close File?"),
		 tr("Are you sure you want to close the data file?"),
                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        CloseFile();
      }
   }
    else {
  DisplayStatusMessage("Error: no file is open\n");
    }
}

void SpikeMainWindow::saveConfigFile(void)
{
    Q3FileDialog *fd = new Q3FileDialog( this, "Save Config File", TRUE );
    fd->setMode( Q3FileDialog::AnyFile );
    QString fileName;
    if ( fd->exec() == QDialog::Accepted ) {
  /* check to see if the file exists */
        fileName = fd->selectedFile();
  /* make sure the user wants to overwrite the file if it exits*/
  if (QFile::exists(fileName)  && QMessageBox::question(
      this, tr("Overwrite File?"), tr("A file called %1 already exists."
    "Do you want to overwrite it?").arg(fileName),
      tr("&Yes"), tr("&No"), QString::null, 1, 1 ) ) {
      return;
  }
  WriteConfigFile((char *)fileName.ascii(), 0, 0);
    }
}

void SpikeMainWindow::masterOpenDataFiles(void)
{
    QString f;
    if (!sysinfo.fileopen) {
  Q3FileDialog *fd = new Q3FileDialog(
    QString(sysinfo.datadir[sysinfo.machinenum]), 
    NULL, this, "Open Data Files", TRUE );
  DisplayStatusMessage("Enter the root name of the data files");
  
  fd->setMode( Q3FileDialog::AnyFile );
  QString fileName;
  if ( fd->exec() == QDialog::Accepted ) {
      fileName = fd->selectedFile();
      f = fileName;
      f.append(".");
      f.append(netinfo.myname);
      f.append(".dat");
      /* make sure the user wants to overwrite the file if it exits*/
      if (QFile::exists(f)  && QMessageBox::question(
    this,
    tr("Overwrite File?"),
    tr("A file called %1 already exists."
        "Do you want to overwrite it?")
        .arg(f),
    tr("&Yes"), tr("&No"),
    QString::null, 1, 1 ) ) {
    return;
      }
      else {
    /* Open the files with the specified root name */
    MasterOpenFiles((char *)fileName.ascii());
      }
  }
    }
    else {
  DisplayStatusMessage("Error: a file is currently open\n");
    }
}



void SpikeMainWindow::masterCloseDataFiles(void)
{
   if (sysinfo.fileopen) {
       if (QMessageBox::question( this,
    tr("Close All Files?"),
    tr("Are you sure you want to close all data files?"),
    tr("&Yes"), tr("&No"),
    QString::null, 1, 1 )  == 0) {
      MasterCloseFiles();
  }
   } 
   else {
  DisplayStatusMessage("Error: files are not open\n");
   }
}

void SpikeMainWindow::reprogramDSPDialog(int master)
{
    if (!sysinfo.acq) {
        if (master) {
      Q3FileDialog *fd = new Q3FileDialog(
        QString(sysinfo.datadir[sysinfo.machinenum]), 
        NULL, this, "Master DSP program file", TRUE );
      DisplayStatusMessage("Select the program file for the Master DSP");
      fd->setMode( Q3FileDialog::AnyFile );
      QString fileName;
      if ( fd->exec() == QDialog::Accepted ) {
    fileName = fd->selectedFile();
    if (ReprogramMasterDSP((char *) fileName.ascii())) {
        QMessageBox::information(this, "DSP Reprogramming Status", "Master DSP Reprogrammed\nWhen you click OK, NSpike will exit.  You must then power cycle the DSPs.\n\nRemember to check the DSP code revision when you next start NSpike");  
        Quit(0);
    }
    else {
        QMessageBox::critical(this, "Error", "Error programming Master DSP. Try again.\nIf the problem persists, try quitting and power cycling the DSPs."); 
    }
      }

  }
  else {
      Q3FileDialog *fd = new Q3FileDialog(
        QString(sysinfo.datadir[sysinfo.machinenum]), 
        NULL, this, "Aux DSP program file", TRUE );
      DisplayStatusMessage("Select the program file for the AUX DSPs");
      fd->setMode( Q3FileDialog::AnyFile );
      QString fileName;
      if ( fd->exec() == QDialog::Accepted ) {
    fileName = fd->selectedFile();
    if (ReprogramAuxDSPs((char *) fileName.ascii())) {
        QMessageBox::information(this, "DSP Reprogramming Status", "Aux DSPs Reprogrammed\nWhen you click OK, NSpike will exit.  You must then power cycle the DSPs.\n\nRemember to check the DSP code revision when you next start NSpike");  
        Quit(0);
    }
    else {
        QMessageBox::critical(this, "Error", "Error programming the Aux DSPs. Try again.\nIf the problem persists, try quitting and power cycling the DSPs."); 
    }
      }
  }
   } 
   else {
  DisplayStatusMessage("Error: cannot reprogram DSPs while acquisition is on\n");
   }
}

void SpikeMainWindow::showDSPCodeRev(void)
{
    int i;

    GetAllDSPCodeRev();
    QString s = QString("Code Revision Numbers:\n");
    for (i = 0; i < MAX_DSPS; i++) {
  if (sysinfo.dspinfo[i].coderev > 0) {
      s.append(QString("DSP %1:\t%2\n").arg(i).arg(sysinfo.dspinfo[i].coderev));
  }
    }
    QMessageBox::information(this, "DSP Code Revisions", s);
    return;
}



void   SpikeMainWindow::toggleUsesCompression()
{
  if (sysinfo.use_compression) { // uncheck
    /* uncheck the item */
    sysinfo.use_compression = 0;
  } 
  else {
    /* check the item */
    sysinfo.use_compression = 1;
  }
  if (sysinfo.fileopen) {
    warnCompSettingsTakeEffect();
  }
  updateUsesCompression();
}

void SpikeMainWindow::updateUsesCompression()
{
    if (!sysinfo.use_compression) { // uncheck
  /* uncheck the item */
  usecompAction->setChecked(false);
  compsetAction->setChecked(false);
    } else {
  /* check the item */
  usecompAction->setChecked(true);
  compsetAction->setChecked(true);
    }
}

void SpikeMainWindow::warnCompSettingsTakeEffect()
{
    QMessageBox::information(this, "Changes Not Yet Applied", "Changes to compression settings take effect\nthe next time the data file is opened.");
}

void SpikeMainWindow::doCompressionSettingsDialog()
{
    bool ok;
    int lvl = QInputDialog::getInteger("Compression Level", "Compression level to use for data files 0 (none) - 9 (slowest)", sysinfo.compression_level, 0, 9, 1, &ok, this, "CompLvlDlg");
    if (ok) {
      sysinfo.compression_level = lvl;
      if (sysinfo.fileopen) warnCompSettingsTakeEffect();
      }
}

void SpikeMainWindow::updateCommonRef(void) 
{
  sysinfo.commonref = crAction->isChecked();
}

void SpikeMainWindow::updateCommonThresh(void) 
{
  sysinfo.commonthresh = ctAction->isChecked();
}

void SpikeMainWindow::updateCommonMDV(void) 
{
  sysinfo.commonmdv = cmdvAction->isChecked();
}

void SpikeMainWindow::updateCommonFilt(void) 
{
  sysinfo.commonfilt = cfAction->isChecked();
}

void SpikeMainWindow::updatePositionOutput() 
{
    if (posOutputAction->isEnabled()) {
  /* uncheck the item */
  posOutputAction->setChecked(false);
  dispinfo.displayratpos = 0;
    }
    else {
  /* check the item */
  posOutputAction->setChecked(true);
  dispinfo.displayratpos = 1;
    }
}

void SpikeMainWindow::updateAllowSyncChange() 
{
    if (posAllowSCAction->isEnabled()) {
  /* uncheck the item */
  posAllowSCAction->setChecked(false);
  sysinfo.allowsyncchanchange = 0;
    }
    else {
  /* check the item */
  posAllowSCAction->setChecked(true);
  sysinfo.allowsyncchanchange = 1;
    }
}


void SpikeMainWindow::triggerOutput(void)
{
    bool ok;
    int output = QInputDialog::getInteger("Pulse Output", 
      "Enter the number of the output to pulse", 0, 0, MAX_BITS, 1, 
      &ok, this);
    if (ok) {
  TriggerOutput(output);
    }
}

void SpikeMainWindow::getOutputToFSProgram(void)
{    
    char userstring[1000];
    bool ok;
    QString text = QInputDialog::getText(
            "NSpike", "Enter string to send to user's digital IO program", 
      QLineEdit::Normal, QString::null, &ok, this );
    if ( ok && !text.isEmpty() ) {
  /* send the text to the user's program*/
  if (text.length() < 1000) {
      strcpy(userstring, text.ascii());
      SendDigIOFSMessage(userstring, (int) text.length() + 1);
  }
  else {
      sprintf(userstring, "Error: string too long");
      DisplayStatusMessage(userstring);
  }
    } 
}

void SpikeMainWindow::setEEGTraceLength(void)
{
  bool ok;
  double length = QInputDialog::getDouble("EEG Trace Length", 
    "Enter the new EEG trace length in seconds ", 1.0, 0.000001, 
    MAX_EEG_TRACE_LENGTH, 6, &ok, this);
  if (ok) {
    SetEEGTraceLength((float) length);
  }
}

void SpikeMainWindow::launchFSGUI(void)
{
  char tmpstring[200];
  /* the fsgui requires an associate FSDATA data type, so check this */
  if (sysinfo.datatype[sysinfo.machinenum] & FSDATA) {
    if (strncmp(sysinfo.fsgui, "stim", 4) == 0) {
      if (dispinfo.fsguiptr == NULL)
	dispinfo.fsguiptr = new DIOInterface();
      else
	((DIOInterface *)dispinfo.fsguiptr)->show();
    }
    else {
      sprintf(tmpstring,"Unknown fs gui:%s", sysinfo.fsgui);
      DisplayErrorMessage(tmpstring);
    }
  }
  else {
     QMessageBox::critical(this, "Error", "Error: FSDATA is not enabled for this machine.\nEdit the config file and restart NSpike."); 
  }
}

void SpikeMainWindow::keyPressEvent( QKeyEvent *e )
{
  int index;
  /* process keyboard events. most of these are handled by the menu bar, so
   * we ignore any that we can't interpret */
  index = dispinfo.qtab->currentPageIndex();
  if (e->key() == Qt::Key_PageDown) { // pg down
    /* if we are on a multipaned spike window, we go down one window */
    if ((index < ntabs - 1) && (this->spikeGLPane[index + 1]-> spikePane)) {
      dispinfo.qtab->setCurrentPage(index + 1);
      this->updateAllInfo();
    }
    else if ((this->spikeGLPane[index]->singleSpikePane == TRUE ) && 
	   (dispinfo.fullscreenelect < sysinfo.nelectrodes - 1)) {
      dispinfo.fullscreenelect++;
      dispinfo.currentdispelect = dispinfo.fullscreenelect;
      this->updateAllInfo();
      DrawInitialScreen();
    }
    e->accept();
  } 
  else if (e->key() == Qt::Key_PageUp) { //pgup
    /* if we are on a multipaned spike window, we go up one window */
    if ((index > 0) && !(this->spikeGLPane[index]->singleSpikePane) && 
      (this->spikeGLPane[index-1]->spikePane)) {
	dispinfo.qtab->setCurrentPage(index - 1);
	this->updateAllInfo();
    }
    else if ((this->spikeGLPane[index]->singleSpikePane == TRUE) && 
	 (dispinfo.fullscreenelect > 0)) {
      dispinfo.fullscreenelect--;
      dispinfo.currentdispelect = dispinfo.fullscreenelect;
      this->updateAllInfo();
      DrawInitialScreen();
    }
    e->accept();
  }
  else {
    e->ignore();
  }
}



