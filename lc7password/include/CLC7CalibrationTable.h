#ifndef __INC_CLC7CALIBRATIONTABLE_H
#define __INC_CLC7CALIBRATIONTABLE_H

#include<QMap>
#include<QVariant>
#include<QDataStream>

class CLC7CalibrationTableRow;
class CLC7CalibrationTable;

class CLC7CalibrationTableCell:public ILC7CalibrationTableCell
{
private:
	CLC7CalibrationTableRow *m_row;
	QVariant m_colId;
	bool m_valid;
    quint64 m_cps;
    QVariant m_data;

public:
    virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	CLC7CalibrationTableCell(CLC7CalibrationTableRow *row, QVariant colId);
	CLC7CalibrationTableCell(CLC7CalibrationTableRow *row, const CLC7CalibrationTableCell &other);
    virtual ~CLC7CalibrationTableCell();
	bool operator==(const CLC7CalibrationTableCell &other) const;
	
	virtual ILC7CalibrationTableRow *GetRow();
	virtual QVariant GetId();
	
	virtual void Reset();

    virtual bool Valid();
    virtual void SetValid(bool valid);

    virtual quint64 CPS();
    virtual void SetCPS(quint64 cps);

    virtual QVariant & Data();

    friend QDataStream &operator<<(QDataStream &out, const CLC7CalibrationTableCell &data);
    friend QDataStream &operator>>(QDataStream &in, CLC7CalibrationTableCell &data);

};


class CLC7CalibrationTableRow:public ILC7CalibrationTableRow
{
private:
	CLC7CalibrationTable *m_table;
	QVariant m_rowId;
	
	QVector<QVariant> m_calibration_col_ids;
    QMap<QVariant, CLC7CalibrationTableCell *> m_calibration_data_by_col_id;

	QVariant m_preferred_col_id;

public:
    virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	CLC7CalibrationTableRow(CLC7CalibrationTable *table, QVariant rowId);
	CLC7CalibrationTableRow(CLC7CalibrationTable *table, const CLC7CalibrationTableRow &other);
    virtual ~CLC7CalibrationTableRow();
	bool operator==(const CLC7CalibrationTableRow &other) const;

	virtual ILC7CalibrationTable *GetTable();
	virtual QVariant GetId();

    virtual void Reset();
    virtual void ClearData();

    virtual bool IsValid();

    virtual QVector<QVariant> GetCalibrationColIds();
	virtual ILC7CalibrationTableCell *GetOrCreateCalibrationCell(QVariant colId, bool create = true);

	virtual QVariant GetBestColId();
	virtual QVariant GetPreferredColId();
	virtual void SetPreferredColId(QVariant colId);

    virtual bool ConfigurationMatch(ILC7CalibrationTableRow *other);

	friend QDataStream &operator<<(QDataStream &out, const CLC7CalibrationTableRow &row);
	friend QDataStream &operator>>(QDataStream &in, CLC7CalibrationTableRow &row);
};


class CLC7CalibrationTable:public ILC7CalibrationTable
{
private:
	QUuid m_default_set_id;
    QVector<QVariant> m_all_calibration_row_ids;    
    QMap<QVariant, CLC7CalibrationTableRow *> m_calibration_row_by_row_id;
	QVariant m_extra_configuration;

public:
    virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

    CLC7CalibrationTable();
    CLC7CalibrationTable(const CLC7CalibrationTable &other);
    virtual ~CLC7CalibrationTable();

    virtual ILC7CalibrationTable *Copy();
    virtual void Release();
	virtual void Reset();
    virtual void ClearData();
    
    virtual bool IsValid();

	virtual void SetDefaultSetId(QUuid id);
	virtual QUuid GetDefaultSetId();
	virtual QVariant & ExtraConfiguration();
    
    virtual QVector<QVariant> GetAllCalibrationRowIds();
    virtual QVector<QVariant> GetAllCalibrationColIds();
	virtual ILC7CalibrationTableRow *GetOrCreateCalibrationTableRow(QVariant rowId, bool create = true);

    virtual bool ConfigurationMatch(ILC7CalibrationTable *other);

	friend QDataStream &operator<<(QDataStream &out, const CLC7CalibrationTable &table);
	friend QDataStream &operator>>(QDataStream &in, CLC7CalibrationTable &table);
};



#endif