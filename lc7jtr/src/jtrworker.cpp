#include"stdafx.h"

CJTRWorker::CJTRWorker(CLC7JTR *lc7jtr, JTRWORKERCTX *ctx, ILC7CommandControl *ctrl, ILC7AccountList *accountlist)
{
	TR;
	m_lc7jtr = lc7jtr;
	m_ctrl=ctrl;
	m_ctx=ctx;
	m_accountlist = accountlist;
	m_is_error=false;
	m_potsize=0;
	m_status_available = false;
	
	// XXX: Sorry for the Win32!
	// If porting this, use 'pevents', there is no equivalent
	// semantic in straight Qt! Qt conditions are not quite right!
	m_workerDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}
 
CJTRWorker::~CJTRWorker() 
{TR;
	CleanUpNodeWorkers();

	CloseHandle(m_workerDoneEvent);
}

void CJTRWorker::CleanUpNodeWorkers()
{
	m_status_available = false;

	foreach(CJTRNodeWorker *nodeworker, m_nodeworkers)
	{
		delete nodeworker;
	}
	m_nodeworkers.clear();
}

void CJTRWorker::stop()
{TR;
	requestInterruption();

	// Tell worker to stop
	SetEvent(m_workerDoneEvent);
	
	wait();
}

bool CJTRWorker::GenerateCrackedWordlist(QString &wordlist_file, QString &error)
{
	// Verify we have passwords to test
	m_accountlist->Acquire();

	bool have_passwords = false;
	size_t cnt = m_accountlist->GetAccountCount();
	for (int i = 0; i < cnt; i++)
	{
		const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(i);

		// Split hashes in account into map
		foreach(LC7Hash lc7hash, acct->hashes)
		{
			if (lc7hash.password.size() > 0)
			{
				have_passwords = true;
				break;
			}
		}
		if (have_passwords)
		{
			break;
		}
	}
	if (!have_passwords)
	{
		// it's okay, but we dont need the pass
		m_accountlist->Release();
		return true;
	}

	// Write out already cracked passwords into temporary dictionary file named 'toggle'
	if (!m_ctx || m_ctx->m_temporary_dir.isEmpty())
	{
		m_accountlist->Release();
		return false;
	}

	QDir tempdir(m_ctx->m_temporary_dir);
	QString togglepath = tempdir.filePath(QString("toggle"));

	QFile togglefile(togglepath);
	if (!togglefile.open(QIODevice::WriteOnly))
	{
		m_accountlist->Release();
		return false;
	}

	for (int i = 0; i < cnt; i++)
	{
		const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(i);

		// Split hashes in account into map
		foreach(LC7Hash lc7hash, acct->hashes)
		{
			if (lc7hash.password.size() > 0)
			{
				QString line(lc7hash.password + "\n");
				QByteArray ba = line.toLatin1();
				togglefile.write(ba); // xxx: until jtr supports utf8 here. latin1 it is :(
			}
		}
	}

	togglefile.close();

	// return the toggle wordlist
	wordlist_file = togglepath;

	m_accountlist->Release();
	return true;
}

void CJTRWorker::slot_workerDone(void)
{
	SetEvent(m_workerDoneEvent);
}

bool CJTRWorker::RunPass(JTRPASS & pass, QString & error)
{TR;
	// Mark start of this pass and set elapsed time at last pass
	m_ctx->m_elapsed_seconds_at_start_of_pass = m_ctx->m_elapsed_seconds;
	
	QDateTime dt_start = QDateTime::currentDateTime();
	quint32 secsleft = 0;
	if (pass.durationblock != -1)
	{
		secsleft = m_ctx->m_duration_block_seconds_left[pass.durationblock];

		// Extra sanity check, this is duplicated in CJTRWorker::run
		if (secsleft == 0) { // duration block timeout
			// If there's no time left in this block, then don't bother
			return true;
		}
	}
	// global timeout sanity check, duplicated in CJTRWorker::run
	if(!m_ctx->m_duration_unlimited && (m_ctx->m_elapsed_seconds_at_start_of_pass >= m_ctx->m_duration_seconds))
	{
		return true;
	}
	
	// Generate hash list if we're not restoring, else just keep the same hashes file from before
	if (!m_ctx->m_restore)
	{
		if (!m_lc7jtr->ProcessHashes(pass.hashtype, false, true))
		{
			m_error = "Failed to create hashes file";
			delete m_ctx;
			m_ctx = NULL;
			return false;
		}
	}

	// Generate crack wordlist if this is a togglepass
	if (pass.wordlist_file == "$$CRACKED$$")
	{
		if (!GenerateCrackedWordlist(pass.wordlist_file, error))
		{
			return false;
		}
		if (pass.wordlist_file == "$$CRACKED$$")
		{
			// Skip the pass if no wordlist file was generated
			return true;
		}
	}

	bool retry = false;
	bool failed = false;
	bool timeout = false;

	do
	{
		retry = false;

		// Do pass
		for (int passnode = 0; passnode<pass.nodes.size(); passnode++)
		{
			StartNode(pass, passnode);
		}
		
		m_status_available = true;
		
		// Wait for all node threads to finish
		QList<CJTRNodeWorker *> alive_workers = m_nodeworkers;
		while (alive_workers.size() > 0)
		{
			WaitForSingleObject(m_workerDoneEvent, 1000);

			// Check for timeout
			QDateTime dt_end = QDateTime::currentDateTime();
			quint32 timespent = dt_start.secsTo(dt_end);
			if ((pass.durationblock != -1 && timespent >= secsleft) ||	// duration block timeout
				(!m_ctx->m_duration_unlimited && (m_ctx->m_elapsed_seconds_at_start_of_pass + timespent) >= m_ctx->m_duration_seconds))		// global timeout
			{
				timeout = true;
			}

			// If some workers are done, see how they ended up.
			// if they force a retry, or they failed, bail everything

			foreach(CJTRNodeWorker *nodeworker, QList<CJTRNodeWorker *>(alive_workers))
			{
				if (nodeworker->isFinished())
				{
					alive_workers.removeOne(nodeworker);

					if (nodeworker->fallbackretry())
					{
						retry = true;
					}
					QString nodeerror;
					if (nodeworker->lasterror(nodeerror))
					{
						error = error + QString("node #%1: %2").arg(nodeworker->passnode()).arg(nodeerror) + "\n";
						failed = true;
					}
				}
			}

			if (retry || failed || timeout || isInterruptionRequested())
			{
				m_status_available = false;

				// Kill off all node workers
				foreach(CJTRNodeWorker *nodeworker, alive_workers)
				{
					nodeworker->stop(timeout);
				}
				alive_workers.clear();
			}
		}

		m_status_available = false;
		CleanUpNodeWorkers();
	} while (retry && !failed && !timeout);

	// Force status processing at end of pass to get any remaining passwords from pot
	m_lc7jtr->ProcessStatus();

	// add to elapsed time
	QDateTime dt_end = QDateTime::currentDateTime();
	quint32 timespent = dt_start.secsTo(dt_end);
	m_ctx->m_elapsed_seconds += timespent;

	// Update duration block if we are part of one
	if (pass.durationblock != -1)
	{
		if (timespent > secsleft)
		{
			m_ctx->m_duration_block_seconds_left[pass.durationblock] = 0;
		}
		else
		{
			m_ctx->m_duration_block_seconds_left[pass.durationblock] -= timespent;
		}
	}


	return !failed;
}

void CJTRWorker::run()
{
	TR;
	setPriority(QThread::LowestPriority);

	// Load up potfile
	QDir tempdir(m_ctx->m_temporary_dir);
	QString potfilename=tempdir.filePath(tempdir.filePath("pot"));
	QFileInfo potinfo(potfilename);
	if(potinfo.exists())
	{
		m_potsize=potinfo.size();
	}
	
	// Do all passes
	while(m_ctx->m_current_pass < m_ctx->m_jtrpasses.size())
	{
		JTRPASS & pass = m_ctx->m_jtrpasses[m_ctx->m_current_pass];

		// Take time from any previous duration blocks that have any left if we are in a pass with a durationblock (not unlimited)
		if (pass.durationblock != -1)
		{
			quint32 extratime = 0;
			for (int db = 0; db < pass.durationblock; db++)
			{
				quint32 dbtime = m_ctx->m_duration_block_seconds_left[db];
				m_ctx->m_duration_block_seconds_left[db] = 0;
				extratime += dbtime;
			}
			m_ctx->m_duration_block_seconds_left[pass.durationblock] += extratime;
		}

		// Mark start of this pass and set elapsed time at last pass
		bool run_pass = true;
		quint32 secsleft = 0;
		if (pass.durationblock != -1)
		{
			if (m_ctx->m_duration_block_seconds_left[pass.durationblock] == 0)	// duration block timeout
			{
				// If there's no time left in this block, then don't bother
				run_pass = false;
			}
		}
		if (!m_ctx->m_duration_unlimited && (m_ctx->m_elapsed_seconds >= m_ctx->m_duration_seconds))  // global timeout
		{
			run_pass = false;
		}


		bool failed = false;
		QString error;
		
		if (run_pass)
		{
			m_ctrl->AppendToActivityLog(QString("\nStarting pass: %1\n").arg(pass.passdescription));
			failed = !RunPass(pass, error);
		}
		
		// Fail out
		if(failed)
		{
			this->set_error(error);
			break;
		}

		// Don't clear session files if we stopped in the middle
		if(isInterruptionRequested())
		{
			break;
		}

		// Clear session files
		QDir tempdir(m_ctx->m_temporary_dir);
		QStringList sessionfiles=tempdir.entryList(QStringList("session*"),QDir::NoDotAndDotDot);
		foreach(QString sessionfile,sessionfiles)
		{
			tempdir.remove(sessionfile);
		}

		// Turn off 'restore' for next pass, since 
		// we only want to restore the last pass that we stopped at
		m_ctx->m_restore = false;

		m_ctx->m_current_pass++;
	}
}

void CJTRWorker::StartNode(JTRPASS &pass, int passnode)
{TR;
	QDir tempdir(m_ctx->m_temporary_dir);
	QString potfilename=tempdir.filePath(tempdir.filePath("pot"));
	QString sessionfilename=tempdir.filePath(QString("session.%1").arg(passnode));
	QString hashesfilename=tempdir.filePath(QString("hashes"));
	
	if(m_ctx->m_restore && !tempdir.exists(sessionfilename+".rec"))
	{
		// This node is already done, dont start it
		return;
	}
	
	CJTRNodeWorker *pJTRNodeWorker = new CJTRNodeWorker(m_ctx, &pass, passnode, m_ctx->m_restore, potfilename, sessionfilename, hashesfilename);
	m_nodeworkers.append(pJTRNodeWorker);

	connect(pJTRNodeWorker, &CJTRNodeWorker::finished, this, &CJTRWorker::slot_workerDone, Qt::DirectConnection);

	pJTRNodeWorker->start();
}

bool CJTRWorker::lasterror(QString &err)
{TR;
	if(!m_is_error)
	{
		return false;
	}
	err=m_error;
	return true;
}

void CJTRWorker::set_error(QString err)
{TR;
	m_error=err;
	m_is_error=true;
}

QString CJTRWorker::format_speed(unsigned long long count)
{
	double cnt=(double)count;
	QString suffix="";
	if(cnt>1000.0)
	{
		cnt/=1000.0;
		suffix="K";
	}
	else
	{
		return QString("%1").arg(count);
	}
	if(cnt>1000.0)
	{
		cnt/=1000.0;
		suffix="M";
	}
	if(cnt>1000.0)
	{
		cnt/=1000.0;
		suffix="G";
	}
	if(cnt>1000.0)
	{
		cnt/=1000.0;
		suffix="T";
	}
	return QString("%1%2").arg(cnt,0,'f',3).arg(suffix);
}

JTRSTATUS CJTRWorker::get_status()
{TR;
	if (!m_status_available)
	{
		return m_last_status;
	}

	FOURCC hashtype = m_ctx->m_jtrpasses[m_ctx->m_current_pass].hashtype;
	int durationblock = m_ctx->m_jtrpasses[m_ctx->m_current_pass].durationblock;
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	LC7HashType lc7hashtype;
	QString error;
	QString hashname = "???";
	if (passlink->LookupHashType(hashtype, lc7hashtype, error))
	{
		hashname = lc7hashtype.name;
	}


	JTRDLL_STATUS total_status;
	memset(&total_status,0,sizeof(JTRDLL_STATUS));

	int count=0;
	int donecount=0;

	foreach(CJTRNodeWorker *nodeworker,m_nodeworkers)
	{
		JTRDLL_STATUS status=nodeworker->get_status();

		if(status.stage==3)
		{
			donecount++;
		}
		else if(status.stage>total_status.stage)
		{			
			total_status.stage=status.stage;
		}
		
		if(status.stage==2)
		{
			total_status.percent += status.percent;
			total_status.time+=status.time;
			total_status.eta+=status.eta;
			total_status.guess_count += status.guess_count;
			total_status.candidates += status.candidates;
			total_status.guesses_per_second += status.guesses_per_second;
			total_status.candidates_per_second += status.candidates_per_second;
			total_status.crypts_per_second += status.crypts_per_second;
			total_status.combinations_per_second += status.combinations_per_second;
			total_status.fanspeed = qMax(status.fanspeed, total_status.fanspeed);
			total_status.temperature = qMax(status.temperature, total_status.temperature);
			total_status.utilization = qMax(status.utilization, total_status.utilization);

			if (status.percent<100 && !total_status.word1[0])
			{
				memcpy(total_status.word1, status.word1, sizeof(total_status.word1));
				memcpy(total_status.word2, status.word2, sizeof(total_status.word2));
			}
			count++;
		}
	}
	
	if(donecount!=m_nodeworkers.size())
	{
		
		JTRSTATUS jtrstatus;

		if(total_status.stage==0)
		{
			JTRSTATUS jtrstatus;
			jtrstatus.status = jtrstatus.status = QString("Pass %1/%2 (%3): Starting Pass %1...").
				arg(m_ctx->m_current_pass + 1).
				arg(m_ctx->m_jtrpasses.size()).
				arg(hashname);
			jtrstatus.percent_done = 0;
			jtrstatus.details="";
			jtrstatus.secs_total = 0;
			
			m_last_status = jtrstatus;
			return jtrstatus;
		}
		else if(total_status.stage==1)
		{
			JTRSTATUS jtrstatus;
			jtrstatus.status = jtrstatus.status = QString("Pass %1/%2 (%3): Initializing Pass %1...").
				arg(m_ctx->m_current_pass + 1).
				arg(m_ctx->m_jtrpasses.size()).
				arg(hashname);
			jtrstatus.percent_done=0;
			jtrstatus.details="";
			jtrstatus.secs_total = 0;
			
			m_last_status = jtrstatus;
			return jtrstatus;
		}
		else if(total_status.stage==2)
		{
			total_status.percent/=count;
			total_status.time/=count;
			total_status.eta/=count;
			
			// XXX Calculate this ourselves?
			
			if (total_status.percent < 0.001)
			{
				total_status.eta = 0;
			}
			else
			{
				total_status.eta = total_status.time * (100.0 - total_status.percent) / total_status.percent;
			}
			

			// for pass time
			//int elapsed_days,elapsed_hours,elapsed_minutes,elapsed_seconds;
			//elapsed_days=total_status.time/(24*60*60);
			//elapsed_hours=(total_status.time % (24*60*60))/(60*60);
			//elapsed_minutes=(total_status.time % (60*60))/60;
			//elapsed_seconds=(total_status.time % 60);
			
			//QString elapsed_str=QString("%1d%2h%3m%4s").arg(elapsed_days).arg(elapsed_hours).arg(elapsed_minutes).arg(elapsed_seconds);

			quint32 elapsed_days,elapsed_hours,elapsed_minutes,elapsed_seconds;
			quint32 elapsed_time = m_ctx->m_elapsed_seconds + total_status.time;
			elapsed_days = elapsed_time / (24 * 60 * 60);
			elapsed_hours = (elapsed_time % (24 * 60 * 60)) / (60 * 60);
			elapsed_minutes = (elapsed_time % (60 * 60)) / 60;
			elapsed_seconds = (elapsed_time % 60);

			QString elapsed_str=QString("%1d%2h%3m%4s").arg(elapsed_days).arg(elapsed_hours).arg(elapsed_minutes).arg(elapsed_seconds);

			// Cap pass eta if our pass has a durationblock
			if (durationblock!=-1)
			{
				quint32 secsleft = m_ctx->m_duration_block_seconds_left[durationblock];
				unsigned int new_eta = (secsleft >= total_status.time) ? (secsleft - total_status.time) : 0;

				if(new_eta < total_status.eta || total_status.eta==0)
				{
					total_status.eta = new_eta;
				}
			}
			// Cap pass eta if our pass somehow has more time than the max time
			if (!m_ctx->m_duration_unlimited)
			{
				unsigned int max_eta = m_ctx->m_duration_seconds - (m_ctx->m_elapsed_seconds + total_status.time);
				if (max_eta < total_status.eta)
				{
					total_status.eta = max_eta;
				}
			}

			QString eta_str;
			if(total_status.eta!=0 && total_status.eta < (24*60*60*365))
			{	
				int eta_days,eta_hours,eta_minutes,eta_seconds;
				eta_days=total_status.eta/(24*60*60);
				eta_hours=(total_status.eta % (24*60*60))/(60*60);
				eta_minutes=(total_status.eta % (60*60))/60;
				eta_seconds=(total_status.eta % 60);
				
				/*
				static unsigned int last_total_status_eta = 0;
				static unsigned int estim_count = 10;
				bool estim = last_total_status_eta < total_status.eta;
				last_total_status_eta = total_status.eta;
				if (estim || estim_count > 0)
				{
					eta_str = QString("Calculating pass time left...  ");
					if (estim)
					{
						estim_count = 10;
					}
					else
					{
						estim_count--;
					}
				}
				else
				{*/
					eta_str = QString("Pass Time Left: %1d%2h%3m%4s  ").arg(eta_days).arg(eta_hours).arg(eta_minutes).arg(eta_seconds);
				//}
				
				total_status.percent=100.0f*(float)total_status.time/((float)total_status.time+(float)total_status.eta);
			}
			else
			{
				eta_str="";
				total_status.percent=0;
			}

			QString max_eta_str;
			if (!m_ctx->m_duration_unlimited)
			{
				unsigned int max_eta = m_ctx->m_duration_seconds - (m_ctx->m_elapsed_seconds + total_status.time);

				int eta_days, eta_hours, eta_minutes, eta_seconds;
				eta_days = max_eta / (24 * 60 * 60);
				eta_hours = (max_eta % (24 * 60 * 60)) / (60 * 60);
				eta_minutes = (max_eta % (60 * 60)) / 60;
				eta_seconds = (max_eta % 60);

				max_eta_str = QString("%1d%2h%3m%4s").arg(eta_days).arg(eta_hours).arg(eta_minutes).arg(eta_seconds);
			}
			else
			{
				max_eta_str = "Infinite";
			}

			jtrstatus.status=QString("Pass %1/%2 (%3): Elapsed Time: %4  %5Max Time Left: %6  Speed: %7c/s  Current Guess: %8..%9").
				arg(m_ctx->m_current_pass+1).
				arg(m_ctx->m_jtrpasses.size()).
				arg(hashname).
				arg(elapsed_str).
				arg(eta_str).
				arg(max_eta_str).
				arg(format_speed(total_status.crypts_per_second)).
				arg(QString::fromUtf8(total_status.word1)).arg(QString::fromUtf8(total_status.word2));

			jtrstatus.details=QString("%1g %2g/s %3p/s %4C/s").
				arg(total_status.guess_count).
				arg(total_status.guesses_per_second).
				arg(total_status.candidates_per_second).
				arg(total_status.combinations_per_second);

			jtrstatus.percent_done=total_status.percent;
			jtrstatus.secs_total = total_status.time;

			m_last_status = jtrstatus;
			return jtrstatus;
		}
	}

	JTRSTATUS jtrstatus;
	jtrstatus.status = QString("Pass %1/%2 (%3): Pass Complete").
		arg(m_ctx->m_current_pass + 1).
		arg(m_ctx->m_jtrpasses.size()).
		arg(hashname);

	jtrstatus.percent_done=100.0;
	jtrstatus.details="";
	jtrstatus.secs_total = 0;

	m_last_status = jtrstatus;
	return jtrstatus;
}

QMap<QByteArray,QString> CJTRWorker::get_cracked()
{TR;
	QMap<QByteArray,QString> crackedmap;

	QDir tempdir(m_ctx->m_temporary_dir);
	QString potfilename=tempdir.filePath(tempdir.filePath("pot"));
	
	QFileInfo potinfo(potfilename);
	if(potinfo.exists())
	{
		size_t potsize=potinfo.size();
		if(m_potsize!=potsize)
		{
			QFile pf(potfilename);
			pf.open(QIODevice::ReadOnly);
			pf.seek(m_potsize);
			QByteArray potba=pf.readAll();
			QString pot=QString::fromUtf8(potba);

			// Remove all \r
			pot = pot.replace("\r", "");

			QStringList potlist=pot.split("\n",QString::SkipEmptyParts);
			
			m_potsize=potsize;

			foreach(QString item, potlist)
			{
				int colon=item.indexOf(":");
				if(colon!=-1)
				{
					QString hash=item.left(colon);
					QString password=item.mid(colon+1);
					
					crackedmap.insert(hash.toLatin1(),password);
				}
			}			
		}
	}

	return crackedmap;
}



//////////////////////////////
// JTRPASSNODE

void JTRPASSNODE::Save(QVariant & v)
{TR;
	QMap<QString,QVariant> saved;
	
	saved["revision"] = 2;

	QByteArray ba_gpuinfo;
	QDataStream ds_gpuinfo(&ba_gpuinfo, QIODevice::WriteOnly);
	ds_gpuinfo << gpuinfo;
	
	saved["node"]=node;
	saved["nodecount"]=nodecount;
	saved["gpuinfo"] = ba_gpuinfo;
	saved["jtrversion"] = jtrversion;
	saved["node_algorithm"]=node_algorithm;
	saved["preflight_node_algorithm"] = preflight_node_algorithm;

	v=saved;
}

bool JTRPASSNODE::Load(const QVariant & v)
{
	TR;
	QMap<QString, QVariant> saved = v.toMap();
	
	node = saved["node"].toInt();
	nodecount = saved["nodecount"].toInt();
	if (saved["revision"].toInt() == 1)
	{	
		bool gpu_enable = saved["gpu_enable"].toBool();
		GPUPLATFORM gpu_platform = (GPUPLATFORM)saved["gpu_platform"].toInt();
		int gpu_jtrindex = saved["gpu_jtrindex"].toInt();
		if (gpu_enable)
		{
			CLC7JTRGPUManager ginfo;
			ginfo.Detect();
			QVector<LC7GPUInfo> givec = ginfo.GetGPUInfo();
			for (auto gi : givec)
			{
				if (gi.internal_index == gpu_jtrindex && gi.platform == gpu_platform)
				{
					gpuinfo = gi;
				}
			}
		}
	}
	else if (saved["revision"].toInt() == 2)
	{
		QByteArray ba_gpuinfo = saved["gpuinfo"].toByteArray();
		QDataStream ds_gpuinfo(&ba_gpuinfo, QIODevice::WriteOnly);
		ds_gpuinfo >> gpuinfo;
	}
	else
	{
		return false;
	}

	jtrversion=saved["jtrversion"].toString();
	node_algorithm=saved["node_algorithm"].toString();
	preflight_node_algorithm = saved["preflight_node_algorithm"].toString();
	
	return true;
}

//////////////////////////////
// JTRPASS

void JTRPASS::Save(QVariant & v)
{TR;
	QMap<QString,QVariant> saved;

	saved["revision"] = 1;

	saved["passnumber"] = passnumber;
	saved["passdescription"] = passdescription;

	saved["durationblock"] = durationblock;

	saved["hashtype"]=hashtype;
	saved["jtrmode"]=jtrmode;
	saved["wordlist_file"]=wordlist_file;
	saved["wordlist_count"] = wordlist_count;
	saved["encoding"] = encoding;
	saved["rule"]=rule;
	saved["leet"]=leet;

	saved["num_chars_min"]=num_chars_min;
	saved["num_chars_max"]=num_chars_max;
	saved["character_set"]=character_set;
	saved["mask"] = mask;

	QList<QVariant> vnodes;
	foreach(JTRPASSNODE node, nodes)
	{
		QVariant vnode;
		node.Save(vnode);
		vnodes.append(vnode);
	}
	saved["nodes"]=vnodes;

	v=saved;
}

bool JTRPASS::Load(const QVariant & v)
{TR;
	QMap<QString,QVariant> saved=v.toMap();

	if (saved["revision"].toInt() != 1)
	{
		return false;
	}

	passnumber = saved["passnumber"].toInt();
	passdescription = saved["passdescription"].toString();

	durationblock = saved["durationblock"].toInt();

	hashtype=(fourcc)saved["hashtype"].toUInt();
	jtrmode=saved["jtrmode"].toString();

	wordlist_file=saved["wordlist_file"].toString();
	wordlist_count=saved["wordlist_count"].toULongLong();
	encoding = saved["encoding"].toString();
	rule=saved["rule"].toString();
	leet=saved["leet"].toBool();

	num_chars_min=saved["num_chars_min"].toUInt();
	num_chars_max=saved["num_chars_max"].toUInt();
	character_set=saved["character_set"].toString();
	mask = saved["mask"].toString();

	QList<QVariant> vnodes=saved["nodes"].toList();
	foreach(QVariant vnode,vnodes)
	{
		JTRPASSNODE node;
		if (!node.Load(vnode))
		{
			return false;
		}
		nodes.append(node);
	}

	return true;
}


//////////////////////////////
// JTRWORKERCTX

JTRWORKERCTX::JTRWORKERCTX()
{TR;
	m_restore=false;
	m_temporary_dir="";
	m_current_pass = 0;
	m_duration_unlimited = false;
	m_duration_seconds = 0;
	m_elapsed_seconds = 0;
	m_elapsed_seconds_at_start_of_pass = 0;
	
	
	// Create new temp directory
	m_temporary_dir = g_pLinkage->NewTemporaryDir();
}

JTRWORKERCTX::~JTRWORKERCTX()
{TR;
	QDir tempdir(m_temporary_dir);
	tempdir.removeRecursively();
}

void JTRWORKERCTX::Save(QVariant & v)
{TR;
	QMap<QString,QVariant> saved;

	saved["revision"] = 1;

	QList<QVariant> vjtrpasses;
	foreach(JTRPASS jtrpass,m_jtrpasses)
	{
		QVariant vjtrpass;
		jtrpass.Save(vjtrpass);
		vjtrpasses.append(vjtrpass);
	}
	saved["jtrpasses"]=vjtrpasses;
	
	QDir tempdir(m_temporary_dir);
	QStringList filenames=tempdir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	QList<QVariant> tempdircontents;
	foreach(QString filename,filenames)
	{
		QMap<QString,QVariant> tdcentry;
		QFile f(tempdir.filePath(filename));
		if(f.open(QIODevice::ReadOnly))
		{
			tdcentry["filename"]=filename;
			tdcentry["contents"]=f.readAll();
		}

		tempdircontents.append(tdcentry);
	}

	saved["tempdircontents"]=tempdircontents;
	saved["current_pass"]=m_current_pass;

	saved["duration_unlimited"] = m_duration_unlimited;
	saved["duration_seconds"] = m_duration_seconds;
	saved["elapsed_seconds"] = m_elapsed_seconds;
	saved["elapsed_seconds_at_start_of_pass"] = m_elapsed_seconds_at_start_of_pass;
	
	QList<QVariant> dbsl;
	foreach(quint32 secsleft, m_duration_block_seconds_left)
	{
		dbsl.append(secsleft);
	}

	saved["duration_block_seconds_left"] = dbsl;

	saved["current_input_encoding"] = m_current_input_encoding;

	v=saved;
}


#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#define EOL "\r\n"
#else
#define EOL "\n"
#endif

void JTRWORKERCTX::RewriteFilePaths(QString filepath, QByteArray & contents)
{TR;
	QFileInfo fi(filepath);
	QString name=fi.fileName();

	if(name.startsWith("session."))
	{
		QByteArray out;
		QTextStream in(&contents);

		QString header=in.readLine();
		if(header!="REC4")
		{
			Q_ASSERT(0);
		}
		out.append(header.toLatin1()+EOL);
		
		QString argstr=in.readLine();
		int argcount=argstr.toInt();
		out.append(argstr.toLatin1()+EOL);
		
		int arg=0;

		while(!in.atEnd())
		{
			QString line=in.readLine();

			if(arg<(argcount-1))
			{
				if(line.startsWith("--session") || line.startsWith("--pot"))
				{
					// Rewrite file path
					QStringList parts=line.split("=");
					QFileInfo fi(QDir::fromNativeSeparators(parts[1]));
					QDir dir(m_temporary_dir);
					parts[1]=QDir::toNativeSeparators(dir.absoluteFilePath(fi.fileName()));
					line=parts.join("=");
				}
				else if(!line.startsWith("--"))
				{
					QFileInfo fi(QDir::fromNativeSeparators(line));
					QDir dir(m_temporary_dir);
					line=QDir::toNativeSeparators(dir.absoluteFilePath(fi.fileName()));
				}
				arg++;
			}

			out.append(line.toLatin1()+EOL);
		}

		contents=out;
	}
}


bool JTRWORKERCTX::Load(const QVariant & v)
{TR;
	m_restore=true;

	QMap<QString,QVariant> saved=v.toMap();
	if (saved["revision"].toInt() != 1)
	{
		return false;
	}

	QList<QVariant> vjtrpasses=saved["jtrpasses"].toList();
	foreach(QVariant vjtrpass,vjtrpasses)
	{
		JTRPASS jtrpass;
		if (!jtrpass.Load(vjtrpass))
		{
			return false;
		}
		m_jtrpasses.append(jtrpass);
	}

	QDir tempdir(m_temporary_dir);
	
	// Unpack saved contents
	QList<QVariant> tempdircontents=saved["tempdircontents"].toList();
	foreach(QVariant v,tempdircontents)
	{
		QMap<QString,QVariant> tdcentry=v.toMap();
		QString filename=tdcentry["filename"].toString();
		QByteArray contents=tdcentry["contents"].toByteArray();

		RewriteFilePaths(filename, contents);

		QFile out(tempdir.filePath(filename));
		if(out.open(QIODevice::WriteOnly))
		{
			out.write(contents);
		}
	}	
	m_current_pass=saved["current_pass"].toInt();

	m_duration_unlimited = saved["duration_unlimited"].toBool();
	m_duration_seconds = saved["duration_seconds"].toUInt();
	
	m_elapsed_seconds = saved["elapsed_seconds"].toUInt();
	m_elapsed_seconds_at_start_of_pass = saved["elapsed_seconds_at_start_of_pass"].toUInt();

	QList<QVariant> dbsl = saved["duration_block_seconds_left"].toList();
	foreach(QVariant secsleft, dbsl)
	{
		m_duration_block_seconds_left.append(secsleft.toUInt());
	}

	m_current_input_encoding = saved["current_input_encoding"].toUuid();
	
	return true;
}

