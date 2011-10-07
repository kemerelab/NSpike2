#include "stimcontrol_defines.h"

extern CommandTimeInfo ctinfo;

void StopOutput(PulseCommand *pulseCmd)
  // generate a command to stop the state machine and, if relevant, the analog output and the associated digital bit
{
  int len = 0;
  unsigned short aOutPort;
  unsigned short command[3];

  /* stop the state machine */
  ResetStateMachine(pulseCmd->statemachine);

  /* Wait for the state machine pointer to go to 0 so that we
   * know it has been stopped */
  while (ReadStateMachinePtr(pulseCmd->statemachine)) {
    usleep(2000);
  }

  if (pulseCmd->digital_only) {
    /* set the bits to be 0 */
    command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd->pin1; 
    command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd->pin2; 
    if (!WriteDSPDIOCommand(command, len ,pulseCmd->statemachine, 0)) {
      fprintf(stderr, "Error writing to statemachine %d\n", pulseCmd->statemachine);
    }
  }
  else {
    if (pulseCmd->aout_mode == DIO_AO_MODE_WAVE) {
      /* disable the arbitrary waveform generator */
      EnableArb(0);
    }
    /* turn off the analog output */
    aOutPort = (pulseCmd->aout == 0) ? DIO_AOUT1_PORT : DIO_AOUT2_PORT;
    command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd->pin1; 
    command[len++] = DIO_S_SET_PORT | aOutPort; 
    command[len++] = 0x00; 
    if (!WriteDSPDIOCommand(command, len, pulseCmd->statemachine, 0)) {
      fprintf(stderr, "Error stopping analog out %d\n", pulseCmd->aout);
    }

  }
  if (SendStartDIOCommand(pulseCmd->statemachine) <= 0) {
    fprintf(stderr,"feedback/stim: error sending start DIO command.\n");
  }
  if (pulseCmd->aout_mode == DIO_AO_MODE_WAVE) {
      /* re-enable the arbitrary waveform generator */
      EnableArb(1);
  }
  /* Wait for the state machine pointer to go to 0 so that we
   * know the output has been turned off */
  while (ReadStateMachinePtr(pulseCmd->statemachine)) {
    usleep(2000);
  }


  fprintf(stderr, "turned off dio and aout, %d\n", aOutPort);
  return;
}


int GeneratePulseCommand(PulseCommand pulseCmd, unsigned short *command) {
  int len = 0;
  int i;
  int tick_pulse_on, tick_pulse_off;
  int waitsamp;
  bool use_arb;

  u32 time_on, time_on_low, time_on_high;

  int digitalPort1, digitalPort2;
  unsigned short aOutPort;
  unsigned short alevel;
  float ascale ;

  tick_pulse_on = pulseCmd.pulse_width * 3;
  tick_pulse_off = pulseCmd.inter_pulse_delay * 3;

  digitalPort1 = (pulseCmd.pin1 / 16);

  /* initialize the wait (wait for 0 samples) */
  command[len++] = DIO_S_WAIT;

  
  if (pulseCmd.start_samp_timestamp) {
    /* set the sample at which the command should start */
    time_on = pulseCmd.start_samp_timestamp;
    time_on_low = (time_on & 0xFFFF);
    time_on_high = (unsigned short) ((time_on & 0xFFFF0000) >> 16);

    command[len++] = DIO_S_WAIT_TIME; // next two samps are time_on
    command[len++] = *( ((unsigned short *) &time_on) + 1 );
    command[len++] = time_on_low;
    /* the approximate additional wait time is the difference between the most 
     * recent timestamp and the start time */
    ctinfo.command_time += pulseCmd.start_samp_timestamp - timestamp;
  }

  if (pulseCmd.pre_delay) {
    /* add the initial delay */
    len += AddWaitToCommand(pulseCmd.pre_delay, command + len, 
	    &ctinfo.command_time);
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
      ctinfo.command_time += pulseCmd.pulse_width;
      if (pulseCmd.is_biphasic) {
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
	command[len++] = DIO_S_SET_OUTPUT_HIGH | pulseCmd.pin2;
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
	ctinfo.command_time += pulseCmd.pulse_width;
      }
      else {
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
      }
      command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd.pin1;
      command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd.pin2;
      if (i < (pulseCmd.n_pulses-1))
	command[len++] = DIO_S_WAIT_WAIT| tick_pulse_off;
	ctinfo.command_time += pulseCmd.inter_pulse_delay;
    }
  }
  else {
    use_arb = false;
    /* create an analog pulse at the desired level. */
    switch (pulseCmd.aout_mode) {
      case (DIO_AO_MODE_CONTINUOUS) :
	alevel = (unsigned short) ((pulseCmd.minv + 
			     ((float) ((pulseCmd.maxv - pulseCmd.minv) * 
		     ((float) pulseCmd.cont_percent) / 100.0))) * USHRT_MAX); 

        EnableArb(0);
	break;
      case (DIO_AO_MODE_PULSE):
	alevel = (unsigned short) ((pulseCmd.minv + 
		     ((float) ((pulseCmd.maxv - pulseCmd.minv) * 
		     ((float) pulseCmd.pulse_percent) / 100.0))) * USHRT_MAX); 
        EnableArb(0);
	break;
      case (DIO_AO_MODE_WAVE):
	SetupArb(pulseCmd.arbinfo);
	use_arb = true;
	break;
      default:
	fprintf(stderr, "Error in spike_fsdata: DIO mode %d unknown\n", pulseCmd.aout_mode);
    }
    aOutPort = (pulseCmd.aout == 0) ? DIO_AOUT1_PORT : DIO_AOUT2_PORT;
    for (i = 0; i < pulseCmd.n_pulses; i++) {
      /* set the digital port to 1 as our signal that we've changed the output */
      command[len++] = DIO_S_SET_OUTPUT_HIGH | pulseCmd.pin1;
      if (!use_arb) {
	/* set the analog port level */
	command[len++] = DIO_S_SET_PORT | aOutPort; 
	command[len++] = alevel;
      }

      /* if this is not continuous mode, we need to turn it off at the
       * desired time */
      if ((pulseCmd.aout_mode != DIO_AO_MODE_CONTINUOUS) &&
          !(use_arb && pulseCmd.arbinfo.continuous)) {
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_on; 
	if (!use_arb) {
	  /* turn off the analog output */
	  command[len++] = DIO_S_SET_PORT | aOutPort; 
	  command[len++] = 0x00; 
	}
	/* reset the digital output */
	command[len++] = DIO_S_SET_OUTPUT_LOW | pulseCmd.pin1;
	ctinfo.command_time += pulseCmd.pulse_width;
      }
      else {
        /* indicate that the command will not complete */
	ctinfo.command_time = INT_MAX;
      }

      if (i < (pulseCmd.n_pulses-1)) {
	command[len++] = DIO_S_WAIT_WAIT | tick_pulse_off;
        if (ctinfo.command_time < UINT_MAX) {
          ctinfo.command_time += pulseCmd.inter_pulse_delay;
        }
      }
    }
  }
  return len;
}

void PulseOutputCommand (PulseCommand pulseCmd) 
{
  if (SendStartDIOCommand(pulseCmd.statemachine) <= 0) {
    fprintf(stderr,"feedback/stim: error sending start DIO command.\n");
  }
  ctinfo.command_cached = 0;
  /* set the time that the next command can be sent if we have not already set 
   * it to be UINT_MAX, which indicates continuous mode */
  if (ctinfo.next_command_time != UINT_MAX) {
    ctinfo.next_command_time = ctinfo.timestamp + ctinfo.command_time;
  }
  ctinfo.message_sent = false;
}

void PrepareStimCommand(PulseCommand *pulseCmd, int nPC)
{
  int i, r, len = 0;
  bool continuous = false;
  unsigned short command[DIO_MAX_COMMAND_LEN];
  u32		cttmp;

  /* reset the command_time so we can calculate it correctly */
  ctinfo.command_time = 0;

  for (i = 0; i < nPC; i++) {
    cttmp = ctinfo.command_time;
    if (pulseCmd[i].pulse_width == DIO_PULSE_COMMAND_END) {
      break;
    }

    if (pulseCmd[i].n_repeats > 1) {
      /* loop through the repeats */
      command[len++] = DIO_S_FOR_BEGIN | (pulseCmd[i].n_repeats + 1);
    }
    len += GeneratePulseCommand(*pulseCmd, command+len);

    /* add in the inter_frame_delay wait  */
    len += AddWaitToCommand(pulseCmd->inter_frame_delay, command + len, 
	    &ctinfo.command_time);
    if (pulseCmd[i].n_repeats > 1) {
      /* loop through the repeats */
      command[len++] = DIO_S_FOR_END;
      /* calculate the time for this command */
      cttmp = ctinfo.command_time - cttmp;
      /* add in the time for the repeats */
      ctinfo.command_time += cttmp * (pulseCmd[i].n_repeats - 1);
    }
    if (pulseCmd[i].n_repeats == -1) {
      // This only occurs for the last pulseCmd, where we then run continuously.
      // To do so, jump back to offset 1 and indicate that the command will not finish
      command[len++] = DIO_S_JUMP_ABS | 1;
      ctinfo.next_command_time = UINT_MAX;
    }
  }
  if (!WriteDSPDIOCommand(command, len, pulseCmd->statemachine, 0)) {
    fprintf(stderr, "Error writing Digital IO command to statemachine %d\n", 
	pulseCmd->statemachine);
  }
  ctinfo.command_cached = 1;
}

