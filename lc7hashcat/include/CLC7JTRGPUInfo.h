#ifndef __INC_CLC7JTRGPUINFO
#define __INC_CLC7JTRGPUINFO

#include<QVector>

enum GPUPLATFORM
{
	GPU_NONE=0,
	GPU_OPENCL=1,
	//GPU_CUDA=2
};

struct GPUINFO
{
	GPUINFO()
	{
		platform = GPU_NONE;
		jtrindex = 0xFFFFFFFF;
		maxclock = 0;
		streamprocessors = 0;
	}

	GPUPLATFORM platform;
	quint32 jtrindex;
	QString name;
	QString vendor;
	QString type;
	QString deviceversion;
	QString	driverversion;
	quint32 maxclock;
	quint32 streamprocessors;

	
	inline bool operator==(const GPUINFO &other) const
	{
		return
			(platform == other.platform) &&
			(jtrindex == other.jtrindex) &&
			(name == other.name) &&
			(vendor == other.vendor) &&
			(type == other.type) &&
			(deviceversion == other.deviceversion) &&
			(driverversion == other.driverversion) &&
			(maxclock == other.maxclock) &&
			(streamprocessors == other.streamprocessors);
	}
	
};

inline QDataStream &operator<<(QDataStream &out, const GPUINFO &gpuinfo)
{
	out << (quint32 &)gpuinfo.platform;
	out << gpuinfo.jtrindex;
	out << gpuinfo.name;
	out << gpuinfo.vendor;
	out << gpuinfo.type;
	out << gpuinfo.deviceversion;
	out << gpuinfo.driverversion;
	out << gpuinfo.maxclock;
	out << gpuinfo.streamprocessors;
	return out;
}

inline QDataStream &operator>>(QDataStream &in, GPUINFO &gpuinfo)
{
	in >> (quint32 &)gpuinfo.platform;
	in >> gpuinfo.jtrindex;
	in >> gpuinfo.name;
	in >> gpuinfo.vendor;
	in >> gpuinfo.type;
	in >> gpuinfo.deviceversion;
	in >> gpuinfo.driverversion;
	in >> gpuinfo.maxclock;
	in >> gpuinfo.streamprocessors;
	return in;
}

class CLC7GPUInfo
{
private:
	
	QVector<GPUINFO> m_gpuinfo;

	bool IsSupportedGPU(const GPUINFO & gi);
	bool ExecuteJTR(QStringList args, QString & out, QString &err, int &retval);
	void DetectOPENCL(void);
	//void DetectCUDA(void);

public:

	CLC7GPUInfo();
	void Detect(void);

	QVector<GPUINFO> GetGPUInfo() const;
};

#endif