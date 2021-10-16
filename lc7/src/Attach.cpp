#include"stdafx.h"

bool Attach::isInteractiveSession()
{TR;
#ifdef Q_OS_WIN32
	HMODULE kernel32 = GetModuleHandle("KERNEL32");
	typedef BOOL WINAPI TYPEOF_ProcessIdToSessionId(DWORD dwProcessId, DWORD *pSessionId);
	TYPEOF_ProcessIdToSessionId *pid2sid = (TYPEOF_ProcessIdToSessionId *)GetProcAddress(kernel32, "ProcessIdToSessionId");
	if (pid2sid == NULL)
	{
		return true;
	}

	DWORD session_id;
	if (!pid2sid(GetCurrentProcessId(), &session_id))
	{
		return true;
	}
	return session_id > 0;
#else
#error IMPLEMENT ME
#endif
}


bool Attach::serverExists(QString servername)
{TR;
	QLocalSocket *client = new QLocalSocket();
	client->connectToServer(servername);
	QLocalSocket::LocalSocketState state = client->state();
	if (state == QLocalSocket::ClosingState || state == QLocalSocket::UnconnectedState)
	{
		delete client;
		return false;
	}
	client->disconnectFromServer();
	delete client;

	return true;
}

