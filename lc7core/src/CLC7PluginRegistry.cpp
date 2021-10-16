#include<stdafx.h>

#include"quazip.h"
#include"quazipfile.h"

#define MAGIC (0xCDC31337)

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	const char *filter="*.dll";
	const char *ext=".dll";
#elif defined(__APPLE__)
	const char *filter="*.dylib";
	const char *ext=".dylib";
#else
	const char *filter="*.so";
	const char *ext=".so";
#endif
	
CLC7PluginRegistry::CLC7PluginRegistry(CLC7Controller *ctrl)
{TR;
	m_ctrl=ctrl;
	m_current_library=NULL;
	m_activated=false;
}

CLC7PluginRegistry::~CLC7PluginRegistry()
{
	TR;

	Q_ASSERT(
		m_plugin_libraries.size() == 0 &&
		m_library_by_internal_name.size() == 0 &&
		m_library_by_plugin.size() == 0 &&
		m_plugin_by_id.size() == 0
	);
}


ILC7Interface *CLC7PluginRegistry::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7PluginRegistry")
	{
		return this;
	}
	return NULL;
}



void CLC7PluginRegistry::SetCurrentLibrary(CLC7PluginLibrary *current)
{TR;
	m_current_library = current;
}

bool CLC7PluginRegistry::RefreshPluginLibraries(QString &error)
{TR;
	if(m_library_by_plugin.size()!=0 || m_plugin_by_id.size()!=0)
	{
		Q_ASSERT(0);
		error="Can't refresh plugin library when plugins are activated. Contact L0phtCrack Support.";
		return false;
	}

	// Clear out all old plugin libraries
	foreach(CLC7PluginLibrary *lib,m_plugin_libraries)
	{
		delete lib;
	}
	m_plugin_libraries.clear();
	m_library_by_internal_name.clear();
	m_library_by_plugin.clear();
	m_plugin_by_id.clear();


	// Load up all new plugin library manifests
	QDir plugindir(m_ctrl->GetPluginsDirectory());
	if(!plugindir.exists())
	{
		return false;
	}
	QStringList pluginentries=plugindir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
	foreach(QString pluginentry, pluginentries)
	{
		QDir entrydir(plugindir);
		entrydir.cd(pluginentry);

		QString mfname=entrydir.filePath("manifest.json");

		QFile mf(mfname);
		if(!mf.exists() || !mf.open(QIODevice::ReadOnly))
		{
			m_ctrl->GetGUILinkage()->ErrorMessage("Malformed plugin",QString("The plugin at %1 does not have a manifest or you do not have permission to open it.").arg(entrydir.path()));
			continue;
		}

		QByteArray mfdata=mf.readAll();

		CLC7PluginLibrary *lib=new CLC7PluginLibrary();
		if(!lib->LoadManifest(mfdata,error))
		{
			delete lib;
			return false;
		}

		lib->SetFullPath(entrydir.absolutePath());

		m_plugin_libraries.append(lib);
		m_library_by_internal_name.insert(lib->GetInternalName(),lib);
	}
	
	return true;
}

bool CLC7PluginRegistry::VerifySignatureAndGetContents(QFile & f, QByteArray & contents)
{TR;
	quint32 siglen;
	if(f.read((char *)&siglen,sizeof(siglen))!=sizeof(siglen))
	{
		return false;
	}
	siglen=qFromLittleEndian(siglen);

	// Read signature
	QByteArray sig=f.read(siglen);
	if(sig.size()!=siglen)
	{
		return false;
	}
	
	// Read contents
	contents=f.readAll();

	// Verify signature
	// xxx todo eventually
	return true;
}

bool CLC7PluginRegistry::InstallPluginLibrary(QString path, QString &error)
{TR;
	QFile in(path);
	if(!in.open(QIODevice::ReadOnly))
	{
		error="Couldn't open input file";
		return false;
	}
	
	// See if this is a signed package
	quint32 magic;
	if(in.read((char *)&magic,sizeof(magic))!=sizeof(magic))
	{
		error="Invalid plugin file";
		return false;
	}
	magic=qFromLittleEndian(magic);

	QByteArray contents;
	if(magic==MAGIC)
	{
		if(!VerifySignatureAndGetContents(in, contents))
		{
			error="Signature is not valid. Not installing.";
			return false;
		}
	}
	else
	{
		in.seek(0);
		contents=in.readAll();

		m_ctrl->GetGUILinkage()->YesNoBox("Unsigned Plugin",QString("The plugin '%1' is not signed, or approved for use in LC7. Only install this from a trusted source. L0phtCrack is not responsible if this plugin causes you damage or loss. If you would like to proceed anyway, press Yes. If you would like to safely cancel this operation, press No.").arg(path));
	}

	// Open zip file contents
	QBuffer contentsbuffer(&contents);	
	QuaZip zip(&contentsbuffer);
	zip.open(QuaZip::mdUnzip);
	
	QuaZipFile file(&zip);

	// Extract Manifest
	QByteArray manifestjson;
	zip.setCurrentFile("manifest.json");
	qint64 size=file.usize();
	file.open(QIODevice::ReadOnly);
	manifestjson=file.readAll();

	// Load Manifest
	CLC7PluginLibrary testlib;
	if(!testlib.LoadManifest(manifestjson,error))
	{		
		// Close
		file.close();
		zip.close();
		return false;
	}
	file.close();

	// See if requirements are met
	foreach(ILC7PluginLibrary::Dependency dep, testlib.GetDependencies())
	{
		CLC7PluginLibrary *deplib;
		if ((deplib = m_library_by_internal_name.value(dep.m_internal_name, NULL)) == NULL)
		{
			error = QString("Dependency not met: %1\nA required plugin is not installed").arg(dep.m_internal_name);
			zip.close();
			return false;
		}
		if (deplib->GetInternalVersion() < dep.m_min_internal_version)
		{
			error = QString("Dependency not met: %1\n"
				"Required version is %d.\n"
				"Currently installed version is %d.\n"
				"Upgrading this plugin is required before installation will succeed.").arg(dep.m_internal_name)
				.arg(dep.m_min_internal_version)
				.arg(deplib->GetInternalVersion());
			zip.close();
			return false;
		}
	}

	// See if this is already installed
	CLC7PluginLibrary *installed;
	bool already_installed=false;
	bool same_version=false;
	bool older_version=false;
	if((installed=m_library_by_internal_name.value(testlib.GetInternalName(),NULL))!=NULL)
	{
		already_installed=true;

		// See if this is the same as something already installed
		if(installed->GetInternalVersion() == testlib.GetInternalVersion())
		{
			same_version=true;	
		}
		// See if this is older than something already installed
		else if (installed->GetInternalVersion() > testlib.GetInternalVersion())
		{
			older_version=true;
		}
	}

	bool install=true;
	if (already_installed)
	{
		if(same_version)
		{
			install=m_ctrl->GetGUILinkage()->YesNoBox("Reinstall?",QString("The plugin '%1' is already installed with the same version (%s). If you would like to reinstall anyway, press Yes. Otherwise, press No.")
				.arg(testlib.GetDisplayName())
				.arg(testlib.GetDisplayVersion()));
		}
		else if(older_version)
		{
			install=m_ctrl->GetGUILinkage()->YesNoBox("Downgrade?",QString("The plugin '%1' is already installed with a newer version (%s). If you would like to downgrade to version %s anyway, press Yes. Otherwise, press No.")
				.arg(testlib.GetDisplayName())
				.arg(installed->GetDisplayVersion())
				.arg(testlib.GetDisplayVersion()));
		}
	}

	if(install)
	{
		// First remove old version
		if(already_installed)
		{
			QDir plugindir(m_ctrl->GetPluginsDirectory());
			if(plugindir.cd(installed->GetInternalName()))
			{
				if(!plugindir.absolutePath().startsWith(m_ctrl->GetPluginsDirectory()))
				{
					error=QString("Not removing out-of-tree directory '%s'")
						.arg(plugindir.absolutePath());
					zip.close();
					return false;
				}
				plugindir.removeRecursively();
			}
		}

		// Then install new version
		QDir plugindir(m_ctrl->GetPluginsDirectory());
		plugindir.mkdir(testlib.GetInternalName());
		if(!plugindir.cd(testlib.GetInternalName()))
		{
			error=QString("Couldn't create installation directory '%s'")
				.arg(plugindir.absoluteFilePath(testlib.GetInternalName()));
			zip.close();
			return false;
		}

		// Verify absolute path
		if(!plugindir.absolutePath().startsWith(m_ctrl->GetPluginsDirectory()))
		{
			error=QString("Not extracting to out-of-tree directory '%s'")
				.arg(plugindir.absolutePath());
			zip.close();
			return false;
		}

		// Extract file hierarchy
		if(zip.goToFirstFile()) do
		{
			QuaZipFile file(&zip);
			if(file.open(QIODevice::ReadOnly))
			{
				QString target=plugindir.absoluteFilePath(file.getActualFileName());
				if(!target.startsWith(m_ctrl->GetPluginsDirectory()))
				{
					error=QString("Not extracting to out-of-tree directory '%s'")
						.arg(target);
					plugindir.removeRecursively();
					zip.close();
					return false;
				}
				
				QFileInfo targetfi(target);
				QString path=targetfi.absolutePath();
				plugindir.mkpath(path);

				// Extract file into target
				QByteArray data=file.readAll();

				file.close();
			}
			else {
				error=QString("Can not open file '%s'. Not installing.")
					.arg(file.getActualFileName());
				plugindir.removeRecursively();
				zip.close();
				return false;
			}
		} while(zip.goToNextFile());
	}

	zip.close();

	if(!RefreshPluginLibraries(error))
	{
		return false;
	}

	return true;
}

bool CLC7PluginRegistry::UninstallPluginLibrary(QString internal_name, QString &error)
{TR;
	if (!IsPluginLibraryDisabled(internal_name))
	{
		error = "Plugin must be disabled before it is uninstalled.";
		return false;
	}

	QDir plugindir(m_ctrl->GetPluginsDirectory());
	if (plugindir.cd(internal_name))
	{
		if (!plugindir.absolutePath().startsWith(m_ctrl->GetPluginsDirectory()))
		{
			error = QString("Not removing out-of-tree directory '%s'")
				.arg(plugindir.absolutePath());
			return false;
		}
		plugindir.removeRecursively();
	}
	else
	{
		error = "Plugin directory could not be found.";
		return false;
	}

	if (!RefreshPluginLibraries(error))
	{
		return false;
	}

	return true;
}

bool CLC7PluginRegistry::IsPluginLibraryDisabled(QString internal_name)
{TR;
	QStringList disabled_libraries = m_ctrl->GetSettings()->value("lc7core:disabled_plugins",QStringList()).toStringList();
	return disabled_libraries.contains(internal_name);
}

void CLC7PluginRegistry::EnablePluginLibrary(QString internal_name)
{TR;
	QStringList disabled_libraries = m_ctrl->GetSettings()->value("lc7core:disabled_plugins",QStringList()).toStringList();
	disabled_libraries.removeOne(internal_name);
	m_ctrl->GetSettings()->setValue("lc7core:disabled_plugins",disabled_libraries);
}

void CLC7PluginRegistry::DisablePluginLibrary(QString internal_name)
{TR;
	QStringList disabled_libraries = m_ctrl->GetSettings()->value("lc7core:disabled_plugins",QStringList()).toStringList();
	if(!disabled_libraries.contains(internal_name))
	{
		disabled_libraries.append(internal_name);
	}
	m_ctrl->GetSettings()->setValue("lc7core:disabled_plugins",disabled_libraries);
}

QList<ILC7PluginLibrary *> CLC7PluginRegistry::GetPluginLibraries()
{TR;
	QList<ILC7PluginLibrary *> pluglibs;
	foreach(CLC7PluginLibrary *library,m_plugin_libraries)
	{
		pluglibs.append((ILC7PluginLibrary *)library);
	}
	
	return pluglibs;
}

ILC7PluginLibrary *CLC7PluginRegistry::GetPluginLibraryByInternalName(QString internal_name)
{TR;
	return m_library_by_internal_name.value(internal_name,NULL);
}


bool CLC7PluginRegistry::LoadPluginLibraries(QList<ILC7PluginLibrary *> & failed_libraries, QString &error)
{TR;
	if(m_activated)
	{
		Q_ASSERT(0);
		m_ctrl->GetGUILinkage()->ErrorMessage("Plugin activation error", "Plugins already activated. Contact L0phtCrack Support.");
		return true;
	}

    const bool isGuiThread = (QThread::currentThread() == QCoreApplication::instance()->thread());
	if(!isGuiThread)
	{
		Q_ASSERT(0);
		error="Plugin activation not done from gui thread. Contact L0phtCrack Support.";
		m_ctrl->GetGUILinkage()->ErrorMessage("Plugin activation error", error);
		return false;
	}

	if(!RefreshPluginLibraries(error))
	{
		return false;
	}

	// Remove libraries with circular dependencies (early)
	bool removed;
	do
	{
		removed=false;

		QList<CLC7PluginLibrary *> to_remove;
		foreach(CLC7PluginLibrary *lib, m_plugin_libraries)
		{
			// remove already failed libraries
			if(failed_libraries.contains(lib))
			{
				continue;
			}
			// remove disabled libraries
			if(IsPluginLibraryDisabled(lib->GetInternalName()))
			{
				continue;
			}

			if(HasCircularDependencies(lib))
			{
				failed_libraries.append(lib);
				lib->Fail("Plugin library has circular dependencies. Contact the plugin developer.");
				removed=true;
			}
		}
	}
	while(removed);

	// Register all libraries that stand a chance
	foreach(CLC7PluginLibrary *lib, m_plugin_libraries)
	{
		// remove disabled libraries
		if(IsPluginLibraryDisabled(lib->GetInternalName()))
		{
			continue;
		}
		
		// remove already failed libraries
		if(failed_libraries.contains(lib))
		{
			continue;
		}

		QLinkedList<QLibrary*> loaded_libraries;
		QList<QLibrary *> registered_libraries;
		
		SetCurrentLibrary(lib);

		try 
		{	
			QDirIterator it(lib->GetFullPath(), QStringList() << filter, QDir::Files, QDirIterator::Subdirectories);
			while (it.hasNext()) 
			{
				QString dllpath = it.next();
				
				QFileInfo dllfi(dllpath);

				SetPath(dllfi.absolutePath());
				QLibrary *library=new QLibrary(dllpath);
				bool loaded=library->load();
				ResetPath();

				if(!loaded)
				{
					delete library;
					continue;
				}
			
				TYPEOF_Register *pRegister=(TYPEOF_Register *)(library->resolve("Register"));
				TYPEOF_Unregister *pUnregister=(TYPEOF_Unregister *)(library->resolve("Unregister"));

				if(pRegister==NULL || pUnregister==NULL)
				{
					delete library;
					continue;
				}

				loaded_libraries.append(library);
			}

			foreach(QLibrary *library, loaded_libraries)
			{
				TYPEOF_Register *pRegister=(TYPEOF_Register *)(library->resolve("Register"));
				if(!(*pRegister)(m_ctrl->GetLinkage()))
				{
					throw FailException(QString("Plugin library failed to register: %1 / %2").arg(lib->GetDisplayName()).arg(library->fileName()));
				}
						
				registered_libraries.append(library);
			}

			// Add all registered libraries to pluginlibrary for later deregistration
			foreach(QLibrary *library, registered_libraries)
			{
				lib->AddLibrary(library);
			}
		}
		catch(FailException e)
		{
			// Unregister all registered libraries
			foreach(QLibrary *ll, registered_libraries)
			{
				TYPEOF_Unregister *pUnregister=(TYPEOF_Unregister *)(ll->resolve("Unregister"));
				(*pUnregister)();
			}
						
			// Delete all loaded libraries
			foreach(QLibrary *ll, loaded_libraries)
			{
				delete ll;
			}

			// Add to failed list and continue to next pluginlibrary
			failed_libraries.append(lib);
			lib->Fail(e.Error());
		}

		SetCurrentLibrary(NULL);
	}

	// Remove libraries with missing or unmet dependencies
	do
	{
		removed=false;

		QList<CLC7PluginLibrary *> to_remove;
		foreach(CLC7PluginLibrary *lib, m_plugin_libraries)
		{
			foreach(ILC7PluginLibrary::Dependency dep, lib->GetDependencies())
			{
				if(m_library_by_internal_name.contains(dep.m_internal_name))
				{
					CLC7PluginLibrary *deplib=m_library_by_internal_name[dep.m_internal_name];
					
					if(failed_libraries.contains(deplib))
					{
						failed_libraries.append(lib);
						removed=true;
					}

					if(deplib->GetInternalVersion() < dep.m_min_internal_version)
					{
						failed_libraries.append(lib);
						removed=true;
					}
				}
				else
				{
					failed_libraries.append(lib);
					removed=true;
				}
			}
		}
	}
	while(removed);

	// Ensure all failed libraries are completely unloaded
	foreach(ILC7PluginLibrary *ipl, failed_libraries)
	{
		CLC7PluginLibrary *lib=(CLC7PluginLibrary *)ipl;
		foreach(QLibrary *ll, lib->GetLibraries())
		{
			TYPEOF_Unregister *pUnregister=(TYPEOF_Unregister *)(ll->resolve("Unregister"));
			(*pUnregister)();
		}
		lib->ReleaseLibraries();
	}

	// For all libraries in dependency order, activate plugins in dependency order
	SortPluginLibraries();
	foreach(CLC7PluginLibrary *lib, m_plugin_libraries)
	{
		if(!lib->Activate())
		{
			failed_libraries.append(lib);
		}
	}

	m_activated=true;
	return true;
}

void CLC7PluginRegistry::UnloadPluginLibraries()
{TR;
    const bool isGuiThread = (QThread::currentThread() == QCoreApplication::instance()->thread());
	if(!isGuiThread)
	{
		Q_ASSERT(0);
		m_ctrl->GetGUILinkage()->ErrorMessage("Plugin deactivation error", "Plugin deactivation not done from gui thread. Contact plugin developer.");
		return;
	}

	if(m_activated)
	{
		// Reverse order of activation for deactivation
		QList<CLC7PluginLibrary *> rev_plugin_libraries;
		foreach(CLC7PluginLibrary *lib, m_plugin_libraries)
		{
			rev_plugin_libraries.prepend(lib);
		}	

		// Deactivate all activated plugins
		foreach(CLC7PluginLibrary *lib, rev_plugin_libraries)
		{
			// Does nothing if not activated
			lib->Deactivate();
		}

		m_activated=false;
	}

	// Unregister all plugin libraries
	foreach(CLC7PluginLibrary *lib, m_plugin_libraries)
	{
		SetCurrentLibrary(lib);
		
		// Unregister all registered libraries
		foreach(QLibrary *ll, lib->GetLibraries() )
		{
			TYPEOF_Unregister *pUnregister=(TYPEOF_Unregister *)(ll->resolve("Unregister"));
			(*pUnregister)();
		}

		lib->ReleaseLibraries();

		SetCurrentLibrary(NULL);

		delete lib;
	}

	m_plugin_libraries.clear();
	m_library_by_internal_name.clear();
	m_library_by_plugin.clear();
	m_plugin_by_id.clear();
}

void CLC7PluginRegistry::RegisterPlugin(ILC7Plugin *pPlugin)
{TR;
	if(!m_current_library)
	{
		Q_ASSERT(0);
		m_ctrl->GetGUILinkage()->ErrorMessage("Plugin registration error", 
			QString("Plugin registered out of library: %1. Contact L0phtCrack Support.").arg(pPlugin->GetID().toString()));
		return;
	}

	m_current_library->AddPlugin(pPlugin);
	m_library_by_plugin.insert(pPlugin,m_current_library);
	m_plugin_by_id.insert(pPlugin->GetID(),pPlugin);
}

void CLC7PluginRegistry::UnregisterPlugin(ILC7Plugin *pPlugin)
{TR;
	if(!m_current_library)
	{
		Q_ASSERT(0);
		m_ctrl->GetGUILinkage()->ErrorMessage("Plugin unregistration error", 
			QString("Plugin unregistered out of library: %1. Contact L0phtCrack Support.").arg(pPlugin->GetID().toString()));
		return;
	}

	m_plugin_by_id.remove(pPlugin->GetID());
	m_library_by_plugin.remove(pPlugin);
	m_current_library->RemovePlugin(pPlugin);
}

ILC7Plugin *CLC7PluginRegistry::FindPluginByID(QUuid id)
{TR;
	if(!m_plugin_by_id.contains(id))
	{
		return NULL;
	}
	return m_plugin_by_id[id];
}

ILC7PluginLibrary *CLC7PluginRegistry::FindLibraryByPlugin(ILC7Plugin *pPlugin)
{TR;
	if(!m_library_by_plugin.contains(pPlugin))
	{
		return NULL;
	}
	return m_library_by_plugin[pPlugin];
}

// This can not fail because circular dependencies
// have already been removed and the set is validated
void CLC7PluginRegistry::SortPluginLibraries(void)
{TR;
	// Sort by plugin library dependency order
	QList<CLC7PluginLibrary *> pluginlibraries(m_plugin_libraries);
	m_plugin_libraries.clear();
	QSet<CLC7PluginLibrary *> loaded;

	bool more=true;
	while(more)
	{
		more=false;
		foreach(CLC7PluginLibrary *lib, pluginlibraries)
		{
			if(loaded.contains(lib))
			{
				continue;
			}

			bool skip=false;
			foreach(ILC7PluginLibrary::Dependency dep, lib->GetDependencies())
			{
				CLC7PluginLibrary *depplug = m_library_by_internal_name[dep.m_internal_name];

				if(!loaded.contains(depplug))
				{
					skip=true;
					break;
				}
			}
			if(!skip)
			{
				m_plugin_libraries.append(lib);
				loaded.insert(lib);
				more=true;
			}
		}
	}
}
	

bool CLC7PluginRegistry::HasCircularDependencies(CLC7PluginLibrary *lib)
{TR;
	QSet<CLC7PluginLibrary *> visited;
	QSet<CLC7PluginLibrary *> next;
	QSet<CLC7PluginLibrary *> nextnext;

	next.insert(lib);

	bool more;
	do
	{
		more=false;
		
		foreach(CLC7PluginLibrary * plug, next)
		{
			visited.insert(plug);

			foreach(ILC7PluginLibrary::Dependency dep, plug->GetDependencies())
			{
				CLC7PluginLibrary *depplug=m_library_by_internal_name[dep.m_internal_name];
				if(visited.contains(depplug))
				{
					return true;
				}
				nextnext.insert(depplug);
				more=true;
			}
		}

		next=nextnext;
		nextnext.clear();
	}
	while(more);
	
	return false;
}

void CLC7PluginRegistry::SetPath(QString dirpath)
{TR;
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	SetDllDirectory(QDir::toNativeSeparators(dirpath).toUtf8());
	//m_path=env.value("PATH","");
	//QString path=QDir::toNativeSeparators(cwd)+";"+m_path;			// Add app directory
	//path=QDir::toNativeSeparators(dirpath)+";"+path;	// Add directory
	//env.insert("PATH",path);
#elif defined(__APPLE__)
	QString cwd=QCoreApplication::applicationDirPath();
	QProcessEnvironment env=QProcessEnvironment::systemEnvironment();

	m_path=env.value("DYLD_LIBRARY_PATH","");
	QString path=QDir::toNativeSeparators(cwd)+";"+m_path;			// Add app directory
	path=QDir::toNativeSeparators(dirpath)+";"+path;	// Add lcplugins directory
	env.insert("DYLD_LIBRARY_PATH",path);
#else
	bitch
#endif
}

void CLC7PluginRegistry::ResetPath(void)
{TR;
/*
	QProcessEnvironment env=QProcessEnvironment::systemEnvironment();
	env.insert("PATH",m_path);
	m_path="";
	*/
#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)
	SetDllDirectory(NULL);
#else
	QProcessEnvironment env=QProcessEnvironment::systemEnvironment();
	env.insert("DYLD_LIBRARY_PATH",m_path);
#endif
}


QList<ILC7PluginLibrary *> CLC7PluginRegistry::FindDependentPlugins(QList<ILC7PluginLibrary *> plugins)
{TR;
	QSet<ILC7PluginLibrary *> pset(plugins.toSet());
	bool changed=true;
	while(changed)
	{
		changed=false;
		foreach(ILC7PluginLibrary *lib, m_plugin_libraries)
		{
			if(pset.contains(lib))
			{
				continue;
			}

			QList<ILC7PluginLibrary::Dependency> deps=lib->GetDependencies();
			foreach(ILC7PluginLibrary::Dependency dep, deps)
			{
				ILC7PluginLibrary *deplib=m_library_by_internal_name[dep.m_internal_name];
				if(pset.contains(deplib) && !pset.contains(lib))
				{
					pset.insert(lib);
					changed=true;
					break;
				}
			}
		}
	}

	// Return only dependent plugins
	QList<ILC7PluginLibrary *> ret;
	foreach(ILC7PluginLibrary *lib, pset)
	{
		if(!plugins.contains(lib))
		{
			ret.append(lib);
		}
	}

	return ret;
}

QList<ILC7PluginLibrary *> CLC7PluginRegistry::FindDependingPlugins(QList<ILC7PluginLibrary *> plugins)
{TR;
	QSet<ILC7PluginLibrary *> pset(plugins.toSet());
	bool changed=true;
	while(changed)
	{
		changed=false;

		QList<ILC7PluginLibrary *> plist(pset.toList());

		foreach(ILC7PluginLibrary *lib, plist)
		{
			QList<ILC7PluginLibrary::Dependency> deps=lib->GetDependencies();
			foreach(ILC7PluginLibrary::Dependency dep, deps)
			{
				ILC7PluginLibrary *deplib=m_library_by_internal_name[dep.m_internal_name];
				if(!pset.contains(deplib))
				{
					pset.insert(deplib);
					changed=true;
					break;
				}
			}
		}
	}

	// Return only depending plugins
	QList<ILC7PluginLibrary *> ret;
	foreach(ILC7PluginLibrary *lib, pset)
	{
		if(!plugins.contains(lib))
		{
			ret.append(lib);
		}
	}

	return ret;
}
