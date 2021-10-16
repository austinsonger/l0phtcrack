#ifndef ReportsConfig_H
#define ReportsConfig_H

#include <QWidget>
#include"ui_reportsconfig.h"

class ReportsConfig : public QWidget
{
	Q_OBJECT

public:
	ReportsConfig(QWidget *parent, QWidget *page, QMap<QString,QVariant> config);
	~ReportsConfig();

	bool GetTitlePage();
	bool GetTableOfContents();
	bool GetExecutiveSummary();
	bool GetDetailedReport();
	bool GetRemediationReport();
	bool GetHistoricalDataReport();
	bool GetIncludeOnlyMatchingAudits();
	bool GetIncludeOnlyCompletedAudits();
	bool GetLastAudit();
	bool GetPreviousAudits();
	bool GetAuditsInDateRange();
	bool GetSpecificAudits();
	int GetNumberOfPreviousAudits();
	bool GetCheckedDateFrom();
	bool GetCheckedDateTo();
	QDateTime GetDateFrom();
	QDateTime GetDateTo();
	QList<QUuid> GetHistoricalData();
	
	bool GetCheckedReportName();
	QString GetReportName();
	bool GetCheckedReportDate();
	QDateTime GetReportDate();
	bool GetCheckedLCLogo();
	bool GetCheckedCompanyName();
	QString GetCompanyName();
	bool GetCheckedCompanyLogo();
	QString GetCompanyLogoPath();
	bool GetCheckedPreparerName();
	QString GetPreparerName();
	bool GetCheckedPreparerLogo();
	QString GetPreparerLogoPath();
	bool GetCheckedConfidentialityStatement();
	QString GetConfidentialityStatement();
	
private:
	Ui::ReportsConfig ui;
	QWidget *m_page;
	
	void UpdateUI();

private slots:

	void onTextChanged(const QString & str);
	void onTextEditChanged();
    void onClicked(bool checked);
	void rowsInserted(const QModelIndex &parent, int first, int last);
	void rowsRemoved(const QModelIndex &parent, int first, int last);
	void onDateChanged(const QDate & date);
    void onDateTimeChanged(const QDateTime &dateTime);

signals:
	void sig_IsValid(bool valid);

};

#endif // ReportsConfig_H
