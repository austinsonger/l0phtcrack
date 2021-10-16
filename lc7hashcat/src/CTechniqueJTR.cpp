#include<stdafx.h>


CTechniqueJTR::CTechniqueJTR()
{TR;
	g_pLinkage->RegisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CTechniqueJTR::NotifySessionActivity);
	m_accountlist=NULL;
}

CTechniqueJTR::~CTechniqueJTR ()
{TR;
	g_pLinkage->UnregisterNotifySessionActivity(ACCOUNTLIST_HANDLER_ID, this, (void (QObject::*)(ILC7Linkage::SESSION_ACTIVITY, ILC7SessionHandler *))&CTechniqueJTR::NotifySessionActivity);
}

ILC7Interface *CTechniqueJTR::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7Component")
	{
		return this;
	}
	return NULL;
}

QUuid CTechniqueJTR::GetID()
{TR;
	return UUID_TECHNIQUEJTR;
}

void CTechniqueJTR::NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler)
{TR;
	switch(activity)
	{
	case ILC7Linkage::SESSION_OPEN_POST:
	case ILC7Linkage::SESSION_NEW_POST:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = (ILC7AccountList *)handler;
		}
		break;
	case ILC7Linkage::SESSION_CLOSE_PRE:
		if (handler && handler->GetId() == ACCOUNTLIST_HANDLER_ID)
		{
			m_accountlist = NULL;		
		}
		break;
	default:
		break;
	}
}


ILC7Component::RETURNCODE CTechniqueJTR::ExecuteCommand(QString command, QStringList args, QMap<QString,QVariant> & config, QString & error, ILC7CommandControl *ctrl)
{TR;
	if(command=="crack")
	{
		CLC7JTRPlugin *jtrplugin = (CLC7JTRPlugin *)g_pLinkage->GetPluginRegistry()->FindPluginByID(UUID_LC7JTRPLUGIN);
		if (!jtrplugin)
		{
			error = "Plugin failure";
			return FAIL;
		}
		jtrplugin->CheckCalibration();

		CLC7JTR jtr(m_accountlist, ctrl);
		
		if(!jtr.Configure(config))
		{
			error=QString("The technique configuration is invalid: %1").arg(jtr.LastError());
			return FAIL;
		}

		jtr.StartCracking();

		bool success = false;
		bool stopped = false;
		QDateTime last_time = QDateTime::currentDateTime();
		QDateTime checkpoint_time = QDateTime::currentDateTime();
		while(1)
		{
			if(jtr.CheckCrackingFinished(success))
			{
				break;
			}

			if (ctrl && (ctrl->StopRequested() || ctrl->PauseRequested()))
			{
				jtr.StopCracking();
				jtr.ProcessStatus();
				jtr.Cleanup();

				config=jtr.GetConfig();

				if(ctrl->StopRequested())
				{
					config.remove("stopped_context");
					return STOPPED;
				}
				return PAUSED;
			}

			QDateTime current_time = QDateTime::currentDateTime();
			if(last_time.secsTo(current_time)>=1)
			{
				last_time=current_time;
				jtr.ProcessStatus();
			}
			if (checkpoint_time.secsTo(current_time) >= 10)
			{
				// Report checkpoint upstream every 10 seconds
				checkpoint_time = current_time;
				QMap<QString, QVariant> checkpoint_config = jtr.GetCheckpointConfig();
				ctrl->SaveCheckpointConfig(checkpoint_config);
			}
			QThread::msleep(250);
		}
	
		jtr.ProcessStatus();

		ctrl->SetStatusText("Done");
		ctrl->UpdateCurrentProgressBar(100);

		if(!success)
		{
			error=jtr.LastError();
			jtr.Cleanup();

			config=jtr.GetConfig();
			return FAIL;
		}
	
		jtr.Cleanup();

		config=jtr.GetConfig();
		return SUCCESS;
	}

	error="Unknown command";
	return FAIL;
}

bool CTechniqueJTR::ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error)
{TR;
	if(command=="crack")
	{
		if(!state.contains("hashtypes"))
		{
			error="No hashes are imported.";
			return false;
		}
		
		QList<QVariant> hashtypes=state["hashtypes"].toList();
		ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);

		foreach(QVariant htv, hashtypes)
		{
			fourcc fcc = (fourcc)htv.toUInt();
			LC7HashType lc7hashtype;
			QString hterror;
			if (!passlink->Lookup(fcc, lc7hashtype, hterror) || !lc7hashtype.registrants["technique"].contains(UUID_LC7JTRPLUGIN))
			{
				error = "Incompatible hash type for this audit technique.";
				return false;
			}
		}

		state["cracked"]=true;
	}
	return true;
}
