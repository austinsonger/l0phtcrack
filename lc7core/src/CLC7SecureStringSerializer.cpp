#include"stdafx.h"

CLC7SecureStringSerializer::CLC7SecureStringSerializer(CLC7Controller *ctrl)
{
	m_ctrl = ctrl;
	m_did_cancel = false;
	m_allow_insecure = false;

	qApp->setProperty("LC7SecureStringSerializeIn", (qulonglong)&CLC7SecureStringSerializer::SerializeIn);
	qApp->setProperty("LC7SecureStringSerializeOut", (qulonglong)&CLC7SecureStringSerializer::SerializeOut);
	qApp->setProperty("LC7SecureStringSerializeContext", (qulonglong)this);
}

CLC7SecureStringSerializer::~CLC7SecureStringSerializer()
{
	qApp->setProperty("LC7SecureStringSerializeIn", QVariant());
	qApp->setProperty("LC7SecureStringSerializeOut", QVariant());
	qApp->setProperty("LC7SecureStringSerializeContext", QVariant());
}

void CLC7SecureStringSerializer::SetAllowInsecure(bool allow_insecure)
{
	m_allow_insecure = allow_insecure;
}

bool CLC7SecureStringSerializer::AllowInsecure()
{
	return m_allow_insecure;
}



void CLC7SecureStringSerializer::DoSerializeOut(QDataStream& out, const LC7SecureString & secstr)
{
	bool insecure = m_ctrl->GetSettings()->value("_core_:allow_insecure_sessions", false).toBool();
	bool init = secstr.IsInitialized();
	out << init;
	if (init)
	{
		out << insecure;
		out << secstr.GetDescription();
		if (insecure)
		{
			out << secstr.GetString();
		}
	}
}

void CLC7SecureStringSerializer::DoSerializeIn(QDataStream& in, LC7SecureString & secstr)
{
	bool insecure;
	bool init;
	in >> init;
	if (init)
	{
		in >> insecure;
		QString desc;
		in >> desc;
		secstr.SetDescription(desc);
		QString str;
		if (insecure)
		{
			in >> str;
		}
		else
		{
			if (m_cache.contains(desc))
			{
				str = m_cache[desc];
			}
			else
			{
				if (!m_ctrl->GetGUILinkage()->AskForPassword(QString("Enter Password"), QString("This session was saved without credentials for security purposes.\n\nTo load it, enter %1.\n\nIf you choose 'cancel', some functionality that required the password may be disabled, such as remediation.").arg(desc), str))
				{
					m_did_cancel = true;
				}
				
				// Cache answer 
				m_cache[desc] = str;
			}
		}
		secstr.SetString(str);
	}
}

bool CLC7SecureStringSerializer::DidCancel()
{
	return m_did_cancel;
}
void CLC7SecureStringSerializer::ClearCache()
{
	m_did_cancel = false;
	m_cache.clear();
}

void CLC7SecureStringSerializer::SerializeOut(void *ctx, QDataStream& out, const LC7SecureString & secstr)
{
	((CLC7SecureStringSerializer *)ctx)->DoSerializeOut(out, secstr);
}

void CLC7SecureStringSerializer::SerializeIn(void *ctx, QDataStream& in, LC7SecureString & secstr)
{
	((CLC7SecureStringSerializer *)ctx)->DoSerializeIn(in, secstr);
}


////////

CLC7SecureStringSerializerOptionWrapper::CLC7SecureStringSerializerOptionWrapper(CLC7SecureStringSerializer *ser, bool allow_insecure)
{
	m_ser = ser;
	m_old_allow_insecure = ser->AllowInsecure();

	m_ser->SetAllowInsecure(allow_insecure);
}

CLC7SecureStringSerializerOptionWrapper::~CLC7SecureStringSerializerOptionWrapper()
{
	m_ser->SetAllowInsecure(m_old_allow_insecure);
}