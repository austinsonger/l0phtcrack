#ifndef __INC_LC7ACCOUNT_H
#define __INC_LC7ACCOUNT_H

#include<qstring.h>
#include<qbytearray.h>
#include"LC7HashType.h"


struct LC7Hash
{
	fourcc hashtype;			// Hash type code
	QByteArray hash;			// Account hash in parseable text format
	QString password;			// Account password as-cracked
	quint8 crackstate;			// Crack-state
	quint32 cracktime;			// How long it took to crack (seconds)
	QString cracktype;			// What type of crack succeeded
};

struct LC7Account
{
public:
	QString username;			// Account user name
	QString userid;				// User ID
	QString userinfo;			// User Info (Full name, home directory, etc)
	QList<LC7Hash> hashes;		// Hashes
	QString domain;				// Account domain
	QString machine;			// Machine address
	quint64 lastchanged;		// Password last changed date (time64_t)
	union
	{
		quint16 flags;
		struct
		{
			quint16 lockedout : 1;		// Flag: Account locked out
			quint16 disabled : 1;		// Flag: Account disabled
			quint16 mustchange : 1;		// Flag: Password must change
			quint16 neverexpires : 1;	// Flag: Password never expires
		};
	};
	qint32 remediations;		// Remediations number (index in to ILC7AccountList import list)
};

#define CRACKSTATE_NOT_CRACKED 0
#define CRACKSTATE_FIRSTHALF_CRACKED 1
#define CRACKSTATE_SECONDHALF_CRACKED 2
#define CRACKSTATE_CRACKED 3

#endif