#include<stdafx.h>


CLC7JTRCalibrationThread::CLC7JTRCalibrationThread(CLC7JTRCalibrationController *controller, QString algo, QString jtrversion, bool mask, const LC7GPUInfo &gpuinfo)
{
	m_controller = controller;
	m_algo = algo;
	m_jtrversion = jtrversion;
	m_cps = 0;
	m_mask = mask;
	m_exejtr = NULL;
	m_gpuinfo = gpuinfo;
}

CLC7JTRCalibrationThread::~CLC7JTRCalibrationThread()
{

}

void CLC7JTRCalibrationThread::abort(void)
{
	if (m_exejtr)
	{
		m_exejtr->Abort(false);
	}
}

void CLC7JTRCalibrationThread::terminate(void)
{
	if (m_exejtr)
	{
		m_exejtr->Terminate();
	}
}

void CLC7JTRCalibrationThread::run(void)
{
	//setPriority(QThread::HighPriority);

	// Run self tests without benchmark
	QString extra_opencl_kernel_args;
	if (!CLC7JTR::SelfTest(m_jtrversion, m_algo, m_gpuinfo, &extra_opencl_kernel_args))
	{
		return;
	}

	// Run benchmark without self tests
	m_exejtr = new CLC7ExecuteJTR(m_jtrversion);

	QStringList args;
	args << "--test=1";
	if (m_mask)
	{
		args << "--mask=?b?b?b?b";
	}
	//args << "--skip-self-tests";

#ifdef _DEBUG
	args << "--verbosity=5";
#endif

	args << QString("--format=%1").arg(m_algo);
	if (m_gpuinfo.platform!=GPU_NONE && m_gpuinfo.internal_index != -1)
	{
		args << QString("--device=%1").arg(m_gpuinfo.internal_index);
	}
	m_exejtr->SetCommandLine(args, extra_opencl_kernel_args);

	QString out, err;
	int retval = m_exejtr->ExecuteWait(out, err);

	CLC7ExecuteJTR *exejtr = m_exejtr;
	m_exejtr = NULL;
	delete exejtr;

	if (retval != 0)
	{
#ifdef _DEBUG
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Args:" + args.join(" ") + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Retval: " + QString::number(retval) + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Out:\n" + out + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Err:\n" + err + "\n");
#endif

		return;
	}

	foreach(QString line, out.split("\n"))
	{
		line = line.trimmed();

		if (line.endsWith("c/s") && (line.contains("Raw:") || line.contains("Many salts:")))
		{
			QStringList parts = line.split(":");
			QString cpspart = parts.last().trimmed();
			if (cpspart.endsWith(" c/s"))
			{
				cpspart = cpspart.left(cpspart.length() - 4);
				quint64 cps = 1;
				if (cpspart.endsWith("K"))
				{
					cps = 1000;
					cpspart = cpspart.left(cpspart.length() - 1);
				}
				else if (cpspart.endsWith("M"))
				{
					cps = 1000000;
					cpspart = cpspart.left(cpspart.length() - 1);
				}
				else if (cpspart.endsWith("G"))
				{
					cps = 1000000000;
					cpspart = cpspart.left(cpspart.length() - 1);
				}
				bool ok = true;
				quint64 cpsnum;
				cpsnum = cpspart.toULongLong(&ok);
				if (!ok)
				{
					ok = true;
					double cpsnumdbl = cpspart.toDouble(&ok);
					if (!ok)
					{
						cps = 0;
					}
					else
					{
						cps = (quint64)((double)cps)*cpsnumdbl;
					}
				}
				else
				{
					cps *= cpsnum;
				}

				m_cps = cps;
			}
		}
	}

}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CLC7JTRCalibrationController::CLC7JTRCalibrationController(ILC7CalibrationTable *table, QVariant rowId, QVariant colId, QObject *callback_object, ILC7PasswordEngine::CALIBRATION_CALLBACK callback)
{
	m_table = table;
	m_rowId_filter = rowId;
	m_colId_filter = colId;
	m_callback_object = callback_object;
	m_callback = callback;
	m_interrupt_type = INTERRUPT_TYPE::NONE;

	connect(this, &CLC7JTRCalibrationController::sig_calibrationCallback, m_callback_object, m_callback, Qt::BlockingQueuedConnection);

}

CLC7JTRCalibrationController::~CLC7JTRCalibrationController()
{

}

void CLC7JTRCalibrationController::report_started()
{
	ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS args;
	args.activity = ILC7PasswordEngine::CALIBRATION_ACTIVITY::CAL_STARTED;
	args.table = m_table;
	args.rowId = m_rowId_filter;
	args.colId = m_colId_filter;
	emit sig_calibrationCallback(args);
}

void CLC7JTRCalibrationController::report_stopped()
{
	ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS args;
	args.activity = ILC7PasswordEngine::CALIBRATION_ACTIVITY::CAL_STOPPED;
	args.table = m_table;
	args.rowId = m_rowId_filter;
	args.colId = m_colId_filter;
	emit sig_calibrationCallback(args);
}

void CLC7JTRCalibrationController::report_begin_cell()
{
	ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS args;
	args.activity = ILC7PasswordEngine::CALIBRATION_ACTIVITY::CAL_BEGIN_CELL;
	args.table = m_table;
	args.rowId = m_rowId;
	args.colId = m_colId;
	emit sig_calibrationCallback(args);
}

void CLC7JTRCalibrationController::report_end_cell()
{
	ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS args;
	args.activity = ILC7PasswordEngine::CALIBRATION_ACTIVITY::CAL_END_CELL;
	args.table = m_table;
	args.rowId = m_rowId;
	args.colId = m_colId;
	emit sig_calibrationCallback(args);
}


void CLC7JTRCalibrationController::report_results()
{
	ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS args;
	args.activity = ILC7PasswordEngine::CALIBRATION_ACTIVITY::CAL_RESULTS;
	args.table = m_table;
	args.rowId = m_rowId;
	args.colId = m_colId;
	emit sig_calibrationCallback(args);
}

void CLC7JTRCalibrationController::report_error(const QString & details)
{
	ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS args;
	args.activity = ILC7PasswordEngine::CALIBRATION_ACTIVITY::CAL_ERROR;
	args.table = m_table;
	args.rowId = m_rowId;
	args.colId = m_colId;
	args.details = details;
	emit sig_calibrationCallback(args);
}



// Run all the cells in the calibration table
bool CLC7JTRCalibrationController::calibrate(void)
{
	bool ok = true;
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	QString error;

	report_started();

	// Blow away kernels cache
	QDir appdatadir(g_pLinkage->GetCacheDirectory());
	appdatadir.mkdir("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
	appdatadir.cd("lc7jtr{9846c8cf-1db1-467e-83c2-c5655aa81936}");
	appdatadir.mkdir("kernels");
	appdatadir.cd("kernels");
	appdatadir.removeRecursively();

	QVector<QVariant> rowIds = m_table->GetAllCalibrationRowIds();

	for (auto rowId : rowIds)
	{
		if (!m_rowId_filter.isNull() && rowId != m_rowId_filter)
		{
			continue;
		}
		m_rowId = rowId;
		m_colId = QVariant();

		ILC7CalibrationTableRow *row = m_table->GetOrCreateCalibrationTableRow(m_rowId, false);
		if (!row)
		{
			report_error("Unable to get calibration table row");
			ok = false;
			continue;
		}

		fourcc fcc;
//		LC7HashType hashtype;
		bool mask;
		if (!CLC7JTR::DecodeCalibrationRowId(m_rowId, fcc, mask))
		{
			report_error("Unable to decode calibration table row id");
			ok = false;
			continue;
		}
//		if (!passlink->LookupHashType(fcc, hashtype, error))
//		{
//			report_error(error);
//			continue;
//		}

		QVector<QVariant> colIds = row->GetCalibrationColIds();

		for (auto colId : colIds)
		{
			if (!m_colId_filter.isNull() && colId != m_colId_filter)
			{
				continue;
			}
			m_colId = colId;

			ILC7CalibrationTableCell *data = row->GetOrCreateCalibrationCell(m_colId, false);
			if (!data)
			{
				report_error("Unable to get calibration table column");
				ok = false;
				continue;
			}
			
			GPUPLATFORM gpu;
			QString cpuins;
			if (!CLC7JTR::DecodeCalibrationColId(m_colId, gpu, cpuins))
			{
				report_error("Unable to decode calibration table col id");
				ok = false;
				continue;
			}

			report_begin_cell();

			// Run a single calibration table cell
			quint64 cps;
			QMap<int, QString> extra_opencl_kernel_args;
			if (run_calibration_threads(fcc, gpu, cpuins, mask, cps))
			{
				data->SetCPS(cps);
				data->SetValid(true);
				
				report_results();
			}
			else
			{
				data->SetValid(true);
				data->SetCPS(0);
				report_results();
				LC7HashType ht;
				QString error;
				if (!passlink->LookupHashType(fcc, ht, error))
				{
					Q_ASSERT(0);
					return false;
				}
				QString procstr;
				switch (gpu)
				{
				case GPU_NONE:
					procstr = QString("%1:%2").arg(CLC7JTR::GetCPUNodeAlgorithm(fcc)).arg(cpuins);
					break;
				case GPU_OPENCL:
					procstr = QString("%1").arg(CLC7JTR::GetOpenCLNodeAlgorithm(fcc));
					break;
				}
				report_error(QString("Calibration for algorithm '%1, %2, %3' failed.").arg(ht.name).arg(procstr).arg(mask ? "mask" : "nomask"));
				ok = false;
			}

			report_end_cell();

			if (m_interrupt_type != INTERRUPT_TYPE::NONE)
			{
				report_stopped();
				return false;
			}
		}
	}

	report_stopped();
	return ok;
}


bool CLC7JTRCalibrationController::run_calibration_threads(fourcc fcc, GPUPLATFORM gpuplatform, QString cpuins, bool mask, quint64 &cps)
{
	QList<CLC7JTRCalibrationThread *> threads;

	QMap<QString, QVariant> extraconfig = m_table->ExtraConfiguration().toMap();
	QString jtrkernel;

	if (gpuplatform == GPU_NONE)
	{
		int cpucount = extraconfig["cpu_thread_count"].toInt();
		jtrkernel = CLC7JTR::GetCPUNodeAlgorithm(fcc);
		
		for (int i = 0; i < cpucount; i++)
		{
			CLC7JTRCalibrationThread *thread = new CLC7JTRCalibrationThread(this, jtrkernel, cpuins, mask, LC7GPUInfo());

			thread->start();

			threads.append(thread);
		}
	}
	else if (gpuplatform==GPU_OPENCL /*|| gpuplatform==GPU_CUDA*/)
	{
		QVector<LC7GPUInfo> gpuinfo;
		if (!CLC7JTR::DecodeGPUINFOVector(extraconfig["gpuinfo"], gpuinfo))
		{
			return false;
		}

		QString platform;
		if (gpuplatform == GPU_OPENCL)
		{
			jtrkernel = CLC7JTR::GetOpenCLNodeAlgorithm(fcc);
			platform = "OpenCL";
		}
		//else if (gpuplatform == GPU_CUDA)
		//{
		//	algo = CLC7JTR::GetCUDANodeAlgorithm(fcc);
		//	platform = "CUDA";
		//}		

		foreach(LC7GPUInfo gi, gpuinfo)
		{
			if (gi.platform != gpuplatform)
				continue;

			CLC7JTRCalibrationThread *thread = new CLC7JTRCalibrationThread(this, jtrkernel, "sse2", mask, gi);

			thread->start();

			threads.append(thread);
		}
	}

	QSet<CLC7JTRCalibrationThread *> remaining_threads;
	foreach(CLC7JTRCalibrationThread *thread, threads)
	{
		remaining_threads.insert(thread);
	}

	cps = 0;
	QDateTime dtstart = QDateTime::currentDateTime();
	bool any_aborted = false;
	foreach(CLC7JTRCalibrationThread *thread, threads)
	{
		bool aborted = false;
		do
		{
			int secs = dtstart.secsTo(QDateTime::currentDateTime());
			if (secs >= 300)
			{
				thread->terminate();
				foreach(CLC7JTRCalibrationThread *remaining_thread, remaining_threads)
				{
					remaining_thread->terminate();
				}
				return false;
			}

			if (m_interrupt_type!=INTERRUPT_TYPE::NONE)
			{
				if (m_interrupt_type == INTERRUPT_TYPE::ABORT && !aborted)
				{
					aborted = true;
					any_aborted = true;
					thread->abort();
				}
				else if (m_interrupt_type == INTERRUPT_TYPE::TERMINATE)
				{
					thread->terminate();
					foreach(CLC7JTRCalibrationThread *remaining_thread, remaining_threads)
					{
						remaining_thread->terminate();
					}
					return false;
				}
			}
		} while (!thread->wait(250));

		if (!aborted)
		{
			cps += thread->m_cps;
		}

		delete thread;
		remaining_threads.remove(thread);
	}

	if (cps == 0 || any_aborted)
	{
		return false;
	}

	return true;	
}

void CLC7JTRCalibrationController::abort(void)
{
	if (m_interrupt_type == INTERRUPT_TYPE::NONE)
	{
		m_interrupt_type = INTERRUPT_TYPE::ABORT;
	}
}

void CLC7JTRCalibrationController::terminate(void)
{
	if (m_interrupt_type != INTERRUPT_TYPE::TERMINATE)
	{
		m_interrupt_type = INTERRUPT_TYPE::TERMINATE;
	}
}

bool CLC7JTRCalibrationController::is_stopping(void)
{
	return m_interrupt_type != INTERRUPT_TYPE::NONE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


CLC7JTRPasswordEngine::CLC7JTRPasswordEngine() :m_accountlist(nullptr), m_controller(nullptr), m_running(false)
{
	TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7JTRPasswordEngine::NotifySessionActivity);
}

CLC7JTRPasswordEngine::~CLC7JTRPasswordEngine()
{
	TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CLC7JTRPasswordEngine::NotifySessionActivity);
}


void CLC7JTRPasswordEngine::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{
	TR;
	switch (activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = (ILC7AccountList *)handler;
		}
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = NULL;
		}
		break;
	default:
		break;
	}
}

ILC7Interface *CLC7JTRPasswordEngine::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7PasswordEngine")
	{
		return this;
	}
	return nullptr;
}

QUuid CLC7JTRPasswordEngine::GetID()
{
	return UUID_JTRPASSWORDENGINE;
}

QString CLC7JTRPasswordEngine::GetDisplayName()
{
	return "JtR";
}

QString CLC7JTRPasswordEngine::GetDescription()
{
	return "John The Ripper";
}

static QVector<QString> cpupreferenceorder = { "sse2", "ssse3", "sse41", "avx", "xop", "avx2" };

static int compare_cpuins(QString cpu1, QString cpu2)
{
	static bool s_init = true;
	static QMap<QString, int> cpumap;
	if (s_init)
	{
		for (int i = 0; i < cpupreferenceorder.size(); i++)
		{
			cpumap[cpupreferenceorder[i]] = i;
		}
		s_init = false;
	}

	int ncpu1 = cpumap[cpu1];
	int ncpu2 = cpumap[cpu2];

	if (ncpu1 < ncpu2)
	{
		return -1;
	}
	if (ncpu1 > ncpu2)
	{
		return 1;
	}
	return 0;
}

QVariant GetBaselineCPUColId(const QVector<QVariant> &colIds)
{
	for (auto colId : colIds)
	{
		GPUPLATFORM gpu;
		QString cpuins;
		if (!CLC7JTR::DecodeCalibrationColId(colId, gpu, cpuins))
		{
			Q_ASSERT(0);
			continue;
		}

		if (gpu == GPU_NONE)
		{
			if (cpuins == cpupreferenceorder[0])
			{
				return colId;
			}
		}
	}
	Q_ASSERT(0);
	return QVariant();
}

QVariant GetBestCPUColId(const QVector<QVariant> &colIds)
{
	QVariant bestid;
	QString bestcpu;
	for (auto colId : colIds)
	{
		GPUPLATFORM gpu;
		QString cpuins;
		if (!CLC7JTR::DecodeCalibrationColId(colId, gpu, cpuins))
		{
			Q_ASSERT(0);
			continue;
		}

		if (gpu == GPU_NONE)
		{
			if (bestcpu.isEmpty() || compare_cpuins(cpuins, bestcpu)>0)
			{
				bestid = colId;
				bestcpu = cpuins;
			}
		}
	}

	return bestid;
}

QVariant GetBestGPUColId(const QVector<QVariant> &colIds)
{
	QVariant bestid;
	QString bestcpu;
	for (auto colId : colIds)
	{
		GPUPLATFORM gpu;
		QString cpuins;
		if (!CLC7JTR::DecodeCalibrationColId(colId, gpu, cpuins))
		{
			Q_ASSERT(0);
			continue;
		}

		if (gpu == GPU_NONE)
		{
			if (bestcpu.isEmpty() || compare_cpuins(cpuins, bestcpu)>0)
			{
				bestid = colId;
				bestcpu = cpuins;
			}
		}
		else if (gpu==GPU_OPENCL)
		{
			return colId;
		}
	}

	return bestid;
}


QList<ILC7PasswordEngine::CALIBRATION_DEFAULT_SET> CLC7JTRPasswordEngine::GetCalibrationDefaultSets()
{
	QList<ILC7PasswordEngine::CALIBRATION_DEFAULT_SET> sets;

	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	ILC7CalibrationTable *table = passlink->NewCalibrationTable();
	ResetCalibrationTable(table);

	QMap<QString, QVariant> extraconfig = table->ExtraConfiguration().toMap();
	QVector<LC7GPUInfo> gpuinfo;
	if (!CLC7JTR::DecodeGPUINFOVector(extraconfig["gpuinfo"], gpuinfo))
	{
		Q_ASSERT(0);
	}
	QStringList suppins = extraconfig["supported_instruction_sets"].toStringList();
	
	bool has_optimized_cpu = suppins.size() > 1;
	bool has_gpu = gpuinfo.size() > 0;

	CALIBRATION_DEFAULT_SET cpusetbase;
	cpusetbase.name = "CPU Baseline";
	cpusetbase.id = UUID_DEFAULTSET_CPUBASE;
	CALIBRATION_DEFAULT_SET cpusetopt;
	cpusetopt.name = "CPU Optimized";
	cpusetopt.id = UUID_DEFAULTSET_CPUOPT;
	CALIBRATION_DEFAULT_SET gpusetopt;
	gpusetopt.name = "GPU Optimized";
	gpusetopt.id = UUID_DEFAULTSET_GPUOPT;

	for (auto rowId : table->GetAllCalibrationRowIds())
	{
		auto row = table->GetOrCreateCalibrationTableRow(rowId, false);
		fourcc fcc;
		bool mask;
		if (!CLC7JTR::DecodeCalibrationRowId(rowId, fcc, mask))
		{
			Q_ASSERT(0);
			continue;
		}
		
		QVector<QVariant> colIds = row->GetCalibrationColIds();
		QVariant base_cpu_colid = GetBaselineCPUColId(colIds);
		QVariant best_cpu_colid = GetBestCPUColId(colIds);
		QVariant best_gpu_colid = GetBestGPUColId(colIds);
		cpusetbase.default_col_by_row[rowId] = base_cpu_colid;
		cpusetopt.default_col_by_row[rowId] = best_cpu_colid;
		gpusetopt.default_col_by_row[rowId] = best_gpu_colid;
	}

	sets.push_back(cpusetbase);
	if (has_optimized_cpu)
	{
		sets.push_back(cpusetopt);
	}
	if (has_gpu)
	{
		sets.push_back(gpusetopt);
	}

	table->Release();

	return sets;
}



bool CLC7JTRPasswordEngine::hashTypeLessThan(fourcc fcc1, fourcc fcc2)
{
	if (fcc1 == fcc2)
	{
		return false;
	}

	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

	LC7HashType ht1, ht2;
	QString error;
	if (!passlink->LookupHashType(fcc1, ht1, error))
	{
		Q_ASSERT(0);
		return false;
	}
	if (!passlink->LookupHashType(fcc2, ht2, error))
	{
		Q_ASSERT(0);
		return false;
	}

	// Windows first, then Unix
	// then everything else in alphabetical order

	int pn1, pn2;
	if (ht1.platform == "Windows")
	{
		pn1 = 0;
	}
	else if (ht1.platform == "Unix")
	{
		pn1 = 1;
	}
	else
	{
		pn1 = 2;
	}

	if (ht2.platform == "Windows")
	{
		pn2 = 0;
	}
	else if (ht2.platform == "Unix")
	{
		pn2 = 1;
	}
	else
	{
		pn2 = 2;
	}

	if (pn1 != 2 || pn2 != 2)
	{
		return pn1 < pn2;
	}

	if (ht1.platform != ht2.platform)
	{
		return ht1.platform < ht2.platform;
	}

	return ht1.name < ht2.name;
}


void CLC7JTRPasswordEngine::ResetCalibrationTable(ILC7CalibrationTable *table, QUuid default_set_id)
{
	// Make calibration table empty, but layout match the current hashes and hardware
	table->Reset();
	table->SetDefaultSetId(default_set_id);

	// Get CPU Info
	QStringList suppins = CLC7JTR::GetSupportedInstructionSets();
	int cpucount = g_pLinkage->GetSettings()->value(UUID_LC7JTRPLUGIN.toString() + ":cpucount", 1).toInt();

	// Get GPU Info
	QVector<LC7GPUInfo> gpuinfo = CLC7JTR::GetSupportedGPUInfo(false);
	
	// Store as extra configuration in table
	QMap<QString, QVariant> extraconfig;
	extraconfig["cpu_thread_count"] = cpucount;
	extraconfig["supported_instruction_sets"] = suppins;
	extraconfig["gpuinfo"] = CLC7JTR::EncodeGPUINFOVector(gpuinfo);
	table->ExtraConfiguration() = extraconfig;

	// Add all the hash types as calibration rows
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);


	QList<fourcc> sortedhashtypes = passlink->ListHashTypes();
	qSort(sortedhashtypes.begin(), sortedhashtypes.end(), hashTypeLessThan);


	// Do this twice, once for masked, once for not masked
	for (auto fcc : sortedhashtypes)
	{
		bool masked = true;
		do
		{
			masked = !masked;

			// Only calibrate hash types we have importers and techniques for
			QString error;
			LC7HashType hashtype;
			if (!passlink->LookupHashType(fcc, hashtype, error))
			{
				Q_ASSERT(0);
				continue;
			}
			if (!(hashtype.registrants.contains("import") && hashtype.registrants.contains("technique")))
			{
				continue;
			}

			QString cpualg = CLC7JTR::GetCPUNodeAlgorithm(fcc);
			QString oclalg = CLC7JTR::GetOpenCLNodeAlgorithm(fcc);
			bool unmasked_gpu = CLC7JTR::GetSupportsUnmaskedGPU(fcc);
		
			QVariant row_id = CLC7JTR::EncodeCalibrationRowId(fcc, masked);
			ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(row_id, true);
			if (!cpualg.isEmpty())
			{
				for (auto ins : suppins)
				{
					QVariant col_id = CLC7JTR::EncodeCalibrationColId(GPU_NONE, ins);
					row->GetOrCreateCalibrationCell(col_id, true);
				}
			}
			if (!oclalg.isEmpty() && (masked || unmasked_gpu))
			{
				bool has_opencl = false;
				//bool has_cuda = false;
				for (auto g : gpuinfo)
				{
					if (g.platform == GPU_OPENCL)
					{
						has_opencl = true;

					}
					//else if (g.platform == GPU_CUDA)
					//{
					//	has_cuda = true;
					//}
				}
				if (has_opencl)
				{
					QVariant col_id = CLC7JTR::EncodeCalibrationColId(GPU_OPENCL, QString());
					row->GetOrCreateCalibrationCell(col_id, true);
				}
			}
		} while (!masked);
	}

	// Fill in default set
	if (!default_set_id.isNull())
	{
		QList<CALIBRATION_DEFAULT_SET> default_sets = GetCalibrationDefaultSets();
		for (auto & default_set: default_sets)
		{
			if (default_set.id == default_set_id)
			{
				for(auto rowId : default_set.default_col_by_row.keys())
				{
					ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
					if (!row)
					{
						Q_ASSERT(0);
						continue;
					}
					QVariant colId = default_set.default_col_by_row[rowId];
					row->SetPreferredColId(colId);
				}

				return;
			}
		}
	}
}

bool CLC7JTRPasswordEngine::GetCalibrationRowInfo(ILC7CalibrationTable *table, QVariant rowId, CALIBRATION_ROW_INFO &rowInfo, QString &error)
{
	ILC7CalibrationTableRow *row = table->GetOrCreateCalibrationTableRow(rowId, false);
	if (!row)
	{
		error = "Row does not exist in calibration table";
		return false;
	}

	fourcc fcc;
	bool mask;
	if (!CLC7JTR::DecodeCalibrationRowId(rowId, fcc, mask))
	{
		error = "Unable to decode calibration row id";
		return false;
	}

	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	LC7HashType hashtype;
	if (!passlink->LookupHashType(fcc, hashtype, error))
	{
		return false;
	}

	rowInfo.engine = "JTR";
	rowInfo.platform = hashtype.platform;
	rowInfo.hashtype = hashtype.name;
	rowInfo.audittype = (mask ? "Brute Force" : "Dictionary");
	rowInfo.description = hashtype.description + (mask ? "(brute force mode)" : " (dictionary mode)");
			
	return true;
}


bool CLC7JTRPasswordEngine::GetCalibrationColInfo(ILC7CalibrationTable *table, QVariant colId, CALIBRATION_COL_INFO &colInfo, QString &error)
{
	GPUPLATFORM gpu;
	QString cpuins;
	if (!CLC7JTR::DecodeCalibrationColId(colId, gpu, cpuins))
	{
		error = "Unable to decode calibration column id";
		return false;
	}

	if (gpu == GPU_NONE)
	{
		colInfo.name = cpuins.toUpper();
		colInfo.description = colInfo.name + " Instruction Set (CPU)";
	}
	else if (gpu == GPU_OPENCL)
	{
		colInfo.name = "GPU/OpenCL";
		colInfo.description = colInfo.name + " (Hardware Accelerated)";
	}

	return true;
}

bool CLC7JTRPasswordEngine::RunCalibration(ILC7CalibrationTable *table, QVariant rowId, QVariant colId, QObject *callback_object, CALIBRATION_CALLBACK callback)
{
	{
		QMutexLocker lock(&m_mutex);
		if (m_running)
		{
			return false;
		}
		m_controller = new CLC7JTRCalibrationController(table, rowId, colId, callback_object, callback);
		m_running = true;
	}

	bool ok = m_controller->calibrate();

	{
		QMutexLocker lock(&m_mutex);
		m_running = false;
		delete m_controller;
		m_controller = nullptr;
	}

	return ok;
}

void CLC7JTRPasswordEngine::StopCalibration(bool force)
{
	QMutexLocker lock(&m_mutex);
	if (!m_running)
	{
		return;
	}
	Q_ASSERT(m_controller);

	if (!force)
	{
		m_controller->abort();
	}
	else
	{
		m_controller->terminate();
	}
}

bool CLC7JTRPasswordEngine::IsCalibrationRunning()
{
	QMutexLocker lock(&m_mutex);
	return m_running;
}

bool CLC7JTRPasswordEngine::IsCalibrationStopping()
{
	QMutexLocker lock(&m_mutex);
	if (!m_running)
	{
		return false;
	}
	Q_ASSERT(m_controller);
	return m_controller->is_stopping();
}

QString CLC7JTRPasswordEngine::GetCalibrationKey(void)
{
	return CLC7JTR::CalibrationKey();
}

ILC7GPUManager *CLC7JTRPasswordEngine::GetGPUManager(void)
{
	return &m_gpu_manager;
}


ILC7Component::RETURNCODE CLC7JTRPasswordEngine::Crack(QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{
	CLC7JTR jtr(this, m_accountlist, ctrl);

	if (!jtr.Configure(config))
	{
		error = QString("The technique configuration is invalid: %1").arg(jtr.LastError());
		return ILC7Component::FAIL;
	}

	jtr.StartCracking();

	bool success = false;
	bool stopped = false;
	QDateTime last_time = QDateTime::currentDateTime();
	QDateTime checkpoint_time = QDateTime::currentDateTime();
	while (1)
	{
		if (jtr.CheckCrackingFinished(success))
		{
			break;
		}

		if (ctrl && (ctrl->StopRequested() || ctrl->PauseRequested()))
		{
			jtr.StopCracking();
			jtr.ProcessStatus();
			jtr.Cleanup();

			config = jtr.GetConfig();

			if (ctrl->StopRequested())
			{
				config.remove("stopped_context");
				return ILC7Component::STOPPED;
			}
			return ILC7Component::PAUSED;
		}

		QDateTime current_time = QDateTime::currentDateTime();
		if (last_time.secsTo(current_time) >= 1)
		{
			last_time = current_time;
			jtr.ProcessStatus();
		}
		if (checkpoint_time.secsTo(current_time) >= 10)
		{
			// Report checkpoint upstream every 10 seconds
			checkpoint_time = current_time;
			QMap<QString, QVariant> checkpoint_config = jtr.GetCheckpointConfig();
			ctrl->SaveCheckpointConfig(checkpoint_config);
		}
		QThread::msleep(250);
	}

	jtr.ProcessStatus();

	ctrl->SetStatusText("Done");
	ctrl->UpdateCurrentProgressBar(100);

	if (!success)
	{
		error = jtr.LastError();
		jtr.Cleanup();

		config = jtr.GetConfig();
		return ILC7Component::FAIL;
	}

	jtr.Cleanup();

	config = jtr.GetConfig();
	return ILC7Component::SUCCESS;
}

bool CLC7JTRPasswordEngine::ValidateCrack(QMap<QString, QVariant> & state, QMap<QString, QVariant> & config, QString & error)
{
	if (!state.contains("hashtypes"))
	{
		error = "No hashes are imported.";
		return false;
	}

	QList<QVariant> hashtypes = state["hashtypes"].toList();
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

	foreach(QVariant htv, hashtypes)
	{
		fourcc fcc = (fourcc)htv.toUInt();
		LC7HashType lc7hashtype;
		QString hterror;
		if (!passlink->LookupHashType(fcc, lc7hashtype, hterror) || !lc7hashtype.registrants["technique"].contains(UUID_LC7JTRPLUGIN))
		{
			error = "Incompatible hash type for this audit technique.";
			return false;
		}
	}

	state["cracked"] = true;
	return true;
}
