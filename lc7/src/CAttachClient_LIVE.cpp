#include "stdafx.h"


////////////////////////////////////////////////////////
// Attach::Client::WindowImpl
////////////////////////////////////////////////////////


Attach::Client::WindowImpl::WindowImpl(QUuid id, QWidget *window)
{
	m_id = id;
	m_window = window;
	m_size = window->size();
	m_pixmap = new QPixmap(m_size);
}

Attach::Client::WindowImpl::~WindowImpl()
{
	delete m_pixmap;
}

void Attach::Client::WindowImpl::resize(QSize size)
{
	if (m_size == size)
	{
		return;
	}

	m_size = size;

	delete m_pixmap;
	m_pixmap = new QPixmap(m_size);

	// This happens before the state change, so
	// resize now if we're not maximized or fullscreened or minimized
	// otherwise resize at paint time, which is after the state change
	if (!m_window->isVisible() || (m_window->windowState() & (Qt::WindowMaximized | Qt::WindowMinimized | Qt::WindowFullScreen)) == 0)
	{
		if (m_window->size() != m_size)
		{
			m_window->resize(m_size);
		}
	}
}


QUuid Attach::Client::WindowImpl::id()
{
	return m_id;
}

QPixmap *Attach::Client::WindowImpl::pixmap()
{
	return m_pixmap;
}

QSize Attach::Client::WindowImpl::size()
{
	return m_size;
}


QWidget *Attach::Client::WindowImpl::window()
{
	return m_window;
}

////////////////////////////////////////////////////////
// Attach::Client
////////////////////////////////////////////////////////

Attach::Client::Client()
{
	m_socket=NULL;
	m_processor=NULL;
}

Attach::Client::~Client()
{ 
	m_socket->disconnect();
	// disconnect slot will clean up
}

bool Attach::Client::connectToServer(QString servername)
{
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

	return true;
}

Attach::Client::WindowImpl *Attach::Client::doNewWindowImpl(QUuid id, QWidget *widget)
{
	WindowImpl *impl = new WindowImpl(id, widget);

	m_windowimpl_by_id[id] = impl;
	m_windowimpl_by_widget[widget] = impl;

	widget->installEventFilter(this);

	return impl;
}

void Attach::Client::doDeleteWindowImpl(WindowImpl *impl)
{
	QWidget *widget = impl->window();

	widget->removeEventFilter(this);

	m_windowimpl_by_id.remove(impl->id());
	m_windowimpl_by_widget.remove(widget);

	delete impl;
}

void Attach::Client::doWindowUpdates(WindowImpl *impl, const WindowUpdates &ups)
{
	QWidget *widget = impl->window();

	bool do_show = false;

	if (ups.bits & WUB_STATE)
	{
		do_show = widget->isVisible();
		widget->setWindowState(ups.state);
	}
	if (ups.bits & WUB_FLAGS)
	{
		do_show = widget->isVisible();
		widget->setWindowFlags(ups.flags);
	}
	if (ups.bits & WUB_SIZE)
	{
		impl->resize(ups.size);
	}
	if (ups.bits & WUB_ICON)
	{
		widget->setWindowIcon(ups.icon);
	}
	if (ups.bits & WUB_ICONTEXT)
	{
		widget->setWindowIconText(ups.icontext);
	}
	if (ups.bits & WUB_MODALITY)
	{
		widget->setWindowModality(ups.modality);
	}
	if (ups.bits & WUB_OPACITY)
	{
		widget->setWindowOpacity(ups.opacity);
	}
	if (ups.bits & WUB_TITLE)
	{
		widget->setWindowTitle(ups.title);
	}
	if ((ups.bits & WUB_VISIBLE) || do_show)
	{
		if (ups.bits & WUB_VISIBLE)
		{
			widget->setVisible(ups.visible);
		}
		else {
			// do_show
			widget->setVisible(true);
		}
	}
}


void Attach::Client::slot_disconnected(void)
{
	// Close all windows
	foreach(WindowImpl *impl, m_windowimpl_by_id.values())
	{
		QWidget *widget = impl->window();
		doDeleteWindowImpl(impl);
		delete widget;
	}
	
	delete m_processor;
	m_processor = NULL;
	m_socket->deleteLater();
	m_socket = NULL;
}


void Attach::Client::slot_processPackets(QList<QByteArray> packets)
{
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
		case SCMD_CREATEWINDOW:
			if (!processCommand_createWindow(ds))
			{
				ok = false;
			}
			break;
		case SCMD_DESTROYWINDOW:
			if (!processCommand_destroyWindow(ds))
			{
				ok = false;
			}
			break;
		case SCMD_UPDATEWINDOW:
			if (!processCommand_updateWindow(ds))
			{
				ok = false;
			}
			break;
		case SCMD_PAINTWINDOW:
			if (!processCommand_paintWindow(ds))
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

bool Attach::Client::processCommand_createWindow(QDataStream &ds)
{
	QString className;
	QUuid winid;
	QUuid parentid;
	WindowUpdates ups;

	ds >> className;
	ds >> winid;
	ds >> parentid;
	ds >> ups;
		
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	QWidget *parentwindow = NULL;
	if (!parentid.isNull())
	{
		parentwindow = m_windowimpl_by_id[parentid]->window();
		if (parentwindow == NULL)
		{
			Q_ASSERT(0);
			return false;
		}
	}
	
	QWidget *widget=NULL;
	if (className == "QMainWindow")
	{
		widget = new QMainWindow(parentwindow, ups.flags);
	}
	else if (className == "QDialog")
	{
		widget = new QDialog(parentwindow, ups.flags);
	}
	else if (className == "QWidget")
	{
		widget = new QWidget(parentwindow, ups.flags);
	}
	else
	{
		Q_ASSERT(0);
		// IMPLEMENT ME
	}
	if (widget == NULL)
	{
		return false;
	}
		
	WindowImpl *impl=doNewWindowImpl(winid, widget);

	// Turn this off cuz we just create the window with the flags
	ups.bits &= ~WUB_FLAGS;

	doWindowUpdates(impl, ups);

	return true;
}

bool Attach::Client::processCommand_destroyWindow(QDataStream &ds)
{
	QUuid winid;
	
	ds >> winid;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	WindowImpl *impl = m_windowimpl_by_id[winid];
	if (impl == NULL)
	{
		Q_ASSERT(0);
		return false;
	}

	QWidget *widget = impl->window();	
	doDeleteWindowImpl(impl);
	delete widget;

	return true;
}

bool Attach::Client::processCommand_updateWindow(QDataStream &ds)
{
	QUuid winid;
	
	ds >> winid;

	WindowUpdates ups;
	
	ds >> ups;

	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	WindowImpl *impl = m_windowimpl_by_id[winid];
	if (impl == NULL)
	{
		Q_ASSERT(0);
		return false;
	}

	doWindowUpdates(impl, ups);

	return true;
}

bool Attach::Client::processCommand_paintWindow(QDataStream &ds)
{
	QUuid winid;
	quint32 rectcount;

	ds >> winid;
	ds >> rectcount;
	if (ds.status() != QDataStream::Ok)
	{
		return false;
	}

	WindowImpl *impl = m_windowimpl_by_id[winid];
	if (impl == NULL)
	{
		Q_ASSERT(0);
		return false;
	}

	QWidget *widget = impl->window();

	// Resize for things that resized during a state change (min/maximize/fullscreen)
	if (widget->size() != impl->size())
	{
		widget->resize(impl->size());
	}

	QPainter painter(impl->pixmap());
	QRegion dirtyregion;
	for (int i = 0; i < rectcount; i++)
	{
		QRect r;
		ds >> r;
		QPixmap pm;
		ds >> pm;

		painter.drawPixmap(r, pm);
		dirtyregion += r;
	}

	widget->update(dirtyregion);
	return true;
}



bool Attach::Client::eventFilter(QObject *obj, QEvent *event)
{
	switch (event->type())
	{

	case QEvent::Resize:
	{
		QResizeEvent *re = static_cast<QResizeEvent *>(event);
		QWidget *widget = static_cast<QWidget *>(obj);

		if (re->oldSize() != re->size())
		{
			WindowImpl *impl = m_windowimpl_by_widget[widget];
			if (impl == NULL)
			{
				Q_ASSERT(0);
				return false;
			}

			if (re->size() != impl->size())
			{
				impl->resize(re->size());
			}

			return true;
		}

		break;
	}

	case QEvent::Paint:
	{
		QPaintEvent *pe = static_cast<QPaintEvent *>(event);
		QWidget *widget = static_cast<QWidget *>(obj);

		WindowImpl *impl = m_windowimpl_by_widget[widget];
		if (impl == NULL)
		{
			Q_ASSERT(0);
			return false;
		}

		QPixmap *pixmap = impl->pixmap();
		QPainter painter(widget);
		painter.setClipRegion(pe->region()); 
		painter.drawPixmap(pixmap->rect(), *pixmap);
				
		return true;
	}

	}
	return QObject::eventFilter(obj, event);
}


