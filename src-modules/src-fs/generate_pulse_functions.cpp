#include "stimcontrol_defines.h"

void StopOutput(PulseCommand *pulseCmd)
  // generate a command to stop the state machine and, if relevant, the analog output and the associated digital bit
{
  int len = 0;
  unsigned short aOutPort;
  unsigned short command[3];

  /* stop the state machine */
  //ResetStateMachine(pulseCmd->statemachine);

  if (pulseCmd->digital_only) {
    /* set the bits to be 0 */
    command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd->pin1; 
    command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd->pin2; 
    if (!WriteDSPDIOCommand(command, len ,pulseCmd->statemachine, 0)) {
      fprintf(stderr, "Error statemachine %d\n", pulseCmd->statemachine);
    }
  }
  else {
    /* turn off the analog output */
    aOutPort = (pulseCmd->aout == 1) ? DIO_AOUT1_PORT : DIO_AOUT2_PORT;
    command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd->pin1; 
    command[len++] = DIO_S_SET_PORT | aOutPort; 
    command[len++] = 0x00; 
    if (!WriteDSPDIOCommand(command, len, pulseCmd->statemachine, 0)) {
      fprintf(stderr, "Error stopping analog out %d\n", pulseCmd->aout);
    }
  }
  if (SendStartDIOCommand(0) <= 0) {
    fprintf(stderr,"feedback/stim: error sending start DIO command.\n");
  }
    fprintf(stderr, "turned off dio and aout, %d\n", aOutPort);
  return;
}


int GeneratePulseCommand(PulseCommand pulseCmd, unsigned short *command) {
  int len = 0;
  int i;
  int tick_pulse_on, tick_pulse_off;
  int waitsamp;

  int digitalPort1, digitalPort2;
  unsigned short aOutPort;
  unsigned short alevel;
  float ascale ;

  tick_pulse_on = pulseCmd.pulse_width * 3;
  tick_pulse_off = pulseCmd.inter_pulse_delay * 3;

  tick_pulse_on = tick_pulse_on - 3; // account for 3 word command processing
  tick_pulse_off = tick_pulse_off - 3; // account for command processing

  digitalPort1 = (pulseCmd.pin1 / 16);

  /* initialize the wait (wait for 0 samples) */
  command[len++] = DIO_S_WAIT;
  if (pulseCmd.pre_delay) {
    // now wait for (pre_delay-1) * 3 samples */
    waitsamp = (pulseCmd.pre_delay - 1) * SAMP_TO_TIMESTAMP;
    while (waitsamp > 65536) {
      command[len++] = DIO_S_WAIT_WAIT | 65533; // subtract 3 samples for processing
      waitsamp -= 65536;
    }
    if (waitsamp > 3) {
      command[len++] = DIO_S_WAIT_WAIT | (waitsamp - 3); 
    }
  }

  if (pulseCmd.digital_only) {
    if (pulseCmd.is_biphasic) {
      digitalPort2 = (pulseCmd.pin2 / 16);
      if (digitalPort1 != digitalPort2)
	fprintf(stderr,"rt_user: warning - digital ports are different. Make sure this is correct\n");
    }

    fprintf(stderr,"rt_user: port %d and pin %d (biphasic: %d)\n",
	digitalPort1, pulseCmd.pin1, pulseCmd.is_biphasic);

    for (i = 0; i < pulseCmd.n_pulses; i++) {
      command[len++] = DIO_S_SET_OUTPUT_HIGH | pulseCmd.pin1;
      if (pulseCmd.is_biphasic) {
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
	command[len++] = DIO_S_SET_OUTPUT_HIGH | pulseCmd.pin2;
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
      }
      else {
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
      }
      command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd.pin1;
      command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd.pin2;
      if (i < (pulseCmd.n_pulses-1))
	command[len++] = DIO_S_WAIT_WAIT| tick_pulse_off;
    }
  }
  else {
    /* create an analog pulse at the desired level. */
    if (pulseCmd.aout_mode == DIO_AO_MODE_CONTINUOUS) {
      alevel = (unsigned short) ((pulseCmd.minv + 
			   ((float) ((pulseCmd.maxv - pulseCmd.minv) * 
		   ((float) pulseCmd.cont_percent) / 100.0))) * USHRT_MAX); 
    }
    else {
      alevel = (unsigned short) ((pulseCmd.minv + 
			   ((float) ((pulseCmd.maxv - pulseCmd.minv) * 
		   ((float) pulseCmd.pulse_percent) / 100.0))) * USHRT_MAX); 
    }
    aOutPort = (pulseCmd.aout == 1) ? DIO_AOUT1_PORT : DIO_AOUT2_PORT;
    for (i = 0; i < pulseCmd.n_pulses; i++) {
      /* set the digital port to 1 as our signal that we've changed the output */
      command[len++] = DIO_S_SET_OUTPUT_HIGH | pulseCmd.pin1;
      /* set the analog port level */
      command[len++] = DIO_S_SET_PORT | aOutPort; 
      command[len++] = alevel;

      fprintf(stderr, "writing to aout %d port %d, level %d\n", pulseCmd.aout, aOutPort, alevel);


      /* if this is not continuous output mode, we need to turn it off at the
       * desired time */
      if (pulseCmd.aout_mode != DIO_AO_MODE_CONTINUOUS) {
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
	/* first turn off the analog output and then signal the change */
	command[len++] = DIO_S_SET_PORT | aOutPort; 
	command[len++] = 0x00; 
	command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd.pin1;
      }

      if (i < (pulseCmd.n_pulses-1))
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_off;
    }
  }
  return len;
}

void PulseOutputCommand (PulseCommand pulseCmd, int ignoreTimestamp) {
  // pulseWidth in 100 us units
  unsigned short command[DIO_MAX_COMMAND_LEN+3];
  int len = 0;
  u32 time_on;
  unsigned short time_on_low, bs_time_on_low;
  unsigned short time_on_high;
  u32 actualnext;

  if (commandCached == 1) {
    if (SendStartDIOCommand(0) <= 0) {
      fprintf(stderr,"feedback/stim: error sending start DIO command.\n");
    }
  }


  if (ignoreTimestamp == 0) {
    if (pulseCmd.start_samp_timestamp <= timestamp * SAMP_TO_TIMESTAMP) {
      fprintf(stderr,"rt_user: Tried to generate future timestamp pulse before now.\n");
      return;
    }

    time_on = pulseCmd.start_samp_timestamp;
    time_on_low = (time_on & 0xFFFF);
    time_on_high = (unsigned short) ((time_on & 0xFFFF0000) >> 16);
    actualnext = time_on_low + (time_on & 0xFFFF0000);

    command[0] = DIO_S_WAIT_TIME; // next two samps are time_on
    command[1] = *( ((unsigned short *) &time_on) + 1 );
    command[2] = time_on_low;

    len = GeneratePulseCommand(pulseCmd, &command[3]);
    len = len + 3;
  }
  else {
    len = GeneratePulseCommand(pulseCmd, command);

  }

  if (!WriteDSPDIOCommand(command, len, pulseCmd.statemachine, 0)) {
    fprintf(stderr, "Error writing Digital IO command\n");
  }
}

u32 PulseCommandLength(PulseCommand pulseCmd) {
    u32 len = 0;
    int i;
    int tick_pulse_on, tick_pulse_off;

    tick_pulse_on = pulseCmd.pulse_width * SAMP_TO_TIMESTAMP;
    tick_pulse_off = pulseCmd.inter_pulse_delay * 3;

    for (i = 0; i < pulseCmd.n_pulses; i++) {
      len += tick_pulse_on;
      if (pulseCmd.is_biphasic)
        len += tick_pulse_on;
      if (i < (pulseCmd.n_pulses-1))
        len += tick_pulse_off;
    }

    return len;
}

void PrepareStimCommand(PulseCommand *pulseCmd, int nPulses)
{
  int i, len = 0;
  unsigned short command[DIO_MAX_COMMAND_LEN+3];
  int whichstatemachine = pulseCmd->statemachine;

  len = GeneratePulseCommand(*pulseCmd, command);

  if (!WriteDSPDIOCommand(command, len ,whichstatemachine, 0)) {
    fprintf(stderr, "Error writing Digital IO command\n");
  }
  else {
    fprintf(stderr,"rt_user: setting up pulse command on statemachine %d\n", whichstatemachine);
  }
  commandCached = 1;
}

