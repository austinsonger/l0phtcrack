#include<stdafx.h>

 CLC7ModifiedListWidget::CLC7ModifiedListWidget(QWidget *parent):QListWidget(parent)
{

}

void CLC7ModifiedListWidget::leaveEvent(QEvent *evt)
{
	emit sig_leave();
}

void CLC7ModifiedListWidget::keyPressEvent(QKeyEvent *evt)
{
	if (evt->key() == Qt::Key_Enter || evt->key() == Qt::Key_Return)
	{
		emit sig_edit(false);
		return;
	}
	if (evt->key() == Qt::Key_Delete || evt->key() == Qt::Key_Backspace)
	{
		emit sig_delete(false);
		return;
	}
	if (evt->key() == Qt::Key_Insert || (evt->modifiers() == Qt::ControlModifier && evt->key() == Qt::Key_N))
	{
		emit sig_new(false);
		return;
	}
	if (evt->key() == Qt::Key_Copy || (evt->modifiers() == Qt::ControlModifier && evt->key() == Qt::Key_C))
	{
		emit sig_copy(false);
		return;
	}
	if (evt->modifiers() == Qt::ControlModifier && evt->key() == Qt::Key_Up)
	{
		emit sig_moveup(false);
		return;
	}
	if (evt->modifiers() == Qt::ControlModifier && evt->key() == Qt::Key_Down)
	{
		emit sig_movedown(false);
		return;
	}

	return QListWidget::keyPressEvent(evt);
}

void CLC7ModifiedListWidget::dropEvent(QDropEvent *evt)
{
	QListWidget::dropEvent(evt);
	emit sig_dropped();
}
