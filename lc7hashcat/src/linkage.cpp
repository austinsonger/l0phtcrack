#include<stdafx.h>

ILC7Linkage *g_pLinkage=NULL;
CLC7JTRPlugin *g_pJTRPlugin = NULL;

extern "C"
{

DLLEXPORT bool Register(ILC7Linkage *pLinkage)
{
	g_pLinkage=pLinkage;

	g_pJTRPlugin = new CLC7JTRPlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pJTRPlugin);

	return true;
}

DLLEXPORT bool Unregister(void)
{
	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pJTRPlugin);
	delete g_pJTRPlugin;
	g_pJTRPlugin = NULL;

	g_pLinkage=NULL;

	return true;
}

}

#if (PLATFORM == PLATFORM_WIN32) || (PLATFORM == PLATFORM_WIN64)

BOOL WINAPI DllMain(
  HANDLE hinstDLL,
  DWORD dwReason,
  LPVOID lpvReserved
)
{
	return TRUE;
}

#else



#endif