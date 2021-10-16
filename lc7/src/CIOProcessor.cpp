#include"stdafx.h"


Attach::CIOProcessor::CIOProcessor(QIODevice *device)
{TR;
	m_device = device;

	m_read_state = READ_START;
	m_read_packetsize = 0;
	m_read_bytesleft = 0;
	m_read_bytesptr = NULL;

	m_write_state = WRITE_END;
	m_write_bytesleft = 0;
	m_write_bytesptr = NULL;

	connect(device, &QIODevice::bytesWritten, this, &CIOProcessor::slot_bytesWritten);
	connect(device, &QIODevice::readyRead, this, &CIOProcessor::slot_readyRead);
	connect(device, &QIODevice::aboutToClose, this, &CIOProcessor::slot_aboutToClose);
}

Attach::CIOProcessor::~CIOProcessor()
{TR;
}

///////////////////////////////////////////////////////
// WRITE
///////////////////////////////////////////////////////

bool Attach::CIOProcessor::readyForSendPacket()
{TR;
	return m_write_state == WRITE_END;
}

void Attach::CIOProcessor::slot_bytesWritten(qint64 bytes)
{TR;
	switch (m_write_state)
	{
	case PUT_COUNT:
		m_write_bytesleft -= bytes;
		m_write_bytesptr += bytes;
		if (m_write_bytesleft==0)
		{
			m_write_state = PUT_BUFFER;
			m_write_bytesleft = m_write_buffer.size();
			m_write_bytesptr = m_write_buffer.data();
		}
		m_device->write(m_write_bytesptr, m_write_bytesleft);
		break;

	case PUT_BUFFER:
		m_write_bytesleft -= bytes;
		m_write_bytesptr += bytes;
		if (m_write_bytesleft == 0)
		{
			if (m_write_packets.count() > 0)
			{
				m_write_buffer = m_write_packets.takeFirst();
				m_write_packetsize = m_write_buffer.size();
				m_write_bytesleft = sizeof(m_write_packetsize);
				m_write_bytesptr = (const char *)&m_write_packetsize;
				m_write_state = PUT_COUNT;
			}
			else
			{
				m_write_state = WRITE_END;
				m_write_bytesptr = NULL;
				return;
			}
		}
		m_device->write(m_write_bytesptr, m_write_bytesleft);
		break;
	}
}

bool Attach::CIOProcessor::sendPacket(QByteArray packet)
{TR;
	if (!readyForSendPacket())
	{
		m_write_packets.append(packet);
		return true;
	}

	m_write_buffer = packet;
	m_write_packetsize = m_write_buffer.size();
	m_write_bytesleft = sizeof(m_write_packetsize);
	m_write_bytesptr = (const char *)&m_write_packetsize;
	m_write_state = PUT_COUNT;

	if (m_device->write((const char *)&m_write_packetsize, sizeof(m_write_packetsize)) == -1)
	{
		return false;
	}

	return true;
}


///////////////////////////////////////////////////////
// READ
///////////////////////////////////////////////////////


bool Attach::CIOProcessor::processRead(const char * & dataptr, quint32 & dataleft)
{TR;
	quint32 toread;

	toread = dataleft;
	if (toread > m_read_bytesleft)
	{
		toread = m_read_bytesleft;
	}
	memcpy(m_read_bytesptr, dataptr, toread);
	m_read_bytesptr += toread;
	dataptr += toread;
	dataleft -= toread;
	m_read_bytesleft -= toread;

	if (m_read_bytesleft == 0)
	{
		return true;
	}

	return false;
}

void Attach::CIOProcessor::slot_readyRead()
{TR;
	QByteArray data = m_device->readAll();
	quint32 dataleft = data.size();
	const char *dataptr = data.data();
	

	while(dataleft > 0)
	{
		switch (m_read_state)
		{
		case READ_START:
			m_read_state = GET_COUNT;

			m_read_bytesleft = 4;
			m_read_bytesptr = (char *)&m_read_packetsize;

			break;
		case GET_COUNT:

			if (processRead(dataptr, dataleft))
			{
				m_read_state = GET_BUFFER;
				m_read_bytesleft = m_read_packetsize;
				m_read_buffer.resize(m_read_packetsize);
				m_read_bytesptr = m_read_buffer.data();
			}

			break;
		case GET_BUFFER:
			if (processRead(dataptr, dataleft))
			{
				m_read_state = GET_COUNT;
				m_read_bytesleft = 4;
				m_read_bytesptr = (char *)&m_read_packetsize;

				m_read_packets.append(m_read_buffer);
			}
			break;
		}
	}

	if (m_read_packets.size() > 0)
	{
		emit sig_processPackets(m_read_packets);
		m_read_packets.clear();
	}
}

void Attach::CIOProcessor::slot_aboutToClose()
{TR;
	emit sig_aboutToClose();
}
