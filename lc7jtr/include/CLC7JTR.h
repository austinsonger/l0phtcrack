#ifndef __INC_CLC7JTR_H
#define __INC_CLC7JTR_H

#include"jtrworker.h"

class CLC7JTRPasswordEngine;
class ILC7CommandControl;
class ILC7AccountList;

class CLC7JTR: public QObject
{
	Q_OBJECT

private:
	friend class CJTRWorker;

	CLC7JTRPasswordEngine *m_engine;
	ILC7CommandControl *m_ctrl;
	QMap<QString,QVariant> m_config;
	ILC7AccountList *m_accountlist;
	QMultiMap<QByteArray, QPair<int, int> > m_accts_by_firsthalf;		// pair of account number and hash number within account
	QMultiMap<QByteArray, QPair<int, int> > m_accts_by_secondhalf;
	QMultiMap<QByteArray, QPair<int, int> > m_accts_by_hash;
	QString m_error;
	ILC7CalibrationTable *m_cal;
	JTRWORKERCTX *m_ctx;
	
	CJTRWorker *m_pJTRWorker;
	QMutex m_statusmutex;

	QMap<QUuid, QTextCodec *> m_encodingcache;

	bool m_bStopRequested;
	bool m_bReset;
	bool m_is_error;

	QString GenerateCommandLine(const JTRPASS & pass, bool restore, QString potfilename, QString sessionfilename);
	bool AddPasses(fourcc hashtype, int durationblock);
	bool AddNTTogglePass(int durationblock);
	bool ProcessHashes(quint32 hashtype, bool create_index, bool create_hash_file);
	bool ConfigurePassNodes(JTRPASS & pass);
	QByteArray ConvertToEncoding(QString str, QUuid encoding = QUuid());
	//bool PreflightNode(JTRPASS & pass, int node);
	static bool SelfTestInternal(QString jtrversion, QString algo, const LC7GPUInfo &gpuinfo, QString extra_kernel_args);

public:

	CLC7JTR(CLC7JTRPasswordEngine *engine, ILC7AccountList *accountlist, ILC7CommandControl *ctrl);
	virtual ~CLC7JTR();

	static QString CalibrationKey();
	static QVariant EncodeCalibrationRowId(fourcc hashtype, bool mask);
	static bool DecodeCalibrationRowId(QVariant rowId, fourcc &hashtype, bool &mask);
	static QVariant EncodeCalibrationColId(GPUPLATFORM gpu, QString cpuinstructionset);
	static bool DecodeCalibrationColId(QVariant colId, GPUPLATFORM &gpu, QString &cpuinstructionset);
	static QVariant EncodeGPUINFOVector(const QVector<LC7GPUInfo> &gpuinfo);
	static bool DecodeGPUINFOVector(QVariant givar, QVector<LC7GPUInfo> &gpuinfo);

	static QStringList GetSupportedInstructionSets(bool include_disabled = false);
	static QVector<LC7GPUInfo> GetSupportedGPUInfo(bool include_disabled = false);
	static void DisableInstructionSet(QString jtrdllversion);
	static bool GetSupportsUnmaskedGPU(fourcc hashtype);
	static QString GetCPUNodeAlgorithm(fourcc hashtype);
	static QString GetOpenCLNodeAlgorithm(fourcc hashtype);
	//static QString GetCUDANodeAlgorithm(fourcc hashtype);

	static bool SelfTest(QString jtrversion, QString algo, const LC7GPUInfo &gpuinfo, QString *extra_opencl_kernel_args);
	virtual bool Configure(QMap<QString,QVariant> config);

	virtual QMap<QString, QVariant> GetConfig();
	virtual QMap<QString, QVariant> GetCheckpointConfig();

	virtual void StartCracking(void);
	virtual void StopCracking(void);
	virtual void ResetCracking(void);
	virtual bool CheckCrackingFinished(bool & success);	
	virtual void ProcessStatus(void);
	virtual void Cleanup(void);
	virtual bool IsReset(void);
	virtual QString LastError(void);

};

#endif