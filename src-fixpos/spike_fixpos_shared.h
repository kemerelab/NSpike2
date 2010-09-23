typedef struct _InterpInfo {
    gzFile		timestampfile;
    char 		timestampfilename[200];
    FILE		*outfile;
    unsigned char	thresh; // the current threshold for bright pixel detection
    int			imagesize;
    float		xzoom;
    float		yzoom;
    int			overlay;
    int			skip;
    int			nbright;
    int			*brightpixels;
    u32 		*timestamp;
    u32			*nexttstamp;
    int			ntstamps;
    int			totalsize;
    char		*frame;
    char		exclude[PIXELS_PER_LINE * LINES_PER_FIELD];
    int 		nexcludeboxes;
    short		excludeboxxbot[MAX_EXCLUDE_BOXES];
    short		excludeboxxtop[MAX_EXCLUDE_BOXES];
    short		excludeboxybot[MAX_EXCLUDE_BOXES];
    short		excludeboxytop[MAX_EXCLUDE_BOXES];
} InterpInfo;

