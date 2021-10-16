#ifndef __INC_ATTACH_H
#define __INC_ATTACH_H

#include<QtCore>
#include<QtNetwork>

namespace Attach
{
	enum SERVERCOMMAND {
		SCMD_PAUSED = 0,
		SCMD_CANTPAUSE = 1
	};

	enum CLIENTCOMMAND {
		CCMD_PAUSE = 0
	};

	class CIOProcessor;
	class Client;
	class Server;

	bool isInteractiveSession();
	bool serverExists(QString servername);
};

#include "CIOProcessor.h"
#include "CAttachServer.h"
#include "CAttachClient.h"

#endif