#include<stdafx.h>

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
#include"nvapi.h"

#define STATUS_SUCCESS 0
typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

#define _WIN32_DCOM
#include <iostream>
#include <comdef.h>
#include <comutil.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

static QPixmap IPictureToQPixmap(IPicture *ipic)
{
	// xxx stub
	return QPixmap();
}

static QFont IFontToQFont(IFont *f)
{
	BSTR name;
	BOOL bold;
	SHORT charset;
	BOOL italic;
	CY size;
	BOOL strike;
	BOOL underline;
	SHORT weight;
	f->get_Name(&name);
	f->get_Bold(&bold);
	f->get_Charset(&charset);
	f->get_Italic(&italic);
	f->get_Size(&size);
	f->get_Strikethrough(&strike);
	f->get_Underline(&underline);
	f->get_Weight(&weight);
	QFont font(QString::fromWCharArray(name), size.Lo / 9750, weight / 97, italic);
	font.setBold(bold);
	font.setStrikeOut(strike);
	font.setUnderline(underline);
	SysFreeString(name);

	return font;
}

QDateTime DATEToQDateTime(DATE ole)
{
	SYSTEMTIME stime;
	if (ole >= 949998 || VariantTimeToSystemTime(ole, &stime) == false)
		return QDateTime();

	QDate date(stime.wYear, stime.wMonth, stime.wDay);
	QTime time(stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds);
	return QDateTime(date, time);
}

QColor OLEColorToQColor(uint col)
{
	// xxx stub
	return QColor(0, 0, 0);
}


void clearVARIANT(VARIANT *var)
{
	if (var->vt & VT_BYREF) {
		switch (var->vt) {
		case VT_BSTR | VT_BYREF:
			SysFreeString(*var->pbstrVal);
			delete var->pbstrVal;
			break;
		case VT_BOOL | VT_BYREF:
			delete var->pboolVal;
			break;
		case VT_I1 | VT_BYREF:
			delete var->pcVal;
			break;
		case VT_I2 | VT_BYREF:
			delete var->piVal;
			break;
		case VT_I4 | VT_BYREF:
			delete var->plVal;
			break;
		case VT_INT | VT_BYREF:
			delete var->pintVal;
			break;
		case VT_UI1 | VT_BYREF:
			delete var->pbVal;
			break;
		case VT_UI2 | VT_BYREF:
			delete var->puiVal;
			break;
		case VT_UI4 | VT_BYREF:
			delete var->pulVal;
			break;
		case VT_UINT | VT_BYREF:
			delete var->puintVal;
			break;
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
		case VT_I8 | VT_BYREF:
			delete var->pllVal;
			break;
		case VT_UI8 | VT_BYREF:
			delete var->pullVal;
			break;
#endif
		case VT_CY | VT_BYREF:
			delete var->pcyVal;
			break;
		case VT_R4 | VT_BYREF:
			delete var->pfltVal;
			break;
		case VT_R8 | VT_BYREF:
			delete var->pdblVal;
			break;
		case VT_DATE | VT_BYREF:
			delete var->pdate;
			break;
		case VT_DISPATCH | VT_BYREF:
			(*var->ppdispVal)->Release();
			delete var->ppdispVal;
			break;
		case VT_ARRAY | VT_VARIANT | VT_BYREF:
		case VT_ARRAY | VT_UI1 | VT_BYREF:
		case VT_ARRAY | VT_BSTR | VT_BYREF:
			SafeArrayDestroy(*var->pparray);
			delete var->pparray;
			break;
		case VT_VARIANT | VT_BYREF:
			delete var->pvarVal;
			break;
		}
		VariantInit(var);
	}
	else {
		VariantClear(var);
	}
}

QVariant VARIANTToQVariant(const VARIANT &arg, const QByteArray &typeName, uint type=0)
{
	QVariant var;
	switch (arg.vt) {
	case VT_BSTR:
		var = QString::fromWCharArray(arg.bstrVal);
		break;
	case VT_BSTR | VT_BYREF:
		var = QString::fromWCharArray(*arg.pbstrVal);
		break;
	case VT_BOOL:
		var = QVariant((bool)arg.boolVal);
		break;
	case VT_BOOL | VT_BYREF:
		var = QVariant((bool)*arg.pboolVal);
		break;
	case VT_I1:
		var = arg.cVal;
		if (typeName == "char")
			type = QVariant::Int;
		break;
	case VT_I1 | VT_BYREF:
		var = *arg.pcVal;
		if (typeName == "char")
			type = QVariant::Int;
		break;
	case VT_I2:
		var = arg.iVal;
		if (typeName == "short")
			type = QVariant::Int;
		break;
	case VT_I2 | VT_BYREF:
		var = *arg.piVal;
		if (typeName == "short")
			type = QVariant::Int;
		break;
	case VT_I4:
		if (type == QVariant::Color || (!type && typeName == "QColor"))
			var = QVariant::fromValue(OLEColorToQColor(arg.lVal));
#ifndef QT_NO_CURSOR
		else if (type == QVariant::Cursor || (!type && (typeName == "QCursor" || typeName == "QCursor*")))
			var = QVariant::fromValue(QCursor(static_cast<Qt::CursorShape>(arg.lVal)));
#endif
		else
			var = (int)arg.lVal;
		break;
	case VT_I4 | VT_BYREF:
		if (type == QVariant::Color || (!type && typeName == "QColor"))
			var = QVariant::fromValue(OLEColorToQColor((int)*arg.plVal));
#ifndef QT_NO_CURSOR
		else if (type == QVariant::Cursor || (!type && (typeName == "QCursor" || typeName == "QCursor*")))
			var = QVariant::fromValue(QCursor(static_cast<Qt::CursorShape>(*arg.plVal)));
#endif
		else
			var = (int)*arg.plVal;
		break;
	case VT_INT:
		var = arg.intVal;
		break;
	case VT_INT | VT_BYREF:
		var = *arg.pintVal;
		break;
	case VT_UI1:
		var = arg.bVal;
		break;
	case VT_UI1 | VT_BYREF:
		var = *arg.pbVal;
		break;
	case VT_UI2:
		var = arg.uiVal;
		break;
	case VT_UI2 | VT_BYREF:
		var = *arg.puiVal;
		break;
	case VT_UI4:
		if (type == QVariant::Color || (!type && typeName == "QColor"))
			var = QVariant::fromValue(OLEColorToQColor(arg.ulVal));
#ifndef QT_NO_CURSOR
		else if (type == QVariant::Cursor || (!type && (typeName == "QCursor" || typeName == "QCursor*")))
			var = QVariant::fromValue(QCursor(static_cast<Qt::CursorShape>(arg.ulVal)));
#endif
		else
			var = (int)arg.ulVal;
		break;
	case VT_UI4 | VT_BYREF:
		if (type == QVariant::Color || (!type && typeName == "QColor"))
			var = QVariant::fromValue(OLEColorToQColor((uint)*arg.pulVal));
#ifndef QT_NO_CURSOR
		else if (type == QVariant::Cursor || (!type && (typeName == "QCursor" || typeName == "QCursor*")))
			var = QVariant::fromValue(QCursor(static_cast<Qt::CursorShape>(*arg.pulVal)));
#endif
		else
			var = (int)*arg.pulVal;
		break;
	case VT_UINT:
		var = arg.uintVal;
		break;
	case VT_UINT | VT_BYREF:
		var = *arg.puintVal;
		break;
	case VT_CY:
		var = arg.cyVal.int64;
		break;
	case VT_CY | VT_BYREF:
		var = arg.pcyVal->int64;
		break;
#if !defined(Q_OS_WINCE) && defined(_MSC_VER) && _MSC_VER >= 1400
	case VT_I8:
		var = arg.llVal;
		break;
	case VT_I8 | VT_BYREF:
		var = *arg.pllVal;
		break;
	case VT_UI8:
		var = arg.ullVal;
		break;
	case VT_UI8 | VT_BYREF:
		var = *arg.pullVal;
		break;
#endif
	case VT_R4:
		var = arg.fltVal;
		break;
	case VT_R4 | VT_BYREF:
		var = *arg.pfltVal;
		break;
	case VT_R8:
		var = arg.dblVal;
		break;
	case VT_R8 | VT_BYREF:
		var = *arg.pdblVal;
		break;
	case VT_DATE:
		var = DATEToQDateTime(arg.date);
		if (type == QVariant::Date || (!type && (typeName == "QDate" || typeName == "QDate*"))) {
			var.convert(QVariant::Date);
		}
		else if (type == QVariant::Time || (!type && (typeName == "QTime" || typeName == "QTime*"))) {
			var.convert(QVariant::Time);
		}
		break;
	case VT_DATE | VT_BYREF:
		var = DATEToQDateTime(*arg.pdate);
		if (type == QVariant::Date || (!type && (typeName == "QDate" || typeName == "QDate*"))) {
			var.convert(QVariant::Date);
		}
		else if (type == QVariant::Time || (!type && (typeName == "QTime" || typeName == "QTime*"))) {
			var.convert(QVariant::Time);
		}
		break;
	case VT_VARIANT:
	case VT_VARIANT | VT_BYREF:
		if (arg.pvarVal)
			var = VARIANTToQVariant(*arg.pvarVal, typeName);
		break;

	case VT_DISPATCH:
	case VT_DISPATCH | VT_BYREF:
		break;
	case VT_UNKNOWN:
	case VT_UNKNOWN | VT_BYREF:
	{
		IUnknown *unkn = 0;
		if (arg.vt & VT_BYREF)
			unkn = *arg.ppunkVal;
		else
			unkn = arg.punkVal;
		
		var=(size_t)unkn;
	}
	break;
	case VT_ARRAY | VT_VARIANT:
	case VT_ARRAY | VT_VARIANT | VT_BYREF:
	{
		SAFEARRAY *array = 0;
		if (arg.vt & VT_BYREF)
			array = *arg.pparray;
		else
			array = arg.parray;

		UINT cDims = array ? SafeArrayGetDim(array) : 0;
		switch (cDims) {
		case 1:
		{
			QVariantList list;

			long lBound, uBound;
			SafeArrayGetLBound(array, 1, &lBound);
			SafeArrayGetUBound(array, 1, &uBound);

			for (long i = lBound; i <= uBound; ++i) {
				VARIANT var;
				VariantInit(&var);
				SafeArrayGetElement(array, &i, &var);

				QVariant qvar = VARIANTToQVariant(var, 0);
				clearVARIANT(&var);
				list << qvar;
			}

			var = list;
		}
		break;

		case 2:
		{
			QVariantList listList; //  a list of lists
			long dimIndices[2];

			long xlBound, xuBound, ylBound, yuBound;
			SafeArrayGetLBound(array, 1, &xlBound);
			SafeArrayGetUBound(array, 1, &xuBound);
			SafeArrayGetLBound(array, 2, &ylBound);
			SafeArrayGetUBound(array, 2, &yuBound);

			for (long x = xlBound; x <= xuBound; ++x) {
				QVariantList list;

				dimIndices[0] = x;
				for (long y = ylBound; y <= yuBound; ++y) {
					VARIANT var;
					VariantInit(&var);
					dimIndices[1] = y;
					SafeArrayGetElement(array, dimIndices, &var);

					QVariant qvar = VARIANTToQVariant(var, 0);
					clearVARIANT(&var);
					list << qvar;
				}

				listList << QVariant(list);
			}
			var = listList;
		}
		break;
		default:
			var = QVariantList();
			break;
		}
	}
	break;

	case VT_ARRAY | VT_BSTR:
	case VT_ARRAY | VT_BSTR | VT_BYREF:
	{
		SAFEARRAY *array = 0;
		if (arg.vt & VT_BYREF)
			array = *arg.pparray;
		else
			array = arg.parray;

		QStringList strings;
		if (!array || array->cDims != 1) {
			var = strings;
			break;
		}

		long lBound, uBound;
		SafeArrayGetLBound(array, 1, &lBound);
		SafeArrayGetUBound(array, 1, &uBound);

		for (long i = lBound; i <= uBound; ++i) {
			BSTR bstr;
			SafeArrayGetElement(array, &i, &bstr);
			strings << QString::fromWCharArray(bstr);
			SysFreeString(bstr);
		}

		var = strings;
	}
	break;

	case VT_ARRAY | VT_UI1:
	case VT_ARRAY | VT_UI1 | VT_BYREF:
	{
		SAFEARRAY *array = 0;
		if (arg.vt & VT_BYREF)
			array = *arg.pparray;
		else
			array = arg.parray;

		QByteArray bytes;
		if (!array || array->cDims != 1) {
			var = bytes;
			break;
		}

		long lBound, uBound;
		SafeArrayGetLBound(array, 1, &lBound);
		SafeArrayGetUBound(array, 1, &uBound);

		if (uBound != -1) { // non-empty array
			bytes.resize(uBound - lBound + 1);
			char *data = bytes.data();
			char *src;
			SafeArrayAccessData(array, (void**)&src);
			memcpy(data, src, bytes.size());
			SafeArrayUnaccessData(array);
		}

		var = bytes;
	}
	break;
	default:
#if !defined(Q_OS_WINCE)
		// support for any SAFEARRAY(Type) where Type can be converted to a QVariant 
		// -> QVariantList
		if (arg.vt & VT_ARRAY) {
			SAFEARRAY *array = 0;
			if (arg.vt & VT_BYREF)
				array = *arg.pparray;
			else
				array = arg.parray;

			QVariantList list;
			if (!array || array->cDims != 1) {
				var = list;
				break;
			}

			// find out where to store the element
			VARTYPE vt;
			VARIANT variant;
			SafeArrayGetVartype(array, &vt);

			void *pElement = 0;
			switch (vt) {
			case VT_BSTR: Q_ASSERT(false); break; // already covered
			case VT_BOOL: pElement = &variant.boolVal; break;
			case VT_I1: pElement = &variant.cVal; break;
			case VT_I2: pElement = &variant.iVal; break;
			case VT_I4: pElement = &variant.lVal; break;
#if defined(_MSC_VER) && _MSC_VER >= 1400
			case VT_I8: pElement = &variant.llVal; break;
			case VT_UI8: pElement = &variant.ullVal; break;
#endif
			case VT_INT: pElement = &variant.intVal; break;
			case VT_UI1: Q_ASSERT(false); break; // already covered
			case VT_UI2: pElement = &variant.uiVal; break;
			case VT_UI4: pElement = &variant.ulVal; break;
			case VT_UINT: pElement = &variant.uintVal; break;
			case VT_CY: pElement = &variant.cyVal; break;
			case VT_R4: pElement = &variant.fltVal; break;
			case VT_R8: pElement = &variant.dblVal; break;
			case VT_DATE: pElement = &variant.date; break;
			case VT_VARIANT: Q_ASSERT(false); break; // already covered
			default:
				break;
			}
			if (!pElement) {
				var = list;
				break;
			}

			long lBound, uBound;
			SafeArrayGetLBound(array, 1, &lBound);
			SafeArrayGetUBound(array, 1, &uBound);

			for (long i = lBound; i <= uBound; ++i) {
				variant.vt = vt;
				SafeArrayGetElement(array, &i, pElement);
				QVariant qvar = VARIANTToQVariant(variant, 0);
				clearVARIANT(&variant);
				list << qvar;
			}

			var = list;
		}
#endif
		break;
	}

	QVariant::Type proptype = (QVariant::Type)type;
	if (proptype == QVariant::Invalid && !typeName.isEmpty()) {
		if (typeName != "QVariant")
			proptype = QVariant::nameToType(typeName);
	}
	if (proptype != QVariant::LastType && proptype != QVariant::Invalid && var.type() != proptype) {
		if (var.canConvert(proptype)) {
			QVariant oldvar = var;
			if (oldvar.convert(proptype))
				var = oldvar;
		}
		else if (proptype == QVariant::StringList && var.type() == QVariant::List) {
			bool allStrings = true;
			QStringList strings;
			const QList<QVariant> list(var.toList());
			for (QList<QVariant>::ConstIterator it(list.begin()); it != list.end(); ++it) {
				QVariant variant = *it;
				if (variant.canConvert(QVariant::String))
					strings << variant.toString();
				else
					allStrings = false;
			}
			if (allStrings)
				var = strings;
		}
		else {
			var = QVariant();
		}
	}
	return var;
}

inline BSTR QStringToBSTR(const QString &str)
{
	return SysAllocStringLen((OLECHAR*)str.unicode(), str.length());
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CLC7SystemMonitor::CLC7SystemMonitor(CLC7Controller *ctrl) :m_wmithread(this)
{
	m_ctrl = ctrl;
	m_error=false;
	
#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
	m_bInitWMI = false;
	m_bInitWMIFailed = false;
	m_bInitPDH = false;
	m_bInitADL = false;
	m_bInitNVAPI = false;
		
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	m_dwNumberOfProcessors = si.dwNumberOfProcessors;

	InitializeWMI();
	if (!m_bInitWMI)
	{
		InitializePDH();
	}
	InitializeADL();
	InitializeNVAPI();

#endif
}

CLC7SystemMonitor::~CLC7SystemMonitor()
{
#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
	if (m_bInitWMI)
	{
		TerminateWMI();
	}
	if (m_bInitPDH)
	{
		TerminatePDH();
	}
	if (m_bInitADL)
	{
		TerminateADL();
	}
	if (m_bInitNVAPI)
	{
		TerminateNVAPI();
	}
#endif
}

ILC7Interface *CLC7SystemMonitor::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7SystemMonitor")
	{
		return this;
	}
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CPU/WMI/WIN32
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64

CWMIThread::CWMIThread(CLC7SystemMonitor *monitor)
{
	m_monitor = monitor;
}

void CWMIThread::slot_wmiQuery(QString query, QList<QMap<QString, QVariant>> *results, QString *error, bool *ret)
{
	*ret= m_monitor->WMIQuery(query, *results, *error);
}

void CLC7SystemMonitor::slot_wmiThread_started()
{
	CoInitialize(NULL);

	HRESULT hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator, (LPVOID *)&m_pLoc);

	if (FAILED(hres))
	{
		m_bInitWMIFailed = true;
		return;
	}

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = m_pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
		NULL,                    // User name. NULL = current user
		NULL,                    // User password. NULL = current
		0,                       // Locale. NULL indicates current
		NULL,                    // Security flags.
		0,                       // Authority (for example, Kerberos)
		0,                       // Context object 
		&m_pSvc                  // pointer to IWbemServices proxy
		);

	if (FAILED(hres))
	{
		m_pLoc->Release();
		m_pLoc = NULL;
		m_bInitWMIFailed = true;
		return;
	}

	hres = CoSetProxyBlanket(
		m_pSvc,                        // Indicates the proxy to set
		RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		NULL,                        // Server principal name 
		RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		NULL,                        // client identity
		EOAC_NONE                    // proxy capabilities 
		);

	if (FAILED(hres))
	{
		m_pSvc->Release();
		m_pSvc = NULL;
		m_pLoc->Release();
		m_pLoc = NULL;
		m_bInitWMIFailed = true;
		return;
	}

	m_bInitWMI = true;
}

void CLC7SystemMonitor::slot_wmiThread_finished()
{
	if (!m_bInitWMI)
	{
		Q_ASSERT(0);
		return;
	}
	m_pSvc->Release();
	m_pLoc->Release();
	CoUninitialize();
	m_bInitWMI = false;
}

void CLC7SystemMonitor::InitializePDH(void)
{
	// Get names of PDH counters
	if (!GetLocalPerformanceCounterName(238, m_processor) ||
		!GetLocalPerformanceCounterName(6, m_pct_processor_time))
	{
		return;
	}

	// Get physical cpu count
	DWORD i;
	m_last_utilizations.resize(m_dwNumberOfProcessors);
	for (i = 0; i < m_dwNumberOfProcessors; i++)
	{
		QString queryname = QString("\\%1(%2)\\%3").arg(m_processor).arg(i).arg(m_pct_processor_time);
		wchar_t *querywstr = (wchar_t *)malloc((queryname.size() + 1)*sizeof(wchar_t));
		int len = queryname.toWCharArray(querywstr);
		querywstr[len] = L'\0';

		m_processer_counter_names.append(querywstr);
		m_last_utilizations[i] = 0;
	}

	// Get PDH Query
	PDH_STATUS ret = PdhOpenQueryW(NULL, NULL, &m_pdhquery);
	if (ret != ERROR_SUCCESS)
	{
		return;
	}

	// For each CPU add a counter
	foreach(wchar_t *countername, m_processer_counter_names)
	{
		PDH_HCOUNTER pdhcounter;
		ret = PdhAddCounterW(m_pdhquery, countername, 0, &pdhcounter);
		if (ret != ERROR_SUCCESS)
		{
			return;
		}
		m_pdhcounters.append(pdhcounter);
	}
	

	m_bInitPDH = true;
}

void CLC7SystemMonitor::TerminatePDH()
{
	if (m_bInitPDH)
	{
		foreach(wchar_t *name, m_processer_counter_names)
		{
			free(name);
		}
		m_processer_counter_names.clear();
		m_pdhcounters.clear();
		if (m_pdhquery)
		{
			PdhCloseQuery(m_pdhquery);
		}

		m_bInitPDH = false;
	}
}

void CLC7SystemMonitor::InitializeWMI(void)
{

	// Start wmi
	connect(&m_wmithread, &QThread::started, this, &CLC7SystemMonitor::slot_wmiThread_started, Qt::DirectConnection);
	connect(&m_wmithread, &QThread::finished, this, &CLC7SystemMonitor::slot_wmiThread_finished, Qt::DirectConnection);
	connect(this, &CLC7SystemMonitor::sig_wmiQuery, &m_wmithread, &CWMIThread::slot_wmiQuery, Qt::BlockingQueuedConnection);
	m_wmithread.moveToThread(&m_wmithread);

	m_wmithread.start();
	while (!m_bInitWMI && !m_bInitWMIFailed)
	{
		QThread::msleep(100);
	}
}

bool CLC7SystemMonitor::WMIQuery(QString query, QList<QMap<QString, QVariant>> & results, QString & error)
{
	if (!m_bInitWMI)
	{
		return false;
	}

	if (QThread::currentThread() != &m_wmithread)
	{
		bool ret;
		emit sig_wmiQuery(query, &results, &error, &ret);
		return ret;
	}
	
	IEnumWbemClassObject* pEnumerator = NULL;
	HRESULT hres = m_pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t(QStringToBSTR(query)),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator);

	if (FAILED(hres))
	{
		return false;
	}

	IWbemClassObject *pclsObj = NULL;
	ULONG uReturn = 0;

	forever
	{
		HRESULT hr = pEnumerator->Next(1000, 1,
			&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		QMap<QString, QVariant> result;

		if (pclsObj->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY) == WBEM_S_NO_ERROR)
		{
			BSTR name;
			VARIANT value;

			while (pclsObj->Next(0, &name, &value, 0, 0) == WBEM_S_NO_ERROR)
			{
				//VariantChangeType(&value, &value, 0, VT_BSTR);
				
				QVariant v = VARIANTToQVariant(value, "", 0);
				QString n = QString::fromWCharArray(name);

				result[n] = v;

				SysFreeString(name);
				clearVARIANT(&value);
			}
		}

		results.append(result);
		
		pclsObj->Release();
	}

	pEnumerator->Release();
	return true;
}

void CLC7SystemMonitor::TerminateWMI(void)
{
	if (m_bInitWMI)
	{
		m_wmithread.requestInterruption();
		m_wmithread.quit();
		m_wmithread.wait();
	}
}


bool CLC7SystemMonitor::GetLocalPerformanceCounterName(quint32 index, QString & name)
{
	wchar_t *buf = (wchar_t *)malloc(256 * sizeof(wchar_t));
	if (!buf)
	{
		return false;
	}
	DWORD bufsize = 256;
	PDH_STATUS err;
	while ((err = PdhLookupPerfNameByIndexW(0, index, buf, &bufsize)) == PDH_MORE_DATA)
	{
		bufsize += 256;
		wchar_t *new_buf = (wchar_t *)realloc(buf, bufsize * sizeof(wchar_t));
		if (!new_buf)
		{
			free(buf);
			return false;
		}
		buf = new_buf;
	}
	if (err != ERROR_SUCCESS)
	{
		free(buf);
		return false;
	}
	name = QString::fromWCharArray(buf);
	free(buf);
	return true;
}

#endif

bool CLC7SystemMonitor::GetAllCPUStatus(QList<ILC7SystemMonitor::CPU_STATUS> & status_per_core, QString &error)
{
	QMutexLocker lock(&m_mutex);

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
	
	DWORD i;

	// Create list
	CPU_STATUS empty;
	empty.current_mhz = 0;
	empty.max_mhz = 0;
	empty.mhz_limit = 0;
	empty.utilization = 0;

	status_per_core.clear();
	for (i = 0; i < m_dwNumberOfProcessors; i++)
	{
		status_per_core.append(empty);
	}

	// Get processor power information
	ULONG ppisize = sizeof(PROCESSOR_POWER_INFORMATION) * m_dwNumberOfProcessors;
	PROCESSOR_POWER_INFORMATION *ppi = (PROCESSOR_POWER_INFORMATION*)malloc(ppisize);

	if (CallNtPowerInformation(ProcessorInformation, NULL, 0, ppi, ppisize) != STATUS_SUCCESS)
	{
		free(ppi);
		error = "Unable to get processor information";
		return false;
	}

	for (i = 0; i < m_dwNumberOfProcessors; i++)
	{
		status_per_core[i].current_mhz = ppi[i].CurrentMhz;
		status_per_core[i].max_mhz = ppi[i].MaxMhz;
		status_per_core[i].mhz_limit = ppi[i].MhzLimit;
	}
	free(ppi);

	// Query WMI for processor utilization
	if (m_bInitWMI)
	{
		QList<QMap<QString, QVariant>> results;
		if (!WMIQuery("SELECT * FROM Win32_PerfFormattedData_PerfOS_Processor", results, error))
		{
			error = "WMI query failed";
			return false;
		}

		for (int r = 0; r < results.size(); r++)
		{
			bool ok = true;
			quint32 procnum = results[r]["Name"].toString().toUInt(&ok);
			if (!ok)
			{
				continue;
			}

			ok = true;
			quint32 util = 100 - (quint32)results[r]["PercentIdleTime"].toULongLong(&ok);
			if (!ok)
			{
				continue;
			}

			status_per_core[procnum].utilization = util;
		}
	}
	else if (m_bInitPDH)
	{
		PDH_STATUS ret;
		ret = PdhCollectQueryData(m_pdhquery);
		if (ret != ERROR_SUCCESS)
		{
			error = "Unable to query data";
			return false;
		}

		i = 0;
		foreach(PDH_HCOUNTER pdhcounter, m_pdhcounters)
		{
			DWORD type;
			PDH_FMT_COUNTERVALUE value;

			ret = PdhGetFormattedCounterValue(pdhcounter, PDH_FMT_DOUBLE, &type, &value);

			if (ret == ERROR_SUCCESS)
			{
				m_last_utilizations[i] = (int)value.doubleValue;
			}

			status_per_core[i].utilization = m_last_utilizations[i];
			i++;
		}

		return true;
	}
	else
	{
		error = "No CPU data available";
		return false;
	}

	
	
	return true;
#else
#error implement me
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ADL
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64 || PLATFORM==PLATFORM_LINUX

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64 
#define ADL_STDCALL __stdcall
#else
#define ADL_STDCALL
#endif

// Memory allocation function
void* ADL_STDCALL ADL_Main_Memory_Alloc(int iSize)
{
    void* lpBuffer = malloc ( iSize );
    return lpBuffer;
}

// Optional Memory de-allocation function
void ADL_STDCALL ADL_Main_Memory_Free(void** lpBuffer)
{
    if ( NULL != *lpBuffer )
    {
        free ( *lpBuffer );
        *lpBuffer = NULL;
    }
}

bool CLC7SystemMonitor::GetErrorMessage(QString & title, QString & message)
{
	title = m_error_title;
	message = m_error_message; 
	return m_error;
}

void CLC7SystemMonitor::InitializeADL()
{
#if PLATFORM==PLATFORM_LINUX
	m_adl = new QLibrary("libatiadlxx.so");
	if (!m_adl->load())
	{
		delete m_adl;
		m_adl = NULL;
		return;
	}
#elif PLATFORM == PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64
	m_adl = new QLibrary("atiadlxx.dll");
	if (!m_adl->load())
	{
		m_adl->setFileName("atiadlxy.dll");
		if (!m_adl->load())
		{
			delete m_adl;
			m_adl = NULL;
			return;
		}
	}
#endif

	ADL_Main_Control_Create = (TYPEOF_ADL_MAIN_CONTROL_CREATE)m_adl->resolve("ADL_Main_Control_Create");
	ADL_Main_Control_Destroy = (TYPEOF_ADL_MAIN_CONTROL_DESTROY)m_adl->resolve("ADL_Main_Control_Destroy");
	ADL_Adapter_NumberOfAdapters_Get = (TYPEOF_ADL_ADAPTER_NUMBEROFADAPTERS_GET)m_adl->resolve("ADL_Adapter_NumberOfAdapters_Get");
	ADL_Adapter_AdapterInfo_Get = (TYPEOF_ADL_ADAPTER_ADAPTERINFO_GET)m_adl->resolve("ADL_Adapter_AdapterInfo_Get");
	ADL_Adapter_Active_Get = (TYPEOF_ADL_ADAPTER_ACTIVE_GET)m_adl->resolve("ADL_Adapter_Active_Get");
	ADL_Adapter_ID_Get = (TYPEOF_ADL_ADAPTER_ID_GET)m_adl->resolve("ADL_Adapter_ID_Get");
	ADL_Overdrive_Caps = (TYPEOF_ADL_OVERDRIVE_CAPS)m_adl->resolve("ADL_Overdrive_Caps");
	
	ADL_Overdrive5_ThermalDevices_Enum = (TYPEOF_ADL_OVERDRIVE5_THERMALDEVICES_ENUM)m_adl->resolve("ADL_Overdrive5_ThermalDevices_Enum");
	ADL_Overdrive5_ODParameters_Get = (TYPEOF_ADL_OVERDRIVE5_ODPARAMETERS_GET)m_adl->resolve("ADL_Overdrive5_ODParameters_Get");
	ADL_Overdrive5_Temperature_Get = (TYPEOF_ADL_OVERDRIVE5_TEMPERATURE_GET)m_adl->resolve("ADL_Overdrive5_Temperature_Get");
	ADL_Overdrive5_FanSpeed_Get = (TYPEOF_ADL_OVERDRIVE5_FANSPEED_GET)m_adl->resolve("ADL_Overdrive5_FanSpeed_Get");
	ADL_Overdrive5_FanSpeedInfo_Get = (TYPEOF_ADL_OVERDRIVE5_FANSPEEDINFO_GET)m_adl->resolve("ADL_Overdrive5_FanSpeedInfo_Get");
	ADL_Overdrive5_ODPerformanceLevels_Get = (TYPEOF_ADL_OVERDRIVE5_ODPERFORMANCELEVELS_GET)m_adl->resolve("ADL_Overdrive5_ODPerformanceLevels_Get");
	ADL_Overdrive5_CurrentActivity_Get = (TYPEOF_ADL_OVERDRIVE5_CURRENTACTIVITY_GET)m_adl->resolve("ADL_Overdrive5_CurrentActivity_Get");
	ADL_Overdrive5_FanSpeed_Set = (TYPEOF_ADL_OVERDRIVE5_FANSPEED_SET)m_adl->resolve("ADL_Overdrive5_FanSpeed_Set");
	ADL_Overdrive5_ODPerformanceLevels_Set = (TYPEOF_ADL_OVERDRIVE5_ODPERFORMANCELEVELS_SET)m_adl->resolve("ADL_Overdrive5_ODPerformanceLevels_Set");
	ADL_Overdrive5_PowerControl_Caps = (TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_CAPS)m_adl->resolve("ADL_Overdrive5_PowerControl_Caps");
	ADL_Overdrive5_PowerControlInfo_Get = (TYPEOF_ADL_OVERDRIVE5_POWERCONTROLINFO_GET)m_adl->resolve("ADL_Overdrive5_PowerControlInfo_Get");
	ADL_Overdrive5_PowerControl_Get = (TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_GET)m_adl->resolve("ADL_Overdrive5_PowerControl_Get");
	ADL_Overdrive5_PowerControl_Set = (TYPEOF_ADL_OVERDRIVE5_POWERCONTROL_SET)m_adl->resolve("ADL_Overdrive5_PowerControl_Set");
	
	ADL_Overdrive6_FanSpeed_Get = (TYPEOF_ADL_OVERDRIVE6_FANSPEED_GET)m_adl->resolve("ADL_Overdrive6_FanSpeed_Get");
	ADL_Overdrive6_ThermalController_Caps = (TYPEOF_ADL_OVERDRIVE6_THERMALCONTROLLER_CAPS)m_adl->resolve("ADL_Overdrive6_ThermalController_Caps");
	ADL_Overdrive6_Temperature_Get = (TYPEOF_ADL_OVERDRIVE6_TEMPERATURE_GET)m_adl->resolve("ADL_Overdrive6_Temperature_Get");
	ADL_Overdrive6_Capabilities_Get = (TYPEOF_ADL_OVERDRIVE6_CAPABILITIES_GET)m_adl->resolve("ADL_Overdrive6_Capabilities_Get");
	ADL_Overdrive6_StateInfo_Get = (TYPEOF_ADL_OVERDRIVE6_STATEINFO_GET)m_adl->resolve("ADL_Overdrive6_StateInfo_Get");
	ADL_Overdrive6_CurrentStatus_Get = (TYPEOF_ADL_OVERDRIVE6_CURRENTSTATUS_GET)m_adl->resolve("ADL_Overdrive6_CurrentStatus_Get");
	ADL_Overdrive6_PowerControl_Caps = (TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_CAPS)m_adl->resolve("ADL_Overdrive6_PowerControl_Caps");
	ADL_Overdrive6_PowerControlInfo_Get = (TYPEOF_ADL_OVERDRIVE6_POWERCONTROLINFO_GET)m_adl->resolve("ADL_Overdrive6_PowerControlInfo_Get");
	ADL_Overdrive6_PowerControl_Get = (TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_GET)m_adl->resolve("ADL_Overdrive6_PowerControl_Get");
	ADL_Overdrive6_FanSpeed_Set = (TYPEOF_ADL_OVERDRIVE6_FANSPEED_SET)m_adl->resolve("ADL_Overdrive6_FanSpeed_Set");
	ADL_Overdrive6_State_Set = (TYPEOF_ADL_OVERDRIVE6_STATE_SET)m_adl->resolve("ADL_Overdrive6_State_Set");
	ADL_Overdrive6_PowerControl_Set = (TYPEOF_ADL_OVERDRIVE6_POWERCONTROL_SET)m_adl->resolve("ADL_Overdrive6_PowerControl_Set");

	ADL2_Adapter_Active_Get = (TYPEOF_ADL2_ADAPTER_ACTIVE_GET)m_adl->resolve("ADL2_Adapter_Active_Get");
	ADL2_OverdriveN_Capabilities_Get = (TYPEOF_ADL2_OVERDRIVEN_CAPABILITIES_GET)m_adl->resolve("ADL2_OverdriveN_Capabilities_Get");
	ADL2_OverdriveN_SystemClocks_Get = (TYPEOF_ADL2_OVERDRIVEN_SYSTEMCLOCKS_GET)m_adl->resolve("ADL2_OverdriveN_SystemClocks_Get");
	ADL2_OverdriveN_SystemClocks_Set = (TYPEOF_ADL2_OVERDRIVEN_SYSTEMCLOCKS_SET)m_adl->resolve("ADL2_OverdriveN_SystemClocks_Set");
	ADL2_OverdriveN_PerformanceStatus_Get = (TYPEOF_ADL2_OVERDRIVEN_PERFORMANCESTATUS_GET)m_adl->resolve("ADL2_OverdriveN_PerformanceStatus_Get");
	ADL2_OverdriveN_FanControl_Get = (TYPEOF_ADL2_OVERDRIVEN_FANCONTROL_GET)m_adl->resolve("ADL2_OverdriveN_FanControl_Get");
	ADL2_OverdriveN_FanControl_Set = (TYPEOF_ADL2_OVERDRIVEN_FANCONTROL_SET)m_adl->resolve("ADL2_OverdriveN_FanControl_Set");
	ADL2_OverdriveN_PowerLimit_Get = (TYPEOF_ADL2_OVERDRIVEN_POWERLIMIT_GET)m_adl->resolve("ADL2_OverdriveN_PowerLimit_Get");
	ADL2_OverdriveN_PowerLimit_Set = (TYPEOF_ADL2_OVERDRIVEN_POWERLIMIT_SET)m_adl->resolve("ADL2_OverdriveN_PowerLimit_Set");
	ADL2_OverdriveN_MemoryClocks_Get = (TYPEOF_ADL2_OVERDRIVEN_MEMORYCLOCKS_GET)m_adl->resolve("ADL2_OverdriveN_MemoryClocks_Get");
	ADL2_OverdriveN_MemoryClocks_Set = (TYPEOF_ADL2_OVERDRIVEN_MEMORYCLOCKS_GET)m_adl->resolve("ADL2_OverdriveN_MemoryClocks_Set");
	ADL2_Overdrive_Caps = (TYPEOF_ADL2_OVERDRIVE_CAPS)m_adl->resolve("ADL2_Overdrive_Caps");
	ADL2_OverdriveN_Temperature_Get = (TYPEOF_ADL2_OVERDRIVEN_TEMPERATURE_GET)m_adl->resolve("ADL2_OverdriveN_Temperature_Get");

	bool have_overdrive_5 = !(
		ADL_Overdrive5_ThermalDevices_Enum == NULL ||
		ADL_Overdrive5_ODParameters_Get == NULL ||
		ADL_Overdrive5_Temperature_Get == NULL ||
		ADL_Overdrive5_FanSpeed_Get == NULL ||
		ADL_Overdrive5_FanSpeedInfo_Get == NULL ||
		ADL_Overdrive5_ODPerformanceLevels_Get == NULL ||
		ADL_Overdrive5_CurrentActivity_Get == NULL ||
		ADL_Overdrive5_FanSpeed_Set == NULL ||
		ADL_Overdrive5_ODPerformanceLevels_Set == NULL ||
		ADL_Overdrive5_PowerControl_Caps == NULL ||
		ADL_Overdrive5_PowerControlInfo_Get == NULL ||
		ADL_Overdrive5_PowerControl_Get == NULL ||
		ADL_Overdrive5_PowerControl_Set == NULL
		);

	bool have_overdrive_6 = !(
		ADL_Overdrive6_FanSpeed_Get == NULL ||
		ADL_Overdrive6_ThermalController_Caps == NULL ||
		ADL_Overdrive6_Temperature_Get == NULL ||
		ADL_Overdrive6_Capabilities_Get == NULL ||
		ADL_Overdrive6_StateInfo_Get == NULL ||
		ADL_Overdrive6_CurrentStatus_Get == NULL ||
		ADL_Overdrive6_PowerControl_Caps == NULL ||
		ADL_Overdrive6_PowerControlInfo_Get == NULL ||
		ADL_Overdrive6_PowerControl_Get == NULL ||
		ADL_Overdrive6_FanSpeed_Set == NULL ||
		ADL_Overdrive6_State_Set == NULL ||
		ADL_Overdrive6_PowerControl_Set == NULL
		);

	bool have_overdrive_N = !(
		ADL2_Adapter_Active_Get == NULL ||
		ADL2_OverdriveN_Capabilities_Get == NULL ||
		ADL2_OverdriveN_SystemClocks_Get == NULL ||
		ADL2_OverdriveN_SystemClocks_Set == NULL ||
		ADL2_OverdriveN_PerformanceStatus_Get == NULL ||
		ADL2_OverdriveN_FanControl_Get == NULL ||
		ADL2_OverdriveN_FanControl_Set == NULL ||
		ADL2_OverdriveN_PowerLimit_Get == NULL ||
		ADL2_OverdriveN_PowerLimit_Set == NULL ||
		ADL2_OverdriveN_MemoryClocks_Get == NULL ||
		ADL2_OverdriveN_MemoryClocks_Set == NULL ||
		ADL2_Overdrive_Caps == NULL ||
		ADL2_OverdriveN_Temperature_Get == NULL
		);

	if (ADL_Main_Control_Create==NULL ||
		ADL_Main_Control_Destroy==NULL ||
		ADL_Adapter_NumberOfAdapters_Get==NULL ||
		ADL_Adapter_AdapterInfo_Get==NULL ||
		ADL_Adapter_Active_Get==NULL ||
		ADL_Adapter_ID_Get==NULL ||
		ADL_Overdrive_Caps==NULL ||
		!(have_overdrive_5 || have_overdrive_6 || have_overdrive_N))
	{
		m_adl->unload();
		delete m_adl;
		m_adl = NULL;

		m_error = true;
		m_error_title = "Old Drivers Detected";
		m_error_message = "Your AMD/ATI graphics drivers do not have the required functionality to run L0phtCrack 7's GPU support. Please upgrade your graphics drivers.";

		return;
	}

	if (ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1) != ADL_OK)
	{
		m_adl->unload();
		delete m_adl;
		m_adl = NULL;

		m_error = true;
		m_error_title = "Unable To Create Graphics Driver Main Control";
		m_error_message = "Your AMD/ATI graphics drivers do not have the required functionality to run L0phtCrack 7's GPU support. Please upgrade your graphics drivers.";

		return;
	}
	
	m_adl_adapters_cached = false;
	m_bInitADL = true;
}

void CLC7SystemMonitor::TerminateADL()
{
	if (m_adl)
	{
		if (ADL_Main_Control_Destroy() != ADL_OK)
		{
			Q_ASSERT(0);
		}

		m_adl->unload();
		delete m_adl;
		m_adl = NULL;
	}
}

void CLC7SystemMonitor::GetOverdrive5Info(int adapterId, int &fanspeed, int &fanspeed_rpm, int &utilization, int &temperature)
{
	fanspeed = -1;
	utilization = -1;
	temperature = -1;

	ADLThermalControllerInfo tci = { 0 };
	tci.iSize = sizeof(ADLThermalControllerInfo);

	int ADL_Err = ADL_Overdrive5_ThermalDevices_Enum(adapterId, 0, &tci);
	if (ADL_Err == ADL_OK)
	{
		if (tci.iThermalDomain == ADL_DL_THERMAL_DOMAIN_GPU)
		{
			// Temperature
			ADLTemperature adlTemperature = { 0 };
			adlTemperature.iSize = sizeof(ADLTemperature);
			if (ADL_OK == ADL_Overdrive5_Temperature_Get(adapterId, 0, &adlTemperature))
			{
				temperature = adlTemperature.iTemperature / 1000; // The temperature is returned in millidegrees Celsius.
			}
		}
	}

	// Fan speed
	ADLFanSpeedInfo fanSpeedInfo = { 0 };
	fanSpeedInfo.iSize = sizeof(ADLFanSpeedInfo);
	if (ADL_OK == ADL_Overdrive5_FanSpeedInfo_Get(adapterId, 0, &fanSpeedInfo))
	{
		if ((fanSpeedInfo.iFlags & ADL_DL_FANCTRL_SUPPORTS_RPM_READ) == ADL_DL_FANCTRL_SUPPORTS_RPM_READ)
		{
			ADLFanSpeedValue fanSpeedValue = { 0 };
			fanSpeedValue.iSize = sizeof(ADLFanSpeedValue);
			fanSpeedValue.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_RPM;
			if (ADL_OK == ADL_Overdrive5_FanSpeed_Get(adapterId, 0, &fanSpeedValue))
			{
				fanspeed_rpm = fanSpeedValue.iFanSpeed;
			}
		}
		if ((fanSpeedInfo.iFlags & ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ) == ADL_DL_FANCTRL_SUPPORTS_PERCENT_READ)
		{
			ADLFanSpeedValue fanSpeedValue = { 0 };
			fanSpeedValue.iSize = sizeof(ADLFanSpeedValue);
			fanSpeedValue.iSpeedType = ADL_DL_FANCTRL_SPEED_TYPE_PERCENT;
			if (ADL_OK == ADL_Overdrive5_FanSpeed_Get(adapterId, 0, &fanSpeedValue))
			{
				fanspeed = fanSpeedValue.iFanSpeed;
			}
		}
	}

	// Utilization
	ADLODParameters overdriveParameters = { 0 };
	overdriveParameters.iSize = sizeof(ADLODParameters);
	if (ADL_Overdrive5_ODParameters_Get(adapterId, &overdriveParameters) == ADL_OK) 
	{
		ADLPMActivity activity = { 0 };
		activity.iSize = sizeof(ADLPMActivity);
		if (ADL_Overdrive5_CurrentActivity_Get(adapterId, &activity) == ADL_OK)
		{
			if (overdriveParameters.iActivityReportingSupported)
			{
				utilization = activity.iActivityPercent;
			}
		}
	}
}

void CLC7SystemMonitor::GetOverdrive6Info(int adapterId, int &fanspeed, int &fanspeed_rpm, int &utilization, int &temperature)
{
	fanspeed = -1;
	utilization = -1;
	temperature = -1;

	ADLOD6FanSpeedInfo fanSpeedInfo = { 0 };
	ADLOD6ThermalControllerCaps thermalControllerCaps = { 0 };
	ADLOD6Capabilities od6Capabilities = { 0 };
	ADLOD6CurrentStatus currentStatus = { 0 };

	if (ADL_Overdrive6_ThermalController_Caps(adapterId, &thermalControllerCaps) == ADL_OK) 
	{
		if (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_CONTROL)
		{
			if ((thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_PERCENT_READ) || (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_FANSPEED_RPM_READ))
			{
				if (ADL_Overdrive6_FanSpeed_Get(adapterId, &fanSpeedInfo) == ADL_OK)
				{
					if (fanSpeedInfo.iSpeedType & ADL_OD6_FANSPEED_TYPE_PERCENT)
					{
						fanspeed = fanSpeedInfo.iFanSpeedPercent;
					}
					else
					{
						fanspeed = 0;
					}
					if (fanSpeedInfo.iSpeedType & ADL_OD6_FANSPEED_TYPE_RPM)
					{
						fanspeed_rpm = fanSpeedInfo.iFanSpeedRPM;
					}
					else
					{
						fanspeed_rpm = 0;
					}
				}
			}
		}

		if (thermalControllerCaps.iCapabilities & ADL_OD6_TCCAPS_THERMAL_CONTROLLER)
		{
			if (ADL_Overdrive6_Temperature_Get(adapterId, &temperature) == ADL_OK)
			{
				temperature = temperature / 1000;
			}
		}

		if (ADL_Overdrive6_Capabilities_Get(adapterId, &od6Capabilities) == ADL_OK)
		{
			if (od6Capabilities.iCapabilities & ADL_OD6_CAPABILITY_GPU_ACTIVITY_MONITOR)
			{
				if (ADL_Overdrive6_CurrentStatus_Get(adapterId, &currentStatus) == ADL_OK)
				{
					utilization = currentStatus.iActivityPercent;
				}
			}
		}
	}
}

void CLC7SystemMonitor::GetOverdriveNInfo(int adapterId, int &fanspeed, int &fanspeed_rpm, int &utilization, int &temperature)
{
	fanspeed = -1;
	utilization = -1;
	temperature = -1;

	ADLODNCapabilities overdriveCapabilities;
	memset(&overdriveCapabilities, 0, sizeof(ADLODNCapabilities));
	if (ADL2_OverdriveN_Capabilities_Get(NULL, adapterId, &overdriveCapabilities) == ADL_OK)
	{
		ADLODNFanControl odNFanControl;
		memset(&odNFanControl, 0, sizeof(ADLODNFanControl));

		if (ADL2_OverdriveN_FanControl_Get(NULL, adapterId, &odNFanControl) == ADL_OK)
		{
			if (odNFanControl.iCurrentFanSpeedMode == ADL_DL_FANCTRL_SPEED_TYPE_RPM || odNFanControl.iCurrentFanSpeedMode == 0)
			{
				fanspeed_rpm = odNFanControl.iCurrentFanSpeed;
				if (overdriveCapabilities.fanSpeed.iMax)
				{
					fanspeed = fanspeed_rpm * 100 / overdriveCapabilities.fanSpeed.iMax;
				}
			}
			else if (odNFanControl.iCurrentFanSpeedMode == ADL_DL_FANCTRL_SPEED_TYPE_PERCENT)
			{
				fanspeed = odNFanControl.iCurrentFanSpeed;
				fanspeed_rpm = overdriveCapabilities.fanSpeed.iMax*fanspeed / 100;
			}
		}
	}

	int temp;
	if (ADL2_OverdriveN_Temperature_Get(NULL, adapterId, 1, &temp) == ADL_OK)
	{
		temperature = temp / 1000;
	}

	ADLODNPerformanceStatus odNPerformanceStatus;
	memset(&odNPerformanceStatus, 0, sizeof(ADLODNPerformanceStatus));
	if (ADL2_OverdriveN_PerformanceStatus_Get(NULL, adapterId, &odNPerformanceStatus) == ADL_OK)
	{
		utilization = odNPerformanceStatus.iGPUActivityPercent;
	}
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NVAPI
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if PLATFORM==PLATFORM_WIN32 || PLATFORM==PLATFORM_WIN64 

void CLC7SystemMonitor::InitializeNVAPI(void)
{
	if (NvAPI_Initialize() == NVAPI_OK)
	{
		m_bInitNVAPI = true;
	}
}

void CLC7SystemMonitor::TerminateNVAPI(void)
{
	NvAPI_Unload();
}




#endif

bool CLC7SystemMonitor::GetAllGPUStatus(QList<ILC7SystemMonitor::GPU_STATUS> & status_per_core, QString &error)
{
	QMutexLocker lock(&m_mutex);

	status_per_core.clear();

	// Process AMD cards
	if (m_bInitADL)
	{
		if (!m_adl_adapters_cached)
		{
			// Do this once for speed
			int  iNumberAdapters;
			if (ADL_OK != ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters))
			{
				error = "Cannot get the number of AMD adapters!";
				return false;
			}

			LPAdapterInfo lpAdapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * iNumberAdapters);
			memset(lpAdapterInfo, '\0', sizeof(AdapterInfo) * iNumberAdapters);

			// Get the AdapterInfo structure for all adapters in the system
			ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters);

			//QSet<QPair<int, int> > found;
			QSet<int> found;

			for (int adapterId = 0; adapterId < iNumberAdapters; adapterId++)
			{
				//			int lpstatus = ADL_FALSE;
				//			if (ADL_OK != ADL_Adapter_Active_Get(adapterId, &lpstatus))
				//				continue;
				//			if (lpstatus != ADL_TRUE)
				//				continue;

				//			if (lpAdapterInfo[adapterId].strUDID[0] == '\0' || lpAdapterInfo[adapterId].iVendorID != 0x3EA)
				//			{
				//				continue;
				//			}
				//			QPair<int, int> adapterkey = QPair<int, int>(lpAdapterInfo[adapterId].iBusNumber, lpAdapterInfo[adapterId].iDeviceNumber);
				int adapterkey;
				if (ADL_OK != ADL_Adapter_ID_Get(adapterId, &adapterkey))
				{
					continue;
				}

				if (found.contains(adapterkey))
				{
					continue;
				}
				found.insert(adapterkey);

				AdapterInfo *ai = lpAdapterInfo + adapterId;

				int iOverdriveSupported = 0;
				int iOverdriveEnabled = 0;
				int iOverdriveVersion = 0;
				if (ADL_OK != ADL_Overdrive_Caps(adapterId, &iOverdriveSupported, &iOverdriveEnabled, &iOverdriveVersion) || !iOverdriveSupported)
				{
					continue;
				}

				ADL_ADAPTER_INFO adapter;
				adapter.adapterName = QString::fromLocal8Bit(ai->strAdapterName);
				adapter.adapterId = adapterId;
				adapter.overdriveVersion = iOverdriveVersion;

				m_adl_adapters.append(adapter);
			}

			free(lpAdapterInfo);

			m_adl_adapters_cached = true;
		}
			
		foreach(ADL_ADAPTER_INFO adapter, m_adl_adapters)
		{
			int fanspeed = 0;
			int fanspeed_rpm = 0;
			int utilization = 0;
			int temperature = 0;

			if (adapter.overdriveVersion == 5)
			{
				GetOverdrive5Info(adapter.adapterId, fanspeed, fanspeed_rpm, utilization, temperature);
			}
			else if (adapter.overdriveVersion == 6)
			{ 
				GetOverdrive6Info(adapter.adapterId, fanspeed, fanspeed_rpm, utilization, temperature);
			}
			else if (adapter.overdriveVersion >= 7)
			{
				GetOverdriveNInfo(adapter.adapterId, fanspeed, fanspeed_rpm, utilization, temperature);
			}

			GPU_STATUS gs;
			gs.gpu_name = adapter.adapterName;
			gs.gpu_type = "AMD";
			gs.fanspeed = fanspeed;
			gs.fanspeed_rpm = fanspeed_rpm;
			gs.utilization = utilization;
			gs.temperature = temperature;
				
			status_per_core.append(gs);
		}
		
	}

	// Process NVIDIA cards
	
	if (m_bInitNVAPI)
	{
		NvPhysicalGpuHandle gpuhandles[NVAPI_MAX_PHYSICAL_GPUS];
		NvU32 gpuCount;
		if (NvAPI_EnumPhysicalGPUs(gpuhandles, &gpuCount) == NVAPI_OK && gpuCount>0)
		{
			
			for (NvU32 gpu = 0; gpu < (int)gpuCount; gpu++)
			{
				GPU_STATUS gs;
			
				NvAPI_ShortString fullname;
				if (NvAPI_GPU_GetFullName(gpuhandles[gpu], fullname) == NVAPI_OK)
				{
					gs.gpu_name = QString::fromLatin1(fullname);
				}
				else
				{
					gs.gpu_name = "Unknown";
				}

				gs.gpu_type = "NVIDIA";


				// Get utilization
				NV_GPU_DYNAMIC_PSTATES_INFO_EX psinfo;
				memset(&psinfo, 0, sizeof(psinfo));
				psinfo.version = NV_GPU_DYNAMIC_PSTATES_INFO_EX_VER;

				if (NvAPI_GPU_GetDynamicPstatesInfoEx(gpuhandles[gpu], &psinfo) == NVAPI_OK)
				{
					gs.utilization = psinfo.utilization[0].bIsPresent ? psinfo.utilization[0].percentage : 0;
				}
				else
				{
					gs.utilization = 0;
				}

				// Get temperature
				NV_GPU_THERMAL_SETTINGS gts;
				memset(&gts, 0, sizeof(gts));
				gts.version = NV_GPU_THERMAL_SETTINGS_VER;

				if (NvAPI_GPU_GetThermalSettings(gpuhandles[gpu], NVAPI_THERMAL_TARGET_ALL, &gts) == NVAPI_OK)
				{
					int temp=0;
					for (NvU32 t = 0; t < gts.count; t++)
					{
						if (gts.sensor[t].currentTemp>temp)
						{
							temp = gts.sensor[t].currentTemp;
						}
					}
					gs.temperature = temp;
				}
				else
				{
					gs.temperature = 0;
				}
				
				// Get fan speed
				NvU32 fs;
				if (NvAPI_GPU_GetTachReading(gpuhandles[gpu], &fs) == NVAPI_OK)
				{

					static NvU32 max_fan_speed = 1500;
					if (fs > max_fan_speed)
					{
						max_fan_speed = fs;
					}

					gs.fanspeed = fs*100/max_fan_speed;
					gs.fanspeed_rpm = fs;
				}
				else
				{
					gs.fanspeed = 0;
					gs.fanspeed_rpm = 0;
				}
				
				status_per_core.append(gs);
			}
		}
	}


	return true;
}
