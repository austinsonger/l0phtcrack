#include<stdafx.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

ILC7Linkage *g_pLinkage=NULL;
CLC7ImportWinPlugin *g_pImportWinPlugin=NULL;

extern "C"
{

DLLEXPORT bool Register(ILC7Linkage *pLinkage)
{
	g_pLinkage=pLinkage;

	g_pImportWinPlugin=new CLC7ImportWinPlugin();
	g_pLinkage->GetPluginRegistry()->RegisterPlugin(g_pImportWinPlugin);

	return true;
}

DLLEXPORT bool Unregister(void)
{
	g_pLinkage->GetPluginRegistry()->UnregisterPlugin(g_pImportWinPlugin);
	delete g_pImportWinPlugin;
	g_pImportWinPlugin=NULL;

	g_pLinkage=NULL;

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
		OpenSSL_add_all_ciphers();
		ERR_load_crypto_strings();
	}

	return TRUE;
}

