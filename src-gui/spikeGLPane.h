#ifndef __SPIKEGL_PANE_H__
#define __SPIKEGL_PANE_H__

#include <QtGui>
#include <qgl.h>
#include <q3buttongroup.h>

#include "sqcompat.h"
#include "spike_defines.h"
#include "spikeInput.h"

class SpikeGLPane : public QGLWidget {
    Q_OBJECT

    public:
        //SpikeGLPane(int nChannels, QWidget *parent = 0);
        SpikeGLPane(QWidget *parent = 0, int paneNum = 0, int nElect = 0);
        SpikeGLPane(QWidget *parent = 0, int paneNum = 0, bool fullScreenElect = FALSE);
        SpikeGLPane(int paneNum = 0, QWidget *parent = 0, const char *name = "", int neegchan1 = 0, 
			int neegchan2 = 0, bool posPane = FALSE);
        ~SpikeGLPane();    
	SpikeTetInput 	**spikeTetInput;
	SpikeTetInfo 	**spikeTetInfo;
	SpikeEEGInfo 	**spikeEEGInfo;
	SpikePosInfo 	*spikePosInfo;
	void updateAllInfo();
	void updateInfo();

    protected:
        void initializeGL();
        void resizeGL(int w, int h);
        void paintGL();
	int  prevW, prevH;  // the previous width and height
	int datatype;	// the type of data displayed on this pane
	int paneNum;	// the number of this pane
	bool fullScreenElect; //True if this is for the full screen electrode
	int nElect;	// the number of electrods on this pane
	int neegchan1;  // the number of eeg channels on the left (for EEG mode)
	int neegchan2;  // the number of eeg channels on the right(for EEG mode)
	QPushButton **paramButtons;

    public:
	bool posPane;
	bool spikePane;
	bool singleSpikePane;
	bool EEGPane;

        
};

#endif
