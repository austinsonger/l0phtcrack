#include"stdafx.h"
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
#include<io.h>
#include<fcntl.h>
#endif


CLC7GPUInfo::CLC7GPUInfo()
{
	TR;
}

bool CLC7GPUInfo::ExecuteJTR(QStringList args, QString & out, QString &err, int &retval)
{
	TR;
	CLC7ExecuteJTR exejtr("sse2");
	exejtr.SetCommandLine(args);
	retval = exejtr.ExecuteWait(out, err);
	return retval == 0;
}

bool CLC7GPUInfo::IsSupportedGPU(const GPUINFO & gi)
{
	TR;
	bool enable_unsupported = g_pLinkage->GetSettings()->value(QString("%1:enableunsupportedgpus").arg(UUID_LC7JTRPLUGIN.toString()), false).toBool();
	if (enable_unsupported)
	{
		return true;
	}

	if (gi.vendor.contains("Advanced Micro") || gi.vendor.contains("AMD") || gi.vendor.contains("ATI"))
	{
		return true;
	}
	if (gi.vendor.contains("NVIDIA"))
	{
		return true;
	}

	return false;
}

void CLC7GPUInfo::DetectOPENCL(void)
{
	TR;
	QStringList openclargs;
	openclargs.append("--list=opencl-devices");

	QString opencl_out;
	QString opencl_err;
	int opencl_retval;

	if (ExecuteJTR(openclargs, opencl_out, opencl_err, opencl_retval))
	{
		// Add opencl devices
		QRegExp re_device("\\s*Device #[0-9]+\\s*\\(([0-9]+)\\)\\s*name:\\s*([^$]*)$");
		QRegExp re_vendor("\\s*Device vendor:\\s*([^$]*)$");
		QRegExp re_deviceversion("\\s*Device version:\\s*([^$]*)$");
		QRegExp re_driverversion("\\s*Driver version:\\s*([^$]*)$");
		QRegExp re_maxclock("\\s*Max clock \\(MHz\\):\\s*([^$]*)$");
		QRegExp re_streamprocessors("\\s*Stream processors:\\s*([0-9]+).*");
		QRegExp re_type("\\s*Device type:\\s*([^ ]+)[^$]*$");

		QStringList lines = opencl_out.split("\n");
		GPUINFO gi;
		bool first_device = true;
		foreach(QString line, lines)
		{
			if (re_device.exactMatch(line))
			{
				// add current device if we have one
				if (first_device)
				{
					first_device = false;
				}
				else
				{
					if (gi.type.startsWith("GPU") && IsSupportedGPU(gi))
					{
						m_gpuinfo.append(gi);
					}
				}

				// start new device
				gi.platform = GPU_OPENCL;
				gi.jtrindex = re_device.cap(1).toInt();
				gi.name = re_device.cap(2).trimmed();
			}
			else if (re_vendor.exactMatch(line))
			{
				gi.vendor = re_vendor.cap(1).trimmed();
			}
			else if (re_deviceversion.exactMatch(line))
			{
				gi.deviceversion = re_deviceversion.cap(1).trimmed();
//				if (gi.deviceversion.contains("CUDA"))
//				{
//					gi.platform = GPU_CUDA;
//				}
			}
			else if (re_driverversion.exactMatch(line))
			{
				gi.driverversion = re_driverversion.cap(1).trimmed();
			}
			else if (re_maxclock.exactMatch(line))
			{
				gi.maxclock = re_maxclock.cap(1).toInt();
			}
			else if (re_streamprocessors.exactMatch(line))
			{
				gi.streamprocessors = re_streamprocessors.cap(1).toInt();
			}
			else if (re_type.exactMatch(line))
			{
				gi.type = re_type.cap(1).trimmed();
			}

		}
		if (!first_device)
		{
			if (gi.type.startsWith("GPU") && IsSupportedGPU(gi))
			{
				m_gpuinfo.append(gi);
			}
		}
	}
}

/*
void CLC7GPUInfo::DetectCUDA()
{
	TR;
	QStringList cudaargs;
	cudaargs.append("--list=cuda-devices");

	QString cuda_out;
	QString cuda_err;
	int cuda_retval;

	if (ExecuteJTR(cudaargs, cuda_out, cuda_err, cuda_retval))
	{
		if (cuda_out.contains("Error: No CUDA-capable devices were detected by the installed CUDA driver."))
		{
			return;
		}

		if (cuda_out.contains("Error: The installed NVIDIA CUDA driver is older than the CUDA runtime library."))
		{
			return;
		}

		// Add cuda devices
		QRegExp re_device("CUDA Device #([0-9]+)");
		QRegExp re_name("\tName:\\s*(.*)");
		QRegExp re_type("\tType:\\s*(.*)");
		QRegExp re_streamprocessors("\tNumber of stream processors:\\s*(.*)");
		QRegExp re_maxclock("\tClock rate:\\s*(.*)");

		QStringList lines = cuda_out.split("\n");
		GPUINFO gi;
		bool first_device = true;
		foreach(QString line, lines)
		{
			if (re_device.exactMatch(line))
			{
				// add current device if we have one
				if (first_device)
				{
					first_device = false;
				}
				else
				{
					if (gi.type.startsWith("GPU"))
					{
						m_gpuinfo.append(gi);
					}
				}

				// start new device
				gi.platform = GPU_CUDA;
				gi.type = "GPU";
				gi.jtrindex = re_device.cap(1).toInt();
			}
			else if (re_type.exactMatch(line))
			{
				gi.type = re_type.cap(1).trimmed();
			}
			else if (re_name.exactMatch(line))
			{
				gi.name = re_device.cap(1).trimmed();
			}
			else if (re_maxclock.exactMatch(line))
			{
				gi.maxclock = re_maxclock.cap(1).toInt();
			}
			else if (re_streamprocessors.exactMatch(line))
			{
				gi.streamprocessors = re_streamprocessors.cap(1).toInt();
			}

		}
		if (!first_device)
		{
			if (gi.type.startsWith("GPU"))
			{
				m_gpuinfo.append(gi);
			}
		}
	}
}
*/
void CLC7GPUInfo::Detect(void)
{
	TR;
	DetectOPENCL();
	//DetectCUDA();
}

QVector<GPUINFO> CLC7GPUInfo::GetGPUInfo() const
{
	TR;
	return m_gpuinfo;
}

