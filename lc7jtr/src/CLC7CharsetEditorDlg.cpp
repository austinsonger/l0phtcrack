#include "stdafx.h"
#include "CLC7CharsetEditor.h"


CLC7CharsetEditorDlg::CLC7CharsetEditorDlg() : QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{TR;
	ui.setupUi(this);

	m_is_valid = false;

//	ui.presetWidgetLayout->removeWidget(ui.presetWidget);
	delete ui.presetWidget;
	ui.presetWidget = g_pLinkage->GetGUILinkage()->CreatePresetWidget(this, new CLC7CharsetEditor(), QString("%1:charsets").arg(UUID_LC7JTRPLUGIN.toString()));
	ui.presetWidgetLayout->addWidget(ui.presetWidget);

	connect(ui.closeButton, &QAbstractButton::clicked, this, &CLC7CharsetEditorDlg::accept);
	
	UpdateUI();
}

CLC7CharsetEditorDlg::~CLC7CharsetEditorDlg()
{TR;
}

void CLC7CharsetEditorDlg::UpdateUI()
{TR;
	ui.closeButton->setEnabled(m_is_valid);
}

void CLC7CharsetEditorDlg::slot_isValid(bool valid)
{TR;
	m_is_valid = valid;
	UpdateUI();
}