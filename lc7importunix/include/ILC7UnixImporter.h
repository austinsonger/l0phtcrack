#ifndef __INC_ILC7UNIXIMPORTER_H
#define __INC_ILC7UNIXIMPORTER_H

class ILC7UnixImporter
{
public:
	virtual ~ILC7UnixImporter() {};

	virtual QString name()=0;
	virtual QString desc()=0;
	virtual QStringList filetypes()=0;
	virtual bool CheckHashTypes(QStringList filenames, QList<bool> &filevalid, QList<FOURCC> & hashtypes)=0;
	virtual bool DoImport(QList<LC7Account> &lc7accounts, QStringList filenames, FOURCC hashtype, QString & error, bool & cancelled) = 0;

};

#endif