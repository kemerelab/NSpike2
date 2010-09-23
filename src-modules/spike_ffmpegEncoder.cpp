#include <stdio.h>
#include <stdlib.h>

// tell the compiler thatffmpeg is strictly C
// otherwise the linker will fail
extern "C" {

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
// #include <avcodec.h>
// #include <mathematics.h>
}

#include "spike_ffmpegEncoder.h"

#define FRAMES_PER_SEC 30

// poor man's class member vars
AVCodecContext *x_pContext = NULL;
AVFrame *x_pAVFrame = NULL;
unsigned char * x_pEncodeBuf;
int x_iEncodeBufSize;


// MPEG-1 supports YUV420P format only.
// Have to convert before encoding
void YUV422PtoYUV420P(){
  int iPix, nPix;
  int iLine, nLines;
  int iU, iV;

  unsigned char * pUin1;
  unsigned char * pUin2;
  unsigned char * pUout;

  unsigned char * pVin1;
  unsigned char * pVin2;
  unsigned char * pVout;

  nPix = x_pContext->width/2;
  nLines = x_pContext->height/2;

  pUout = x_pAVFrame->data[1];
  pUin1 = pUout;
  pUin2 = pUout + nPix;

  pVout = x_pAVFrame->data[2];
  pVin1 = pVout;
  pVin2 = pVout + nPix;
  

  for( iLine = 0; iLine < nLines; iLine++){
    for( iPix = 0; iPix < nPix; iPix++){
      iU = *pUin1++ + *pUin2++;
      *pUout++ = iU/2;
      iV = *pVin1++ + *pVin2++;
      *pVout++ = iV/2;
    }
    // jump over the next line
    pUin1+= nPix;
    pUin2+= nPix;
    pVin1+= nPix;
    pVin2+= nPix;
  }

}


int open_encoder( int iCodecID, int iWidth, int iHeight, 
                  int iQuant, // quantization parameter 1=best quality, 31-worst
                  int iGopSize, // Group of frames size, i.e. n frames between I frames. 
                                // Can be set to 0 to have mjpeg-like coding
                                // Ignored by MJPEG
                  unsigned char * pYbuf,  //input plane
                  unsigned char * pUbuf,  //input plane
                  unsigned char * pVbuf,  //input plane
                  unsigned char * pEncodeBuf,  //output buffer
                  int iEncodeBufSize  ){       //output buffer size
                  

  int FFcodecID;
  AVCodec *pCodec = NULL;

  // must be called before using avcodec lib 
  avcodec_init();
  // register all codecs
  avcodec_register_all();
  x_pContext= avcodec_alloc_context();
  x_pAVFrame= avcodec_alloc_frame();

  switch (iCodecID) {

    case MPEG1_CODEC:
      FFcodecID = CODEC_ID_MPEG1VIDEO;
      x_pContext->pix_fmt = PIX_FMT_YUV420P;
    break;

    case MPEG2_CODEC:
      FFcodecID = CODEC_ID_MPEG2VIDEO;
      x_pContext->pix_fmt = PIX_FMT_YUV422P;
    break;

    case MJPEG_CODEC:
      FFcodecID = CODEC_ID_MJPEG;
      x_pContext->pix_fmt = PIX_FMT_YUVJ422P;
    break;

    default:
      fprintf(stderr, "FFMPG: Codec not supported yet!\n");
      exit(1);
  }

  x_pContext->qmin = iQuant;  // min compression: 1 to 31 range  
  x_pContext->qmax = iQuant;  // max compression: 1 to 31 range

  // resolution must be a multiple of two 
  x_pContext->width = iWidth;
  x_pContext->height = iHeight;
  // time base = 1/frames per second
  x_pContext->time_base= (AVRational){1,FRAMES_PER_SEC};
  // group of frames size: 0 means I - frames only, like mjpg
  x_pContext->gop_size = iGopSize; 

  pCodec = avcodec_find_encoder(  (CodecID) FFcodecID);
  if (!pCodec) {
    fprintf(stderr, "FFMPG: Codec not found\n");
    exit(1);
  }

  // the context is ready - open the codec 
  if ( avcodec_open(x_pContext, pCodec) < 0) {
      fprintf(stderr, "FFMPG: could not open encoder\n");
      exit(1);
  }

  x_pAVFrame->data[0] = pYbuf;
  x_pAVFrame->linesize[0] = iWidth;
  x_pAVFrame->data[1] = pUbuf;
  x_pAVFrame->linesize[1] = iWidth/2;
  x_pAVFrame->data[2] = pVbuf;
  x_pAVFrame->linesize[2] = iWidth/2;
  
  x_pEncodeBuf = pEncodeBuf;
  x_iEncodeBufSize = iEncodeBufSize;
  fprintf(stderr, "FFMPG: Opened encoder: %s\n", pCodec->name);
  return 0;
}

// assumes YUV422P input format
// returns size of encoded chunk
// Note: encode_frame() can modify input buffers !!!
int encode_frame( ){
  int iEncodedSize;

  if( x_pContext->pix_fmt == PIX_FMT_YUV420P) {
    YUV422PtoYUV420P(); // convert input buf
  }
  // encode the frame, output to the 
  iEncodedSize = avcodec_encode_video( x_pContext, 
                                       x_pEncodeBuf, x_iEncodeBufSize, x_pAVFrame);

  return iEncodedSize;
}

// finishes encoding of buffered frames
// returns size of encoded chunk
// not needed in MJPEG, but does not hurt
// call in the end of encoding until returned size is zero
int encode_delayed_frames( ){
  int iEncodedSize;

  iEncodedSize = avcodec_encode_video(x_pContext, 
                                      x_pEncodeBuf, x_iEncodeBufSize, NULL);
  return iEncodedSize;
}

int close_encoder(){

  avcodec_close(x_pContext);
  av_free(x_pContext);
  av_free(x_pAVFrame);
  fprintf(stderr, "FFMPG: Closed encoder\n");

  return 0;
} 
 
