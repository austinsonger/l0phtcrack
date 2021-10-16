#ifndef __INC_CLC7CHARSETEDITORDLG_H
#define __INC_CLC7CHARSETEDITORDLG_H

#include <QWidget>
#include"ui_charseteditordlg.h"
#include"CLC7CharsetEditor.h"

class CLC7CharsetEditorDlg : public QDialog
{
	Q_OBJECT;

public:

	CLC7CharsetEditorDlg();
	~CLC7CharsetEditorDlg();

private:
	Ui::charsetEditorDialog ui;

	CLC7CharsetEditor *m_editor;
	bool m_is_valid;

	void UpdateUI();
	
public slots:
	void slot_isValid(bool valid);

};

#endif
