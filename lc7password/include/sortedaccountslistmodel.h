#ifndef __INC_SORTEDACCOUNTSLISTMODEL_H
#define __INC_SORTEDACCOUNTSLISTMODEL_H

class SortedAccountsListModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:

	SortedAccountsListModel(QObject *parent);

protected:
	
	virtual bool lessThan(const QModelIndex & left, const QModelIndex & right) const;

signals:
	void askOrder(int column, Qt::SortOrder order);

public slots:
	void slot_enable_sorting(bool enable);

	void sort(int column, Qt::SortOrder order);
	void doSort(int column, Qt::SortOrder order);
};

 
#endif