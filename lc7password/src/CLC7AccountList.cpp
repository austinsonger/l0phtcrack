#include<stdafx.h>



///////////////////////////////////////////////////////////////////////////////

CLC7AccountList::CLC7AccountList(QUuid handler_id):m_acquire(QMutex::Recursive)
{TR;
	m_handler_id = handler_id;
	m_is_acquired = false;
	memset(&m_stats, 0, sizeof(STATS));
}

CLC7AccountList::~CLC7AccountList()
{TR;
}

ILC7Interface *CLC7AccountList::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7AccountList")
	{
		return this;
	}
	return NULL;
}

void CLC7AccountList::ReportModified(void)
{
	g_pLinkage->ReportSessionModified();
}


void CLC7AccountList::RegisterUpdateNotification(QObject *slot_obj, void (QObject::*slot_func)(QList<ACCOUNT_UPDATE_ACTION *>))
{
	// cast: (void(QObject::*)(QList<ACCOUNT_UPDATE_ACTION>))
	connect(this, &CLC7AccountList::sig_update, slot_obj, slot_func, Qt::QueuedConnection);
	connect(this, &CLC7AccountList::sig_direct_update, slot_obj, slot_func, Qt::DirectConnection);
}

void CLC7AccountList::AddUpdateAction(ACCOUNT_UPDATE_ACTION *update)
{
	if (m_updates.size()>0 && m_updates.last()->type == update->type)
	{
		switch (update->type)
		{
		case ACCT_CLEAR:
			return;
		case ACCT_APPEND:
			Q_ASSERT(update->count == update->accts.size());
			m_updates.last()->count += update->count;
			m_updates.last()->accts.append(update->accts);
			delete update;
			return;
		case ACCT_INSERT:
			Q_ASSERT(update->count == update->accts.size());
			if ((m_updates.last()->index + m_updates.last()->count) == update->index)
			{
				m_updates.last()->count += update->count;
				m_updates.last()->accts.append(update->accts);
				delete update;
				return;
			}
			break;
		case ACCT_REPLACE:
			Q_ASSERT(update->count == update->accts.size());
			if ((m_updates.last()->index + m_updates.last()->accts.size()) == update->index)
			{
				m_updates.last()->count += update->count;
				m_updates.last()->accts.append(update->accts);
				delete update;
				return;
			}
			break;
		case ACCT_REMOVE:
			// No batching here
			break;
		}
	}

	// If not batched yet, add update to end
	m_updates.append(update);
}

void CLC7AccountList::SendUpdates()
{
	Q_ASSERT(m_is_acquired);
	const bool isGuiThread = (QThread::currentThread() == QCoreApplication::instance()->thread());

	if (m_updates.size() > 0)
	{
		if (isGuiThread)
		{
			emit sig_direct_update(m_updates);
		}
		else
		{
			emit sig_update(m_updates);
		}
		m_updates.clear();
	}
}



/// ILC7SessionHandler /////////////////////////////////////////////////////////

QUuid CLC7AccountList::GetId()
{TR;
	return m_handler_id;
}

void CLC7AccountList::Acquire()
{TR;
	m_acquire.lock();
	m_is_acquired = true;
}

void CLC7AccountList::Release()
{TR;
	Q_ASSERT(m_is_acquired);

	// Send updates performed during this session
	SendUpdates();

	m_is_acquired = false;
	m_acquire.unlock();
}

/// ILC7AccountList //////////////////////////////////////////////////////////////

int CLC7AccountList::GetAccountCount() const
{
	return m_accounts.size();
}

ILC7AccountList::STATS CLC7AccountList::GetStats() const
{
	return m_stats;
}

bool CLC7AccountList::ClearAccounts()
{TR;
	QMutexLocker locker(&m_acquire);

	// Do nothing if we have no accounts
	if(m_accounts.size()==0)
	{
		return true;
	}

	// Clear accounts
	m_accounts.clear();
	
	// Clear remediation actions
	m_remediations.clear();

	// Add action to list of updates
	ACCOUNT_UPDATE_ACTION *act = new ACCOUNT_UPDATE_ACTION;
	act->type = ACCT_CLEAR;
	AddUpdateAction(act);

	clear_stats();
	
	// Session is now modified
	ReportModified();

	return true;
}

const LC7Account *CLC7AccountList::GetAccountAtConstPtrFast(int pos) const
{
	// Ensure we are acquired
	Q_ASSERT(m_is_acquired);

	// Fail if we're out of range
	if(pos>=m_accounts.size())
	{
		return NULL;
	}

	// Return the Nth account
	return &m_accounts.at(pos);
}

bool CLC7AccountList::increase_remediation_refcount(const LC7Account &acct)
{
	// Increase refcount on remediations
	if (acct.remediations != -1)
	{
		if (acct.remediations >= m_remediations.size())
		{
			Q_ASSERT(0);
			return false;
		}
		REMEDIATIONS & cfg = m_remediations[acct.remediations];
		if (cfg.refcount == -1)
		{
			// Found dead remediation config
			Q_ASSERT(0);
			return false;
		}

		// Increase reference count
		cfg.refcount++;
	}

	return true;
}

bool CLC7AccountList::decrease_remediation_refcount(const LC7Account &acct)
{
	// Increase refcount on remediation action
	if (acct.remediations != -1)
	{
		if (acct.remediations >= m_remediations.size())
		{
			Q_ASSERT(0);
			return false;
		}
		REMEDIATIONS & cfg = m_remediations[acct.remediations];
		if (cfg.refcount == -1)
		{
			// Found dead remediation action
			Q_ASSERT(0);
			return false;
		}

		// Increase reference count
		cfg.refcount--;

		// If refcount reaches zero, then nuke this remediation action, allowing it to be reallocated
		if (cfg.refcount == 0)
		{
			cfg.refcount = -1;
		}
	}

	return true;
}


bool CLC7AccountList::AppendAccount(const LC7Account &acct)
{
	TR;

	// Ensure we are acquired
	Q_ASSERT(m_is_acquired);

	if (!increase_remediation_refcount(acct))
	{
		Q_ASSERT(0);
		return false;
	}

	// Append account
	m_accounts.append(acct);
	
	add_to_stats(acct);

	// Add action to list of updates
	ACCOUNT_UPDATE_ACTION *act = new ACCOUNT_UPDATE_ACTION;
	act->type = ACCT_APPEND;
	act->count = 1;
	act->accts.append(acct);
	AddUpdateAction(act);

	// Session is now modified
	ReportModified();

	return true;
}

bool CLC7AccountList::InsertAccount(int pos, const LC7Account &acct)
{
	TR;
	
	// Ensure we are acquired
	Q_ASSERT(m_is_acquired);
	if (pos < 0 || pos >= m_accounts.size())
	{
		return false;
	}
	
	if (!increase_remediation_refcount(acct))
	{
		Q_ASSERT(0);
		return false;
	}

	// Insert account
	m_accounts.insert(pos, acct);
	
	add_to_stats(acct);

	// Add action to list of updates
	ACCOUNT_UPDATE_ACTION *act = new ACCOUNT_UPDATE_ACTION;
	act->type = ACCT_INSERT;
	act->index = pos;
	act->count = 1;
	act->accts.append(acct);
	AddUpdateAction(act);

	// Session is now modified
	ReportModified();

	return true;
}

bool CLC7AccountList::ReplaceAccountAt(int pos, const LC7Account & acct)
{TR;
	// Ensure we are acquired
	Q_ASSERT(m_is_acquired);
	
	// Do nothing if out of range
	if (pos <0 || pos >= m_accounts.size())
	{
		Q_ASSERT(0);
		return false;
	}

	if (!increase_remediation_refcount(acct))
	{
		Q_ASSERT(0);
		return false;
	}
	if (!decrease_remediation_refcount(m_accounts[pos]))
	{
		Q_ASSERT(0);
		return false;
	}

	// Replace account
	remove_from_stats(m_accounts[pos]);
	m_accounts[pos] = acct;
	add_to_stats(acct);
	
	// Add action to list of updates
	ACCOUNT_UPDATE_ACTION *act = new ACCOUNT_UPDATE_ACTION;
	act->type = ACCT_REPLACE;
	act->index = pos;
	act->count = 1;
	act->accts.append(acct);
	AddUpdateAction(act);

	// Session is now modified
	ReportModified();

	return true;
}


bool CLC7AccountList::RemoveAccounts(const std::set<int> &positions)
{TR;
	
	// Ensure we are acquired
	Q_ASSERT(m_is_acquired);

	// Do nothing if out of range
	foreach(int pos, positions)
	{
		if (pos >= m_accounts.size())
		{
			return false;
		}
	}

	// Do get rid of remediations and remove from stats
	int num_removed = 0;
	foreach(int pos, positions)
	{
		if (!decrease_remediation_refcount(m_accounts[pos - num_removed]))
		{
			Q_ASSERT(0);
			return false;
		}

		remove_from_stats(m_accounts[pos - num_removed]);
		m_accounts.removeAt(pos-num_removed);
		num_removed++;
	}

	// Add action to list of updates
	ACCOUNT_UPDATE_ACTION *act = new ACCOUNT_UPDATE_ACTION;
	act->type = ACCT_REMOVE;
	act->positions = positions;
	AddUpdateAction(act);

	// Session is now modified
	ReportModified();

	return true;
}


/// Remediation Actions /////////////////////////////////////////////////////////////////////////////////////////

qint32 CLC7AccountList::AddRemediations(const LC7Remediations &remediations)
{
	// Ensure we are acquired
	Q_ASSERT(m_is_acquired);

	// Find empty slot
	int num=-1;

	REMEDIATIONS * premediations = NULL;
	for (int i = 0; i < m_remediations.size(); i++)
	{
		if (m_remediations[i].refcount == -1)
		{
			premediations = &m_remediations[i];
			num = i;
		}
	}
	if (!premediations)
	{
		num = m_remediations.size();
		REMEDIATIONS r;
		r.refcount = -1;
		r.remediations = remediations;
		m_remediations.append(r);
		premediations = &m_remediations[num];
	}

	(premediations->refcount)++;

	return num;
}

bool CLC7AccountList::GetRemediations(qint32 num, LC7Remediations &remediations)
{
	// Ensure we are acquired
	Q_ASSERT(m_is_acquired);

	if (num < 0 || num >= m_remediations.size())
	{
		Q_ASSERT(0);
		return false;
	}

	REMEDIATIONS *pcfg = &m_remediations[num];
	if (pcfg->refcount == -1)
	{
		Q_ASSERT(0);
		return false;
	}

	remediations = pcfg->remediations;

	return true;
}


/// Serialization /////////////////////////////////////////////////////////////////////////////////////////

bool CLC7AccountList::Save(QDataStream & out)
{TR;
	QMutexLocker locker(&m_acquire);

	quint32 version = 2;
	out << version;

	quint32 cnt=m_accounts.size();
	out << cnt;

	out << m_stats.total;
	out << m_stats.cracked;
	out << m_stats.partially_cracked;
	out << m_stats.locked_out;
	out << m_stats.disabled;
	out << m_stats.must_change;
	out << m_stats.non_expiring;

	for(quint32 i=0;i<cnt;i++)
	{
		const LC7Account & acct = m_accounts.at(i);

		out << acct.username;		// Account user name
		out << acct.userinfo;		// Account user info
		out << acct.userid;			// User ID

		quint32 hashcount = acct.hashes.size();
		out << hashcount;

		foreach(LC7Hash lc7hash, acct.hashes)
		{
			out << lc7hash.hashtype;			// Hash type code
			out << lc7hash.hash;			// Account hash in parseable text format
			out << lc7hash.password;			// Account password as-cracked
			out << lc7hash.crackstate;			// Crack-state
			out << lc7hash.cracktime;			// How long it took to crack (seconds)
			out << lc7hash.cracktype;			// What type of crack succeeded
		}

		out << acct.domain;			// Account domain
		out << acct.machine;		// Machine address
		out << acct.lastchanged;	// Password age in days
		out << acct.flags;			// Flags
		out << acct.remediations;		// Remediation action number
	}

	quint32 rcnt = m_remediations.size();
	out << rcnt;

	for (quint32 i = 0; i < rcnt; i++)
	{
		const REMEDIATIONS & remediations = m_remediations.at(i);
		
		out << remediations.refcount;
		out << remediations.remediations;
	}

	if (out.status()!=QDataStream::Ok)
	{
		return false;
	}
	return true;
}

bool CLC7AccountList::Load(QDataStream & in)
{TR;
	ClearAccounts();

	quint32 version;
	in >> version;

	if (version > 2)
	{
		return false;
	}

	quint32 cnt;
	in >> cnt;
		
	in >> m_stats.total;
	in >> m_stats.cracked;
	in >> m_stats.partially_cracked;
	in >> m_stats.locked_out;
	in >> m_stats.disabled;
	in >> m_stats.must_change;
	in >> m_stats.non_expiring;

	if (cnt > 0)
	{
		for (quint32 i = 0; i < cnt; i++)
		{
			LC7Account acct;

			in >> acct.username;		// Account user name
			in >> acct.userinfo;		// Account user info
			in >> acct.userid;			// User ID

			quint32 hashcount;
			in >> hashcount;
			for (quint32 h = 0; h < hashcount; h++)
			{
				LC7Hash lc7hash;
				in >> lc7hash.hashtype;			// Hash type code
				in >> lc7hash.hash;			// Account hash in parseable text format
				in >> lc7hash.password;			// Account password as-cracked
				in >> lc7hash.crackstate;			// Crack-state
				in >> lc7hash.cracktime;			// How long it took to crack (seconds)
				in >> lc7hash.cracktype;			// What type of crack succeeded

				acct.hashes.append(lc7hash);
			}

			in >> acct.domain;			// Account domain
			in >> acct.machine;			// Machine address
			in >> acct.lastchanged;		// Password age in days
			in >> acct.flags;			// Flags

			if (version >= 2)
			{
				in >> acct.remediations;
			}
			else
			{
				acct.remediations = -1;
			}

			m_accounts.append(acct);
		}
	}

	if (version >= 2)
	{
		quint32 rcnt;
		in >> rcnt;

		m_remediations.clear();

		for (quint32 i = 0; i < rcnt; i++)
		{
			REMEDIATIONS remediations;

			in >> remediations.refcount;
			in >> remediations.remediations;

			m_remediations.append(remediations);
		}
	}

	if (in.status() != QDataStream::Ok)
	{
		return false;
	}

	return true;
}


/// Stats ////////////////////////////////////////////////////////////////////////////////////

static int get_crackstatesum(const LC7Account *acct)
{
	if (acct->hashes.size() == 0)
	{
		return -1;
	}

	int num_uncracked = 0;
	int num_cracked = 0;

	foreach(LC7Hash lc7hash, acct->hashes)
	{
		if (lc7hash.crackstate == CRACKSTATE_FIRSTHALF_CRACKED || lc7hash.crackstate == CRACKSTATE_SECONDHALF_CRACKED)
		{
			return 1;
		}
		if (lc7hash.crackstate == CRACKSTATE_CRACKED)
		{
			num_cracked++;
		}
		if (lc7hash.crackstate == CRACKSTATE_NOT_CRACKED)
		{
			num_uncracked++;
		}
	}
	if (num_uncracked == acct->hashes.size())
	{
		return 0;
	}
	if (num_cracked == acct->hashes.size())
	{
		return 2;
	}
	return 1;
}

void CLC7AccountList::clear_stats(void)
{
	memset(&m_stats, 0, sizeof(m_stats));
}


void CLC7AccountList::add_to_stats(const LC7Account &acct)
{
	m_stats.total++;
	
	if (acct.lockedout)
	{
		m_stats.locked_out++;
	}
	
	if (acct.disabled)
	{
		m_stats.disabled++;
	}
	
	if (acct.mustchange)
	{
		m_stats.must_change++;
	}
	
	if (acct.neverexpires)
	{
		m_stats.non_expiring++;
	}

	int crackstatesum = get_crackstatesum(&acct);
	if (crackstatesum == 1)
	{
		m_stats.partially_cracked++;
	}
	else if (crackstatesum == 2)
	{
		m_stats.cracked++;
	}
}

void CLC7AccountList::remove_from_stats(const LC7Account &acct)
{
	m_stats.total--;
	
	if (acct.lockedout)
	{
		m_stats.locked_out--;
	}

	if (acct.disabled)
	{
		m_stats.disabled--;
	}

	if (acct.mustchange)
	{
		m_stats.must_change--;
	}

	if (acct.neverexpires)
	{
		m_stats.non_expiring--;
	}

	int crackstatesum = get_crackstatesum(&acct);
	if (crackstatesum == 1)
	{
		m_stats.partially_cracked--;
	}
	else if (crackstatesum == 2)
	{
		m_stats.cracked--;
	}
}


