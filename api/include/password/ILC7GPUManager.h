#ifndef __INC_ILC7GPUMANAGER_H
#define __INC_ILC7GPUMANAGER_H

#include"core/ILC7Interface.h"
#include"LC7GPUInfo.h"

class ILC7GPUManager:public ILC7Interface
{
protected:
	virtual ~ILC7GPUManager() {}

public:

	virtual void Detect(void) = 0;
	virtual QVector<LC7GPUInfo> GetGPUInfo() const = 0;
};

#endif