#ifndef __INC_CATTACHSERVER_H
#define __INC_CATTACHSERVER_H


class Attach::Server : public QObject
{
	Q_OBJECT;
private:

	class WindowImpl;
	class ConnectionWindowImpl;
	class Connection;

	QString m_servername;
	QSharedMemory m_sharedmemory;

	QLocalServer *m_server;
	QMap<QWidget *, WindowImpl *> m_serverwindows;
	QMap<QLocalSocket *, Connection *> m_serverconnections;

	bool m_painting;
	QTimer m_paint_timer;

	int m_paintstate;

	bool m_listening;

protected:

	virtual void addServerWindow(QWidget *window);
	virtual void removeServerWindow(QWidget *window);
	virtual void sendWindowUpdates(QWidget *window, const WindowUpdates &ups);
	
	virtual void startPainting(void);
	virtual void stopPainting(void);

	virtual bool eventFilter(QObject *obj, QEvent *event);

protected slots:
	void slot_newConnection();
	void slot_paintConnections();
	void slot_disconnectedConnection();

public:

	Server();
	virtual ~Server();

	virtual bool listen(QString servername);
	virtual bool shutdown(void);
	virtual bool isListening();
	virtual void addRootWindow(QWidget *rootwindow);
};

class Attach::Server::ConnectionWindowImpl
{
private:
	QRegion m_dirtyregion;
	WindowImpl *m_serverimpl;

public:
	ConnectionWindowImpl(WindowImpl *serverimpl);

	WindowImpl *serverimpl();
	QRegion & dirtyregion();
};

class Attach::Server::WindowImpl
{
private:
	WindowImpl *m_parent;
	QWidget *m_widget;
	QUuid m_id;
	QPixmap *m_pixmap;
	QRegion m_dirtyregion;

public:
	WindowImpl(WindowImpl *parent, QWidget *widget);
	~WindowImpl();

	void resize(QSize size);
		
	QUuid id();
	WindowImpl *parent();
	QWidget *widget();
	QRegion & dirtyregion();
	QPixmap *pixmap();
};

class Attach::Server::Connection
{
private:
	QLocalSocket *m_socket;
	CIOProcessor m_processor;
	QMap<WindowImpl *, ConnectionWindowImpl *> m_windows;

public:
	Connection(QLocalSocket *conn);
	~Connection();

	void disconnectFromServer(void);
	CIOProcessor *processor();

	bool addWindow(WindowImpl *window);
	bool removeWindow(WindowImpl *window);
	bool paintWindow(WindowImpl *window);
	bool updateWindow(WindowImpl *window, const WindowUpdates & ups);

protected:

	bool sendCommand_createWindow(ConnectionWindowImpl *window);
	bool sendCommand_destroyWindow(ConnectionWindowImpl *window);
	bool sendCommand_updateWindow(ConnectionWindowImpl *window, const WindowUpdates & ups);
	bool sendCommand_paintWindow(ConnectionWindowImpl *window);

};





#endif