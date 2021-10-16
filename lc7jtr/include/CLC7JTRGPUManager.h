#ifndef __INC_CLC7JTRGPUMANAGER
#define __INC_CLC7JTRGPUMANAGER

#include<QVector>

class CLC7JTRGPUManager : public ILC7GPUManager
{
private:
	
	QVector<LC7GPUInfo> m_gpuinfo;

	bool IsSupportedGPU(const LC7GPUInfo & gi);
	bool ExecuteJTR(QStringList args, QString & out, QString &err, int &retval);
	void DetectOPENCL(void);
	//void DetectCUDA(void);

public:

	CLC7JTRGPUManager();
	virtual ~CLC7JTRGPUManager();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void Detect(void);

	virtual QVector<LC7GPUInfo> GetGPUInfo() const;
};

#endif