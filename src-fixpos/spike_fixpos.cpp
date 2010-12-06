/* spike_fixpos: Code for manually entering occluded diode positions and
 * setting detection thresholds
 *
*Usage:
*
*  Defining Diode Positions
*  ^^^^^^^^^^^^^^^^^^^^^^^^
*	When the position markers are hollow, the user can define where
*	the front and back diode positions are. The left mouse button is
*	for the front diode, and the right button is for the back diode. When
*	return is pressed, the position is saved and playback is resumed.
*
*  Main Menu
*  ^^^^^^^^^
*  	The main menu pops up when the middle mouse button is pressed.
*	Keep the button pressed down and release to make a selection.
*
*  Fast Keys
*  ^^^^^^^^^
*  	You can press keys at any time to make quick changes. These keys are:
*
*	spacebar - pause playback
*	enter - execute command
*	' - revert to the last user-picked diode positions
*	t - change threshold
*	r - change exclude ring size
*	b - step back one frame
*	n - step foreward one frame
*	c - clear excluded pixels
*	x - add pixel to exclude list
*
*	S - foreward to next full image
*	B - back to first frame
*	Q - quit
*
*  Command Line
*  ^^^^^^^^^^^^
*  	Type spike_fixpos -a for command line options.
*
***************************************************************************
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


#define _FILE_OFFSET_BITS 64 //define for 64 bit offsets
#include "spikecommon.h"
#include "time.h"
#include "spike_dio.h"
#include "spike_fixpos_defines.h"

/* mpeg includes */
extern "C"
{ 
#include <mpeg2.h>
#include <mpeg2convert.h> 
}

/* GL includes */
#include <GL/gl.h>	// Header File For The OpenGL32 Library
//#include <GL/glu.h>	// Header File For The GLu32 Library
#include <GL/glut.h>	// Header File For The glut Library

#define MPEG_BUFFER_SIZE	16384
//#define MPEG_BUFFER_SIZE	2
#define fopen fopen64

#define TMPSTRINGLEN 200

typedef struct _WindowInfo {
    float 	bottom;
    float	top;
    float	left;
    float	right;
    float	imagestartx;
    float	imagestarty;
    int 	width;
    int 	height;
    Point	nbrightloc;
    Point	threshloc;
    Point	timeloc;
    Point	inputloc;
} WindowInfo;



struct Image {
    unsigned char	*pixels;
    struct Image	*prev;
    struct Image	*next;
};

struct BrightInfo {
    u32 	bufferinfo[10]; /* the first two elements are,
                           respectively, the timestamp and the number of bright elements */
    u32		nexttimestamp; // the timestamp of the next buffer (we always read one ahead 
    int			*brightpixels;
    struct BrightInfo	*prev;
    struct BrightInfo	*next;
    float		frontx;
    float		fronty;
    float		backx;
    float		backy;
};

struct BrightInfo *MakeBrightInfo(struct BrightInfo *last);
struct Image *MakeImage(struct Image *lastimage);


int Init();
int readposrec(FILE *file, PosRec *posrec);
int ProcessNextImage(void);
void ExcludeAllBoxes();
void UnExcludeAllBoxes();
void ExcludeBoxes(int startbox, int endbox);
void UnExcludeBoxes(int startbox, int endbox);
void DeleteImage(struct Image *currentimage);
void DeleteBright(struct BrightInfo *bright);
int WritePos(struct BrightInfo *brightptr);
void Finish(void);
void UpdatePosImage(u32 *posbufferinfo);
void GetBrightPixels(int trackblack);
void NewBrightPixels (float *slope, float *b);
void GetExcludedPixels(void);
void DisplayExcludeBoxes(void);
void DisplayImage(GLubyte *currentimage);
void DisplayRatPos(void);
void DisplayNBright(void);
void DisplayThresh(void);
void DisplayTime(void);
void WinlocToImage(int winx, int winy, int *imagex, int *imagey);
void WindowToWinloc(int winx, int winy, int *winlocx, int *winlocy);
char *FormatTS(u32 time);
void MainDisplay(int v);
void Swap(void);
void Reshape(int w, int h);
void Keyboard(unsigned char key, int x, int y);
void Mouse(int button, int state, int x, int y);
void MouseMotion(int x, int y);
void ProcessInputString(unsigned char inputchar);
void DisplayInputString(char *inputmessage, char *inputstr);
void DrawLargeFontString(char *str, Point *loc);
void DrawSmallFontString(char *str, Point *loc);
void GetOvalVerteces(Oval *oval);
void GetTmpOval(Oval *origoval, Oval *tmpoval);
void position_assign(float *frontx, float *backx, float *fronty, float *backy,
	             int nbright, int *brightpixels, int *flag, float *slope,
		     float *b, float lastdir, int nskipped, float maxassigneddist, Oval *oval, int trackblack, int numlights);
int IsInPolygon(float wx,float wy,float *xvert,float *yvert,int ncoords);
void xy_to_1d (float frontx, float fronty, float backx, float backy, int *front, int *back);
void init_menus(void);
void main_menu(int item);
void drawCircle(float circx, float circy, float radius, float Red, float Green, float Blue);
void drawOval(Oval *oval, float Red, float Green, float Blue);
//u32 ParseTimestamp(char *s);
int strcount(char *s,char c);




DiodePosInfo 		CurrentSpecs;
DiodePosInfo 		NextSpecs;
DiodePosInfo		PreviousSpecs;
DiodePosInfo		LastValidSpecs;
DiodePosInfo		LastFSDefinedSpecs;
SysInfo    		sysinfo;
NetworkInfo		netinfo;
DigIOInfo		digioinfo;
FSDataInfo		fsdatainfo;
CommonDSPInfo		cdspinfo;
struct Image    	*imageptr, *tmpimageptr, *firstimageptr;
InterpInfo 		info;
WindowInfo 		wininfo;
struct BrightInfo 	*brightinfoptr, *tmpbrightinfoptr, *firstbrightinfoptr,
                        *lastvalidpos;
Oval			origoval, tmpoval;
unsigned short		*newbrightptr;
GLubyte			*currentimage;

char 			timestring[TMPSTRINGLEN+1];
char 			tmpstring[TMPSTRINGLEN+1];
char 			inputstr[TMPSTRINGLEN+1];
int			inputstrlen;
char 			inputmessage[TMPSTRINGLEN+1];
char 			inputchar;
int			framespersec;
int			userinput;
int			lost;
int 			nskipped;
int			readnextimage;
int			userpause;
int			step;
int 			imagenum;
int			getnextfullimage;
int			flag;
int			displayall;
int			front;
int 			back;
int			dumptofile;
int			assignfront;
int			positionassigned;
int			tmpx1, tmpx2, tmpy1, tmpy2;
int                     trackblack;
int                     numlights;
clock_t			nowtime, latertime;
float			timepassed, waittime;

float			speed = 10;
int			offset64 = 0;  // 1 if we are using a 64 bit offset file


u32		lasttime = 0;

unsigned long		tstart, tend;
// u32		tstart, tend;

float			lastdir;	// the last heading direction
float			maxassigneddist = 0;	//average distance between front and back diodes (only user selected examples)
float			circx, circy; //specs for the ignore circle
float			majoraxis = 0, minoraxis = 0;


/* mpeg decoding related defines */
FILE		*mpegfile;
int 		imageready;
int		first = 1;
uint8_t 	buffer[MPEG_BUFFER_SIZE * 16];
mpeg2dec_t 	*decoder;
const mpeg2_info_t *mpeginfo;
const mpeg2_sequence_t *sequence;
mpeg2_state_t 	state;
size_t 		size;

int main(int argc, char **argv) {

    int nxtarg = 0;
    int i;
    int tmp;
    float ftmp;
    char tmpstring[TMPSTRINGLEN+1];
    char filename[TMPSTRINGLEN+1];
    char outfilename[TMPSTRINGLEN+1];
    char playbackfilename[TMPSTRINGLEN+1];
    char offsetfilename[TMPSTRINGLEN+1];
    char starttimestr[TMPSTRINGLEN+1];
    char endtimestr[TMPSTRINGLEN+1];
    float slope;
    float b;
    PosRec posrec;
    size_t	outputseek;

    info.timestampfile = 0;
    info.offsetfile = 0;
    info.outfile = 0;
    displayall = 1;

    info.thresh = 0;
    info.skip = 0;
    info.append = 0;
    tstart = 1;
    tend = ULONG_MAX;
    trackblack = 0;
    numlights = 2;
    tmpx1 = tmpx2 = tmpy1 = tmpy2 = 0;

    /* initialize the mpeg related variables */
    decoder = mpeg2_init ();
    if (decoder == NULL) {
        fprintf (stderr, "Could not allocate a mpeg decoder object.\n");
        exit (1);
    }
    mpeginfo = mpeg2_info (decoder);
    size = (size_t) -1;
    outfilename[0] = '\0';


    while (++nxtarg < argc) {
	if (strcmp(argv[nxtarg], "-t") == 0) {
	    strcpy(info.timestampfilename, argv[++nxtarg]);
	    /* check to make sure we can open the file; */
	    if ((info.timestampfile = gzopen(info.timestampfilename, "r")) 
		    == NULL) {
		fprintf(stderr, "Error opening %s for reading\n",
			info.timestampfilename);
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-p") == 0) {
	    /* open the position */
	    strcpy(filename, argv[++nxtarg]);
	    if ((mpegfile = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", outfilename);
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-o") == 0) {
            /* open the output file */
            strcpy(outfilename, argv[++nxtarg]);
        }
	else if (strcmp(argv[nxtarg], "-f") == 0) {
	    /* open the offset file */
	    strcpy(offsetfilename, argv[++nxtarg]);
	    if ((info.offsetfile = fopen(offsetfilename, "r")) == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", 
			offsetfilename);
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-f64") == 0) {
	    /* open the 64 bit offset file */
	    strcpy(offsetfilename, argv[++nxtarg]);
	    if ((info.offsetfile = fopen(offsetfilename, "r")) == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", 
			offsetfilename);
		exit(-1);
            }
	    offset64 = 1;
	}
	else if (strcmp(argv[nxtarg], "-thresh") == 0) {
	    if (((tmp = atoi(argv[++nxtarg])) > 0) && (tmp < 256)) {
		info.thresh = (unsigned char) tmp;
            }
	    else {
		fprintf(stderr, "Error: threshold %d out of bounds (0 <= thresh <= 255)\n", tmp);
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-skip") == 0) {
	    if ((tmp = atoi(argv[++nxtarg])) > 0) {
		info.skip = tmp;
            }
	    else {
		fprintf(stderr, "Error: skip must be > 0\n");
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-black") == 0) {
	    
            trackblack = atoi(argv[++nxtarg]);
           
	}
        else if (strcmp(argv[nxtarg], "-lights") == 0) {
	    
            numlights = atoi(argv[++nxtarg]);
            if ((numlights > 2)||(numlights < 1)){
               fprintf(stderr, "Error: number of lights must be 1 or 2\n");
	       exit(-1);
            }
           
	}
        else if (strcmp(argv[nxtarg], "-speed") == 0) {
	    if ((ftmp = atof(argv[++nxtarg])) > 0) {
		speed = ftmp;
            }
	    else {
		fprintf(stderr, "Error: speed must be > 0\n");
		exit(-1);
            }
	}
        else if (strcmp(argv[nxtarg], "-tstart") == 0) {
	    tstart = ParseTimestamp(argv[++nxtarg]);
	}
	else if (strcmp(argv[nxtarg], "-tend") == 0) {
	    tend = ParseTimestamp(argv[++nxtarg]);
	}
	else if (strcmp(argv[nxtarg], "-displayall") == 0) {
	    displayall = atoi(argv[++nxtarg]);
	}
	else if (strcmp(argv[nxtarg], "-imagesize") == 0) {
	    sysinfo.posimagesize[0] = atoi(argv[++nxtarg]);
	    sysinfo.posimagesize[1] = atoi(argv[++nxtarg]);
	}
	else if (strcmp(argv[nxtarg], "-append") == 0) {
	    info.append = 1;
	}
	else if (strcmp(argv[nxtarg], "-playback") == 0) {
	    info.playback = 1;
	    strcpy(playbackfilename, argv[++nxtarg]);
	    if ((info.playbackfile = fopen(playbackfilename, "r")) == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", 
			playbackfilename);
		exit(-1);
            }
	}
	else {
	    fprintf(stderr, "Usage: nspike_fixpos -p mpegfile -t timestampfile [[-f offsetfile] [-f64 64bitoffsetfile]] -o outputfile [-append] [-playback] [-tstart tstart] [-tend tend] [-b brightpixelsfile] [-thresh threshold divisor] [-skip nframes] [-black 0/1] [-lights 1/2] [-displayall 0/1] [-speed speed_up_factor] [-imagesize x y]\n");
	    fprintf(stderr, "       append indicates that you want to append to a previously started output file.\n");
	    fprintf(stderr, "       tstart and tend are the desired start and end timestamps\n");
	    fprintf(stderr, "       brightpixels fgile is the name of a file previously created by spike_fixpos\n");
	    fprintf(stderr, "       thresh defaults to the threshold and divisor from the configuration header\n");
	    fprintf(stderr, "       skip defaults to 0 and indicates the number of frames where the diode position assignment fails that can be skipped before the user is asked to enter the coordinates\n");
	    fprintf(stderr, "       black switches the tracking mode to use black pixels and only one tracking point (default 0)\n");
            fprintf(stderr, "       lights enables one or two tracking points (default 2) \n");
            fprintf(stderr, "       displayall default to 1 to display all frames and can be set to 0 to display only those frames that require user feedback\n");
	    fprintf(stderr, "       speed sets the speed up factor for display (e.g. 5 will cause the program to display 5 seconds of recordings in 1 second.)\n");
            exit(1);
	}
    }

    /* check to make sure that all necessary arguments have been specified */
    if ((mpegfile == NULL) || (info.timestampfile == 0) || 
	    (outfilename[0] == '\0')) {
	fprintf(stderr, "Error: must specificy mpeg, timestamp and output files\n");
	exit(1);
    }
    /* check to see that the user did not specify both append and playback */
    if (info.append && info.playback) {
	fprintf(stderr, "Error: you cannot both append and playback simultaneously\n");
	exit(1);
    }

    /* create the brightinfoptr in case we need it for appending */
    brightinfoptr = MakeBrightInfo(NULL);

    /* open up the outfile file in the correct mode */
    if ((info.outfile = fopen(outfilename, "r")) != NULL) {
	/* we should exit unless the user specified that we should append or if
	 * this is playback mode */
	if ((!info.append) && (!info.playback)) {
	    fprintf(stderr, "Error: %s already exists and append was not specified\n", outfilename);
	    exit(-1);
	}
	else {
	    /* skip past the header of the position file */
	    do {
		if (fgets(tmpstring, TMPSTRINGLEN, info.outfile) == NULL) {
      fprintf(stderr, "Error reading header of position file.\n");
      exit(-1);
    }
	    } while ((strncmp(tmpstring, "%%ENDHEADER", 10) != 0) &&
		     (strncmp(tmpstring, "%%ENDCONFIG", 10) != 0));
	}
	if (info.append) {
	    /* read in all of the position records from the output file */
	    while (!feof(info.outfile)) {
		if (readposrec(info.outfile, &posrec)) {
		    brightinfoptr->frontx = posrec.x1;
		    brightinfoptr->fronty = posrec.y1;
		    brightinfoptr->backx = posrec.x2;
		    brightinfoptr->backy = posrec.y2;
		    brightinfoptr->bufferinfo[0] = posrec.timestamp;
		    outputseek = ftell(info.outfile);
		    /* reset tstart if we have moved past the specified tstart*/
		    if (posrec.timestamp > tstart) {
			tstart = posrec.timestamp;
		    }
		}
	    }
	    /* close the file */
	    fclose(info.outfile);
	}
    }
    /* reopen the file */
    if ((info.outfile = fopen(outfilename, "r+")) == NULL) {
	if (info.append) {
	    fprintf(stderr, "Error opening %s for appending\n", outfilename);
	    exit(-1);
	}
    }
    if (info.outfile == NULL) {
	/* open the file for normal writing */
	if ((info.outfile = fopen(outfilename, "w")) == NULL) {
	    fprintf(stderr, "Error opening %s for reading\n", outfilename);
	    exit(-1);
	}
    }

    if (!info.append) {
	/* write out an uncompressed header to the output file */
	fprintf(info.outfile, "%%%%BEGINHEADER\n");
	fprintf(info.outfile, "%% File type:\tBinary\n");
	fprintf(info.outfile, "%% Record type:\tT\n");
	fprintf(info.outfile, "%% Extraction type:\textended dual diode position\n");
	fprintf(info.outfile, "%% Fields:\t  timestamp,8,4,1 xfront,2,2,1    yfront,2,2,1    xback,2,2,1     yback,2,2,1\n");
	fprintf(info.outfile, "%%%%ENDHEADER\n");
    }
    else {
	/* seek to the end of the last complete record */
	fseek(info.outfile, outputseek, SEEK_SET);
    }

    if (info.playback) {
	/* read past the header of the file to be played back */
	do {
	    if (fgets(tmpstring, TMPSTRINGLEN, info.playbackfile) == NULL) {
        fprintf(stderr, "Error getting header of playbackfile.\n");
        exit(-1);
      }
	} while ((strncmp(tmpstring, "%%ENDHEADER", 10) != 0) &&
		 (strncmp(tmpstring, "%%ENDCONFIG", 10) != 0));
    }


    /* skip past the header of the timestamp file*/
    do {
        gzgets(info.timestampfile, tmpstring, TMPSTRINGLEN);
    } while ((strncmp(tmpstring, "%%ENDHEADER", 10) != 0) &&
             (strncmp(tmpstring, "%%ENDCONFIG", 10) != 0));

    info.nexcludeboxes = 0;
    for (i = 0; i < info.imagesize; i++) {
	info.exclude[i] = 0;
    }


    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(INIT_WINDOW_WIDTH, INIT_WINDOW_HEIGHT);
    glutCreateWindow("Position Data");
    glutSetCursor(GLUT_CURSOR_CROSSHAIR);

    Init();

    /* function callbacks */
    glutDisplayFunc(Swap);
    //glutIdleFunc(Swap);
    glutTimerFunc((int) (1000 / (speed * 30)), MainDisplay, 0);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(Mouse);
    glutMotionFunc(MouseMotion);
    init_menus();

    glutSwapBuffers();
    glutMainLoop();

    return 1;
}

int Init()
{
    PosRec		posrec;
    u32			offset, lastoffset;
    u32			lasttime = 0;
    int			startframenum;
    u32			*tstampptr;

    int			nfull;
    int			i, ret;


    waittime = .001;
    nowtime = clock();

    inputchar = '\0';
    inputstrlen = 0;

    /* set up the window locations */
    wininfo.imagestartx = wininfo.imagestarty = 0;
    wininfo.width = INIT_WINDOW_WIDTH;
    wininfo.height = INIT_WINDOW_HEIGHT;
    wininfo.nbrightloc.x = 5;
    wininfo.nbrightloc.y = -10;
    wininfo.threshloc.x = 230;
    wininfo.threshloc.y = -20;
    wininfo.timeloc.x = 230;
    wininfo.timeloc.y = -10;
    wininfo.inputloc.x = 5;
    wininfo.inputloc.y = -20;
    if (sysinfo.posimagesize[0] == 0) {
        sysinfo.posimagesize[0] = PIXELS_PER_LINE;
	sysinfo.posimagesize[1] = LINES_PER_FIELD;
    }
    info.imagesize = sysinfo.posimagesize[0] * sysinfo.posimagesize[1];
    wininfo.top = sysinfo.posimagesize[1];
    wininfo.right = sysinfo.posimagesize[0];
    wininfo.bottom = wininfo.top / -8;
    wininfo.left = 0;

    if (info.thresh == 0) {
	info.thresh = sysinfo.posthresh;
    }

    imagenum = 0;

    info.nbright = 0;
    nskipped = 0;

    assignfront = 1;

    /* allocate space for the read in pixels and colors */
    info.brightpixels = (int *) calloc(info.imagesize + 1, sizeof(int));
    newbrightptr = (unsigned short *) calloc(2 * info.imagesize, sizeof(unsigned short));

    /* create space for the displayed image */
    currentimage = (GLubyte *) calloc(info.imagesize * 3, sizeof(GLubyte));


    /* make the first image and the first brightinfoptr structure.
     * Note that the very first image and brightinfoptrs are not used*/
    imageptr = MakeImage(NULL);
    firstimageptr = imageptr;
    firstbrightinfoptr = brightinfoptr;

    origoval.xcenter = tmpoval.xcenter = 0;
    origoval.ycenter = tmpoval.ycenter = 0;
    origoval.majoraxis = tmpoval.majoraxis = 0;
    origoval.minoraxis = tmpoval.minoraxis = 0;

    CurrentSpecs.CenterOfMass.x = 0;
    CurrentSpecs.CenterOfMass.y = 0;
    CurrentSpecs.Center.x = 0;
    CurrentSpecs.Center.y = 0;
    CurrentSpecs.Front.x = 0;
    CurrentSpecs.Front.y = 0;
    CurrentSpecs.Back.x = 0;
    CurrentSpecs.Back.y = 0;
    LastValidSpecs = CurrentSpecs;
    LastFSDefinedSpecs = CurrentSpecs;
    PreviousSpecs = CurrentSpecs;
    NextSpecs.CenterOfMass.x = 0;
    NextSpecs.CenterOfMass.y = 0;
    NextSpecs.Center.x = 0;
    NextSpecs.Center.y = 0;
    NextSpecs.Front.x = 0;
    NextSpecs.Front.y = 0;
    NextSpecs.Back.x = 0;
    NextSpecs.Back.y = 0;

    lost = 1;
    userinput = 0;
    readnextimage = 1;
    userpause = 0;
    step = 0;
    lastdir = -10;

    info.first = 1;
    info.ntstamps = 0;
    info.timestamp = (u32 *) malloc(MAX_FRAMES * sizeof(u32));
    info.offset = (u32 *) malloc(MAX_FRAMES * sizeof(u32));
    info.offset64 = (unsigned long long *) malloc(MAX_FRAMES * 
	    sizeof(unsigned long long));
    
    info.nexttstamp = info.timestamp;


    /* read in the timestamps. NOTE: we start at an offset of 1 to get the
     * frames to line up correctly with the video */
    tstampptr = info.timestamp + 1;
    startframenum = -1;
    while (gzread(info.timestampfile, tstampptr,  sizeof(u32)) == sizeof(u32)) {
	if ((startframenum < 0) && (*tstampptr >= tstart)) {
	    startframenum = info.ntstamps;
	}
	info.ntstamps++;
	tstampptr++;
    }

    fprintf(stderr, "read in %d timestamps, start frame = %d\n", info.ntstamps,
	    startframenum);
    

    /* process images until we get to tstart */
    /* if we have an offset file, read in the offsets, find the frst time >=
     * tstart, and move to that offset in the mpeg */
    if (info.offsetfile) {
	/* read past the header */
	do {
	    if (fgets(tmpstring, TMPSTRINGLEN, info.offsetfile) == NULL) {
        fprintf(stderr, "Error reading past header of mpeg file.\n");
        exit(-1);
      }
	} while (strncmp(tmpstring, "%%ENDHEADER", 10) != 0);
	/* get the first frame from the mpeg file */
	ProcessNextImage();
	if (!offset64) {
	    fread(info.offset, sizeof(u32), info.ntstamps, info.offsetfile);
	    fseek(mpegfile, info.offset[startframenum], SEEK_SET);
	}
	else {
	    fread(info.offset64, sizeof(unsigned long long), info.ntstamps, 
		    info.offsetfile);
	    fseeko(mpegfile, info.offset64[startframenum], SEEK_SET);
	    //fprintf(stderr, "file offset at %lld, should be %lld\n", ftello(mpegfile), info.offset64[startframenum]);
	}
	info.nexttstamp += startframenum - 1;
	mpeg2_reset(decoder, 0);
	if (!ProcessNextImage()) {
	    fprintf(stderr, "Error: reached end of mpeg file. Exiting\n");
	    exit(-1);
	}
    }
    else {
	/* go through frame by frame */
	startframenum = -1;
	do {
	    //fprintf(stderr, "frame at offset %d\n", ftell(mpegfile));
	    if (!ProcessNextImage()) {
		fprintf(stderr, "Error: reached end of mpeg file. Exiting %d\n", startframenum);
		exit(-1);
	    }
	    if (brightinfoptr->bufferinfo[0] - lasttime > (60 * SEC_TO_TSTAMP)){
		lasttime = brightinfoptr->bufferinfo[0];
		printf("Time ~= %d min\n", lasttime / (60 * SEC_TO_TSTAMP));
	    }
	    startframenum++;
	} while ((brightinfoptr->bufferinfo[0] < tstart) && !feof(mpegfile));
    }
    /* if we are playing back, read position records from the playback file
     * until we find the one corresponding to the current time */
    if (info.playback) {
	do {
	    if (readposrec(info.playbackfile, &posrec)) {
		CurrentSpecs.Front.x = brightinfoptr->frontx = posrec.x1;
		CurrentSpecs.Front.y = brightinfoptr->fronty = posrec.y1;
		CurrentSpecs.Back.x = brightinfoptr->backx = posrec.x2;
		CurrentSpecs.Back.y = brightinfoptr->backy = posrec.y2;
	    }
	} while (posrec.timestamp != brightinfoptr->bufferinfo[0]);
    }

    //fprintf(stderr, "feof = %d, skipped to frame %d, time = %ld, offset = %d\n", feof(mpegfile), startframenum, brightinfoptr->bufferinfo[0], ftell(mpegfile));
    return 1;
}


void MainDisplay(int v)
/* on each Display loop, we do the following:
 * 1. read in the next set of frames from the position file if appropriate
 * 2. update the position image */
{

   FILE *outfile;
   int i;
   float slope, b;
   PosRec posrec;

    glClear(GL_COLOR_BUFFER_BIT);

    GetExcludedPixels();
    GetBrightPixels(trackblack);

    /*get x,y positions of front and back clusters, bufferinfo statement so
     * that it skips to later frames
     */
    if ((!positionassigned && !flag && !userpause)||(step)) {
	PreviousSpecs = CurrentSpecs;
	position_assign(&brightinfoptr->frontx, &brightinfoptr->backx,
	    &brightinfoptr->fronty, &brightinfoptr->backy, info.nbright,
	    info.brightpixels, &flag, &slope, &b, lastdir, nskipped, 
	    maxassigneddist, &tmpoval, trackblack, numlights);

	//CurrentSpecs contains info on the bright pixels, and so does brightinfoptr.  The front and back diode info are redundant.

	if ((flag) && (!step)) {
		//if a flag was thrown, the recorded position is 0,0
		brightinfoptr->frontx = 0;
		brightinfoptr->backx = 0;
		brightinfoptr->fronty = 0;
		brightinfoptr->backy = 0;
	}
	else {
		brightinfoptr->frontx = CurrentSpecs.Front.x;
		brightinfoptr->backx = CurrentSpecs.Back.x;
		brightinfoptr->fronty = CurrentSpecs.Front.y;
		brightinfoptr->backy = CurrentSpecs.Back.y;
	}

	/*NewBrightPixels adds bright pixels for front, back, best-fit line, flag*/
	//NewBrightPixels(&slope, &b);
	/* we only assign lastdir if the fit was valid. Otherwise it carries
	 * over from the last frame */


	//Update head direction if position was assigned ok
	if (!flag) {
	    lastdir = atan2(brightinfoptr->fronty - brightinfoptr->backy, brightinfoptr->frontx - brightinfoptr->backx);
	    lost = 0;
	}
    }

    if (dumptofile) {
	outfile = fopen("file.dump", "w");
	for (i = 0; i < info.nbright; i++) {
	    fprintf(outfile, "%d\t%d\n", info.brightpixels[i] %
		    sysinfo.posimagesize[0], info.brightpixels[i] /
		    sysinfo.posimagesize[0]);
	}
	fclose(outfile);
	dumptofile = 0;
    }

    /*convert x,y coordinates of front and back to positions in the brightpixel array*/

    if (displayall || flag) {
	DisplayImage(currentimage);
    }

    //The solid position markers only show when the video is playing, otherwise the markers are hollow
    if ((!lost)&&(!userpause)) {
    	DisplayRatPos();
    }
    DisplayTime();
    DisplayExcludeBoxes();


    tmpoval.xcenter = CurrentSpecs.Center.x;
    tmpoval.ycenter = CurrentSpecs.Center.y;
    tmpoval.angle = atan2f(CurrentSpecs.Front.y - CurrentSpecs.Back.y,
                           CurrentSpecs.Front.x - CurrentSpecs.Back.x);
    /* offset the original oval coordinates by the center and rotate */
    GetTmpOval(&origoval, &tmpoval);


    //displays a green oval; all bright pixels outside it are ignored
    if (tmpoval.majoraxis > 0) {
    	drawOval(&tmpoval, 0, 1, 0);
    }

    //If the video is paused, the rat position markers are hollow to show the user that they can be changed
    if ((userinput)||(userpause)) {
	drawCircle(CurrentSpecs.Front.x, CurrentSpecs.Front.y,3,1,1,0);
	drawCircle(CurrentSpecs.Back.x, CurrentSpecs.Back.y,3,0,0,1);
	if (flag) {

	}

    }
    if ((!userpause) && (flag) && (!positionassigned)){
	/* have the user enter the coordinates if we have skipped the maximum
	 * number of images*/
	lost = 1;
	if (nskipped == info.skip) {
	    readnextimage = 0;
	    CurrentSpecs = NextSpecs; //Show user best guess
	    userinput = 1;
	    userpause = 1;

	}
	else if (!userpause) {
	    nskipped++;
	    //fprintf(stderr, "*** NSKIPPED = %d\n", nskipped);
	    /* reset the flag so we can move on */
	    flag = 0;
	}
    }
    else {
	/* reset nskipped to reflect the fact that the current image is valid */
	nskipped = 0;
	/* keep track of the last valid position */

	LastValidSpecs = CurrentSpecs;

	readnextimage = 1;
    }

    /* if we have less than the minimum number of bright pixels, we
     * will stop read in new images until the user presses the return key */
    if (readnextimage && !userpause) {
	/* copy the list of bright pixels to brightinfoptr */
	brightinfoptr->bufferinfo[1] = info.nbright;
	if (brightinfoptr->brightpixels == NULL) {
	    brightinfoptr->brightpixels = (int *) calloc(info.nbright, sizeof(int));
        }
	else {
	    brightinfoptr->brightpixels = (int *) realloc(brightinfoptr->brightpixels,
	                                                  info.nbright * sizeof(int));
        }
	memcpy(brightinfoptr->brightpixels, info.brightpixels, info.nbright * sizeof(int));

	imagenum++;
	if (imagenum >= NSAVEDIMAGES) {
	    /* delete the first image */
	    tmpimageptr = firstimageptr;
	    firstimageptr = firstimageptr->next;
	    DeleteImage(tmpimageptr);
	    /* write out the first brightinfo structure */
	    WritePos(firstbrightinfoptr);
	    /* delete the firstbright structure */
	    tmpbrightinfoptr = firstbrightinfoptr;
	    firstbrightinfoptr = firstbrightinfoptr->next;
	    DeleteBright(tmpbrightinfoptr);
            imagenum--;
       }
    }

    DisplayNBright();
    DisplayThresh();

    if (inputchar != 0) {
        DisplayInputString(inputmessage, inputstr);
    }

    /* now read in the next image */
    if (readnextimage && !userpause) {
        /* if we have not yet read in the next image, read it in */
	if (imageptr->next == NULL) {
	    /* make new image and brightinfo structures */
	    imageptr->next = MakeImage(imageptr);
	    imageptr = imageptr->next;
	    brightinfoptr->next = MakeBrightInfo(brightinfoptr);
	    brightinfoptr = brightinfoptr->next;
	    /* get the next image */
	    ProcessNextImage();
	    /* if we are in playback mode, get the front and back diode
	     * positions from the playback file */
	    if ((info.playback) && (readposrec(info.playbackfile, &posrec))) {
		CurrentSpecs.Front.x = brightinfoptr->frontx = posrec.x1;
		CurrentSpecs.Front.y = brightinfoptr->fronty = posrec.y1;
		CurrentSpecs.Back.x = brightinfoptr->backx = posrec.x2;
		CurrentSpecs.Back.y = brightinfoptr->backy = posrec.y2;
	    }
	    /* we stop if we're past tend or if the timestamp is 0, indicating
	     * that we've run out of frames */
	    if ((brightinfoptr->bufferinfo[0] >= tend) || 
	        (brightinfoptr->bufferinfo[0] == 0)) {
		Finish();
	    }
	}
	else {
	    /* move on to the next image and brightinfo structures */
	    imageptr = imageptr->next;
	    brightinfoptr = brightinfoptr->next;
	}
	/* since we just read this in, positionassigned should be 0 */
	if (!info.playback) {
	    positionassigned = 0;
	}
	else {
	    positionassigned = 1;
	    lost = 0;
	}
    }
    /* Update the image */
    UpdatePosImage(brightinfoptr->bufferinfo);

    glutSwapBuffers();

    if (step) {
	step = 0;
	userpause = 1;
    }
    glutTimerFunc((int) (1000 / (speed * 30)), MainDisplay, 0);
    
    return;
}
void Swap(void)
{
    glutSwapBuffers();
}




int ProcessNextImage(void) 
{
    unsigned char *iptr, *rgbptr;
    int i;
    /* this image has no previously assigned bright pixels */
    info.nbright = 0;

    /* decode the new image */
    do {
	state = mpeg2_parse (decoder);
        sequence = mpeginfo->sequence;
        switch (state) {
        case STATE_BUFFER:
            size = fread (buffer, 1, MPEG_BUFFER_SIZE, mpegfile);
            mpeg2_buffer (decoder, buffer, buffer + size);
            break;
	case STATE_SEQUENCE:
	    mpeg2_convert (decoder, mpeg2convert_rgb24, NULL);
            break;
        case STATE_SLICE:
	    break;
        case STATE_END:
        case STATE_INVALID_END:
            break;
        default:
            break;
        }
    } while (((!mpeginfo->display_fbuf) || (state != STATE_PICTURE)) && 
	    !feof(mpegfile));
    if (feof(mpegfile)) {
	return 0;
    }

    /* copy the luminace to the current image */
    memcpy(currentimage, mpeginfo->display_fbuf->buf[0], 
	    info.imagesize * 3);
    rgbptr = currentimage;
    iptr = imageptr->pixels;
    /* convert to black and white */
    for (i = 0; i <  info.imagesize; i++) {
	*(iptr++) = *(rgbptr += 3);
    }


    /* set the timestamp */
    brightinfoptr->bufferinfo[0] = *info.nexttstamp++;

    return 1;
}


void UpdatePosImage(u32 *posbufferinfo)
{
    unsigned char *newcolorptr, *tmpptr;
    int i;
    newcolorptr = imageptr->pixels;
    tmpptr = currentimage;
    for (i = 0; i < info.imagesize; i++) {
        /* assign the new color to the appropriate three elements of the current
         * image */
        *(tmpptr++) = *newcolorptr;
        *(tmpptr++) = *newcolorptr;
        *(tmpptr++) = *(newcolorptr++);
    }
    return;
}

/*function declaration changed in order to take input for purpose of drawing best-fit line*/
void GetBrightPixels(int trackblack)
{
    int 		i, j;
    int			inlist;
    int			pixelnum;
    int			newbright;
    int			*imagebrightptr, *prevbrightptr;
    unsigned short	*newpixelptr;
    int			*tmpbrightptr;
    unsigned char 	*newcolorptr;
    GLubyte		*tmpptr;
    struct Image	*tmpimageptr;
    char		*excludeptr;


    /*Following variables added by TW for purpose of drawing best-fit line*/

    float 	y;
    int 	x, rx, ry, pixnum2;
    GLubyte	*tmpptr2;



    /* threshold the image to get a list of bright pixels */
    info.nbright = 0;
    tmpbrightptr = info.brightpixels;
    tmpptr = imageptr->pixels;
    excludeptr = info.exclude;
    for (i = 0; i < info.imagesize; i++) {
	if (trackblack) {
         if (*(tmpptr++) <= info.thresh) {
               /* check to see if this is an excluded pixel */
               if (*excludeptr == 0) {
                  *(tmpbrightptr++) = i;
                  info.nbright++;
               }
         }
	}
        else {
         if (*(tmpptr++) >= info.thresh) {
               /* check to see if this is an excluded pixel */
               if (*excludeptr == 0) {
                  *(tmpbrightptr++) = i;
                  info.nbright++;
               }
         }
        }
        excludeptr++;
    }

    /* now gow through the list of bright pixels and make them red */
    imagebrightptr = info.brightpixels;
    for (i = 0; i < info.nbright; i++) {
        pixelnum = *imagebrightptr;
	tmpptr = currentimage + 3 * pixelnum;


		/* assign the new color to the appropriate three elements */
		*(tmpptr++) = 255;
		*(tmpptr++) = 0;
		*(tmpptr++) = 0;
	imagebrightptr++;
    }
    return;
}

void GetExcludedPixels()
{
    int 		i, pixelnum;
    GLubyte		*tmpptr;
    struct Image	*tmpimageptr;
    char		*excludeptr;

    tmpptr = currentimage;
    excludeptr = info.exclude;
    /* go through the list of excluded pixels and turn them green */
    for (i = 0; i < info.imagesize; i++) {
	if (*excludeptr) {
	    *(tmpptr++)=0;
	    *(tmpptr++)=255;
	    *(tmpptr++)=0;
	}
	else {
	    tmpptr += 3;
	}
	excludeptr++;
    }
    return;
}

void NewBrightPixels (float *slope, float *b){


    float 	y;
    int 	*imagebrightptr, i, j, x, rx, ry, pixelnum, pixnum2;
    GLubyte	*tmpptr, *tmpptr2;

    imagebrightptr = info.brightpixels;

    /*in case of flag -- (problem with the clustering) -- make all bright
     * pixels green.
     */
    if (flag){
    	for (i = 0; i < info.nbright; i++) {
        	pixelnum = *imagebrightptr;
		tmpptr = currentimage + 3 * pixelnum;
	    	*(tmpptr++)=0;
	    	*(tmpptr++)=255;
	    	*(tmpptr++)=0;
		imagebrightptr++;
	}

    }


     /*this if condition is set for the case in which coordinates have
      *been switched because slope approaches vertical -- this draws a
      *vertical line, through *b, which in this case has returned to be
      *x-intercept.
     */


    if (*slope != 0){

	for (x=10;x<sysinfo.posimagesize[0]-10;x++){
	    y=((*slope) * x) + *b;
	    if (y < sysinfo.posimagesize[1]) {
		ry = (int) floor(y);
		pixnum2 = ry*sysinfo.posimagesize[0]+x;
		tmpptr2 = currentimage + 3*pixnum2;
		*(tmpptr2++) = 255;
		*(tmpptr2++) = 0;
		*(tmpptr2++) = 0;
	    }
        }
    }


    else{
       fprintf(stderr, "drawing vertical line\n");
       rx=(int) floor(*b);
       for (y = 10; y < sysinfo.posimagesize[1] - 10; y++){
	   pixnum2 = (int) (y * sysinfo.posimagesize[0] + rx);
	   tmpptr2 = currentimage + 3 * pixnum2;
	   *(tmpptr2++) = 255;
	   *(tmpptr2++) = 0;
	   *(tmpptr2++) = 0;
	}
    }

    /*assign front bright pixel*/
    tmpptr = currentimage + 3 * front;
    *(tmpptr++) = 0;
    *(tmpptr++)	= 255;
    *(tmpptr++) = 0;

    /*assign back bright pixel*/
    tmpptr = currentimage + 3 * back;
    *(tmpptr++) = 0;
    *(tmpptr++) = 255;
    *(tmpptr++) = 0;

    return;
}

int WritePos(struct BrightInfo *brightptr)
{
    short tmp[4];

    fseek(info.outfile, ftell(info.outfile), SEEK_SET);
    /* write out the position information */
    if (fwrite(brightptr->bufferinfo, sizeof(u32), 1, info.outfile)
	    != 1) {
	fprintf(stderr, "Error: unable to write timestamp to output file\n");
	return 0;
    }
    tmp[0] = round(brightptr->frontx);
    tmp[1] = round(brightptr->fronty);
    tmp[2] = round(brightptr->backx);
    tmp[3] = round(brightptr->backy);
    if (fwrite(tmp, sizeof(short), 4, info.outfile) != 4) {
	fprintf(stderr, "Error: unable to position to output file\n");
	return 0;
    }
    if (brightptr->bufferinfo[0] < lasttime) {
	fprintf(stderr, "Warning: writing out backwards time: current = %ld, last = %ld\n", 
      (long int)brightptr->bufferinfo[0], (long int)lasttime);
    }
    lasttime = brightptr->bufferinfo[0];
    return 1;
}

void Finish(void)
/* write out all of the remaining positions and exit */
{
    while ((firstbrightinfoptr != NULL) && (firstbrightinfoptr->brightpixels != NULL)) {
        WritePos(firstbrightinfoptr);
	firstbrightinfoptr = firstbrightinfoptr->next;
    }
    fclose(info.outfile);
    exit(0);
}

void DisplayImage(GLubyte *currentimage)
{
    glPixelZoom(info.xzoom, info.yzoom);
    glRasterPos2f(0, 0);
    glDrawPixels(sysinfo.posimagesize[0], sysinfo.posimagesize[1], GL_RGB,
                 GL_UNSIGNED_BYTE, currentimage);
    return;
}

void DisplayExcludeBoxes(void)
{
    //displays the hollow green box during the mouse drag to exclude pixels
    int i;

    glColor3f(0.0, 1.0, 0.0);
    if (tmpx1 != 0) {
	glBegin(GL_LINES);
	glVertex3d(tmpx1, tmpy1, 0);
	glVertex3d(tmpx2, tmpy1, 0);
	glVertex3d(tmpx2, tmpy1, 0);
	glVertex3d(tmpx2, tmpy2, 0);
	glVertex3d(tmpx2, tmpy2, 0);
	glVertex3d(tmpx1, tmpy2, 0);
	glVertex3d(tmpx1, tmpy2, 0);
	glVertex3d(tmpx1, tmpy1, 0);
	glEnd();
    }
    glEnd();
    return;
}

void DisplayNBright(void)
{
    char tmpstring[100];

    glColor3f(1.0, 1.0, 1.0);
    sprintf(tmpstring, "NBright: %3d", info.nbright);
    DrawLargeFontString(tmpstring, &wininfo.nbrightloc);
    return;
}

void DisplayThresh(void)
{
    char tmpstring[100];

    glColor3f(1.0, 1.0, 1.0);
    sprintf(tmpstring, "Thresh: %d", (int) info.thresh);
    DrawLargeFontString(tmpstring, &wininfo.threshloc);

    return;
}

void drawCircle(float circx, float circy, float radius, float Red, float Green, float Blue)
{
   //draws a circle with defined position, radius, and color

   float ang;
   glPushMatrix();
   glColor3f(Red, Green, Blue);
   glTranslatef(circx, circy, 0);
   glBegin(GL_LINE_LOOP);
       for(ang=0; ang <= 2*PI; ang += .1) {
           glVertex2d( radius * cos(ang), radius * sin(ang));
       }
   glEnd();

   glPopMatrix();
}

void drawOval(Oval *oval, float Red, float Green, float Blue)
{
   //draws the oval specified in the Oval structure */
   int i;

   glColor3f(Red, Green, Blue);
   glBegin(GL_LINE_LOOP);
   for (i = 0; i < NUM_OVAL_VERTECES; i++) {
       glVertex2d(oval->xcoord[i], oval->ycoord[i]);
   }
   glEnd();
}

void DisplayRatPos(void)
{
    //Draws solid circles during playing video
    glEnable(GL_POINT_SMOOTH);
    glPointSize(10.0f);
    glColor3f(1.0, 1.0, 0.0);
    glBegin(GL_POINTS);
    glVertex3f(CurrentSpecs.Front.x, CurrentSpecs.Front.y, 0.0f);
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(CurrentSpecs.Back.x, CurrentSpecs.Back.y, 0.0f);
    glEnd();
    return;
}

void DisplayTime(void)
{
    char 	*time;

    glColor3f(1.0, 1.0, 1.0);
    time = FormatTS(brightinfoptr->bufferinfo[0]);
    sprintf(tmpstring, "Time: %s", time);
    DrawLargeFontString(tmpstring, &wininfo.timeloc);
    return;
}

char *FormatTS(u32 time)
{
    /* write the time to timestring in hh:mm:ss.s format */
    sprintf(timestring,"%1lu:%02lu:%02lu %04lu", time/36000000L, (time/600000L)%60,
                                                (time/10000L)%60, time%10000L);
    return timestring;
}

void DisplayInputString(char *inputmessage, char *inputstr)
    /* display the message and the current response in the input window */
{
    glColor3f(1.0, 1.0, 1.0);
    strcpy(tmpstring, inputmessage);
    strcat(tmpstring, inputstr);
    DrawLargeFontString(tmpstring, &wininfo.inputloc);
    return;
}

void DrawLargeFontString(char *str, Point *loc)
{
    int len, i;


    glRasterPos2f(loc->x, loc->y);
    len = strlen(str);
    for (i = 0; i < len; i++) {
	glutBitmapCharacter(LARGE_FONT, str[i]);
    }
    return;
}

void DrawSmallFontString(char *str, Point *loc)
{
    int len, i;

    glRasterPos2f(loc->x, loc->y);
    len = strlen(str);
    for (i = 0; i < len; i++) {
	glutBitmapCharacter(SMALL_FONT, str[i]);
    }
    return;
}


/* change camera when window is resized */
void Reshape(int w, int h) {

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(wininfo.left, wininfo.right, wininfo.bottom, wininfo.top, -1000, 1000);
    glViewport(0, 0, w, h);
    wininfo.width = w;
    wininfo.height = h;
    info.xzoom = (float) w / (float) sysinfo.posimagesize[0];
    /* adjust for the negative bottom coordinates and flip the image so that it
     * appears as it does in the camera*/
    info.yzoom = (float) (h * ((float) wininfo.top / (float) (wininfo.top-wininfo.bottom))) / (float) sysinfo.posimagesize[1];
    glMatrixMode(GL_MODELVIEW);

}


void init_menus(void) {

    /* Menu ID's */
    int Main_Menu	=	1;

    Main_Menu = glutCreateMenu(main_menu);
    glutAddMenuEntry("Add Exclude box", 'a');
    glutAddMenuEntry("Clear exclude pixels", 'c');
    glutAddMenuEntry("Remove last exclude box", 'd');
    glutAddMenuEntry("Change exclude oval size", 'r');
    glutAddMenuEntry("Backup one frame", 'b');
    glutAddMenuEntry("Start / Stop Playback", ' ');
    glutAddMenuEntry("Set Threshold", 't');
    glutAddMenuEntry("Set speed up factor", 'f');
    glutAddMenuEntry("Set frame skip", 's');
    glutAddMenuEntry("Skip to next full image", 'S');
    glutAddMenuEntry("Quit", 'Q');
    glutAttachMenu(GLUT_MIDDLE_BUTTON);
}

void main_menu(int item)
{
    int i;

    switch (item) {
	//case ' ':
	    /* set userpause */
	  //  userpause = !userpause;
	    /* turn off single image stepping */
	  //  step = 0;
	  //  break;
	case 'a':
	    /* add a box of excluded pixels */
	    inputchar = 'a';
	    sprintf(inputmessage, "Click and drag to define box (press ret to accept) ");
	    DisplayInputString(inputmessage, inputstr);
	    break;
	case 'c':
	    /* clear the list of excluded pixels and boxes*/
	    for (i = 0; i < info.imagesize; i++) {
		info.exclude[i] = 0;
	    }
	    info.nexcludeboxes = 0;  //reset number of exclude boxes
	    break;
	case 'd':
	    //remove the last exclude box
	    if (info.nexcludeboxes > 0) {
		UnExcludeAllBoxes();
		info.nexcludeboxes--;  //decrease number of boxes by one
		ExcludeAllBoxes();
	    }
	    break;
	case 's':
	    /* prompt the user for the skip number */
	    inputchar = 's';
	    sprintf(inputmessage, "Enter the number of frames to skip ");
	    DisplayInputString(inputmessage, inputstr);
	    break;

	default:
	    Keyboard(item, 0, 0);
	    break;
    }
    return;
}

//Keyboard items can be accessed without going through the menu.  Also, when return is pressed, ProcessInputString is called

void Keyboard(unsigned char key, int x, int y)
{
    int  sumx, sumy;
    int brightcounter,picx, picy;
    int include;
    float pixeldistance, radius;
    int tmp, i;
    char *charptr;
    char tmpstring[TMPSTRINGLEN+1];
    FILE 	*outfile;

    //fprintf(stderr, "key pressed %c %d\n", key, (int) key);
    // inputchar defines which menu item was picked
    if (inputchar != '\0') {
        /* we are in the middle of recieving a string, so add the next character to the
	 * string */
	switch (key) {
	    case 13:
		/* this is the Enter key. If input char is not \0, we have an input string to
		 * process */
		ProcessInputString(inputchar);
		inputstr[0] = '\0';
		inputstrlen = 0;
		inputchar = '\0';
		inputmessage[0] = '\0';
		/* clear the message */
		break;
	    case 8:
		/* the backspace key was pressed, so go back a character */
		if (inputstrlen > 0) inputstr[--inputstrlen] = '\0';
		break;
	    default:
		inputstr[inputstrlen++] = key;
		inputstr[inputstrlen] = '\0';
		break;
        }
    }
    else {
	switch (key) {
	    //if the return key was pressed when no menu item was selected, it is probably because a rat position was just assigned
	    case 13:

		if (userpause) {
		    //make the two structures that contain front and back diode info the same
		    //CurrentSpecs contains the front and back positions from the last valid position, or the newly picked position
		    //from the Mouse function

		    brightinfoptr->frontx = CurrentSpecs.Front.x;
		    brightinfoptr->fronty = CurrentSpecs.Front.y;
		    brightinfoptr->backx = CurrentSpecs.Back.x;
		    brightinfoptr->backy = CurrentSpecs.Back.y;
		    LastFSDefinedSpecs = LastValidSpecs = CurrentSpecs;

		    userinput = 0;
		    userpause = 0;
		    readnextimage = 1;
		    positionassigned = 1;
		    nskipped = 0;
		    flag = 0;
		    lastdir = atan2(brightinfoptr->fronty - brightinfoptr->backy,
				    brightinfoptr->frontx - brightinfoptr->backx);
		    //update the exclude circle position, which is also the midpoint between the front and back diodes
		    circx = (brightinfoptr->frontx + brightinfoptr->backx)/2;
		    circy = (brightinfoptr->fronty + brightinfoptr->backy)/2;

		    /*This for loop takes brightpixels, which is a list of positions from 1 to 76,800
		    *320*240 array compressed into 1-D, and assigns these positions into 2-D values (picx and picy)
		    *First position in this array is (0,0)...sysinfo.posimagesize[0] gives number of columns.
		    we do this here in order to calculate the center of mass of the bright pixels
		    */
		    sumx = 0;
		    sumy = 0;
		    brightcounter = 0;
		    radius = MAX(tmpoval.majoraxis / 2, tmpoval.minoraxis / 2);
		    for (i = 0; i < info.nbright; i++) {
			picy=info.brightpixels[i]/sysinfo.posimagesize[0];
			picx=info.brightpixels[i]%sysinfo.posimagesize[0];
			pixeldistance = sqrt(sqr(picx - tmpoval.xcenter) + 
					     sqr(picy - tmpoval.ycenter));

			//fprintf(stderr, "%f ", pixeldistance);
			include = 0;
			if (radius == 0) {
			    /* there is no exclude oval */
			    include = 1;
			}
			else if (pixeldistance < radius) {
			    /* we're within a circle the size of the major axis, so check to se
			     * if we're inside the polygon */
			    if (IsInPolygon(picx, picy, tmpoval.xcoord,
					tmpoval.ycoord, NUM_OVAL_VERTECES))
				include = 1;
			}
			if (include) {
			    sumx += picx;
			    sumy += picy;
			    brightcounter++;
			}
		    }

		    if (brightcounter > 0) {
			CurrentSpecs.CenterOfMass.x = sumx/brightcounter;
			CurrentSpecs.CenterOfMass.y = sumy/brightcounter;
		    }
		    else {
			CurrentSpecs.CenterOfMass.x = tmpoval.xcenter;
			CurrentSpecs.CenterOfMass.y = tmpoval.ycenter;
		    }
		    CurrentSpecs.Center.x = tmpoval.xcenter;
		    CurrentSpecs.Center.y = tmpoval.ycenter;
		    break;
		}
	    case ' ':
		/* space bar was hit, so pause or unpause the display */
		userpause = !userpause;
		/* reset step */
		step = 0;
		break;
	    case 'c':
		/* clear the list of excluded pixels and boxes*/
		for (i = 0; i < info.imagesize; i++) {
		    info.exclude[i] = 0;
		}
		break;
	    case 'd':
		dumptofile = 1;
		break;
	    case 'b':
		/* go back a frame */
		userpause = 1;
		step = 1;

		if (imageptr->prev != NULL) {
		    imageptr = imageptr->prev;
		    brightinfoptr = brightinfoptr->prev;
		    imagenum--;
		    UpdatePosImage(brightinfoptr->bufferinfo);
		    positionassigned = 1;
		    CurrentSpecs.Front.x = brightinfoptr->frontx;
		    CurrentSpecs.Front.y = brightinfoptr->fronty;
		    CurrentSpecs.Back.x = brightinfoptr->backx;
		    CurrentSpecs.Back.y = brightinfoptr->backy;
		    LastFSDefinedSpecs = LastValidSpecs = CurrentSpecs;
		}
		break;
	    case 'B':
		/* go back to the first saved frame */

		imageptr = firstimageptr;
		brightinfoptr = firstbrightinfoptr;
		imagenum = 0;
		UpdatePosImage(brightinfoptr->bufferinfo);
		positionassigned = 1;
		CurrentSpecs.Front.x = brightinfoptr->frontx;
		CurrentSpecs.Front.y = brightinfoptr->fronty;
		CurrentSpecs.Back.x = brightinfoptr->backx;
		CurrentSpecs.Back.y = brightinfoptr->backy;
		LastFSDefinedSpecs = LastValidSpecs = CurrentSpecs;
		break;
	    case 'n':
		/* step through a single image */
		userpause = 0;

		step = 1;
		break;
	    case 'Q':
	    	Finish();
		break;
	    case 't':
		inputchar = 't';
		sprintf(inputmessage, "Enter the new threshold: ");
		break;
	    case 'r':
		inputchar = 'r';
	        sprintf(inputmessage, "Current oval axes: %0.0f, %0.0f, New size (zero for off): ", origoval.majoraxis, origoval.minoraxis);
	        break;
	    case 'f':
		inputchar = 'f';
	        sprintf(inputmessage, "Current speed: %f, New speed: ", speed);
	        break;	    
            case 'S':
		getnextfullimage = 1;
		break;
	    case 'x':
		/* start adding pixels to the exclude list */
		inputchar = 'x';
		sprintf(inputmessage, "Click on the pixels to be excluded (press ret when done) ");
		break;
	    case '\'':

		CurrentSpecs = LastFSDefinedSpecs;
		brightinfoptr->frontx = CurrentSpecs.Front.x;
		brightinfoptr->fronty = CurrentSpecs.Front.y;
		brightinfoptr->backx = CurrentSpecs.Back.x;
		brightinfoptr->backy = CurrentSpecs.Back.y;
		break;

	    default:
		//fprintf(stderr, "key pressed %c %d\n", key, (int) key);
		break;
	}
    }
    return;
}

void ExcludeAllBoxes()  //excludes all boxes in the list
{
   if (info.nexcludeboxes > 0) {
        int tmp = info.nexcludeboxes - 1;
	ExcludeBoxes(0,tmp);
   }
}

void UnExcludeAllBoxes() //unexcludes the boxes from the list
{
   if (info.nexcludeboxes > 0) {
        int tmp = info.nexcludeboxes - 1;
	UnExcludeBoxes(0,tmp);
   }
}

void UnExcludeBoxes(int startbox, int endbox) //unexcludes the selected boxes
{
   int tmp = startbox;
   int i,j;
   if (info.nexcludeboxes > 0) {
     while(tmp <= endbox) {
	for (i = info.excludeboxxbot[tmp]; i < info.excludeboxxtop[tmp]; i++) {
		for (j = info.excludeboxybot[tmp]; j < info.excludeboxytop[tmp]; j++) {
			if (j * PIXELS_PER_LINE + i < info.imagesize) {
			info.exclude[j * PIXELS_PER_LINE + i] = 0;
			}
		}
	}
        tmp++;
     }
   }
     return;
}

void ExcludeBoxes(int startbox, int endbox) //excludes the selected boxes
{
   int tmp = startbox;
   int i,j;
   if (info.nexcludeboxes > 0) {
     while(tmp <= endbox) {
	for (i = info.excludeboxxbot[tmp]; i < info.excludeboxxtop[tmp]; i++) {
		for (j = info.excludeboxybot[tmp]; j < info.excludeboxytop[tmp]; j++) {
			if (j * PIXELS_PER_LINE + i < info.imagesize) {
			info.exclude[j * PIXELS_PER_LINE + i] = 1;
			}
		}
	}
        tmp++;
     }
   }
     return;
}

void ProcessInputString(unsigned char inputchar)
    /* given the type of command specified by inputchar, process the input string */
{
    int  i, j, tmp, index, sumx, sumy;
    int brightcounter,picx, picy;
    float pixeldistance;
    float ftmp, ftmp2;



    switch(inputchar) {

    case 'a':
	/* add the box to the list */
	tmp = info.nexcludeboxes;
	if (tmpx1 > tmpx2) {
	    info.excludeboxxtop[tmp] = tmpx1;
	    info.excludeboxxbot[tmp] = tmpx2;
	}
	else {
	    info.excludeboxxtop[tmp] = tmpx2;
	    info.excludeboxxbot[tmp] = tmpx1;
	}
	if (tmpy1 > tmpy2) {
	    info.excludeboxytop[tmp] = tmpy1;
	    info.excludeboxybot[tmp] = tmpy2;
	}
	else {
	    info.excludeboxytop[tmp] = tmpy2;
	    info.excludeboxybot[tmp] = tmpy1;
	}

	tmpx1 = tmpy1 = tmpx2 = tmpy2 = 0;
	info.nexcludeboxes++;
	ExcludeAllBoxes();
	break;
    case 't':
        /* get the new threshold */
        tmp = atoi(inputstr);
	if ((tmp >= 0) && (tmp <= 255)) {
	    info.thresh = tmp;
	}
	else {
	    printf("Error: %d out of bounds for threshold\n", tmp);
        }
	break;

    case 's':
	//new frame skip
	tmp = atoi(inputstr);
	if (tmp >= 0) {
	    info.skip = tmp;
	}
	break;
    case 'r':
	//new exclude oval size
	sscanf(inputstr, "%f%f", &ftmp, &ftmp2);
	if ((ftmp > 0) && (ftmp2 > 0)) {
	    origoval.majoraxis = ftmp;
	    origoval.minoraxis = ftmp2;
	    GetOvalVerteces(&origoval);
	    GetTmpOval(&origoval, &tmpoval);
	}
	break;
    case 'f':
	//new speed
	ftmp = atof(inputstr);
	if (ftmp >= 0) {
	   speed = ftmp;
	}
	break;
    default:
    	break;
    }



    return;
}

void Mouse(int button, int state, int x, int y)
/* This function allows the user to indicate the locations of the front and
 * back diodes, and to define the exclude boxes */
{
    int i, j, xcoord, ycoord, index;
    int inlist = 0;
    float tempdist;
    GLubyte	*tmpptr;

    //fprintf(stderr, "mouse button %d state %d at %d, %d, inputchar = %c\n", button, state, x, y, inputchar);
    /* check to see if the user clicked in the image window when the program
     * was paused */
    if ((button == GLUT_LEFT_BUTTON) && (y <= round(((float) wininfo.height /
                        ((wininfo.top - wininfo.bottom) / wininfo.top))))) {
	WinlocToImage(x, y, &xcoord, &ycoord);
	if (state == GLUT_DOWN) {
	    switch (inputchar) {

	    case 'x' :
		/* if the clicked pixel is not in the list of excluded
		 * pixels, add it to the list, and if it is in the list,
		 * remove it */
		index = ycoord * PIXELS_PER_LINE + xcoord;
		info.exclude[index] = !info.exclude[index];
		//GetBrightPixels();
		break;
	    case 'a':
		/* This mouse press gives the upper right or lower left corner
		 * of an exclude box */
		WindowToWinloc(x, y, &tmpx1, &tmpy1);
		tmpx2 = tmpx1;
		tmpy2 = tmpy1;
		//fprintf(stderr, "new box at %d %d, mouse %d %d\n", tmpx1, tmpy1, x, y);
		break;
	    case '\0':
		//If no menu item was selected, then the user is probably defining the front and back diodes
		//the left button is used for the front diode
		if ((userinput)||(userpause)) {
			// use the clicked coordinate as the location of the front diode
			CurrentSpecs.Front.x = brightinfoptr->frontx = xcoord;
			CurrentSpecs.Front.y = brightinfoptr->fronty = ycoord;
			if ((trackblack)||(numlights == 1)) {
                        
                            CurrentSpecs.Back.x = brightinfoptr->backx = xcoord;
			    CurrentSpecs.Back.y = brightinfoptr->backy = ycoord;
                        }
                        
                        //fprintf(stderr,"mouse: %d, %d\n",(int)(CurrentSpecs.Front.x), (int)(CurrentSpecs.Front.y));
			if (CurrentSpecs.Back.x == 0) {
				CurrentSpecs.Center = CurrentSpecs.Front;
			}
			else {
				CurrentSpecs.Center.x = ((CurrentSpecs.Front.x + CurrentSpecs.Back.x)/2);
				CurrentSpecs.Center.y = ((CurrentSpecs.Front.y + CurrentSpecs.Back.y)/2);
			}
			if (userpause) {
				positionassigned = 1;

			}
		}
		break;
	    default:
		break;
	    }

	}


    }
    else if ((button == GLUT_RIGHT_BUTTON) && (y <= round(((float) wininfo.height /
                        ((wininfo.top - wininfo.bottom) / wininfo.top))))) {
	WinlocToImage(x, y, &xcoord, &ycoord);
	if (state == GLUT_DOWN) {
		switch (inputchar) {
		case '\0':
			//the right button is used for the back diode
			if ((userinput)||(userpause)) {
				if ((trackblack)||(numlights == 1)) {
                                
                                    CurrentSpecs.Front.x = brightinfoptr->frontx = xcoord;
			            CurrentSpecs.Front.y = brightinfoptr->fronty = ycoord;
                                }
                                CurrentSpecs.Back.x = brightinfoptr->backx = xcoord;
				CurrentSpecs.Back.y = brightinfoptr->backy = ycoord;
				if (CurrentSpecs.Front.x == 0) {
					CurrentSpecs.Center = CurrentSpecs.Back;
				}
				else {
					CurrentSpecs.Center.x = ((CurrentSpecs.Front.x + CurrentSpecs.Back.x)/2);
					CurrentSpecs.Center.y = ((CurrentSpecs.Front.y + CurrentSpecs.Back.y)/2);
				}
				if (userpause) {
					positionassigned = 1;

				}
			}

			break;
		default:
			break;
		}


	}
    }
    return;
}

void MouseMotion(int x, int y)
    /* if we are defining an exclusion box, this sets the coordinates for the
     * box */
{
    int xwinloc, ywinloc;

    if (inputchar == 'a') {
	WindowToWinloc(x, y, &xwinloc, &ywinloc);
	tmpx2 = (int) MIN(xwinloc, wininfo.right);
	tmpx2 = (int) MAX(tmpx2, wininfo.left);
	tmpy2 = (int) MIN(ywinloc, wininfo.top);
	tmpy2 = (int) MAX(tmpy2, 0);
    }
    return;
}

struct Image *MakeImage(struct Image *lastimage)
/* create a new Image structure and set it's prev pointer to lastimage */
{
    struct Image 	*newimage;

    newimage = (struct Image *) malloc(sizeof(struct Image));
    newimage->prev = lastimage;
    newimage->next = NULL;
    newimage->pixels = (unsigned char *) calloc(info.imagesize, sizeof(unsigned char));
    return newimage;
}

void DeleteImage(struct Image *currentimage)
/* delete the current image and set the next image's prev pointer to NULL */
{
   struct Image *tmpimage;

   tmpimage = currentimage->next;
   tmpimage->prev = NULL;
   free(currentimage->pixels);
   free(currentimage);
   return;
}

struct BrightInfo *MakeBrightInfo(struct BrightInfo *last)
/* create a new BrightInfo structure and set it's prev pointer to last*/
{
    struct BrightInfo 	*newbright;

    newbright = (struct BrightInfo *) malloc(sizeof(struct BrightInfo));
    newbright->prev = last;
    newbright->next = NULL;
    newbright->brightpixels = NULL;
    /* TEST */
    newbright->frontx = -1;
    newbright->fronty = -1;
    newbright->backx = -1;
    newbright->backy = -1;
    /* TEST */
    newbright->bufferinfo[0] = newbright->bufferinfo[1] = 0;
    return newbright;
}

void DeleteBright(struct BrightInfo *bright)
/* delete the current brightinfo ptr and set the next image's prev pointer to NULL */
{
   struct BrightInfo *tmpbright;

   tmpbright = bright->next;
   tmpbright->prev = NULL;
   free(bright->brightpixels);
   free(bright);
   return;
}

void WinlocToImage(int winx, int winy, int *imagex, int *imagey)
{
   *imagex = round((float) winx / info.xzoom);
   *imagey =  round((((float) wininfo.height /
				((wininfo.top - wininfo.bottom) /
			          wininfo.top)) - winy) / info.yzoom);
    return;
}
void WindowToWinloc(int winx, int winy, int *winlocx, int *winlocy)
{
   *winlocx = round((float) winx * ((float) (wininfo.right -
		   wininfo.left)) / ((float) wininfo.width));
   *winlocy = round((float) (winy - wininfo.height) *
	            ((float) (wininfo.top - wininfo.bottom) /
		    ((float) (-wininfo.height))) + wininfo.bottom);
    return;
}

void GetOvalVerteces(Oval *oval)
    /* calculate the verteces of a vertically oriented oval with the major and
     * minor axes in the oval structure */

{
    int i;
    float angle;

    for (i = 0; i < NUM_OVAL_VERTECES; i++) {
	angle = i * TWOPI / NUM_OVAL_VERTECES;
	oval->xcoord[i] = oval->majoraxis / 2 * cos(angle);
	oval->ycoord[i] = oval->minoraxis / 2 * sin(angle);
    }
}

void GetTmpOval(Oval *origoval, Oval *tmpoval)
    /* rotates the oval by the angle in oval->angle */
{
    int i;
    float rotmat[2][2];
    float tmpx, tmpy;

    tmpoval->majoraxis = origoval->majoraxis;
    tmpoval->minoraxis = origoval->minoraxis;
    /* calculate the rotation matrix */
    rotmat[0][0] = rotmat[1][1] = cos(tmpoval->angle);
    rotmat[0][1] = sin(tmpoval->angle);
    rotmat[1][0] = -rotmat[0][1];

    /* apply the matrix */
    for (i = 0; i < NUM_OVAL_VERTECES; i++) {
	tmpx = origoval->xcoord[i];
	tmpy = origoval->ycoord[i];
	tmpoval->xcoord[i] = tmpoval->xcenter + tmpx * rotmat[0][0] + 
			    tmpy * rotmat[1][0];
	tmpoval->ycoord[i] = tmpoval->ycenter + tmpx * rotmat[0][1] + 
			    tmpy * rotmat[1][1];
    }
}

int readposrec(FILE *file, PosRec *posrec)
   /* returns 0 on error */
{
   u32 time;
   short s[4];

   if (fread(&time, sizeof(u32), 1, file) != 1)
       return 0;
   if (fread(s, sizeof(short), 4, file) != 4)
       return 0;

   posrec->timestamp = time;
   posrec->x1 = s[0];
   posrec->y1 = s[1];
   posrec->x2 = s[2];
   posrec->y2 = s[3];
   return 1;
}

