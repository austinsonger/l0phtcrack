#ifndef __INC_CLC7CALIBRATEGUI_H
#define __INC_CLC7CALIBRATEGUI_H

#include "ui_calibrationdialog.h"

class CLC7CalibrationThread : public QThread
{
	Q_OBJECT

public:
	CLC7CalibrationThread();
	virtual ~CLC7CalibrationThread();
	virtual void run(void);
	void abort(void);
	void terminate(void);

signals:
	void started();
	void finished();

};


class CLC7CalibrationDialog : public QDialog
{
	Q_OBJECT
public:
	struct TABLE_KEY
	{
		ILC7CalibrationTable *table;
		int rownum;
		int colnum;
	};

	struct ID_KEY
	{
		ILC7CalibrationTable *table;
		QVariant id;
	};



private:

	Ui::CalibrationDialog ui;
	
	QVector<ILC7PasswordEngine *> m_engines;
	QMap<ILC7PasswordEngine *, QList<ILC7PasswordEngine::CALIBRATION_DEFAULT_SET>> m_engine_default_sets;

	ILC7PasswordLinkage * m_passlink;
	ILC7ColorManager * m_colman;
	CLC7CalibrationThread m_calibration_thread;

	enum class CALIBRATION_TYPE
	{
		EVERYTHING=0,
		TABLE=1,
		ROW=2,
		CELL=3
	};
	CALIBRATION_TYPE m_calibration_type;
	ILC7CalibrationTable *m_current_table;
	QVariant m_current_rowId;
	QVariant m_current_colId;

	ILC7CalibrationTable *m_context_table;
	QVariant m_context_rowId;
	QVariant m_context_colId;

	QMap<ILC7PasswordEngine *, ILC7CalibrationTable *> m_calibration_table_by_engine;
	QMap<ILC7CalibrationTable *, ILC7PasswordEngine *> m_engine_by_calibration_table;
	QMap<ILC7CalibrationTable *, QTableWidget *> m_table_widgets_by_calibration_table;
	
	QHash<TABLE_KEY, ILC7CalibrationTableCell *> m_table_cells_by_row_col;
	QHash<ID_KEY, int> m_table_row_by_rowId;
	QHash<ID_KEY, int> m_table_col_by_colId;

	int TableRowIdToRow(ILC7CalibrationTable *table, QVariant rowId);
	int TableColIdToCol(ILC7CalibrationTable *table, QVariant colId);

	void AddEnginePresetsBox(ILC7PasswordEngine *engine, QLayout *presetslayout);
	void AddEngineCalibrationTable(ILC7PasswordEngine *engine, QLayout *calibrationlayout);
	bool tableIsCompatibleWithEngine(ILC7CalibrationTable *table, ILC7PasswordEngine *engine);
	void runCalibration(void);
	void refreshCalibration(void);
	void refreshRow(ILC7CalibrationTable *table, QVariant rowId);
	void refreshCell(ILC7CalibrationTable *table, QVariant rowId, QVariant colId);
	void UpdateUI(void);
	bool eventFilter(QObject *obj, QEvent *event);
	void resizeEvent(QResizeEvent *e);

	struct JOB
	{
		ILC7CalibrationTable *m_table;
		QVariant m_rowId;
		QVariant m_colId;
	};
	QList<JOB> m_jobs;

	void calibrationCallback(const ILC7PasswordEngine::CALIBRATION_CALLBACK_ARGUMENTS &args);
	void updatePreferredCalibration(ILC7CalibrationTable *table, QVariant rowId);

	void saveCalibrations();

public:
	CLC7CalibrationDialog(QWidget *parent);
	virtual ~CLC7CalibrationDialog();

	virtual void accept();
	virtual void reject();

	void recalibrateCell();
	void cellClicked(int row, int column);
	void customContextMenuRequested(const QPoint &pos);
	void createSpinnerWidget(ILC7CalibrationTable *table, QVariant rowId, QVariant colId);
	void removeSpinnerWidget(ILC7CalibrationTable *table, QVariant rowId, QVariant colId);

public slots:
	
	void slot_refreshCell(ILC7CalibrationTable *table, QVariant rowId, QVariant colId);
	void slot_refreshTableWidget(ILC7CalibrationTable *table);

	void slot_calibrateButton_clicked(bool checked);
	void slot_singleCalibrateButton_clicked(bool checked);

	void slot_cellClicked(int row, int column);
	void slot_customContextMenuRequested(const QPoint &pos);
	void slot_recalibrateCell(void);

	void slot_calibrationThread_started();
	void slot_calibrationThread_finished();
	
	void slot_presetClicked(bool checked);

signals:
	void sig_refreshTableWidget(ILC7CalibrationTable *table);
	void sig_refreshCell(ILC7CalibrationTable *table, QVariant rowId, QVariant colId);
	void sig_setProgressBarValue(int value);
	void sig_setProgressBarRange(int lo, int hi);

};


class CLC7CalibrateGUI :public QObject, public ILC7Component
{
	Q_OBJECT;

private:

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);
	void CheckCalibration(void);

signals:
	void sig_checkCalibration(void);

public slots:
	void slot_checkCalibration(void);

public:
	CLC7CalibrateGUI();
	virtual ~CLC7CalibrateGUI();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	// ILC7Component
	virtual QUuid GetID();
	virtual ILC7Component::RETURNCODE ExecuteCommand(QString command, QStringList args, QMap<QString, QVariant> & config, QString & error, ILC7CommandControl *ctrl = NULL);
	virtual bool ValidateCommand(QMap<QString, QVariant> & state, QString command, QStringList args, QMap<QString, QVariant> & config, QString & error);
};

#endif