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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with nspike; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "spikeStatusbar.h"
#include "spike_main.h"
//#include "spike_functions.h"
//#include "spikeMainWindow.h"

// #include <string>
// #include <sstream>

extern DisplayInfo dispinfo;
extern SysInfo sysinfo;
// extern CommonDSPInfo cdspinfo;
// extern SpikeMainWindow *spikeMainWindow;
// extern char *tmpstring;

SpikeInfo::SpikeInfo(QWidget* parent) : QStatusBar(parent)
{
    // Now implemented in Status Bar

    QString s;

    QFont f( "SansSerif", 12, QFont::Normal );

    clear = new QPushButton("Clear Status", this, "ClearStatus");
    connect(clear, SIGNAL( clicked() ), this, SLOT(clearStatus()) );
    addPermanentWidget(clear);

    message = new QLabel("test", this, 0);
    message->setFont(f);
    message->setText("");
    addPermanentWidget(message,3);

    QWidget *FileDiskStatus = new QWidget(this);
    QGridLayout *statusLayout = new QGridLayout(FileDiskStatus);

    fileStatus = new QLabel(FileDiskStatus);
    fileStatus->setFont(f);
    fileStatus->setAutoFillBackground(true);
    //addPermanentWidget(fileStatus);
    statusLayout->addWidget(fileStatus,0,0,1,1);

    fileSize = new QLabel(FileDiskStatus);
    fileSize->setFont(f);
    fileSize->setAutoFillBackground(true);
    //addPermanentWidget(fileSize);
    statusLayout->addWidget(fileSize,0,1,1,1);

    diskStatus = new QLabel(FileDiskStatus);
    diskStatus->setFont(f);
    diskStatus->setAutoFillBackground(true);
    //addPermanentWidget(diskStatus);
    statusLayout->addWidget(diskStatus,1,0,1,1);

    diskFree = new QLabel(FileDiskStatus);
    diskFree->setFont(f);
    diskFree->setAutoFillBackground(true);
    //addPermanentWidget(diskFree);
    statusLayout->addWidget(diskFree,1,1,1,1);

    FileDiskStatus->setLayout(statusLayout);
    addPermanentWidget(FileDiskStatus);
}


SpikeInfo::~SpikeInfo() {
}

void SpikeInfo::clearStatus() { 
  message->setText(""); sysinfo.newmessage = 0;
};

void SpikeInfo::updateInfo(void) 
{
  QString s;
  /* update the labels */

  QPalette palRed;
  QPalette palGreen;
  QPalette palDefault;
  palRed.setColor(QPalette::Window,QColor(255,50,50));
  palGreen.setColor(QPalette::Window,QColor(0, 200, 0));

  // default colors
  message->setPaletteForegroundColor("black");
  fileStatus->setPalette(palDefault);
  fileSize->setPalette(palDefault);
  diskStatus->setPalette(palDefault);
  diskFree->setPalette(palDefault);

  if (sysinfo.newmessage) { // display the current status message
    message->setText(QString(dispinfo.statusmessage));
  }
  if (dispinfo.errormessage[0] == '\0') {
    message->setStyleSheet("color: black;");
  }
  else {
    /* display the current error message in red*/
    message->setStyleSheet("color: red;");
  }

  if (sysinfo.fileopen) {
    fileStatus->setText(QString(sysinfo.datafilename));
    fileSize->setText(QString("%1 MB").arg(sysinfo.datafilesize, 0, 'f',1));

    fileStatus->setPalette(palGreen);
    fileSize->setPalette(palGreen);
  }
  else {
    fileStatus->setText("No File");
    fileSize->setText(QString("0.0 MB"));

    fileStatus->setPalette(palRed);
    //fileSize->setPalette(palRed);
  }

  FormatTS(&s, sysinfo.disktime);
  if (sysinfo.diskon) {
    diskStatus->setText(QString("Disk on  %1").arg(s));
    diskStatus->setPalette(palGreen);
  }
  else if (sysinfo.fileopen) {
    diskStatus->setText(QString("Disk off  %1").arg(s));
    diskStatus->setPalette(palRed);
  }

  diskFree->setText(QString("%1 GB free").arg(sysinfo.diskfree/1000, 0, 'f',1));
  if (sysinfo.diskfree < 500) 
    diskFree->setPalette(palRed);
}

