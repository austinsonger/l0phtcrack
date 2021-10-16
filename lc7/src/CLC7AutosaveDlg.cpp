#include<stdafx.h>


CLC7AutosaveDlg::CLC7AutosaveDlg(ILC7Controller *ctrl, QWidget *parent) : QDialog(parent, Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	m_ctrl = ctrl;

	ui.setupUi(this);

	ui.autosavedSessionTable->clear();
	ui.autosavedSessionTable->setColumnCount(1);
	ui.autosavedSessionTable->setHorizontalHeaderLabels(QStringList() << "Auto-saved Session Files");
	ui.autosavedSessionTable->verticalHeader()->setVisible(false);
	ui.autosavedSessionTable->horizontalHeader()->setStretchLastSection(true);
	
	connect(ui.autosavedSessionTable, &QTableWidget::currentItemChanged, this, &CLC7AutosaveDlg::slot_autosavedSessionTable_currentItemChanged);
	connect(ui.autosavedSessionTable, &QTableWidget::cellDoubleClicked, this, &CLC7AutosaveDlg::slot_autosavedSessionTable_cellDoubleClicked);

	connect(ui.restoreButton, &QAbstractButton::clicked, this, &CLC7AutosaveDlg::slot_restoreButton_clicked);
	connect(ui.closeButton, &QAbstractButton::clicked, this, &CLC7AutosaveDlg::slot_closeButton_clicked);
	connect(ui.deleteButton, &QAbstractButton::clicked, this, &CLC7AutosaveDlg::slot_deleteButton_clicked);
	connect(ui.deleteAllButton, &QAbstractButton::clicked, this, &CLC7AutosaveDlg::slot_deleteAllButton_clicked);
	
	RefreshContent();
}

CLC7AutosaveDlg::~CLC7AutosaveDlg()
{
}


void CLC7AutosaveDlg::RefreshContent()
{
	QFileInfoList entryinfolist = m_ctrl->GetAutoSavedSessions();
	
	if (entryinfolist.size() == 0)
	{
		// Just punt, there's nothing to do now.
		reject();
		return;
	}

	ui.autosavedSessionTable->setRowCount(entryinfolist.size());

	int row = 0;
	foreach(QFileInfo fi, entryinfolist)
	{
		QString path = fi.absoluteFilePath();
		QString name = fi.fileName();
		QString datestr = fi.lastModified().toLocalTime().toString(Qt::DefaultLocaleShortDate);
		
		QTableWidgetItem *item = new QTableWidgetItem(QString("%1 (%2)").arg(name).arg(datestr));
		item->setData(257, path);
		ui.autosavedSessionTable->setItem(row, 0, item);
		row++;
	}

	UpdateUI();
}


void CLC7AutosaveDlg::UpdateUI()
{
	bool is_selected = false;
	if (ui.autosavedSessionTable->currentItem() != NULL)
	{
		is_selected = true;
	}
	
	ui.deleteButton->setEnabled(is_selected);
	ui.restoreButton->setEnabled(is_selected);
}

QString CLC7AutosaveDlg::autosaveSessionPath()
{
	return m_autosaveSessionPath;
}

void CLC7AutosaveDlg::slot_autosavedSessionTable_currentItemChanged(QTableWidgetItem * current, QTableWidgetItem * previous)
{
	if (!current)
	{
		m_autosaveSessionPath.clear();
	}

	m_autosaveSessionPath = current->data(257).toString();

	UpdateUI();
}

void CLC7AutosaveDlg::slot_autosavedSessionTable_cellDoubleClicked(int row, int column)
{
	QTableWidgetItem *item = ui.autosavedSessionTable->item(row, column);
	if (!item)
	{
		return;
	}

	m_autosaveSessionPath = item->data(257).toString();

	UpdateUI();
	
	accept();
}

void CLC7AutosaveDlg::slot_restoreButton_clicked(bool checked)
{
	accept();
}

void CLC7AutosaveDlg::slot_closeButton_clicked(bool checked)
{
	reject();
}

void CLC7AutosaveDlg::slot_deleteButton_clicked(bool checked)
{
	QFile::remove(m_autosaveSessionPath);
	RefreshContent();
}

void CLC7AutosaveDlg::slot_deleteAllButton_clicked(bool checked)
{
	QFileInfoList entryinfolist = m_ctrl->GetAutoSavedSessions();

	foreach(QFileInfo fi, entryinfolist)
	{
		QFile::remove(fi.absoluteFilePath());
	}

	RefreshContent();
}
