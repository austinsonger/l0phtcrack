#ifndef __INC_CLC7WIZARDPAGE_H
#define __INC_CLC7WIZARDPAGE_H

#define WIZARD_CONNECT_BUTTON(x) connect((x), &QAbstractButton::clicked, this, &CLC7WizardPage::slot_abstractbutton_clicked); 
#define WIZARD_CONNECT_GROUPBOX_BUTTON(x) connect((x), &QGroupBox::clicked, this, &CLC7WizardPage::slot_groupbox_clicked); 
#define WIZARD_CONNECT_LINEEDIT(x) connect((x), &QLineEdit::textChanged, this, &CLC7WizardPage::slot_lineedit_textchanged);

class CLC7WizardPage : public QWizardPage
{
	Q_OBJECT

public:
	CLC7WizardPage(QWidget *parent = 0);
	~CLC7WizardPage();

	virtual void UpdateUI() = 0;

	bool isComplete() const;
	QMap<QString, QVariant> GetConfig();

protected:
	void CreateConfigWidget(QUuid widgetuuid, QMap<QString, QVariant> config = QMap<QString, QVariant>());

public slots:

	void slot_abstractbutton_clicked(bool);
	void slot_groupbox_clicked(bool);
	void slot_lineedit_textchanged(const QString &);
	void slot_isValid(bool);

protected:
	bool m_is_valid;

	QWidget *m_configWidget;
	ILC7Component *m_configComponent;

};

#endif