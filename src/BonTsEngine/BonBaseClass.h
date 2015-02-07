#ifndef BON_BASE_CLASS_H
#define BON_BASE_CLASS_H


#include "Common.h"
#include "Exception.h"
#include "TsUtilClass.h"


class ABSTRACT_CLASS_DECL CBonBaseClass : public CBonErrorHandler
{
	CTracer *m_pTracer;
public:
	CBonBaseClass();
	virtual ~CBonBaseClass() = 0;
	virtual void SetTracer(CTracer *pTracer);
protected:
	void Trace(CTracer::TraceType Type, LPCTSTR pszOutput, ...);
};


#endif
