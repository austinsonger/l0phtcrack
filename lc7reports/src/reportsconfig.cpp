#include "stdafx.h"
#include "ReportsConfig.h"

ReportsConfig::ReportsConfig(QWidget *parent, QWidget *page, QMap<QString,QVariant> config)
	: QWidget(parent)
{
	ui.setupUi(this);
	ui.NoDataLabel->setObjectName("error");

	ui.TitlePageCheckbox->setChecked(config.contains("checked_title_page")?config["checked_title_page"].toBool():true);
	ui.TableOfContentsCheckbox->setChecked(config.contains("checked_table_of_contents")?config["checked_table_of_contents"].toBool():true);
	ui.ExecutiveSummaryCheckbox->setChecked(config.contains("checked_executive_summary")?config["checked_executive_summary"].toBool():true);
	ui.DetailedReportCheckbox->setChecked(config.contains("checked_detailed_report")?config["checked_detailed_report"].toBool():true);
	ui.RemediationReportCheckbox->setChecked(config.contains("checked_remediation_report")?config["checked_remediation_report"].toBool():true);
	ui.HistoricalDataReportCheckbox->setChecked(config.contains("checked_historical_data_report")?config["checked_historical_data_report"].toBool():false);
	ui.IncludeOnlyMatchingCheckbox->setChecked(config.contains("checked_include_only_matching_audits")?config["checked_include_only_matching_audits"].toBool():true);
	ui.IncludeOnlyCompletedCheckbox->setChecked(config.contains("checked_include_only_completed_audits")?config["checked_include_only_completed_audits"].toBool():true);
	ui.LastAuditRadio->setChecked(config.contains("checked_last_audit")?config["checked_last_audit"].toBool():true);
	ui.PreviousAuditsRadio->setChecked(config.contains("checked_previous_audits")?config["checked_previous_audits"].toBool():false);
	ui.ReportsInDateRangeRadio->setChecked(config.contains("checked_audits_in_date_range")?config["checked_audits_in_date_range"].toBool():false);
	ui.SpecificReportsRadio->setChecked(config.contains("checked_specific_audits")?config["checked_specific_audits"].toBool():false);
	ui.NumberOfPreviousEdit->setText(config.contains("number_of_previous_audits")?QString("%1").arg(config["number_of_previous_audits"].toInt()):"1");
	ui.FromCheckbox->setChecked(config.contains("checked_date_from")?config["checked_date_from"].toBool():false);
	ui.FromDate->setDate(config.contains("date_from")?config["date_from"].toDate():QDate::currentDate());
	ui.ToCheckbox->setChecked(config.contains("checked_date_to")?config["checked_date_to"].toBool():false);
	ui.ToDate->setDate(config.contains("date_to")?config["date_to"].toDate():QDate::currentDate());

	ui.HistoricalDataTree->setColumnCount(2);
	QStringList slHeaders;
	slHeaders.append("Date");
	slHeaders.append("Description");
	ui.HistoricalDataTree->setHeaderLabels(slHeaders);
	
	if(config.contains("historical_data"))
	{
		foreach(QVariant v,config["historical_data"].toList())
		{
			QUuid id=v.toUuid();
			ILC7HistoricalData *phd=g_pLinkage->GetHistoricalData();
			QDateTime auditdatetime;
			QVariant data;
			QString error;
			if(phd->GetDataDetailByUuid("audits",id,auditdatetime,data,error))
			{
				QMap<QString,QVariant> datamap = data.toMap();
				if(datamap.contains("description"))
				{
					QString auditdesc=datamap["description"].toString();
					QTreeWidgetItem *item=new QTreeWidgetItem();
					item->setText(0,auditdatetime.toString(Qt::DefaultLocaleShortDate));
					item->setText(1,auditdesc);
					ui.HistoricalDataTree->invisibleRootItem()->addChild(item);
				}
			}
		}
	}

	QString default_preparer_name;
	default_preparer_name = qgetenv("USER");
	if(default_preparer_name.isEmpty()) 
	{
		default_preparer_name = qgetenv("USERNAME");
	}

	ui.ReportNameCheckbox->setChecked(config.contains("checked_report_name")?config["checked_report_name"].toBool():true);
	ui.ReportNameEdit->setText(config.contains("report_name")?config["report_name"].toString():"");
	ui.ReportDateCheckbox->setChecked(config.contains("checked_report_date")?config["checked_report_date"].toBool():true);
	ui.ReportDateEdit->setDateTime(config.contains("report_date")?config["report_date"].toDateTime():QDateTime::currentDateTime());
	ui.L0phtCrackLogoCheckbox->setChecked(config.contains("checked_lc_logo")?config["checked_lc_logo"].toBool():true);
	ui.CompanyNameCheckbox->setChecked(config.contains("checked_company_name")?config["checked_company_name"].toBool():true);
	ui.CompanyNameEdit->setText(config.contains("company_name")?config["company_name"].toString():"");
	ui.CompanyLogoCheckbox->setChecked(config.contains("checked_company_logo")?config["checked_company_logo"].toBool():false);
	ui.CompanyLogoPath->setText(config.contains("company_logo_path")?config["company_logo_path"].toString():"");
	ui.PreparerNameCheckbox->setChecked(config.contains("checked_preparer_name")?config["checked_preparer_name"].toBool():true);
	ui.PreparerNameEdit->setText(config.contains("preparer_name")?config["preparer_name"].toString():default_preparer_name);
	ui.PreparerLogoCheckbox->setChecked(config.contains("checked_preparer_logo")?config["checked_preparer_logo"].toBool():false);
	ui.PreparerLogoPath->setText(config.contains("preparer_logo_path")?config["preparer_logo_path"].toString():"");
	ui.ConfidentialityStatementCheckbox->setChecked(config.contains("checked_confidentiality_statement")?config["checked_confidentiality_statement"].toBool():true);
	ui.ConfidentialityStatementEdit->setText(config.contains("confidentiality_statement")?config["confidentiality_statement"].toString():"Confidential - Do Not Distribute");

	connect(ui.TitlePageCheckbox,&QAbstractButton::clicked,this,&ReportsConfig::onClicked);
	connect(ui.TableOfContentsCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.ExecutiveSummaryCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.DetailedReportCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.RemediationReportCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.HistoricalDataReportCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.IncludeOnlyMatchingCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.IncludeOnlyCompletedCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.LastAuditRadio,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.PreviousAuditsRadio,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.ReportsInDateRangeRadio,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.SpecificReportsRadio,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.NumberOfPreviousEdit,&QLineEdit::textChanged,this, &ReportsConfig::onTextChanged);
	connect(ui.FromCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.ToCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.FromDate,&QDateEdit::dateChanged,this,&ReportsConfig::onDateChanged);
	connect(ui.ToDate,&QDateEdit::dateChanged,this,&ReportsConfig::onDateChanged);
	connect(ui.HistoricalDataTree->model(),&QAbstractItemModel::rowsInserted,this,&ReportsConfig::rowsInserted);
	connect(ui.HistoricalDataTree->model(),&QAbstractItemModel::rowsRemoved,this,&ReportsConfig::rowsRemoved);
	connect(ui.ReportNameCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.ReportNameEdit,&QLineEdit::textChanged,this, &ReportsConfig::onTextChanged);
	connect(ui.ReportDateCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.ReportDateEdit,&QDateTimeEdit::dateTimeChanged,this, &ReportsConfig::onDateTimeChanged);
	connect(ui.L0phtCrackLogoCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.CompanyNameCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.CompanyNameEdit,&QLineEdit::textChanged,this, &ReportsConfig::onTextChanged);
	connect(ui.CompanyLogoCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.CompanyLogoPath,&QLineEdit::textChanged,this, &ReportsConfig::onTextChanged);
	connect(ui.PreparerNameEdit,&QLineEdit::textChanged,this, &ReportsConfig::onTextChanged);
	connect(ui.PreparerLogoCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.PreparerLogoPath,&QLineEdit::textChanged,this, &ReportsConfig::onTextChanged);
	connect(ui.ConfidentialityStatementCheckbox,&QAbstractButton::clicked, this, &ReportsConfig::onClicked);
	connect(ui.ConfidentialityStatementEdit,&QTextEdit::textChanged,this, &ReportsConfig::onTextEditChanged);
	
	ui.NumberOfPreviousEdit->setValidator(new QIntValidator(1, 2147483647, this));

	connect(this,SIGNAL(sig_IsValid(bool)),page,SLOT(slot_IsValid(bool)));

	UpdateUI();
}

ReportsConfig::~ReportsConfig()
{TR;

}

bool ReportsConfig::GetTitlePage()
{TR;
	return ui.TitlePageCheckbox->isChecked();
}

bool ReportsConfig::GetTableOfContents()
{TR;
	return ui.TableOfContentsCheckbox->isChecked();
}

bool ReportsConfig::GetExecutiveSummary()
{TR;
	return ui.ExecutiveSummaryCheckbox->isChecked();
}

bool ReportsConfig::GetDetailedReport()
{TR;
	return ui.DetailedReportCheckbox->isChecked();
}

bool ReportsConfig::GetRemediationReport()
{TR;
	return ui.RemediationReportCheckbox->isChecked();
}

bool ReportsConfig::GetHistoricalDataReport()
{TR;
	return ui.HistoricalDataReportCheckbox->isChecked();
}

bool ReportsConfig::GetIncludeOnlyMatchingAudits()
{TR;
	return ui.IncludeOnlyMatchingCheckbox->isChecked();
}

bool ReportsConfig::GetIncludeOnlyCompletedAudits()
{TR;
	return ui.IncludeOnlyCompletedCheckbox->isChecked();
}

bool ReportsConfig::GetLastAudit()
{TR;
	return ui.LastAuditRadio->isChecked();
}

bool ReportsConfig::GetPreviousAudits()
{TR;
	return ui.PreviousAuditsRadio->isChecked();
}

bool ReportsConfig::GetAuditsInDateRange()
{TR;
	return ui.ReportsInDateRangeRadio->isChecked();
}

bool ReportsConfig::GetSpecificAudits()
{TR;
	return ui.SpecificReportsRadio->isChecked();
}

int ReportsConfig::GetNumberOfPreviousAudits()
{TR;
	return ui.NumberOfPreviousEdit->text().toInt();
}

bool ReportsConfig::GetCheckedDateFrom()
{TR;
	return ui.FromCheckbox->isChecked();
}

bool ReportsConfig::GetCheckedDateTo()
{TR;
	return ui.ToCheckbox->isChecked();
}

QDateTime ReportsConfig::GetDateFrom()
{TR;
	return ui.FromDate->dateTime();
}

QDateTime ReportsConfig::GetDateTo()
{TR;
	return ui.ToDate->dateTime();
}

QList<QUuid> ReportsConfig::GetHistoricalData()
{TR;
	QList<QUuid> histlist;

	int i,count=ui.HistoricalDataTree->invisibleRootItem()->childCount();
	for(i=0;i<count;i++)
	{
		QTreeWidgetItem *item=ui.HistoricalDataTree->invisibleRootItem()->child(i);
		histlist.append(item->data(0,Qt::DisplayRole).toUuid());
	}

	return histlist;
}

bool ReportsConfig::GetCheckedReportName()
{TR;
	return ui.ReportNameCheckbox->isChecked();
}

QString ReportsConfig::GetReportName()
{TR;
	return ui.ReportNameEdit->text();
}

bool ReportsConfig::GetCheckedReportDate()
{TR;
	return ui.ReportDateCheckbox->isChecked();
}

QDateTime ReportsConfig::GetReportDate()
{TR;
	return ui.ReportDateEdit->dateTime();
}

bool ReportsConfig::GetCheckedLCLogo()
{TR;
	return ui.L0phtCrackLogoCheckbox->isChecked();
}

bool ReportsConfig::GetCheckedCompanyName()
{TR;
	return ui.CompanyNameCheckbox->isChecked();
}

QString ReportsConfig::GetCompanyName()
{TR;
	return ui.CompanyNameEdit->text();
}

bool ReportsConfig::GetCheckedCompanyLogo()
{TR;
	return ui.CompanyLogoCheckbox->isChecked();
}

QString ReportsConfig::GetCompanyLogoPath()
{TR;
	return ui.CompanyLogoPath->text();
}

bool ReportsConfig::GetCheckedPreparerName()
{TR;
	return ui.PreparerNameCheckbox->isChecked();
}

QString ReportsConfig::GetPreparerName()
{TR;
	return ui.PreparerNameEdit->text();
}

bool ReportsConfig::GetCheckedPreparerLogo()
{TR;
	return ui.PreparerLogoCheckbox->isChecked();
}

QString ReportsConfig::GetPreparerLogoPath()
{TR;
	return ui.PreparerLogoPath->text();
}

bool ReportsConfig::GetCheckedConfidentialityStatement()
{TR;
	return ui.ConfidentialityStatementCheckbox->isChecked();
}

QString ReportsConfig::GetConfidentialityStatement()
{TR;
	return ui.ConfidentialityStatementEdit->toPlainText();
}
	
void ReportsConfig::UpdateUI()
{TR;
	// See if we have any audits at all
	bool have_audits = false;
	ILC7HistoricalData *phd=g_pLinkage->GetHistoricalData();
	quint32 count;
	QString error;
	if(phd->GetDataCount("audits",count,error) && count>0)
	{
		have_audits = true;
	}

	
	if(!have_audits)
	{
		this->setEnabled(false);
		ui.NoDataLabel->setVisible(true);
		emit sig_IsValid(false);
		return;
	}
	else
	{
	
		ui.NoDataLabel->setVisible(false);
		this->setEnabled(true);
	}

	
	bool valid=true;
	if(!(GetTitlePage() || GetTableOfContents() || GetExecutiveSummary() || GetDetailedReport() || GetRemediationReport() || GetHistoricalDataReport()))
	{
		valid=false;
	}
	
	bool has_historical = ui.HistoricalDataReportCheckbox->isChecked();
	ui.IncludeOnlyMatchingCheckbox->setEnabled(valid && has_historical);
	ui.IncludeOnlyCompletedCheckbox->setEnabled(valid && has_historical);
	ui.LastAuditRadio->setEnabled(valid && has_historical);
	ui.PreviousAuditsRadio->setEnabled(valid && has_historical);
	ui.NumberOfPreviousEdit->setEnabled(valid && has_historical);
	ui.ReportsInDateRangeRadio->setEnabled(valid && has_historical);
	ui.FromCheckbox->setEnabled(valid && has_historical);
	ui.ToCheckbox->setEnabled(valid && has_historical);
	ui.FromDate->setEnabled(valid && has_historical);
	ui.ToDate->setEnabled(valid && has_historical);
	ui.SpecificReportsRadio->setEnabled(valid && has_historical);
	ui.SelectHistoricalDataButton->setEnabled(valid && has_historical);
	ui.HistoricalDataTree->setEnabled(valid && has_historical);

	ui.ReportNameEdit->setEnabled(valid && ui.ReportNameCheckbox->isChecked());
	ui.ReportDateEdit->setEnabled(valid && ui.ReportDateCheckbox->isChecked());
	ui.CompanyNameEdit->setEnabled(valid && ui.CompanyNameCheckbox->isChecked());
	ui.CompanyLogoPath->setEnabled(valid && ui.CompanyLogoCheckbox->isChecked());
	ui.BrowseCompanyLogoButton->setEnabled(valid && ui.CompanyLogoCheckbox->isChecked());
	ui.PreparerNameEdit->setEnabled(valid && ui.PreparerNameCheckbox->isChecked());
	ui.PreparerLogoPath->setEnabled(valid && ui.PreparerLogoCheckbox->isChecked());
	ui.BrowsePreparerLogoButton->setEnabled(valid && ui.PreparerLogoCheckbox->isChecked());
	ui.ConfidentialityStatementEdit->setEnabled(valid && ui.ConfidentialityStatementCheckbox->isChecked());

	emit sig_IsValid(valid);
}


void ReportsConfig::onTextChanged(const QString & str)
{TR;
	UpdateUI();
}

void ReportsConfig::onTextEditChanged()
{TR;
	UpdateUI();
}

void ReportsConfig::onClicked(bool checked)
{TR;
	UpdateUI();
}

void ReportsConfig::rowsInserted(const QModelIndex &parent, int first, int last)
{TR;
	UpdateUI();
}

void ReportsConfig::rowsRemoved(const QModelIndex &parent, int first, int last)
{TR;
	UpdateUI();
}

void ReportsConfig::onDateChanged(const QDate & date)
{TR;
	UpdateUI();
}

void ReportsConfig::onDateTimeChanged(const QDateTime &dateTime)
{TR;
	UpdateUI();
}

