#ifndef __INC_ILC7PLUGINREGISTRY_H
#define __INC_ILC7PLUGINREGISTRY_H

#include"core/ILC7Interface.h"

#include<qstring.h>
#include<quuid.h>
#include<qlist.h>

class ILC7Plugin;

class ILC7PluginRegistry:public ILC7Interface
{
protected:
	virtual ~ILC7PluginRegistry() {};

public:

	virtual bool InstallPluginLibrary(QString path, QString &error)=0;
	virtual bool UninstallPluginLibrary(QString path, QString &error)=0;
	virtual void EnablePluginLibrary(QString internal_name)=0;
	virtual void DisablePluginLibrary(QString internal_name)=0;
	virtual bool IsPluginLibraryDisabled(QString internal_name)=0;
	virtual QList<ILC7PluginLibrary *> GetPluginLibraries()=0;
	virtual ILC7PluginLibrary *GetPluginLibraryByInternalName(QString internal_name)=0;
		
	virtual bool LoadPluginLibraries(QList<ILC7PluginLibrary *> & failed_libraries, QString &error)=0;
	virtual void UnloadPluginLibraries()=0;
		
	virtual QList<ILC7PluginLibrary *> FindDependentPlugins(QList<ILC7PluginLibrary *> plugins)=0;
	virtual QList<ILC7PluginLibrary *> FindDependingPlugins(QList<ILC7PluginLibrary *> plugins)=0;

	virtual void RegisterPlugin(ILC7Plugin *pPlugin)=0;
	virtual void UnregisterPlugin(ILC7Plugin *pPlugin)=0;
	virtual ILC7Plugin *FindPluginByID(QUuid id)=0;
	virtual ILC7PluginLibrary *FindLibraryByPlugin(ILC7Plugin *pPlugin)=0;
};


#endif