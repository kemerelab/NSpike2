/* spikeFSData.cpp: qt object for selecting data sent to spike_fsdata.cpp
 *
 * Copyright 2010 Loren M. Frank
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

#include "spikeFSData.h"
#include "spikecommon.h"
#include "spikeGLPane.h"
#include <string>
#include <sstream>

extern DisplayInfo dispinfo;
extern SysInfo sysinfo;
extern NetworkInfo netinfo;
extern DigIOInfo digioinfo;
extern FSDataInfo fsdatainfo;



fsDataDialog::fsDataDialog(QWidget* parent, 
	const char* name, bool modal, Qt::WFlags fl)
    : QDialog( parent, name, modal, fl)
{
    int nrows, row;
    int ncols, midcol, col;
    int i;

    QString s;

    nrows = MIN(sysinfo.maxelectnum, 16) + 6;
    ncols = 2 * (sysinfo.maxelectnum / 16 + 1) + 1;
    midcol = ncols / 2 + 1;

    Q3GridLayout *grid = new Q3GridLayout(this, nrows, ncols, 0, 0, "fsDataDialogLayout");

    QSizePolicy p(QSizePolicy::Preferred, QSizePolicy::Preferred, FALSE);
    QSizePolicy p2(QSizePolicy::Maximum, QSizePolicy::Maximum, FALSE);

    QFont fs( "SansSerif", 10, QFont::Normal );

    posSelect = new QRadioButton("Send Position", this, "sendPosButton");
    posSelect->setFont(fs);
    grid->addMultiCellWidget(posSelect, 0, 0, 0, midcol - 1);
    if (fsdatainfo.sendpos) {
	posSelect->setChecked(TRUE);
    }

    digioSelect = new QRadioButton("Send Digital IO", this, "sendDigIOButton");
    digioSelect->setFont(fs);
    grid->addMultiCellWidget(digioSelect, 0, 0, midcol, ncols-1);
    if (fsdatainfo.senddigio) {
	digioSelect->setChecked(TRUE);
    }
    
    /* create the labs for the spike and continuous data */
    spikeLabel = new QLabel("SpikeElectrodes", this, 0);
    spikeLabel->setFont(fs);
    spikeLabel->setText("Spike Electrodes");
    grid->addMultiCellWidget(spikeLabel, 2, 2, 0, midcol - 2);

    contLabel = new QLabel("ContElectrodes", this, 0);
    contLabel->setFont(fs);
    contLabel->setText("Cont. Electrodes");
    grid->addMultiCellWidget(contLabel, 2, 2, midcol, ncols-1);

    /* allocate space for the spike and continuous radio buttons */
    electSpikeSelect = (QRadioButton **) new QRadioButton 
						*[sysinfo.maxelectnum + 1];
    electContSelect = (QRadioButton **) new QRadioButton 
						*[sysinfo.maxelectnum + 1];

    for (i = 1; i <= sysinfo.maxelectnum; i++) {
	/* FIX TO GO THROUGH CHANNELINFO for each MACHINE */
	/* create a radio button for both the spike and continuous data
	 * selectors */
	col = (i-1)/16;
	row = (i-1)%16 + 4;
	s = QString("%1").arg(i);
	electSpikeSelect[i] = new QRadioButton(s, this, "electSpikeSelect");
	electSpikeSelect[i]->setFont(fs);
	electSpikeSelect[i]->setAutoExclusive(false);
	grid->addMultiCellWidget(electSpikeSelect[i], row, row, col, 
		col);
	if (fsdatainfo.spikeelect[i]) {
	    electSpikeSelect[i]->setChecked(TRUE);
	}

	electContSelect[i] = new QRadioButton(s, this, "electContSelect");
	electContSelect[i]->setFont(fs);
	electContSelect[i]->setAutoExclusive(false);
	grid->addMultiCellWidget(electContSelect[i], row, row, midcol + col, 
		midcol + col);
	if (fsdatainfo.contelect[i]) {
	    electContSelect[i]->setChecked(TRUE);
	}
    }
    
    /* create the accept button */
    accept = new QPushButton("Accept", this, "AcceptDialog");
    connect(accept, SIGNAL(clicked()), this, SLOT(acceptSettings()));
    grid->addMultiCellWidget(accept, nrows-1, nrows-1, 0, ncols - 1); 

    /* we will emit the finished signal once we have finished up, so we connect
     * that signal to the accept slot */
    connect(this, SIGNAL(finished()), this, SLOT(accept()));

    show();
}

void fsDataDialog::setFSDataInfo() {
    int i;
    /* use the state of the buttons to set the user data info */
    if (posSelect->isChecked()) {
	fsdatainfo.sendpos = TRUE;
    }
    if (digioSelect->isChecked()) {
	fsdatainfo.senddigio = TRUE;
    }
    fsdatainfo.sendcont = FALSE;
    fsdatainfo.sendspike = FALSE;
    for (i = 1; i <= sysinfo.maxelectnum; i++) {
	if (electSpikeSelect[i]->isChecked()) {
	    fsdatainfo.spikeelect[i] = TRUE;
	    fsdatainfo.sendspike = TRUE;
	}
	else {
	    fsdatainfo.spikeelect[i] = FALSE;
	}
	if (electContSelect[i]->isChecked()) {
	    fsdatainfo.contelect[i] = TRUE;
	    fsdatainfo.sendcont = TRUE;
	}
	else {
	    fsdatainfo.contelect[i] = FALSE;
	}
    }

    /* now we need to send out these new settings to everyone */
    SendFSDataInfo();

    emit finished();
    return;
}

fsDataDialog::~fsDataDialog() {
}



