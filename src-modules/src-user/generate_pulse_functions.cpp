#include "stimcontrol_defines.h"

void ByteSwap(unsigned short *sptr, int nelem);

int GeneratePulseCommand(PulseCommand pulseCmd, unsigned short *command) {
  int len = 0;
  int i;
  int tick_pulse_on, tick_pulse_off;

  int laserPort1, laserPort2;
  uint16_t p1mask, p2mask;

  tick_pulse_on = pulseCmd.pulse_width * 3;
  tick_pulse_off = pulseCmd.inter_pulse_delay * 3;

  tick_pulse_on = tick_pulse_on - 3; // account for 3 word command processing
  tick_pulse_off = tick_pulse_off - 3; // account for command processing

  laserPort1 = (pulseCmd.pin1 / 16);
  p1mask = (uint16_t) (1 << (pulseCmd.pin1 % 16));
  if (pulseCmd.is_biphasic) {
    laserPort2 = (pulseCmd.pin2 / 16);
    p2mask = (uint16_t) (1 << (pulseCmd.pin2 % 16));
    if (laserPort1 != laserPort2)
      fprintf(stderr,"rt_user: warning - laser ports are different. probably won't work.");
  }

  fprintf(stderr,"rt_user: port %d and pin %d (biphasic: %d)\n",
      laserPort1, p1mask, pulseCmd.is_biphasic);

  for (i = 0; i < pulseCmd.n_pulses; i++) {
    command[len++] = DIO_S_SET_PORT | laserPort1; 
    if (pulseCmd.is_biphasic) {
      command[len++] = p1mask;
      command[len++] = DIO_S_WAIT | tick_pulse_on; 
      command[len++] = DIO_S_SET_PORT | laserPort1; 
      command[len++] = p2mask; 
      command[len++] = DIO_S_WAIT | tick_pulse_on; 
    }
    else {
      command[len++] = p1mask; 
      command[len++] = DIO_S_WAIT | tick_pulse_on; 
    }
    command[len++] = DIO_S_SET_PORT | laserPort1; 
    command[len++] = 0x00; 
    if (i < (pulseCmd.n_pulses-1))
      command[len++] = DIO_S_WAIT | tick_pulse_off;
  }

  return len;
}

void PulseLaserCommand (PulseCommand pulseCmd, int ignoreTimestamp) {
    // pulseWidth in 100 us units
    unsigned short command[DIO_MAX_COMMAND_LEN+3];
    int len = 0;
    u32 time_on;
    unsigned short time_on_low, bs_time_on_low;
    unsigned short time_on_high;
    u32 actualnext;

    if (commandCached == 1) {
      if (SendStartDIOCommand(0) <= 0) {
        fprintf(stderr,"rt_user: error sending data to master dsp.\n");
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

    if (!WriteDSPDIOCommand(command, len ,0 ,0)) {
        fprintf(stderr, "Error writing Digital IO command to Master DSP\n");
    }
 
    //SendMessage(client_data[SPIKE_MAIN].fd, DIO_COMMAND, (char *) command,  len * sizeof(unsigned short)); 
    //SendStartDIOCommand(0);
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

void PrepareStimCommand(PulseCommand pulseCmd)
{
  int len;
  unsigned short command[DIO_MAX_COMMAND_LEN+3];
  int whichstatemachine = 0;

  len = GeneratePulseCommand(pulseCmd, command);

  WriteDSPDIOCommand(command, len ,whichstatemachine ,0);
  fprintf(stderr,"rt_user: setting up pulse command on statemachine %d\n", whichstatemachine);

  commandCached = 1;
}

