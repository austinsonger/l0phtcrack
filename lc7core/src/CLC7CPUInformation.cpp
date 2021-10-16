#include"stdafx.h"
#include <signal.h>
#include <intrin.h>

CLC7CPUInformation::~CLC7CPUInformation()
{

}

ILC7Interface *CLC7CPUInformation::GetInterfaceVersion(QString interface_name)
{
	if (interface_name == "ILC7CPUInformation")
	{
		return this;
	}
	return NULL;
}

int CLC7CPUInformation::CoreCount(void)
{
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );

	DWORD numCPU = sysinfo.dwNumberOfProcessors;

	return numCPU;
#else
	return std::thread::hardware_concurrency();
#endif

}


QString CLC7CPUInformation::Vendor(void) 
{ 
	return QString::fromStdString(CPU_Rep.vendor_); 
}

QString CLC7CPUInformation::Brand(void) 
{ 
	return QString::fromStdString(CPU_Rep.brand_); 
}

bool CLC7CPUInformation::SSE3(void) { return CPU_Rep.f_1_ECX_[0]; }
bool CLC7CPUInformation::PCLMULQDQ(void) { return CPU_Rep.f_1_ECX_[1]; }
bool CLC7CPUInformation::MONITOR(void) { return CPU_Rep.f_1_ECX_[3]; }
bool CLC7CPUInformation::SSSE3(void) { return CPU_Rep.f_1_ECX_[9]; }
bool CLC7CPUInformation::FMA(void) { return CPU_Rep.f_1_ECX_[12]; }
bool CLC7CPUInformation::CMPXCHG16B(void) { return CPU_Rep.f_1_ECX_[13]; }
bool CLC7CPUInformation::SSE41(void) { return CPU_Rep.f_1_ECX_[19]; }
bool CLC7CPUInformation::SSE42(void) { return CPU_Rep.f_1_ECX_[20]; }
bool CLC7CPUInformation::MOVBE(void) { return CPU_Rep.f_1_ECX_[22]; }
bool CLC7CPUInformation::POPCNT(void) { return CPU_Rep.f_1_ECX_[23]; }
bool CLC7CPUInformation::AES(void) { return CPU_Rep.f_1_ECX_[25]; }
bool CLC7CPUInformation::XSAVE(void) { return CPU_Rep.f_1_ECX_[26]; }
bool CLC7CPUInformation::OSXSAVE(void) { return CPU_Rep.f_1_ECX_[27]; }
bool CLC7CPUInformation::AVX(void) { return CPU_Rep.f_1_ECX_[28]; }
bool CLC7CPUInformation::F16C(void) { return CPU_Rep.f_1_ECX_[29]; }
bool CLC7CPUInformation::RDRAND(void) { return CPU_Rep.f_1_ECX_[30]; }
bool CLC7CPUInformation::MSR(void) { return CPU_Rep.f_1_EDX_[5]; }
bool CLC7CPUInformation::CX8(void) { return CPU_Rep.f_1_EDX_[8]; }
bool CLC7CPUInformation::SEP(void) { return CPU_Rep.f_1_EDX_[11]; }
bool CLC7CPUInformation::CMOV(void) { return CPU_Rep.f_1_EDX_[15]; }
bool CLC7CPUInformation::CLFSH(void) { return CPU_Rep.f_1_EDX_[19]; }
bool CLC7CPUInformation::MMX(void) { return CPU_Rep.f_1_EDX_[23]; }
bool CLC7CPUInformation::FXSR(void) { return CPU_Rep.f_1_EDX_[24]; }
bool CLC7CPUInformation::SSE(void) { return CPU_Rep.f_1_EDX_[25]; }
bool CLC7CPUInformation::SSE2(void) { return CPU_Rep.f_1_EDX_[26]; }
bool CLC7CPUInformation::FSGSBASE(void) { return CPU_Rep.f_7_EBX_[0]; }
bool CLC7CPUInformation::BMI1(void) { return CPU_Rep.f_7_EBX_[3]; }
bool CLC7CPUInformation::HLE(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[4]; }
bool CLC7CPUInformation::AVX2(void) { return CPU_Rep.f_7_EBX_[5]; }
bool CLC7CPUInformation::BMI2(void) { return CPU_Rep.f_7_EBX_[8]; }
bool CLC7CPUInformation::ERMS(void) { return CPU_Rep.f_7_EBX_[9]; }
bool CLC7CPUInformation::INVPCID(void) { return CPU_Rep.f_7_EBX_[10]; }
bool CLC7CPUInformation::RTM(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[11]; }
bool CLC7CPUInformation::AVX512F(void) { return CPU_Rep.f_7_EBX_[16]; }
bool CLC7CPUInformation::RDSEED(void) { return CPU_Rep.f_7_EBX_[18]; }
bool CLC7CPUInformation::ADX(void) { return CPU_Rep.f_7_EBX_[19]; }
bool CLC7CPUInformation::AVX512PF(void) { return CPU_Rep.f_7_EBX_[26]; }
bool CLC7CPUInformation::AVX512ER(void) { return CPU_Rep.f_7_EBX_[27]; }
bool CLC7CPUInformation::AVX512CD(void) { return CPU_Rep.f_7_EBX_[28]; }
bool CLC7CPUInformation::SHA(void) { return CPU_Rep.f_7_EBX_[29]; }
bool CLC7CPUInformation::PREFETCHWT1(void) { return CPU_Rep.f_7_ECX_[0]; }
bool CLC7CPUInformation::LAHF(void) { return CPU_Rep.f_81_ECX_[0]; }
bool CLC7CPUInformation::LZCNT(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_ECX_[5]; }
bool CLC7CPUInformation::ABM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[5]; }
bool CLC7CPUInformation::SSE4a(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[6]; }
bool CLC7CPUInformation::XOP(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[11]; }
bool CLC7CPUInformation::TBM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[21]; }
bool CLC7CPUInformation::SYSCALL(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[11]; }
bool CLC7CPUInformation::MMXEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[22]; }
bool CLC7CPUInformation::RDTSCP(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[27]; }
bool CLC7CPUInformation::_3DNOWEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[30]; }
bool CLC7CPUInformation::_3DNOW(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[31]; }
bool CLC7CPUInformation::XMM_SAVED(void) { return CPU_Rep.f_1_ECX_[26] && CPU_Rep.f_bv_0_EAX_[1]; }
bool CLC7CPUInformation::YMM_SAVED(void) { return CPU_Rep.f_1_ECX_[26] && CPU_Rep.f_bv_0_EAX_[2]; }

static void sigillhandler(int)
{
	throw "barf";
}

bool CLC7CPUInformation::InstructionSet_Internal::GetXGETBV(unsigned long long &bv0)
{
	bool ok = true;

#ifdef _WIN32
	__try 
	{
#else
	// Catch an illegal instruction
	void(__cdecl *prevsigill)(int) = signal(SIGILL, sigillhandler);
	try
	{
#endif
		bv0 = _xgetbv(0);
#ifdef _WIN32
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		bv0 = 0;
		ok = false;
	}
#else
	}
	catch(...)
	{
		bv0 = 0;
		ok = false;
	}
	signal(SIGILL, prevsigill);
#endif

	return ok;
}

CLC7CPUInformation::InstructionSet_Internal::InstructionSet_Internal(): 
	nIds_( 0 ),
	nExIds_( 0 ),
	isIntel_( false ),
	isAMD_( false ),
	f_1_ECX_( 0 ),
	f_1_EDX_( 0 ),
	f_7_EBX_( 0 ),
	f_7_ECX_( 0 ),
	f_81_ECX_( 0 ),
	f_81_EDX_( 0 ),
	data_(),
	extdata_(),
	f_bv_0_EAX_( 0 )
{
	//int cpuInfo[4] = {-1};
	std::array<int, 4> cpui;

	// Calling __cpuid with 0x0 as the function_id argument
	// gets the number of the highest valid function ID.
	__cpuid(cpui.data(), 0);
	nIds_ = cpui[0];

	for (int i = 0; i <= nIds_; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		data_.push_back(cpui);
	}

	// Capture vendor string
	char vendor[0x20];
	memset(vendor, 0, sizeof(vendor));
	*reinterpret_cast<int*>(vendor) = data_[0][1];
	*reinterpret_cast<int*>(vendor + 4) = data_[0][3];
	*reinterpret_cast<int*>(vendor + 8) = data_[0][2];
	vendor_ = vendor;
	if (vendor_ == "GenuineIntel")
	{
		isIntel_ = true;
	}
	else if (vendor_ == "AuthenticAMD")
	{
		isAMD_ = true;
	}

	// load bitset with flags for function 0x00000001
	if (nIds_ >= 1)
	{
		f_1_ECX_ = data_[1][2];
		f_1_EDX_ = data_[1][3];
	}

	// load bitset with flags for function 0x00000007
	if (nIds_ >= 7)
	{
		f_7_EBX_ = data_[7][1];
		f_7_ECX_ = data_[7][2];
	}

	// Calling __cpuid with 0x80000000 as the function_id argument
	// gets the number of the highest valid extended ID.
	__cpuid(cpui.data(), 0x80000000);
	nExIds_ = cpui[0];

	char brand[0x40];
	memset(brand, 0, sizeof(brand));

	for (int i = 0x80000000; i <= nExIds_; ++i)
	{
		__cpuidex(cpui.data(), i, 0);
		extdata_.push_back(cpui);
	}

	// load bitset with flags for function 0x80000001
	if (nExIds_ >= 0x80000001)
	{
		f_81_ECX_ = extdata_[1][2];
		f_81_EDX_ = extdata_[1][3];
	}

	// Interpret CPU brand string if reported
	if (nExIds_ >= 0x80000004)
	{
		memcpy(brand, extdata_[2].data(), sizeof(cpui));
		memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
		memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
		brand_ = brand;
	}

	// XGETBV
	if (f_1_ECX_[26])
	{
		unsigned long long bv0;
		if (GetXGETBV(bv0))
		{
			f_bv_0_EAX_ = (unsigned long)(bv0);
			f_bv_0_EDX_ = (unsigned long)(bv0 >> 32);
		}
		else
		{
			f_1_ECX_[26] = 0;
			f_bv_0_EAX_ = 0;
			f_bv_0_EDX_ = 0;
		}
	}
	else
	{
		f_bv_0_EAX_ = 0;
		f_bv_0_EDX_ = 0;
	}


};










