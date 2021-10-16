#include<stdafx.h>
#include<string.h>

// Create copy of jtrdll
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
QString ext = ".dll";
#elif defined(LINUX)
QString ext = ".so";
#elif defined(__APPLE__)
QString ext = ".dylib";
#endif

/////////////////////////////////

CLC7JTRDLL::CLC7JTRDLL(QString jtrdllversion)
{
	TR;
	m_jtrdll = NULL;
	m_jtrdllversion = jtrdllversion;

	m_jtrfname = g_pLinkage->NewTemporaryFile(false, ext);
	QDir d(g_pLinkage->GetPluginsDirectory());
	d.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");

	QFile::copy(d.filePath(QString("jtrdll_%1%2").arg(m_jtrdllversion).arg(ext)), m_jtrfname);
#if defined(_DEBUG) && (PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64)
	QFile::copy(d.filePath(QString("jtrdll_%1%2").arg(m_jtrdllversion).arg(".pdb")), m_jtrfname.left(m_jtrfname.length() - ext.length()) + ".pdb");
#endif

	m_jtrdll = new QLibrary(m_jtrfname);
	if (!m_jtrdll->load())
	{
		TRDBG("Can't load jtrdll");
		delete m_jtrdll;
		m_jtrdll = NULL;
		QFile::remove(m_jtrfname);
#if defined(_DEBUG) && (PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64)
		QFile::remove(m_jtrfname.left(m_jtrfname.length() - ext.length()) + ".pdb");
#endif
		return;
	}

	m_jtrdll_main = (TYPEOF_jtrdll_main *)m_jtrdll->resolve("jtrdll_main");
	m_jtrdll_abort = (TYPEOF_jtrdll_abort *)m_jtrdll->resolve("jtrdll_abort");
	m_jtrdll_get_status = (TYPEOF_jtrdll_get_status *)m_jtrdll->resolve("jtrdll_get_status");
	m_jtrdll_get_charset_info = (TYPEOF_jtrdll_get_charset_info *)m_jtrdll->resolve("jtrdll_get_charset_info");
	m_jtrdll_cleanup = (TYPEOF_jtrdll_cleanup *)m_jtrdll->resolve("jtrdll_cleanup");
	m_jtrdll_preflight = (TYPEOF_jtrdll_preflight *)m_jtrdll->resolve("jtrdll_preflight");
	m_jtrdll_set_extra_opencl_kernel_args = (TYPEOF_jtrdll_set_extra_opencl_kernel_args *)m_jtrdll->resolve("jtrdll_set_extra_opencl_kernel_args");

	if (m_jtrdll_main == NULL || m_jtrdll_abort == NULL || m_jtrdll_get_status == NULL || m_jtrdll_get_charset_info == NULL || m_jtrdll_cleanup == NULL || m_jtrdll_preflight == NULL || m_jtrdll_set_extra_opencl_kernel_args == NULL)
	{
		TRDBG("Can't resolve jtrdll functions");
		Q_ASSERT(0);
		delete m_jtrdll;
		m_jtrdll = NULL;
		m_jtrdll_main = NULL;
		m_jtrdll_abort = NULL;
		m_jtrdll_get_status = NULL;
		m_jtrdll_get_charset_info = NULL;
		m_jtrdll_cleanup = NULL;
		m_jtrdll_preflight = NULL;
		m_jtrdll_set_extra_opencl_kernel_args = NULL;
		QFile::remove(m_jtrfname);
#if defined(_DEBUG) && (PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64)
		QFile::remove(m_jtrfname.left(m_jtrfname.length() - ext.length()) + ".pdb");
#endif
	}
}


void CLC7JTRDLL::do_jtrdll_cleanup()
{
	__try
	{
		m_jtrdll_cleanup();
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

CLC7JTRDLL::~CLC7JTRDLL()
{
	TR;
	if (m_jtrdll)
	{
		do_jtrdll_cleanup();

		m_jtrdll_main = NULL;
		m_jtrdll_abort = NULL;
		m_jtrdll_get_status = NULL;
		m_jtrdll_get_charset_info = NULL;
		m_jtrdll_cleanup = NULL;
		m_jtrdll_preflight = NULL;
		m_jtrdll_set_extra_opencl_kernel_args = NULL;

		m_jtrdll->unload();
		delete m_jtrdll;
		m_jtrdll = NULL;
/* xxx
		QFile::remove(m_jtrfname);
		#if defined(_DEBUG) && (PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64)
		QFile::remove(m_jtrfname.left(m_jtrfname.length() - ext.length()) + ".pdb");
		#endif
*/
	}
}

bool CLC7JTRDLL::IsValid()
{
	return m_jtrdll != NULL;
}

int CLC7JTRDLL::main(int argc, char **argv, struct JTRDLL_HOOKS *hooks)
{
	return m_jtrdll_main(argc, argv, hooks);
}

void CLC7JTRDLL::terminate(void)
{
	m_jtrdll_abort(0);
}

void CLC7JTRDLL::abort(bool timeout)
{
	m_jtrdll_abort(timeout?1:0);
}

void CLC7JTRDLL::get_status(struct JTRDLL_STATUS *jtrdllstatus)
{
	return m_jtrdll_get_status(jtrdllstatus);
}

int CLC7JTRDLL::get_charset_info(const char *path, unsigned char * charmin, unsigned char *charmax, unsigned char *len, unsigned char *count, unsigned char allchars[256])
{
	return m_jtrdll_get_charset_info(path, charmin, charmax, len, count, allchars);
}

void CLC7JTRDLL::preflight(int argc, char **argv, struct JTRDLL_HOOKS *hooks, struct JTRDLL_PREFLIGHT *jtrdllpreflight)
{
	m_jtrdll_preflight(argc, argv, hooks, jtrdllpreflight);
}

void CLC7JTRDLL::set_extra_opencl_kernel_args(const char *extra_opencl_kernel_args)
{
	m_jtrdll_set_extra_opencl_kernel_args(extra_opencl_kernel_args);
}


