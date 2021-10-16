#ifndef __INC_CLC7ACTIONCATEGORY_H
#define __INC_CLC7ACTIONCATEGORY_H

class CLC7Action;

class CLC7ActionCategory:public QObject, public ILC7ActionCategory
{
	Q_OBJECT;

private:
	friend class CLC7Action;
	friend class CLC7Controller;

	QString m_internal_name;
	QString m_name;
	QString m_desc;
	QIcon m_icon;

	QMap<QString, CLC7ActionCategory *> m_action_categories;
	QList<CLC7Action *> m_actions;
	
	int m_refcount;
	quint32 m_gen;
	CLC7ActionCategory *m_category;

protected:

	virtual int Ref();
	virtual int Unref();
	virtual void RemoveChildCategory(CLC7ActionCategory *child);
	virtual void RemoveChildAction(CLC7Action *child);

	static QList<CLC7ActionCategory *> s_dead_categories;
	static void GarbageCollect(void);

	CLC7ActionCategory();

public:
	CLC7ActionCategory(CLC7ActionCategory *category, QString internal_name, QString name, QString desc, QIcon icon=QIcon());
	virtual ~CLC7ActionCategory();
	virtual ILC7Interface *GetInterfaceVersion(QString interface_name);

	virtual ILC7ActionCategory *ParentCategory();
	virtual QString InternalName();
	virtual QString Name();
	virtual QString Desc();
	virtual QIcon Icon();

	virtual quint32 GetGenerationNumber();
		
	virtual ILC7ActionCategory *CreateActionCategory(QString internal_name, QString name, QString desc, QIcon icon=QIcon());
	virtual void RemoveActionCategory(ILC7ActionCategory *cat);
	virtual QList<ILC7ActionCategory *> GetActionCategories();

	virtual ILC7Action *CreateAction(QUuid componentid, QString command, QStringList args, QString name, QString desc, QIcon icon = QIcon());
	virtual void RemoveAction(ILC7Action *act);
	virtual QList<ILC7Action *> GetActions();
};

#endif