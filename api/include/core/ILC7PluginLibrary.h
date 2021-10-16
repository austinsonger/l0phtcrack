#ifndef __INC_ILC7PLUGINLIBRARY_H
#define __INC_ILC7PLUGINLIBRARY_H

#include"core/ILC7Interface.h"

class ILC7PluginLibrary:public ILC7Interface
{
protected:
	virtual ~ILC7PluginLibrary() {}

public:
	
	// Manifest
	virtual QString GetDisplayName()=0;
	virtual QString GetDisplayVersion()=0;
	virtual QString GetAuthorName()=0;
	virtual QString GetInternalName()=0;
	virtual quint32 GetInternalVersion()=0;
	virtual QString GetOperatingSystem()=0;
	virtual QString GetUpdateURL()=0;
	virtual bool IsSystemLibrary()=0;
	virtual QDateTime GetReleaseDate()=0;

	struct Dependency {
		QString m_internal_name;
		quint32 m_min_internal_version;
	};
	
	virtual QList<Dependency> GetDependencies()=0;
	
	// State
	virtual QList<ILC7Plugin *> GetPlugins()=0;

	enum STATE
	{
		DEACTIVATED=0,
		ACTIVATED=1,
		FAILED=2
	};

	virtual STATE GetState()=0;
	virtual void Fail(QString reason)=0;
	virtual QString GetFailureReason()=0;
	virtual QString GetFullPath()=0;

	virtual QList<QLibrary *> GetLibraries()=0;
	

	// Activation
	virtual bool Activate()=0;
	virtual void Deactivate()=0;

};


#endif