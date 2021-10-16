#ifndef __INC_CLC7PLUGINLIBRARY_H
#define __INC_CLC7PLUGINLIBRARY_H


class CLC7PluginLibrary:public ILC7PluginLibrary
{
private:

	QString m_display_name;
	QString m_display_version;
	QString m_author_name;
	QString m_internal_name;
	quint32 m_internal_version;
	QString m_operating_system;
	QString m_update_url;
	bool m_is_system_library;
	QDateTime m_release_date;
	QList<ILC7PluginLibrary::Dependency> m_dependencies;

	QString m_full_path;
	ILC7PluginLibrary::STATE m_state;
	QString m_fail_reason;
	QList<ILC7Plugin *> m_plugins;
	QSet<QUuid> m_activated_plugin_ids;
	QList<ILC7Plugin *> m_activated_plugins;
	QList<QLibrary *> m_libraries;

public:
	CLC7PluginLibrary();
	virtual ~CLC7PluginLibrary();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual bool LoadManifest(QByteArray manifest_json, QString & error);

	// Manifest
	virtual QString GetDisplayName();
	virtual QString GetDisplayVersion();
	virtual QString GetAuthorName();
	virtual QString GetInternalName();
	virtual quint32 GetInternalVersion();
	virtual QString GetOperatingSystem();
	virtual QString GetUpdateURL();
	virtual bool IsSystemLibrary();
	virtual QDateTime GetReleaseDate();

	virtual QList<ILC7PluginLibrary::Dependency> GetDependencies();
	
	// State
	virtual QList<ILC7Plugin *> GetPlugins();
	virtual void AddPlugin(ILC7Plugin *plugin);
	virtual void RemovePlugin(ILC7Plugin *plugin);
	
	virtual ILC7PluginLibrary::STATE GetState();
	virtual void Fail(QString reason);
	virtual QString GetFailureReason();

	virtual void SetFullPath(QString path);
	virtual QString GetFullPath();
	
	virtual void AddLibrary(QLibrary *lib);
	virtual void ReleaseLibraries(void);
	virtual QList<QLibrary *> GetLibraries();

	// Activation
	virtual bool Activate();
	virtual void Deactivate();

};


#endif