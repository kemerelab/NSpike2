/***************************************************************************
 *   Copyright (C) 2006 by EB (yfh2@xxxxxxxxxxx)         *
 *                   *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.           *
 *                   *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of  *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   *
 *   GNU General Public License for more details.        *
 *                   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the       *
 *   Free Software Foundation, Inc.,               *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.       *

***************************************************************************/

#include "audio.h"

#include <alsa/asoundlib.h>
#include <QSound>
#include <QThread>
#include <QMessageBox>

#if WORDS_BIGENDIAN
#define SwapLE16(x) ((((u_int16_t)x)<<8)|(((u_int16_t)x)>>8))
#define SwapLE32(x) ((((u_int32_t)x)<<24)|((((u_int32_t)x)<<8)&0x00FF0000) \
         |((((u_int32_t)x)>>8)&0x0000FF00)|(((u_int32_t)x)>>24))
#else
#define SwapLE16(x) (x)
#define SwapLE32(x) (x)
#endif



#include <alsa/asoundlib.h>

#define   AUDIOBUFFERSIZE        1024

QAlsaSound::QAlsaSound(int soundNum, const QString& filename, QObject* parent):
  QSound(filename, parent), soundNum(soundNum)
{

  Path = filename;
  is_available = initialise();


}



bool QAlsaSound::initialise()
{

  if (QSound::isAvailable())
    return TRUE ; 

#ifdef Q_OS_LINUX
  char   buffer [ AUDIOBUFFERSIZE ] ;

  //device = strdup("plughw:0,0");      /* playback device */
  device = strdup("default");     /* playback device */


  char*   ptr ;
  u_long  databytes ;
  snd_pcm_format_t format;
  snd_pcm_hw_params_t *params;

  int err;

  /*
   * Open the file for reading:
   */

  //Path = "/home/loren/Src/NSpike2/sounds/tone3000right.wav";
  //  Path ="/home/lorenlab/gideon/NSpike2/sounds/longSong.wav";


  //Path="/home/eb/Documents/travail.wav";

  QMessageBox msgBox;
  QString errorMessage;
  if ( (fd = open(Path.toLatin1().constData(),O_RDONLY)) < 0 ) {
    errorMessage = QString("Error Opening WAV file %1").arg(Path.toLatin1().constData());
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;
  }

  if ( lseek(fd,0L,SEEK_SET) != 0L ) {
    errorMessage = QString("Error Rewinding WAVE file %1").arg(Path.toLatin1().constData());
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;     /* Wav file must be seekable device */
  }


  read (fd, buffer, AUDIOBUFFERSIZE) ;

  if (findchunk (buffer, "RIFF", AUDIOBUFFERSIZE) != buffer) {
    errorMessage = QString("%1: Bad format: Cannot find RIFF file marker").arg(Path.toLatin1().constData());
    msgBox.setText(errorMessage);
    msgBox.exec();
    return  FALSE ;
  }

  if (! findchunk (buffer, "WAVE", AUDIOBUFFERSIZE)) {
    errorMessage = QString("%1: Bad format: Cannot find WAVE file marker").arg(Path.toLatin1().constData());
    msgBox.setText(errorMessage);
    msgBox.exec();
    return  FALSE ;
  }

  ptr = findchunk (buffer, "fmt ", AUDIOBUFFERSIZE) ;

  if (! ptr) {
    errorMessage = QString("%1: Bad format: Cannot find 'fmt' file marker").arg(Path.toLatin1().constData());
    msgBox.setText(errorMessage);
    msgBox.exec();
    return  FALSE ;
  }

  ptr += 4 ;      /* Move past "fmt ".*/
  memcpy (&waveformat, ptr, sizeof (WAVEFORMAT)) ;
  waveformat.dwSize = SwapLE32(waveformat.dwSize);
  waveformat.wFormatTag = SwapLE16(waveformat.wFormatTag) ;
  waveformat.wChannels = SwapLE16(waveformat.wChannels) ;
  waveformat.dwSamplesPerSec = SwapLE32(waveformat.dwSamplesPerSec) ;
  waveformat.dwAvgBytesPerSec = SwapLE32(waveformat.dwAvgBytesPerSec) ;
  waveformat.wBlockAlign = SwapLE16(waveformat.wBlockAlign) ;
  waveformat.wBitsPerSample = SwapLE16(waveformat.wBitsPerSample) ;

  
  ptr = findchunk (buffer, "data", AUDIOBUFFERSIZE) ;

  if (! ptr) {
    errorMessage = QString("%1: Bad format: Cannot find 'data' file marker").arg(Path.toLatin1().constData());
    msgBox.setText(errorMessage);
    msgBox.exec();
    return  FALSE ;
  }

  ptr += 4 ;      /* Move past "data".*/
  memcpy (&databytes, ptr, sizeof (u_long)) ;

  samples    = databytes / waveformat.wBlockAlign ;
  datastart  = ((u_long) (ptr + 4)) - ((u_long) (&(buffer[0]))) ;

  switch (waveformat.wBitsPerSample)
  {
    case 8:
      format = SND_PCM_FORMAT_U8 ;
      break;
    case 16:  
      format = SND_PCM_FORMAT_S16_LE ;
      break;
    case 32 :
      format = SND_PCM_FORMAT_S32_LE;
      break;
    default :
	errorMessage = QString("%1: Bad format: %2 bits per second").arg(Path.toLatin1().constData()).arg(waveformat.wBitsPerSample); 
	msgBox.setText(errorMessage);
	msgBox.exec();
      return  FALSE ;
      break;
  }

  //fprintf(stdout,"%s - format :%d, %i Hz, %i channels \n",Path.latin (),waveformat.wBitsPerSample, waveformat.dwSamplesPerSec, waveformat.wChannels);

  /*
   *      ALSA pain
   */
  snd_pcm_hw_params_alloca(&params);
  

  if ((err = snd_pcm_open (&handle, device, SND_PCM_STREAM_PLAYBACK,
		  SND_PCM_ASYNC)) < 0) {
//		  SND_PCM_NONBLOCK)) < 0) {
      errorMessage = QString("Cannot open audio device %s (%s)").arg(device).arg(snd_strerror (err));
      msgBox.setText(errorMessage);
      msgBox.exec();
      return FALSE;
    }

//  if ((err = snd_pcm_nonblock(handle, 1))< 0) {
//      fprintf (stdout,"nonblock setting error: %s", snd_strerror(err));
//      return FALSE;
//  }
  

    /* Init hwparams with full configuration space */
  if (snd_pcm_hw_params_any(handle, params) < 0) {
    errorMessage = QString("Cannot configure PCM device");
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;
    }


  err = snd_pcm_hw_params_set_access(handle, params, 
	  SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    errorMessage = QString("Access type not available");
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;
  }

  err = snd_pcm_hw_params_set_format(handle, params, format);
  if (err < 0) {
    errorMessage = QString("Sample format type not available");
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;
  }

  err = snd_pcm_hw_params_set_channels(handle, params, waveformat.wChannels);
  if (err < 0) {
    errorMessage = QString("Channel count not available");
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;
  }

  err = snd_pcm_hw_params_set_rate_near(handle, params, 
	  &waveformat.dwSamplesPerSec, 0);
  if (err < 0) {
    errorMessage = QString("Unable to set rate to %1").arg(waveformat.dwSamplesPerSec);
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;
  }

  assert(err >= 0);       

  err = snd_pcm_hw_params(handle, params);

  if (err < 0) {
    errorMessage = QString("Unable to set hardware parameters");
    msgBox.setText(errorMessage);
    msgBox.exec();
    return FALSE;
  }

  chunk_size = 0;
  buffer_size=0;
  snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
  snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
  bits_per_sample = snd_pcm_format_physical_width(format);
  bits_per_frame = bits_per_sample * waveformat.wChannels;
  chunk_bytes = chunk_size * bits_per_frame / 8;
  
  /* allocate space for the buffer */
  buffer2 = (char *)malloc (buffer_size);

  stopSound = false;

  return TRUE ;

#endif

}



void QAlsaSound::play(int snum)
{

  if (!is_available) {
    fprintf(stderr, "not available\n");
    return;
  }

  /* check to see if this is the correct sound */
  if (snum != soundNum) {
    return;
  }

#ifdef Q_OS_LINUX

  int err;
  /* 
   * start playback
   */ 

  err=lseek(fd,datastart,SEEK_SET);

  //int written;
  int count,f;
  snd_pcm_start(handle);
  while ((count = read (fd, buffer2,buffer_size)))  {
    f=count*8/bits_per_frame;
    do {
      /* check to see if we want to stop writing.  This variable is set by the
       * stop() function */
      if (stopSound) {
	  snd_pcm_drop(handle);
	  free(buffer2);
	  return;
      }
      /* write the data and prepare the pcm if there is an error writing */
      snd_pcm_wait(handle, -1);
      if ((frames = snd_pcm_writei(handle, buffer2, f)) < 0) {
	  snd_pcm_prepare(handle);
	  fprintf(stderr, "frames < 0\n");
      }
    } while (frames < 0);

/******************************************************************************************
*   
*   Portion of code not used, because it does not change the playback 
*   quality on my box. This code is more or less part of aplay ot other 
*   players
*
*         written=0;
*         while (f > 0) {
*
*           frames = snd_pcm_writei(handle, buffer2+written, f);
*           if (frames == -EAGAIN || (frames >= 0 && frames < f)) 
*             snd_pcm_wait(handle, 1000);
*           else if (frames < 0)//{
*             frames = snd_pcm_recover(handle, frames, 0);
*             //snd_pcm_prepare(handle);//}
*           if (frames < 0) {
*             printf("snd_pcm_writei failed: %s\n", snd_strerror(err));
*             break;
*           }
*
*           if (frames > 0)
*           {
*             f -= frames;
*             written += frames * bits_per_frame / 8;
*           }       
*         }
*       
*****************************************************************************************/

  }

#endif

}

void QAlsaSound::stop(int snum)
{
  if ((!is_available) | (snum != soundNum))
    return;
 
  snd_pcm_drop(handle);

}

void QAlsaSound::close()
{
  if (!is_available)
    return;

  if (handle) {
      snd_pcm_hw_free(handle);
  }
  free(buffer2);
}




char* QAlsaSound::findchunk  (char* pstart, char* fourcc, size_t n)
{       char    *pend ;
  int       k, test ;

  pend = pstart + n ;

  while (pstart < pend)
  {       if (*pstart == *fourcc)       /* found match for first char*/
    {       test = TRUE ;
      for (k = 1 ; fourcc [k] != 0 ; k++)
        test = (test ? ( pstart [k] == fourcc [k] ) : FALSE) ;
      if (test)
        return  pstart ;
      } ; /* if*/
    pstart ++ ;
    } ; /* while lpstart*/

  return  NULL ;
}  /* findchuck*/

