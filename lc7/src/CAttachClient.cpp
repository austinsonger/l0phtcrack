#include "stdafx.h"

////////////////////////////////////////////////////////
// Attach::Client
////////////////////////////////////////////////////////

Attach::Client::Client()
{
	m_socket=NULL;
	m_processor=NULL;
	m_done = false;
}

Attach::Client::~Client()
{TR;
	m_socket->disconnect();
	// disconnect slot will clean up
}

bool Attach::Client::connectToServer(QString servername, QString & resumefile)
{TR;
	m_socket = new QLocalSocket();
	m_processor = new CIOProcessor(m_socket);
	
	m_socket->connectToServer(servername, QIODevice::ReadWrite);
	if (!m_socket->waitForConnected())
	{
		delete m_processor;
		m_processor = NULL;
		delete m_socket;
		m_socket = NULL; 
		return false;
	}

	connect(m_socket, &QLocalSocket::disconnected, this, &Client::slot_disconnected);
	connect(m_processor, &CIOProcessor::sig_processPackets, this, &Client::slot_processPackets);

	m_done = false;

	sendCommand_pause();

	// Wait for disconnect
	while (!m_done)
	{
		QCoreApplication::processEvents();
		QThread::yieldCurrentThread();
	}

	if (m_resumefile.isNull())
	{
		return false;
	}

	resumefile = m_resumefile;


	return true;
}

void Attach::Client::slot_disconnected(void)
{TR;
	delete m_processor;
	m_processor = NULL;
	m_socket->deleteLater();
	m_socket = NULL;
	m_done = true;
}


void Attach::Client::slot_processPackets(QList<QByteArray> packets)
{TR;
	bool ok = true;
	foreach(QByteArray packet, packets)
	{
		QDataStream ds(&packet, QIODevice::ReadOnly);

		SERVERCOMMAND scmd;
		ds >> (quint32 &)scmd;
		if (ds.status() != QDataStream::Ok)
		{
			ok = false;
			break;
		}

		switch (scmd)
		{
		case SCMD_PAUSED:
			if (!processCommand_paused(ds))
			{
				ok = false;
			}
			break;
		case SCMD_CANTPAUSE:
			if (!processCommand_cantPause(ds))
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

bool Attach::Client::sendCommand_pause()
{TR;
	QByteArray data;
	QDataStream ds(&data, QIODevice::ReadWrite);

	ds << (quint32)CCMD_PAUSE;

	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	return m_processor->sendPacket(data);
}


bool Attach::Client::processCommand_paused(QDataStream &ds)
{TR;
	QString sessionFile;
	ds >> sessionFile;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	m_resumefile = sessionFile;

	m_done = true;

	return true;
}

bool Attach::Client::processCommand_cantPause(QDataStream &ds)
{TR;
	m_done = true;

	return true;
}

