#include"stdafx.h"

CSystemJTR::CSystemJTR()
{TR;
}

CSystemJTR::~CSystemJTR()
{TR;
}

ILC7Interface *CSystemJTR::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CSystemJTR::GetID()
{TR;
	return UUID_SYSTEMJTR;
}

void CSystemJTR::AddOption(QList<QVariant> & keys,
						   QString settingskey,
						   QString name,
						   QString desc,
						   QVariant default_value,
						   bool require_restart,
						   QString option1key, QVariant option1value,
						   QString option2key, QVariant option2value,
						   QString option3key, QVariant option3value,
						   QString option4key, QVariant option4value,
						   QString option5key, QVariant option5value)
{TR;
	QMap<QString, QVariant> options;
	options["name"]=name;
	options["desc"]=desc;
	options["default"]=default_value;
	options["type"]=QString(default_value.typeName());
	options["settingskey"]=settingskey;
	options["require_restart"] = require_restart;
	if(!option1key.isEmpty())
	{
		options[option1key]=option1value;
	}
	if(!option2key.isEmpty())
	{
		options[option2key]=option2value;
	}
	if(!option3key.isEmpty())
	{
		options[option3key]=option3value;
	}
	if(!option4key.isEmpty())
	{
		options[option4key]=option4value;
	}
	if(!option5key.isEmpty())
	{
		options[option5key]=option5value;
	}

	keys.append(options);
}



bool CSystemJTR::GetOptions(QMap<QString, QVariant> & config, QString & error)
{TR;
	QList<QVariant> keys;

	ILC7CPUInformation *cpuid = g_pLinkage->GetCPUInformation();
	bool has_avx2 = cpuid->AVX() && cpuid->AVX2() && cpuid->XSAVE() && cpuid->OSXSAVE() && cpuid->XMM_SAVED() && cpuid->YMM_SAVED() && cpuid->MOVBE() && cpuid->FMA();


	/*
	// Prefer OpenCL over CUDA
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString()+":prefercuda",
		"Prefer CUDA over OpenCL", 
		"When a choice between CUDA and OpenCL algorithms exists, prefer the CUDA algorithm.",  
		true,
		false);

	// Enable CPU with GPU
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString()+":enablecpuwithgpu",
		"Enable CPU with GPU", 
		"When using GPU cracking, also enable CPU cracking. This may take longer to complete overall, depending on the number of CPU cores available and their clock speed.",  
		false,
		false);

	// Prefer CPU for LM Hashes
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":prefercpuforlm",
		"Prefer CPU for LM Hashes",
		"When using GPU cracking, continue to use CPU cracking for LM hashes, but use GPU for NTLM hashes. LM Hashes are faster on the CPU in most configurations.",
		false,
		false);
	*/

	// Enable SSE2
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":enablesse2",
		"Enable SSE2 Instruction Set",
		"Allow SSE2 instructions to be used in CPU cracking cores if available.",
		true,
		false);

	// Enable SSSE3
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":enablessse3",
		"Enable SSSE3 Instruction Set",
		"Allow SSSE3 instructions to be used in CPU cracking cores if available.",
		false,
		false);

	// Enable SSE41
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":enablesse41",
		"Enable SSE4.1 Instruction Set",
		"Allow SSE4.1 instructions to be used in CPU cracking cores if available.",
		false,
		false);

#ifdef _WIN64
	// Enable AVX
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":enableavx",
		"Enable AVX Instruction Set",
		"Allow Intel AVX instructions to be used on CPU cracking cores if available.",
		!has_avx2,
		false);

	// Enable XOP
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":enablexop",
		"Enable XOP Instruction Set",
		"Allow AMD XOP instructions to be used on CPU cracking cores if available.",
		true,
		false);

	// Enable AVX
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":enableavx2",
		"Enable AVX2 Instruction Set",
		"Allow Intel AVX2 instructions to be used on CPU cracking cores if available.",
		true,
		false);
#endif

	// CPU count to use
	int maxcpucount = g_pLinkage->GetCPUInformation()->CoreCount();

	AddOption(keys, 
		UUID_LC7JTRPLUGIN.toString()+":cpucount",
		"CPU Thread Count", 
		"Number of CPU cores/threads to utilize for cracking",
		maxcpucount,
		false,
		"minimum",1,
		"maximum",maxcpucount);


	// Enable unsupported GPUs
	AddOption(keys,
		UUID_LC7JTRPLUGIN.toString() + ":enableunsupportedgpus",
		"Enable Unsupported GPUs",
		"Some GPUs are not officially supported by LC7. If you enable this option you can try them, but you may get hangs/crashes or erratic behavior.",
		false,
		true);

	CLC7GPUInfo gpuinfo;
	gpuinfo.Detect();
	QVector<GPUINFO> gi=gpuinfo.GetGPUInfo();

	foreach(GPUINFO g,gi)
	{
		QString platform;
		if(g.platform==GPU_OPENCL)
		{
			platform="OpenCL";
		}
		//else if(g.platform==GPU_CUDA)
		//{
		//		platform="CUDA";
		//}

		// Add enable gpu
		AddOption(keys, 
			UUID_LC7JTRPLUGIN.toString()+QString(":enablegpu_%1_%2").arg(platform).arg(g.jtrindex),
			QString("Enable %1 GPU #%2 (%3, %4)").arg(platform).arg(g.jtrindex).arg(g.name).arg(g.vendor),
			QString("Use this graphics processor for auditing algorithms. Can perform much faster than CPU if you have a good graphics card."), 
			true,
			false);
	}
	
	config["keys"]=keys;


	return true;
}

ILC7Component::RETURNCODE CSystemJTR::ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="get_options")
	{
		if(!GetOptions(config,error))
		{
			return FAIL;
		}
		return SUCCESS;
	}
	if (command == "calibrate")
	{
		CLC7Calibrate cal(NULL);
		g_pLinkage->GetGUILinkage()->ShadeUI(true);
		int ret = cal.exec();
		g_pLinkage->GetGUILinkage()->ShadeUI(false);

		return SUCCESS;
	}
	return FAIL;
}

bool CSystemJTR::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	return true;
}

