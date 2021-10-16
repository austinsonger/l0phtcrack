#include<stdafx.h>

ILC7Linkage *g_pLinkage=NULL;
CLC7BasePlugin *g_pBasePlugin=NULL;

extern "C"
{

DLLEXPORT bool Register(ILC7Linkage *pLinkage)
{
	g_pLinkage=pLinkage;

	g_pBasePlugin=new CLC7BasePlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pBasePlugin);
	
	return true;
}

DLLEXPORT bool Unregister(void)
{
	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pBasePlugin);
	delete g_pBasePlugin;
	g_pBasePlugin=NULL;

	g_pLinkage=NULL;

	return true;
}

}