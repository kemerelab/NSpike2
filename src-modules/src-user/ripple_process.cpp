#include "spikecommon.h"
#include "spike_dio.h"

#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"

extern SysInfo sysinfo;
extern UserDataInfo userdatainfo;
extern double fNumerator[NFILT];
extern double fDenominator[NFILT];

extern RippleStimParameters rippleStimParameters;
extern PulseCommand rippleStimPulseCmd;

extern RippleFilterStatus ripFiltStat[MAX_ELECTRODES];
extern SpeedFilterStatus speedFiltStat;

int counter;

int timeSinceLast;


void InitRipple(void)
{
  RippleFilterStatus *rptr;
  int i;


  rippleStimParameters.pulse_length = DIO_RT_DEFAULT_PULSE_LEN;
  rippleStimParameters.sampDivisor = DIO_RT_DEFAULT_SAMP_DIVISOR;
  rippleStimParameters.ripCoeff1 = DIO_RT_DEFAULT_RIPPLE_COEFF1;
  rippleStimParameters.ripCoeff2 = DIO_RT_DEFAULT_RIPPLE_COEFF2;
  rippleStimParameters.time_delay = DIO_RT_DEFAULT_RIPPLE_TIME_DELAY;
  rippleStimParameters.jitter = DIO_RT_DEFAULT_RIPPLE_JITTER;
  rippleStimParameters.ripple_threshold = DIO_RT_DEFAULT_RIPPLE_THRESHOLD;
  rippleStimParameters.n_above_thresh = DIO_RT_DEFAULT_RIPPLE_N_ABOVE_THRESH;
  rippleStimParameters.lockout = DIO_RT_DEFAULT_RIPPLE_LOCKOUT;
  rippleStimParameters.speed_threshold = DIO_RT_DEFAULT_RIPPLE_SPEED_THRESH;

  //rippleStimPulseCmd = GenerateSimplePulseCmd(rippleStimParameters.pulse_length);

  ResetRippleData();
  ResetRippleCounters();

  ResetSpeedData();
}

void ResetSpeedData()
{
  memset(speedFiltStat.speed, 0, sizeof(double) * NSPEED_FILT_POINTS);
  speedFiltStat.ind = NSPEED_FILT_POINTS - 1;
  speedFiltStat.lastx = 0;
  speedFiltStat.lasty = 0;
}

void ResetRippleData()
{
  int i;
  RippleFilterStatus *rptr = ripFiltStat;

  for (i = 0; i <= MAX_ELECTRODE_NUMBER ; i++, rptr++) {
    memset(rptr, 0, sizeof(RippleFilterStatus));
  }
}

void ResetRippleCounters(void)
{
  counter = 0;
  timeSinceLast = 0;
}

double updateLastval(RippleFilterStatus *rptr, double d) {
  int i;
  double mn = 0;
  
  for (i = 0; i < NLAST_VALS; i++) {
    mn += rptr->lastval[i];
  }

  mn = mn / (double) NLAST_VALS;

  rptr->lastval[rptr->lvind++] = d;

  if (rptr->lvind == NLAST_VALS)
    rptr->lvind = 0;

  return mn;
}

double RippleFilter(RippleFilterStatus *rptr, double d) {
  double val = 0;
  int jind;
  int i;

  rptr->fX[rptr->filtind] = d;
  rptr->fY[rptr->filtind] = 0;

  for (i = 0; i < NFILT; i++) {
    jind = (rptr->filtind + i) % NFILT;
    val = val + rptr->fX[jind] * fNumerator[i] - 
                rptr->fY[jind] * fDenominator[i];
  }
  rptr->fY[rptr->filtind] = val;

  rptr->filtind--;

  if (rptr->filtind < 0)
    rptr->filtind += NFILT;

  return val;
}


int ProcessRippleData(short electnum, double d) {
  int stim = 0;

//  static double v = 0.0;
//  static double posgain = 0.0;

  static u32 lastStimTimestamp = 0;

  double gain;
  double rd;

  RippleFilterStatus *rptr;

  rptr = ripFiltStat + electnum;

  timeSinceLast = timestamp - lastStimTimestamp;
  /* --------------------------------------------------- */
  /* 
     ARE WE IN LOCKOUT? 
   */
  if (timeSinceLast < rippleStimParameters.lockout) {
    /* We're in lockout period: need to zero filter inputs. */
    rd = RippleFilter(rptr, 0.0); // zero filter inputs
    // test
    rptr->currentVal = rptr->rippleMean;
    return 0;
  }
  /* --------------------------------------------------- */

  rd = RippleFilter(rptr, d); // (filter maintains state across calls)

  /* --------------------------------------------------- */
  /* 
     HAVE WE JUST STARTED? 
   */
  /* Wait at least 10000 samples before starting to stimulate
   * after ripple detection program is initially begun */
  if (counter <  10000) {
    counter++;
    return 0;
  }
  /* --------------------------------------------------- */

  /* --------------------------------------------------- */
  /* 
     OK TO PROCESS DATA...
   */
  double y = fabs(rd); // y is absolute value of ripple filtered

  /* Only update mean and standard deviation if NOT stimulating
   * this stops the stimulation artifact from changing the values */
  if (realtimeProcessingEnabled == 0) {
    rptr->rippleMean = rptr->rippleMean + (y - rptr->rippleMean)/rippleStimParameters.sampDivisor;
    rptr->rippleSd = (fabs(y - rptr->rippleMean) - rptr->rippleSd)/ 
	(double) rippleStimParameters.sampDivisor + rptr->rippleSd;
  }


  /* --------------------------------------------------- */
  /* The goods from Jim. */
  double df = y - rptr->currentVal;

  if (df > 0) {
    /* use the mean of the last 20 values to determine the gain for this increase */
    gain = rippleStimParameters.ripCoeff1;
    rptr->posgain = updateLastval(rptr, gain); // update to include this point
    rptr->currentVal = rptr->currentVal + df * rptr->posgain;
  }
  else {
    /* the gain for the decrease is fixed */
    gain = rippleStimParameters.ripCoeff2;
    rptr->posgain = updateLastval(rptr, gain); // update to include this point
    rptr->currentVal = rptr->currentVal + df * gain;
  }

  //fprintf(stderr, "y = %2.2f, df = %2.2f, currentval = %2.2f, gain = %2.2f, pg = %2.2f\n", y, df, rptr->currentVal, gain, rptr->posgain);

  /* --------------------------------------------------- */


 
  /* Jim's algorithm outputs a magic value "v" which is roughly
   * the envelope of ripple magnitude. Check to see if v has
   * crossed threshold and stimulate if so. */

  // The threshold is expressed in standard deviations above mean
  rptr->currentThresh = rptr->rippleMean + rptr->rippleSd * rippleStimParameters.ripple_threshold;
  if (nAboveRippleThresh(ripFiltStat) >= rippleStimParameters.n_above_thresh) {
    stim = 1;
  }

  /* --------------------------------------------------- */
  /* 
     LAST CHECKS:
   */
  if (realtimeProcessingEnabled == 0) // Escape here if we're not enabled to stimulate.
    return 0;

  if (ratSpeed > rippleStimParameters.speed_threshold) // Escape if rat is running too fast (speed is too high)
    return 0;
  /* --------------------------------------------------- */
  if (stim == 1) {
    timeSinceLast = 0; // If we're going to stimulate, reset lockout counter
    lastStimTimestamp = timestamp;
  }
  return stim;
}

int nAboveRippleThresh(RippleFilterStatus *rptr)
{
  int i, electnum;
  int nAbove = 0;

  for (i = 0; i <= userdatainfo.ncont; i++) {
    electnum = userdatainfo.contnum[i];
    if ((rptr[electnum].currentVal > rptr[electnum].currentThresh)) {
      nAbove++;
    }
  }
  return nAbove;
}      
  

void sendRippleStatusUpdate (void) {
  char tmps[1000];
  int i, electnum, offset = 0;;
  
  for (i = 0; i <= userdatainfo.ncont; i++) {
    electnum = userdatainfo.contnum[i];
    if (ripFiltStat[electnum].rippleMean != 0) {
      offset += sprintf(tmps+offset, "Elect %d: Ripple mean (std): %2.2f (%2.2f)\n", electnum,
	ripFiltStat[electnum].rippleMean, ripFiltStat[electnum].rippleSd);
    }
  }
  offset += sprintf(tmps+offset, "\nRat speed: %2.2f cm/sec\n",ratSpeed);
  sprintf(tmps+offset, "\nTimestamps since last %ld\n",timeSinceLast);
  SendMessage(client_data[SPIKE_MAIN].fd, DIO_RT_STATUS_RIPPLE_DISRUPT, tmps, 1000); 
}


