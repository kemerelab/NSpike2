#ifndef __SPIKE_V4LCAPTURE_H__
#define __SPIKE_V4LCAPTURE_H__

//#define NO_NSPIKE_DEBUG    // stand-alone version

#ifdef NO_NSPIKE_DEBUG
  #define BYTES_PER_PIXEL  2  // should also be defined in spike_defines.h
#else  
  #include "spike_position.h"  // PIXELS_PER_LINE LINES_PER_FIELD
  #include "spike_defines.h"   //  #define BYTES_PER_PIXEL  2
#endif


extern void open_device(const char * video_dev_name, int * pvideo_fd);
extern void init_device( int inputnum, int width, int height );
extern void start_capturing ();
extern int read_frame( unsigned char *outbuf, long int * sec, long int * usec );
extern void stop_capturing();
extern void uninit_device();
extern void close_device();
extern void YUY422to422P( unsigned char *inbuf, 
                        unsigned char *outY, unsigned char *outU, unsigned char *outV );
extern void YUY422to420P( unsigned char *inbuf, 
                        unsigned char *outY, unsigned char *outU, unsigned char *outV );

#endif  // #ifndef __SPIKE_V4LCAPTURE_H__

