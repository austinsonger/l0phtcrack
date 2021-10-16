#ifndef __INC_CLC7COLORMANAGER_H
#define __INC_CLC7COLORMANAGER_H

class CLC7ColorManager: public QObject, public ILC7ColorManager
{
	Q_OBJECT

protected:
	void RecalculateColors();
	QPixmap HueRecolorPixmap(const QPixmap *pixmap, QColor huecolor);
	QPixmap MonoRecolorPixmap(const QPixmap *pixmap, QColor basecolor);
	void DoCommandLinkButtonStyling(QCommandLinkButton *commandlinkbutton);

	QList<QCommandLinkButton *> m_clbuttons;
	QColor m_textcolor;
	QColor m_basecolor;
	QColor m_highlightcolor;
	QString m_stylesheet;
	QMap<QString,int> m_shades;
	QMap<QString,int> m_default_shades;
	QString m_temporary_dir;
	QScreen *m_currentScreen;
	int m_size_ratio;
	bool m_bNeedsReload;

signals:
	void sig_RecolorCallback();

public slots:
	void reload();
	void slot_settingsValueChanged(QString key, const QVariant &val);
	void slot_screenChanged(QScreen *screen);
	void onCommandLinkButtonPressed();
	void onCommandLinkButtonReleased();
	void slot_destroyedCLButton(QObject *object);
	void slot_reloadSettings(void);

public:
	CLC7ColorManager();
	virtual ~CLC7ColorManager();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual void ReloadSettings(void);
	
	virtual QMap<QString,int> GetDefaultShades();
	virtual QColor GetDefaultBaseColor();
	virtual QColor GetDefaultHighlightColor();

	virtual void SetBaseColor(QColor color);
	virtual void SetHighlightColor(QColor color);
	virtual QColor GetBaseColor();
	virtual QColor GetHighlightColor();

	virtual QString GetStyleSheet(void);
	virtual QString GetTextColor();
	virtual QString GetInverseTextColor();
	virtual QString GetHighlightShade(QString shade);
	virtual QString GetBaseShade(QString shade);

	virtual QPixmap GetHueColorPixmap(QPixmap pixmap, QColor huecolor = QColor());
	virtual QPixmap GetHueColorPixmapResource(QString resource_path, QColor huecolor = QColor());
	virtual QPixmap GetMonoColorPixmap(QPixmap pixmap, QColor basecolor = QColor());
	virtual QPixmap GetMonoColorPixmapResource(QString resource_path, QColor basecolor = QColor());

	virtual QIcon GetMonoColorIcon(QString resource, QColor normal = QColor(), QColor active = QColor(), QColor disabled = QColor());

	virtual QGraphicsEffect *CreateShadowEffect(void);
	virtual void StyleCommandLinkButton(QCommandLinkButton *commandlinkbutton);

	virtual int GetSizeRatio(void);

	virtual void RegisterRecolorCallback(QObject *slot_obj, void (QObject::*slot_method)(void));
};

#endif