#ifndef __INC_ILC7InstructionSet_H
#define __INC_ILC7InstructionSet_H

#include"core/ILC7Interface.h"

class ILC7CPUInformation:public ILC7Interface
{
protected:
	virtual ~ILC7CPUInformation() {}

public:
	
	// getters
	
	virtual int CoreCount(void)=0;
	virtual QString Vendor(void)=0;
	virtual QString Brand(void)=0;

	virtual bool SSE3(void)=0;
	virtual bool PCLMULQDQ(void)=0;
	virtual bool MONITOR(void)=0;
	virtual bool SSSE3(void)=0;
	virtual bool FMA(void)=0;
	virtual bool CMPXCHG16B(void)=0;
	virtual bool SSE41(void)=0;
	virtual bool SSE42(void)=0;
	virtual bool MOVBE(void)=0;
	virtual bool POPCNT(void)=0;
	virtual bool AES(void)=0;
	virtual bool XSAVE(void)=0;
	virtual bool OSXSAVE(void)=0;
	virtual bool AVX(void)=0;
	virtual bool F16C(void)=0;
	virtual bool RDRAND(void)=0;

	virtual bool MSR(void)=0;
	virtual bool CX8(void)=0;
	virtual bool SEP(void)=0;
	virtual bool CMOV(void)=0;
	virtual bool CLFSH(void)=0;
	virtual bool MMX(void)=0;
	virtual bool FXSR(void)=0;
	virtual bool SSE(void)=0;
	virtual bool SSE2(void)=0;

	virtual bool FSGSBASE(void)=0;
	virtual bool BMI1(void)=0;
	virtual bool HLE(void)=0;
	virtual bool AVX2(void)=0;
	virtual bool BMI2(void)=0;
	virtual bool ERMS(void)=0;
	virtual bool INVPCID(void)=0;
	virtual bool RTM(void)=0;
	virtual bool AVX512F(void)=0;
	virtual bool RDSEED(void)=0;
	virtual bool ADX(void)=0;
	virtual bool AVX512PF(void)=0;
	virtual bool AVX512ER(void)=0;
	virtual bool AVX512CD(void)=0;
	virtual bool SHA(void)=0;

	virtual bool PREFETCHWT1(void)=0;

	virtual bool LAHF(void)=0;
	virtual bool LZCNT(void)=0;
	virtual bool ABM(void)=0;
	virtual bool SSE4a(void)=0;
	virtual bool XOP(void)=0;
	virtual bool TBM(void)=0;

	virtual bool SYSCALL(void)=0;
	virtual bool MMXEXT(void)=0;
	virtual bool RDTSCP(void)=0;
	virtual bool _3DNOWEXT(void)=0;
	virtual bool _3DNOW(void)=0;

	virtual bool XMM_SAVED(void)=0;
	virtual bool YMM_SAVED(void)=0;

};

#endif