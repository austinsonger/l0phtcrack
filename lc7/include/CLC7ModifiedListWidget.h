#ifndef __INC_CLC7MODIFIEDLISTWIDGET_H
#define __INC_CLC7MODIFIEDLISTWIDGET_H



class CLC7ModifiedListWidget : public QListWidget
{
	Q_OBJECT
signals:
	void sig_leave();
	void sig_dropped();
	void sig_edit(bool);
	void sig_delete(bool);
	void sig_new(bool);
	void sig_copy(bool);
	void sig_moveup(bool);
	void sig_movedown(bool);
	
public:
	CLC7ModifiedListWidget(QWidget *parent);
	virtual void leaveEvent(QEvent *evt);
	virtual void keyPressEvent(QKeyEvent *evt);
	virtual void dropEvent(QDropEvent *evt);
};

#endif