#include<stdafx.h>


///////////////////////////////////////////////////////////////////////////

ILC7Interface *CLC7CalibrationTableCell::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7CalibrationTableCell")
	{
		return this;
	}
	return nullptr;
}

CLC7CalibrationTableCell::CLC7CalibrationTableCell(CLC7CalibrationTableRow *row, QVariant colId) :m_row(row), m_colId(colId)
{
	Reset();
}

CLC7CalibrationTableCell::CLC7CalibrationTableCell(CLC7CalibrationTableRow *row, const CLC7CalibrationTableCell &other) : m_row(row), m_colId(other.m_colId)
{
	m_valid = other.m_valid;
	m_cps = other.m_cps;
	m_data = other.m_data;
}

CLC7CalibrationTableCell::~CLC7CalibrationTableCell()
{
}

bool CLC7CalibrationTableCell::operator==(const CLC7CalibrationTableCell &other) const
{
	return m_valid == other.m_valid && m_cps == other.m_cps && m_data == other.m_data;
}

ILC7CalibrationTableRow *CLC7CalibrationTableCell::GetRow()
{
	return m_row;
}

QVariant CLC7CalibrationTableCell::GetId()
{
	return m_colId;
}

void CLC7CalibrationTableCell::Reset()
{
	m_valid = false;
	m_cps = 0;
}

bool CLC7CalibrationTableCell::Valid()
{
	return m_valid;
}

void CLC7CalibrationTableCell::SetValid(bool valid)
{
	m_valid = valid;
}

quint64 CLC7CalibrationTableCell::CPS()
{
	return m_cps;
}

void CLC7CalibrationTableCell::SetCPS(quint64 cps)
{
	m_cps = cps;
}

QVariant & CLC7CalibrationTableCell::Data()
{
	return m_data;
}

QDataStream & operator<<(QDataStream &out, const CLC7CalibrationTableCell &data)
{
	int32_t version = 1;
	out << version;
	out << data.m_valid;
	out << data.m_cps;
	out << data.m_data;
	return out;
}
QDataStream & operator>>(QDataStream &in, CLC7CalibrationTableCell &data)
{
	int32_t version;
	in >> version;
	if (version<1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}
	if (version>1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}
	in >> data.m_valid;
	in >> data.m_cps;
	in >> data.m_data;
	return in;
}

///////////////////////////////////////////////////////////////////////////

ILC7Interface *CLC7CalibrationTableRow::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7CalibrationTableRow")
	{
		return this;
	}
	return nullptr;
}

CLC7CalibrationTableRow::CLC7CalibrationTableRow(CLC7CalibrationTable *table, QVariant rowId):m_table(table), m_rowId(rowId)
{

}

CLC7CalibrationTableRow::CLC7CalibrationTableRow(CLC7CalibrationTable *table, const CLC7CalibrationTableRow &other) : m_table(table), m_rowId(other.m_rowId), m_calibration_col_ids(other.m_calibration_col_ids)
{
	QMapIterator<QVariant, CLC7CalibrationTableCell *> i(other.m_calibration_data_by_col_id);
	while (i.hasNext())
	{
		i.next();
		m_calibration_data_by_col_id[i.key()] = new CLC7CalibrationTableCell(*i.value());
	}
}

CLC7CalibrationTableRow::~CLC7CalibrationTableRow()
{
	Reset();
}


bool CLC7CalibrationTableRow::operator==(const CLC7CalibrationTableRow &other) const
{

	if (m_calibration_col_ids.size() != other.m_calibration_col_ids.size())
	{
		return false;
	}
	for (int i = 0; i < m_calibration_col_ids.size(); i++)
	{
		if (m_calibration_col_ids[i] != other.m_calibration_col_ids[i])
		{
			return false;
		}
	}
	
	if (m_calibration_data_by_col_id.size() != other.m_calibration_data_by_col_id.size())
	{
		return false;
	}

	QMapIterator<QVariant, CLC7CalibrationTableCell *> i(m_calibration_data_by_col_id);
	QMapIterator<QVariant, CLC7CalibrationTableCell *> j(other.m_calibration_data_by_col_id);
	while (i.hasNext() && j.hasNext())
	{
		i.next();
		j.next();

		if (!(*i.value() == *j.value()))
		{
			return false;
		}

		Q_ASSERT(i.hasNext() == j.hasNext());
	}

	return true;
}

ILC7CalibrationTable *CLC7CalibrationTableRow::GetTable()
{
	return m_table;
}

QVariant CLC7CalibrationTableRow::GetId()
{
	return m_rowId;
}

void CLC7CalibrationTableRow::Reset()
{
	QMapIterator<QVariant, CLC7CalibrationTableCell *> i(m_calibration_data_by_col_id);
	while (i.hasNext())
	{
		i.next();
		delete i.value();
	}
	
	m_calibration_col_ids.clear();
	m_calibration_data_by_col_id.clear();
}

void CLC7CalibrationTableRow::ClearData()
{
	QMapIterator<QVariant, CLC7CalibrationTableCell *> i(m_calibration_data_by_col_id);
	while (i.hasNext())
	{
		i.next();
		i.value()->Reset();
	}
}

bool CLC7CalibrationTableRow::IsValid()
{
	QMapIterator<QVariant, CLC7CalibrationTableCell *> i(m_calibration_data_by_col_id);
	while (i.hasNext())
	{
		i.next();
		if (!i.value()->Valid())
		{
			return false;
		}
	}

	return true;
}

QVector<QVariant> CLC7CalibrationTableRow::GetCalibrationColIds()
{
	return m_calibration_col_ids;
}

ILC7CalibrationTableCell *CLC7CalibrationTableRow::GetOrCreateCalibrationCell(QVariant colId, bool create)
{
	if (!m_calibration_data_by_col_id.contains(colId))
	{
		if (!create)
		{
			return nullptr;
		}
		m_calibration_data_by_col_id[colId] = new CLC7CalibrationTableCell(this, colId);
		m_calibration_col_ids.append(colId);
	}
	
	return m_calibration_data_by_col_id[colId];
}

QVariant CLC7CalibrationTableRow::GetBestColId()
{
	QMapIterator<QVariant, CLC7CalibrationTableCell *> i(m_calibration_data_by_col_id);
	QVariant bestColId = m_calibration_col_ids.first();
	quint64 bestcps = 0;
	while (i.hasNext())
	{
		i.next();
		if (i.value()->Valid())
		{
			if (i.value()->CPS() >= bestcps)
			{
				bestcps = i.value()->CPS();
				bestColId = i.key();
			}
		}
	}
	return bestColId;
}

QVariant CLC7CalibrationTableRow::GetPreferredColId()
{
	return m_preferred_col_id;
}

void CLC7CalibrationTableRow::SetPreferredColId(QVariant colId)
{
	m_preferred_col_id = colId;
}

bool CLC7CalibrationTableRow::ConfigurationMatch(ILC7CalibrationTableRow *other)
{
	CLC7CalibrationTableRow *other_impl = (CLC7CalibrationTableRow *)other;

	if (m_calibration_col_ids.size() != other_impl->m_calibration_col_ids.size())
	{
		return false;
	}
	for (int i = 0; i < m_calibration_col_ids.size(); i++)
	{
		if (m_calibration_col_ids[i] != other_impl->m_calibration_col_ids[i])
		{
			return false;
		}
	}
	return true;
}


QDataStream & operator<<(QDataStream &out, const CLC7CalibrationTableRow &row)
{
	int32_t version = 1;
	out << version;
	out << row.m_preferred_col_id;
	out << row.m_calibration_col_ids;
	
	Q_ASSERT(row.m_calibration_data_by_col_id.size() == row.m_calibration_col_ids.size());

	QMapIterator<QVariant, CLC7CalibrationTableCell *> i(row.m_calibration_data_by_col_id);
	while (i.hasNext())
	{
		i.next();
		out << i.key();
		out << *i.value();
	}

	return out;
}

QDataStream & operator>>(QDataStream &in, CLC7CalibrationTableRow &row)
{
	int32_t version;
	in >> version;
	if (version<1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}
	if (version>1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}

	in >> row.m_preferred_col_id;
	in >> row.m_calibration_col_ids;
	for (int i = 0; i < row.m_calibration_col_ids.size(); i++)
	{
		QVariant key;
		in >> key;

		CLC7CalibrationTableCell *value = new CLC7CalibrationTableCell(&row, key);
		in >> *value;

		row.m_calibration_data_by_col_id[key] = value;
	}

	return in;
}


///////////////////////////////////////////////////////////////////////////

ILC7Interface *CLC7CalibrationTable::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7CalibrationTable")
	{
		return this;
	}
	return nullptr;
}


CLC7CalibrationTable::CLC7CalibrationTable()
{

}

CLC7CalibrationTable::CLC7CalibrationTable(const CLC7CalibrationTable &other) : m_extra_configuration(other.m_extra_configuration), m_all_calibration_row_ids(other.m_all_calibration_row_ids) 
{
	QMapIterator<QVariant, CLC7CalibrationTableRow *> i(other.m_calibration_row_by_row_id);
	while (i.hasNext()) 
	{
		i.next();
		
		m_calibration_row_by_row_id[i.key()] = new CLC7CalibrationTableRow(this, *i.value());
	}
}

CLC7CalibrationTable::~CLC7CalibrationTable()
{
	Reset();
}

ILC7CalibrationTable *CLC7CalibrationTable::Copy()
{
	return new CLC7CalibrationTable(*this);
}

void CLC7CalibrationTable::Release()
{
	delete this;
}

void CLC7CalibrationTable::Reset()
{
	m_default_set_id = QUuid();
	QMapIterator<QVariant, CLC7CalibrationTableRow *> i(m_calibration_row_by_row_id);
	while (i.hasNext())
	{
		i.next();
		delete i.value();
	}
	m_calibration_row_by_row_id.clear();
	m_all_calibration_row_ids.clear();
}

void CLC7CalibrationTable::ClearData()
{
	QMapIterator<QVariant, CLC7CalibrationTableRow *> i(m_calibration_row_by_row_id);
	while (i.hasNext())
	{
		i.next();
		i.value()->ClearData();
	}
}

bool CLC7CalibrationTable::IsValid()
{
	QMapIterator<QVariant, CLC7CalibrationTableRow *> i(m_calibration_row_by_row_id);
	while (i.hasNext())
	{
		i.next();
		if (!i.value()->IsValid())
			return false;
	}
	return true;
}

void CLC7CalibrationTable::SetDefaultSetId(QUuid id)
{
	m_default_set_id = id;
}

QUuid CLC7CalibrationTable::GetDefaultSetId()
{
	return m_default_set_id;
}

QVariant & CLC7CalibrationTable::ExtraConfiguration()
{
	return m_extra_configuration;
}

QVector<QVariant> CLC7CalibrationTable::GetAllCalibrationRowIds()
{
	return m_all_calibration_row_ids;
}

QVector<QVariant> CLC7CalibrationTable::GetAllCalibrationColIds()
{
	QVector<QVariant> all_col_ids;
	std::set<QVariant> all_col_ids_set;
	QMapIterator<QVariant, CLC7CalibrationTableRow *> i(m_calibration_row_by_row_id);
	while (i.hasNext())
	{
		i.next();
		QVector<QVariant> col_ids = i.value()->GetCalibrationColIds();
		foreach(QVariant col_id, col_ids)
		{
			if (all_col_ids_set.find(col_id) != all_col_ids_set.end())
			{
				continue;
			}
			all_col_ids_set.insert(col_id);
			all_col_ids.append(col_id);
		}
	}
	return all_col_ids;
}

ILC7CalibrationTableRow *CLC7CalibrationTable::GetOrCreateCalibrationTableRow(QVariant rowId, bool create)
{
	if (!m_calibration_row_by_row_id.contains(rowId))
	{
		if (!create)
		{
			return nullptr;
		}
		m_calibration_row_by_row_id[rowId] = new CLC7CalibrationTableRow(this, rowId);
		m_all_calibration_row_ids.append(rowId);
	}
	return m_calibration_row_by_row_id[rowId];
}

bool CLC7CalibrationTable::ConfigurationMatch(ILC7CalibrationTable *other)
{
	CLC7CalibrationTable *other_impl = (CLC7CalibrationTable *)other;


	if (m_all_calibration_row_ids.size() != other_impl->m_all_calibration_row_ids.size())
	{
		return false;
	}
	for (int i = 0; i < m_all_calibration_row_ids.size(); i++)
	{
		if (m_all_calibration_row_ids[i] != other_impl->m_all_calibration_row_ids[i])
		{
			return false;
		}
	}

	if (m_calibration_row_by_row_id.size() != other_impl->m_calibration_row_by_row_id.size() ||
		m_extra_configuration != other_impl->m_extra_configuration)
	{
		return false;
	}
	
	QMapIterator<QVariant, CLC7CalibrationTableRow *> i(m_calibration_row_by_row_id);
	QMapIterator<QVariant, CLC7CalibrationTableRow *> j(other_impl->m_calibration_row_by_row_id);
	while (i.hasNext() && j.hasNext())
	{
		i.next();
		j.next();

		if (!i.value()->ConfigurationMatch(j.value()))
		{
			return false;
		}

		Q_ASSERT(i.hasNext() == j.hasNext());
	}

	return true;
}

QDataStream & operator<<(QDataStream &out, const CLC7CalibrationTable &table)
{
	int32_t version = 1;
	out << version;
	out << table.m_default_set_id;
	out << table.m_extra_configuration;
	out << table.m_all_calibration_row_ids;

	Q_ASSERT(table.m_calibration_row_by_row_id.size() == table.m_calibration_row_by_row_id.size());

	QMapIterator<QVariant, CLC7CalibrationTableRow *> i(table.m_calibration_row_by_row_id);
	while (i.hasNext())
	{
		i.next();
		out << i.key();
		out << *i.value();
	}

	return out;
}

QDataStream & operator>>(QDataStream &in, CLC7CalibrationTable &table)
{
	int32_t version;
	in >> version;
	if (version<1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}
	if (version>1)
	{
		in.setStatus(QDataStream::ReadCorruptData);
		return in;
	}
	in >> table.m_default_set_id;
	in >> table.m_extra_configuration;
	in >> table.m_all_calibration_row_ids;
	for (int i = 0; i < table.m_all_calibration_row_ids.size(); i++)
	{
		QVariant key;
		in >> key;

		CLC7CalibrationTableRow *value = new CLC7CalibrationTableRow(&table, key);
		in >> *value;

		table.m_calibration_row_by_row_id[key] = value;
	}

	return in;
}

