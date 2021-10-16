#include<stdafx.h>

CLC7WizardPage::CLC7WizardPage(QWidget *parent) :QWizardPage(parent)
{
	m_is_valid = true;
	m_configWidget = NULL;
}

CLC7WizardPage::~CLC7WizardPage()
{
	if (m_configWidget)
	{
		delete m_configWidget;
	}
}

void CLC7WizardPage::slot_abstractbutton_clicked(bool checked)
{
	TR;
	UpdateUI();
	emit completeChanged();
}

void CLC7WizardPage::slot_groupbox_clicked(bool checked)
{
	TR;
	UpdateUI();
	emit completeChanged();
}

void CLC7WizardPage::slot_lineedit_textchanged(const QString &text)
{
	TR;

	UpdateUI();
	emit completeChanged();
}

void CLC7WizardPage::slot_isValid(bool valid)
{
	TR;

	m_is_valid = valid;

	UpdateUI();
	emit completeChanged();
}

bool CLC7WizardPage::isComplete() const
{
	return m_is_valid;
}

void CLC7WizardPage::CreateConfigWidget(QUuid widgetuuid, QMap<QString, QVariant> config)
{
	m_configComponent = g_pLinkage->FindComponentByID(widgetuuid);

	config["pagewidget"] = (qulonglong)this;
	QStringList args;
	args << "create";
	QString error;
	if (m_configComponent->ExecuteCommand("gui", args, config, error, NULL) != ILC7Component::SUCCESS)
	{
		Q_ASSERT(0);
		return;
	}
	m_configWidget = (QWidget *)config["widget"].toULongLong();
	m_configWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QMap<QString, QVariant> CLC7WizardPage::GetConfig()
{
	QMap<QString, QVariant> config;
	config["widget"] = (qulonglong)m_configWidget;

	QStringList args;
	args << "store";
	QString error;
	if (m_configComponent->ExecuteCommand("gui", args, config, error, NULL) != ILC7Component::SUCCESS)
	{
		Q_ASSERT(0);
		return QMap<QString, QVariant>();
	}
	config.remove("widget");

	return config;
}
