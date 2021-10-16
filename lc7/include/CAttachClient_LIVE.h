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
	QSharedMemory m_sharedmemory;

	QLocalSocket *m_socket;
	CIOProcessor *m_processor;

	QMap<QUuid, WindowImpl *> m_windowimpl_by_id;
	QMap<QWidget *, WindowImpl *> m_windowimpl_by_widget;

public slots:
	void slot_processPackets(QList<QByteArray> packets);
	void slot_disconnected(void);

protected:
	
	bool processCommand_createWindow(QDataStream &ds);
	bool processCommand_destroyWindow(QDataStream &ds);
	bool processCommand_updateWindow(QDataStream &ds);
	bool processCommand_paintWindow(QDataStream &ds);

	virtual bool eventFilter(QObject *obj, QEvent *event);
	
	WindowImpl *doNewWindowImpl(QUuid id, QWidget *widget);
	void doDeleteWindowImpl(WindowImpl *impl);
	void doWindowUpdates(WindowImpl *impl, const WindowUpdates &ups);

public:

	Client();
	virtual ~Client();

	virtual bool connectToServer(QString servername);
};

class Attach::Client::WindowImpl :public QObject
{
	Q_OBJECT;

protected:
	
	QUuid m_id;
	QPixmap *m_pixmap;
	QWidget *m_window;
	QSize m_size;	// server size. client size is m_window->size()

public:

	WindowImpl(QUuid id, QWidget *window);
	virtual ~WindowImpl();

	void resize(QSize size);

	QUuid id();
	QPixmap *pixmap();
	QWidget *window();
	QSize size();
};

#endif