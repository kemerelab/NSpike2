#define INIT_WINDOW_WIDTH   640
#define INIT_WINDOW_HEIGHT  480
#define MAX_EXCLUDE_BOXES	100
#define NSAVEDIMAGES   1200	// save the last 1200 images
#define LARGE_FONT      GLUT_BITMAP_HELVETICA_18
#define SMALL_FONT      GLUT_BITMAP_HELVETICA_12

#define BUF_LENGTH	80000	
#define MAX_FRAMES	1000000
#define NUM_OVAL_VERTECES	16

typedef struct _DiodePosInfo {
    Point	CenterOfMass;
    Point	Center;
    Point	Front;
    Point	Back;
} DiodePosInfo;

typedef struct _PosRec {
    u32		timestamp;
    short	x1;
    short	y1;
    short	x2;
    short	y2;
} PosRec;

typedef struct _InterpInfo {
    gzFile		timestampfile;
    char 		timestampfilename[200];
    FILE		*outfile;
    FILE		*offsetfile;
    FILE		*playbackfile;
    int			append;
    int			playback;
    unsigned char	thresh; // the current threshold for bright pixel detection
    int			imagesize;
    int			rgbimagesize;
    float		xzoom;
    float		yzoom;
    int			overlay;
    int			skip;
    int			nbright;
    int			*brightpixels;
    u32 		*timestamp;
    u32			*nexttstamp;
    u32			*offset;
    unsigned long long	*offset64;
    int			first; // 1 if this is the first image
    int			ntstamps;
    int			totalsize;
    char		exclude[PIXELS_PER_LINE * LINES_PER_FIELD];
    int 		nexcludeboxes;
    short		excludeboxxbot[MAX_EXCLUDE_BOXES];
    short		excludeboxxtop[MAX_EXCLUDE_BOXES];
    short		excludeboxybot[MAX_EXCLUDE_BOXES];
    short		excludeboxytop[MAX_EXCLUDE_BOXES];
} InterpInfo;


typedef struct _Oval {
    float angle;
    float xcenter;
    float ycenter;
    float majoraxis;
    float minoraxis;
    float xcoord[NUM_OVAL_VERTECES];
    float ycoord[NUM_OVAL_VERTECES];
} Oval;
