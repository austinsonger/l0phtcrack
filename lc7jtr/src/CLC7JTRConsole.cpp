#include "stdafx.h"
#include "CLC7JTRConsole.h"

GenerateCharsetThread::GenerateCharsetThread(CLC7JTRConsole *console)
{TR;
	m_console = console;

	connect(this, &GenerateCharsetThread::sig_consoleOut, m_console, &CLC7JTRConsole::slot_consoleOut);
	connect(this, &QThread::finished, m_console, &CLC7JTRConsole::slot_threadFinished);
}

void GenerateCharsetThread::ProcessStdOut(QByteArray str)
{TR;
	emit sig_consoleOut(QString::fromUtf8(str));
}

void GenerateCharsetThread::ProcessStdErr(QByteArray str)
{TR;
	emit sig_consoleOut(QString::fromUtf8(str));
}

void GenerateCharsetThread::run(void)
{TR;
	QString pottmp = g_pLinkage->NewTemporaryFile();
	QFile ptfile(pottmp);
	if (!ptfile.open(QIODevice::WriteOnly))
	{
		m_exitcode = 1;
		emit sig_consoleOut("Error: Unable to open temporary file.");
		QFile::remove(pottmp);
		return;
	}

	if (m_inchars.size() > 0)
	{
		emit sig_consoleOut("Parsing input characters...");
		foreach(unsigned char b, m_inchars)
		{
			ptfile.write(QByteArray(":") + QByteArray(1, b) + QByteArray("\n"));
		}
		emit sig_consoleOut("done\n");

	}
	if (m_dictionary_files.size() > 0)
	{
		foreach(QString df, m_dictionary_files)
		{
			emit sig_consoleOut(QString("Parsing dictionary file '%1'...").arg(QDir::toNativeSeparators(df)));
			QFile dffile(df);
			if (!dffile.open(QIODevice::ReadOnly))
			{
				m_exitcode = 2;
				emit sig_consoleOut(QString("Error: Unable to open dictionary file: %1\n").arg(QDir::toNativeSeparators(df)));
				ptfile.close();
				QFile::remove(pottmp);
				return;
			}
			while (!dffile.atEnd())
			{
				QByteArray line = dffile.readLine();
				line = QByteArray(":") + line;
				ptfile.write(line);
			}
			emit sig_consoleOut("done\n");
		}
	}
	if (m_pot_files.size() > 0)
	{
		foreach(QString pf, m_pot_files)
		{
			emit sig_consoleOut(QString("Parsing pot file '%1'...").arg(QDir::toNativeSeparators(pf)));
			QFile pffile(pf);
			if (!pffile.open(QIODevice::ReadOnly))
			{
				m_exitcode = 2;
				emit sig_consoleOut(QString("Error: Unable to open pot file: %1\n").arg(QDir::toNativeSeparators(pf)));
				ptfile.close();
				QFile::remove(pottmp);
				return;
			}
			while (!pffile.atEnd())
			{
				QByteArray line = pffile.readLine();
				ptfile.write(line);
			}
			emit sig_consoleOut("done\n");
		}
	}

	ptfile.close();

	CLC7ExecuteJTR exejtr("sse2");
	QStringList args;
	args << QString("--make-charset=%1").arg(QDir::toNativeSeparators(m_charset_file));
	args << QString("--pot=%1").arg(QDir::toNativeSeparators(pottmp));
	exejtr.SetCommandLine(args);
	int res = exejtr.ExecutePipe(this);
	QFile::remove(pottmp);
	
	m_exitcode = res;
}


/////////////////////////////////////////////////////////////////////////

CLC7JTRConsole::CLC7JTRConsole()
{TR;
	ui.setupUi(this);

	const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	ui.consoleText->setFont(fixedFont);

	connect(ui.buttonBox, &QDialogButtonBox::accepted, this, &CLC7CharsetEditorDlg::accept);
	connect(ui.buttonBox, &QDialogButtonBox::rejected, this, &CLC7CharsetEditorDlg::reject);

	m_executing_thread = NULL;

	UpdateUI();
}

CLC7JTRConsole::~CLC7JTRConsole()
{TR;
}

void CLC7JTRConsole::UpdateUI()
{TR;
	if (m_executing_thread && m_executing_thread->exitCode() == 0)
	{
		ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
		ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	}
	else
	{
		ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
		ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
	}
}

void CLC7JTRConsole::slot_consoleOut(QString str)
{TR;
	ui.consoleText->moveCursor(QTextCursor::End);
	ui.consoleText->insertPlainText(str);
	ui.consoleText->moveCursor(QTextCursor::End);
	//ui.consoleText->appendPlainText(str);
}

void CLC7JTRConsole::slot_threadFinished(void)
{TR;
	UpdateUI();
}

int CLC7JTRConsole::execGenerateCharset(QString charset_file, QByteArray inchars, QStringList dictionary_files, QStringList pot_files)
{TR;
	GenerateCharsetThread *thr = new GenerateCharsetThread(this);
	thr->m_charset_file = charset_file;
	thr->m_inchars = inchars;
	thr->m_dictionary_files = dictionary_files;
	thr->m_pot_files = pot_files;
	
	m_executing_thread = thr;
	thr->start();

	int res = exec();

	if (!thr->isFinished())
	{
		thr->terminate();
	}

	m_executing_thread = NULL;
	delete thr;
	
	return res;
	
}
