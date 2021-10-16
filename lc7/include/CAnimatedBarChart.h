#ifndef __INC_CAnimatedBarChart_H
#define __INC_CAnimatedBarChart_H

class CAnimatedBarChart :public QWidget
{
	Q_OBJECT

private:

	QTimer m_refresh;

	int	m_iWidth;
	int	m_iHeight;

	int m_iCurPaintBuffer;
	QImage *m_pPaintBuffers[2];
	//QImage *m_pBarsCacheBuffer;
	//QImage *m_pBarsTextCacheBuffer;
	//QImage *m_pHeaderCacheBuffer;
	QSize m_sizePaintBuffer;

	bool m_hover;
	bool m_active;

	union
	{
		quint32 m_dirty;
		struct
		{
			//quint32 m_dirty_background : 1; // background doesnt need caching
			quint32 m_dirty_bars : 1;
			quint32 m_dirty_bars_text : 1;
			quint32 m_dirty_header : 1;
		};
	};
	
	QColor m_background_color[2];
	QColor m_border_color[2];
	int m_dpi_scale;
	int m_border_width;
	int m_border_gap;
	QString m_header_text;
	Qt::Orientation m_header_text_orientation;
	Qt::Alignment m_header_alignment;
	QFont m_header_font;
	QColor m_header_text_color[2];
	QStaticText m_st_header;

	struct BARINFO
	{
		QString id;
		QStaticText m_st_bar;
		quint32 enabled_mask;

		static const quint32 ENABLED_text = 1;
		static const quint32 ENABLED_width = 2;
		static const quint32 ENABLED_gap = 4;
		static const quint32 ENABLED_value = 8;
		static const quint32 ENABLED_text_alignment = 16;
		static const quint32 ENABLED_font = 32;
		static const quint32 ENABLED_bar_color_inactive = 64;
		static const quint32 ENABLED_bar_color_active = 128;
		static const quint32 ENABLED_text_color_inactive = 256;
		static const quint32 ENABLED_text_color_active = 512;
		static const quint32 ENABLED_text_orientation = 1024;
		static const quint32 ENABLED_tiptext = 2048;

		QString text;
		QString tiptext;
		int width;
		int gap;
		double value;
		Qt::Alignment text_alignment;
		QFont font;
		QColor bar_color_inactive;
		QColor bar_color_active;
		QColor text_color_inactive;
		QColor text_color_active;
		Qt::Orientation text_orientation;

		QRect last_rect;
		bool hover;

		BARINFO()
		{
			enabled_mask = 0;
			width = -1;
			gap = -1;
			value = 0.0;
			hover = false;
			text_alignment = Qt::AlignCenter;
			text_orientation = Qt::Horizontal;
		}
	};

	Qt::Orientation m_bars_orientation;
	BARINFO m_default_bi;
	QMap<QString, int> m_bar_index;
	QList<BARINFO> m_bar_info;

	QSize m_minimumSizeHint;
	QSize m_sizeHint;

public:
	CAnimatedBarChart(QWidget *parent);
	virtual ~CAnimatedBarChart();

	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

	void reset();
	void clear();

	void setActive(bool active);
	bool active(void) const;

	void setDPIScale(int scale);
	int dpiIScale() const;

	void setBarsOrientation(Qt::Orientation orientation);
	Qt::Orientation barsOrientation() const;

	void setBorderWidth(int width);
	int borderWidth(void) const;

	void setBorderGap(int gap);
	int borderGap(void) const;

	void setBackgroundColor(QColor backgroundcolor, bool active);
	QColor backgroundColor(bool active) const;
	void setBorderColor(QColor bordercolor, bool active);
	QColor borderColor(bool active) const;

	void setHeaderText(const QString & text);
	QString headerText() const;
	void setHeaderTextColor(QColor headertextcolor, bool active);
	QColor HeaderTextColor(bool active) const;
	void setHeaderTextOrientation(Qt::Orientation orientation);
	Qt::Orientation headerTextOrientation() const;
	void setHeaderTextAlignment(Qt::Alignment align);
	Qt::Alignment headerTextAlignment() const;
	void setHeaderTextFont(QFont font);
	QFont headerTextFont() const;
	
	void addBar(QString id);
	
	void insertBar(int position, QString id);
	
	int barPosition(QString id);
	QString barId(int position);

	void removeAt(int position);
	int count() const;

	void setBarValue(QString id, double value);
	double barValue(QString id) const;
	void setBarText(QString id, QString text);
	QString barText(QString id) const;
	void setBarToolTip(QString id, QString tiptext);
	QString barToolTip(QString id) const;
	void setBarWidth(QString id, int width);
	int barWidth(QString id) const;
	void setBarGap(QString id, int gap);
	int barGap(QString id) const;
	void setBarTextFont(QString id, QFont font);
	QFont barTextFont(QString id) const;
	void setBarColor(QString id, QColor clr, bool active);
	QColor barColor(QString id, bool active) const;
	void setBarTextColor(QString id, QColor clr, bool active);
	QColor barTextColor(QString id, bool active) const;
	void setBarTextOrientation(QString id, Qt::Orientation orientation);
	Qt::Orientation barTextOrientation(QString id) const;
	void setBarTextAlignment(QString id, Qt::Alignment align);
	Qt::Alignment barTextAlignment(QString id) const;

protected:
	void recalculateSize(void);
	void initialize();
	void initPaintBuffers();
	
	void drawFrame();
	void drawHeader(QPainter & painter);
	void drawBar(QPainter & painter, int & barpos , BARINFO &barinfo);

	virtual void resizeEvent(QResizeEvent *);
	virtual void paintEvent(QPaintEvent*);
	virtual void enterEvent(QEvent *);
	virtual void leaveEvent(QEvent *);
	virtual void mouseMoveEvent(QMouseEvent *e);

	inline QImage *FrontBuffer() { return m_pPaintBuffers[m_iCurPaintBuffer]; }
	inline QImage *BackBuffer() { return m_pPaintBuffers[1 - m_iCurPaintBuffer]; }
	inline void SwapBuffers() { m_iCurPaintBuffer = 1 - m_iCurPaintBuffer; }

	void initBARINFO(BARINFO &bi);

private slots:
	void slot_refresh(void);

};


#endif