#include<stdafx.h>

ILC7Linkage *g_pLinkage=NULL;
CLC7ImportUnixPlugin *g_pImportUnixPlugin=NULL;

extern "C"
{

DLLEXPORT bool Register(ILC7Linkage *pLinkage)
{
	g_pLinkage = pLinkage;

	g_pImportUnixPlugin = new CLC7ImportUnixPlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pImportUnixPlugin);

	return true;
}

DLLEXPORT bool Unregister(void)
{
	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pImportUnixPlugin);
	delete g_pImportUnixPlugin;
	g_pImportUnixPlugin = NULL;

	g_pLinkage = NULL;

	return true;
}

}


BOOL WINAPI DllMain(
  HANDLE hinstDLL,
  DWORD dwReason,
  LPVOID lpvReserved
)
{
	if(dwReason==DLL_PROCESS_ATTACH)
	{
	
	}

	return TRUE;
}

