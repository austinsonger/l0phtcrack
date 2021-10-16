#ifndef __INC_CIOPROCESSOR_H
#define __INC_CIOPROCESSOR_H


class Attach::CIOProcessor :public QObject
{
	Q_OBJECT;
public:

	CIOProcessor(QIODevice *device);
	virtual ~CIOProcessor();

	bool readyForSendPacket();
	bool sendPacket(QByteArray packet);
	
public slots:
	
	void slot_readyRead();
	void slot_aboutToClose();
	void slot_bytesWritten(qint64 bytes);

signals:

	void sig_processPackets(QList<QByteArray> packets);
	void sig_aboutToClose();

protected:

	bool processRead(const char * & dataptr, quint32 & dataleft);
	bool processWrite(const char * & dataptr, quint32 & dataleft);

private:

	QIODevice *m_device;

	// Write
	enum WRITESTATE {
		PUT_COUNT = 0,
		PUT_BUFFER = 1,
		WRITE_END = 2
	};
	WRITESTATE m_write_state;
	quint32 m_write_bytesleft;
	const char * m_write_bytesptr;
	quint32 m_write_packetsize;
	QByteArray m_write_buffer;
	QList<QByteArray> m_write_packets;

	// Read
	enum READSTATE {
		READ_START = 0,
		GET_COUNT = 1,
		GET_BUFFER = 2
	};
	
	READSTATE m_read_state;
	quint32 m_read_packetsize;
	QByteArray m_read_buffer;
	quint32 m_read_bytesleft;
	char * m_read_bytesptr;
	QList<QByteArray> m_read_packets;
	
};


#endif