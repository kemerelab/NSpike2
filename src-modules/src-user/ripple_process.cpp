#include "spikecommon.h"
#include "spike_dio.h"

#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"

#include "filtercoeffs.h"

extern DaqToUserInfo           daq_to_user_info;

extern RippleStimParameters rippleStimParameters;
extern PulseCommand rippleStimPulseCmd;

// ripple processing data:
#define NLAST_VALS 20

double lastval[NLAST_VALS];
int counter;

double rippleMean;
double rippleSd;
double muaMean;
double muaSd;
int timeSinceLast;

double fX[NFILT];
double fY[NFILT];

void InitRipple(void)
{
  rippleStimParameters.pulse_length = DIO_RT_DEFAULT_PULSE_LEN;
  rippleStimParameters.sampDivisor = DIO_RT_DEFAULT_SAMP_DIVISOR;
  rippleStimParameters.ripCoeff1 = DIO_RT_DEFAULT_RIPPLE_COEFF1;
  rippleStimParameters.ripCoeff2 = DIO_RT_DEFAULT_RIPPLE_COEFF2;
  rippleStimParameters.muaCoeff1 = DIO_RT_DEFAULT_MUA_COEFF1;
  rippleStimParameters.muaCoeff2 = DIO_RT_DEFAULT_MUA_COEFF2;
  rippleStimParameters.time_delay = DIO_RT_DEFAULT_RIPPLE_TIME_DELAY;
  rippleStimParameters.jitter = DIO_RT_DEFAULT_RIPPLE_JITTER;
  rippleStimParameters.ripple_threshold = DIO_RT_DEFAULT_RIPPLE_THRESHOLD;
  rippleStimParameters.mua_threshold = DIO_RT_DEFAULT_MUA_THRESHOLD;
  rippleStimParameters.lockout = DIO_RT_DEFAULT_RIPPLE_LOCKOUT;
  rippleStimParameters.speed_threshold = DIO_RT_DEFAULT_RIPPLE_SPEED_THRESH;

  //rippleStimPulseCmd = GenerateSimplePulseCmd(rippleStimParameters.pulse_length);

  ResetRippleData();
}

void ResetRippleData(void)
{
  int i;

  for (i = 0; i < NLAST_VALS; i++)
    lastval[i] = 0;

  for (i = 0; i < NFILT; i++) {
    fX[i] = 0; fY[i] = 0;
  }

  counter = 0;

  rippleMean = 0.0;
  rippleSd = 0.0;
  muaMean = 0.0;
  muaSd = 0.0;
  timeSinceLast = 0;
}

double updateLastval(double d) {
  static int ind = 0;
  int i;
  double mn = 0;
  
  for (i = 0; i < NLAST_VALS; i++) 
    mn += lastval[i];

  mn = mn/NLAST_VALS;

  lastval[ind++] = d;
  if (ind == 20)
    ind = 0;

  return mn;
}

double RippleFilter(double d) {
  static int ind = 0;
  double val = 0;
  int jind;
  int i;

  fX[ind] = d;
  fY[ind] = 0;

  for (i = 0; i < NFILT; i++) {
    jind = (ind + i) % NFILT;
    val = val + fX[jind] * fNumerator[i] - 
                fY[jind] * fDenominator[i];
  }
  fY[ind] = val;

  ind--;

  if (ind < 0)
    ind += NFILT;

  return val;
}


int ProcessRippleData(double d) {
  int stim = 0;

  static double v = 0.0;
  static double posgain = 0.0;

  static u32 lastStimTimestamp = 0;

  double gain;
  double rd;


  timeSinceLast = timestamp - lastStimTimestamp;
  /* --------------------------------------------------- */
  /* 
     ARE WE IN LOCKOUT? 
   */
  if (timeSinceLast < rippleStimParameters.lockout) {
    /* We're in lockout period: need to zero filter inputs. */
    rd = RippleFilter(0.0); // zero filter inputs
    return 0;
  }
  /* --------------------------------------------------- */

  rd = RippleFilter(d); // (filter maintains state across calls)

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
  double y = abs(rd); // y is absolute value of ripple filtered

  /* Only update mean and standard deviation if NOT stimulating
   * this stops the stimulation artifact from changing the values */
  if (realtimeProcessingEnabled == 0) {
    rippleMean = rippleMean + (y - rippleMean)/rippleStimParameters.sampDivisor;
    rippleSd = (fabs(y - rippleMean) - rippleSd)/ 
	(double) rippleStimParameters.sampDivisor + rippleSd;
  }

  // The threshold is expressed in standard deviations above mean
  double thr_tmp = rippleMean + rippleStimParameters.ripple_threshold * rippleSd;

  /* --------------------------------------------------- */
  /* The goods from Jim. */
  double df = y - v;

  if (df > 0) {
    gain = rippleStimParameters.ripCoeff1;
    v = v + df * posgain;
  }
  else {
    gain = rippleStimParameters.ripCoeff2;
    v = v + df * posgain;
  }

  posgain = updateLastval(gain); // ready for next time
  /* --------------------------------------------------- */


 
  /* Jim's algorithm outputs a magic value "v" which is roughly
   * the envelope of ripple magnitude. Check to see if v has
   * crossed threshold and stimulate if so. */
  if (v > thr_tmp) 
    stim = 1;

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

void sendRippleStatusUpdate (void) {
  RippleStatusMsg status;

  status.ripMean = rippleMean;
  status.ripStd = rippleSd;
  status.muaMean = muaMean;
  status.muaStd = muaSd;
  status.sincelast = timeSinceLast;
  status.isRunning = realtimeProcessingEnabled;
  status.ratSpeed = ratSpeed;
  SendMessage(client_data[SPIKE_MAIN].fd, DIO_RT_STATUS_RIPPLE_DISRUPT, (char *) &(status),  sizeof(RippleStatusMsg)); 
}


