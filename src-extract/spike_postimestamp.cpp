/* spike_postimestamp: Code for assigning timestamps to position frames 
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


#include "spikecommon.h"
#include "time.h"
#include "spike_dio.h"

#define MAX_CPU_TIME_DELAY 50  // at most 5 ms between cpu times to get accurate dsp correspondence

#define TMPSTRINGLEN 200

int nextvalidcpudsptime(FILE *cpudsptimefile, u32 *cpudsp);

int main(int argc, char **argv) 
{

    int 	nxtarg = 0;
    int 	i;
    char	c;
    char 	tmpstring[TMPSTRINGLEN+1];
    char 	filename[TMPSTRINGLEN+1];
    char 	outfilename[TMPSTRINGLEN+1];
    FILE	*cpudsptimefile = NULL;
    FILE	*cpupostimefile = NULL;
    FILE	*possynctimefile = NULL;
    FILE	*outfile = NULL;

    u32		cpudsp1[3], cpudsp2[3]; // this assumes that each timecheck contains three numbers, the cpu time, the dsp time, and the subsequent cpu time 
    u32 	cpupos;
    u32 	lastcpupos;
    u32		dspframetime;
    u32		possynctime = 0;
    int 	ngaps = 0;  // the number of > 40 ms gaps 
    int		ncomputed = 0; // the number of times we used the computed timestamp instead of the sync edge time
    u32		tmpdiff, lastdiff, maxframetimediff = 0;

    while (++nxtarg < argc) {
	if (strcmp(argv[nxtarg], "-cd") == 0) {
	    strcpy(filename, argv[++nxtarg]);
	    /* open the file */
	    if ((cpudsptimefile = fopen(filename, "r")) 
		    == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", filename);
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-cp") == 0) {
	    /* open cpu pos time file */
	    strcpy(filename, argv[++nxtarg]);
	    if ((cpupostimefile = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", filename);
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-ps") == 0) {
	    /* open pos sync times file */
	    strcpy(filename, argv[++nxtarg]);
	    if ((possynctimefile = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "Error opening %s for reading\n", filename);
		exit(-1);
            }
	}
	else if (strcmp(argv[nxtarg], "-o") == 0) {
            /* open the output file */
            strcpy(outfilename, argv[++nxtarg]);
            if ((outfile = fopen(outfilename, "w")) == NULL) {
                fprintf(stderr, "Error opening %s for writing\n", outfilename);
                exit(-1);
            }
        }
	else {
	    fprintf(stderr, "Usage: nspike_postimestamp -cd cpudsptimefile -cp cpupostimefile -ps possynctimesfile -o outputfile\n"); 
            exit(1);
	}
    }

    /* check to make sure that all necessary arguments have been specified */
    if ((cpudsptimefile == NULL) || (cpupostimefile == NULL) || 
	    (possynctimefile == NULL) || (outfile == NULL)) {
	fprintf(stderr, "Usage: nspike_postimestamp -cd cpudsptimefile -cp cpupostimefile -ps possynctimesfile -o outputfile\n"); 
	exit(1);
    }

    /* write out an uncompressed header to the output file */
    fprintf(outfile, "%%%%BEGINHEADER\n");
    fprintf(outfile, "%% File type:\tBinary\n");
    fprintf(outfile, "%% Extraction type:\tdsp position frame time stamps \n");
    fprintf(outfile, "%% Fields:\t  timestamp (unsigned int)\n");
    fprintf(outfile, "%%%%ENDHEADER\n");

    /* read past the headers of the input files */

    do {
        fgets(tmpstring, TMPSTRINGLEN, cpudsptimefile);
    } while ((strncmp(tmpstring, "%%ENDHEADER", 10) != 0) &&
             (strncmp(tmpstring, "%%ENDCONFIG", 10) != 0));

    do {
        fgets(tmpstring, TMPSTRINGLEN, cpupostimefile);
    } while ((strncmp(tmpstring, "%%ENDHEADER", 10) != 0) &&
             (strncmp(tmpstring, "%%ENDCONFIG", 10) != 0));

    do {
        fgets(tmpstring, TMPSTRINGLEN, possynctimefile);
    } while ((strncmp(tmpstring, "%%ENDHEADER", 10) != 0) &&
             (strncmp(tmpstring, "%%ENDCONFIG", 10) != 0));

    if (nextvalidcpudsptime(cpudsptimefile, cpudsp1) == 0) {
	exit(-1);
    }
    cpudsp2[0] = 0;
    /* Now we go through the cpupostimefile and find the possync time that
     * corresponds to each frame */

    lastcpupos = 0;
    while (!feof(cpupostimefile)) {
	/* read in the next cpupostimefile entry */
	if (fread(&cpupos, sizeof(u32), 1, cpupostimefile) != 1) {
	    fprintf(stderr, "Error reading cpu - position frame time from cpupostimefile at offset %ld, eof = %d\n", ftell(cpupostimefile), feof(cpupostimefile));
	    break;
	}
	if (lastcpupos && (cpupos - lastcpupos > 400)) {
	    ngaps++;
	}
	lastcpupos = cpupos;
	/* make sure that the next cpudsp time is after the current cpu frame
	 * time */

	while ((cpudsp2[0] < cpupos) && nextvalidcpudsptime(cpudsptimefile,
			cpudsp2)) {
	    /* move cpudsp2 to cpudsp1 and move on */
	    memcpy(cpudsp1, cpudsp2, 3 * sizeof(u32));
	}

	/* use the closest cpudsp time to convert the current frame time to a
	 * dsp clock time */
	if (absv(cpudsp1[0] - cpupos) < absv(cpudsp2[0] - cpupos)) {
	    dspframetime = cpupos + (cpudsp1[1] - cpudsp1[0]);
	}
	else {
	    dspframetime = cpupos + (cpudsp2[1] - cpudsp2[0]);
	}
	/* initialize the first possync time so that we read in the correct
	 * first value */
	if (possynctime == 0) {
	    possynctime = dspframetime;
	}
	/* find the pos sync time closest to the dspframe time and write it out
	 * to the file */
	tmpdiff = UINT_MAX;
	/* check to see if the possynctime is much greater than the current
	 * dspframetime as can occur when a time offset has been added */
	if (possynctime < dspframetime + 10000) {
	    do {
		lastdiff = tmpdiff;
		/* get the timestamp */
		if (fread(&possynctime, sizeof(u32), 1, possynctimefile) != 1) {
		    fprintf(stderr, "Error reading position frame sync time from possynctime file\n");
		    exit(-1);
		}
		/* get the transition */
		if (fread(&c, sizeof(char), 1, possynctimefile) != 1) {
		    fprintf(stderr, "Error reading position frame sync type from possynctime file\n");
		    exit(-1);
		}
	    } while (((tmpdiff = abs(possynctime - dspframetime)) > 50) && 
		 (tmpdiff < lastdiff));
	}
	if (tmpdiff >= 1000) {
	    /* we should use the computed time instead of the read in time, as
	     * we may have missed the edge of a frame */
	    fwrite(&dspframetime, sizeof(u32), 1, outfile);
	    ncomputed++;
	}
	else {
	    fwrite(&possynctime, sizeof(u32), 1, outfile);
	    if (tmpdiff > maxframetimediff) {
		maxframetimediff = tmpdiff;
	    }
	}
    }
    fprintf(stderr, "Timestamp file created, %d gaps > 40 ms\nLargest discrepancy between computed and actual dsp frame times = %2.1f ms\nNumber of frames for which the computed timestamp was used = %d\n", ngaps, 
	    (float) maxframetimediff / 10, ncomputed);

    fclose(outfile);
    fclose(possynctimefile);
    fclose(cpudsptimefile);
    fclose(cpupostimefile);
}

int nextvalidcpudsptime(FILE *cpudsptimefile, u32 *cpudsp)
{
    /* read in the first valid cpu dsp correspondence records */
    do {
	if (fread(cpudsp, sizeof(u32), 3, cpudsptimefile) != 3) {
	    return 0;
	}
    } while (!feof(cpudsptimefile) && (cpudsp[2] - cpudsp[0] > 
		MAX_CPU_TIME_DELAY));
    return 1;
}
