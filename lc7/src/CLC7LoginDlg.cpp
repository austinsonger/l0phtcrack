#include<stdafx.h>


CLC7LoginDlg::CLC7LoginDlg(QWidget *parent, ILC7Controller *ctrl, QString title, QString message, QString username, QString password) : QDialog(parent, Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);
	m_ctrl = ctrl;
	
	setWindowTitle(title);
	ui.message->setText(message);
	ui.username->setText(username);
	ui.password->setText(password);

	UpdateUI();
}

CLC7LoginDlg::~CLC7LoginDlg()
{

}

QString CLC7LoginDlg::Username()
{
	return ui.username->text();
}

QString CLC7LoginDlg::Password()
{
	return ui.password->text();
}


void CLC7LoginDlg::UpdateUI()
{
	ui.okButton->setEnabled(ui.username->text().size() > 0);
}
