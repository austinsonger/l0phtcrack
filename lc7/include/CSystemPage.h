#ifndef __INC_CSYSTEMPAGE_H
#define __INC_CSYSTEMPAGE_H

#include <QtWidgets/QMainWindow>
#include "ui_system.h"

#include "qtpropertymanager.h"
#include "qteditorfactory.h"
#include "qttreepropertybrowser.h"
#include "qtvariantproperty.h"
#include "qtbuttonpropertybrowser.h"
#include "qtgroupboxpropertybrowser.h"

class ILC7Controller;

class CSystemPage : public QWidget
{
	Q_OBJECT

public:
	CSystemPage(QWidget *parent, ILC7Linkage *pLinkage, ILC7Controller *ctrl);
	virtual ~CSystemPage();

	virtual void NotifySessionActivity(ILC7Linkage::SESSION_ACTIVITY activity, ILC7SessionHandler *handler);

	void showEvent(QShowEvent *evt);
	void hideEvent(QHideEvent *evt);
	void RefreshContent();
	void UpdateUI();

private:
	ILC7Linkage *m_pLinkage;
	ILC7Controller *m_ctrl;

	Ui::SystemForm ui;
	bool m_enable_ui;
	bool m_bRefreshingContent;

	ILC7WorkQueue *m_batch_workqueue;
	ILC7WorkQueue *m_single_workqueue;

	QtVariantPropertyManager *m_varManager;
    QtVariantEditorFactory *m_varFactory;
	QtTreePropertyBrowser *m_propBrowser;
	QMap<QtVariantProperty *, QString> m_settingskeys;
	QMap<QtVariantProperty *, bool> m_require_restart;

	QList<ILC7PluginLibrary *> m_selected_plugins;

	QAbstractButton *m_helpbutton;

	void RefreshAbout();
	void RefreshSettings(bool reset_defaults=false);
	void RefreshPlugins();

	void ResetPlugins();

public slots:
	void slot_uiEnable(bool enable);
	void slot_onSystemPageCheckbox(int state);

	void slot_helpButtonClicked();

	void slot_onInstallPlugin();
	void slot_onUninstallPlugins();
	void slot_onEnablePlugins();
	void slot_onDisablePlugins();

	void slot_clearButton_clicked(int checked);
	void slot_resetButton_clicked(int checked);
	void slot_resetSavedButton_clicked(int checked);

public slots:
	void PropertyValueChanged(QtProperty *property, const QVariant &value);
	void RecolorCallback(void);
};

#endif
