#ifndef __SPIKE_FS_DATA_H__
#define __SPIKE_FS_DATA_H__

#include <QtGui>

#include <q3textedit.h>
#include <q3listbox.h>
#include <QKeyEvent>
#include <Q3GridLayout>
#include <Q3PopupMenu>
#include <q3buttongroup.h>
#include <q3filedialog.h>
#include <Q3TextStream>
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <Q3BoxLayout>

#include "spikeGLPane.h"
#include "spike_dsp.h"
#include "spike_dio.h"
#include "spike_main.h"

extern DigIOInfo digioinfo;
extern DisplayInfo dispinfo;

/* Reward GUI */

class userDataDialog : public QDialog {
	Q_OBJECT

    public:
	    userDataDialog(QWidget *parent = 0, 
		    const char *name = 0, bool model = FALSE, 
		    Qt::WFlags fl = 0);
	    ~userDataDialog();
	    void setFSDataInfo();

    public slots:
	    void acceptSettings(void) { setFSDataInfo(); };

    signals:
	    void finished(void);

    protected:
	QRadioButton  	*posSelect;
	QRadioButton  	*digioSelect;
	QLabel		*spikeLabel;
	QLabel		*contLabel;
	QRadioButton  	**electSpikeSelect;
	QRadioButton  	**electContSelect;
	QPushButton	*accept;
};


#endif // spikeFSData.h
