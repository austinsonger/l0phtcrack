#ifndef __INC_LC7SecureString_H
#define __INC_LC7SecureString_H

class LC7SecureString
{
	friend QDataStream& operator<<(QDataStream& out, const LC7SecureString& v);
	friend QDataStream& operator>>(QDataStream& in, LC7SecureString& v);

protected:

	QString m_description;
	QString m_string;
	bool m_initialized;

public:

	LC7SecureString()
	{
		m_initialized = false;
	}

	LC7SecureString(QString str, QString description)
	{
		m_string = str;
		m_description = description;
		m_initialized = true;
	}

	LC7SecureString(const LC7SecureString & copy) {
		m_string = copy.m_string;
		m_description = copy.m_description;
		m_initialized = copy.m_initialized;
	}

	virtual ~LC7SecureString() {
	}

public:

	bool IsInitialized(void) const { return m_initialized; }

	QString GetDescription() const { return m_description; };
	void SetDescription(QString description) { m_description = description; }

	QString GetString() const { return m_string; };
	void SetString(QString str) { m_string = str; m_initialized = true; }

	QString ToSerializedString() const {
		QByteArray ba;
		QDataStream ds(&ba,QIODevice::WriteOnly);
		ds << m_initialized;
		ds << m_string;
		ds << m_description;
		return QString::fromLatin1(ba.toBase64());
	}
	
	static LC7SecureString FromSerializedString(QString str) {
		QByteArray ba = QByteArray::fromBase64(str.toLatin1());
		QDataStream ds(ba);
		LC7SecureString ss;
		ds >> ss.m_initialized;
		ds >> ss.m_string;
		ds >> ss.m_description;
		return ss;
	}

	typedef void TYPEOF_SeralizeOut(void *ctx, QDataStream& out, const LC7SecureString & secstr);
	typedef void TYPEOF_SeralizeIn(void *ctx, QDataStream& in, LC7SecureString & secstr);
};

Q_DECLARE_METATYPE(LC7SecureString);

inline QDataStream& operator<<(QDataStream& out, const LC7SecureString& secstr) {
	
	LC7SecureString::TYPEOF_SeralizeOut *SerializeOut = (LC7SecureString::TYPEOF_SeralizeOut *)(qApp->property("LC7SecureStringSerializeOut").toULongLong());
	void *ctx = (void *)(qApp->property("LC7SecureStringSerializeContext").toULongLong());

	SerializeOut(ctx, out, secstr);

	return out;
}

inline QDataStream& operator>>(QDataStream& in, LC7SecureString& secstr) {
	
	LC7SecureString::TYPEOF_SeralizeIn *SerializeIn = (LC7SecureString::TYPEOF_SeralizeIn *)(qApp->property("LC7SecureStringSerializeIn").toULongLong());
	void *ctx = (void *)(qApp->property("LC7SecureStringSerializeContext").toULongLong());

	SerializeIn(ctx, in, secstr);

	return in;

}

#endif