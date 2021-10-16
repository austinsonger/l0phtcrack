#include <stdafx.h>
#include "CAttachServer.h"

////////////////////////////////////////////////////////
// Attach::Server::WindowImpl
////////////////////////////////////////////////////////

Attach::Server::WindowImpl::WindowImpl(WindowImpl *parent, QWidget *widget)
{
	m_id = QUuid::createUuid();
	m_parent = parent;
	m_widget = widget;
	m_pixmap = new QPixmap(widget->size());
	m_dirtyregion = QRegion(m_pixmap->rect());
}

Attach::Server::WindowImpl::~WindowImpl()
{
	delete m_pixmap;
}

void Attach::Server::WindowImpl::resize(QSize size)
{
	if (size == m_pixmap->size())
	{
		return;
	}

	delete m_pixmap;
	m_pixmap = new QPixmap(size);
}



QUuid Attach::Server::WindowImpl::id()
{
	return m_id;
}

Attach::Server::WindowImpl *Attach::Server::WindowImpl::parent()
{
	return m_parent;
}

QWidget *Attach::Server::WindowImpl::widget()
{
	return m_widget;
}

QPixmap *Attach::Server::WindowImpl::pixmap()
{
	return m_pixmap;
}

QRegion & Attach::Server::WindowImpl::dirtyregion()
{
	return m_dirtyregion;
}


////////////////////////////////////////////////////////
// Attach::Server::ConnectionWindowImpl
////////////////////////////////////////////////////////

Attach::Server::ConnectionWindowImpl::ConnectionWindowImpl(WindowImpl *serverimpl)
{
	m_serverimpl = serverimpl;
	m_dirtyregion = QRegion(serverimpl->pixmap()->rect());
}

Attach::Server::WindowImpl *Attach::Server::ConnectionWindowImpl::serverimpl()
{
	return m_serverimpl;
}

QRegion & Attach::Server::ConnectionWindowImpl::dirtyregion()
{
	return m_dirtyregion;
}

////////////////////////////////////////////////////////
// Attach::Server::Connection
////////////////////////////////////////////////////////

Attach::Server::Connection::Connection(QLocalSocket *conn) :m_processor(conn)
{
	m_socket = conn;
}

Attach::Server::Connection::~Connection()
{
	foreach(ConnectionWindowImpl *asw, m_windows.values())
	{
		delete asw;
	}
}

void Attach::Server::Connection::disconnectFromServer(void)
{
	m_socket->disconnectFromServer();
}

Attach::CIOProcessor *Attach::Server::Connection::processor()
{
	return &m_processor; 
}


bool Attach::Server::Connection::addWindow(WindowImpl *window)
{
	if (m_windows.contains(window))
	{
		return false;
	}
	
	m_windows[window] = new Attach::Server::ConnectionWindowImpl(window);
	
	// Issue command to add window to clients
	return sendCommand_createWindow(m_windows[window]);
}

bool Attach::Server::Connection::removeWindow(WindowImpl *window)
{
	if (!m_windows.contains(window))
	{
		return false;
	}

	// Issue command to remove window from clients
	bool ret = sendCommand_destroyWindow(m_windows[window]);

	delete m_windows[window];
	m_windows.remove(window);

	return ret;
}


bool Attach::Server::Connection::paintWindow(WindowImpl *window)
{
	ConnectionWindowImpl *connwindow = m_windows[window];
	if (!connwindow)
	{
		return false;
	}

	connwindow->dirtyregion() += window->dirtyregion();

	if (connwindow->dirtyregion().rectCount() > 0)
	{
		return sendCommand_paintWindow(connwindow);
	}

	// This says we painted it even if we didn't
	// it's debatable if this is the right thing to do.
	return true;
}

bool Attach::Server::Connection::updateWindow(WindowImpl *window, const WindowUpdates & ups)
{
	ConnectionWindowImpl *connwindow = m_windows[window];
	if (!connwindow)
	{
		return false;
	}

	return sendCommand_updateWindow(connwindow, ups);
}

bool Attach::Server::Connection::sendCommand_createWindow(ConnectionWindowImpl *connwindow)
{
	WindowImpl *window = connwindow->serverimpl();

	QByteArray data;
	QDataStream ds(&data, QIODevice::ReadWrite);

	WindowImpl *parentwindow = NULL;
	if (window->parent())
	{
		parentwindow = window->parent();
	}

	window->widget()->dumpObjectTree();

	ds << (quint32)SCMD_CREATEWINDOW;
	if (window->widget()->inherits("QMainWindow"))
	{
		ds << QString("QMainWindow");
	}
	else if (window->widget()->inherits("QDialog"))
	{
		ds << QString("QDialog");
	}
	else
	{
		ds << QString("QWidget");
	}
	ds << window->id();
	ds << (parentwindow ? parentwindow->id() : QUuid());

	WindowUpdates ups = WindowUpdates::fromWidget(window->widget(),WUB_ALL);
	ds << ups;

	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	return m_processor.sendPacket(data);
}

bool Attach::Server::Connection::sendCommand_destroyWindow(ConnectionWindowImpl  *connwindow)
{
	QByteArray data;
	QDataStream ds(&data, QIODevice::ReadWrite);

	ds << (quint32)SCMD_DESTROYWINDOW;
	ds << connwindow->serverimpl()->id();
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	return m_processor.sendPacket(data);
}


bool Attach::Server::Connection::sendCommand_updateWindow(ConnectionWindowImpl  *connwindow, const WindowUpdates & ups)
{
	QByteArray data;
	QDataStream ds(&data, QIODevice::ReadWrite);

	ds << (quint32)SCMD_UPDATEWINDOW;
	ds << connwindow->serverimpl()->id();
	ds << ups;

	return m_processor.sendPacket(data);
}

bool Attach::Server::Connection::sendCommand_paintWindow(ConnectionWindowImpl *connwindow)
{
	if (!m_processor.readyForSendPacket())
	{
		// It's okay to drop window paints
		return false;
	}

	WindowImpl *window = connwindow->serverimpl();
	
	QByteArray data;
	QDataStream ds(&data, QIODevice::ReadWrite);

	ds << (quint32)SCMD_PAINTWINDOW;
	ds << connwindow->serverimpl()->id();
	ds << (quint32)connwindow->dirtyregion().rectCount();
	foreach(QRect r, connwindow->dirtyregion().rects())
	{
		ds << r;
		QPixmap pm;
		

		//pm = window->pixmap()->copy(r);

		//if (window->widget()->testAttribute(Qt::WA_NativeWindow))
		//{
			pm = QPixmap::grabWindow(window->widget()->winId(),r.left(), r.top(), r.width(), r.height());
		//}
		//else
		//{
//			pm = window->widget()->grab(r);
		//}

		

		ds << pm;
	}
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	if (!m_processor.sendPacket(data))
	{
		return false;
	}

	connwindow->dirtyregion() = QRegion();
	return true;
}



////////////////////////////////////////////////////////
// Attach::Server
////////////////////////////////////////////////////////


Attach::Server::Server()
{
	m_servername = "";
	m_server = NULL;
	
	m_painting = false;

	m_paint_timer.setTimerType(Qt::PreciseTimer);
	m_paint_timer.setInterval(16);

	m_listening = false;
	m_paintstate = 0;

	connect(&m_paint_timer, &QTimer::timeout, this, &Server::slot_paintConnections);
}

Attach::Server::~Server()
{
	shutdown();
}


bool Attach::Server::listen(QString servername)
{
	m_sharedmemory.setKey(servername);
	if (!m_sharedmemory.create(1024 * 1024))
	{
		return false;
	}

	m_server = new QLocalServer(this);
	m_server->setSocketOptions(QLocalServer::UserAccessOption);
	if (!m_server->listen(servername))
	{
		m_sharedmemory.detach();
		return false;
	}
	connect(m_server, &QLocalServer::newConnection, this, &Server::slot_newConnection);

	m_servername = servername;
	m_listening = true;
	m_paintstate = 0;

	return true;
}

bool Attach::Server::shutdown(void)
{
	if (!m_listening)
	{
		return false;
	}
	// This will have slot clean up
	foreach(QLocalSocket *conn, m_serverconnections.keys())
	{
		conn->disconnectFromServer();
	}
	m_serverconnections.clear();
	m_server->close();
	delete m_server;
	m_server = NULL;
	m_sharedmemory.detach();
	m_listening = false;
	m_servername = "";
	m_paintstate = 0;

	return true;
}

bool Attach::Server::isListening(void)
{
	return m_listening;
}

void Attach::Server::addRootWindow(QWidget *rootwindow)
{
	addServerWindow(rootwindow);
}


void Attach::Server::addServerWindow(QWidget *window)
{
	// Add master window
	WindowImpl *parentwindowimpl=NULL;
	QWidget *parentwidget = window->parentWidget();
	if (parentwidget)
	{
		QWidget *parentwindow = parentwidget->window();
		if (m_serverwindows.contains(parentwindow))
		{
			parentwindowimpl = m_serverwindows[parentwindow];
		}
	}

	WindowImpl *serverwindow = new WindowImpl(parentwindowimpl, window);
	m_serverwindows[window] = serverwindow;

	// Add windows to each connection
	QList<Connection *> deadconns;
	foreach(Connection *serverconn, m_serverconnections)
	{
		if (!serverconn->addWindow(serverwindow))
		{
			deadconns.append(serverconn);
		}
	}
	
	foreach(Connection *serverconn, deadconns)
	{
		serverconn->disconnectFromServer();
	}

	window->installEventFilter(this);
}

void Attach::Server::removeServerWindow(QWidget *window)
{
	WindowImpl *serverwindow = m_serverwindows[window];

	// Remove windows from each connection
	QList<Connection *> deadconns;
	foreach(Connection *serverconn, m_serverconnections)
	{
		if(!serverconn->removeWindow(serverwindow))
		{
			deadconns.append(serverconn);
		}
	}

	foreach(Connection *serverconn, deadconns)
	{
		serverconn->disconnectFromServer();
	}

	// Remove master window
	delete m_serverwindows[window];
	m_serverwindows.remove(window);

	window->removeEventFilter(this);
}


void Attach::Server::slot_newConnection()
{
	QLocalSocket *conn;

	QList<Connection *> deadconns;
	while ((conn = m_server->nextPendingConnection()) != NULL)
	{
		Connection *serverconn = new Connection(conn);
		m_serverconnections[conn] = serverconn;

		connect(conn, &QLocalSocket::disconnected, this, &Server::slot_disconnectedConnection);

		// Add master windows to new connection
		foreach(WindowImpl *serverwindow, m_serverwindows)
		{
			if (!serverconn->addWindow(serverwindow))
			{
				deadconns.append(serverconn);
			}
		}

	}

	if (m_serverconnections.size() > 0)
	{
		startPainting();
	}

	foreach(Connection *deadconn, deadconns)
	{
		deadconn->disconnectFromServer();
	}
}	


void Attach::Server::slot_disconnectedConnection()
{
	QLocalSocket *conn = (QLocalSocket *)sender();
	Connection *serverconn = m_serverconnections[conn];

	delete serverconn;
	conn->deleteLater();

	m_serverconnections.remove(conn);

	if (m_serverconnections.count() == 0)
	{
		stopPainting();
	}
}

void Attach::Server::startPainting(void)
{
	if (m_painting)
	{
		return;
	}

	m_paint_timer.start();
	
	m_painting = true;
}

void Attach::Server::sendWindowUpdates(QWidget *window, const WindowUpdates &ups)
{
	WindowImpl *serverwindow = m_serverwindows[window];

	foreach(Connection *conn, m_serverconnections.values())
	{
		conn->updateWindow(serverwindow, ups);
	}
}


void Attach::Server::slot_paintConnections()
{
	foreach(Connection *conn, m_serverconnections.values())
	{
		foreach(WindowImpl *serverwindow, m_serverwindows)
		{
			conn->paintWindow(serverwindow);
		}
	}

	// Now that we've painted all connections, the master regions are no longer dirty
	foreach(WindowImpl *serverwindow, m_serverwindows)
	{
		serverwindow->dirtyregion() = QRegion();
	}
}

void Attach::Server::stopPainting(void)
{
	if (!m_painting)
	{
		return;
	}

	m_paint_timer.stop();

	m_painting = false;
}


bool Attach::Server::eventFilter(QObject *obj, QEvent *event)
{
	switch (event->type())
	{
	case QEvent::ChildAdded:
	{
		QChildEvent* ce = static_cast<QChildEvent*>(event);
		// Install the filter to each new child object created
		ce->child()->installEventFilter(this);
		break;
	}
	case QEvent::ChildRemoved:
	{
		QChildEvent* ce = static_cast<QChildEvent*>(event);
		// Remove the the filter from each new child object removed
		ce->child()->removeEventFilter(this);
		break;
	}
			
	case QEvent::Create:
	{
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();
		
		if (widget == window)
		{
			addServerWindow(window);
		}

		break;
	}
	case QEvent::Destroy:
	{
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();

		if (widget == window)
		{
			removeServerWindow(window);
		}

		break;
	}
	
	case QEvent::Paint:
	{
		QPaintEvent *pe = static_cast<QPaintEvent *>(event);
		QWidget *widget = static_cast<QWidget *>(obj);

		QWidget *window = widget->window();

		WindowImpl *serverwindow = m_serverwindows[window];
		QRect serverwindowrect = pe->rect().translated(widget->mapTo(window, QPoint(0, 0)));
		QRegion reg = pe->region().translated(widget->mapTo(window, QPoint(0, 0)));

		// Update server dirty region
		serverwindow->dirtyregion() += reg;

		break;
		/*
		if (m_paintstate == 0)
		{
			m_paintstate++;

			// Reissue paint event
			qApp->notify(obj, event);

			// Paintstate is now 2
			WindowImpl *serverwindow = m_serverwindows[window];
			QRect serverwindowrect = pe->rect().translated(widget->mapTo(window, QPoint(0, 0)));

			QRegion reg = pe->region().translated(widget->mapTo(window, QPoint(0, 0)));

			// This causes a second paint event
			//QPixmap pm = widget->grab(pe->rect());
			QPainter painter(serverwindow->pixmap());
			widget->render(&painter, serverwindowrect.topLeft(), pe->region(), (QWidget::RenderFlags)0);

			// Paintstate is now 3

			// Draw on the server pixmap
			//painter.drawPixmap(serverwindowrect, pm);

			// Update server dirty region
			serverwindow->dirtyregion() += reg;

			// Reset paintstate to zero
			m_paintstate = 0;
		}
		else if (m_paintstate == 1)
		{
			m_paintstate++;

			// Allow event to paint itself normally
			return false;
		}
		else if (m_paintstate == 2)
		{
			m_paintstate++;

			// Allow event to paint the grab normally
			return false;
		}

		// And we're done, so don't pass the paint event on again
		return true;
		*/
	}
	
	case QEvent::Show:
	{
		QShowEvent *se = static_cast<QShowEvent *>(event);
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();
		if (widget == window)
		{
			WindowUpdates ups;
			ups.bits = WUB_VISIBLE;
			ups.visible = true;

			sendWindowUpdates(window, ups);
		}
		break;
	}
	
	case QEvent::Hide:
	{
		QShowEvent *se = static_cast<QShowEvent *>(event);
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();
		if (widget == window)
		{
			WindowUpdates ups;
			ups.bits = WUB_VISIBLE;
			ups.visible = false;

			sendWindowUpdates(window, ups);
		}
		break;
	}

	case QEvent::Resize:
	{
		QResizeEvent *re = static_cast<QResizeEvent *>(event);
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();

		if (widget == window)
		{
			WindowImpl *serverwindow = m_serverwindows[window];
			serverwindow->resize(re->size());

			WindowUpdates ups;
			ups.bits = WUB_SIZE;
			ups.size = re->size();
			
			sendWindowUpdates(window, ups);
		}
		break;
	}

	case QEvent::WindowTitleChange:
	{
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();

		if (widget == window)
		{
			WindowUpdates ups;
			ups.bits = WUB_TITLE;
			ups.title = window->windowTitle();

			sendWindowUpdates(window, ups);
		}
		break;
	}

	case QEvent::WindowIconChange:
	{
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();

		if (widget == window)
		{
			WindowUpdates ups;
			ups.bits = WUB_ICON;
			ups.icon = window->windowIcon();

			sendWindowUpdates(window, ups);
		}
		break;
	}

	case QEvent::IconTextChange:
	{
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();

		if (widget == window)
		{
			WindowUpdates ups;
			ups.bits = WUB_ICONTEXT;
			ups.icontext = window->windowIconText();

			sendWindowUpdates(window, ups);
		}
		break;
	}

	case QEvent::WindowStateChange:
	{
		QWidget *widget = static_cast<QWidget *>(obj);
		QWidget *window = widget->window();

		if (widget == window)
		{
			WindowUpdates ups;
			ups.bits = WUB_STATE;
			ups.state = window->windowState();
			
			sendWindowUpdates(window, ups);
		}
		break;
	}
	

	}
	return QObject::eventFilter(obj, event);
}

