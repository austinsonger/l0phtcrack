#include<stdafx.h>


//////////////////////////////////////////////////////////////////////////////////////////

CLC7CalibrationThread::CLC7CalibrationThread(QString algo, QString jtrversion, int jtrindex, bool mask, GPUPLATFORM platform, QString vendor)
{
	m_algo = algo;
	m_jtrversion = jtrversion;
	m_jtrindex = jtrindex;
	m_cps = 0;
	m_mask = mask;
	m_exejtr = NULL;
	m_platform = platform;
	m_vendor = vendor;
}

CLC7CalibrationThread::~CLC7CalibrationThread()
{

}

void CLC7CalibrationThread::abort(void)
{
	if (m_exejtr)
	{
		m_exejtr->Abort(false);
	}
}

void CLC7CalibrationThread::terminate(void)
{
	if (m_exejtr)
	{
		m_exejtr->Terminate();
	}
}

bool CLC7CalibrationThread::selftest(QString extra_kernel_args)
{
	bool passes_self_test = true;

	m_exejtr = new CLC7ExecuteJTR(m_jtrversion);

	QStringList args;
	args << "--test=0";

	args << QString("--format=%1").arg(m_algo);
	if (m_jtrindex != -1)
	{
		args << QString("--device=%1").arg(m_jtrindex);
	}
#ifdef _DEBUG
	args << "--verbosity=5";
#endif
	m_exejtr->SetCommandLine(args, extra_kernel_args);

	QString out, err;
	int retval = m_exejtr->ExecuteWait(out, err);

	if (retval != 0)
	{
#ifdef _DEBUG
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Args:" + args.join(" ") + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Retval: " + QString::number(retval) + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Out:\n" + out + "\n");
		g_pLinkage->GetGUILinkage()->AppendToActivityLog("Err:\n" + err + "\n");
#endif
		passes_self_test = false;
	}

	CLC7ExecuteJTR *exejtr = m_exejtr;
	m_exejtr = NULL;
	delete exejtr;

	return passes_self_test;
}

void CLC7CalibrationThread::run(void)
{
	//setPriority(QThread::HighPriority);

	// Run self tests without benchmark
	bool passes_self_test = false;
	QString extra_opencl_kernel_args;
	if (selftest(extra_opencl_kernel_args))
	{
		passes_self_test = true;
	}
	else if (m_platform == GPU_OPENCL && 
			(m_vendor.contains("Advanced Micro") || m_vendor.contains("AMD") || m_vendor.contains("ATI")))
	{
		extra_opencl_kernel_args = "-O1";
		if (selftest(extra_opencl_kernel_args))
		{
			passes_self_test = true;
		}
	}

	// Run benchmark without self tests
	if (passes_self_test)
	{
		m_exejtr = new CLC7ExecuteJTR(m_jtrversion);

		QStringList args;
		args << "--test=1";
		if (m_mask)
		{
			args << "--mask=?b?b?b?b";
		}
		args << "--skip-self-tests";

#ifdef _DEBUG
		args << "--verbosity=5";
#endif

		args << QString("--format=%1").arg(m_algo);
		if (m_jtrindex != -1)
		{
			args << QString("--device=%1").arg(m_jtrindex);
		}
		m_exejtr->SetCommandLine(args, extra_opencl_kernel_args);


		QString out, err;
		int retval = m_exejtr->ExecuteWait(out, err);

#ifdef _DEBUG
		if (retval != 0)
		{
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Args:" + args.join(" ") + "\n");
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Retval: " + QString::number(retval) + "\n");
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Out:\n" + out + "\n");
			g_pLinkage->GetGUILinkage()->AppendToActivityLog("Err:\n" + err + "\n");
		}
#endif

		if (retval == 0)
		{
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
						m_extra_opencl_kernel_args = extra_opencl_kernel_args;
					}
				}
			}
		}

		CLC7ExecuteJTR *exejtr = m_exejtr;
		m_exejtr = NULL;
		delete exejtr;
	}

}

//////////////////////////////////////////////////////////////////////////////////////////


CLC7CalibrationData::CLC7CalibrationData()
{
	m_jtrkernel="";
	m_cps=0;
}

CLC7CalibrationData::CLC7CalibrationData(const CLC7CalibrationData &copy)
{
	m_jtrkernel = copy.m_jtrkernel;
	m_cps = copy.m_cps;
	m_extra_opencl_kernel_args = copy.m_extra_opencl_kernel_args;
}

quint64 CLC7CalibrationData::CPS() const
{
	return m_cps;
}

void CLC7CalibrationData::setCPS(quint64 cps)
{
	m_cps = cps;
}

QString CLC7CalibrationData::JTRKernel() const
{
	return m_jtrkernel;
}

void CLC7CalibrationData::setJTRKernel(QString jtrkernel)
{
	m_jtrkernel = jtrkernel;
}

QMap<int, QString> CLC7CalibrationData::ExtraOpenCLKernelArgs() const
{
	return m_extra_opencl_kernel_args;
}

void CLC7CalibrationData::setExtraOpenCLKernelArgs(QMap<int, QString> extra_opencl_kernel_args)
{
	m_extra_opencl_kernel_args = extra_opencl_kernel_args;
}

QDataStream &operator<<(QDataStream &out, const CLC7CalibrationData &data)
{
	out << data.m_jtrkernel;
	out << data.m_cps;
	out << data.m_extra_opencl_kernel_args;

	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7CalibrationData &data)
{
	in >> data.m_jtrkernel;
	in >> data.m_cps;
	in >> data.m_extra_opencl_kernel_args;

	return in;
}

//////////////////////////////////////////////////////////////////////////////////////////

CLC7CalibrationHashType::CLC7CalibrationHashType()
{
	m_hashtype = 0;
}

CLC7CalibrationHashType::CLC7CalibrationHashType(fourcc hashtype)
{
	m_hashtype = hashtype;
}

CLC7CalibrationHashType::CLC7CalibrationHashType(const CLC7CalibrationHashType & ht)
{
	m_hashtype = ht.m_hashtype;
}

bool CLC7CalibrationHashType::operator==(const CLC7CalibrationHashType & other) const
{
	return m_hashtype == other.m_hashtype;
}

bool CLC7CalibrationHashType::operator<(const CLC7CalibrationHashType & other) const
{
	return m_hashtype < other.m_hashtype;
}

fourcc CLC7CalibrationHashType::hashtype() const
{
	return m_hashtype;
}


QDataStream &operator<<(QDataStream &out, const CLC7CalibrationHashType &ht)
{
	out << ht.m_hashtype;
	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7CalibrationHashType &ht)
{
	in >> ht.m_hashtype;
	return in;
}


//////////////////////////////////////////////////////////////////////////////////////////



CLC7CalibrationProcessorType::CLC7CalibrationProcessorType()
{
	m_gpuplatform = GPU_NONE;
	m_cpuinstructionset = "";
}

CLC7CalibrationProcessorType::CLC7CalibrationProcessorType(QString cpuinstructionset)
{
	m_gpuplatform = GPU_NONE;
	m_cpuinstructionset = cpuinstructionset;
}

CLC7CalibrationProcessorType::CLC7CalibrationProcessorType(GPUPLATFORM gpuplatform)
{
	m_gpuplatform = gpuplatform;
	m_cpuinstructionset = "";
}

CLC7CalibrationProcessorType::CLC7CalibrationProcessorType(const CLC7CalibrationProcessorType & copy)
{
	m_gpuplatform = copy.m_gpuplatform;
	m_cpuinstructionset = copy.m_cpuinstructionset;
}

bool CLC7CalibrationProcessorType::operator==(const CLC7CalibrationProcessorType & other) const
{
	return (m_gpuplatform == other.m_gpuplatform) &&
		(m_cpuinstructionset == other.m_cpuinstructionset);
}

bool CLC7CalibrationProcessorType::operator<(const CLC7CalibrationProcessorType & other) const
{
	if (m_gpuplatform < other.m_gpuplatform)
	{
		return true;
	}
	if (m_gpuplatform > other.m_gpuplatform)
	{
		return false;
	}
	return m_cpuinstructionset < other.m_cpuinstructionset;
}

GPUPLATFORM CLC7CalibrationProcessorType::gpuplatform() const
{
	return m_gpuplatform;
}

QString CLC7CalibrationProcessorType::cpuinstructionset() const
{
	return m_cpuinstructionset;
}


QDataStream &operator<<(QDataStream &out, const CLC7CalibrationProcessorType &pt)
{
	out << pt.m_gpuplatform;
	out << pt.m_cpuinstructionset;
	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7CalibrationProcessorType &pt)
{
	in >> (int&)pt.m_gpuplatform;
	in >> pt.m_cpuinstructionset;
	return in;
}

//////////////////////////////////////////////////////////////////////////////////////////


CLC7Calibration::CLC7CalibrationKey::CLC7CalibrationKey()
{
}

CLC7Calibration::CLC7CalibrationKey::CLC7CalibrationKey(bool mask, const CLC7CalibrationHashType &hashtype, const CLC7CalibrationProcessorType &proctype)
{
	m_mask = mask;
	m_hashtype = hashtype;
	m_proctype = proctype;
}

CLC7Calibration::CLC7CalibrationKey::CLC7CalibrationKey(const CLC7CalibrationKey & copy)
{
	m_mask = copy.m_mask;
	m_hashtype = copy.m_hashtype;
	m_proctype = copy.m_proctype;
}

bool CLC7Calibration::CLC7CalibrationKey::operator<(const CLC7Calibration::CLC7CalibrationKey & other) const
{
	if (m_mask == false && other.m_mask == true)
	{
		return true;
	}
	if (m_mask == true && other.m_mask == false)
	{
		return false;
	}
	if (m_hashtype < other.m_hashtype)
	{
		return true;
	}
	if (other.m_hashtype < m_hashtype)
	{
		return false;
	}
	return m_proctype < other.m_proctype;
}

QDataStream &operator<<(QDataStream &out, const CLC7Calibration::CLC7CalibrationKey &key)
{
	out << key.m_mask;
	out << key.m_hashtype;
	out << key.m_proctype;
	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7Calibration::CLC7CalibrationKey &key)
{
	in >> key.m_mask;
	in >> key.m_hashtype;
	in >> key.m_proctype;
	return in;
}


//////////////////////////////////////////////////////////////////////////////////////////



CLC7Calibration::CLC7Calibration()
{
	m_cpu_thread_count = 0;
	m_is_valid = false;
}

bool CLC7Calibration::isValid() const
{
	return m_is_valid;
}

void CLC7Calibration::setValid(bool valid)
{
	m_is_valid = valid;
}


// Configuration
bool CLC7Calibration::configurationMatch(const CLC7Calibration & other) const
{
	// Compare everything but data, and ensure everything is valid
	return
		(m_available_cputypes == other.m_available_cputypes) &&
		(m_cpu_thread_count == other.m_cpu_thread_count) &&
		(m_gpuinfo == other.m_gpuinfo) &&
		(m_hashtypes == other.m_hashtypes) &&
		(m_proctypes == other.m_proctypes)
		;
}

QList<CLC7CalibrationHashType> CLC7Calibration::hashTypes() const
{
	return m_hashtypes;
}

QList<CLC7CalibrationProcessorType> CLC7Calibration::processorTypes() const
{
	return m_proctypes;
}

QStringList CLC7Calibration::availableCPUTypes() const
{
	return m_available_cputypes;
}

int CLC7Calibration::CPUThreadCount() const
{
	return m_cpu_thread_count;
}

QVector<GPUINFO> CLC7Calibration::GPUInfo() const
{
	return m_gpuinfo;
}

void CLC7Calibration::addHashType(const CLC7CalibrationHashType & hashtype)
{
	if (m_hashtypes.contains(hashtype))
	{
		Q_ASSERT(0);
		return;
	}
	m_hashtypes.prepend(hashtype);
}

void CLC7Calibration::addProcessorType(const CLC7CalibrationProcessorType & proctype)
{
	if (m_proctypes.contains(proctype))
	{
		Q_ASSERT(0);
		return;
	}

	m_proctypes.append(proctype);
}

void CLC7Calibration::setAvailableCPUTypes(QStringList available_cputypes)
{
	m_available_cputypes = available_cputypes;
}

void CLC7Calibration::setCPUThreadCount(int cputhreadcount)
{
	m_cpu_thread_count = cputhreadcount;
}

void CLC7Calibration::setGPUInfo(QVector<GPUINFO> gpuinfo)
{
	m_gpuinfo = gpuinfo;
}

// Calibration data
void CLC7Calibration::clearCalibrationData(void)
{
	m_is_valid = false;
	m_calibrationdata.clear();
	m_preferred_methods[0].clear();
	m_preferred_methods[1].clear();
}

void CLC7Calibration::setCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype, const CLC7CalibrationData &value)
{
	CLC7CalibrationKey key(mask, hashtype, proctype);
	m_calibrationdata[key] = value;
}

bool CLC7Calibration::hasCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype) const
{
	CLC7CalibrationKey key(mask, hashtype, proctype);
	return m_calibrationdata.contains(key);
}

bool CLC7Calibration::getCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype, CLC7CalibrationData & value) const
{
	CLC7CalibrationKey key(mask, hashtype, proctype);
	if (!m_calibrationdata.contains(key))
	{
		return false;
	}

	value = m_calibrationdata[key];
	return true;
}

// Selected method
void CLC7Calibration::setPreferredMethod(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype)
{
	m_preferred_methods[mask?1:0][hashtype] = proctype;
}

bool CLC7Calibration::getPreferredMethod(bool mask, const CLC7CalibrationHashType & hashtype, CLC7CalibrationProcessorType &proctype) const
{
	if (!m_preferred_methods[mask ? 1 : 0].contains(hashtype))
		return false;

	proctype = m_preferred_methods[mask ? 1 : 0][hashtype];
	return true;
}

void CLC7Calibration::saveCalibration(const CLC7Calibration &cal)
{
	// Only save valid calibrations
	if (!cal.isValid())
	{
		Q_ASSERT(0);
		return;
	}

	// See if loaded calibration meets current system configuration requirements
	ILC7Settings *settings = g_pLinkage->GetSettings();

#if PLATFORM == PLATFORM_WIN32
	QString calkey = QString("%1:calibration_win32").arg(UUID_LC7JTRPLUGIN.toString());
#elif PLATFORM == PLATFORM_WIN64
	QString calkey = QString("%1:calibration_win64").arg(UUID_LC7JTRPLUGIN.toString());
#else
#error "key plz"
#endif

	QByteArray calba;
	QDataStream calds(&calba, QIODevice::WriteOnly);;
	calds << cal;

	settings->setValue(calkey, calba);
	settings->sync();
}

void CLC7Calibration::loadCalibration(CLC7Calibration & cal)
{
	// See if loaded calibration meets current system configuration requirements
	ILC7Settings *settings = g_pLinkage->GetSettings();

#if PLATFORM == PLATFORM_WIN32
	QString calkey = QString("%1:calibration_win32").arg(UUID_LC7JTRPLUGIN.toString());
#elif PLATFORM == PLATFORM_WIN64
	QString calkey = QString("%1:calibration_win64").arg(UUID_LC7JTRPLUGIN.toString());
#else
#error "key plz"
#endif
	if (settings->contains(calkey))
	{
		QByteArray calba;
		calba = settings->value(calkey).toByteArray();

		QDataStream calds(calba);

		calds >> cal;
	}
}

void CLC7Calibration::emptyCalibration(CLC7Calibration & cal)
{
	cal = CLC7Calibration();

	ILC7CPUInformation *cpuinfo = g_pLinkage->GetCPUInformation();

	CLC7GPUInfo gpuinfo;
	gpuinfo.Detect();

	int corecount = g_pLinkage->GetSettings()->value(QString("%1:cpucount").arg(UUID_LC7JTRPLUGIN.toString()), cpuinfo->CoreCount()).toInt();
	QStringList suppins = CLC7JTR::GetSupportedInstructionSets();

	// Configuration
	cal.setGPUInfo(gpuinfo.GetGPUInfo());
	cal.setCPUThreadCount(corecount);
	cal.setAvailableCPUTypes(suppins);
	
	// Add hashes
	ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	QList<fourcc> accttypes = passlink->List();
	foreach(fourcc fcc, accttypes)
	{
		LC7HashType lc7hashtype;
		QString error;
		if (!passlink->Lookup(fcc, lc7hashtype, error))
		{
			Q_ASSERT(0);
			continue;
		}

		// Add only the ones supported by JtR that also have importers
		if (lc7hashtype.registrants["technique"].contains(UUID_LC7JTRPLUGIN) && lc7hashtype.registrants["import"].size() > 0)
		{
			cal.addHashType(fcc);
		}
	}

	// Add CPUs
	foreach(QString cputype, suppins)
	{
		cal.addProcessorType(CLC7CalibrationProcessorType(cputype));
	}

	// Add GPUs
	bool has_opencl = false;
	//bool has_cuda = false;
	foreach(GPUINFO gi, gpuinfo.GetGPUInfo())
	{
		QString platform;
		if (gi.platform == GPU_OPENCL)
		{
			platform = "OpenCL";
		}
		//else if (gi.platform == GPU_CUDA)
		//{
		//	platform = "CUDA";
		//}

		bool enabled = g_pLinkage->GetSettings()->value(UUID_LC7JTRPLUGIN.toString() + QString(":enablegpu_%1_%2").arg(platform).arg(gi.jtrindex), true).toBool();
		if (enabled)
		{
			if (gi.platform == GPU_OPENCL)
			{
				has_opencl = true;
			}

			//if (gi.platform == GPU_CUDA)
			//{
			//	has_cuda = true;
			//}
		}
	}

	if (has_opencl)
	{
		cal.addProcessorType(GPU_OPENCL);
	}
	//if (has_cuda)
	//{
	//	cal.addProcessorType(GPU_CUDA);
	//}
}


bool CLC7Calibration::GetHashTypePreferredProcessor(bool mask, fourcc hashtype, GPUPLATFORM & gpuplatform, QString & cpuinstructionset, QString &jtrkernel, QMap<int, QString> & extra_opencl_kernel_args) const
{
	if (!isValid())
	{
		return false;
	}

	CLC7CalibrationProcessorType cpt;
	if (!getPreferredMethod(mask, hashtype, cpt))
	{
		return false;
	}

	CLC7CalibrationData value;
	if (!getCalibrationData(mask, hashtype, cpt, value))
	{
		return false;
	}

	gpuplatform = cpt.gpuplatform();
	cpuinstructionset = cpt.cpuinstructionset();
	jtrkernel = value.JTRKernel();
	extra_opencl_kernel_args = value.ExtraOpenCLKernelArgs();

	return true;
}


// Serialization
QDataStream &operator<<(QDataStream &out, const CLC7Calibration &cal)
{
	quint32 version = 2;
	out << version;

	out << cal.m_is_valid;

	out << cal.m_hashtypes;
	out << cal.m_proctypes;
	out << cal.m_calibrationdata;
	out << cal.m_preferred_methods[0];
	out << cal.m_preferred_methods[1];

	out << cal.m_available_cputypes;
	out << cal.m_cpu_thread_count;
	out << cal.m_gpuinfo;

	return out;
}

QDataStream &operator>>(QDataStream &in, CLC7Calibration &cal)
{
	quint32 version;
	in >> version;
	if (version == 1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}
	if (version > 2)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}

	in >> cal.m_is_valid;

	in >> cal.m_hashtypes;
	in >> cal.m_proctypes;
	in >> cal.m_calibrationdata;
	in >> cal.m_preferred_methods[0];
	in >> cal.m_preferred_methods[1];

	in >> cal.m_available_cputypes;
	in >> cal.m_cpu_thread_count;
	in >> cal.m_gpuinfo;

	return in;
}
