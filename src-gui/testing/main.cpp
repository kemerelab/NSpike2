
#include "../spikeUserGUI.h"

int main( int argc, char ** argv ) {
    QApplication a( argc, argv );
    laserControl* mw = new laserControl();
    mw->setCaption( "Qt Example - Application" );
    mw->show();
    a.connect( &a, SIGNAL(lastWindowClosed()), &a, SLOT(quit()) );
    return a.exec();
}
