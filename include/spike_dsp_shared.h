#ifndef __SPIKE_DSP_SHARED_H__
#define __SPIKE_DSP_SHARED_H__

int WriteDSPDIOCommand(unsigned short *command, int len, int statemachine=-1, int sendResetStateMachine=1);
int SendStartDIOCommand(int s);
int ResetStateMachine(int number);
int ResetStateMachines();
int WriteDSPDIORestartStateMachine(int s);
int LookForDSPDIOResponse(void);
int NextDIOStateMachine(void);
#ifndef DIO_ON_MASTER_DSP
int SetAOut(int aout, unsigned short level);
int WriteArbWaveForm(unsigned short *wavefm, int len);
int SetArbAOutChan(unsigned short aout);
int SetArbPointer(unsigned short offset);
int EnableArb(unsigned short enable);
int SetArbTrigger(unsigned short trigger);
int SetupArb(int arb);

#endif
#endif

