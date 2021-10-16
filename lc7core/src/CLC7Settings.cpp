#include"stdafx.h"

#undef TR
#define TR

CLC7Settings::CLC7Settings()
{TR;
	m_settings=new QSettings();
}

CLC7Settings::~CLC7Settings()
{TR;
	delete m_settings;
}

ILC7Interface *CLC7Settings::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Settings")
	{
		return this;
	}
	return NULL;
}

void CLC7Settings::AddValueChangedListener(QObject *slot_recv, void (QObject::*slot_method)(QString key, const QVariant &value))
{TR;
	QObject::connect(this,&CLC7Settings::valueChanged,slot_recv,slot_method);
}

void CLC7Settings::AddValueRemovedListener(QObject *slot_recv, void (QObject::*slot_method)(QString key))
{TR;
	QObject::connect(this,&CLC7Settings::valueRemoved,slot_recv,slot_method);
}


void CLC7Settings::clear()
{TR;
	m_settings->clear();
}

void CLC7Settings::sync()
{TR;
	m_settings->sync();
}

QSettings::Status CLC7Settings::status() const
{TR;
	return m_settings->status();
}

void CLC7Settings::beginGroup(const QString &prefix)
{TR;
	m_settings->beginGroup(prefix);
}

void CLC7Settings::endGroup()
{TR;
	m_settings->endGroup();
}

QString CLC7Settings::group() const
{TR;
	return m_settings->group();
}

int CLC7Settings::beginReadArray(const QString &prefix)
{TR;
	return m_settings->beginReadArray(prefix);
}

void CLC7Settings::beginWriteArray(const QString &prefix, int size)
{TR;
	m_settings->beginWriteArray(prefix,size);
}

void CLC7Settings::endArray()
{TR;
	m_settings->endArray();
}

void CLC7Settings::setArrayIndex(int i)
{TR;
	m_settings->setArrayIndex(i);
}

QStringList CLC7Settings::allKeys() const
{TR;
	return m_settings->allKeys();
}

QStringList CLC7Settings::childKeys() const
{TR;
	return m_settings->childKeys();
}

QStringList CLC7Settings::childGroups() const
{TR;
	return m_settings->childGroups();
}

bool CLC7Settings::isWritable() const
{TR;
	return m_settings->isWritable();
}

void CLC7Settings::setValue(const QString &key, const QVariant &value)
{TR;

	bool exists = m_settings->contains(key);
	m_settings->setValue(key,value);
	if (exists)
	{
		emit valueChanged(key, value);
	}
}

QVariant CLC7Settings::value(const QString &key, const QVariant &defaultValue) const
{TR;
	return m_settings->value(key,defaultValue);
}

void CLC7Settings::remove(const QString &key)
{TR;
	m_settings->remove(key);
	emit valueRemoved(key);
}

bool CLC7Settings::contains(const QString &key) const
{TR;
	return m_settings->contains(key);
}

void CLC7Settings::setFallbacksEnabled(bool b)
{TR;
	m_settings->setFallbacksEnabled(b);
}

bool CLC7Settings::fallbacksEnabled() const
{TR;
	return m_settings->fallbacksEnabled();
}

QString CLC7Settings::fileName() const
{TR;
	return m_settings->fileName();
}

QSettings::Format CLC7Settings::format() const
{TR;
	return m_settings->format();
}

QSettings::Scope CLC7Settings::scope() const
{TR;
	return m_settings->scope();
}

QString CLC7Settings::organizationName() const
{TR;
	return m_settings->organizationName();
}

QString CLC7Settings::applicationName() const
{TR;
	return m_settings->applicationName();
}

void CLC7Settings::setIniCodec(QTextCodec *codec)
{TR;
	m_settings->setIniCodec(codec);
}

void CLC7Settings::setIniCodec(const char *codecName)
{TR;
	m_settings->setIniCodec(codecName);
}

QTextCodec *CLC7Settings::iniCodec() const
{TR;
	return m_settings->iniCodec();
}


