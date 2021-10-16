#ifndef __INC_CLC7SECURESTRINGSERIALIZER_H
#define __INC_CLC7SECURESTRINGSERIALIZER_H

class CLC7Controller;

class CLC7SecureStringSerializer
{
private:
	CLC7Controller *m_ctrl;

	QMap<QString, QString> m_cache;
	bool m_did_cancel;
	bool m_allow_insecure;
public:
	
	CLC7SecureStringSerializer(CLC7Controller *ctrl);
	~CLC7SecureStringSerializer();

	void SetAllowInsecure(bool allow_insecure);
	bool AllowInsecure();

	void DoSerializeOut(QDataStream& out, const LC7SecureString & secstr);
	void DoSerializeIn(QDataStream& in, LC7SecureString & secstr);
	
	bool DidCancel();
	void ClearCache(void);

public:

	static void SerializeOut(void *ctx, QDataStream& out, const LC7SecureString & secstr);
	static void SerializeIn(void *ctx, QDataStream& in, LC7SecureString & secstr);
};

class CLC7SecureStringSerializerOptionWrapper
{
private:
	CLC7SecureStringSerializer *m_ser;
	bool m_old_allow_insecure;
public:
	CLC7SecureStringSerializerOptionWrapper(CLC7SecureStringSerializer *ser, bool allow_insecure);
	~CLC7SecureStringSerializerOptionWrapper();
};

#endif