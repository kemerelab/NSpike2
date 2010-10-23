/*
 * spike_extract: Program for extracting compressed data from a spike data file
 *
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


#define _FILE_OFFSET_BITS 64
#include "spikecommon.h"
#include "spike_dio.h"
#define fopen fopen64


int WriteSpikeData(u32 offset);
int WriteContData(u32 offset);
int WritePosData(u32 offset);
int WriteDIOData(u32 offset);
int WriteEventData(u32 offset);
short GetNextRecord(char *data, u32 *fileoffset, int *datasize);
void Usage(void);

typedef struct _ExtractInfo {
    gzFile  datafile;
    int   extractspike;
    int   extractcont;
    int   extractpos;
    int   extractpos64;
    int   extractdio;
    int   extractdiotext;
    int   extractevent;
    int   fixtime;
    u32   toffset;
} ExtractInfo;

int niter = 0;

ExtractInfo 	extractinfo;
SysInfo		sysinfo;
NetworkInfo	netinfo;
DigIOInfo	digioinfo;
CommonDSPInfo	cdspinfo;
UserDataInfo	userdatainfo;
char		*tmpdata;
char 		datafilename[200];

double		actualtime, computedtime;




int main(int argc, char **argv) 
{
    FILE 		*datafile;
    char		tmpstring[200];
    char 		configfilename[200];
    u32	startoffset;
    int 		nxtarg = 0;    

    extractinfo.extractspike = 0;
    extractinfo.extractcont = 0;
    extractinfo.extractpos = 0;
    extractinfo.extractpos64 = 0;
    extractinfo.extractdio = 0;
    extractinfo.extractdiotext = 0;
    extractinfo.extractevent = 0;
    extractinfo.fixtime = 0;
    extractinfo.toffset = 0;
    datafilename[0] = '\0';

    while (++nxtarg < argc) {
        if (strcmp(argv[nxtarg], "-spike") == 0) {
      extractinfo.extractspike = 1;
        }
        else if (strcmp(argv[nxtarg], "-cont") == 0) {
      extractinfo.extractcont = 1;
        }
        else if (strcmp(argv[nxtarg], "-pos") == 0) {
      extractinfo.extractpos = 1;
        }
        else if (strcmp(argv[nxtarg], "-pos64") == 0) {
      extractinfo.extractpos = 1;
      extractinfo.extractpos64 = 1;
        }
        else if (strcmp(argv[nxtarg], "-dio") == 0) {
      extractinfo.extractdio = 1;
        }
        else if (strcmp(argv[nxtarg], "-diotext") == 0) {
      extractinfo.extractdiotext = 1;
        }
        else if (strcmp(argv[nxtarg], "-event") == 0) {
      extractinfo.extractevent = 1;
        }
        else if (strcmp(argv[nxtarg], "-all") == 0) {
      extractinfo.extractspike = 1;
      extractinfo.extractcont = 1;
      extractinfo.extractpos = 1;
      extractinfo.extractdiotext = 1;
      extractinfo.extractevent = 1;
        }
        else if (strcmp(argv[nxtarg], "-toffset") == 0) {
      extractinfo.toffset = ParseTimestamp(argv[++nxtarg]);
  }
        else if (argv[nxtarg][0] != '-') {
            /* this is the name of the data file */
      strcpy(datafilename, argv[nxtarg]);
        }
        else {
      Usage();
            exit(0);
        }    
    } 
    if (datafilename[0] == '\0') {
  exit(0);
    }
    
    /* Read in the configuration header */
    /* Note that this opens and closes the data file. This is a bit inefficient, but it
     * is easier to use the function from spike config than to do anything else */
    sysinfo.machinenum = -1;
    if (ReadConfigFile(datafilename, 1) < 0) {
       fprintf(stderr, "Error in configuration header of %s, exiting.\n", datafilename);
       exit(-1);
    }
    /* set the DSP information */
    SetDSPInfo();

    /* set the total number of electrodes */
    sysinfo.nelectrodes = sysinfo.nchannels[sysinfo.machinenum] / NCHAN_PER_ELECTRODE;

    /* allocate space for the data records */
    /* the character data is for the position image */
    tmpdata = (char *) calloc(SAVE_BUF_SIZE, sizeof(char));

    /* first we write out the configuration to a config files to save it */
    strcpy(configfilename, datafilename);
    strcat(configfilename, ".config");
    WriteConfigFile(configfilename, 0, 1);

    /* now reopen the datafile */
    if ((extractinfo.datafile = gzopen(datafilename, "r")) == NULL) {
  fprintf(stderr,"Error opening data file %s for reading\n", argv[nxtarg]);
  exit(-1);
    } 
    /* skip through until we are past the header */
    do {
        gzgets(extractinfo.datafile, tmpstring, 200);
    } while ((strncmp(tmpstring, "%%ENDHEADER", 10) != 0) && 
             (strncmp(tmpstring, "%%ENDCONFIG", 10) != 0));
    startoffset = gztell(extractinfo.datafile);

    if (extractinfo.toffset) {
  fprintf(stderr, "Extracting data with time offset of %ld timestamps\n",
    (long int)extractinfo.toffset);
   }


    if (extractinfo.extractspike && (sysinfo.datatype[sysinfo.machinenum] & SPIKE)) {
        if (WriteSpikeData(startoffset) < 0) {
      exit(-1);
        }
    }

    if (extractinfo.extractcont && (sysinfo.datatype[sysinfo.machinenum] & CONTINUOUS)) {
        if (WriteContData(startoffset) < 0) {
      exit(-1);
  }
    }

    if (extractinfo.extractpos && (sysinfo.datatype[sysinfo.machinenum] & POSITION)) {
        if (WritePosData(startoffset) < 0) {
      exit(-1);
  }
    }

    if ((extractinfo.extractdio || extractinfo.extractdiotext) && (sysinfo.datatype[sysinfo.machinenum] & DIGITALIO)) {
        if (WriteDIOData(startoffset) < 0) {
      exit(-1);
  }
    }

    if (extractinfo.extractevent) {
        if (WriteEventData(startoffset) < 0) {
      exit(-1);
  }
    }
    return 1;
}

int WriteSpikeData(u32 offset)
{
    /* for each electrode, make a directory of the appropriate name and write out 
     * the spike files */
    FILE    **outfile;
    int     *electnumfile;
    SpikeBuffer   *stmp;
    ElectrodeData electdata;
    int     i, j;
    int     electnum;
    int     maxelectnum;
    int     depth;
    int     ndays;
    int     retval;
    int     datasize;
    int     nspikes;
    char    command[100];
    char    outfilename[100];
    char    tmpstring[100];
    char    electnumstr[4];
    char    depthstr[50];
    // char   depthstr[5];
    short   recordtype;
    u32 fileoffset, lastspiketime;
    size_t    tmpsize;
    int     exists = 0;


    /* allocate space for the list of files and the lookup table for the
     * electrode numbers */
    outfile = (FILE **) malloc(sizeof(FILE *) * sysinfo.nelectrodes);
    maxelectnum = 0;
    for (i = 0; i < sysinfo.nelectrodes; i++) {
  if (sysinfo.channelinfo[sysinfo.machinenum][i*NCHAN_PER_ELECTRODE].number > maxelectnum) {
      maxelectnum = sysinfo.channelinfo[sysinfo.machinenum]
    [i*NCHAN_PER_ELECTRODE].number;
  }
    }
    electnumfile = (int *) calloc(maxelectnum + 1, sizeof(int));

    for (i = 0; i < sysinfo.nelectrodes; i++) {
  lastspiketime = 0;
        /* get the electrode number, depth, and days for the first channel of this electrode */
  electnum = sysinfo.channelinfo[sysinfo.machinenum][i*NCHAN_PER_ELECTRODE].number;
  depth = sysinfo.channelinfo[sysinfo.machinenum][i*NCHAN_PER_ELECTRODE].depth;
  /* this current electrode number is the i-th file */
  electnumfile[electnum] = i;
  /* construct strings for each of them so that all of the directory names are the
   * same length */
        if (electnum < 10) {
      sprintf(electnumstr, "0%d", electnum);
        }
  else {
      sprintf(electnumstr, "%d", electnum);
        }
        if (depth < 10) {
      sprintf(depthstr, "00%d", depth);
        }
  else if (depth < 100) {
      sprintf(depthstr, "0%d", depth);
  }
  else {
      sprintf(depthstr, "%d", depth);
  }
  /* now put them all together in tmpstring and command */
  sprintf(tmpstring, "%s-%s", electnumstr, depthstr);
  sprintf(command, "mkdir %s", tmpstring);
  fprintf(stderr, "executing %s\n", command);
  /* make the directory */
  retval = system(command);
  /* return -1 on error */
  if ((retval == -1) || (retval == 127)) {
      fprintf(stderr, "Error: unable to execute %s\n", command);
      return -1;
        }
  /* put .tt on the end of the spike file */
  sprintf(outfilename, "%s/%s.tt", tmpstring, tmpstring);

        /* check to see if the file exists and warn the user if it does */
  exists = 0;
  if ((outfile[i] = fopen(outfilename, "r")) != NULL) {
      fprintf(stderr, "Warning: appending to previously created file %s\n", outfilename);
      fclose(outfile[i]); 
      exists = 1;
        }

  /* now open up the spike file */
  if ((outfile[i] = fopen(outfilename, "a")) == NULL) {
      fprintf(stderr, "Error: unable to open file [%s]\n", outfilename);
      return -1;
        }
  if (!exists) {
      /* write out the necessary header elements for spikeparms */
      fprintf(outfile[i], "%%%%BEGINHEADER\n");
      fprintf(outfile[i], "%% File type:    Binary\n");
      fprintf(outfile[i], "%% Extraction type:      tetrode waveforms\n");
      fprintf(outfile[i], "%% Fields:       timestamp,8,4,1 waveform,2,2,%d\n", NPOINTS_PER_SPIKE * NCHAN_PER_ELECTRODE);
      fprintf(outfile[i], "%%%%ENDHEADER\n");
  }

    }
    /* go to the offset location in the data file so that we are past the configuration header */
    gzseek(extractinfo.datafile, offset, SEEK_SET);
    fileoffset = offset;

    /* go through the file looking for spikes from this electrode */
    do {
  recordtype = GetNextRecord(tmpdata, &fileoffset, &datasize);
  if (recordtype == SPIKE_DATA_TYPE) {
      stmp = (SpikeBuffer *) tmpdata;
      /* copy the data into an ElectrodeData structure */
      nspikes = datasize / sizeof(SpikeBuffer);
      for (i = 0; i < nspikes; i++, stmp++) {
    electdata.timestamp = stmp->timestamp;
    memcpy(electdata.data, stmp->data, NTOTAL_POINTS_PER_SPIKE *
      sizeof(short));
    if (fwrite(&electdata, 1, sizeof(ElectrodeData),
          outfile[electnumfile[stmp->electnum]]) 
        != sizeof(ElectrodeData)) {
        fprintf(stderr, "Error: unable to write spike data\n");
        return -1;
    }
      }
  }
    } while (!gzeof(extractinfo.datafile));
    /* close all of the output files */
    for (i = 0; i < sysinfo.nelectrodes; i++) {
  fclose(outfile[i]);
    }
    return 0;
}

int WriteContData(u32 offset)
{
  /* for each , make a directory of the appropriate name and write out 
   * the spike files */
  FILE    **outfile;
  ContBuffer    *ctmp;
  DSPInfo   *dptr;
  ChannelInfo   *ch;
  int     i, j;
  int     electnum;
  int     depth;
  int     nsamplesperbuf;
  int     datasize;
  char    command[100];
  char    outfilename[100];
  char    electnumstr[4];
  char    depthstr[50];
  //char    depthstr[5];
  short   recordtype;
  int     electnumfile[MAX_CHANNELS];
  u32     fileoffset;
  u32     tmptime;
  short   *shortptr;
  int     numsamples;
  double    samplingrate;
  bool    lastodd = 0;
  bool    oddframe = 0;
  char    transition;


  /* allocate space for the list of output file names */
  outfile = (FILE **) malloc(sysinfo.nchannels[sysinfo.machinenum] * sizeof(FILE *));
  /* create a lookup table for the file indeces corresponding to the
   * electrode number */
  for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
    /* get the electrode number, depth, and ndays for the first channel of this electrode */
    ch = sysinfo.channelinfo[sysinfo.machinenum] + i;
    electnum = ch->number;
    /* this current electrode number is the i-th file */
    electnumfile[electnum] = i;
    /* check to see if this is the position sync channel and if so,
     * overwrite the name */
    if (ch->dspchan == DSP_POS_SYNC_CHAN) {
      sprintf(outfilename, "possynctimes");
      if ((outfile[i] = fopen(outfilename, "r")) != NULL) {
        fprintf(stderr, "Warning: appending to previously created file %s\n", outfilename);
        fclose(outfile[i]); 
        /* reopen the file in append mode */
        if ((outfile[i] = fopen(outfilename, "a")) == NULL) {
          fprintf(stderr, "Error: unable to open file [%s]\n", outfilename);
          return -1;
        }
      }
      else {
        if ((outfile[i] = fopen(outfilename, "w")) == NULL) {
          fprintf(stderr, "Error: unable to open file [%s]\n", outfilename);
          return -1;
        }
        /* Write out a header to tell us what's in the file */
        fprintf(outfile[i], "%%%%BEGINHEADER\n");
        fprintf(outfile[i], "%% Vertical Sync times for position data\n");
        fprintf(outfile[i], "%% File type:    Binary\n");
        fprintf(outfile[i], "%% Fields: timestamp (u32) transition (char) (0 for even to odd, 1 for odd to even)\n");
        fprintf(outfile[i], "%%%%ENDHEADER\n");
      }
    }
    else {
      depth = sysinfo.channelinfo[sysinfo.machinenum][i].depth;
      depth = ch->depth;
      /* construct strings for each of them so that all of the directory 
       * names are the same length */
      if (electnum < 10) {
        sprintf(electnumstr, "0%d", electnum);
      }
      else {
        sprintf(electnumstr, "%d", electnum);
      }
      if (depth < 10) {
        sprintf(depthstr, "00%d", depth);
      }
      else if (depth < 100) {
        sprintf(depthstr, "0%d", depth);
      }
      else {
        sprintf(depthstr, "%d", depth);
      }
      /* now put them all together */
      sprintf(outfilename, "%s-%s", electnumstr, depthstr);
      /* put .eeg on the end of the spike file */
      strcat(outfilename, ".eeg");


      /* check to see if the file exists and warn the user if it does */
      if ((outfile[i] = fopen(outfilename, "r")) != NULL) {
        fprintf(stderr, "Warning: appending to previously created file %s\n", outfilename);
        fclose(outfile[i]); 
        /* reopen the file in append mode */
        if ((outfile[i] = fopen(outfilename, "a")) == NULL) {
          fprintf(stderr, "Error: unable to open file [%s]\n", outfilename);
          return -1;
        }
      }
      else {
        if ((outfile[i] = fopen(outfilename, "w")) == NULL) {
          fprintf(stderr, "Error: unable to open file [%s]\n", outfilename);
          return -1;
        }
        /* Write out a header to tell us what's in the file */
        fprintf(outfile[i], "%%%%BEGINHEADER\n");
        fprintf(outfile[i], "%% Continuous data from electrode %d\n", i+1);
        fprintf(outfile[i], "%% File type:    Binary\n");
        fprintf(outfile[i], "%% Fields timestamp nsamples samplingrate samples\n");
        fprintf(outfile[i], "%%%%ENDHEADER\n");
        /* go through the file looking for samples from this electrode */
      }
    }
  }
  /* go to the offset location in the data file so that we are past the configuration header */
  gzseek(extractinfo.datafile, offset, SEEK_SET);
  fileoffset = offset;

  do {
    niter++;
    recordtype = GetNextRecord(tmpdata, &fileoffset, &datasize);
    //if (recordtype == -1) break;
    /* write out the record */
    if (recordtype == CONTINUOUS_DATA_TYPE) {
      ctmp = (ContBuffer *) tmpdata;

      dptr = sysinfo.dspinfo + ctmp->dspnum;
      /* write out the timestamp */
      for (i = 0; i < dptr->nchan; i++) {
        /* make sure that this is not the position sync channel */
        if (dptr->dspchan[i] != DSP_POS_SYNC_CHAN) {
          electnum = dptr->electinfo[i].number;
          //electnum = 1;
          if (fwrite(&ctmp->timestamp, sizeof(u32), 1, 
                outfile[electnumfile[electnum]]) != 1) {
            fprintf(stderr, "Error: unable to write continuous data\n");
            return -1;
          }
          /* write out the number of samples */
          numsamples = (int) dptr->nsampout;
          if (fwrite(&numsamples, sizeof(int), 1, 
                outfile[electnumfile[electnum]]) != 1) {
            fprintf(stderr, "Error: unable to write continuous data\n");
            return -1;
          }
          /* write out the sampling rate */
          samplingrate = (double) dptr->samprate;
          if (fwrite(&samplingrate, sizeof(double), 1, 
                outfile[electnumfile[electnum]]) != 1){
            fprintf(stderr, "Error: unable to write continuous data\n");
            return -1;
          }
        }
      }
      /* write out the samples */
      shortptr = ctmp->data;
      for (j = 0; j < dptr->nsampout; j++) {
        for (i = 0; i < dptr->nchan; i++) {
          electnum = dptr->electinfo[i].number;
          if (dptr->dspchan[i] == DSP_POS_SYNC_CHAN) {
            /* look for a transition between odd and even */
            oddframe = *shortptr & DSP_VIDEO1_SYNC_ODD;
            if ((!oddframe && lastodd) || (oddframe && !lastodd)) {
              /* write out a timestamp and the transition type */
              tmptime = ctmp->timestamp + (u32) (((double) j / 
                    (double) dptr->samprate) * SEC_TO_TSTAMP);
              if (fwrite(&tmptime, sizeof(u32), 1, 
                    outfile[electnumfile[electnum]]) != 1) {
                fprintf(stderr, "Error: unable to write position sync data\n");
                return -1;
              }
              transition = lastodd ? DSP_ODD_TO_EVEN_SYNC :
                DSP_EVEN_TO_ODD_SYNC;
              if (fwrite(&transition, sizeof(char), 1, 
                    outfile[electnumfile[electnum]]) != 1) {
                fprintf(stderr, "Error: unable to write position sync data\n");
                return -1;
              }
              lastodd = !lastodd;
            }
          }
          else {
            if (fwrite(shortptr, sizeof(short), 1, 
                  outfile[electnumfile[electnum]]) 
                != 1) {
              fprintf(stderr, "Error: unable to write continuous data for electrode %d\n", i);
              return -1;
            }
          }
          shortptr++;
        }
      }
    }
  } while (!gzeof(extractinfo.datafile));
  /* close all of the output files */
  for (i = 0; i < sysinfo.nchannels[sysinfo.machinenum]; i++) {
    fclose(outfile[i]);
  }
  return 0;
}

int WritePosData(u32 offset)
/* Write out the position data and a separate file for timestamps */
{
    
    FILE    *pipe;
    FILE    *mpegout;
    FILE    *foffsetout;
    FILE    *tcheckout;
    FILE    *tstampout;
    PosMPEGBuffer *ptmp;
    char    tmpstring[200];
    char    command[200];
    char    *tmpptr;
    int     datasize;
    char    tstampfilename[100];
    char    mpegfilename[100];
    char    foffsetname[100];
    char    tcheckname[100];
    short   recordtype;
    u32     fileoffset;
    u32     mpegoffset;
    unsigned long long  mpegoffset64;
    u32     lasttimestamp = 0;
    int     currentslice;
    u32     size;
    int     i;

    
    /* generate the command */
    sprintf(command, "pwd");
    if ((pipe = popen(command, "r")) == NULL) {
        fprintf(stderr, "Error executing pwd\n");
    }
    /* read in the present working directory */
    fgets(tmpstring, 200, pipe);
    pclose(pipe);

    /* get rid of the '\n' in tmpstring */
    tmpptr = strchr(tmpstring, '\n');
    *tmpptr = '\0';

    /* find the last "/" in the directory; everything past that point is the name of the
     * current directory */
    tmpptr = strrchr(tmpstring, '/');

    /* create an output file containing the timestamp information*/
    sprintf(tstampfilename, "%s.cpupostimestamp", ++tmpptr);
    /* check to see if the file exists and warn the user if it does */
    if ((tstampout = fopen(tstampfilename, "r")) != NULL) {
  fprintf(stderr, "Warning: appending to previously created file %s\n", tstampfilename);
  fclose(tstampout); 
  /* open the file for appending */
  if ((tstampout = fopen(tstampfilename, "a")) == NULL) {
      fprintf(stderr, "Error: unable to open file [%s]\n", tstampfilename);
      return -1;
  }
    }
    else { 
  /* open the file for appending */
  if ((tstampout = fopen(tstampfilename, "a")) == NULL) {
      fprintf(stderr, "Error: unable to open file [%s]\n", tstampfilename);
      return -1;
  }
  /* write out an uncompressed header */
  fprintf(tstampout, "%%%%BEGINHEADER\n");
  fprintf(tstampout, "%% File type:    Binary\n");
  fprintf(tstampout, "%% Extraction type:  mpeg cpu frame times \n");
  fprintf(tstampout, "%% Fields:       time (unsigned int)\n");
  fprintf(tstampout, "%%%%ENDHEADER\n");
    }

    /* now open the video file */
    switch ( sysinfo.videocodec ) {
      case  MPEG1_CODEC:
        sprintf(mpegfilename, "%s.mpeg", tmpptr);
      break;
      case  MPEG2_CODEC:
        sprintf(mpegfilename, "%s.m2v", tmpptr);
      break;
      case  MJPEG_CODEC: 
        sprintf(mpegfilename, "%s.mjpg", tmpptr);
      break;
      default:
        fprintf(stderr, "Error: unknown video codec: [%d] (assuming mpeg)\n",sysinfo.videocodec);
        sprintf(mpegfilename, "%s.mpeg", tmpptr);
    }
    if ((mpegout = fopen(mpegfilename, "r")) != NULL) {
  fprintf(stderr, "Warning: appending to previously created file %s\n", mpegfilename);
  fclose(mpegout); 
    }
    /* open the file for appending */
    if ((mpegout = fopen(mpegfilename, "a")) == NULL) {
  fprintf(stderr, "Error: unable to open mpeg file [%s]\n", mpegfilename);
  return -1;
    }

    /* open the offset file */
    sprintf(foffsetname, "%s.mpegoffset", tmpptr);
    /* check to see if the file exists */
    if ((foffsetout = fopen(foffsetname, "r")) == NULL) {
  /* open the file for writing */
  if ((foffsetout = fopen(foffsetname, "w")) == NULL) {
      fprintf(stderr, "Error: unable to open file [%s]\n", foffsetname);
      return -1;
  }
  /* write out a header to the offset file */
  fprintf(foffsetout, "%%%%BEGINHEADER\n");
  fprintf(foffsetout, "%% File type:    Binary\n");
  fprintf(foffsetout, "%% Extraction type:      mpeg file offset\n");
  if (!extractinfo.extractpos64) {
      fprintf(foffsetout, "%% Fields:       offset (unsigned int)\n");
  }
  else {
      fprintf(foffsetout, "%% Fields:       offset (unsigned long long)\n");
  }
  fprintf(foffsetout, "%%%%ENDHEADER\n");
    }
    else {
  /* append to the already created file */
  if ((foffsetout = fopen(foffsetname, "a")) == NULL) {
      fprintf(stderr, "Error: unable to open file %s for appending\n", foffsetname);
      return -1;
  }
    }

    /* open the time check file */
    sprintf(tcheckname, "%s.cpudsptimecheck", tmpptr);
    /* check to see if the file exists */
    if ((tcheckout = fopen(tcheckname, "r")) == NULL) {
  /* open the file for writing */
  if ((tcheckout = fopen(tcheckname, "w")) == NULL) {
      fprintf(stderr, "Error: unable to open file [%s]\n", tcheckname);
      return -1;
  }
  /* write out a header to the offset file */
  fprintf(tcheckout, "%%%%BEGINHEADER\n");
  fprintf(tcheckout, "%% File type:    Binary\n");
  fprintf(tcheckout, "%% Extraction type:      time data\n");
  fprintf(tcheckout, "%% Fields:       cpu_time (u32) dsp_time (u32) cpu_time (u32)\n");
  fprintf(tcheckout, "%%%%ENDHEADER\n");
    }
    else {
  /* append to the already created file */
  if ((tcheckout = fopen(tcheckname, "a")) == NULL) {
      fprintf(stderr, "Error: unable to open file %s for appending\n", tcheckname);
      return -1;
  }
    }
    /* go to the offset location in the data file so that we are past the configuration header */
    gzseek(extractinfo.datafile, offset, SEEK_SET);
    fileoffset = offset;

    /* allocate space for the position buffers */
    ptmp = (PosMPEGBuffer *) malloc((sysinfo.mpegslices + 1) * 
      sizeof(PosMPEGBuffer));

    /* go through the file looking for position or timecheck buffers */
    lasttimestamp = 0;
    currentslice = 0;
    size = 0;
    do {
  recordtype = GetNextRecord(tmpdata, &fileoffset, &datasize);
  //if (recordtype == -1) break;
  /* write out the record if it is position data or time check data*/
  if (recordtype == POSITION_DATA_TYPE) {
      memcpy(ptmp + currentslice, tmpdata, datasize);
      /* check to see if the timestamp matches lasttimestamp, in which
       * case we move on to the next slice */
      if (ptmp[currentslice].timestamp == lasttimestamp) {
    currentslice++;
      }
      /* otherwise we write out all of the slices up to, but not
       * including the current one if we're past the first slice*/
      else if (currentslice > 0) {
          if (fwrite(&ptmp[0].timestamp, sizeof(u32), 1, tstampout) 
      != 1) {
        fprintf(stderr, "Error: unable to write mpeg frame timestamp to %s\n", mpegfilename);
        return -1;
    } 
    /* write out the current offset in the mpeg */
    if (!extractinfo.extractpos64) {
        mpegoffset = (u32) ftell(mpegout);
        fwrite(&mpegoffset, 1, sizeof(u32), foffsetout);
    }
    else {
        mpegoffset64 = (unsigned long long) ftello(mpegout);
        fwrite(&mpegoffset64, 1, sizeof(unsigned long long), 
          foffsetout);
    } 
    for (i = 0; i < currentslice; i++) {
        size += ptmp[i].size;
    }
    /* write out the slices */
    for (i = 0; i < currentslice; i++) {
        if (fwrite(ptmp[i].frame, ptmp[i].size, 1, mpegout)
          != 1) {
      fprintf(stderr, "Error: unable to write mpeg data to %s\n", mpegfilename);
      return -1; 
        }
    }
    /* copy the last frame to the first frame */
    memcpy(ptmp, ptmp+currentslice, sizeof(PosBuffer));
    currentslice = 1;
    lasttimestamp = ptmp[0].timestamp;
    size = 0;
      }
      else {
    /* we've read in the first slice only, so move on to the next
     * one */
    currentslice++;
    lasttimestamp = ptmp[0].timestamp;
      }
  }
  else if (recordtype == TIME_CHECK_DATA_TYPE) {
      if (fwrite(tmpdata, datasize, 1, tcheckout) != 1) {
    fprintf(stderr, "Error: unable to write time check data to %s\n", tcheckname);
    return -1; 
      }
  }

    } while (!gzeof(extractinfo.datafile));
    fclose(tstampout);
    fclose(mpegout);
    fclose(foffsetout);
    return 0;
}


int WriteDIOData(u32 offset)
{
    FILE    *pipe;
    FILE    *outfile;
    char    tmpstring[200];
    char    command[200];
    char    *tmpptr;
    int     datasize;
    char    outfilename[100];
    short   recordtype;
    u32     *u32ptr;
    unsigned short  s, stmp;
    u32     fileoffset;
    int     i, pnum;
    DIOBuffer   *dtmp;

    
    /* generate the command */
    sprintf(command, "pwd");
    if ((pipe = popen(command, "r")) == NULL) {
        fprintf(stderr, "Error executing pwd\n");
    }
    /* read in the present working directory */
    fgets(tmpstring, 200, pipe);
    pclose(pipe);

    /* get rid of the '\n' in tmpstring */
    tmpptr = strchr(tmpstring, '\n');
    *tmpptr = '\0';

    /* find the last "/" in the directory; everything past that point is the name of the
     * current directory */
    tmpptr = strrchr(tmpstring, '/');

    /* copy the remainder of the string to the posfilename */
    sprintf(outfilename, "%s.dio", ++tmpptr);

    /* check to see if the file exists and warn the user if it does */
    if ((outfile = fopen(outfilename, "r")) != NULL) {
  fprintf(stderr, "Warning: appending to previously created file %s\n", outfilename);
  fclose(outfile); 
    }

    /* open the file for appending */
    if ((outfile = fopen(outfilename, "a")) == NULL) {
  fprintf(stderr, "Error: unable to open file [%s]\n", outfilename);
  return -1;
    }
    /* go to the offset location in the data file so that we are past the configuration header */
    gzseek(extractinfo.datafile, offset, SEEK_SET);
    fileoffset = offset;

    do {
  recordtype = GetNextRecord(tmpdata, &fileoffset, &datasize);
  /* write out the record if it is digital IO data */
  if (recordtype == DIGITALIO_DATA_TYPE) {
      if (extractinfo.extractdiotext == 0) {
    if (fwrite(tmpdata, 1, datasize, outfile) != datasize) {
        fprintf(stderr, "Error: unable to write digital IO data to %s\n", 
          outfilename);
        return -1;
    }
      }
      else {
    dtmp = (DIOBuffer *) tmpdata;
    fprintf(outfile, "%u\t", dtmp->timestamp);
    /* write out 64 binary digits to represent the status */
    for (pnum = 0; pnum < MAX_DIO_PORTS; pnum++) {
        for (i = 0; i < 16; i++) {
      if (dtmp->status[pnum] & (1 << 15-i)) {
          fprintf(outfile, "1");
      }
      else {
          fprintf(outfile, "0");
      }
        }
    }
    fprintf(outfile, "\n");
      }
  }
    } while (!gzeof(extractinfo.datafile));
    fclose(outfile);
    return 0;
}

int WriteEventData(u32 offset)
{
    FILE    *pipe;
    FILE    *outfile;
    char    tmpstring[200];
    char    command[200];
    char    *tmpptr;
    int     datasize;
    char    outfilename[100];
    short   recordtype;
    EventBuffer   *event;
    u32 fileoffset;

    
    /* generate the command */
    sprintf(command, "pwd");
    if ((pipe = popen(command, "r")) == NULL) {
        fprintf(stderr, "Error executing pwd\n");
    }
    /* the name lf the event file is the name of the input file with .event
     * appended to it */
    sprintf(outfilename, "%s.event", datafilename);

    /* check to see if the file exists and warn the user if it does */
    if ((outfile = fopen(outfilename, "r")) != NULL) {
  fprintf(stderr, "Warning: appending to previously created file %s\n", outfilename);
  fclose(outfile); 
    }

    /* open the file for appending */
    if ((outfile = fopen(outfilename, "a")) == NULL) {
  fprintf(stderr, "Error: unable to open file [%s]\n", outfilename);
  return -1;
    }
    if (ftell(outfile) == 0) {
  /* write out a string with the description of each field */
  fprintf(outfile, "%%time\ttype\tdescript\n");
    }

    /* go to the offset location in the data file so that we are past the configuration header */
    gzseek(extractinfo.datafile, offset, SEEK_SET);
    fileoffset = offset;

    /* go through the file looking for spikes from this electrode */
    do {
  recordtype = GetNextRecord(tmpdata, &fileoffset, &datasize);
  //if (recordtype == -1) break;
  /* write out the record in ascii if it is event data */
  if (recordtype == EVENT_DATA_TYPE) {
      event = (EventBuffer *) tmpdata;
      if (fprintf(outfile, "%ld\t%d\t%s\n", (long int)event->timestamp, event->type, event->descript) == -1) {
    fprintf(stderr, "Error: unable to write event data to %s\n", 
            outfilename);
    return -1;
      }
  }
    } while (!gzeof(extractinfo.datafile));
    fclose(outfile);
    return 0;
}

short GetNextRecord(char *data, u32 *fileoffset, int *datasize)
{
    PosMPEGBuffer   ptmp;
    ContBuffer    ctmp;
    SpikeBuffer   *stmp;
    EventBuffer   etmp;
    DIOBuffer   dtmp;
    DSPInfo   *dptr;
    u32     tptr[3];
    short     recordtype;
    int     tmp;
    int     size;

    int i;

    /* get the character that will tell us what kind of record we're looking at */
    if ((tmp = gzread(extractinfo.datafile, &recordtype, sizeof(short))) != sizeof(short)) {
  fprintf(stderr, "Error: unable to read in record type from file at offset %ld\n", 
      (long int)(*fileoffset));
  return -1;
    }
    else {
  *fileoffset += sizeof(short);
    }
    *datasize = 0;
    switch ((int) recordtype) {
  case SPIKE_DATA_TYPE:
      /* read in the data size */
      if (gzread(extractinfo.datafile, &size, sizeof(int)) != sizeof(int)) {
    fprintf(stderr, "Error: unable to read in size from file at offset %ld\n", 
        (long int)(*fileoffset));
    return -1;
      }
      *fileoffset += sizeof(int);
      if (gzread(extractinfo.datafile, data, size) != size) {
    fprintf(stderr, "Error: unable to read in size from file at offset %ld\n", 
        (long int)(*fileoffset));
    return -1;
      }
      *fileoffset += size;
      *datasize = size;
      stmp = (SpikeBuffer *) data;
      /* add in toffset */
      for (i = 0; i < (size / sizeof(SpikeBuffer)); i++) {
    stmp[i].timestamp += extractinfo.toffset;
      }
      break;
  case CONTINUOUS_DATA_TYPE:
      /* read in the size of the record */
      if (gzread(extractinfo.datafile, &size, sizeof(int)) != sizeof(int)) {
    fprintf(stderr, "Error: unable to read in size from file at offset %ld\n", 
        (long int)(*fileoffset));
    return -1;
      }
      *fileoffset += sizeof(int);
      /* read in the data */
      if (gzread(extractinfo.datafile, &ctmp, size) != size) {
    fprintf(stderr, "Error: unable to read in continuous data from file at offset %ld\n", 
        (long int) (*fileoffset));
    return -1;
      }
      *fileoffset += size;
      *datasize += size;
      /* add in toffset */
      ctmp.timestamp += extractinfo.toffset;
      memcpy(data, &ctmp, sizeof(ContBuffer));
      break;
  case POSITION_DATA_TYPE:
      /* read in the timestamp */
      size = sizeof(ptmp.timestamp);
      if (gzread(extractinfo.datafile, &ptmp.timestamp, size) != size) {
    fprintf(stderr, "Error: unable to read in position timestamp from file at offset %ld\n",
        (long int) (*fileoffset));
    return -1;
      }
      *datasize += size;
      fileoffset += size;
      size = sizeof(ptmp.size);
      /* read in the size */
      if (gzread(extractinfo.datafile, &ptmp.size, size) != size) {
    fprintf(stderr, "Error: unable to read in position size from file at offset %ld\n",
        (long int) (*fileoffset));
    return -1;
      }
      fileoffset += size;
      *datasize += size;
      /* read in the data */
      size = ptmp.size * sizeof(unsigned char);
      if (gzread(extractinfo.datafile, &ptmp.frame, size) != size) {
    fprintf(stderr, "Error: unable to read in position data from file at offset %ld\n",
        (long int) (*fileoffset));
    return -1;
      }
      fileoffset += size;
      *datasize += size;
      /* add in toffset */
      ptmp.timestamp += extractinfo.toffset;
      memcpy(data, &ptmp, *datasize);
      break;
  case DIGITALIO_DATA_TYPE:
      size = DIO_BUF_STATIC_SIZE;
      if (gzread(extractinfo.datafile, &dtmp, size) 
        != size) {
    fprintf(stderr, "Error: unable to read in digital IO data from file at offset %ld\n",
        (long int) (*fileoffset));
    return -1;
      }
      *datasize += size;
      fileoffset += size;
      /* add in toffset */
      dtmp.timestamp += extractinfo.toffset;
      memcpy(data, &dtmp, *datasize);
      break;
  case EVENT_DATA_TYPE:
      /* the data consist of an event structure */
      size = sizeof(EventBuffer);
      *datasize = size;
      if (gzread(extractinfo.datafile, &etmp, size) != size) {
    fprintf(stderr, "Error: unable to read in event data from file at offset %ld\n",
        (long int) (*fileoffset));
    return -1;
      }
      fileoffset += size;
      /* add in toffset */
      etmp.timestamp += extractinfo.toffset;
      memcpy(data, &etmp, *datasize);
      break;
        case TIME_CHECK_DATA_TYPE:
      /* the data are three u32 times */
      size = TIME_CHECK_BUF_STATIC_SIZE;
      *datasize = size;
      if (gzread(extractinfo.datafile, tptr, size) != size) {
    fprintf(stderr, "Error: unable to read in timecheck data from file at offset %ld\n",
        (long int) (*fileoffset));
    return -1;
      }
      fileoffset += size;
      /* add in toffset to all three values */
      for (i = 0; i < 3; i++) {
    tptr[i] += extractinfo.toffset;
      }
      memcpy(data, tptr, *datasize);
      break;
  default:
      fprintf(stderr, "Error: unknown record type %d at offset %ld\n", recordtype,
        (long int) (*fileoffset));
      break;
    }

    return recordtype;
}

void Usage(void) 
{

  fprintf(stderr,"nspike_extract version %s\n", VERSION);
  fprintf(stderr,"Usage: nspike_extract [-spike] [-cont] [[-pos] [-pos64]] [-dio] [-diotext] [-event] [-all] [-toffset time_to_add] [-fixtime actualtime computedtime] datafile\n");
  fprintf(stderr,"\t-toffset will add the specified amount of time to each timestamp.  Use this if data collection was interrupted and you need to append data from one session to the end of another. \n");
}
