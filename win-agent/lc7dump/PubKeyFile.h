#ifndef __INC_PUBKEYFILE_H
#define __INC_PUBKEYFILE_H

#ifdef QT_CORE_LIB
	// Qt Implementation
#include<QtGlobal>
#include<qfile.h>
#define U32 quint32
#else
	
#include <Windows.h>
#define U32 DWORD
#endif

#include <inttypes.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/err.h>

class CPubKeyFile
{
private:
#ifdef QT_CORE_LIB
	QFile *m_file;
#else
	HANDLE m_file;
#endif
	bool m_bDidOpen;
	bool m_bIsWrite;
	
	unsigned char *m_buffer;
	unsigned char *m_buffer_enc;
	unsigned char *m_buffer_dec;
	U32 m_bufferlength;
	U32 m_bufferpos;
	U32 m_bufferallocated;
	
	const EVP_CIPHER *m_cipher;
	U32 m_ivlen;
	EVP_PKEY *m_pkey;
	EVP_CIPHER_CTX m_ctx;
	unsigned char *m_evp_ek;

	bool m_bSerializedSymmetricKey;
		
	bool LoadAndDecryptBuffer();
	bool EncryptAndSaveBuffer();

	void CommonConstruct();

public:
	CPubKeyFile();
#ifdef QT_CORE_LIB
	CPubKeyFile(QFile *file, QByteArray key, bool bIsWrite);
	virtual bool Open(QString fname, bool bWrite);
	virtual void SetKey(QByteArray key, bool pub);
#else
	CPubKeyFile(HANDLE hFile, const char *key, bool bIsWrite);
	virtual bool Open(const wchar_t *pfname, bool bWrite, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
	virtual void SetKey(const char *key, bool pub);
#endif
	virtual ~CPubKeyFile();

	virtual bool Close();
	
	virtual bool Read(void *buf, U32 len, U32 *pRead);
	virtual bool Write(const void *buf, U32 len, U32 *pWritten);

};

#endif