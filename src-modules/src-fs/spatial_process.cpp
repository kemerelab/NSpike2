#include "spikecommon.h"
#include "spike_dio.h"

#include "stimcontrol_defines.h"
#include "spike_stimcontrol_defines.h"

extern SysInfo sysinfo;
extern FSDataInfo fsdatainfo;

extern SpatialStimParameters spatialStimParameters;
extern PulseCommand spatialStimPulseCmd;

extern SpeedFilterStatus speedFiltStat;
extern SpatialFilterStatus spatialFiltStat;

void InitSpatial(void)
{
  fprintf(stderr, "Initializing spatial variables\n");
  spatialFiltStat.stimOn = false;
  ResetSpeedData();
}

bool ProcessSpatialData(u32 xpos, u32 ypos) 
{
  /* update the current location */
  spatialStimParameters.xpos = xpos;
  spatialStimParameters.ypos = ypos;


  if (realtimeProcessingEnabled == 0) {
    return 0;
  }

  /* check that the speed is in the right window */
  if ((ratSpeed < spatialStimParameters.minSpeed) ||
      (ratSpeed > spatialStimParameters.maxSpeed)) {
    fprintf(stderr, "speed low %f\n", ratSpeed);
    return 0;
  }

  return ((xpos >= spatialStimParameters.lowerLeftX) && 
          (xpos <= spatialStimParameters.upperRightX) && 
          (ypos >= spatialStimParameters.lowerLeftY) && 
          (ypos <= spatialStimParameters.upperRightY));

}



void sendSpatialStatusUpdate (void) {
  char tmps[1000];

  int offset = 0;
  
  if (realtimeProcessingEnabled) {
    offset += sprintf(tmps, "Spatial Stimulation ENABLED\n\n");
  }
  else {
    offset += sprintf(tmps, "Spatial Stimulation disabled\n\n");
  }

  offset += sprintf(tmps+offset, "Rat location %d, %d\n\nStimulation: ", 
	  spatialStimParameters.xpos, spatialStimParameters.ypos);
  if (spatialFiltStat.stimOn) {
    sprintf(tmps+offset, "ON\n");
  }
  else {
    sprintf(tmps+offset, "OFF\n");
  }

  SendMessage(client_data[SPIKE_MAIN].fd, DIO_RT_STATUS_SPATIAL_STIM, tmps, 1000); 
}


