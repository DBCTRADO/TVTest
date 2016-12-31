#ifndef BON_BASE_CLASS_H
#define BON_BASE_CLASS_H


#include "Common.h"
#include "Exception.h"
#include "TsUtilClass.h"


class ABSTRACT_CLASS_DECL CBonBaseClass : public CBonErrorHandler
{
public:
	CBonBaseClass();
	virtual ~CBonBaseClass() = 0;
	virtual void SetTracer(CTracer *pTracer);

protected:
	void Trace(CTracer::TraceType Type, LPCTSTR pszOutput, ...);

	CTracer *m_pTracer;
};


#endif
