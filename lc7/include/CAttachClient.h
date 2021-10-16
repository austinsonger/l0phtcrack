#ifndef __INC_CATTACHCLIENT_H
#define __INC_CATTACHCLIENT_H


class QLocalServer;
class QLocalSocket;


class Attach::Client : public QObject
{
	Q_OBJECT;
	
private:

	class WindowImpl;
	
	QString m_servername;

	QLocalSocket *m_socket;
	CIOProcessor *m_processor;

	bool m_done;

	QString m_resumefile;

public slots:
	void slot_processPackets(QList<QByteArray> packets);
	void slot_disconnected(void);

protected:
	bool sendCommand_pause();

	bool processCommand_paused(QDataStream &ds);
	bool processCommand_cantPause(QDataStream &ds);

public:

	Client();
	virtual ~Client();

	virtual bool connectToServer(QString servername, QString & resumefile);
};


#endif