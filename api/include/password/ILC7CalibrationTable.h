#ifndef __INC_ILC7CALIBRATIONTABLE_H
#define __INC_ILC7CALIBRATIONTABLE_H

#include<QVector>
#include<QVariant>
#include<QMap>

class ILC7CalibrationTableRow;
class ILC7CalibrationTable;


class ILC7CalibrationTableCell:public ILC7Interface
{
protected:
    virtual ~ILC7CalibrationTableCell() {}

public:
	virtual ILC7CalibrationTableRow *GetRow() = 0;
	virtual QVariant GetId() = 0;
	virtual void Reset() = 0;

    virtual bool Valid()=0;
    virtual void SetValid(bool valid)=0;

    virtual quint64 CPS()=0;
    virtual void SetCPS(quint64 cps)=0;

    virtual QVariant & Data()=0;
};

class ILC7CalibrationTableRow:public ILC7Interface
{
protected:
    virtual ~ILC7CalibrationTableRow() {}

public:

	virtual ILC7CalibrationTable *GetTable() = 0;
	virtual QVariant GetId() = 0;
	virtual void Reset() = 0;
    virtual void ClearData()=0;

    virtual bool IsValid()=0;

    virtual QVector<QVariant> GetCalibrationColIds()=0;
    virtual ILC7CalibrationTableCell *GetOrCreateCalibrationCell(QVariant colId, bool create = true)=0;

	virtual QVariant GetBestColId()=0;
	virtual QVariant GetPreferredColId()=0;
	virtual void SetPreferredColId(QVariant colId)=0;

    virtual bool ConfigurationMatch(ILC7CalibrationTableRow *other)=0;
};



class ILC7CalibrationTable:public ILC7Interface
{

protected:
    virtual ~ILC7CalibrationTable() {}
    
public:
    virtual ILC7CalibrationTable *Copy()=0;
    virtual void Release()=0;
	virtual void Reset()=0;
    virtual void ClearData()=0;
    
    virtual bool IsValid()=0;

	virtual void SetDefaultSetId(QUuid id) = 0;
	virtual QUuid GetDefaultSetId() = 0;
    virtual QVariant & ExtraConfiguration()=0;
    
    virtual QVector<QVariant> GetAllCalibrationRowIds()=0;
    virtual QVector<QVariant> GetAllCalibrationColIds()=0;
	virtual ILC7CalibrationTableRow *GetOrCreateCalibrationTableRow(QVariant rowId, bool create = true) = 0;

    virtual bool ConfigurationMatch(ILC7CalibrationTable *other)=0;
};



#endif