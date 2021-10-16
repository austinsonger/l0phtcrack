#include<stdafx.h>


CLC7ActivationDlg::CLC7ActivationDlg(QWidget *parent, ILC7Controller *ctrl) : QDialog(parent, Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);
	m_ctrl = ctrl;

	connect(ui.activateOfflineButton, &QAbstractButton::clicked, this, &CLC7ActivationDlg::slot_activateOfflineButton_clicked);
	connect(ui.activateOnlineButton, &QAbstractButton::clicked, this, &CLC7ActivationDlg::slot_activateOnlineButton_clicked);
	connect(ui.licenseNameEdit, &QLineEdit::textChanged, this, &CLC7ActivationDlg::slot_licenseNameEdit_textChanged);
	connect(ui.activationKeyEdit, &QLineEdit::textChanged, this, &CLC7ActivationDlg::slot_activationKeyEdit_textChanged);
	connect(ui.cancelButton, &QAbstractButton::clicked, this, &CLC7ActivationDlg::reject);

	//ui.activationKeyEdit->setInputMask(">NNNNNNNNNNNNNNNN;_");

	UpdateUI();
}

CLC7ActivationDlg::~CLC7ActivationDlg()
{

}

void CLC7ActivationDlg::UpdateUI()
{
	bool enablebuttons = true;
	if (ui.licenseNameEdit->text().isEmpty() || ui.activationKeyEdit->text().isEmpty())
	{
		enablebuttons = false;
	}
	if (ui.activationKeyEdit->text().size() != 16)
	{
		enablebuttons = false;
	}
	ui.activateOfflineButton->setEnabled(enablebuttons);
	ui.activateOnlineButton->setEnabled(enablebuttons);
}

void CLC7ActivationDlg::slot_activateOfflineButton_clicked(int checked)
{
	//hide();

	QString licensename = ui.licenseNameEdit->text();
	QString activationkey = ui.activationKeyEdit->text();

	CLC7OfflineActivationDlg dlg(NULL, m_ctrl, licensename, activationkey);
	if (dlg.exec())
	{
		accept();
		return;
	}

	//show();
}

void CLC7ActivationDlg::slot_activateOnlineButton_clicked(int checked)
{
	QString licensename = ui.licenseNameEdit->text();
	QString activationkey = ui.activationKeyEdit->text().toUpper();

	ILC7Licensing *lic = m_ctrl->GetLicensingDomain(QUuid());
	ILC7Licensing::STATUS st = lic->Query(QString(), activationkey, QMap<QString,QVariant>());
	bool reactivate = false;
	if (st==ILC7Licensing::NEEDS_ACTIVATION)
	{

		//hide();

		CLC7OnlineActivationDlg dlg(NULL, m_ctrl, licensename, activationkey);
		if (dlg.exec())
		{
			accept();
			return;
		}

		//show();

	}
	else if (st == ILC7Licensing::NEEDS_REACTIVATION)
	{

		//hide();

		CLC7OnlineReactivationDlg dlg(NULL, m_ctrl, licensename, activationkey);
		if (dlg.exec())
		{
			accept();
			return;
		}

		//show();

	}
	else
	{
		QString err = QString("Licensing Error: %1").arg(lic->LastStatusDetails());
		m_ctrl->GetGUILinkage()->ErrorMessage("Error", err);
	}

	return;
}

void CLC7ActivationDlg::slot_licenseNameEdit_textChanged(const QString &)
{
	UpdateUI();
}

void CLC7ActivationDlg::slot_activationKeyEdit_textChanged(const QString &)
{
	UpdateUI();
}
