#include"stdafx.h"

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#include<io.h>
#include<fcntl.h>
#include<crtdbg.h>
#endif



CJTRNodeWorker::CJTRNodeWorker(JTRWORKERCTX *ctx, JTRPASS *pass, int passnode, bool restore, QString potfilename, QString sessionfilename, QString hashesfilename) 
{TR;
	m_ctx = ctx;
	m_pass=pass;
	m_passnode=passnode;
	m_restore=restore;
	m_potfilename=potfilename;
	m_sessionfilename=sessionfilename;
	m_hashesfilename=hashesfilename;
	m_is_error=false;
	m_fallbackretry = false;
	
	m_exejtr = new CLC7ExecuteJTR(m_pass->nodes[m_passnode].jtrversion);
}
 
CJTRNodeWorker::~CJTRNodeWorker() 
{TR;
	delete m_exejtr;
	m_exejtr = NULL;
}

int CJTRNodeWorker::passnode()
{
	return m_passnode;
}

bool CJTRNodeWorker::fallbackretry()
{
	return m_fallbackretry;
}


bool CJTRNodeWorker::VerifyRestoreSession()
{TR;
	// Load up args from session file
	QStringList sessionargs;
	QFile infile(m_sessionfilename+".rec");
	if(!infile.open(QIODevice::ReadOnly))
	{
		return false;
	}
	QTextStream in(&infile);
	while(!in.atEnd())
	{
		sessionargs.append(in.readLine());
	}
	
	// Trim down to just args
	if(sessionargs[0]=="REC4")
	{
		sessionargs.removeFirst();
		int count=sessionargs[0].toInt();
		sessionargs=sessionargs.mid(1,count-1);
	}
	else
	{
		Q_ASSERT(0);
		return false;
	}

	// JtR session args must match at least our command line
	// If JtR adds extra arguments, it's okay too.
	foreach(QString arg, m_args)
	{
		if(!sessionargs.contains(arg))
		{
			return false;
		}
	}

	return true;
}

/*
static QString ToShortName(QString path)
{
	std::wstring wstr = path.toStdWString();

	DWORD length = GetShortPathNameW(wstr.c_str(), NULL, 0);
	if (length == 0)
		return path;

	wchar_t *buffer = new wchar_t[length];
	length = GetShortPathNameW(wstr.c_str(), buffer, length);
	if (length == 0)
	{
		delete[] buffer;
		return path;
	}
	
	QString ret = QString::fromWCharArray(buffer);
	delete[] buffer;
	return ret;
}
*/

bool CJTRNodeWorker::GenerateCommandLine(bool preflight)
{TR;
	m_args.clear();
	m_args
		<< "--no-log"
#ifdef _DEBUG
		<< "--verbosity=5"
#endif
//		<< "--skip-self-tests"
		;
	
	if (!preflight)
	{
		m_args << QString("--session=%1").arg(QDir::toNativeSeparators(m_sessionfilename));
		m_args << QString("--pot=%1").arg(QDir::toNativeSeparators(m_potfilename));
		m_args << QString("--format=%1").arg(m_pass->nodes[m_passnode].node_algorithm);

		if (m_pass->nodes[m_passnode].gpuinfo.platform != GPU_NONE && m_pass->nodes[m_passnode].gpuinfo.internal_index!=-1)
		{
			m_args << QString("--devices=%1").arg(m_pass->nodes[m_passnode].gpuinfo.internal_index);
		}
	}
	else
	{
		m_args << QString("--format=%1").arg(m_pass->nodes[m_passnode].preflight_node_algorithm);
	}

	if(m_pass->nodes.size()>1)
	{
		m_args << QString("--node=%1/%2").arg(m_passnode+1).arg(m_pass->nodes.size());
	}

	if (m_pass->jtrmode == "single")
	{
		m_args << QString("--single")
			<< QString("--encoding=%1").arg("UTF-8");
	}
	else if (m_pass->jtrmode == "wordlist")
	{
		m_args << QString("--wordlist=%1").arg(QDir::toNativeSeparators(m_pass->wordlist_file))
			<< QString("--encoding=%1").arg(m_pass->encoding)
			<< QString("--rules=%1").arg(m_pass->rule);
		if (m_pass->leet)
		{
			m_args << QString("--external=HybridLeet");
		}
	}
	else if(m_pass->jtrmode=="incremental")
	{
		m_args << QString("--incremental=file:%1").arg(QDir::toNativeSeparators(m_pass->character_set));
		m_args << QString("--min-length=%1").arg(m_pass->num_chars_min);
		m_args << QString("--max-length=%1").arg(m_pass->num_chars_max);
		m_args << QString("--internal-codepage=%1").arg(m_pass->encoding);
	}
	else if (m_pass->jtrmode == "mask")
	{
		if (m_pass->num_chars_min == m_pass->num_chars_max)
		{
			m_args << QString("--mask=%1").arg(m_pass->mask.repeated(m_pass->num_chars_max));
		}
		else
		{
			m_args << QString("--mask=%1").arg(m_pass->mask);
			m_args << QString("--min-length=%1").arg(m_pass->num_chars_min);
			m_args << QString("--max-length=%1").arg(m_pass->num_chars_max);
		}
		m_args << QString("--internal-codepage=%1").arg(m_pass->encoding);
	}
	else
	{
		Q_ASSERT(false);
		return false;
	}
	
	/*
	
	Don't bother doing time restriction here
	
	if (!m_ctx->m_duration_unlimited)
	{
		// Get total max run time in seconds
		int maxruntime = (m_ctx->m_duration_hours * 3600) + (m_ctx->m_duration_minutes * 60);

		// Take out the seconds spent in previous passes
		maxruntime -= m_ctx->m_elapsed_seconds_at_start_of_pass;

		// Divide time out by the remaining passes
		// This ensures each pass gets time to run, favoring the later passes.
		// xxx: yes, this is totally inaccurate
		//maxruntime /= (m_ctx->m_jtrpasses.size() - m_ctx->m_current_pass);

		m_args << QString("--max-run-time=%1").arg(maxruntime);
	}
	*/

	QString path = QDir::toNativeSeparators(m_hashesfilename);


	m_args << path;

	return true;
}


bool CJTRNodeWorker::ExecuteJTRCommandLine()
{
	TR;

	if (!m_exejtr->IsValid())
	{
		set_error("No version of the cracking engine is compatible with your system. Using a more modern CPU is required.");
		return false;
	}
	
	QString extra_opencl_kernel_args;
	if (!CLC7JTR::SelfTest(m_pass->nodes[m_passnode].jtrversion,
		m_pass->nodes[m_passnode].node_algorithm,
		m_pass->nodes[m_passnode].gpuinfo,
		&extra_opencl_kernel_args))
	{
		if (m_pass->nodes[m_passnode].gpuinfo.platform == GPU_NONE)
		{
			set_error("Self test failed for this algorithm. Try a different algorithm and report this issue to support@l0phtcrack.com.");
		}
		else
		{
			set_error("Self test failed for this algorithm. Try a different algorithm or update your GPU drivers.");
		}
		return false;
	}

#ifdef _DEBUG
	g_pLinkage->GetGUILinkage()->AppendToActivityLog(QString("ExecuteJTR: %1").arg(m_args.join("\n")));
#endif

	m_exejtr->SetCommandLine(m_args, extra_opencl_kernel_args);

	int retval;
	if ((retval = m_exejtr->ExecutePipe(this)) != 0)
	{
		QString instructionset;
		if (m_exejtr->HadIllegalInstruction())
		{
			if (m_pass->nodes[m_passnode].gpuinfo.platform!=GPU_NONE)
			{
				set_error(QString("The GPU performed an illegal operation. Your GPU may not be supported by L0phtCrack at this time. Contact support@l0phtcrack.com."));
				return false;
			}
			else
			{
				// Try a different instruction set
				g_pLinkage->GetGUILinkage()->AppendToActivityLog(QString("Rejecting instruction set '%1'. Falling back...").arg(m_pass->nodes[m_passnode].jtrversion));
				CLC7JTR::DisableInstructionSet(m_pass->nodes[m_passnode].jtrversion);
				m_fallbackretry = true;
			
				return false;
			}
		}
		if (!isInterruptionRequested())
		{
			set_error(QString("Error code: %1").arg(retval));
			return false;
		}
	}

	return true;
}

bool CJTRNodeWorker::preflight(CLC7ExecuteJTR::PREFLIGHT & preflight)
{
	if (!GenerateCommandLine(true))
	{
		set_error("Couldn't generate command line.");
		return false;
	}

	if (!m_exejtr->IsValid())
	{
		set_error("No version of the cracking engine is compatible with your system. Using a more modern CPU is required.");
		return false;
	}


	QString extra_opencl_kernel_args;
	if (!CLC7JTR::SelfTest(m_pass->nodes[m_passnode].jtrversion,
		m_pass->nodes[m_passnode].node_algorithm,
		m_pass->nodes[m_passnode].gpuinfo,
		&extra_opencl_kernel_args))
	{
		if (m_pass->nodes[m_passnode].gpuinfo.platform == GPU_NONE)
		{
			set_error("Self test failed for this algorithm. Try a different algorithm and report this issue to support@l0phtcrack.com.");
		}
		else
		{
			set_error("Self test failed for this algorithm. Try a different algorithm or update your GPU drivers.");
		}
		return false;
	}
#ifdef _DEBUG
	g_pLinkage->GetGUILinkage()->AppendToActivityLog(QString("PreflightJTR: %1").arg(m_args.join("\n")));
#endif

	m_exejtr->SetCommandLine(m_args, extra_opencl_kernel_args);

	m_exejtr->Preflight(preflight);
	if (!preflight.valid)
	{
		set_error("Preflight was invalid.");
		return false;
	}

	QString instructionset;
	if (m_exejtr->HadIllegalInstruction())
	{
		set_error("Preflight was invalid.");
		return false;
	}

	return true;
}

void CJTRNodeWorker::run() 
{TR;
	if(!GenerateCommandLine(false))
	{
		set_error("Couldn't generate command line.");
		return;
	}
	
	// Verify restore session args
	if(m_restore)
	{
		if(!VerifyRestoreSession())
		{
			set_error("Configuration has changed, session can not be restarted.");
			return;
		}

		// Restore is valid
		m_args.clear();
		m_args 
			<< "--no-log" 
#ifdef _DEBUG
			<< "--verbosity=5"
#endif
//			<< "--skip-self-tests"
			<< QString("--restore=%1").arg(QDir::toNativeSeparators(m_sessionfilename));
	}
			
	if(!ExecuteJTRCommandLine())
	{
		return;
	}
}

void CJTRNodeWorker::stop(bool timeout)
{TR;
	requestInterruption();

	// Tell JTR to stop
	if (m_exejtr)
	{
		m_exejtr->Abort(timeout);
	}
	
	wait();
}

JTRDLL_STATUS CJTRNodeWorker::get_status()
{
	if (!m_exejtr)
	{
		JTRDLL_STATUS jtrdllstatus;
		memset(&jtrdllstatus, 0, sizeof(jtrdllstatus));
		return jtrdllstatus;
	}

	JTRDLL_STATUS status = m_exejtr->GetStatus();

	m_etas.push_back(status.eta);
	if(m_etas.length()==11)
	{
		m_etas.pop_front();
	}
	unsigned int avg_eta=0;
	foreach(unsigned int eta, m_etas)
	{
		avg_eta+=eta;
	}
	avg_eta/=m_etas.length();

	status.eta = avg_eta;

	return status;
}

bool CJTRNodeWorker::lasterror(QString &err)
{TR;
	if(!m_is_error)
	{
		return false;
	}
	err=m_error;
	return true;
}

void CJTRNodeWorker::set_error(QString err)
{
	m_error=err;
	m_is_error=true;
}


static QString filterPrintable(QString str)
{
	QString out;
	foreach(QChar c, str)
	{
		if (c.isPrint() || c.isSpace())
		{
			out += c;
		}
	}
	return out;
}

void CJTRNodeWorker::ProcessStdOut(QByteArray line)
{
	QString out = filterPrintable(QString::fromUtf8(line));
	if (out.trimmed().size() == 0)
	{
		return;
	}
	if (m_pass->nodes.size() > 1)
	{
		out = QString("Node %1: ").arg(m_passnode + 1) + out;
	}
	g_pLinkage->GetGUILinkage()->AppendToActivityLog(out);
}

void CJTRNodeWorker::ProcessStdErr(QByteArray line)
{
	QString err = filterPrintable(QString::fromUtf8(line));
	if (err.trimmed().size() == 0)
	{
		return;
	}
	if (m_pass->nodes.size() > 1)
	{
		err = QString("Node %1: ").arg(m_passnode + 1) + err;
	}
	g_pLinkage->GetGUILinkage()->AppendToActivityLog(err);
}
