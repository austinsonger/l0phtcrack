#include "lc7signertask.h"
#include "qcommandline.h"

#include "CkPrivateKey.h"
#include "CkPublicKey.h"
#include "CkRsa.h"

#define MAGIC (0xCDC31337)

void print(QString out)
{
	printf("%s",out.toLatin1().constData());
}

LC7SignerTask::LC7SignerTask(QObject *parent) : QObject(parent)
{
	m_app=QCoreApplication::instance();
	m_cmdline=new QCommandLine(*m_app);

	m_verify=false;
	m_key_file="";
	m_in_file="";
	m_out_file="";

	if(!m_rsa.UnlockComponent("NOCTEMRSA_7oMoLJcX3RsG"))
	{
		Q_ASSERT(false);
	}
}

LC7SignerTask::~LC7SignerTask()
{
	delete m_cmdline;
}

void LC7SignerTask::switchFound(const QString & name)
{
	if(name=="verify")
	{
		m_verify=true;
	}
}


void LC7SignerTask::optionFound(const QString & name, const QVariant & value)
{
	if(name=="key")
	{
		m_key_file = value.toString();
	}
}

void LC7SignerTask::paramFound(const QString & name, const QVariant & value)
{
	if(name=="infile")
	{
		m_in_file=value.toString();
	}
	else if(name=="outfile")
	{
		m_out_file=value.toString();
	}
}

void LC7SignerTask::parseError(const QString & error)
{
	print(QString("Error: ") + error + "\n");
	m_cmdline->showHelp(true, -1);
	m_app->exit(1);
}


void LC7SignerTask::run()
{
	m_cmdline->addSwitch('v', "verify", "Verify the infile as signed correctly instead of signing");
	m_cmdline->addOption('k',"key", "Path to private key to sign file with. Defaults to lc7.key in the working directory when signing, and lc7.pub when verifying.");
	m_cmdline->addParam("infile", "Path to file to sign or verify", QCommandLine::Mandatory);
	m_cmdline->addParam("outfile", "Path for signed output file when signing. Defaults to <infile>.signed", QCommandLine::Optional);
	m_cmdline->enableVersion(true);
	m_cmdline->enableHelp(true);

	connect(m_cmdline, SIGNAL(switchFound(const QString &)), this, SLOT(switchFound(const QString &)));
	connect(m_cmdline, SIGNAL(optionFound(const QString &, const QVariant &)), this, SLOT(optionFound(const QString &, const QVariant &)));
	connect(m_cmdline, SIGNAL(paramFound(const QString &, const QVariant &)), this, SLOT(paramFound(const QString &, const QVariant &)));
	connect(m_cmdline, SIGNAL(parseError(const QString &)), this, SLOT(parseError(const QString &)));

	m_cmdline->parse();
	
	if(m_key_file.isEmpty())
	{
		if(m_verify)
		{
			m_key_file="lc7.pub";
		}
		else
		{
			m_key_file="lc7.key";
		}
	}

	if(m_verify)
	{
		do_verify();
	}
	else
	{
		do_sign();
	}

	m_app->exit(0);
    //emit finished();
}


void LC7SignerTask::do_verify()
{
	QByteArray indata;
	QByteArray sig;
	if(!ReadSignedFile(m_in_file, indata, sig))
	{
		m_app->exit(6);
		return;
	}

	CkPublicKey pkey;
	if(!pkey.LoadOpenSslPemFile(m_key_file.toLatin1()))
	{
		print(QString("Could not load pem file: %1\n").arg(m_key_file));
		m_app->exit(7);
		return;
	}
	
	m_rsa.ImportPublicKeyObj(pkey);

    m_rsa.put_LittleEndian(false);
	
	CkByteData ckdata;
	ckdata.append2(indata.constData(),indata.size());

	CkByteData cksig;
	cksig.append2(sig.constData(),sig.size());
	
	if(!m_rsa.VerifyBytes(ckdata,"SHA-512",cksig))
	{
		print("Signature is NOT VALID.\n");
		m_app->exit(8);
		return;
	}
	print("Signature is valid.\n");
	m_app->exit(0);
	return;
}


void LC7SignerTask::do_sign()
{
	QFile in(m_in_file);
	if(!in.open(QIODevice::ReadOnly))
	{
		print(QString("Could not open input file: %1\n").arg(m_in_file));
		m_app->exit(4);
		return;
	}
	QByteArray indata = in.readAll();

	CkPrivateKey pkey;
	if(!pkey.LoadPemFile(m_key_file.toLatin1()))
	{
		print(QString("Could not load pem file: %1\n").arg(m_key_file));
		m_app->exit(2);
		return;
	}

	if(!m_rsa.ImportPrivateKeyObj(pkey))
	{
		print("Could not import private key, invalid format.\n");
		m_app->exit(3);
		return;
	}
	
    m_rsa.put_LittleEndian(false);

	CkByteData data;
	data.append2(indata.constData(),indata.size());

	CkByteData sigout;
	m_rsa.SignBytes(data,"SHA-512",sigout);

	QByteArray sig((const char *)sigout.getData(),sigout.getSize());

	if(m_out_file.isEmpty())
	{
		m_out_file=m_in_file+".signed";
	}

	if(!WriteSignedFile(m_out_file, indata, sig))
	{
		m_app->exit(5);
		return;
	}
}


bool LC7SignerTask::WriteSignedFile(QString out_file, QByteArray outdata, QByteArray sig)
{
	QFile out(out_file);
	if(!out.open(QIODevice::WriteOnly))
	{
		print(QString("Could not open output file: %1\n").arg(out_file));
		return false;
	}
	
	// Define header
	quint32 magic=qToLittleEndian(MAGIC);
	quint32 siglen=qToLittleEndian(sig.size());
	
	// Write header
	out.write((const char *)&magic,sizeof(magic));
	out.write((const char *)&siglen,sizeof(siglen));

	// Write signature
	out.write(sig);

	// Write signed data
	out.write(outdata);
	
	return true;
}

bool LC7SignerTask::ReadSignedFile(QString in_file, QByteArray &indata, QByteArray &sig)
{
	QFile in(in_file);
	if(!in.open(QIODevice::ReadOnly))
	{
		print(QString("Could not open input file: %1\n").arg(in_file));
		return false;
	}
	
	// Define header
	quint32 magic;
	quint32 siglen;
	
	// Read header
	if(in.read((char *)&magic,sizeof(magic))!=sizeof(magic))
	{
		print("Invalid header length.\n");
		return false;
	}
	magic=qFromLittleEndian(magic);
	if(magic!=MAGIC)
	{
		print("Invalid header. Not an LC7 Signed Package.\n");
		return false;
	}
	
	if(in.read((char *)&siglen,sizeof(siglen))!=sizeof(siglen))
	{
		print("Invalid header length.\n");
		return false;
	}
	siglen=qFromLittleEndian(siglen);
	
	// Read signature
	sig=in.read(siglen);
	if(sig.size()!=siglen)
	{
		print("Invalid signature, incorrect size.\n");
		return false;
	}

	// Write signed data
	indata=in.readAll();
	
	return true;
}

	