#include<stdafx.h>
#include<QPrinter>
//#include<qwebpage.h>
//#include<QWebFrame>

CAccountsExport::CAccountsExport(ILC7AccountList *accountlist, ILC7CommandControl *ctrl)
{
    m_accountlist = accountlist;
    m_ctrl = ctrl;
	m_plink = GET_ILC7PASSWORDLINKAGE(g_pLinkage);
	m_current_page=0;
	m_page_total=0;
}

CAccountsExport::~CAccountsExport()
{
}

void CAccountsExport::setFilename(QString filename)
{
    m_filename = filename;
}

void CAccountsExport::setFormat(QString format)
{
    m_format = format;
}

void CAccountsExport::setIncludeStyle(bool style)
{
    m_include_style = style;
}

void CAccountsExport::enableColumn(QString colname, bool enable)
{
    m_column_enabled[colname] = enable;
}

void CAccountsExport::DetermineHashes(void)
{
	m_accountlist->Acquire();

    int i, cnt = m_accountlist->GetAccountCount();
    for (i = 0; i < cnt; i++)
    {
        const LC7Account *acct = m_accountlist->GetAccountAtConstPtrFast(i);
        foreach(LC7Hash lc7hash, acct->hashes)
        {
            if (!m_hashtypes.contains(lc7hash.hashtype))
            {
                m_hashtypes.append(lc7hash.hashtype);
            }
        }
    }

	m_accountlist->Release();
}

bool CAccountsExport::DoExport(QString &error, bool &cancelled)
{
    cancelled = false;

    DetermineHashes();

    QFile outfile(m_filename);
    if (!outfile.open(QIODevice::WriteOnly))
    {
        error = "Unable to open file for export.";
        return false;
    }

	if (m_format == "PDF")
	{
		outfile.close();
		return EmitDocument_PDF(error);
	}

	m_ctrl->SetStatusText("Rendering accounts");
	m_ctrl->UpdateCurrentProgressBar(0);

    if (m_format=="CSV")
    {
        StartDocument_CSV(outfile);
    }
    else if (m_format == "HTML")
    {
        StartDocument_HTML(outfile);
    }
    else if (m_format == "XML")
    {
        StartDocument_XML(outfile);
    }

	m_accountlist->Acquire();
    int i, cnt = m_accountlist->GetAccountCount();
    for (i = 0; i < cnt; i++)
    {
		if (m_ctrl->StopRequested())
		{
			break;
		}

		const LC7Account * acct = m_accountlist->GetAccountAtConstPtrFast(i);
    
        if (m_format == "CSV")
        {
            EmitAccount_CSV(outfile, *acct);
        }
        else if (m_format == "HTML")
        {
            EmitAccount_HTML(outfile, *acct);
        }
        else if (m_format == "XML")
        {
            EmitAccount_XML(outfile, *acct);
        }

		if ((i % 100) == 0)
		{
			m_ctrl->SetStatusText(QString("Rendering accounts (%1/%2)").arg(i).arg(cnt));
			m_ctrl->UpdateCurrentProgressBar((quint32)(i * 100 / cnt));
		}
	}
	m_ctrl->SetStatusText(QString("Rendered accounts"));

    if (m_format == "CSV")
    {
        EndDocument_CSV(outfile);
    }
    else if (m_format == "HTML")
    {
        EndDocument_HTML(outfile);
    }
    else if (m_format == "XML")
    {
        EndDocument_XML(outfile);
    }

	m_accountlist->Release();

    outfile.close();

	m_ctrl->UpdateCurrentProgressBar(100);

    return true;
}

QStringList CAccountsExport::GenerateColumnNames()
{
    QStringList columns;
    if (m_column_enabled["domain"])
    {
        columns << "Domain";
    }
    if (m_column_enabled["username"])
    {
        columns << "Username";
    }

    foreach(fourcc hashtype, m_hashtypes)
    {
        LC7HashType lc7hashtype;
        QString error;
        QString name = "Unknown";
        if (m_plink->LookupHashType(hashtype, lc7hashtype, error))
        {
            name = lc7hashtype.name;
        }

        if (m_column_enabled["hashes"])
        {
            columns << QString("%1 Hash").arg(name);
        }
        if (m_column_enabled["passwords"])
        {
            columns << QString("%1 Password").arg(name);
        }
        if (m_column_enabled["audited_status"])
        {
            columns << QString("%1 State").arg(name);
        }
    }

    if (m_column_enabled["user_info"])
    {
        columns << "User Info";
    }
    if (m_column_enabled["user_id"])
    {
        columns << "User Id";
    }
    if (m_column_enabled["machine"])
    {
        columns << "Machine";
    }
    if (m_column_enabled["last_changed_time"])
    {
        columns << "Last Changed Time";
    }

    if (m_column_enabled["locked_out"])
    {
        columns << "Lockout";
        columns << "Disabled";
        columns << "Expired";
        columns << "No Expire";
    }

    return columns;
}

QStringList CAccountsExport::GenerateColumns(const LC7Account &acct)
{
    QStringList columns;

    if (m_column_enabled["domain"])
    {
        columns << acct.domain;
    }
    if (m_column_enabled["username"])
    {
        columns << acct.username;
    }

	foreach(fourcc hashtype, m_hashtypes)
	{
		QString hash = "", password = "", status = "";

		foreach(LC7Hash lc7hash, acct.hashes)
		{
			if (lc7hash.hashtype == hashtype)
			{
				hash = QString::fromLatin1(lc7hash.hash);
				password = lc7hash.password;
				// status
				if (lc7hash.crackstate == CRACKSTATE_NOT_CRACKED)
				{
					status = "Not Cracked";
				}
				else if (lc7hash.crackstate == CRACKSTATE_FIRSTHALF_CRACKED)
				{
					status = "1st Half Cracked";
				}
				else if (lc7hash.crackstate == CRACKSTATE_SECONDHALF_CRACKED)
				{
					status = "2nd Half Cracked";
				}
				else
				{
					QString hashstate;

					if (!lc7hash.cracktype.isEmpty())
					{
						hashstate = QString("Cracked (%1): ").arg(lc7hash.cracktype);
					}
					else
					{
						hashstate = QString("Cracked: ");
					}
					quint32 secs = lc7hash.cracktime % 60;
					quint32 mins = (lc7hash.cracktime / 60) % 60;
					quint32 hrs = (lc7hash.cracktime / 3600) % 24;
					quint32 days = (lc7hash.cracktime / (3600 * 24));

					QString out;
					if (days > 0)
					{
						out += QString("%1d").arg(days);
					}
					if (hrs > 0 || out.size() > 0)
					{
						out += QString("%1h").arg(hrs);
					}
					if (mins > 0 || out.size() > 0)
					{
						out += QString("%1m").arg(mins);
					}
					if (secs > 0 || out.size() > 0)
					{
						out += QString("%1s").arg(secs);
					}
					if (out.size() == 0)
					{
						out = "instantly";
					}

					hashstate += out;

					status = hashstate;
				}
			}
		}
		if (m_column_enabled["hashes"])
		{
			columns << hash;
		}
		if (m_column_enabled["passwords"])
		{
			columns << password;
		}
		if (m_column_enabled["audited_status"])
		{
			columns << status;
		}
	}

    if (m_column_enabled["user_info"])
    {
        columns << acct.userinfo;
    }
    if (m_column_enabled["user_id"])
    {
        columns << acct.userid;
    }
    if (m_column_enabled["machine"])
    {
        columns << acct.machine;
    }
    if (m_column_enabled["last_changed_time"])
    {
		if (acct.lastchanged == 0)
		{
			columns << "";
		}
		else
		{
			columns << QDateTime::fromTime_t(acct.lastchanged).toString(Qt::ISODate);
		}
    }
    if (m_column_enabled["locked_out"])
    {
        columns << (acct.lockedout ? "Y" : "N");
        columns << (acct.disabled ? "Y" : "N");
        columns << (acct.mustchange ? "Y" : "N");
        columns << (acct.neverexpires ? "Y" : "N");
    }

    return columns;
}

///////////
// CSV
///////////

void CAccountsExport::StartDocument_CSV(QIODevice & outfile)
{
    outfile.setTextModeEnabled(true);

    QString headers;

    QStringList columns = GenerateColumnNames();

    bool first = true;
    foreach(QString colname, columns)
    {
        if (!first)
        {
            headers += ",";
        }
        else
        {
            first = false;
        }

        headers += colname;
    }

    outfile.write(headers.toUtf8() + "\n");
}

void CAccountsExport::EmitAccount_CSV(QIODevice & outfile, const LC7Account &acct)
{
    QStringList columns = GenerateColumns(acct);

    QString line;
    bool first = true;
    foreach(QString col, columns)
    {
        if (!first)
        {
            line += ",";
        }
        else
        {
            first = false;
        }

//        if (col.contains("Not Cracked"))
//        {
//            int i = 1;
//        }

		col = col.replace("\n", "");
		col = col.replace("\r", "");
		if (col.contains(",") || col.contains("\""))
        {
            col = col.replace("\"", "\"\"");
            col = QString("\"%1\"").arg(col);
        }

        line += col;
    }

    outfile.write(line.toUtf8() + "\n");
}

void CAccountsExport::EndDocument_CSV(QIODevice & outfile)
{
}

///////////
// PDF
///////////

/*
void CAccountsExport::StartDocument_PDF(QIODevice & outfile)
{
}

void CAccountsExport::EmitAccount_PDF(QIODevice & outfile, const LC7Account &acct)
{
}

void CAccountsExport::EndDocument_PDF(QIODevice & outfile)
{
}
*/

void PrintCallback(QList<QVariant> args)
{
	QPainter *painter = (QPainter *)(args[0].toULongLong());
	QPrinter *printer = (QPrinter *)(args[1].toULongLong());
	QString htmlstr = args[2].toString();

//	QWebPage page;
//	page.mainFrame()->setHtml(htmlstr);

//	page.mainFrame()->render(painter, QWebFrame::ContentsLayer);//, printer->pageRect().translated(-printer->pageRect().x(), -printer->pageRect().y()));
}

void PrintWholeCallback(QList<QVariant> args)
{
	QPrinter *printer = (QPrinter *)(args[0].toULongLong());
	QString htmlstr = args[1].toString();

//	QWebPage page;
//	page.mainFrame()->setHtml(htmlstr);

//	page.mainFrame()->print(printer);
}

bool CAccountsExport::EmitPage_PDF(QPainter *painter, QPrinter *printer, int page, int count, QString &error)
{
	QBuffer htmlbuffer;
	htmlbuffer.open(QIODevice::ReadWrite);

	m_current_page = page + 1;
	m_page_total = count;

	StartDocument_HTML(htmlbuffer);

	for (int i = (page*30); i < ((page+1)*30); i++)
	{
		const LC7Account * acct = m_accountlist->GetAccountAtConstPtrFast(i);
		EmitAccount_HTML(htmlbuffer, *acct);
	}

	EndDocument_HTML(htmlbuffer);

	QString htmlstr = QString::fromUtf8(htmlbuffer.buffer());

	bool success = true;

	QList<QVariant> args;
	args.append((qulonglong)painter);
	args.append((qulonglong)printer);
	args.append(htmlstr);
	
	g_pLinkage->GetGUILinkage()->Callback(PrintCallback, args);

	return success;

}

bool CAccountsExport::EmitDocument_PDF(QString &error)
{
//	QPdfWriter printer(m_filename);
//	printer.setResolution(300);
//	printer.setPageSize(QPrinter::Letter);

	QPrinter printer(QPrinter::HighResolution);
	printer.setOutputFormat(QPrinter::PdfFormat);
	printer.setOutputFileName(m_filename);
	printer.setPageSize(QPrinter::Letter);

	QBuffer htmlbuffer;
	htmlbuffer.open(QIODevice::ReadWrite);

	StartDocument_HTML(htmlbuffer);

	m_ctrl->SetStatusText(QString("Rendering accounts"));

	size_t cnt = m_accountlist->GetAccountCount();
	for (int i = 0; i < cnt; i++)
	{
		if ((i % 100) == 0)
		{
			m_ctrl->SetStatusText(QString("Rendering accounts (%1/%2)").arg(i).arg(cnt));
		}
	
		const LC7Account * acct = m_accountlist->GetAccountAtConstPtrFast(i);
		EmitAccount_HTML(htmlbuffer, *acct);
	}

	m_ctrl->SetStatusText(QString("Rendered accounts"));

	EndDocument_HTML(htmlbuffer);

	QString htmlstr;
	htmlstr = QString::fromUtf8(htmlbuffer.buffer());

	bool success = true;
	m_ctrl->SetStatusText(QString("Rendering PDF File"));

	QList<QVariant> args;
	args.append((qulonglong)&printer);
	args.append(htmlstr);

	g_pLinkage->GetGUILinkage()->Callback(PrintWholeCallback, args);

	return success;





//	QPrinter printer(QPrinter::HighResolution);
//	printer.setOutputFormat(QPrinter::PdfFormat);
//	printer.setOutputFileName(m_filename);


//	QPainter pnt;
//	pnt.begin(&printer);

	//page.mainFrame()->setTextSizeMultiplier(1.5);
	/*
	m_ctrl->SetStatusText("Rendering pages");
	int cnt = (int)m_accountlist->GetAccountCount();
	int pages = (cnt + 29) / 30;
	m_ctrl->UpdateCurrentProgressBar(0);
	for (int i = 0; i < pages; i++)
	{
		if (m_ctrl->StopRequested())
		{
			break;
		}
		m_ctrl->SetStatusText(QString("Rendering page (%1/%2)").arg(i+1).arg(pages));

		if (!EmitPage_PDF(&pnt, &printer, i, pages, error))
		{
			return false;
		}

		bool success = printer.printerState() != QPrinter::Error;
		if (!success)
		{
			error = "Unable to write to PDF file";
			return false;
		}
		m_ctrl->UpdateCurrentProgressBar(i * 100 / cnt);

		if (i != (pages - 1))
		{
			printer.newPage();
		}
	}
	
	pnt.end();
	*/
	m_ctrl->SetStatusText(QString("Rendered pages"));
	m_ctrl->UpdateCurrentProgressBar(100);

	return true;
}





///////////
// HTML
///////////

void CAccountsExport::StartDocument_HTML(QIODevice & outfile)
{
    outfile.setTextModeEnabled(true);

    m_alt = false;

    QString sessionstr;
    if (!g_pLinkage->IsSessionOpen(&sessionstr))
    {
        Q_ASSERT(0);
    }
    if (sessionstr.isEmpty())
    {
        sessionstr = "Untitled Session";
    }
	else
	{
		sessionstr = QDir::toNativeSeparators(sessionstr);
	}

    QString datestr = QDateTime::currentDateTime().toString(Qt::SystemLocaleLongDate);

    QString title = QString("L0phtCrack 7 Accounts Export - (%1) %2")
        .arg(sessionstr)
        .arg(datestr);

	QString header;
	
	if (m_include_style && m_format == "HTML")
	{
		header = QString("<h1>L0phtCrack 7 Accounts Export</h1><h2>(%1)</h2><i>%2</i><br><br>")
			.arg(sessionstr)
			.arg(datestr);
	}
	else if (m_include_style && m_format == "PDF" && m_current_page == 1)
	{
		header = QString("<h1>L0phtCrack 7 Accounts Export</h1><h2>(%1)</h2><i>%2</i><br><br>")
			.arg(sessionstr)
			.arg(datestr);
	}
	else if (m_include_style && m_format == "PDF" && m_current_page != 1)
	{
		header = QString("<h3>L0phtCrack 7 Accounts Export</h1><h2>(%1)</h2><i>%2</i><br><br>")
			.arg(sessionstr)
			.arg(datestr);
	}
	
    QString style;
	if ((m_include_style && m_format == "HTML") || m_format=="PDF")
    {
        style = QString(
            "            <style>\n"
            "                .datagrid table { border-collapse: collapse; text-align: left; width: 100%; }\n"
            "                .datagrid { font: normal 12pt/150% Arial, Helvetica, sans-serif; background: #fff; overflow: hidden; border: 1px solid #8C8C8C; -webkit-border-radius: 3px; -moz-border-radius: 3px; border-radius: 3px; }\n"
            "                .datagrid table td, .datagrid table th { padding: 3px 10px; }\n"
            "                .datagrid table thead th { background:-webkit-gradient( linear, left top, left bottom, color-stop(0.05, #8C8C8C), color-stop(1, #7D7D7D) ); background:-moz-linear-gradient( center top, #8C8C8C 5%, #7D7D7D 100% );filter:progid:DXImageTransform.Microsoft.gradient(startColorstr='#8C8C8C', endColorstr='#7D7D7D');background-color:#8C8C8C; color:#FFFFFF; font-size: 12pt; font-weight: bold; border-left: 1px solid #A3A3A3; }\n" 
            "                .datagrid table thead th:first-child { border: none; }\n"
            "                .datagrid table tbody td { color: #7D7D7D; border-left: 1px solid #DBDBDB;font-size: 10pt;font-weight: normal; }\n"
            "                .datagrid table tbody .alt td { background: #EBEBEB; color: #7D7D7D; }\n"
            "                .datagrid table tbody td:first-child { border-left: none; }\n"
            "                .datagrid table tbody tr:last-child td { border-bottom: none; }\n"
            "            </style>\n"
            );
    }

    outfile.write(QString(
        "<!DOCTYPE html>\n"
        "    <html>\n"
        "        <head>\n"
        "            <meta charset = \"utf-8\">\n"
        "            <title>%1</title>\n"
        "%2"
        "        </head>\n"
        "    <body>\n"
        "%3"
        "        <table>\n"
        "            <thead><tr>\n"
        )
        .arg(title.toHtmlEscaped())
        .arg(style)
        .arg(m_include_style ? QString("%1            \n<div class=\"datagrid\">\n").arg(header):QString("")).toUtf8());

    QStringList columns = GenerateColumnNames();
    QString headers;
    foreach(QString colname, columns)
    {
        colname = colname.toHtmlEscaped();
        headers += QString("                <th>%1</th>\n").arg(colname);
    }

    outfile.write(headers.toUtf8());

    outfile.write(
        "            </tr></thead>\n"
        );
}

void CAccountsExport::EmitAccount_HTML(QIODevice & outfile, const LC7Account &acct)
{
    outfile.write(
        QString("            <tbody><tr%1>\n")
        .arg((((m_include_style && m_format=="HTML") || m_format=="PDF") && m_alt) ? " class=\"alt\"" : "").toUtf8()
        );

    QStringList columns = GenerateColumns(acct);
    QString rows;
    foreach(QString col, columns)
    {
        col = col.toHtmlEscaped();
        rows += QString("                <td>%2</td>\n")
            .arg(col);
    }

    outfile.write(rows.toUtf8());

    outfile.write(
        "            </tr></tbody>\n"
        );

    m_alt = !m_alt;
}

void CAccountsExport::EndDocument_HTML(QIODevice & outfile)
{
    outfile.write(QString(
        "        </table>\n"
        "%1"
        "    </body>\n"
        "</html>\n"
        ).arg(((m_include_style && m_format=="HTML") || m_format=="PDF") ? "        </div>\n" : "").toUtf8());
}


///////////
// HTML
///////////

void CAccountsExport::StartDocument_XML(QIODevice & outfile)
{
    outfile.setTextModeEnabled(true);

    m_alt = false;

    QString sessionstr;
    if (!g_pLinkage->IsSessionOpen(&sessionstr))
    {
        Q_ASSERT(0);
    }
    if (!sessionstr.isEmpty())
    {
        sessionstr = QDir::toNativeSeparators(sessionstr);
    }

    QString datestr = QDateTime::currentDateTime().toString(Qt::ISODate);

    outfile.write(QString(
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<accounts sessionfile=\"%1\" exporttime=\"%2\">\n")
        .arg(sessionstr.toHtmlEscaped())
        .arg(datestr.toHtmlEscaped()).toUtf8());
}

void CAccountsExport::EmitAccount_XML(QIODevice & outfile, const LC7Account &acct)
{
    outfile.write("    <account>\n");
    if (m_column_enabled["domain"])
    {
        outfile.write(QString("            <domain>%1</domain>\n").arg(acct.domain.toHtmlEscaped()).toUtf8());
    }
    if (m_column_enabled["username"])
    {
        outfile.write(QString("            <username>%1</username>\n").arg(acct.username.toHtmlEscaped()).toUtf8());
    }

    if (acct.hashes.size() > 0)
    {
        outfile.write(QString("            <hashtypes>\n").toUtf8());
    }

    foreach(LC7Hash lc7hash, acct.hashes)
    {
        LC7HashType lc7hashtype;
        QString err;
        if (!m_plink->LookupHashType(lc7hash.hashtype, lc7hashtype, err))
        {
            Q_ASSERT(0);
            continue;
        }

        outfile.write(QString("                <hashtype name=\"%1\">\n").arg(lc7hashtype.name.toHtmlEscaped()).toUtf8());
        if (m_column_enabled["hashes"])
        {
			outfile.write(QString("                    <hash>%1</hash>\n").arg(QString::fromLatin1(lc7hash.hash).toHtmlEscaped()).toUtf8());
        }
        if (m_column_enabled["passwords"])
        {
			outfile.write(QString("                    <password>%1</password>\n").arg(lc7hash.password.toHtmlEscaped()).toUtf8());
        }
        if (m_column_enabled["audited_status"])
        {
            // status
            if (lc7hash.crackstate == CRACKSTATE_NOT_CRACKED)
            {
                outfile.write(QString("                    <status>not cracked</status>\n").toUtf8());
            }
            else if (lc7hash.crackstate == CRACKSTATE_FIRSTHALF_CRACKED)
            {
                outfile.write(QString("                    <status>1st half cracked</status>\n").toUtf8());
            }
            else if (lc7hash.crackstate == CRACKSTATE_SECONDHALF_CRACKED)
            {
                outfile.write(QString("                    <status>2nd half cracked</status>\n").toUtf8());
            }
            else
            {
                outfile.write(QString("                    <status>cracked</status>\n").toUtf8());
				outfile.write(QString("                    <cracktype>%1</cracktype>\n").arg(lc7hash.cracktype.toHtmlEscaped()).toUtf8());
                outfile.write(QString("                    <cracktime>%1</cracktime>\n").arg(lc7hash.cracktime).toUtf8());
            }
        }
        outfile.write(QString("                </hashtype>\n").toUtf8());
    }
	if (acct.hashes.size() > 0)
	{
		outfile.write(QString("            </hashtypes>\n").toUtf8());
	}

    if (m_column_enabled["user_info"])
    {
        outfile.write(QString("            <userinfo>%1</userinfo>\n").arg(acct.userinfo.toHtmlEscaped()).toUtf8());
    }
    if (m_column_enabled["user_id"])
    {
		outfile.write(QString("            <userid>%1</userid>\n").arg(acct.userid.toHtmlEscaped()).toUtf8());
    }
    if (m_column_enabled["machine"])
    {
        outfile.write(QString("            <machine>%1</machine>\n").arg(acct.machine.toHtmlEscaped()).toUtf8());
    }
    if (m_column_enabled["last_changed_time"])
    {
        QString last_changed_time = QDateTime::fromTime_t(acct.lastchanged).toString(Qt::ISODate);
        outfile.write(QString("            <lastchanged>%1</lastchanged>\n").arg(acct.machine.toHtmlEscaped()).toUtf8());
    }
    if (m_column_enabled["locked_out"])
    {
        outfile.write(QString("        <lockedout>%1</lockedout>\n").arg(acct.lockedout?1:0).toUtf8());
        outfile.write(QString("        <disabled>%1</disabled>\n").arg(acct.disabled?1:0).toUtf8());
        outfile.write(QString("        <expired>%1</expired>\n").arg(acct.mustchange?1:0).toUtf8());
        outfile.write(QString("        <neverexpires>%1</neverexpires>\n").arg(acct.neverexpires?1:0).toUtf8());
    }
    outfile.write("    </account>\n");
}

void CAccountsExport::EndDocument_XML(QIODevice & outfile)
{
    outfile.write(QString("</accounts>").toUtf8());
}


