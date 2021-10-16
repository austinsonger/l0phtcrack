#include "lc7signerapplication.h"
#include "lc7signertask.h"
#include "qcommandline.h"
#include<qtimer.h>

LC7SignerApplication::LC7SignerApplication(int argc, char *argv[])
	: QCoreApplication(argc,argv)
{
	setOrganizationDomain("l0phtcrack.com");
	setOrganizationName("L0pht Holdings LLC");
	setApplicationName("LC7 Signer");
	setApplicationVersion("1.0");

	// Task parented to the application so that it
    // will be deleted by the application.
    m_task = new LC7SignerTask(this);

    // This will cause the application to exit when
    // the task signals finished.    
    QObject::connect(m_task, SIGNAL(finished()), this, SLOT(quit()));

    // This will run the task from the application event loop.
    QTimer::singleShot(0, m_task, SLOT(run()));
}


LC7SignerApplication::~LC7SignerApplication()
{
    delete m_task;
}
