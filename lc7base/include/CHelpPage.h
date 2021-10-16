#ifndef __INC_CHELPPAGE_H
#define __INC_CHELPPAGE_H

#include<QtWidgets/QMainWindow>
#include<QHelpEngine>
#include<QHelpIndexWidget>
#include<QHelpContentWidget>

#include "ui_help.h"

class CHelpPage : public QWidget
{
	Q_OBJECT

public:
	CHelpPage(QWidget *parent = 0);
	virtual ~CHelpPage();

	virtual void UpdateUI(void);

public slots:
	void RecolorCallback(void);

public slots:
	void slot_uiEnable(bool enable);

	void slot_backButton_clicked(bool enable);
	void slot_forwardButton_clicked(bool enable);
	void slot_homeButton_clicked(bool enable);
	void slot_searchButton_clicked(bool enable);
	void slot_SearchEdit_textChanged(const QString &text);
	void slot_HelpWebView_titleChanged(const QString &text);
	void slot_HelpWebView_loadFinished(bool ok);
	void slot_helpButtonClicked();

	void slot_contentWidget_expandItem(const QModelIndex &index);

private:
	Ui::HelpForm ui;
	bool m_enable_ui;
	bool m_first_page;
	QString m_searchtext;
	QHelpEngine *m_helpEngine;
	QTabWidget *m_tabWidget;
	QAbstractButton *m_helpbutton;
	HelpBrowser *m_helpBrowser;
	QHelpContentWidget *m_contentWidget;
	//QHelpIndexWidget *m_indexWidget;
	QSplitter *m_horizSplitter;

	void DoSearch(QString query);

};

extern CHelpPage *CreateHelpPage();

#endif
