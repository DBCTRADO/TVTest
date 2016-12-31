#include "stdafx.h"
#include "Simd.h"
#include <intrin.h>
#include "../Common/DebugDef.h"


enum {
	INSTRUCTION_MMX    = 0x00000001U,
	INSTRUCTION_SSE    = 0x00000002U,
	INSTRUCTION_SSE2   = 0x00000004U,
	INSTRUCTION_SSE3   = 0x00000008U,
	INSTRUCTION_SSSE3  = 0x00000010U,
	INSTRUCTION_SSE4_1 = 0x00000020U,
	INSTRUCTION_SSE4_2 = 0x00000040U
};


static unsigned int GetSupportedInstructions()
{
#if defined(_M_IX86) || defined(_M_AMD64)
	int CPUInfo[4];

	::__cpuid(CPUInfo, 1);

	unsigned int Supported = 0;

	if (CPUInfo[3] & 0x00800000)
		Supported |= INSTRUCTION_MMX;
	if (CPUInfo[3] & 0x02000000)
		Supported |= INSTRUCTION_SSE;
	if (CPUInfo[3] & 0x04000000)
		Supported |= INSTRUCTION_SSE2;
	if (CPUInfo[2] & 0x00000001)
		Supported |= INSTRUCTION_SSE3;
	if (CPUInfo[2] & 0x00000200)
		Supported |= INSTRUCTION_SSSE3;
	if (CPUInfo[2] & 0x00080000)
		Supported |= INSTRUCTION_SSE4_1;
	if (CPUInfo[2] & 0x00100000)
		Supported |= INSTRUCTION_SSE4_2;

	return Supported;
#else
	return 0;
#endif
}


template<typename T> inline void AtomicAnd32(T *pDest, T Operand)
{
#ifdef _M_IX86
	::_InterlockedAnd((long volatile*)pDest, (long)Operand);
#else
	::InterlockedAnd((LONG volatile*)pDest, (LONG)Operand);
#endif
}

template<typename T> inline void AtomicOr32(T *pDest, T Operand)
{
#ifdef _M_IX86
	::_InterlockedOr((long volatile*)pDest, (long)Operand);
#else
	::InterlockedOr((LONG volatile*)pDest, (LONG)Operand);
#endif
}


class CCpuIdentify
{
public:
	CCpuIdentify()
	{
		m_Available = GetSupportedInstructions();
		m_Enabled = m_Available;

		TRACE(TEXT("MMX %s SSE %s SSE2 %s SSE3 %s SSSE3 %s SSE4.1 %s SSE4.2 %s\n"),
			  (m_Available & INSTRUCTION_MMX)    ? TEXT("avail") : TEXT("n/a"),
			  (m_Available & INSTRUCTION_SSE)    ? TEXT("avail") : TEXT("n/a"),
			  (m_Available & INSTRUCTION_SSE2)   ? TEXT("avail") : TEXT("n/a"),
			  (m_Available & INSTRUCTION_SSE3)   ? TEXT("avail") : TEXT("n/a"),
			  (m_Available & INSTRUCTION_SSSE3)  ? TEXT("avail") : TEXT("n/a"),
			  (m_Available & INSTRUCTION_SSE4_1) ? TEXT("avail") : TEXT("n/a"),
			  (m_Available & INSTRUCTION_SSE4_2) ? TEXT("avail") : TEXT("n/a"));
	}

	bool IsAvailable(unsigned int Instruction) const
	{
		return (m_Available & Instruction) == Instruction;
	}

	bool IsEnabled(unsigned int Instruction) const
	{
		return (m_Enabled & Instruction) == Instruction;
	}

	void SetEnabled(unsigned int Instruction, bool bEnabled)
	{
		if (bEnabled)
			AtomicOr32(&m_Enabled, Instruction & m_Available);
		else
			AtomicAnd32(&m_Enabled, ~Instruction);
	}

private:
	unsigned int m_Available;
	unsigned int m_Enabled;
};

static CCpuIdentify g_CpuIdentify;




namespace TsEngine
{


#if !defined(_M_AMD64)
bool IsSSE2Available()
{
	return g_CpuIdentify.IsAvailable(INSTRUCTION_SSE2);
}
#endif


bool IsSSE2Enabled()
{
	return g_CpuIdentify.IsEnabled(INSTRUCTION_SSE2);
}


void SetSSE2Enabled(bool bEnabled)
{
	g_CpuIdentify.SetEnabled(INSTRUCTION_SSE2, bEnabled);
}


}
