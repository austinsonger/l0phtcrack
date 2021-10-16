#ifndef __INC_CATTACHSERVER_H
#define __INC_CATTACHSERVER_H


class LC7Main;

class Attach::Server : public QObject
{
	Q_OBJECT;
private:

	class Connection;

	QString m_servername;

	QLocalServer *m_server;
	QMap<QLocalSocket *, Connection *> m_serverconnections;
	ILC7GUILinkage *m_guilinkage;

	bool m_listening;

protected slots:
	void slot_newConnection();
	void slot_disconnectedConnection();


public:

	Server();
	virtual ~Server();

	void doPause();

	virtual bool listen(QString servername);
	virtual bool shutdown(void);
	virtual bool isListening();
	virtual void setGUILinkage(ILC7GUILinkage *linkage);
};

class Attach::Server::Connection : public QObject
{
	Q_OBJECT;
private:
	QLocalSocket *m_socket;
	CIOProcessor m_processor;
	Attach::Server *m_server;

public:
	Connection(QLocalSocket *conn, Attach::Server *server);
	~Connection();

	void disconnectFromServer(void);
	CIOProcessor *processor();

public slots:
	void slot_processPackets(QList<QByteArray> packets);
	
public:
	bool processCommand_pause(QDataStream &ds);

	bool sendCommand_paused(QString sessionFile);
	bool sendCommand_cantPause();
};





#endif