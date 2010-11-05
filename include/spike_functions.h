/* Common Functions */

void SetupFDList(fd_set *readfds, int *maxfds, SocketInfo *server_message, 
	SocketInfo *server_data);
void AddFD(int fd, SocketInfo *s, int *fdlist);
void RemoveFD(int fd, SocketInfo *s, int *fdlist);

int GetMachineNum(const char *name);

int InitializeMasterSlaveNetworkMessaging(void);
void SetupModuleMessaging(void);

int StartNetworkMessaging(SocketInfo *server_message, 
	SocketInfo *client_message, SocketInfo *server_data, 
	SocketInfo *client_data);

int GetModuleID(char *modulename);
int GetSlaveNum(const char *name);
void ByteSwap(short *sptr, int nelem);
void ByteSwap(unsigned short *sptr, int nelem);
void ByteSwap(u32 *lptr, int nelem);

int GetServerSocket(const char *name);
int GetServerSocket(const char *name, int timeoutsec, int timeoutusec);
int GetClientSocket(const char *name);
/* the second version of GetClientSocket is used only by spike_matlab to get,
 * if available, a socket to send data to Matlab */
int GetClientSocket(const char *name, int timeoutsec, int timeoutusec);

int GetTCPIPServerSocket(unsigned short port);
int GetTCPIPClientSocket(const char *name, unsigned short port); 

int GetUDPClientSocket(const char *name, unsigned short port);
int GetUDPServerSocket(unsigned short port);

int SendMessage(int fd, int message, const char *data, int datalen);
int SendMessage(SocketInfo *c, int message, const char *data, int datalen);
int GetMessage(int fd, char *messagedata, int *datalen, int block);
int WaitForMessage(int fd, int message, float sec); 
int WaitForMessage(int fd, int message, float sec, char *data, int *datalen);

void ClearData(int fd);

int ReadConfigFile(char *configfilename, int datafile);
int WriteConfigFile(char *outfilename, int gzip, int datafile);

/* DSP commands */
int ProgramAllDSPS(void);
int ProgramLocalDSPS(void);
int SetDSPInfo(void);
int SetDSPAudioGain(int channel, short gain);
int SetDSPAudioCutoff(int channel, short cutoff);
int SetDSPAudioDelay(int channel, short cutoff);
int DSPMuteAudio(int mute);
int ResetDSP(short dspnum);
int StartLocalDSPAcquisition(void);
int StopLocalDSPAcquisition(void);
int StartDSPAcquisition(short dspnum);
int StopDSPAcquisition(short dspnum);
void StartLocalAcq(void);
void StopLocalAcq(void);
int SetAudio(ChannelInfo *ch, int commandorigin); 
int SetAudio(int num, ChannelInfo *ch, int commandorigin);
u32 ReadDSPTimestamp(short dspnum);
int UpdateChannel(ChannelInfo *ch, int local);
void GetDSPFilterCoeff(short lowfilt, short highfilt, short *data);
int ReprogramMasterDSP(char *filename);
int ReprogramAuxDSPs(char *filename);
int ReprogramLocalAuxDSPS(unsigned short *program);
int GetAllDSPCodeRev(void);

int SendChannelInfo(ChannelInfo *ch, int slaveorig);


int ReadNetworkConfigFile(char *configfilename);

void CloseSockets(SocketInfo *s);

void ErrorMessage(const char *errorstring, SocketInfo *client_message);
void StatusMessage(const char *errorstring, SocketInfo *client_message);

int SendPosBuffer(int fd, PosBuffer *posbuf);
int GetPosBuffer(int fd, PosBuffer *posbuf);
int SendPosMPEGBuffer(int fd, PosMPEGBuffer *posbuf);

void GetContBuffer(char *data, ContBuffer *c);

void SetupClock(void);
void ResetClock(void);


void UpdatePosInfo(unsigned char newthresh);
int AllowChanChange(int chanind);

u32 ParseTimestamp(char *s);

/* Neuralynx amplifier functions */ 
int CheckFilters(short low, short high);
int InitializeAmpControl(char *ampport);
int ProgramAmpChannel(int chan);
int ProgramAmpElectrode(int elect);


//short *junkdata;
