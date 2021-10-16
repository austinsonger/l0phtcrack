#include"stdafx.h"


CLC7CommandControl::CLC7CommandControl(CLC7WorkQueue *workqueue)
{
	m_workqueue=workqueue;
	m_parent = nullptr;
	m_sub_count = 0;
}

CLC7CommandControl::CLC7CommandControl(CLC7CommandControl *parent, QString statustext)
{
	m_parent = parent;
	m_statustext = statustext;
	m_sub_count = 0;
}


CLC7CommandControl::~CLC7CommandControl()
{
	Q_ASSERT(m_sub_count == 0);
}

ILC7Interface *CLC7CommandControl::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7CommandControl")
	{
		return this;
	}
	return NULL;
}

ILC7CommandControl *CLC7CommandControl::GetSubControl(QString statustext)
{
	m_sub_count++;
	return new CLC7CommandControl(this, statustext);
}

void CLC7CommandControl::ReleaseSubControl(ILC7CommandControl *ctrl)
{
	CLC7CommandControl *subctrl = (CLC7CommandControl *)ctrl;
	Q_ASSERT(subctrl->m_parent == this);
	Q_ASSERT(m_sub_count != 0);
	m_sub_count--;
	delete subctrl;
}

void CLC7CommandControl::ReleaseSubControl()
{
	Q_ASSERT(m_parent);
	m_parent->ReleaseSubControl(this);
}

void CLC7CommandControl::AppendToActivityLog(QString text)
{
	if (m_parent)
	{
		m_parent->AppendToActivityLog(m_statustext + text);
		return;
	}

	m_workqueue->GetWorkQueueWidget()->AppendToActivityLog(text);
}

void CLC7CommandControl::SetStatusText(QString text)
{
	if (m_parent)
	{
		m_parent->SetStatusText(m_statustext + text);
		return;
	}

	m_workqueue->GetWorkQueueWidget()->SetStatusText(text);
}

void CLC7CommandControl::UpdateCurrentProgressBar(quint32 cur)
{
	if (m_parent)
	{
		m_parent->UpdateCurrentProgressBar(cur);
		return;
	}
	m_workqueue->GetWorkQueueWidget()->UpdateCurrentProgressBar(cur);
}

bool CLC7CommandControl::PauseRequested()
{
	if (m_parent)
	{
		return m_parent->PauseRequested();
	}	
	return m_workqueue->IsPauseRequested();
}

bool CLC7CommandControl::StopRequested()
{
	if (m_parent)
	{
		return m_parent->StopRequested();
	}
	return m_workqueue->IsStopRequested();
}

void CLC7CommandControl::SaveCheckpointConfig(QMap<QString, QVariant> checkpoint_config)
{
	if (m_parent)
	{
		m_parent->SaveCheckpointConfig(checkpoint_config);
		return;
	}
	m_workqueue->SaveCheckpointConfig(checkpoint_config);
}

