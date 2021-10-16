#ifndef __INC_CLC7JTRDLL_H
#define __INC_CLC7JTRDLL_H

#include"jtrdll.h"

class CLC7JTRDLL
{
private:

	QLibrary *m_jtrdll;
	QString m_jtrdllversion;
	QString m_jtrfname;
	TYPEOF_jtrdll_main *m_jtrdll_main;
	TYPEOF_jtrdll_abort *m_jtrdll_abort;
	TYPEOF_jtrdll_get_status *m_jtrdll_get_status;
	TYPEOF_jtrdll_get_charset_info *m_jtrdll_get_charset_info;
	TYPEOF_jtrdll_cleanup *m_jtrdll_cleanup;
	TYPEOF_jtrdll_preflight *m_jtrdll_preflight;
	TYPEOF_jtrdll_set_extra_opencl_kernel_args *m_jtrdll_set_extra_opencl_kernel_args;

public:
	CLC7JTRDLL(QString jtrdllversion);
	~CLC7JTRDLL();

	bool IsValid();

	int main(int argc, char **argv, struct JTRDLL_HOOKS *hooks);
	void abort(bool timeout);
	void terminate(void);
	void get_status(struct JTRDLL_STATUS *jtrdllstatus);
	int get_charset_info(const char *path, unsigned char * charmin, unsigned char *charmax, unsigned char *len, unsigned char *count, unsigned char allchars[256]);
	void preflight(int argc, char **argv, struct JTRDLL_HOOKS *hooks, struct JTRDLL_PREFLIGHT *jtrdllpreflight);
	void set_extra_opencl_kernel_args(const char *extra_opencl_kernel_args);

};

#endif
