#include "stimcontrol_defines.h"

void ByteSwap(unsigned short *sptr, int nelem);


void ParsePulseFile (char *inData, PulseCommand *pulseArray) {
  char *tok;

  int matches;
  char command;
  int pulseWidth, repetitions, interPulseDelay, multipleRepetitions;
  float frequency;

  int pulseInd = 0;

  int i;

  int line = 0;

  tok = strtok(inData,"\n");
  while (tok != NULL) {
    matches = sscanf(tok, "%c %d %f %d %d %d", &command, 
        &pulseWidth, &frequency, &repetitions, &interPulseDelay, 
        &multipleRepetitions);
    fprintf(stderr,"rt_user: %s|",tok);
    if (matches < 1)
      fprintf(stderr,"Bad command\n");
    else {
      switch (command) {
      case ';' :
        fprintf(stderr,"Comment\n");
        break;
      case 'd' :
        fprintf(stderr,"Delay, ");
        if (matches < 2)
          fprintf(stderr,"[parse error]\n");
        else {
          fprintf(stderr,"%d ms\n", pulseWidth);
          pulseArray[pulseInd].type = 'd';
          pulseArray[pulseInd].line = line;
          pulseArray[pulseInd].is_part_of_sequence = 0;
          pulseArray[pulseInd++].delay = pulseWidth;
        }
        break;
      case 'r' :
        fprintf(stderr,"Repeat to beginning\n");
        fprintf(stderr,"%d ms\n", pulseWidth);
        pulseArray[pulseInd].type = 'r';
        pulseArray[pulseInd].line = line;
        pulseArray[pulseInd++].is_part_of_sequence = 0;
        break;
      case 'p' :
        fprintf(stderr,"Single pulse, ");
        if (matches < 2)
          fprintf(stderr,"[parse error]\n");
        else {
          fprintf(stderr,"%d (100 us units)\n", pulseWidth);
          pulseArray[pulseInd].type = 'p';
          pulseArray[pulseInd].line = line;
          pulseArray[pulseInd].is_part_of_sequence = 0;
          pulseArray[pulseInd].repetitions = 1;
	  pulseArray[pulseInd].frequency = 1; // b/c of a "feature"
          pulseArray[pulseInd++].pulse_width = pulseWidth;
        }
        break;
      case 's' :
        fprintf(stderr,"Pulse sequence, ");
        if (matches == 4) {
          fprintf(stderr,"single pulse set: %d %d ms pulses at %f Hz\n",
              repetitions, pulseWidth, frequency);
          fprintf(stderr,"%d (100 us units)\n", pulseWidth);
          pulseArray[pulseInd].type = 's';
          pulseArray[pulseInd].line = line;
          pulseArray[pulseInd].pulse_width = pulseWidth;
          pulseArray[pulseInd].frequency = frequency;
          pulseArray[pulseInd].repetitions = repetitions;
          pulseArray[pulseInd++].is_part_of_sequence = 0;
        }
        else if (matches == 6) {
          fprintf(stderr,"multiple pulse set: %d %d ms pulses at %f Hz. ",
              repetitions, pulseWidth, frequency);
          fprintf(stderr,"repeated %d times, with %d ms separation.\n",
              multipleRepetitions, interPulseDelay);
          pulseArray[pulseInd].type = 's';
          pulseArray[pulseInd].line = line;
          pulseArray[pulseInd].pulse_width = pulseWidth;
          pulseArray[pulseInd].frequency = frequency;
          pulseArray[pulseInd].repetitions = repetitions;
          for (i = 0; i < (multipleRepetitions - 1); i++) {
            pulseArray[pulseInd++].is_part_of_sequence = 1;
            pulseArray[pulseInd].type = 'd';
            pulseArray[pulseInd].line = line;
            pulseArray[pulseInd].is_part_of_sequence = 1;
            pulseArray[pulseInd++].delay = interPulseDelay;
            pulseArray[pulseInd].type = 's';
            pulseArray[pulseInd].line = line;
            pulseArray[pulseInd].pulse_width = pulseWidth;
            pulseArray[pulseInd].frequency = frequency;
            pulseArray[pulseInd].repetitions = repetitions;
          }
          pulseArray[pulseInd++].is_part_of_sequence = 0;
        }
        else
          fprintf(stderr,"[parse error]\n");
        break;
      default :
        fprintf(stderr,"Unknown command type\n");
        break;
      }
    }
    tok = strtok(NULL,"\n");

    if (pulseInd >= MAX_PULSES) {
      fprintf(stderr,"rt_user: !! Maximum number of pulses exceeded. !!\n");
      break;
    }

    line++;
  }
  pulseArray[pulseInd].type = 0; // mark end of pulseArray
  pulseArray[pulseInd].start_samp_timestamp = 0; // mark end of pulseArray

}

void PulseLaser (int pulseWidth) {
    // pulseWidth in 100 us units
    unsigned short command[5];
    int tick_pulse_on;

    tick_pulse_on = pulseWidth * 3; // convert to 30 kHz ticks
    command[0] = DIO_S_SET_PORT | laserPort; 
    command[1] = pin1; 
    command[2] = DIO_S_WAIT | tick_pulse_on; 
    command[3] = DIO_S_SET_PORT | laserPort; 
    command[4] = 0x00; 

    SendMessage(outputfd, DIO_COMMAND, (char *) command,  
      5 * sizeof(unsigned short)); 

    fprintf(stderr, "rt_user: P: %d x 100 us\n", pulseWidth);
}

int GeneratePulseCommand(PulseCommand pulseCmd, unsigned short *command) {
    int len = 0;
    int i;
    int tick_pulse_on, tick_pulse_off;

    tick_pulse_on = pulseCmd.pulse_width * 3;
    tick_pulse_off = round(30000.0 / pulseCmd.frequency);
    tick_pulse_off = tick_pulse_off - tick_pulse_on;

    tick_pulse_on = tick_pulse_on - 3; // account for 3 word command processing
    tick_pulse_off = tick_pulse_off - 3; // account for command processing

    if (tick_pulse_off <= 0) {
	fprintf(stderr,"rt_user: S: Warning - no delay in frequency... ");
	tick_pulse_off = 1;
	fprintf(stderr,"New frequency: %f Hz\n", 
		(30000.0/(tick_pulse_on + tick_pulse_off)));
    }

    if (tick_pulse_on > 0x7FFF) {
	fprintf(stderr,"Pulse on, %d ticks, too long.\n", tick_pulse_on);
	return -1;
    }
    if (tick_pulse_off > 0x7FFF) {
	fprintf(stderr,"Pulse off, %d ticks, too long.\n", tick_pulse_off);
	return -1;
    }

#ifdef DEBUG_USER
    fprintf(stderr,"rt_user: S: %d ms pulses, %6.3fHz \n", 
      pulseCmd.pulse_width, (30000.0/(tick_pulse_on + tick_pulse_off)));
#endif

    if (pulseCmd.repetitions > 10) {
	fprintf(stderr,"Number of pulses, %d, too many.\n", pulseCmd.repetitions);
	return -1;
    }

    for (i = 0; i < pulseCmd.repetitions; i++) {
        command[len++] = DIO_S_SET_PORT | laserPort; 
        if (biphasicStim) {
          command[len++] = pin1; 
          command[len++] = DIO_S_WAIT | tick_pulse_on; 
          command[len++] = DIO_S_SET_PORT | laserPort; 
          command[len++] = pin2; 
          command[len++] = DIO_S_WAIT | tick_pulse_on; 
        }
        else {
          command[len++] = pin1; 
          command[len++] = DIO_S_WAIT | tick_pulse_on; 
        }
        command[len++] = DIO_S_SET_PORT | laserPort; 
        command[len++] = 0x00; 
	  if (i < (pulseCmd.repetitions-1))
	      command[len++] = DIO_S_WAIT | tick_pulse_off;
    }

#ifdef DEBUG_USER
    fprintf(stderr,"rt_user: Pulse command length: %d - ", len);
    for (i = 0; i < len; i++)
	    fprintf(stderr, "%d",command[i]);
    fprintf(stderr,"\n");
#endif
    return len;

}


void PulseLaserCommand (PulseCommand pulseCmd, int ignoreTimestamp) {
    // pulseWidth in 100 us units
    unsigned short command[DIO_MAX_COMMAND_LEN+3];
    int len = 0;
    u32 time_on;
    unsigned short time_on_low, bs_time_on_low;
    u32 actualnext;

    if (commandCached == 1) {
      if (SendStartDIOCommand() <= 0)
        fprintf(stderr,"rt_user: error sending data to master dsp.\n");
      return;
    }


    if (ignoreTimestamp == 0) {

      if (pulseCmd.start_samp_timestamp <= timestamp * SAMP_TO_TIMESTAMP) {
        fprintf(stderr,"rt_user: Tried to generate future timestamp pulse before now.\n");
        return;
      }

      time_on = pulseCmd.start_samp_timestamp;
      time_on_low = (time_on & 0xFFFF);
      /* if ((time_on_low > 0x8000) && (time_on_low < 0xD000))
        time_on_low = 0xD000; */
      actualnext = time_on_low + (time_on & 0xFFFF0000);

      command[0] = DIO_S_WAIT_TIME; // next two samps are time_on
      command[1] = *( ((unsigned short *) &time_on) + 1 );
      command[2] = time_on_low;
      // command[2] = *( ((unsigned short *) &time_on) );
#ifdef DEBUG_USER
      fprintf(stderr,"rt_user: command %04x %04x (next: %d)\n", command[1], time_on_low, actualnext/3);
#endif

      len = GeneratePulseCommand(pulseCmd, &command[3]);
      len = len + 3;
    }
    else {
      len = GeneratePulseCommand(pulseCmd, command);
    }

    SendMessage(outputfd, DIO_COMMAND, (char *) command,  len * sizeof(unsigned short)); 
}

u32 PulseCommandLength(PulseCommand pulseCmd) {
    u32 len = 0;
    int i;
    int tick_pulse_on, tick_pulse_off;

    tick_pulse_on = pulseCmd.pulse_width * SAMP_TO_TIMESTAMP;
    tick_pulse_off = round(30000.0 / pulseCmd.frequency);
    tick_pulse_off = tick_pulse_off - tick_pulse_on;

    if (tick_pulse_off <= 0) {
	tick_pulse_off = 1;
    }

    if (tick_pulse_on > 0x7FFF) {
	return -1;
    }
    if (tick_pulse_off > 0x7FFF) {
	return -1;
    }

    if (pulseCmd.repetitions > 10) {
	return -1;
    }

    for (i = 0; i < pulseCmd.repetitions; i++) {
      len += tick_pulse_on;
      if (biphasicStim)
        len += tick_pulse_on;
      if (i < (pulseCmd.repetitions-1))
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

  SendMessage(outputfd, DIO_SPEC_STATEMACHINE, (char *)&whichstatemachine,  sizeof(int)); 
  SendMessage(outputfd, DIO_COMMAND_TO_STATEMACHINE, (char *) command,  len * sizeof(unsigned short)); 
  //SendMessage(outputfd, DIO_RUN_STATEMACHINE, (char *)&whichstatemachine,  sizeof(int)); 
  fprintf(stderr,"rt_user: setting up pulse command on statemachine %d\n", whichstatemachine);

  commandCached = 1;
}

