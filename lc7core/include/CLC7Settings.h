#ifndef __INC_CLC7SETTINGS_H
#define __INC_CLC7SETTINGS_H

class CLC7Settings:public QObject, public ILC7Settings
{
	Q_OBJECT;
	
private:
	QSettings *m_settings;

signals:
	void valueChanged(QString key, const QVariant &value);
	void valueRemoved(QString key);

public:

	CLC7Settings();
	virtual ~CLC7Settings();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void AddValueChangedListener(QObject *slot_recv, void (QObject::*slot_method)(QString key, const QVariant &value));
	virtual void AddValueRemovedListener(QObject *slot_recv, void (QObject::*slot_method)(QString key));

    virtual void clear();
    virtual void sync();
    virtual QSettings::Status status() const;

    virtual void beginGroup(const QString &prefix);
    virtual void endGroup();
    virtual QString group() const;

    virtual int beginReadArray(const QString &prefix);
    virtual void beginWriteArray(const QString &prefix, int size = -1);
    virtual void endArray();
    virtual void setArrayIndex(int i);

    virtual QStringList allKeys() const;
    virtual QStringList childKeys() const;
    virtual QStringList childGroups() const;
    virtual bool isWritable() const;

    virtual void setValue(const QString &key, const QVariant &value);
    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    virtual void remove(const QString &key);
    virtual bool contains(const QString &key) const;

    virtual void setFallbacksEnabled(bool b);
    virtual bool fallbacksEnabled() const;

    virtual QString fileName() const;
    virtual QSettings::Format format() const;
    virtual QSettings::Scope scope() const;
    virtual QString organizationName() const;
    virtual QString applicationName() const;

    virtual void setIniCodec(QTextCodec *codec);
    virtual void setIniCodec(const char *codecName);
    virtual QTextCodec *iniCodec() const;
};

#endif