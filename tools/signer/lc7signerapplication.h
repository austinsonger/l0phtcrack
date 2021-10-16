#ifndef LC7SIGNERAPPLICATION_H
#define LC7SIGNERAPPLICATION_H

#include <QtCore>

class LC7SignerTask;

class LC7SignerApplication : public QCoreApplication
{
	Q_OBJECT

public:
	LC7SignerApplication(int argc, char *argv[]);
	~LC7SignerApplication();

private:
	LC7SignerTask *m_task;

};

#endif // LC7SIGNERAPPLICATION_H
