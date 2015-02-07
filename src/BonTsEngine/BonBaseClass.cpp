#include "stdafx.h"
#include "BonBaseClass.h"




CBonBaseClass::CBonBaseClass()
	: m_pTracer(NULL)
{
}


CBonBaseClass::~CBonBaseClass()
{
}


void CBonBaseClass::SetTracer(CTracer *pTracer)
{
	m_pTracer = pTracer;
}


void CBonBaseClass::Trace(CTracer::TraceType Type, LPCTSTR pszOutput, ...)
{
	if (m_pTracer != NULL && pszOutput != NULL) {
		va_list Args;

		va_start(Args, pszOutput);
		m_pTracer->TraceV(Type, pszOutput, Args);
		va_end(Args);
	}
}
