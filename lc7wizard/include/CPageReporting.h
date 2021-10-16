#ifndef __INC_CPAGEREPORTING_H
#define __INC_CPAGEREPORTING_H

class CPageReporting : public CLC7WizardPage
{
	Q_OBJECT

public:
	CPageReporting(QWidget *parent = 0);

	int nextId() const;
	bool isComplete() const;
	void UpdateUI(void);

private slots:
	void slot_browseCSVFileButton_clicked(bool checked);
	void slot_formatbutton_clicked(bool checked);

private:
	QButtonGroup m_buttongroup;
	QLabel *m_reportLabel;
	QGroupBox *m_exportReportFileGroupBox;
	CLabelRadioButton *m_csvRadioButton;
	CLabelRadioButton *m_pdfRadioButton;
	CLabelRadioButton *m_htmlRadioButton;
	CLabelRadioButton *m_xmlRadioButton;
	QLineEdit *m_reportFileLineEdit;
	QPushButton *m_browseReportFileButton;
	CLabelCheckBox *m_displayPasswordsCheckbox;
	CLabelCheckBox *m_displayHashesCheckbox;
//	CLabelCheckBox *m_displayAuditTimeCheckBox;
//	CLabelCheckBox *m_displayAuditMethodCheckBox;
	QString m_format_name;
	QString m_format_ext;

};

#endif