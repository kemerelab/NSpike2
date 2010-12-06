#ifndef __SPIKE_MAIN_H__
#define __SPIKE_MAIN_H__
#include <GL/gl.h>	// Header File For The OpenGL32 Library
#include <GL/glu.h>	// Header File For The GLu32 Library
#include "../src-gui/spikeInput.h"
#include "../src-gui/spikeStatusbar.h"
#include "../src-gui/spikeGLPane.h"
#include "../src-gui/spikeFSGUI.h"
#include "qrect.h"
#include "q3buttongroup.h"
#include "qtabwidget.h"
#include "qwidget.h"
#include "q3popupmenu.h"
//Added by qt3to4:
#include <QLabel>

/* color file for eeg display */
#define COLOR_FILE	"spike_rgbcolor"

/* FIX THIS */
/* fonts to use */
#define LARGE_FONT 	NULL
#define SMALL_FONT 	NULL

/* the number of buffers to allocate for the display program (the maximum
 * number of spikes we want to be able to have before we start not displaying
 * data */
#define NDISPLAY_DATA_BUFS	 50

/* Screen coordinates */
#define X_MIN		-100.0
#define X_MAX		 100.0
#define Y_MIN		-100.0
#define Y_MAX		 100.0

/* set the default window size */
#define DEFAULT_SCREEN_WIDTH	1024
#define DEFAULT_SCREEN_HEIGHT	768

/* Call lists for the display of different types of data */
#define TET_SPIKE_CALL_LIST	1
#define TET_PROJ_CALL_LIST	2
#define TET_BUTTON_CALL_LIST	3
#define EEG_CALL_LIST		4
#define EEG_BUTTON_CALL_LIST	5
#define POSITION_CALL_LIST	6


#define NELECT_PER_SCREEN	4	// display 4 electrodes at a time
/* sizes for spike display windows*/
#define TET_SPIKE_WIN_WIDTH		(14.75 * dispinfo.totalxsize / 200)
#define TET_SPIKE_WIN_HEIGHT		(60.0 * dispinfo.totalysize / 200)
#define TET_SPIKE_WIN_X_START		(0.0 *  dispinfo.totalxsize / 200)
#define TET_SPIKE_WIN_Y_START		(20.0 * dispinfo.totalysize / 200)
#define TET_SPIKE_WIN_Y_ZERO		(22.0 * dispinfo.totalysize / 200)	// the location of zero in terms of the distance above the start of the spike window
#define TET_SPIKE_BUTTON_HEIGHT	(TET_SPIKE_WIN_Y_START - TET_PROJ_WIN_Y_START)

#define TET_FULL_SCREEN_XSTART		-99 // the origin for a full screen display
#define TET_FULL_SCREEN_YSTART		-90 // the origin for a full screen display

/* size for projection display windows*/
#define TET_PROJ_WIN_WIDTH		(19.0 * dispinfo.totalxsize / 200)
#define TET_PROJ_WIN_HEIGHT		(27.0 * dispinfo.totalysize / 200)		
#define TET_PROJ_WIN_X_START	        (60.0 * dispinfo.totalxsize / 200)
#define TET_PROJ_WIN_Y_START		(5.0 * dispinfo.totalysize / 200)

/* size for the threshold lines */
#define TET_THRESH_LENGTH 		1.5 * dispinfo.totalxsize / 200

/* EEG defines */
#define NEEG_WINDOWS			2 	// show a window on each side of the screen
#define EEG_WIN_WIDTH			(81.0 * dispinfo.totalxsize / 200)
#define EEG_WIN1_X_START		(-82.0 * dispinfo.totalxsize / 200)
#define EEG_WIN1_Y_START		(-98 * dispinfo.totalysize / 200) // this gives flush with fullscreen
//#define EEG_WIN1_Y_START		(-85.5 * dispinfo.totalysize / 200)
#define EEG_WIN1_HEIGHT			(198 * dispinfo.totalysize / 200)
//#define EEG_WIN1_HEIGHT			(182.0 * dispinfo.totalysize / 200)
#define EEG_WIN2_X_START		(18.0 * dispinfo.totalxsize / 200)
#define EEG_WIN2_Y_START		(-98 * dispinfo.totalysize / 200)
//#define EEG_WIN2_Y_START		(-85.5 * dispinfo.totalysize / 200)
#define EEG_WIN2_HEIGHT			(108 * dispinfo.totalysize / 200)
#define EEG_TICK_HEIGHT			(2 * dispinfo.totalysize / 200)

#define MAX_EEG_TRACE_LENGTH		100.0 // max 100 seconds

#define MAX_PROJ_WIN_POINTS		100000  /* save up to 1e5 points for the projection window displays */


/* Position defines */
#define POS_WIN_WIDTH		EEG_WIN_WIDTH
#define POS_WIN_HEIGHT		(67 * dispinfo.totalysize / 200)
//#define POS_WIN_HEIGHT		(50 * dispinfo.totalysize / 200)
#define POS_WIN_X_START		EEG_WIN2_X_START
//#define POS_WIN_Y_START		(-20 * dispinfo.totalxsize / 200)
#define POS_WIN_Y_START		(24.5 * dispinfo.totalysize / 200)
#define POS_COLORBAR_WIDTH		(5.0 * dispinfo.totalxsize / 200)
#define POS_COLORBAR_HEIGHT		POS_WIN_HEIGHT
#define POS_COLORBAR_X_START		(12.0 * dispinfo.totalxsize / 200)
#define POS_COLORBAR_Y_START		POS_WIN_Y_START
#define POS_COLORBAR_THRESH_X_START     (8.0 * dispinfo.totalxsize / 200)
#define POS_COLORBAR_THRESH_WIDTH	(3.0 * dispinfo.totalxsize / 200)
#define POS_COLORBAR_THRESH_HALF_HEIGHT	(1.5 * dispinfo.totalysize / 200)

#define COMMON_BUTTON_HEIGHT		6 // common buttons are 6 units high
#define NCOMMON_BUTTONS			4 // 4 common buttons 


/* structures */


typedef struct _Color {
    float r;
    float g;
    float b;
} Color ;

typedef struct _ProjectionWindowData {
    short 	*point; 	// the list of spike waveform peaks for the electrode
    int		maxpoints;	// the total number of points in the list
    short   	*lastdrawn;	// the pointer to the last draw point for each electrode
    short   	*end;		// the pointer to the last valid point for each electrode
    int		endindex;	// the index of the last valid point 
} ProjectionWindowData;

typedef struct _ButtonInfo {
    Point	p1;	// the lower left corner
    Point	p2;	// the upper right corner
    float	shadowsize; // the amount to shift for the shadown rectangles 
    int		selected; // 1 if this button is currently selected
    int		nstrings; // 1 - 4 to indicate the number of strings that should be drawn
    char 	str[4][100]; // the strings that should be placed in this button
    Point	strloc[4];   // the starting location for each string
    void	*strfont[4]; // the fonts for each string
} ButtonInfo;


typedef struct _DisplayInfo {
    int 	screen_width;
    int 	screen_height;
    int 	default_screen_width;
    int 	default_screen_height;
    int 	fullscreen;
    int		nelectperscreen;
    float 	totalxsize;
    float 	totalysize;
    float	xunit; 		// a unit increment in the x direction
    float	yunit; 		// a unit increment in the y direction

    /* indicators of the currently selected channel and electrode */
    int		currentchan;
    int		currentelect;

    QButtonGroup	**audioGroup; // the button groups for the audio buttons 
    QButtonGroup	**fsaudioGroup; // the button groups for the full screen audio buttons 
    QTabWidget		*qtab; // the list of tabbed pages;
    int 		ntabs;
    QWidget		**w; 	// the individual pages
    void 	        *fsguiptr;  // the fsgui pointer
    
    SpikeTetInput	**spikeTetInput; // the list button sets for thresholds, etc.
    SpikeTetInfo	**spikeTetInfo; // the depth and number info
    SpikeEEGInfo	**spikeEEGInfo; // the information for eeg channels
    SpikePosInfo	*spikePosInfo; // the information for position data
    SpikeInfo		*spikeInfo; // file and disk information, status info
    QButtonGroup	*EEGButtonGroup; // the button group for the eeg buttons
    QString		timeString;
    QLabel		*timeLabel;

    char		statusmessage[200];
    char		errormessage[200];

    /* tetrode display variables */
    SpikeBuffer	spikebuf;	// the most recent spike 
    int		nprojwin;
    Point 	spikewinorig[NCHAN_PER_ELECTRODE]; // the starting coordinate for each of the spike windows
    Point 	*projwinorig; 	// the starting coordinate for each of the projection windows
    Point	**spikewaveform; // the current spike waveform to be displayed for each electrode 
    float 	spikewinxinc;    // the x increment between adjacent waveform points
    float 	*spikewinyscale;   // the multiplier to translate into screen coordinates
    float	spikewinymax;	 // the value to use for clipping the tops of the spike waveforms
    float	spikewinymin;	 // the value to use for clipping the bottoms of the spike waveforms
    float 	*projwinxscale;   // the multiplier to translate into screen coordinates
    float 	*projwinyscale;   // the multiplier to translate into screen coordinates
    float	projwinxmax;	 // the value to use for clipping the projection window
    float	projwinymax;	 // the value to use for clipping the projection window
    Point	*threshloc;	 // the locations for the threshold lines
    int		*overlay;	 // 0 or 1 for each electrode
    ProjectionWindowData  *projdata;  // the lists of projection window points 
    int 	*electnum;	 // the user defined numbers of all the electrodes on this machine
    int 	*electind;	 // the index for each of the user defined electrode numbers
    int 	currentdispelect; // the first of the electrode currently displayed
    Point	*electloc;	 // the coordinates for the translations for the electrodes
    int		fullscreenelect;  // the number of the electrode that is currently displayed in full screen mode (or -1 if none)
    int		nelectbuttons;   // the number of buttons for each electrode
    ButtonInfo  *electbutton;      // the buttons for the electrodes 
    Point	electnumloc1;	// the botton right location of the electrode number string
    Point	electnumloc2;	// the top left location of the electrode number string
    Point	depthloc1;	// the botton right location of the electrode depth string
    Point	depthloc2;	// the top left location of the electrode depth string

    /* EEG display variables */
    ContBuffer	contbuf;	// the most recent continuous data
    int 	neegchan1;	// the number of eeg channels on the left
    int 	neegchan2;	// the number of eeg channels on the right
    int		ncolors;
    Color	*color;		// the list of colors for eeg traces
    Point	*eegtraceorig;	// the origins for the eeg windows
    int		*eegpointnum;	// the index of the last drawn point
    float 	*eegtraceoffset; // the current y position of each eegtrace
    float 	eegtracestarttime; // the earliest start time of a currently displayed trace
    Point	**eegtrace;    // the current trace to be drawn 
    int		*neegpoints;	// the number of eeg points displayed across the screen
    Color	**eegcolor;	// a pointer to the colors of each trace
    float	*eegxinc;	// the x increment for each point
    float	*eegyscale;	// the y scale multiplier for each trace;
    float	*eegymin;	// the minimum y value
    float	*eegymax;	// the maximum y value
    float 	*eegchanheight;	// the y size of the "window" that each trace is displayed in
    int		eegskip;	// the number of points to skip in each trace
    ButtonInfo  *eegbutton;      // the buttons for the eeg channels
    int		*chanind;	// the display indeces for each channel
    float 	contzoom;	// the zoom factor for the continuous display 
    float	eegtracelength;	// the size, in seconds, of the eeg traces

    /* Position display variables */
    PosBuffer		*posbuf;	// the current position buffer
    GLubyte		*posimage;	// the current image (in RGB format)
    int			imagesize;	// the total size of the image in pixels
    Point		posloc;		// the x,y coordinates of the bottom left corner of the image
    Point		poszoom;	// the zoom factor to make the image the right size
    int			posoverlay;	// 0 or 1 for overlay off / on
    char 		*posoverlaypixels; // the pixels to keep on when posoverlay is 1
    Point		poscolorbarloc[2]; // the coordinates of the bottom left and top right corner of the color bar (used for thresholding)
    float		colorbaryscale; // the y scale multiplier for the colorbar;
    Point		posthreshloc[3];// the coordinates for the position threshold triangle (this must be shifted to the correct y position)
    Point		posdiffstrloc[2]; // the coordinates for the posdiff string
    Point		posdataratestrloc[2]; // the coordinates for the posdatarate string
    float		posdatarate;	      // the current rate, in MB / min, for position data 
    int			displayratpos;	      // the current center of mass of the tracked pixels in the display
} DisplayInfo;

struct PosDataRate {
    int			display;
    int			bytes;
    struct PosDataRate  *next;
};

struct DisplayData {
    SpikeBuffer		spikebuf[MAX_SPIKES_PER_BUF];
    int			nspikes;
    ContBuffer		contbuf;
    short		datatype;
    struct DisplayData 	*next; 
};

/* function prototypes */
int InitializeModuleMessaging(void);
void GetNetworkConfig(char *filename);
void SendSystemConfig(void);
void InitializeDataBuffer(void);

void SetupClock(void);
void ResetClock(void);
void StopClock(void);
void StartClock(void);
void TestSync(void);

void ResetAll(void);

void InitGraphics(void);
void InitGL(int Width, int Height);

void Main_Loop(void);
void Reshape(int w, int h);
void Keyboard(unsigned char key);
void Mouse(int button, int state, int x, int y);
void SpecialKeyboard(int key, int x, int y);
void MainMenu(int item);
void InitMenus(void);
void MainLoop(void);
void ProcessMasterCommand(int command);
int OpenFile(void);
void CloseFile(void);
void StartSave(void);
void StopSave(void);
void ClearAll(void);


/* FS Data Functions */
void UpdateFSDataCount(void);
void SendFSDataInfo(void);
void SendFSDataMessage(int messagetype, char *message, int len);
void SendDigIOInfo(void);
void MasterFSDataStart(void);
void MasterFSDataStop(void);
int FSDataStart(void);
int FSDataStop(void);

/* Master commands */
void ToggleAcq(void);
void MasterClearAll(void);
void MasterStartSave(void);
void MasterStopSave(void);
void MasterOpenFiles(char *rootname);
void MasterCloseFiles(void);
void ResetClock(void);
void TestSync(void);
void Quit(int status);
void UpdateMenus(void);

void Usage(void);

int StartAcquisition(void);
int StopAcquisition(void);
void StartAllAcquisition(void);
void StopAllAcquisition(void);
int StartModuleAcquisition(void);
int StartModuleAcquisition(int modulenum);
int StopModuleAcquisition(void);
int StopModuleAcquisition(int modulenum);


void DrawInitialScreen(void);
void DefineCommonDrawInfo(void);
void UpdateCommonButton(int bnum);
void DisplayCommon(void);

void GLToWindow(Point glpoint, QRect &geom, int *windowx, int *windowy,
		float xscale, float yscale);

void UpdateChanInfo(int chanNum, int type, int newVal);
void UpdateChannelButtonText(int electrode, int chan);
float SpikeWindowScale(ChannelInfo *ch);
void DefineTetrodeDrawInfo(void);
void DisplayAllTetrodeWindows(void);
void DisplayTetrodeWindow(int elect);
void DisplayAllTetrodeTextAndButtons(void);
void DisplayTetrodeTextAndButtons(int elect);
void DisplayTetrodeButtons(int elect);
void UpdateChanMaxDispVal(int elect, int chan, short mdv); 
void UpdateTetrodeMaxDispVal(int elect, short mdv); 
void UpdateTetrodeThresh(int elect, short thresh); 
int UpdateChanThresh(int elect, int chan, short newval);
int DrawChanThresh(int elect, int chan);
void UpdateTetrodeLowFilt(int elect, short thresh); 
int UpdateChanLowFilt(int chan, short newval);
void UpdateTetrodeHighFilt(int elect, short thresh); 
int UpdateChanHighFilt(int chan, short newval);
void DisplayAllTetrodeThresholds(void);
void UpdateAllTetrodeButtonText(void);
void UpdateTetrodeButtonText(int elect);
void UpdateSpikeProjScale(int elect);
void DisplayTetrodeNumber(int elect);
void DisplayTetrodeDepth(int elect);
void SetDepth(int electnum, int depth);
void SetReference(int electnum, int refelect, int refchan);



void DrawInitialEEGScreen(void);
void UpdateEEGButtonText(void);
void DisplayEEGButtons(void);
void DefineEEGDrawInfo(void);
void SetEEGInc(void);
void ResetEEGTraces(int clearlast);
void UpdateContScale(int chan);
void SetEEGTraceLength(float length);


void DrawInitialPosScreen(void);
void DefinePosDrawInfo(void);
void SetPosLoc(void);
void UpdatePosImage(PosBuffer *posbuf);
void DisplayPosImage(void);
void DisplayPosDataRate(void);
void TogglePositionOutput(void);


void DisplayTime(void);

void DrawLargeFontString(char *str, Point *loc);
void DrawSmallFontString(char *str, Point *loc);
void FormatTS(QString *timestring, u32 timestamp);
void FormatTS(char *timestring, u32 timestamp);

void PushTetrodeMatrix(int elect);

void DrawButton(ButtonInfo *button);

void ChangeActiveButton(int move);

void SwitchDisplayModes(void);
void SendDataType(void);

void DisplayElectrodeData(SpikeBuffer *spikebuf, int nspikes);
int ElectrodeOnScreen(int electind);
int abschannelnum(int electrode, int chan);


void DisplayContinuousData(ContBuffer *contbuf);

void DisplayStatusMessage(char *message);
void DisplayErrorMessage(char *message);
void DisplayStatusMessage(const char *message);
void DisplayErrorMessage(const char *message);

void ProcessInputString(unsigned char inputchar);
void DisplayInputString(char *inputmessage, char *inputstr);

void spikeexit(int status);

#endif
