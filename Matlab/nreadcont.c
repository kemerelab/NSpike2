/*
    readcont.c 

    Reads in cont data from a single channel cont file between tstart and tend
    and puts it into a structure 

*/

#include "mexheader.h"
#include "mex.h"
#include "fileiomat.h"
#include <unistd.h>



#define NDIMS    1     /* number of dimensions in the data file */

/******************************************************************************
  INTERFACE FUNCTION
  */

void mexFunction(
    int           nlhs,           /* number of expected outputs */
    mxArray       *plhs[],        /* array of pointers to output arguments */
    int           nrhs,           /* number of inputs */
    const mxArray *prhs[])         /* array of pointers to input arguments */
{
    FILE             *efile;
    char             *contfile;
    char            *timestring1;
    char            *timestring2;
    char            tmpstring[200];
    const char         *fieldnames[] = {"descript", "fields", "starttime", "samprate", "data"};
    u32     tstart, tend;
    unsigned int    stringlen;
    NewContRec          tmpcontrec, emptycontrec;
    mxArray            *data;
    mxArray            *datatmp;
    mxArray            *datastruct;
    mxArray            *mxtmp;
    double            *tmpdata;
    u32   lasttime;
    double            *contdata;
    double            starttime;
    double            startoffset;
    double            reclen;
    double            *sampfreq;
    double            *nsamp;
    int             dims[2]= {1,1};
    int             ncontrec;
    int             offset;
    int             currenti;
    int             i, j, k, numinterp, nrec;
    

      /* Check numbers of arguments */
      if ((nrhs != 5)) {
           mexErrMsgTxt("Usage: contrec = readcont(contfile, sampfreq, nsamp, tstart, tend)");
    }
      if (nlhs < 1) {
        mexErrMsgTxt("readcont must be called with one output argument");
      }


    /* get the file name */
    stringlen = (mxGetM(prhs[0]) * mxGetN(prhs[0])) + 1;
    contfile = (char *) mxCalloc(stringlen, sizeof(char));
    mxGetString(prhs[0], contfile, stringlen);
    
    /* get the sampling frequency */
    sampfreq = mxGetPr(prhs[1]);
    /* sanity check */
    if ((*sampfreq < 1) || (*sampfreq > 30000)) {
       mexErrMsgTxt("Error: sampfreq must be between 1 and 300000");
    }

    /* get the nsamp per buffer parameter*/
    nsamp = mxGetPr(prhs[2]);
    /* sanity check */
    if ((*nsamp < 1) || (*nsamp > 1000)) {
       mexErrMsgTxt("Error: invalid value for nsamp; check the header of the cont file");
    }


    /* get tstart */
    stringlen = (mxGetM(prhs[3]) * mxGetN(prhs[3])) + 1;
    timestring1 = (char *) mxCalloc(stringlen, sizeof(char));
    mxGetString(prhs[3], timestring1, stringlen);
    tstart = ParseTimestamp(timestring1);

    /* get tend */
    stringlen = (mxGetM(prhs[4]) * mxGetN(prhs[4])) + 1;
    timestring2 = (char *) mxCalloc(stringlen, sizeof(char));
    mxGetString(prhs[4], timestring2, stringlen);
    tend = ParseTimestamp(timestring2);


    /* open the cont file */
    if ((efile = fopen(contfile, "r")) == NULL) {
        sprintf(tmpstring, "Error opening %s for reading\n", contfile);
        mexErrMsgTxt(tmpstring);
    }

    /* get rid of the header */    
    do {
        fgets(tmpstring, 200, efile);
    } while ((strncmp(tmpstring, "%%ENDHEADER", 11) != 0));

    /* get the first record of the file */
    tmpcontrec.data = (short *) mxCalloc(*nsamp, sizeof(short));
    readnewcontrec(efile, &tmpcontrec, *nsamp);
    
    /* allocate space for the data */
    emptycontrec.data = (short *) mxCalloc(*nsamp, sizeof(short));
    for (i = 0; i < (int) *nsamp; i++) {
	emptycontrec.data[i] = SHRT_MIN;
    }

    /* calculate the length (in time) of a record */
    reclen = (*nsamp / *sampfreq) * TIMESCALE;

    /* find the offsets of the first record to be included. */
    while ((tmpcontrec.timestamp < (tstart - reclen)) && (!feof(efile))) {
        readnewcontrec(efile, &tmpcontrec, *nsamp);
    } 
    if (feof(efile)) {
        sprintf(tmpstring, "Error in start time %s\n", timestring1);
        mexErrMsgTxt(tmpstring);
    }
    starttime = (double) tmpcontrec.timestamp / TIMESCALE;
    lasttime = tmpcontrec.timestamp;

    /* put the cont data2 into a Matlab structure with five fields */
    datastruct = mxCreateStructArray(2, dims, 5, fieldnames);

    /* set the first two fields */
    sprintf(tmpstring, "cont data from %s: %s to %s\n", contfile,
            timestring1, timestring2);
    mxtmp = mxCreateString((const char *)tmpstring); 
    mxSetField(datastruct, 0, fieldnames[0], mxtmp);
    sprintf(tmpstring, "contamplitude");
    mxtmp = mxCreateString((const char *)tmpstring); 
    mxSetField(datastruct, 0, fieldnames[1], mxtmp);


    /* put the start time in the starttime field */
    datatmp = mxCreateDoubleMatrix(1, 1, mxREAL);
    tmpdata = mxGetPr(datatmp);
    *tmpdata = (double) tmpcontrec.timestamp / TIMESCALE;
    starttime = *tmpdata;
    mxSetField(datastruct, 0, fieldnames[2], datatmp);

    /* calculate the number of records that need to be read in. Note that
     * because the clock cards run at slightly different frequencies than the
     * data acquisition cards, we add in a fudge factor of 1% to make sure we
     * have enough records */
    ncontrec =  (int) (ceil(((double) (tend - tstart)) / reclen) * 1.01);

    /* allocate space for the list of cont samples. As the reclen is not exact
     * due to rounding error in the sampling frequency, round the sampling
     * frequency upward to get an upper bound on the number of samples */

    contdata = (double *) mxCalloc(ncontrec * *nsamp, sizeof(double));

    /* copy the first set of samples to contdata */
    currenti = 0;
    for (i = 0 ; i < *nsamp; i++) {
        contdata[currenti++] = (double) tmpcontrec.data[i];
    }
    
    /* read in the rest of the records and put the cont data into tmpdata */
    nrec = 1;
    while(!feof(efile)) {
        /* break out of the loop if the timestamp is > tend */
	if ((readnewcontrec(efile, &tmpcontrec, *nsamp) == 0) || 
	    (tmpcontrec.timestamp > tend)) {
            break;
        }
        /* check for "holes" in the data where the difference between two
         * adjacent timestamps is > 1.5 * reclen */
        if ((tmpcontrec.timestamp - lasttime) < (.5 * reclen)) {
	    mexPrintf("short time %ld\n", tmpcontrec.timestamp - lasttime);
	}
        if ((tmpcontrec.timestamp - lasttime) > (1.5 * reclen)) {
            sprintf(tmpstring, "Error: excessive difference between current and last timestamps: %ld vs %ld\nFilling in difference with SHRT_MIN\n", tmpcontrec.timestamp, lasttime);
	    mexPrintf(tmpstring);
	    numinterp = round((double)(tmpcontrec.timestamp - lasttime) / 
		              reclen) - 1;
	    for (k = 0; k < numinterp; k++) {
		emptycontrec.timestamp = lasttime + (u32) reclen;
		/* copy the samples to tmpdata */
		nrec++;
		for (i = 0 ; i < *nsamp; i++) {
		    contdata[currenti++] = (double) emptycontrec.data[i];
		}
		lasttime += round(reclen);
	    }
        }
	/* copy the samples to tmpdata */
	nrec++;
	for (i = 0 ; i < *nsamp; i++) {
	    contdata[currenti++] = (double) tmpcontrec.data[i];
	}
	lasttime = tmpcontrec.timestamp;
    }    
    /* allocate space for the data */
    data = mxCreateDoubleMatrix(currenti, NDIMS, mxREAL);
    tmpdata = mxGetPr(data);

    /* copy the cont data into tmpdata */
    for (i = 0 ; i < currenti; i++) {
        tmpdata[i] = contdata[i];
    }

    /* set the data field */
    mxSetField(datastruct, 0, fieldnames[4], data);
    plhs[0] = datastruct;

    /* recalculate the sampling frequency based on the total time from starttime
     * to lasttime+reclen to correct for rounding errors */
    *sampfreq = currenti / ((double)(lasttime + reclen) / TIMESCALE - starttime);
/*    mexPrintf("comparison of file versus computed sampfreq: %lf vs. %lf\n",
               tmpcontrec.sampfreq, sampfreq);
    mexPrintf("%d,  %lf, %lf, %lf\n", currenti, (double) lasttime/ TIMESCALE,
                (double)reclen/ TIMESCALE,  starttime); */
    /* put the sampling frequency in the sampfreq field */
    datatmp = mxCreateDoubleMatrix(1, 1, mxREAL);
    tmpdata = mxGetPr(datatmp);
    *tmpdata = *sampfreq; 
    mxSetField(datastruct, 0, fieldnames[3], datatmp);

    /* free up the allocated variables  */
    mxFree(contfile);
    mxFree(timestring1);
    mxFree(timestring2);  
    fclose(efile); 
}







