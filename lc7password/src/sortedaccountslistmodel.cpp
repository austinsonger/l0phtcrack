#include"stdafx.h"

SortedAccountsListModel::SortedAccountsListModel(QObject *parent) : QSortFilterProxyModel(parent)
{
}

bool SortedAccountsListModel::lessThan(const QModelIndex & left, const QModelIndex & right) const
{
	AccountsListModel *model = ((AccountsListModel *)sourceModel());	
	return model->lessThan(left, right);
}

void SortedAccountsListModel::slot_enable_sorting(bool enable)
{
	setDynamicSortFilter(enable);
}


void SortedAccountsListModel::sort(int column, Qt::SortOrder order)
{
	emit askOrder(column, order);
}

void SortedAccountsListModel::doSort(int column, Qt::SortOrder order)
{
	QSortFilterProxyModel::sort(column, order);
}
