#ifndef __SQCOMPAT_H__
#define __SQCOMPAT_H__

#include <sys/time.h>
#include <qobject.h>
#include <iostream>
#include "spike_position.h"
#include "spike_defines.h"

using namespace std;

class SQCompat : public QObject {

	Q_OBJECT

	public:
		static SQCompat *instance() {
			if (_instance == NULL)
				_instance = new SQCompat();
			return _instance;
		}
		static void destroyInstance() {
			if (_instance != NULL)
				delete _instance;
		}

		void notifyContinuousDataReceived(short *bufferInfo, short *data) {
			emit continuousDataReceived(bufferInfo, data);
		}
		
	
		static SysInfo& sysInfo();
		
		public slots:	
		void spikeProcessMessages();
		void getTimeCheck();

	signals:
		void continuousDataReceived(short *bufferinfo, short *data);

	// should be private. I just changed to public to get rid of compiler warnings
	public:
		SQCompat() {}
		~SQCompat() {}
		static SQCompat * _instance;



};

#endif
