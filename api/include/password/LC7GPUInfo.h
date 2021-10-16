#ifndef __INC_LC7LC7GPUINFO_H
#define __INC_LC7LC7GPUINFO_H

enum GPUPLATFORM
{
	GPU_NONE=0,
	GPU_OPENCL=1,
	//GPU_CUDA=2
};

struct LC7GPUInfo
{
	LC7GPUInfo()
	{
		platform = GPU_NONE;
		internal_index = 0xFFFFFFFF;
		maxclock = 0;
		streamprocessors = 0;
	}

	GPUPLATFORM platform;
	quint32 internal_index;
	QString name;
	QString vendor;
	QString type;
	QString deviceversion;
	QString	driverversion;
	quint32 maxclock;
	quint32 streamprocessors;

	
	inline bool operator==(const LC7GPUInfo &other) const
	{
		return
			(platform == other.platform) &&
			(internal_index == other.internal_index) &&
			(name == other.name) &&
			(vendor == other.vendor) &&
			(type == other.type) &&
			(deviceversion == other.deviceversion) &&
			(driverversion == other.driverversion) &&
			(maxclock == other.maxclock) &&
			(streamprocessors == other.streamprocessors);
	}
	
};

inline QDataStream &operator<<(QDataStream &out, const LC7GPUInfo &gpuinfo)
{
	out << (quint32 &)gpuinfo.platform;
	out << gpuinfo.internal_index;
	out << gpuinfo.name;
	out << gpuinfo.vendor;
	out << gpuinfo.type;
	out << gpuinfo.deviceversion;
	out << gpuinfo.driverversion;
	out << gpuinfo.maxclock;
	out << gpuinfo.streamprocessors;
	return out;
}

inline QDataStream &operator>>(QDataStream &in, LC7GPUInfo &gpuinfo)
{
	in >> (quint32 &)gpuinfo.platform;
	in >> gpuinfo.internal_index;
	in >> gpuinfo.name;
	in >> gpuinfo.vendor;
	in >> gpuinfo.type;
	in >> gpuinfo.deviceversion;
	in >> gpuinfo.driverversion;
	in >> gpuinfo.maxclock;
	in >> gpuinfo.streamprocessors;
	return in;
}

#endif