#ifndef __SPIKE_FFMPEGENCODER_H__
#define __SPIKE_FFMPEGENCODER_H__

//#define NO_NSPIKE_DEBUG    // stand-alone version

#ifdef NO_NSPIKE_DEBUG
  #define MPEG1_CODEC     1
  #define MPEG2_CODEC     2
  #define MJPEG_CODEC     3
#else  
  #include "spike_position.h"  // PIXELS_PER_LINE LINES_PER_FIELD
  #include "spike_defines.h"   //  #define BYTES_PER_PIXEL  2
#endif



int open_encoder( int iCodecID, int iWidth, int iHeight, 
                  int iQuant, // quantization parameter 1=best quality, 31-worst
                  int iGopSize, // Group of frames size, i.e. n frames between I frames. 
                                // Can be set to 0 to have mjpeg-like coding
                                // Ignored by MJPEG
                  unsigned char * pYbuf,  //input plane
                  unsigned char * pUbuf,  //input plane
                  unsigned char * pVbuf,  //input plane
                  unsigned char * pEncodeBuf,  //output buffer
                  int iEncodeBufSize  );       //output buffer size


int encode_frame( );

int encode_delayed_frames( );

int close_encoder();

#endif  // #ifndef __SPIKE_FFMPEGENCODER_H__

