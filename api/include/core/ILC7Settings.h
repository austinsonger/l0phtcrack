#ifndef __INC_ILC7SETTINGS_H
#define __INC_ILC7SETTINGS_H

#include"core/ILC7Interface.h"

#include<qobject.h>
#include<qstring.h>
#include<qstringlist.h>
#include<qsettings.h>
#include<qvariant.h>
#include<qtextcodec.h>

class ILC7Settings:public ILC7Interface
{
protected:
    virtual ~ILC7Settings() {}

public:

	virtual void AddValueChangedListener(QObject *slot_recv, void (QObject::*slot_method)(QString key, const QVariant &value))=0;
	virtual void AddValueRemovedListener(QObject *slot_recv, void (QObject::*slot_method)(QString key))=0;

    virtual void clear()=0;
    virtual void sync()=0;
    virtual QSettings::Status status() const=0;

    virtual void beginGroup(const QString &prefix)=0;
    virtual void endGroup()=0;
    virtual QString group() const=0;

    virtual int beginReadArray(const QString &prefix)=0;
    virtual void beginWriteArray(const QString &prefix, int size = -1)=0;
    virtual void endArray()=0;
    virtual void setArrayIndex(int i)=0;

    virtual QStringList allKeys() const=0;
    virtual QStringList childKeys() const=0;
    virtual QStringList childGroups() const=0;
    virtual bool isWritable() const=0;

    virtual void setValue(const QString &key, const QVariant &value)=0;
    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const=0;

    virtual void remove(const QString &key)=0;
    virtual bool contains(const QString &key) const=0;

    virtual void setFallbacksEnabled(bool b)=0;
    virtual bool fallbacksEnabled() const=0;

    virtual QString fileName() const=0;
    virtual QSettings::Format format() const=0;
    virtual QSettings::Scope scope() const=0;
    virtual QString organizationName() const=0;
    virtual QString applicationName() const=0;

    virtual void setIniCodec(QTextCodec *codec)=0;
    virtual void setIniCodec(const char *codecName)=0;
    virtual QTextCodec *iniCodec() const=0;
};


#endif