#include<stdafx.h>

ILC7Linkage *g_pLinkage=NULL;
CLC7PasswordPlugin *g_pPasswordPlugin=NULL;
CLC7PasswordGUIPlugin *g_pPasswordGUIPlugin=NULL;

extern "C"
{

DLLEXPORT bool Register(ILC7Linkage *pLinkage)
{
	g_pLinkage=pLinkage;

	g_pPasswordPlugin=new CLC7PasswordPlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pPasswordPlugin);
	
	g_pPasswordGUIPlugin=new CLC7PasswordGUIPlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pPasswordGUIPlugin);
	
	return true;
}

DLLEXPORT bool Unregister(void)
{
	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pPasswordGUIPlugin);
	delete g_pPasswordGUIPlugin;
	g_pPasswordGUIPlugin=NULL;

	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pPasswordPlugin);
	delete g_pPasswordPlugin;
	g_pPasswordPlugin=NULL;

	g_pLinkage=NULL;

	return true;
}

}