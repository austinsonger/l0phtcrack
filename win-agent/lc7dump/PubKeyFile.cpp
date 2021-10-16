#ifdef QT_CORE_LIB
	// Qt Implementation
	#include<qdebug.h>	
#define INVALID_FILE nullptr
#else
	// Windows Implmentation
	#define _WIN32_WINNT _WIN32_WINNT_WINXP
#define INVALID_FILE INVALID_HANDLE_VALUE
#endif

#include"PubKeyFile.h"

#include <stdio.h>
#include <assert.h>
#include <inttypes.h>

U32 netswap(U32 const net) {
	uint8_t *data = (uint8_t *)&net;
	return ((U32)data[3] << 0)
		| ((U32)data[2] << 8)
		| ((U32)data[1] << 16)
		| ((U32)data[0] << 24);
}
#define htonl netswap
#define ntohl netswap


//#define NO_SYMMETRIC_ENCRYPTION 1
#define BUFFER_SIZE (65536)
#define KEY_BYTE_LENGTH (32)
#define HEADER_BYTE_LENGTH (1024)

#ifdef _DEBUG
#define Stringize( L )			#L
#define MakeString( M, L )		M(L)
#define $Line					MakeString( Stringize, __LINE__ )
#ifdef QT_CORE_LIB
inline void qdebugout(const char *str) {
	qDebug() << str;
}
inline void qdebugoutw(const wchar_t *wstr) {
	qDebug() << wstr;
}
#define DBGOUT(x) qdebugout(x)
#define DBGOUTW(x) qdebugoutw(x)
#define DBGTRACE  qdebugout("@LINE:" $Line "\r\n" )
#else
#define DBGOUT(x) OutputDebugStringA(x)
#define DBGOUTW(x) OutputDebugStringW(x)
#define DBGTRACE  OutputDebugStringA("@LINE:" $Line "\r\n" )
#endif
#else
#define DBGOUT(x) 
#define DBGTRACE
#endif

extern int hash_idx, prng_idx;

#ifdef QT_CORE_LIB
bool read_file(QFile *file, void *lpBuffer, U32 nNumberOfBytesToRead, U32 *lpNumberOfBytesRead)
{
	qint64 bytesread = file->read((char *)lpBuffer, (qint64)nNumberOfBytesToRead);
	if (bytesread < 0) {
		return false;
	}
	*lpNumberOfBytesRead = (U32)bytesread;
	return true;
}
bool write_file(QFile *file, const void *lpBuffer, U32 nNumberOfBytesToWrite, U32 *lpNumberOfBytesWritten)
{
	qint64 byteswritten = file->write((const char *)lpBuffer, (qint64)nNumberOfBytesToWrite);
	if (byteswritten < 0) {
		return false;
	}
	*lpNumberOfBytesWritten = (U32)byteswritten;
	return true;
}
void close_file(QFile **file) {
	delete *file;
	*file = nullptr;
}
#else
bool read_file(HANDLE hFile, void *lpBuffer, U32 nNumberOfBytesToRead, U32 *lpNumberOfBytesRead)
{
	return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, NULL) == TRUE;
}
bool write_file(HANDLE hFile, void *lpBuffer, U32 nNumberOfBytesToWrite, U32 *lpNumberOfBytesWritten)
{
	return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL) == TRUE;
}
void close_file(HANDLE *hFile) {
	CloseHandle(*hFile);
	*hFile = INVALID_HANDLE_VALUE;
}
#endif


void CPubKeyFile::CommonConstruct() 
{
	m_bDidOpen = false;
	m_bufferallocated = BUFFER_SIZE + EVP_MAX_IV_LENGTH;
	m_buffer = (unsigned char *)malloc(m_bufferallocated);
	m_buffer_enc = (unsigned char *)malloc(m_bufferallocated);
	m_bufferlength = 0;
	m_bufferpos = 0;
	m_bSerializedSymmetricKey = false;
	m_cipher = EVP_aes_256_cbc();
	m_ivlen = EVP_CIPHER_iv_length(m_cipher);
	EVP_CIPHER_CTX_init(&m_ctx);
	m_pkey = NULL;
	m_evp_ek = NULL;
}


CPubKeyFile::CPubKeyFile()
{
//	DBGTRACE;

#ifdef QT_CORE_LIB
	m_file = nullptr;
#else
	m_file = INVALID_HANDLE_VALUE;
#endif
	m_bIsWrite=false;

	CommonConstruct();
}

#ifdef QT_CORE_LIB
CPubKeyFile::CPubKeyFile(QFile *file, QByteArray key, bool bIsWrite)
#else
CPubKeyFile::CPubKeyFile(HANDLE hFile, const char *key, bool bIsWrite)
#endif
{
//	DBGTRACE;

#ifdef QT_CORE_LIB
	m_file = file;
#else
	m_file = hFile;
#endif

	m_bIsWrite=bIsWrite;

	CommonConstruct();

	SetKey(key, bIsWrite);
}

CPubKeyFile::~CPubKeyFile()
{
	Close();
	free(m_buffer);
}

bool CPubKeyFile::LoadAndDecryptBuffer()
{
	if(m_pkey == NULL || !m_bDidOpen || m_bIsWrite) {
		DBGOUT("LoadAndDecryptBuffer: key not set or didn't open or wrong mode");
		Close();
		return false;
	}

	U32 nread = 0;

	if (!m_bSerializedSymmetricKey)
	{
		unsigned char iv[EVP_MAX_IV_LENGTH];

		// Read encrypted key length byte length
		U32 eklen = 0;
		if (!read_file(m_file, &eklen, 4, &nread) || nread != 4)
		{
			DBGOUT("LoadAndDecryptBuffer: ReadFile eklen failed");
			Close();
			return false;
		}
		eklen = ntohl(eklen);

		// Read encrypted key
		nread = 0;
		if (!read_file(m_file, m_evp_ek, eklen, &nread) || nread != eklen)
		{
			DBGOUT("LoadAndDecryptBuffer: ReadFile evp ek failed");
			Close();
			return false;
		}

		// Read IV
		nread = 0;
		if (!read_file(m_file, &iv, m_ivlen, &nread) || nread != m_ivlen)
		{
			DBGOUT("LoadAndDecryptBuffer: ReadFile iv failed");
			Close();
			return false;
		}

		// Open EVP seal
		if (!EVP_OpenInit(&m_ctx, m_cipher, m_evp_ek, eklen, iv, m_pkey))
		{
			DBGOUT("LoadAndDecryptBuffer: EVP_OpenInit failed");
			Close();
			return false;
		}

		m_bSerializedSymmetricKey = true;
	}

	// Read encrypted bytes
	U32 enc_bufferlength;
	if (!read_file(m_file, m_buffer_enc, BUFFER_SIZE, &enc_bufferlength))
	{
		DBGOUT("LoadAndDecryptBuffer: ReadFile data failed");
		Close();
		return false;
	}
#if NO_SYMMETRIC_ENCRYPTION
	memcpy(m_buffer, m_buffer_enc, enc_bufferlength);
	m_bufferlength = enc_bufferlength;
#else
	int buflen;
	if (enc_bufferlength != 0) {
		if (!EVP_OpenUpdate(&m_ctx, m_buffer, &buflen, m_buffer_enc, enc_bufferlength))
		{
			DBGOUT("LoadAndDecryptBuffer: EVP_OpenUpdate failed");
			Close();
			return false;
		}
		if (buflen == 0) {
			if (!EVP_OpenFinal(&m_ctx, m_buffer, &buflen))
			{
				DBGOUT("LoadAndDecryptBuffer: EVP_OpenFinal failed");
				Close();
				return false;
			}
		}
	}
	else {
		if (!EVP_OpenFinal(&m_ctx, m_buffer, &buflen))
		{
			DBGOUT("LoadAndDecryptBuffer: EVP_OpenFinal failed");
			Close();
			return false;
		}
	}
	m_bufferlength = buflen;
#endif

	m_bufferpos=0;

	return true; 
}

bool CPubKeyFile::EncryptAndSaveBuffer()
{
	if (m_pkey == NULL || m_file == INVALID_FILE || !m_bIsWrite) {
		DBGOUT("EncryptAndSaveBuffer: key not set or didn't open or wrong mode");
		Close();
		return false;
	}

	U32 nwritten = 0;

	if (!m_bSerializedSymmetricKey)
	{
		unsigned char iv[EVP_MAX_IV_LENGTH];

		// Start EVP seal
		int eklen;
		if (!EVP_SealInit(&m_ctx, m_cipher, &m_evp_ek, &eklen, iv, &m_pkey, 1))
		{
			DBGOUT("EncryptAndSaveBuffer: EVP_SealInit failed");
			Close();
			return false;
		}

		//////////////////////////////

		// Write encrypted key byte length
		U32 eklen_n = htonl(eklen);
		if (!write_file(m_file, &eklen_n, 4, &nwritten) || nwritten != 4)
		{
			DBGOUT("EncryptAndSaveBuffer: WriteFile eklen_n failed");
			Close();
			return false;
		}

		// Write encrypted key
		nwritten = 0;
		if (!write_file(m_file, m_evp_ek, eklen, &nwritten) || nwritten != eklen)
		{
			DBGOUT("EncryptAndSaveBuffer: WriteFile evp_ek failed");
			Close();
			return false;
		}

		// Write IV
		if (!write_file(m_file, iv, m_ivlen, &nwritten) || nwritten != m_ivlen)
		{
			DBGOUT("EncryptAndSaveBuffer: WriteFile iv failed");
			Close();
			return false;
		}
		
		m_bSerializedSymmetricKey = true;
	}

	// Encrypt
	int len_out;
#if NO_SYMMETRIC_ENCRYPTION
	memcpy(m_buffer_out, m_buffer, m_bufferlength);
	len_out = (int)m_bufferlength;
#else 
	if (!EVP_SealUpdate(&m_ctx, m_buffer_enc, &len_out, m_buffer, m_bufferlength)) {
		DBGOUT("EncryptAndSaveBuffer: EVP_SealUpdate failed");
		Close();
		return false;
	}
#endif
	
	// Write encrypted bytes
	nwritten = 0;
	if(!write_file(m_file, m_buffer_enc, (U32)len_out, &nwritten) || nwritten != (U32)len_out)
	{
		DBGOUT("EncryptAndSaveBuffer: WriteFile failed");
		return false;
	}

	// Start buffer back at zero
	m_bufferlength = 0;

	return true; 
}

#ifdef QT_CORE_LIB
bool CPubKeyFile::Open(QString fname, bool bWrite)
#else
bool CPubKeyFile::Open(const wchar_t *pfname, bool bWrite, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
#endif
{
	if (m_file != INVALID_FILE)
	{
		return false;
	}

#ifdef QT_CORE_LIB

	m_file = new QFile(fname);
	if(!m_file->open(bWrite?QIODevice::OpenModeFlag::ReadWrite | QIODevice::OpenModeFlag::Truncate : QIODevice::ReadOnly | QIODevice::ExistingOnly)) {
		m_bDidOpen=false;
		delete m_file;
		m_file = nullptr;
		return false;
	}
#else
	m_file = CreateFile(pfname, bWrite ? FILE_GENERIC_WRITE : FILE_GENERIC_READ,
		FILE_SHARE_READ | (bWrite ? 0 : FILE_SHARE_WRITE), lpSecurityAttributes,
		bWrite ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (m_file == INVALID_HANDLE_VALUE)
	{
		m_bDidOpen = false;
		return false;
	}
#endif
	m_bDidOpen=true;
	m_bIsWrite=bWrite;
	m_bSerializedSymmetricKey = false;
	return true;
}

bool CPubKeyFile::Close()
{
//DBGOUT("Close()");
	if (m_file == INVALID_FILE)
	{
		return false;
	}

	bool ok = true;
	if(m_bIsWrite)
	{
		if(!EncryptAndSaveBuffer())
		{
			ok = false;
		}

#if NO_SYMMETRIC_ENCRYPTION
#else
		// Seal final
		int len_out;
		if (!EVP_SealFinal(&m_ctx, m_buffer_enc, &len_out))
		{
			DBGOUT("Close: EVP_SealFinal failed.");
			ok = false;
		} else {
			// Write encrypted bytes
			U32 nwritten = 0;
			if (!write_file(m_file, m_buffer_enc, (U32)len_out, &nwritten) || nwritten != (U32)len_out)
			{
				DBGOUT("Close: WriteFile failed");
				return false;
			}
		}
#endif
	}

	m_bSerializedSymmetricKey = false;

	if (m_pkey)
		EVP_PKEY_free(m_pkey);
	if (m_evp_ek)
		free(m_evp_ek);

	if(!m_bDidOpen)
	{
		m_file = INVALID_FILE;
		return ok;
	}

	close_file(&m_file);
	
	m_bDidOpen=false;

	return ok;
}

#ifdef QT_CORE_LIB
void CPubKeyFile::SetKey(QByteArray key, bool pub)
#else
void CPubKeyFile::SetKey(const char *key, bool pub)
#endif
{
//	DBGTRACE;
	if (m_pkey) {
		DBGOUT("SetKey: Key already set");
		return;
	}

	RSA *rsa = NULL;
#ifdef QT_CORE_LIB
	BIO *bufio = BIO_new_mem_buf(key.constData(), (int)key.size());
#else
	BIO *bufio = BIO_new_mem_buf((void*)key, (int)strlen(key));
#endif
	if(pub)
	{
		PEM_read_bio_RSA_PUBKEY(bufio, &rsa, 0, NULL);
	}
	else
	{
		PEM_read_bio_RSAPrivateKey(bufio, &rsa, 0, NULL);
	}
	BIO_free(bufio);

	m_pkey = EVP_PKEY_new();
	if (!EVP_PKEY_assign_RSA(m_pkey, rsa))
	{
		DBGOUT("SetKey: EVP_PKEY_assign_RSA failed");
		return;
	}
	
	m_evp_ek = (unsigned char *)malloc(EVP_PKEY_size(m_pkey));
	
//	DBGTRACE;
}


bool CPubKeyFile::Read(void *buf, U32 len, U32 *pRead)
{
	if(m_file==INVALID_FILE || m_bIsWrite)
	{
		return false;
	}

	U32 readcursor=0;
	while(len>0)
	{
		// Get maximum we can read from this buffer
		U32 bufbytesleft = m_bufferlength - m_bufferpos;		
		// If the buffer is empty, load a new buffer
		if(bufbytesleft == 0)
		{
			if(!LoadAndDecryptBuffer())
			{
				return false;
			}
		}
		// If there's still no buffer, we're done
		bufbytesleft = m_bufferlength - m_bufferpos;
		if (bufbytesleft == 0)
		{
			break;
		}
		U32 readlen = (len < bufbytesleft)?len:bufbytesleft;
		
		// Read from buffer
		memcpy(((unsigned char *)buf) + readcursor, m_buffer + m_bufferpos, readlen);
		readcursor += readlen;
		m_bufferpos += readlen;
		len-=readlen;
	}

	if(pRead)
	{
		*pRead=readcursor;
	}

	return true;
}

bool CPubKeyFile::Write(const void *buf, U32 len, U32 *pWritten)
{
	if(m_file==INVALID_FILE || !m_bIsWrite)
	{
		return false;
	}
	
	//char writeout[256];
	//_snprintf_s(writeout, 256, "Write(%d), buflen=%d, bufalloc=%d\n", len, m_bufferlength, m_bufferallocated);
	//DBGOUT(writeout);

	U32 written=0;
	while(len>0)
	{
		U32 filllen = len;
		if( (m_bufferlength + filllen) > BUFFER_SIZE)
		{
			filllen = (BUFFER_SIZE - m_bufferlength);
		}

		//char writeout[256];
		//_snprintf_s(writeout, 256, "  memcpy(buflen=%d, filllen=%d)\n", m_bufferlength, filllen);
		//DBGOUT(writeout);

		memcpy(m_buffer + m_bufferlength, (char *)buf + written, filllen);
		len -= filllen;
		m_bufferlength += filllen;
		written += filllen;

		if(m_bufferlength == BUFFER_SIZE)
		{
			if(!EncryptAndSaveBuffer())
			{
				return false;
			}
		}
	}
	
	if(pWritten)
	{
		*pWritten=written;
	}

	return true;
}
