#include"stdafx.h"
#include<string.h>

////////////////////////////////////////////////////

#ifdef _MSC_VER
#define STRDUP _strdup
#else
#define STRDUP strdup
#endif

#define JTR_CACHE_VERSION 1

class CLC7ExecuteJTRHandler_Strings:public ILC7ExecuteJTRHandler
{
private:
	QString m_stdout;
	QString m_stderr;
public:
	CLC7ExecuteJTRHandler_Strings()
	{
	}

	virtual ~CLC7ExecuteJTRHandler_Strings()
	{
	}

	virtual void ProcessStdOut(QByteArray str)
	{
		m_stdout+=QString::fromLatin1(str);
	}

	virtual void ProcessStdErr(QByteArray str)
	{
		m_stderr+=QString::fromLatin1(str);
	}

	virtual QString GetStdOut()
	{
		return m_stdout;
	}
	virtual QString GetStdErr()
	{
		return m_stderr;
	}
};


void createJTRDLL(QList<QVariant> args)
{
	CLC7JTREXEDLL **jtrdll = (CLC7JTREXEDLL **)(args[1].toULongLong());
	*jtrdll = new CLC7JTREXEDLL(args[0].toString());
}

void destroyJTRDLL(QList<QVariant> args)
{
	CLC7JTREXEDLL *jtrdll = (CLC7JTREXEDLL *)(args[0].toULongLong());
	delete jtrdll;
}

CLC7ExecuteJTR::CLC7ExecuteJTR(QString version)
{TR;
	m_illegal_instruction = false;

	QList<QVariant> args;
	args.append(version);
	args.append((qulonglong)&m_jtrdll);
	g_pLinkage->GetGUILinkage()->Callback(createJTRDLL, args);
}

CLC7ExecuteJTR::~CLC7ExecuteJTR()
{TR;
	QList<QVariant> args;
	args.append((qulonglong)m_jtrdll);
	g_pLinkage->GetGUILinkage()->Callback(destroyJTRDLL, args);
}

bool CLC7ExecuteJTR::IsValid()
{
	return m_jtrdll->IsValid();
}


void CLC7ExecuteJTR::SetCommandLine(QStringList args, QString extra_opencl_kernel_args)
{TR;
	QDir johndir(g_pLinkage->GetPluginsDirectory());
	johndir.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
	QString command = QDir::toNativeSeparators(johndir.filePath("john.exe"));
	
	m_command=command;
	m_args=args;
	m_extra_opencl_kernel_args = extra_opencl_kernel_args;

	TRDBG(QString("CLC7ExecuteJTR: cwd='%1' command='%2' c='%3'\n").arg(QDir::currentPath()).arg(command).arg(extra_opencl_kernel_args).toUtf8().constData());
	foreach(QString arg, args)
	{
		TRDBG(QString("arg='%1'\n").arg(arg).toUtf8().constData());
	}
}
	

bool CLC7ExecuteJTR::HadIllegalInstruction()
{
	return m_illegal_instruction;
}

static void null_stdout_hook(void *ctx, const char *str)
{
}

static void null_stderr_hook(void *ctx, const char *str)
{
}

void CLC7ExecuteJTR::CheckCache(QDir cachedir)
{
	QFile versionfile(cachedir.absoluteFilePath("version"));
	bool validversion = false;
	if (versionfile.exists())
	{
		if (versionfile.open(QIODevice::ReadOnly))
		{
			QByteArray versionstr = versionfile.readAll();
			int version = versionstr.trimmed().toInt();
			if (version == JTR_CACHE_VERSION)
			{
				validversion = true;
			}
			versionfile.close();
		}
	}
	if (!validversion)
	{
		cachedir.removeRecursively();
		cachedir.mkpath(".");
		if (versionfile.open(QIODevice::WriteOnly))
		{
			versionfile.write(QByteArray::number(JTR_CACHE_VERSION));
			versionfile.close();
		}
	}
}

void CLC7ExecuteJTR::Preflight(PREFLIGHT &preflight)
{
	m_illegal_instruction = false;

	int numargs = m_args.length();
	char **argv = (char **)malloc(sizeof(const char *) * (numargs + 2));
	char **original_argv = (char **)malloc(sizeof(const char *) * (numargs + 2));

	QByteArray ba_command = m_command.toUtf8();
	original_argv[0] = argv[0] = STRDUP(ba_command.constData());
	for (int i = 0; i<numargs; i++)
	{
		QByteArray ba_arg = m_args[i].toUtf8();
		original_argv[i + 1] = argv[i + 1] = STRDUP(ba_arg.constData());
	}
	original_argv[numargs + 1] = argv[numargs + 1] = NULL;

	JTRDLL_HOOKS hooks;
	memset(&hooks, 0, sizeof(hooks));

	hooks.ctx = NULL;
	QDir appdatadir(g_pLinkage->GetCacheDirectory());
	appdatadir.mkdir("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
	appdatadir.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");

	CheckCache(appdatadir);

	appdatadir.mkdir("kernels");	

	strcpy_s(hooks.appdatadir, sizeof(hooks.appdatadir), QDir::toNativeSeparators(appdatadir.absolutePath()).toUtf8().constData());

	hooks.appdatadir[_MAX_PATH - 1] = 0;
	hooks.caught_sigill = 0;
	hooks.stdout_hook = null_stdout_hook;
	hooks.stderr_hook = null_stderr_hook;

	JTRDLL_PREFLIGHT jtrdllpreflight;
	m_jtrdll->preflight(numargs + 1, argv, &hooks, &jtrdllpreflight);
	
	preflight.valid = jtrdllpreflight.valid;
	preflight.salt_count = jtrdllpreflight.salt_count;
	preflight.wordlist_rule_count = jtrdllpreflight.wordlist_rule_count;
	preflight.mask_candidates = jtrdllpreflight.mask_candidates;
	preflight.incremental_candidates = jtrdllpreflight.incremental_candidates;
	
	for (int i = 0; i <= numargs; i++)
	{
		free(original_argv[i]);
	}
	free(original_argv);
	free(argv);

	if (hooks.caught_sigill)
	{
		m_illegal_instruction = true;
	}

	return;
}

int CLC7ExecuteJTR::ExecuteWait(QString & out, QString &err)
{
	CLC7ExecuteJTRHandler_Strings handler;
	int ret=ExecutePipe(&handler);
	out=handler.GetStdOut();
	err=handler.GetStdErr();
	return ret;
}

static void stdout_hook(void *ctx, const char *str)
{
	((ILC7ExecuteJTRHandler *)ctx)->ProcessStdOut(str);
}

static void stderr_hook(void *ctx, const char *str)
{
	((ILC7ExecuteJTRHandler *)ctx)->ProcessStdErr(str);
}


int CLC7ExecuteJTR::ExecutePipe(ILC7ExecuteJTRHandler *handler)
{
	int retval;
	__try
	{
		retval = ExecutePipeGuarded(handler);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		retval = 1;
		m_illegal_instruction = true;
	}

	return retval;
}

int CLC7ExecuteJTR::ExecutePipeGuarded(ILC7ExecuteJTRHandler *handler)
{TR;
	m_illegal_instruction = false;

	int numargs=m_args.length();
	char **argv=(char **)malloc(sizeof(const char *) * (numargs+2));
	char **original_argv=(char **)malloc(sizeof(const char *) * (numargs+2));
	
	QByteArray ba_command=m_command.toUtf8();
	original_argv[0] = argv[0] = STRDUP(ba_command.constData());
	for(int i=0;i<numargs;i++)
	{
		QByteArray ba_arg = m_args[i].toUtf8();
		original_argv[i+1]=argv[i+1]=STRDUP(ba_arg.constData());
	}
	original_argv[numargs+1]=argv[numargs+1] = NULL;
	
	JTRDLL_HOOKS hooks;
	memset(&hooks, 0, sizeof(hooks));

	hooks.ctx=handler;
	QDir appdatadir(g_pLinkage->GetCacheDirectory());
	appdatadir.mkdir("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
	appdatadir.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");

	CheckCache(appdatadir);

	appdatadir.mkdir("kernels");
	
	strcpy_s(hooks.appdatadir, sizeof(hooks.appdatadir), QDir::toNativeSeparators(appdatadir.absolutePath()).toUtf8().constData());

	hooks.appdatadir[_MAX_PATH - 1] = 0;
	hooks.caught_sigill = 0;
	hooks.stdout_hook=stdout_hook;
	hooks.stderr_hook=stderr_hook;
	
	m_jtrdll->set_extra_opencl_kernel_args(m_extra_opencl_kernel_args.toUtf8().constData());

	int retval;
	retval = m_jtrdll->main(numargs + 1, argv, &hooks);
	
	for(int i=0;i<=numargs;i++)
	{
		free(original_argv[i]);
	}
	free(original_argv);
	free(argv);

	if (hooks.caught_sigill)
	{
		m_illegal_instruction = true;
	}

	return retval;
}

void CLC7ExecuteJTR::Abort(bool timeout)
{
	TR;

	m_jtrdll->abort(timeout);
}

void CLC7ExecuteJTR::Terminate(void)
{
	TR;

	m_jtrdll->terminate();
}


JTRDLL_STATUS CLC7ExecuteJTR::GetStatus(void)
{
	JTRDLL_STATUS jtrdllstatus;
	m_jtrdll->get_status(&jtrdllstatus);
	return jtrdllstatus;
}



