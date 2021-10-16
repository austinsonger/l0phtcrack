#ifndef __INC_CLC7PLUGINREGISTRY_H
#define __INC_CLC7PLUGINREGISTRY_H

class CLC7PluginLibrary;

class CLC7PluginRegistry:public QObject, public ILC7PluginRegistry
{
	Q_OBJECT;

private:

	bool m_activated;
	CLC7PluginLibrary *m_current_library;
	CLC7Controller *m_ctrl;
	QList<CLC7PluginLibrary *> m_plugin_libraries;
	QMap<QString, CLC7PluginLibrary *> m_library_by_internal_name;

	QMap<ILC7Plugin *, CLC7PluginLibrary *> m_library_by_plugin;
	QMap<QUuid, ILC7Plugin *> m_plugin_by_id;

	QString m_path;
		
protected:

	void SetCurrentLibrary(CLC7PluginLibrary *current);
	bool RefreshPluginLibraries(QString & error);
	void SortPluginLibraries(void);
	bool HasCircularDependencies(CLC7PluginLibrary *lib);
	void SetPath(QString dirpath);
	void ResetPath(void);
	bool VerifySignatureAndGetContents(QFile & f, QByteArray & contents);
	
public:

	CLC7PluginRegistry(CLC7Controller *ctrl);
	virtual ~CLC7PluginRegistry();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);


	virtual bool InstallPluginLibrary(QString path, QString &error);
	virtual bool UninstallPluginLibrary(QString path, QString &error);
	virtual void EnablePluginLibrary(QString internal_name);
	virtual void DisablePluginLibrary(QString internal_name);
	virtual bool IsPluginLibraryDisabled(QString internal_name);
	virtual QList<ILC7PluginLibrary *> GetPluginLibraries();
	virtual ILC7PluginLibrary *GetPluginLibraryByInternalName(QString internal_name);
		
	virtual bool LoadPluginLibraries(QList<ILC7PluginLibrary *> & failed_libraries, QString &error);
	virtual void UnloadPluginLibraries();
		
	virtual QList<ILC7PluginLibrary *> FindDependentPlugins(QList<ILC7PluginLibrary *> plugins);
	virtual QList<ILC7PluginLibrary *> FindDependingPlugins(QList<ILC7PluginLibrary *> plugins);

	virtual void RegisterPlugin(ILC7Plugin *pPlugin);
	virtual void UnregisterPlugin(ILC7Plugin *pPlugin);
	virtual ILC7Plugin *FindPluginByID(QUuid id);
	virtual ILC7PluginLibrary *FindLibraryByPlugin(ILC7Plugin *pPlugin);
};


#endif