#ifndef __INC_LC7HASHTYPE_H
#define __INC_LC7HASHTYPE_H

typedef quint32 fourcc;

#define FOURCC(x) ( ((fourcc)(x)[0]) | ((fourcc)((x)[1]<<8)) | ((fourcc)((x)[2]<<16)) | ((fourcc)((x)[3]<<24)) )
#define FOURCCSTRING(x) QString("%1%2%3%4").arg( ((x)>>0)&0xFF ).arg( ((x)>>8)&0xFF ).arg( ((x)>>16)&0xFF ).arg( ((x)>>24)&0xFF )

struct LC7HashType {
	fourcc fourcc;
	QString name;
	QString platform;
	QString description;
	QMap<QString, QSet<QUuid>> registrants; // keys are action categories 'import', 'technique', values are plugin uuids
};

// Well-known account types, others may exist and be registered via ILC7PasswordLinkage

#define HASHTYPE_LM						"LM__"		// LM hash
#define HASHTYPE_NT						"__NT"		// NT Hash
#define HASHTYPE_LM_CHALRESP			"LMCR"		// LM response + challenge
#define HASHTYPE_NTLM_CHALRESP			"NTCR"		// NT response + challenge
#define HASHTYPE_LM_CHALRESP_V2			"LMC2"		// LM response + challenge v2
#define HASHTYPE_NTLM_CHALRESP_V2		"NTC2"		// NT response + challenge v2
#define HASHTYPE_UNIX_DES				"UDES"		// UNIX Salted DES crypt
#define HASHTYPE_UNIX_MD5				"UMD5"		// UNIX Salted MD5 $1$
#define HASHTYPE_UNIX_BLOWFISH			"UBLO"		// UNIX Salted BLOWFISH $2$ or $2a$ 
#define HASHTYPE_UNIX_SHA256			"USH8"		// UNIX Salted SHA256 $5$
#define HASHTYPE_UNIX_SHA512			"USH9"		// UNIX Salted SHA512 $6$
#define HASHTYPE_AIX_MD5				"AMD5"		// AIX Salted MD5 {smd5}
#define HASHTYPE_AIX_SHA1				"ASH1"		// AIX Salted SHA1 {ssha1}
#define HASHTYPE_AIX_SHA256				"ASH8"		// AIX Salted SHA256 {ssha256}
#define HASHTYPE_AIX_SHA512				"ASH9"		// AIX Salted SHA512 {ssha512}
#define HASHTYPE_MAC_SHA1				"MSS1"		// MAC OS X 10.4-10.6 Salted SHA-1 (no prefix, 52 hex digits)
#define HASHTYPE_MAC_SHA512				"MSS9"		// MAC OS X 10.7 Salted SHA-512 $LION$
#define HASHTYPE_MAC_PBKDF2_SHA512		"MSP9"		// MAC OS X 10.8+ PBKDF2-SALTED-SHA512 $pbkdf2-hmac-sha512$, $ml$

#endif