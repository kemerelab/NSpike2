
#include "../spikeFSGUI.h"

DigIOInfo digioinfo;
SysInfo sysinfo;

void  SendDAQFSMessage(int message, char *data, int datalen) {
  qDebug("Sending DAQ FS message %d",message);
  return;
}

int SendMessage(int a, int messageID, const char *c, int d) {
  qDebug("Sending message %d",messageID);
  return 0;
}

void StartDigIOProgram(int prognum) {
  qDebug("Starting program %d",prognum);
  return;
}
  
int main( int argc, char ** argv ) {
    sysinfo.machinenum = 0;
    sysinfo.nchannels[0] = 1;
    sysinfo.channelinfo[0][1].number = 1;
    sysinfo.channelinfo[0][1].electchan = 0;

    QApplication a( argc, argv );
    DIOInterface* mw = new DIOInterface();
    mw->setCaption( "Qt Example - Application" );
    mw->show();
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    return a.exec();
}
