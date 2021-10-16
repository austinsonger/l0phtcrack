#ifndef __INC_CLC7CALIBRATE_H
#define __INC_CLC7CALIBRATE_H

#include "ui_calibrate.h"

#include"CLC7ExecuteJTR.h"

class CLC7CalibrationData
{

private:

	QString m_jtrkernel;
	quint64 m_cps;
	QMap<int, QString> m_extra_opencl_kernel_args;

public:

	CLC7CalibrationData();
	CLC7CalibrationData(const CLC7CalibrationData &copy);

	quint64 CPS() const;
	void setCPS(quint64 cps);

	QString JTRKernel() const;
	void setJTRKernel(QString jtrkernel);

	QMap<int, QString> ExtraOpenCLKernelArgs() const;
	void setExtraOpenCLKernelArgs(QMap<int, QString> extra_opencl_kernel_args);

	friend QDataStream &operator<<(QDataStream &out, const CLC7CalibrationData &data);
	friend QDataStream &operator>>(QDataStream &in, CLC7CalibrationData &data);
};

class CLC7CalibrationHashType
{
private:
	fourcc m_hashtype;

public:

	CLC7CalibrationHashType();
	CLC7CalibrationHashType(fourcc hashtype);
	CLC7CalibrationHashType(const CLC7CalibrationHashType & ht);
	bool operator==(const CLC7CalibrationHashType & other) const;
	bool operator<(const CLC7CalibrationHashType & other) const;

	fourcc hashtype() const;

	friend QDataStream &operator<<(QDataStream &out, const CLC7CalibrationHashType &ht);
	friend QDataStream &operator>>(QDataStream &in, CLC7CalibrationHashType &ht);
};

class CLC7CalibrationProcessorType
{
private:
	GPUPLATFORM m_gpuplatform;
	QString m_cpuinstructionset;
	
public:

	CLC7CalibrationProcessorType();
	CLC7CalibrationProcessorType(QString cpuinstructionset);
	CLC7CalibrationProcessorType(GPUPLATFORM gpuplatform);
	CLC7CalibrationProcessorType(const CLC7CalibrationProcessorType & copy);
	bool operator==(const CLC7CalibrationProcessorType & other) const;
	bool operator<(const CLC7CalibrationProcessorType & other) const;
 
	GPUPLATFORM gpuplatform() const;
	QString cpuinstructionset() const;

	friend QDataStream &operator<<(QDataStream &out, const CLC7CalibrationProcessorType &pt);
	friend QDataStream &operator>>(QDataStream &in, CLC7CalibrationProcessorType &pt);
};


class CLC7Calibration
{
public:

	class CLC7CalibrationKey
	{
	private:
		bool m_mask;
		CLC7CalibrationHashType m_hashtype;
		CLC7CalibrationProcessorType m_proctype;

	public:
		CLC7CalibrationKey();
		CLC7CalibrationKey(bool mask, const CLC7CalibrationHashType &hashtype, const CLC7CalibrationProcessorType &proctype);
		CLC7CalibrationKey(const CLC7CalibrationKey & copy);
		bool operator<(const CLC7CalibrationKey & other) const;

		friend QDataStream &operator<<(QDataStream &out, const CLC7CalibrationKey &key);
		friend QDataStream &operator>>(QDataStream &in, CLC7CalibrationKey &key);
	};

private:

	QList<CLC7CalibrationHashType> m_hashtypes;
	QList<CLC7CalibrationProcessorType> m_proctypes;
	QMap<CLC7CalibrationKey, CLC7CalibrationData> m_calibrationdata;
	QMap<CLC7CalibrationHashType, CLC7CalibrationProcessorType> m_preferred_methods[2];
	QStringList m_available_cputypes;
	int m_cpu_thread_count;
	QVector<GPUINFO> m_gpuinfo;
	
	bool m_is_valid;

public:
	
	CLC7Calibration();

	bool isValid() const;
	void setValid(bool valid);

	// Configuration
	bool configurationMatch(const CLC7Calibration & other) const;
	
	void addHashType(const CLC7CalibrationHashType & hashtype);
	void addProcessorType(const CLC7CalibrationProcessorType & proctype);
	void setAvailableCPUTypes(QStringList available_cputypes);
	void setCPUThreadCount(int cputhreadcount);
	void setGPUInfo(QVector<GPUINFO> gpuinfo);
	
	QList<CLC7CalibrationHashType> hashTypes() const;
	QList<CLC7CalibrationProcessorType> processorTypes() const;
	QStringList availableCPUTypes() const;
	int CPUThreadCount() const;
	QVector<GPUINFO> GPUInfo() const;
	
	// Calibration data
	void clearCalibrationData(void);
	void setCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype, const CLC7CalibrationData &value);
	bool hasCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype) const;
	bool getCalibrationData(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype, CLC7CalibrationData & value) const;

	// Selected method
	void setPreferredMethod(bool mask, const CLC7CalibrationHashType & hashtype, const CLC7CalibrationProcessorType &proctype);
	bool getPreferredMethod(bool mask, const CLC7CalibrationHashType & hashtype, CLC7CalibrationProcessorType &proctype) const;
	bool GetHashTypePreferredProcessor(bool mask, fourcc hashtype, GPUPLATFORM & gpuplatform, QString & cpuinstructionset, QString &jtrkernel, QMap<int, QString> & extra_opencl_kernel_args) const;

	// Serialization
	friend QDataStream &operator<<(QDataStream &out, const CLC7Calibration &cal);
	friend QDataStream &operator>>(QDataStream &in, CLC7Calibration &cal);

	static void saveCalibration(const CLC7Calibration & cal);
	static void loadCalibration(CLC7Calibration & cal);
	static void emptyCalibration(CLC7Calibration & cal);

};

class CLC7CalibrationThread : public QThread
{
public:

	CLC7ExecuteJTR *m_exejtr;
	quint64 m_cps;
	QString m_extra_opencl_kernel_args;
	QString m_jtrversion;
	QString m_algo;
	int m_jtrindex;
	bool m_mask;
	GPUPLATFORM m_platform;
	QString m_vendor;
	
private:

	bool selftest(QString extra_kernel_args);

public:
	CLC7CalibrationThread(QString algo, QString jtrversion, int jtrindex, bool mask, GPUPLATFORM platform, QString vendor);
	virtual ~CLC7CalibrationThread();
	virtual void run(void);
	void abort(void);
	void terminate(void);

};


class CLC7Calibrate : public QDialog
{
	Q_OBJECT
private:
	struct REFRESHCELLARGS
	{
		bool mask;
		CLC7CalibrationHashType cht;
		CLC7CalibrationProcessorType cpt;
		int row;
		int col;
	};

	Ui::CalibrationDialog ui;
	QThread m_calibration_thread;
	
	ILC7PasswordLinkage * m_passlink;
	ILC7ColorManager * m_colman;
	
	CLC7Calibration m_calibration;

	bool m_context_calibration;
	bool m_context_calibration_mask;
	CLC7CalibrationHashType m_context_cht;
	CLC7CalibrationProcessorType m_context_cpt;


	void UpdateUI(void);

	void runCalibration(void);
	void refreshCalibration(void);
	void refreshCalibrationInternal(bool mask);
	void refreshCell(bool mask, CLC7CalibrationHashType cht, CLC7CalibrationProcessorType cpt, int row, int col);

	bool runCalibrationThreads(QString algo, GPUPLATFORM gpuplatform, QString jtrversion, bool mask, quint64 &cps, QMap<int, QString> &extra_opencl_kernel_args);

public:
	CLC7Calibrate(QWidget *parent);
	virtual ~CLC7Calibrate();


	void recalibrateCell(bool mask);
	void cellClicked(bool mask, int row, int column);
	void customContextMenuRequested(bool mask, const QPoint &pos);

public slots:

	void slot_refreshCell(REFRESHCELLARGS *args);
	
	void slot_createSpinnerWidget(bool mask, int row, int col);
	void slot_removeSpinnerWidget(bool mask, int row, int col);
	void slot_refreshTableWidget(bool mask);

	void slot_calibrateButton_clicked(bool checked);
	
	void slot_cellClicked_nomask(int row, int column);
	void slot_cellClicked_mask(int row, int column);
	void slot_customContextMenuRequested_nomask(const QPoint &pos);
	void slot_customContextMenuRequested_mask(const QPoint &pos);
	void slot_recalibrateCell_nomask(void);
	void slot_recalibrateCell_mask(void);

	void slot_calibrationThread_started();
	void slot_calibrationThread_finished();

	void slot_accepted(void);

signals:
	void sig_createSpinnerWidget(bool mask, int row, int col);
	void sig_removeSpinnerWidget(bool mask, int row, int col);
	void sig_refreshTableWidget(bool mask);
	void sig_refreshCell(REFRESHCELLARGS *args);
	void sig_setProgressBarValue(int value);
	void sig_setProgressBarRange(int lo, int hi);

public:
	virtual void reject();


};


#endif