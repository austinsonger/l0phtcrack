#include"stdafx.h"
#include<stdio.h>
#include<QtZlib/zlib.h>

#define UNCOMPRESSED 0

#if UNCOMPRESSED
#define LOG_FILE_NAME "lc7_log.txt"
#define gzFile FILE
#define gzopen fopen
#define gzprintf fprintf
#define gzclose fclose
#define gzflush(x,y) fflush(x)
#else
#define LOG_FILE_NAME "lc7_log.txt.gz"
#endif

gzFile g_debuglog = NULL;
int g_indent=0;

QMutex g_debuglock;


void null_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
}

void debug_message_handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	QMutexLocker lock(&g_debuglock);
		
	if (!g_debuglog)
	{
		QByteArray debugpath(QDir::temp().filePath(LOG_FILE_NAME).toLatin1());
		if ((g_debuglog = gzopen(debugpath, "wt")) == NULL)
		{
			QMessageBox msgBox;
			msgBox.setText(QString("Opening the log file '%1' has failed.\nLogging will be disabled for this session.").arg(QString::fromLatin1(debugpath)));
			msgBox.exec();			

			qInstallMessageHandler(0);

			return;
		}
	}

    QByteArray localMsg = msg.toLocal8Bit();

	QTime curtime=QTime::currentTime();
	QByteArray t=curtime.toString().toUtf8();
	
	QString indentstr(g_indent,' ');
	QByteArray indent=indentstr.toUtf8();

	switch (type) {
    case QtDebugMsg:
		gzprintf(g_debuglog, "%s %sDebug: %s (%s:%u, %s)\n", t.constData(), indent.constData(), localMsg.constData(), context.file, context.line, context.function);
		gzflush(g_debuglog, Z_FULL_FLUSH);
        break;
    case QtWarningMsg:
		gzprintf(g_debuglog, "%s %sWarning: %s (%s:%u, %s)\n", t.constData(), indent.constData(), localMsg.constData(), context.file, context.line, context.function);
		gzflush(g_debuglog, Z_FULL_FLUSH);
		break;
    case QtCriticalMsg:
		gzprintf(g_debuglog, "%s %sCritical: %s (%s:%u, %s)\n", t.constData(), indent.constData(), localMsg.constData(), context.file, context.line, context.function);
		gzflush(g_debuglog, Z_FULL_FLUSH);
		break;
    case QtFatalMsg:
		gzprintf(g_debuglog, "%s %sFatal: %s (%s:%u, %s)\n", t.constData(), indent.constData(), localMsg.constData(), context.file, context.line, context.function);
		gzflush(g_debuglog, Z_FULL_FLUSH);
		abort();
    }

	if(msg.startsWith("[enter]"))
	{
		g_indent+=2;
	}
	else if(msg.startsWith("[exit]"))
	{
		g_indent-=2;
	}
}

void init_debug()
{
	qInstallMessageHandler(null_message_handler);
}

void enable_debug()
{
	if(!g_debuglog)
	{
		qInstallMessageHandler(debug_message_handler);
		qDebug("--- Debug Log Opened ---");
	}
}

void disable_debug()
{
	if (g_debuglog)
	{
		qDebug("--- Debug Log Closed ---");

		qInstallMessageHandler(null_message_handler);

		gzclose(g_debuglog);
		g_debuglog = NULL;
	}
}

