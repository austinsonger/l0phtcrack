#include"stdafx.h"

CPageSummary::CPageSummary(QWidget *parent)
	: CLC7WizardPage(parent)
{
	TR;
	setTitle(tr("Summary"));
	//setPixmap(QWizard::WatermarkPixmap, QPixmap(":/images/watermark.png"));

	QLabel *topLabel = new QLabel(tr("The following actions will be performed:"));
	topLabel->setWordWrap(true);

	m_actionsEdit = new QTextEdit(NULL);
	m_actionsEdit->setReadOnly(true);
	m_actionsEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget(topLabel);
	layout->addWidget(m_actionsEdit);
	setLayout(layout);

	UpdateUI();
}

void CPageSummary::initializePage()
{
	m_actionsEdit->clear();

	if (field("import_windows").toBool())
	{
		if (field("import_windows_local").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard()->page(CLC7Wizard::Page_Windows_Import_Local))->GetConfig();
			m_actionsEdit->append("* Import hashes from local Windows system");

			bool use_current_creds = config["use_current_creds"].toBool();
			bool use_saved_creds = config["use_saved_creds"].toBool();
			bool use_specific_creds = config["use_specific_creds"].toBool();
			QString username = config["username"].toString();
			QString domain = config["domain"].toString();
			bool save_creds = config["save_creds"].toBool();

			if (use_current_creds)
			{
				m_actionsEdit->append("   Use current credentials");
			}
			else if (use_saved_creds)
			{
				m_actionsEdit->append("   Use saved credentials");
			}
			else if (use_specific_creds)
			{
				m_actionsEdit->append("   Use specific credentials:");
				m_actionsEdit->append(QString("      Username: %1").arg(username));
				m_actionsEdit->append(QString("      Domain: %1").arg(domain));
			}
			if (save_creds)
			{
				m_actionsEdit->append("   Save credentials");
			}

		}
		else if (field("import_windows_remote_smb").toBool())
		{
			QMap<QString, QVariant> config;
			config = ((CLC7WizardPage *)wizard()->page(CLC7Wizard::Page_Windows_Import_Remote_SMB))->GetConfig();
			m_actionsEdit->append("* Import hashes from remote Windows system via Directory Services or an SMB remote agent");
		
			QString host = config["host"].toString();
			bool use_current_creds = config["use_current_creds"].toBool();
			bool use_saved_creds = config["use_saved_creds"].toBool();
			bool use_saved_default_creds = config["use_saved_default_creds"].toBool();
			bool use_specific_creds = config["use_specific_creds"].toBool();
			QString username = config["username"].toString();
			QString domain = config["domain"].toString();
			bool save_creds = config["save_creds"].toBool();
			bool save_default_creds = config["save_default_creds"].toBool();

			m_actionsEdit->append(QString("   Host: %1").arg(host));

			if (use_current_creds)
			{
				m_actionsEdit->append("   Use current credentials");
			}
			else if (use_saved_creds)
			{
				m_actionsEdit->append("   Use saved credentials");
			}
			else if (use_saved_default_creds)
			{
				m_actionsEdit->append("   Use default saved credentials");
			}
			else if (use_specific_creds)
			{
				m_actionsEdit->append("   Use specific credentials:");
				m_actionsEdit->append(QString("      Username: %1").arg(username));
				m_actionsEdit->append(QString("      Domain: %1").arg(domain));
			}
			if (save_creds)
			{
				m_actionsEdit->append("   Save credentials");
			}
			if (save_default_creds)
			{
				m_actionsEdit->append("   Save credentials as default");
			}
		}
		else if (field("import_windows_pwdump").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard()->page(CLC7Wizard::Page_Windows_Import_PWDump))->GetConfig();
			m_actionsEdit->append("* Import hashes from Windows PWDump-compatible file");

			QString filename = config["filename"].toString();

			m_actionsEdit->append(QString("   Filename: %1").arg(filename));
		}
	}
	else if (field("import_unix").toBool())
	{
		if (field("import_unix_ssh").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard()->page(CLC7Wizard::Page_Unix_Import_SSH))->GetConfig();

			bool use_saved_creds = config["use_saved_creds"].toBool();
			bool use_saved_default_creds = config["use_saved_default_creds"].toBool();
			bool use_specific_creds = config["use_specific_creds"].toBool();
			QString host = config["host"].toString();
			QStringList host_history = config["host_history"].toStringList();
			QString username = config["username"].toString();
			bool use_password_auth = config["use_password_auth"].toBool();
			bool use_public_key_auth = config["use_public_key_auth"].toBool();
			QString private_key_file = config["private_key_file"].toString();
			bool no_elevation = config["no_elevation"].toBool();
			bool sudo_elevation = config["sudo_elevation"].toBool();
			bool su_elevation = config["su_elevation"].toBool();
			bool save_creds = config["save_creds"].toBool();
			bool save_default_creds = config["save_default_creds"].toBool();
			FOURCC hashtype = (FOURCC)config["hashtype"].toUInt();

			ILC7PasswordLinkage *passlink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
			LC7HashType htype;
			QString error;
			QString hashname = "Unknown";
			if (passlink->LookupHashType(hashtype, htype, error))
			{
				hashname = htype.description;
			}

			m_actionsEdit->append(QString("* Import hashes from remote Unix machine via SSH"));
			m_actionsEdit->append(QString("   Host: %1").arg(host));
			if (use_saved_creds)
			{
				m_actionsEdit->append("   Use saved credentials for this host");
			}
			else if (use_saved_default_creds)
			{
				m_actionsEdit->append("   Use default saved credentials");
			}
			else if (use_specific_creds)
			{
				m_actionsEdit->append("   Use specific credentials:");
				m_actionsEdit->append(QString("      Username: %1").arg(username));
				if (use_password_auth)
				{
					m_actionsEdit->append(QString("      Use password authentication"));
				}
				else if (use_public_key_auth)
				{
					m_actionsEdit->append(QString("      Use public key authentication"));
				}
				if (no_elevation)
				{
					m_actionsEdit->append(QString("      Use no elevation"));
				}
				else if (sudo_elevation)
				{
					m_actionsEdit->append(QString("      Use 'sudo' elevation"));
				}
				else if (su_elevation)
				{
					m_actionsEdit->append(QString("      Use 'su' elevation"));
				}
				if (save_creds)
				{
					m_actionsEdit->append(QString("      Save credentials for this host"));
				}
				if (save_creds)
				{
					m_actionsEdit->append(QString("      Save credentials as default"));
				}
			}
			m_actionsEdit->append(QString("   Hash Type: %1").arg(hashname));


		}
		else if (field("import_unix_shadow").toBool())
		{
			QMap<QString, QVariant> config = ((CLC7WizardPage *)wizard()->page(CLC7Wizard::Page_Unix_Import_File))->GetConfig();

			QString file1name = config["file1name"].toString();
			QString file2name = config["file2name"].toString();
			QString file3name = config["file3name"].toString();

			m_actionsEdit->append(QString("* Import hashes from file"));
			if (!file1name.isEmpty())
			{
				m_actionsEdit->append(QString("   File 1: %1").arg(file1name));
			}
			if (!file2name.isEmpty())
			{
				m_actionsEdit->append(QString("   File 2: %1").arg(file2name));
			}
			if (!file3name.isEmpty())
			{
				m_actionsEdit->append(QString("   File 3: %1").arg(file3name));
			}
		}
	}
	
	m_actionsEdit->append("");

	if (field("audit_quick").toBool())
	{
		m_actionsEdit->append("* Perform 'quick' audit");
		m_actionsEdit->append("   User-info single mode attack");
		m_actionsEdit->append("   Dictionary attack (wordlist-big.txt, common permutations)");
	}
	else if (field("audit_common").toBool())
	{
		m_actionsEdit->append("* Perform 'common' audit");
		m_actionsEdit->append("   User-info single mode attack");
		m_actionsEdit->append("   Dictionary attack (wordlist-big.txt, jumbo permutations)");
		m_actionsEdit->append("   Brute-force attack (1 hour, alphanumeric+space)");
	}
	else if (field("audit_thorough").toBool())
	{
		m_actionsEdit->append("* Perform 'thorough' audit");
		m_actionsEdit->append("   User-info single mode attack");
		m_actionsEdit->append("   Dictionary attack (wordlist-huge.txt, jumbo permutations)");
		m_actionsEdit->append("   Brute-force attack (6 hours, ASCII)");
	}
	else if (field("audit_strong").toBool())
	{
		m_actionsEdit->append("* Perform 'strong' audit");
		m_actionsEdit->append("   User-info single mode attack");
		m_actionsEdit->append("   Dictionary attack (wordlist-small.txt, all permutations)");
		m_actionsEdit->append("   Brute-force attack (24 hours, ISO-8859-1)");
	}

	if (field("report_export").toBool())
	{
		if (field("report_csv").toBool())
		{
			m_actionsEdit->append("\n* Export CSV file");
		}
		else if (field("report_html").toBool())
		{
			m_actionsEdit->append("\n* Export HTML file");
		}
		else if (field("report_xml").toBool())
		{
			m_actionsEdit->append("\n* Export XML file");
		}
		QString file = field("report_file").toString();
		m_actionsEdit->append(QString("   Filename: %1").arg(file));
	}

	m_actionsEdit->append("\nDisplay Options:");
	if (field("report_display_passwords").toBool())
	{
		m_actionsEdit->append("   Display passwords");
	}
	else
	{
		m_actionsEdit->append("   DO NOT Display passwords");
	}
	if (field("report_display_hashes").toBool())
	{
		m_actionsEdit->append("   Display hashes");
	}
	else
	{
		m_actionsEdit->append("   DO NOT Display hashes");
	}
	/*
	if (field("report_display_audit_time").toBool())
	{
		m_actionsEdit->append("   Display audit time");
	}
	else
	{
		m_actionsEdit->append("   DO NOT Display audit time");
	}
	if (field("report_display_audit_method").toBool())
	{
		m_actionsEdit->append("   Display audit method");
	}
	else
	{
		m_actionsEdit->append("   DO NOT Display audit method");
	}
	*/
	m_actionsEdit->append("\nScheduling Options:");
	if (field("schedule_now").toBool())
	{
		m_actionsEdit->append("   Run this audit immediately");
	}
	else if (field("schedule_later").toBool())
	{
		m_actionsEdit->append("   Schedule this audit for later:");
		QDateTime dt = field("schedule_datetime").toDateTime();
		QString timestr = dt.toString(Qt::DateFormat::DefaultLocaleLongDate);
		m_actionsEdit->append(QString("      Time: %1").arg(timestr));
	}

}

int CPageSummary::nextId() const
{TR;
	return -1;
}

bool CPageSummary::isComplete() const
{TR;
	return true;
}



void CPageSummary::UpdateUI()
{
	TR;
}
