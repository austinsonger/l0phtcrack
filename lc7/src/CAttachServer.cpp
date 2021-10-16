#include <stdafx.h>
#include "CAttachServer.h"
////////////////////////////////////////////////////////
// Attach::Server::Connection
////////////////////////////////////////////////////////

Attach::Server::Connection::Connection(QLocalSocket *conn, Attach::Server *server) :m_processor(conn)
{TR;
	m_server = server;
	m_socket = conn;

	connect(&m_processor, &CIOProcessor::sig_processPackets, this, &Connection::slot_processPackets);
}

Attach::Server::Connection::~Connection()
{TR;
}

void Attach::Server::Connection::disconnectFromServer(void)
{TR;
	m_socket->disconnectFromServer();
}

Attach::CIOProcessor *Attach::Server::Connection::processor()
{TR;
	return &m_processor; 
}


void Attach::Server::Connection::slot_processPackets(QList<QByteArray> packets)
{TR;
	bool ok = true;
	foreach(QByteArray packet, packets)
	{
		QDataStream ds(&packet, QIODevice::ReadOnly);

		CLIENTCOMMAND ccmd;
		ds >> (quint32 &)ccmd;
		if (ds.status() != QDataStream::Ok)
		{
			ok = false;
			break;
		}

		switch (ccmd)
		{
		case CCMD_PAUSE:
			if (!processCommand_pause(ds))
			{
				ok = false;
			}
			break;
		}
		if (!ok)
		{
			break;
		}
	}

	if (!ok)
	{
		Q_ASSERT(0);
		m_socket->disconnect();
	}
}

bool Attach::Server::Connection::processCommand_pause(QDataStream &ds)
{TR;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	m_server->doPause();
	
	return true;
}




bool Attach::Server::Connection::sendCommand_paused(QString sessionFile)
{TR;
	QByteArray data;
	QDataStream ds(&data, QIODevice::ReadWrite);

	ds << (quint32)SCMD_PAUSED;
	ds << sessionFile;

	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	return m_processor.sendPacket(data);
}

bool Attach::Server::Connection::sendCommand_cantPause()
{TR;
	QByteArray data;
	QDataStream ds(&data, QIODevice::ReadWrite);

	ds << (quint32)SCMD_CANTPAUSE;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	return m_processor.sendPacket(data);
}




////////////////////////////////////////////////////////
// Attach::Server
////////////////////////////////////////////////////////


Attach::Server::Server()
{
	m_servername = "";
	m_server = NULL;
	m_guilinkage = NULL;

	m_listening = false;
}

Attach::Server::~Server()
{TR;
	shutdown();
}

void Attach::Server::setGUILinkage(ILC7GUILinkage *guilinkage)
{TR;
	m_guilinkage = guilinkage;
}

bool Attach::Server::listen(QString servername)
{TR;
	m_server = new QLocalServer(this);
	m_server->setSocketOptions(QLocalServer::UserAccessOption);
	if (!m_server->listen(servername))
	{
		return false;
	}
	connect(m_server, &QLocalServer::newConnection, this, &Server::slot_newConnection);

	m_servername = servername;
	m_listening = true;

	return true;
}

bool Attach::Server::shutdown(void)
{TR;
	if (!m_listening)
	{
		return false;
	}

	return true;
}

bool Attach::Server::isListening(void)
{TR;
	return m_listening;
}

void Attach::Server::slot_newConnection()
{TR;
	QLocalSocket *conn;

	while ((conn = m_server->nextPendingConnection()) != NULL)
	{
		Connection *serverconn = new Connection(conn, this);
		m_serverconnections[conn] = serverconn;

		connect(conn, &QLocalSocket::disconnected, this, &Server::slot_disconnectedConnection);
	}
}	




void Attach::Server::slot_disconnectedConnection()
{TR;
	QLocalSocket *conn = (QLocalSocket *)sender();
	Connection *serverconn = m_serverconnections[conn];

	delete serverconn;
	conn->deleteLater();

	m_serverconnections.remove(conn);
}

void Attach::Server::doPause()
{TR;
	QString sessionFile = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).absoluteFilePath("lc7attachsession.lc7");

	if (m_guilinkage->PauseBackgroundSession(sessionFile))
	{
		foreach(Connection *serverconn, m_serverconnections)
		{
			serverconn->sendCommand_paused(sessionFile);
		}
	}
	else
	{
		foreach(Connection *serverconn, m_serverconnections)
		{
			serverconn->sendCommand_cantPause();
		}
	}

	m_guilinkage->Exit(true);
}