#include<stdafx.h>

/////////////////////////////////

CLC7JTREXE::CLC7JTREXE(QString jtrdllversion) : m_cmdid(0)
{
	TR;

	m_jtrdllversion = jtrdllversion;

	QDir plugins(g_pLinkage->GetPluginsDirectory());
	plugins.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");

	connect(&m_jtrprocess, &QProcess::readyRead, this, &CLC7JTREXE::slot_readyRead);
	connect(this, &CLC7JTREXE::sig_RequestCommand, this, &CLC7JTREXE::slot_RequestCommand, Qt::BlockingQueuedConnection);
// Don't bother with this because if we 'kill' jtrexe it will trigger this as 'crashed'
//	connect(&m_jtrprocess, static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error), [=](QProcess::ProcessError error){ 
//		QMessageBox::critical(NULL, "Process error", QString("Error code: %1").arg(error));
//	});

	m_jtrexepath = plugins.absoluteFilePath("jtrexe.exe");
	m_jtrprocess.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
	m_jtrprocess.start(QString("\"%1\" %2 --processhost").arg(m_jtrexepath)
		.arg(m_jtrdllversion.isEmpty() ? "" : QString("--force-%1").arg(m_jtrdllversion.toLower()))
		);

	QProcess::ProcessError err = m_jtrprocess.error();
	m_jtrprocess.waitForStarted();
#ifdef _WIN32
	Q_PID pid = m_jtrprocess.pid();
//	g_jom.AddProcessToJob(pid->hProcess);
//	SetPriorityClass(pid->hProcess, ABOVE_NORMAL_PRIORITY_CLASS);
#endif
}

CLC7JTREXE::~CLC7JTREXE()
{
	//m_jtrprocess.write("exit\n");
	//m_jtrprocess.waitForBytesWritten();
	//if (!m_jtrprocess.waitForFinished(5000))
	//{
		m_jtrprocess.kill();
	//}
}


bool CLC7JTREXE::IsValid()
{
	return m_jtrprocess.state() == QProcess::ProcessState::Running;
}





/////////////////////////////////////////////////////////////////////////////////

int CLC7JTREXE::main(int argc, char **argv, struct JTRDLL_HOOKS *hooks)
{
	// Call jtrdll_main
	QString cmdstr="jtrdll_main\n";
	cmdstr += QString("%1\n").arg(hooks->appdatadir);
	cmdstr += QString("%1\n").arg(argc);
	for (int arg = 0; arg < argc; arg++)
	{
		cmdstr += (QString("%1\n").arg(argv[arg]).toUtf8());
	}
	
	// Get control pipe
	QByteArray ctlpipestr = RequestCommand(cmdstr);
	if (!ctlpipestr.startsWith("pipe="))
	{
		Q_ASSERT(0);
		return -1;
	}
	QString ctlpipename = QString::fromUtf8(ctlpipestr.mid(5));

	PIPETYPE pipe = OpenControlPipe(ctlpipename);
	if (!pipe)
	{
		Q_ASSERT(0);
		return -1;
	}

	int ret = 0;

	QString cmd;
	QByteArray data;
	while (waitForCommand(pipe, cmd, data))
	{
		if (cmd == "sigill")
		{
			hooks->caught_sigill = true;
			ret = *(int *)(data.data());
		}
		else if (cmd == "return")
		{
			ret = *(int *)(data.data());
		}
		else if (cmd == "stdout")
		{
			hooks->stdout_hook(hooks->ctx, data);
		}
		else if (cmd == "stderr")
		{
			hooks->stderr_hook(hooks->ctx, data);
		}
	}

	CloseControlPipe(pipe);

	return ret;
}

void CLC7JTREXE::terminate()
{
	m_jtrprocess.terminate();
}


void CLC7JTREXE::abort(bool timeout)
{
	// Call jtrdll_abort (no control pipe for this one)
	QString cmdstr = "jtrdll_abort\n";
	cmdstr += QString("%1\n").arg(timeout?1:0);
	RequestCommand(cmdstr);
}


void CLC7JTREXE::get_status(struct JTRDLL_STATUS *jtrdllstatus)
{
	memset(jtrdllstatus, 0, sizeof(struct JTRDLL_STATUS));

	// Call jtrdll_get_status
	QByteArray ctlpipestr = RequestCommand("jtrdll_get_status\n");
	if (!ctlpipestr.startsWith("pipe="))
	{
		Q_ASSERT(0);
		return;
	}
	QString ctlpipename = QString::fromUtf8(ctlpipestr.mid(5));

	PIPETYPE pipe = OpenControlPipe(ctlpipename);
	if (!pipe)
	{
		Q_ASSERT(0);
		return;
	}

	QString cmd;
	QByteArray data;
	while (waitForCommand(pipe, cmd, data))
	{
		if (cmd == "status" && data.size() == sizeof(JTRDLL_STATUS))
		{
			*jtrdllstatus = *(JTRDLL_STATUS *)(data.data());
		}
	}

	CloseControlPipe(pipe);

	return;
}

int CLC7JTREXE::get_charset_info(const char *path, unsigned char * charmin, unsigned char *charmax, unsigned char *len, unsigned char *count, unsigned char allchars[256])
{
	// Call jtrdll_main
	QString cmdstr = "jtrdll_get_charset_info\n";
	cmdstr += QDir::toNativeSeparators(QString("%1\n").arg(path));
	
	// Get control pipe
	QByteArray ctlpipestr = RequestCommand(cmdstr);
	if (!ctlpipestr.startsWith("pipe="))
	{
		Q_ASSERT(0);
		return -1;
	}
	QString ctlpipename = QString::fromUtf8(ctlpipestr.mid(5));

	PIPETYPE pipe = OpenControlPipe(ctlpipename);
	if (!pipe)
	{
		Q_ASSERT(0);
		return -1;
	}

	int ret = 0;

	QString cmd;
	QByteArray data;
	while (waitForCommand(pipe, cmd, data))
	{
		if (cmd == "charmin" && data.size() == sizeof(unsigned char))
		{
			*charmin = *(unsigned char *)(data.data());
		}
		else if (cmd == "charmax" && data.size() == sizeof(unsigned char))
		{
			*charmax = *(unsigned char *)(data.data());
		}
		else if (cmd == "len" && data.size() == sizeof(unsigned char))
		{
			*len = *(unsigned char *)(data.data());
		}
		else if (cmd == "count" && data.size() == sizeof(unsigned char))
		{
			*count = *(unsigned char *)(data.data());
		}
		else if (cmd == "allchars" && data.size() == 256)
		{
			memcpy(allchars, data.data(), 256);
		}
		else if (cmd == "return" && data.size() == sizeof(ret))
		{
			ret = *(int *)(data.data());
		}
	}

	CloseControlPipe(pipe);

	return ret;
}

void CLC7JTREXE::preflight(int argc, char **argv, struct JTRDLL_HOOKS *hooks, struct JTRDLL_PREFLIGHT *jtrdllpreflight)
{
	// Call jtrdll_main
	QString cmdstr = "jtrdll_preflight\n";
	cmdstr += QString("%1\n").arg(hooks->appdatadir);
	cmdstr += QString("%1\n").arg(argc);
	for (int arg = 0; arg < argc; arg++)
	{
		cmdstr += (QString("%1\n").arg(argv[arg]).toUtf8());
	}

	// Get control pipe
	QByteArray ctlpipestr = RequestCommand(cmdstr);
	if (!ctlpipestr.startsWith("pipe="))
	{
		Q_ASSERT(0);
		return;
	}
	QString ctlpipename = QString::fromUtf8(ctlpipestr.mid(5));

	PIPETYPE pipe = OpenControlPipe(ctlpipename);
	if (!pipe)
	{
		Q_ASSERT(0);
		return;
	}

	int ret = 0;

	QString cmd;
	QByteArray data;
	while (waitForCommand(pipe, cmd, data))
	{
		if (cmd == "preflight")
		{
			*jtrdllpreflight = *(struct JTRDLL_PREFLIGHT *)data.data();
		}
		else if (cmd == "sigill")
		{
			hooks->caught_sigill = true;
			memset(jtrdllpreflight, 0, sizeof(struct JTRDLL_PREFLIGHT));
		}
		else if (cmd == "stdout")
		{
			hooks->stdout_hook(hooks->ctx, data);
		}
		else if (cmd == "stderr")
		{
			hooks->stderr_hook(hooks->ctx, data);
		}
	}

	CloseControlPipe(pipe);
}



void CLC7JTREXE::set_extra_opencl_kernel_args(const char *extra_opencl_kernel_args)
{
	// Call jtrdll_main
	QString cmdstr = "jtrdll_set_extra_opencl_kernel_args\n";
	cmdstr += QString("%1\n").arg(extra_opencl_kernel_args);

	// Get control pipe
	QByteArray ctlpipestr = RequestCommand(cmdstr);
	if (!ctlpipestr.startsWith("pipe="))
	{
		Q_ASSERT(0);
		return;
	}
	QString ctlpipename = QString::fromUtf8(ctlpipestr.mid(5));

	PIPETYPE pipe = OpenControlPipe(ctlpipename);
	if (!pipe)
	{
		Q_ASSERT(0);
		return;
	}

	CloseControlPipe(pipe);
}
/////////////////////////////////////////////////////////////////////////////////

QByteArray CLC7JTREXE::RequestCommand(QString cmdstr)
{
	QByteArray ret;
	const bool isGuiThread = (QThread::currentThread() == QCoreApplication::instance()->thread());
	if (isGuiThread)
	{
		slot_RequestCommand(cmdstr, ret);
	}
	else
	{
		emit sig_RequestCommand(cmdstr, ret);
	}

	return ret;
}

void CLC7JTREXE::slot_RequestCommand(QString cmdstr, QByteArray & ret)
{
	QMutexLocker reqcmdlocker(&m_reqcmd_mutex);

	// Write the command string plus a command id
	quint32 cmdid = m_cmdid;
	m_cmdid++;
	m_jtrprocess.write(QString("%1:%2").arg(cmdid).arg(cmdstr).toLatin1());
	m_jtrprocess.waitForBytesWritten();
	
	// Wait for response from pipe
	QByteArray response;
	forever
	{
		{
			QMutexLocker resplocker(&m_responses_mutex);
			if (m_responses.contains(cmdid))
			{
				response = m_responses[cmdid];
				break;
			}
		}

		m_jtrprocess.waitForReadyRead(1000);
		if (m_jtrprocess.state() == QProcess::NotRunning)
		{
			int exitcode = m_jtrprocess.exitCode();
			QString err = m_jtrprocess.errorString();
			QProcess::ProcessError errcode = m_jtrprocess.error();

			Q_ASSERT(0);
			return;
		}
	}
		
	if (response.endsWith("\r\n"))
		response.chop(2);
	else if (response.endsWith("\n"))
		response.chop(1);

	ret = response;
}


void CLC7JTREXE::slot_readyRead(void)
{
	QByteArray strstdout = m_jtrprocess.readAll();

	int colon = strstdout.indexOf(':');
	if (colon == -1)
	{
		Q_ASSERT(0);
		return;
	}
	
	quint32 cmdid = strstdout.left(colon).toUInt();
	QByteArray resp = strstdout.mid(colon + 1);

	QMutexLocker resplocker(&m_responses_mutex);
	m_responses[cmdid] = resp;

	// XXX: in a better world, we'd signal an event here
}




CLC7JTREXE::PIPETYPE CLC7JTREXE::OpenControlPipe(QString ctlpipename)
{
#ifdef _WIN32
	PIPETYPE pipe = CreateFile(
		ctlpipename.toLatin1(),   // pipe name 
		GENERIC_READ |  // read and write access 
		GENERIC_WRITE,
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		0,              // default attributes 
		NULL);         // no template file 
	if (pipe == INVALID_HANDLE_VALUE)
	{
		DWORD err = GetLastError();
		Q_ASSERT(0);

		return NULL;
	}
#else
#error "named pipe"
#endif
	return pipe;
}

void CLC7JTREXE::CloseControlPipe(PIPETYPE pipe)
{
#ifdef _WIN32
	CloseHandle(pipe);
#else
#error
#endif
}

bool CLC7JTREXE::readPipe(PIPETYPE pipe, size_t length, void *data)
{
#ifdef _WIN32
	char *cdata = (char *)data;

	while (length > 0)
	{
		DWORD dwBytesRead;
		if (!ReadFile(pipe, cdata, (DWORD)length, &dwBytesRead, NULL))
		{
			DWORD err = GetLastError();
			return false;
		}
		cdata += dwBytesRead;
		length -= dwBytesRead;
	}
#else
#error
#endif

	return true;
}


bool CLC7JTREXE::waitForCommand(PIPETYPE pipe, QString &cmd, QByteArray &data)
{
#ifdef _WIN32

	DWORD len;
	if (!readPipe(pipe, sizeof(len), &len))
	{
		return false;
	}

	QByteArray bacmdname;
	bacmdname.resize(len);
	if (!readPipe(pipe, len, bacmdname.data()))
	{
		return false;
	}
	cmd = QString::fromLatin1(bacmdname);

	if (!readPipe(pipe, sizeof(len), &len))
	{
		return false;
	}

	data.resize(len);
	if (len > 0)
	{
		if (!readPipe(pipe, len, data.data()))
		{
			return false;
		}
	}

	return true;

#else
#error
#endif
}

