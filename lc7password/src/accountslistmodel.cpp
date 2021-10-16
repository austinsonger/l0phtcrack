#include"stdafx.h"
#include<set>

#undef TR
#define TR

typedef std::pair<int, int> Interval;

#define SORTKEY_NOT_CRACKED 0xFFFFFFFF
#define SORTKEY_FIRSTHALF_CRACKED 0xFFFFFFFE
#define SORTKEY_SECONDHALF_CRACKED 0xFFFFFFFD
#define SORTKEY_NO_HASH 0xFFFFFFFC

template<class InputIterator, class OutputIterator>
void setToIntervals(InputIterator first, InputIterator last, OutputIterator dest)
{
	typedef std::iterator_traits<InputIterator>::value_type item_type;
	typedef typename std::pair<item_type, item_type> pair_type;
	pair_type r(-std::numeric_limits<item_type>::max(),
		-std::numeric_limits<item_type>::max());

	for (; first != last; ++first)
	{
		item_type i = *first;
		if (i != r.second + 1)
		{
			if (r.second >= 0)
				*dest = r;
			r.first = i;
		}
		r.second = i;
	}
	*dest = r;
}


AccountsListModel::AccountsListModel(QObject *parent, ILC7AccountList *accounts)
	:QAbstractTableModel(parent)
{
	ENSURE_GUI_THREAD;

	m_accounts = accounts;

	m_columncount = 0;
	m_rowcount = 0;

	m_progress_box = NULL;

	if (m_accounts)
	{
		m_accounts->RegisterUpdateNotification(this, (void(QObject::*)(ILC7AccountList::ACCOUNT_UPDATE_ACTION_LIST)) &AccountsListModel::slot_update_action);

		do_full_update();
	}

	m_colman = g_pLinkage->GetGUILinkage()->GetColorManager();
	m_colman->RegisterRecolorCallback(this, (void (QObject::*)(void))&AccountsListModel::slot_recolor_callback);

	m_enableSortingTimer.setSingleShot(true);
	connect(&m_enableSortingTimer, &QTimer::timeout, this, &AccountsListModel::slot_enable_sorting_timer);

	slot_recolor_callback();
}

AccountsListModel::~AccountsListModel()
{TR;
	ENSURE_GUI_THREAD;
}

void AccountsListModel::setProgressBox(ILC7ProgressBox *box)
{
	m_progress_box = box;
}


void AccountsListModel::slot_recolor_callback(void)
{
	m_check_pixmap = m_colman->GetHueColorPixmapResource(":/lc7/green_check.png");
	m_blank_pixmap = m_colman->GetHueColorPixmapResource(":/lc7/blank11.png");
	m_textcolor = QColor(m_colman->GetTextColor());
	m_inversetextcolor = QColor(m_colman->GetInverseTextColor());
	m_lock_icon = m_colman->GetMonoColorIcon(":/lc7/lock.png", m_textcolor, m_textcolor, m_inversetextcolor);
	m_disabled_icon = m_colman->GetMonoColorIcon(":/lc7/disabled.png", m_textcolor, m_textcolor, m_inversetextcolor);
	m_expired_icon = m_colman->GetMonoColorIcon(":/lc7/expired.png", m_textcolor, m_textcolor, m_inversetextcolor);
	m_infinity_icon = m_colman->GetMonoColorIcon(":/lc7/infinity.png", m_textcolor, m_textcolor, m_inversetextcolor);
	
	beginResetModel();
	endResetModel();

	emit sig_modelchanged();
}

void AccountsListModel::do_full_update(void)
{
	m_accounts->Acquire();

	beginResetModel();

	update_action_clear();

	for (int i = 0; i < m_accounts->GetAccountCount(); i++)
	{
		update_action_append(m_accounts->GetAccountAtConstPtrFast(i));
	}

	m_accounts->Release();
	
	endResetModel();

	emit sig_modelchanged();
}

void AccountsListModel::update_action_clear()
{
	m_normal_table.clear();
	m_hash_table.clear();
	m_rowcount = 0;
	m_columncount = 0;
	m_normal_column_count.resize(10);
	for (int i = 0; i < 10; i++)
	{
		m_normal_column_count[i] = 0;
	}
	m_hash_column_count.resize(0);
}

void AccountsListModel::update_column_count(void)
{
	// Update total column count
	m_columncount = 0;
	foreach(int cnt, m_normal_column_count)
	{
		if (cnt > 0)
		{
			m_columncount++;
		}
	}

	// Always include 3 columns per hash type
	m_columncount += m_hash_column_count.size() * 3;
}

void AccountsListModel::add_columns(const LC7Account *acct)
{
	if (!acct->domain.isEmpty())
	{
		m_normal_column_count[0]++;
		if (m_normal_column_count[0] == 1)
		{
			m_columnschanged = true;
		}
	}
	if (!acct->username.isEmpty())
	{
		m_normal_column_count[1]++;
		if (m_normal_column_count[1] == 1)
		{
			m_columnschanged = true;
		}
	}
	if (!acct->userinfo.isEmpty())
	{
		m_normal_column_count[2]++;
		if (m_normal_column_count[2] == 1)
		{
			m_columnschanged = true;
		}
	}
	if (!acct->userid.isEmpty())
	{
		m_normal_column_count[3]++;
		if (m_normal_column_count[3] == 1)
		{
			m_columnschanged = true;
		}
	}
	if (!acct->machine.isEmpty())
	{
		m_normal_column_count[4]++;
		if (m_normal_column_count[4] == 1)
		{
			m_columnschanged = true;
		}
	}
	if (acct->lastchanged!=0)
	{
		m_normal_column_count[5]++;
		if (m_normal_column_count[5] == 1)
		{
			m_columnschanged = true;
		}
	}

	// Always show status columns
	m_normal_column_count[6]++;
	if (m_normal_column_count[6] == 1)
	{
		m_columnschanged = true;
	}
	m_normal_column_count[7]++;
	if (m_normal_column_count[7] == 1)
	{
		m_columnschanged = true;
	}
	m_normal_column_count[8]++;
	if (m_normal_column_count[8] == 1)
	{
		m_columnschanged = true;
	}
	m_normal_column_count[9]++;
	if (m_normal_column_count[9] == 1)
	{
		m_columnschanged = true;
	}

	// Add hash columns
	foreach(LC7Hash hash, acct->hashes)
	{
		bool found = false;
		for (int hcol = 0; hcol< m_hash_column_count.size(); hcol++)
		{
			if (m_hash_column_count[hcol].hashtype == hash.hashtype)
			{
				m_hash_column_count[hcol].count++;
				found = true;
			}
		}
		if (!found)
		{
			HASH_COLUMN col;
			col.hashtype = hash.hashtype;
			col.count = 1;
			m_hash_column_count.append(col);

			// Build empty column for this hash type
			QList<QVector<QVariant>> hashcol;
			for (int r = 0; r < m_rowcount; r++)
			{
				QVector<QVariant> row(4,QVariant());
				
//				row.append(QVariant());
//				row.append(QVariant());
//				row.append(QVariant());
//				row.append(QVariant());	// Add extra column for sort key for status

				hashcol.append(row);
			}

			m_hash_table.insert(hash.hashtype, hashcol);
			
			m_columnschanged = true;
		}
	}

	update_column_count();
}

void AccountsListModel::remove_columns(int pos)
{
	for (int col = 0; col < 10; col++)
	{
		if (!m_normal_table[pos][col].isNull())
		{
			m_normal_column_count[col]--;
			if (m_normal_column_count[col] == 0)
			{
				m_columnschanged = true;
			}
		}
	}

	// Remove hash columns	
	QList<int> deadcolumns;
	for (int col = 0; col < m_hash_column_count.size(); col++)
	{
		fourcc hashtype = m_hash_column_count[col].hashtype;
		const QVector<QVariant> & htcols = m_hash_table[hashtype][pos];
		if (htcols[0].isNull())
		{
			// Doesn't have hash type
		}
		else
		{
			// Does have hash type, decrement
			m_hash_column_count[col].count--;
			if (m_hash_column_count[col].count == 0)
			{
				deadcolumns.append(col);
				m_columnschanged = true;
			}
		}
	}

	// Purge empty hash columns
	int numremoved = 0;
	foreach(int deadcolumn, deadcolumns)
	{
		m_hash_table.remove(m_hash_column_count[deadcolumn - numremoved].hashtype);
		m_hash_column_count.removeAt(deadcolumn - numremoved);
		numremoved++;
	}
	
	update_column_count();
}

static void generate_normal_table_row(QVector<QVariant> & normal_row, const LC7Account *acct)
{
	Q_ASSERT(normal_row.size() == 10);

	if (!acct->domain.isEmpty())
	{
		normal_row[0]=acct->domain;
	}
	
	if (!acct->username.isEmpty())
	{
		normal_row[1]=acct->username;
	}
	
	if (!acct->userinfo.isEmpty())
	{
		normal_row[2]=acct->userinfo;
	}
	
	if (!acct->userid.isEmpty())
	{
		normal_row[3]=acct->userid;
	}
	
	if (!acct->machine.isEmpty())
	{
		normal_row[4]=acct->machine;
	}
	
	if (acct->lastchanged != 0)
	{
		normal_row[5]=acct->lastchanged;
	}
	
	normal_row[6]=acct->lockedout;
	normal_row[7]=acct->disabled;
	normal_row[8]=acct->mustchange;
	normal_row[9]=acct->neverexpires;
}

static void generate_hash_table_row(QVector<QVariant> & hash_row, fourcc hashtype, const LC7Account *acct)
{
	bool found = false;
	
	Q_ASSERT(hash_row.size() == 4);

	foreach(LC7Hash lc7hash, acct->hashes)
	{
		if (lc7hash.hashtype == hashtype)
		{
			hash_row[0]=QString::fromLatin1(lc7hash.hash);
			hash_row[1]=lc7hash.password;

			// status
			qulonglong statussortkey;
			QString status;
			if (lc7hash.crackstate == CRACKSTATE_NOT_CRACKED)
			{
				status = "Not Cracked";
				statussortkey = SORTKEY_NOT_CRACKED;
			}
			else if (lc7hash.crackstate == CRACKSTATE_FIRSTHALF_CRACKED)
			{
				status = "1st Half Cracked";
				statussortkey = SORTKEY_FIRSTHALF_CRACKED;
			}
			else if (lc7hash.crackstate == CRACKSTATE_SECONDHALF_CRACKED)
			{
				status = "2nd Half Cracked";
				statussortkey = SORTKEY_SECONDHALF_CRACKED;
			}
			else
			{
				if (!lc7hash.cracktype.isEmpty())
				{
					status = QString("Cracked (%1): ").arg(lc7hash.cracktype);
				}
				else
				{
					status = QString("Cracked: ");
				}
				quint32 secs = lc7hash.cracktime % 60;
				quint32 mins = (lc7hash.cracktime / 60) % 60;
				quint32 hrs = (lc7hash.cracktime / 3600) % 24;
				quint32 days = (lc7hash.cracktime / (3600 * 24));

				QString out;
				if (days > 0)
				{
					out += QString("%1d").arg(days);
				}
				if (hrs > 0 || out.size() > 0)
				{
					out += QString("%1h").arg(hrs);
				}
				if (mins > 0 || out.size() > 0)
				{
					out += QString("%1m").arg(mins);
				}
				if (secs > 0 || out.size() > 0)
				{
					out += QString("%1s").arg(secs);
				}
				if (out.size() == 0)
				{
					out = "instantly";
				}
				status += out;

				statussortkey = lc7hash.cracktime;
			}

			hash_row[2]=status;
			hash_row[3]=statussortkey;

			found = true;
			break;
		}
	}
	if (!found)
	{
		hash_row[0]=QVariant();
		hash_row[1]=QVariant();
		hash_row[2]=QString("No Password Hash");
		hash_row[3] = qulonglong(SORTKEY_NO_HASH);
	}
}

void AccountsListModel::update_action_append(const LC7Account *acct)
{
	// Ensure we have all hash columns for the hash types we've seen so far
	add_columns(acct);

	// Add row for all normal columns
	QVector<QVariant> normal_row(10,QVariant());
	generate_normal_table_row(normal_row, acct);
	
	m_normal_table.append(normal_row);
	
	// Add row for all hash columns
	foreach(HASH_COLUMN hcol, m_hash_column_count)
	{
		QList<QVector<QVariant> > & hash_table = m_hash_table[hcol.hashtype];
	
		QVector<QVariant> hash_row(4,QVariant());
		generate_hash_table_row(hash_row, hcol.hashtype, acct);

		hash_table.append(hash_row);
	}

	m_rowcount++;
}

void AccountsListModel::update_action_insert(int pos, const LC7Account *acct)
{
	// Ensure we have all hash columns for the hash types we've seen so far
	add_columns(acct);

	// Add row for all normal columns
	QVector<QVariant> normal_row(10,QVariant());
	generate_normal_table_row(normal_row, acct);

	m_normal_table.insert(pos, normal_row);

	// Add row for all hash columns
	foreach(HASH_COLUMN hcol, m_hash_column_count)
	{
		QList<QVector<QVariant> > & hash_table = m_hash_table[hcol.hashtype];

		QVector<QVariant> hash_row(4,QVariant());
		generate_hash_table_row(hash_row, hcol.hashtype, acct);

		hash_table.insert(pos, hash_row);
	}

	m_rowcount++;
}

void AccountsListModel::update_action_replace(int pos, const LC7Account *acct)
{
	// Remove any hash columns for which we have no hashes any more
	remove_columns(pos);

	// Ensure we have all hash columns for the hash types we've seen so far
	add_columns(acct);

	// Add row for all normal columns
	QVector<QVariant> normal_row(10,QVariant());
	generate_normal_table_row(normal_row, acct);

	m_normal_table.replace(pos, normal_row);

	// Add row for all hash columns
	foreach(HASH_COLUMN hcol, m_hash_column_count)
	{
		QList<QVector<QVariant> > & hash_table = m_hash_table[hcol.hashtype];

		QVector<QVariant> hash_row(4,QVariant());
		generate_hash_table_row(hash_row, hcol.hashtype, acct);

		hash_table.replace(pos, hash_row);
	}
}

void AccountsListModel::update_action_remove(int pos)
{
	// Remove any hash columns for which we have no hashes any more
	remove_columns(pos);

	// Remove row for all normal columns
	m_normal_table.removeAt(pos);

	// Remove row for all hash columns
	foreach(HASH_COLUMN hcol, m_hash_column_count)
	{
		QList<QVector<QVariant> > & hash_table = m_hash_table[hcol.hashtype];

		hash_table.removeAt(pos);
	}

	m_rowcount--;
}

void AccountsListModel::disableSortingBriefly()
{
	m_enableSortingTimer.start(2000);
	emit sig_enable_sorting(false);
}

void AccountsListModel::slot_enable_sorting_timer(void)
{
	emit sig_enable_sorting(true);
}

void AccountsListModel::slot_update_action(ILC7AccountList::ACCOUNT_UPDATE_ACTION_LIST updates)
{
	emit sig_enable_sorting(false);

//	disableSortingBriefly();

	m_columnschanged = false;

	foreach(ILC7AccountList::ACCOUNT_UPDATE_ACTION *update, updates)
	{
		// execute batched operation
		switch (update->type)
		{
		case ILC7AccountList::ACCT_CLEAR:
			beginResetModel();
			update_action_clear();
			endResetModel();
		
			break;

		case ILC7AccountList::ACCT_APPEND:
			beginInsertRows(QModelIndex(), m_rowcount, m_rowcount + update->count - 1);
			foreach(const LC7Account &acct, update->accts)
			{
				update_action_append(&acct);
			}
			endInsertRows();

			break;

		case ILC7AccountList::ACCT_INSERT:
			{
				beginInsertRows(QModelIndex(), update->index, update->index + update->count - 1);
				int idx = update->index;
				foreach(const LC7Account &acct, update->accts)
				{
					update_action_insert(idx, &acct);
					idx++;
				}
				endInsertRows();

			}
			break;
		case ILC7AccountList::ACCT_REPLACE:
			{
				int idx = update->index;
				foreach(const LC7Account &acct, update->accts)
				{
					update_action_replace(idx, &acct);
					idx++;
				}
				
				emit dataChanged(createIndex(update->index, 0), createIndex(update->index + update->count - 1, m_columncount - 1));
			}
			break;
		case ILC7AccountList::ACCT_REMOVE:
			{
				int count = (int)update->positions.size();
			
				std::vector<Interval> intervals;
				setToIntervals(update->positions.begin(), update->positions.end(), std::back_inserter(intervals));

				int num_removed = 0;
				for (auto it : intervals)
				{
					beginRemoveRows(QModelIndex(), it.first - num_removed, it.second - num_removed);
					for (int i = 0; i < ((it.second-it.first)+1); i++)
					{
						update_action_remove((it.first + i) - num_removed);
						num_removed++;
						if ((num_removed & 15) == 0)
						{
							if (m_progress_box)
							{
								m_progress_box->UpdateProgressBar(num_removed);
								QCoreApplication::processEvents();
							}
						}
					}
					endRemoveRows();
				}

				if (m_progress_box)
				{
					QCoreApplication::processEvents();
				}
	
				break;
			}
		}

		delete update;
	}

	if (m_columnschanged)
	{
		// Cheap hack, could be better.
		beginResetModel();
		endResetModel();
	}

	emit sig_modelchanged();

	emit sig_enable_sorting(true);
}


int AccountsListModel::rowCount(const QModelIndex & /*parent*/) const
{TR;
	ENSURE_GUI_THREAD;
	return m_rowcount;
}

int AccountsListModel::columnCount(const QModelIndex & /*parent*/) const
{
	ENSURE_GUI_THREAD;
	return m_columncount;
}


bool AccountsListModel::get_column_type(int realcolumn, int & col, fourcc &hashtype, int &hcolnum) const
{
	TR;
	ENSURE_GUI_THREAD;

	int nth = realcolumn;

	if (m_normal_column_count[0])
	{
		if (nth == 0)
		{
			col = 0;
			hcolnum = 0;
			return false;
		}
		nth--;
	}
	if (m_normal_column_count[1])
	{
		if (nth == 0)
		{
			col = 1;
			hcolnum = 0;
			return false;
		}
		nth--;
	}

	if (nth < (m_hash_column_count.size() * 3))
	{
		hashtype = m_hash_column_count[nth / 3].hashtype;
		col = nth % 3;
		hcolnum = nth / 3;
		return true;
	}
	nth -= (m_hash_column_count.size() * 3);

	for (col = 2; col < m_normal_column_count.size(); col++)
	{
		if (m_normal_column_count[col])
		{
			if (nth == 0)
			{
				return false;
			}
			nth--;
		}
	}

	Q_ASSERT(0);
	return false;
}

int AccountsListModel::get_crackstatecol(int row, int hcolnum) const
{
	bool has_hash = false;
	bool has_cracked = false;
	bool has_partial_crack = false;
	
	const HASH_COLUMN & hcol = m_hash_column_count[hcolnum];

	if (m_hash_table[hcol.hashtype][row][3].isValid())
	{
		quint32 sortkey = m_hash_table[hcol.hashtype][row][3].toUInt();
		if (sortkey == SORTKEY_NOT_CRACKED)
		{
			has_hash = true;
		}
		else if (sortkey == SORTKEY_FIRSTHALF_CRACKED || sortkey == SORTKEY_SECONDHALF_CRACKED)
		{
			has_hash = true;
			has_partial_crack = true;
		}
		else if (sortkey == SORTKEY_NO_HASH)
		{
			has_hash = false;
		}
		else
		{
			has_hash = true;
			has_cracked = true;
		}
	}

	if (!has_hash)
	{
		return -1;
	}
	if (has_partial_crack)
	{
		return 1;
	}
	if (has_cracked)
	{
		return 2;
	}
	return 0;
}

int AccountsListModel::get_crackstatesum(int row) const
{
	bool has_hash = false;
	bool has_uncracked = false;
	bool has_cracked = false;
	bool has_partial_crack = false;
	for (int i = 0; i < m_hash_column_count.size();i++)
	{
		int crackstate = get_crackstatecol(row, i);

		if (crackstate == 0)
		{
			has_hash = true;
			has_uncracked = true;
		}
		else if (crackstate == 1)
		{
			has_hash = true;
			has_partial_crack = true;
		}
		else if (crackstate == 2)
		{
			has_hash = true;
			has_cracked = true;
		}
	}
	
	if (!has_hash)
	{
		return -1;
	}
	if ((has_partial_crack && !has_uncracked) ||
		(has_cracked && has_uncracked))
	{
		return 1;
	}
	if (has_cracked)
	{
		return 2;
	}
	return 0;

}


QVariant AccountsListModel::data(const QModelIndex &index, int role) const
{TR;
	ENSURE_GUI_THREAD;
	
	if (!m_accounts)
	{
		return QVariant();
	}
	int row = index.row();
	int realcol = index.column();
	if (row < 0 || row >= m_rowcount)
	{
		return QVariant(); 
	}
	if (realcol < 0 || realcol >= m_columncount)
	{
		return QVariant();
	}
		
	int sizeratio = m_colman->GetSizeRatio();

	bool is_hash_column;
	int col;
	fourcc hashtype;
	int hcolnum;
	is_hash_column = get_column_type(realcol, col, hashtype, hcolnum);

	bool obscure = g_pLinkage->GetSettings()->value(UUID_PASSWORDUIOPTIONS.toString() + ":obscure_passwords", QVariant(true)).toBool();

	if (role == Qt::DisplayRole)
	{
		if (is_hash_column)
		{
			const QList<QVector<QVariant> > & hash_table = m_hash_table[hashtype];
			bool found = false;
			
			if (col == 0)
			{
				// hash
				return hash_table[row][0];
			}
			else if (col == 1)
			{
				// password
				if (obscure)
				{
					return QString(hash_table[row][1].toString().length(), '*');
				}
				return hash_table[row][1];
			}
			else if (col == 2)
			{
				// status
				return hash_table[row][2];
			}
		}
		else
		{
			switch (col)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
				return m_normal_table[row][col];
			case 5:
			{
				quint64 ullAge = m_normal_table[row][col].toULongLong();
				if (ullAge == 0)
					return "Never";
				return QDateTime::fromTime_t(ullAge).toString(Qt::SystemLocaleShortDate);
			}
			case 6:
			case 7:
			case 8:
			case 9:
			default:
				break;
			}
		}
	}
	if (role == Qt::AccessibleTextRole)
	{
		if (is_hash_column)
		{
			const QList<QVector<QVariant> > & hash_table = m_hash_table[hashtype];
			bool found = false;

			if (col == 0)
			{
				// hash
				return hash_table[row][0];
			}
			else if (col == 1)
			{
				// password
				if (obscure)
				{
					return QString(hash_table[row][1].toString().length(), '*');
				}
			}
			else if (col == 2)
			{
				// status
				return hash_table[row][2];
			}
		}
		else
		{
			switch (col)
			{
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
				return m_normal_table[row][col];
			case 5:
			{
				quint64 ullAge = m_normal_table[row][col].toULongLong();
				if (ullAge == 0)
					return "Never";
				return QDateTime::fromTime_t(ullAge).toString(Qt::SystemLocaleShortDate);
			}
			case 6:
			case 7:
			case 8:
			case 9:
			default:
				return m_normal_table[row][col].toBool() ? "X" : "";
				break;
			}
		}
	}
	else if (role == Qt::DecorationRole)
	{
		if (!is_hash_column)
		{
			switch (col)
			{
			default:
				break;
			case 6:
			case 7:
			case 8:
			case 9:
				return m_normal_table[row][col].toBool() ? m_check_pixmap : m_blank_pixmap;
			}
		}
	}
	else if (role == Qt::BackgroundRole)
	{
		int cs;
		cs = get_crackstatesum(row);
		if (is_hash_column)
		{
			int colcs = get_crackstatecol(row, hcolnum);
			if (colcs > cs)
			{
				cs = colcs;
			}
		}

		if (cs == -1)		// No hash
		{
			return QColor(0x8f, 0x8f, 0x8f);
		}
		else if (cs == 0)	// Not Cracked	
		{
			return QVariant();
		}
		else if (cs == 1)	// Partially cracked
		{
			return QColor(0xd6, 0xbd, 0x30);
		}
		else if (cs == 2)	// Cracked
		{
			return QColor(0xcf, 0x3a, 0x3a);
		}
	}
	else if (role == Qt::TextColorRole)
	{
		int cs;
		cs = get_crackstatesum(row);
		if (is_hash_column)
		{
			int colcs = get_crackstatecol(row, hcolnum);
			if (colcs > cs)
			{
				cs = colcs;
			}
		}

		if (cs == 0)
		{
			return m_textcolor;
		}
		else
		{
			return QColor(0, 0, 0);
		}
	}
	else if (role == Qt::ToolTipRole)
	{
		if (!is_hash_column)
		{
			switch (col)
			{
			default:
				break;
			case 6:
				return m_normal_table[row][col].toBool() ? QVariant("Account is locked out") : QVariant();
			case 7:
				return m_normal_table[row][col].toBool() ? QVariant("Account is disabled") : QVariant();
			case 8:
				return m_normal_table[row][col].toBool() ? QVariant("Account password must be changed") : QVariant();
			case 9:
				return m_normal_table[row][col].toBool() ? QVariant("Account password never expires") : QVariant();
			}
		}
	}
	return QVariant();

}

QVariant AccountsListModel::verticalHeaderData(int section, int role) const
{TR;
	ENSURE_GUI_THREAD;

	int sizeratio = m_colman->GetSizeRatio();

	quint32 cnt = 0;
	if (m_accounts)
	{
		cnt = (quint32)m_accounts->GetAccountCount();
	}

	if (role == Qt::DisplayRole || role == Qt::UserRole || role == Qt::ToolTipRole)
	{
		if (cnt == 0)
		{
			return QVariant();
		}

		return QString(" %1 ").arg(section + 1);
	}
	else if (role == Qt::SizeHintRole)
	{
		if (cnt == 0)
		{
			return QSize(24 * sizeratio, 24 * sizeratio);
		}
	}

	return QVariant();
}

QVariant AccountsListModel::horizontalHeaderData(int section, int role) const
{TR;
	ENSURE_GUI_THREAD;
	
	int sizeratio = m_colman->GetSizeRatio();

	bool is_hash_column;
	int col;
	fourcc hashtype;
	int hcolnum;
	is_hash_column = get_column_type(section, col, hashtype, hcolnum);

	if (role == Qt::DisplayRole || role == Qt::UserRole || role == Qt::ToolTipRole)
	{
		if (is_hash_column)
		{
			QString out;

			ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
			LC7HashType lc7hashtype;
			QString err;
			if (!passlink->LookupHashType(hashtype, lc7hashtype, err))
			{
				out = "Unknown";
			}
			else
			{
				out = lc7hashtype.name;
			}

			if (col == 0)
			{
				out += " Hash";
			}
			else if (col == 1)
			{
				out += " Password";
			}
			else if (col == 2)
			{
				out += " State";
			}

			return out;
		}
		else
		{
			switch (col)
			{
			case 0:
				return "Domain";
			case 1:
				return "Username";
			case 2:
				return "User Info";
			case 3:
				return "User Id";
			case 4:
				return "Machine";
			case 5:
				return "Last Changed";
			case 6:
				if (role == Qt::UserRole || role == Qt::ToolTipRole)
					return "Lockout";
				break;
			case 7:
				if (role == Qt::UserRole || role == Qt::ToolTipRole)
					return "Disabled";
				break;
			case 8:
				if (role == Qt::UserRole || role == Qt::ToolTipRole)
					return "Password Must Change";
				break;
			case 9:
				if (role == Qt::UserRole || role == Qt::ToolTipRole)
					return "No Password Expiration";
				break;
			}
		}
	}
	else if (role == Qt::DecorationRole)
	{
		if (is_hash_column)
		{
		}
		else
		{
			switch (col)
			{
			case 6:
				return m_lock_icon;
			case 7:
				return m_disabled_icon;
			case 8:
				return m_expired_icon;
			case 9:
				return m_infinity_icon;
			default:
				break;
			}
		}
	}
	else if (role == Qt::SizeHintRole)
	{
		if (is_hash_column)
		{
		}
		else
		{
			switch (col)
			{
			case 6:
				//return "Lockout";
				return QSize(26 * sizeratio, 26 * sizeratio);
			case 7:
				//return "Disabled";
				return QSize(26 * sizeratio, 26 * sizeratio);
			case 8:
				//return "Password Must Change";
				return QSize(26 * sizeratio, 26 * sizeratio);
			case 9:
				//return "Password Never Expires";
				return QSize(26 * sizeratio, 26 * sizeratio);
			default:
				break;
			}
		}
	}

	return QVariant();
}

QVariant AccountsListModel::headerData(int section, Qt::Orientation orientation, int role) const
{TR;
	ENSURE_GUI_THREAD;

	if (orientation == Qt::Vertical)
	{
		return verticalHeaderData(section, role);
	}
	else if (orientation == Qt::Horizontal)
	{
		return horizontalHeaderData(section, role);
	}

	Q_ASSERT(0);
	return QVariant();
}


static bool lessThan_Int_or_String(QString a, QString b)
{
	bool is_int1;
	bool is_int2;
	int i1 = a.toInt(&is_int1, 10);
	int i2 = b.toInt(&is_int2, 10);
	if (is_int1 && is_int2)
	{
		if (i1 < i2) return true;
		if (i1 > i2) return false;
		return false;
	}
	return QString::compare(a, b, Qt::CaseInsensitive);
}

bool AccountsListModel::lessThan(const QModelIndex & left, const QModelIndex & right) const
{
	if (left.column() < right.column())
	{
		return true;
	}
	if (left.column() > right.column())
	{
		return false;
	}

	bool is_hash_column;
	int col;
	fourcc hashtype;
	int hcolnum;
	is_hash_column = get_column_type(left.column(), col, hashtype, hcolnum);

	if (is_hash_column)
	{
		const QList<QVector<QVariant> > & hash_table = m_hash_table[hashtype];
		const QVector<QVariant> & leftrow = hash_table[left.row()];
		const QVector<QVariant> & rightrow = hash_table[right.row()];

		if (col == 0)
		{
			return QString::compare(leftrow[col].toString(), rightrow[col].toString(), Qt::CaseInsensitive) < 0;
		}
		else if (col == 1)
		{
			return QString::compare(leftrow[col].toString(), rightrow[col].toString(), Qt::CaseSensitive) < 0;
		}
		else if (col == 2)
		{
			// Use sort key instead of this column's text
			return leftrow[3].toUInt() < rightrow[3].toUInt();
		}
	}
	else
	{
		const QVector<QVariant> & leftrow = m_normal_table[left.row()];
		const QVector<QVariant> & rightrow = m_normal_table[right.row()];

		switch (col)
		{
		case 0:
			return QString::compare(leftrow[col].toString(), rightrow[col].toString(), Qt::CaseInsensitive) < 0;
		case 1:
			return QString::compare(leftrow[col].toString(), rightrow[col].toString(), Qt::CaseInsensitive) < 0;
		case 2:
			return QString::compare(leftrow[col].toString(), rightrow[col].toString(), Qt::CaseInsensitive) < 0;
		case 3:
			return lessThan_Int_or_String(leftrow[col].toString(), rightrow[col].toString());
		case 4:
			return QString::compare(leftrow[col].toString(), rightrow[col].toString(), Qt::CaseInsensitive) < 0;
		case 5:
			return leftrow[col].toULongLong() < rightrow[col].toULongLong();
		case 6:
			return (leftrow[col].toBool() ? 1 : 0) < (rightrow[col].toBool() ? 1 : 0);
		case 7:
			return (leftrow[col].toBool() ? 1 : 0) < (rightrow[col].toBool() ? 1 : 0);
		case 8:
			return (leftrow[col].toBool() ? 1 : 0) < (rightrow[col].toBool() ? 1 : 0);
		case 9:
			return (leftrow[col].toBool() ? 1 : 0) < (rightrow[col].toBool() ? 1 : 0);
		}
	}

	Q_ASSERT(0); // Shouldn't get here!
	return false;
}