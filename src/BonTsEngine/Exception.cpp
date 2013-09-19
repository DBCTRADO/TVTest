#include "stdafx.h"
#include <new>
#include "Exception.h"
#include "StdUtil.h"




/*
CBonException::CBonException(LPCTSTR pszFormat, ...)
{
	va_list Args;
	TCHAR szText[1024];
	int Length;

	va_start(Args,pszFormat);
	Length=_vsntprintf(szText,sizeof(szText)/sizeof(TCHAR),pszFormat,Args);
	va_end(Args);
	m_pszText=new TCHAR[Length+1];
	::lstrcpy(m_pszText,szText);
	m_ErrorCode=0;
}


CBonException::CBonException(int ErrorCode,LPCTSTR pszFormat, ...)
{
	va_list Args;
	TCHAR szText[1024];
	int Length;

	va_start(Args,pszFormat);
	Length=_vsntprintf(szText,sizeof(szText)/sizeof(TCHAR),pszFormat,Args);
	va_end(Args);
	m_pszText=new TCHAR[Length+1];
	::lstrcpy(m_pszText,szText);
	m_ErrorCode=ErrorCode;
}
*/


CBonException::CBonException()
	: m_pszText(NULL)
	, m_pszAdvise(NULL)
	, m_pszSystemMessage(NULL)
	, m_ErrorCode(0)
{
}


CBonException::CBonException(LPCTSTR pszText,LPCTSTR pszAdvise,LPCTSTR pszSystemMessage)
	: m_pszText(NULL)
	, m_pszAdvise(NULL)
	, m_pszSystemMessage(NULL)
	, m_ErrorCode(0)
{
	SetText(pszText);
	SetAdvise(pszAdvise);
	SetSystemMessage(pszSystemMessage);
}


CBonException::CBonException(int ErrorCode,LPCTSTR pszText,LPCTSTR pszAdvise,LPCTSTR pszSystemMessage)
	: m_pszText(NULL)
	, m_pszAdvise(NULL)
	, m_pszSystemMessage(NULL)
	, m_ErrorCode(ErrorCode)
{
	SetText(pszText);
	SetAdvise(pszAdvise);
	SetSystemMessage(pszSystemMessage);
}


CBonException::CBonException(const CBonException &Exception)
	: m_pszText(NULL)
	, m_pszAdvise(NULL)
	, m_pszSystemMessage(NULL)
{
	*this=Exception;
}


CBonException::~CBonException()
{
	Clear();
}


CBonException &CBonException::operator=(const CBonException &Exception)
{
	if (&Exception!=this) {
		SetText(Exception.m_pszText);
		SetAdvise(Exception.m_pszAdvise);
		SetSystemMessage(Exception.m_pszSystemMessage);
		m_ErrorCode=Exception.m_ErrorCode;
	}
	return *this;
}


void CBonException::SetText(LPCTSTR pszText)
{
	if (m_pszText!=NULL) {
		delete [] m_pszText;
		m_pszText=NULL;
	}
	if (pszText!=NULL) {
		try {
			m_pszText=StdUtil::strdup(pszText);
		} catch (std::bad_alloc&) {
			//m_pszText=NULL;
		}
	}
}


void CBonException::SetAdvise(LPCTSTR pszAdvise)
{
	if (m_pszAdvise!=NULL) {
		delete [] m_pszAdvise;
		m_pszAdvise=NULL;
	}
	if (pszAdvise!=NULL) {
		try {
			m_pszAdvise=StdUtil::strdup(pszAdvise);
		} catch (std::bad_alloc&) {
			//m_pszAdvise=NULL;
		}
	}
}


void CBonException::SetSystemMessage(LPCTSTR pszSystemMessage)
{
	if (m_pszSystemMessage!=NULL) {
		delete [] m_pszSystemMessage;
		m_pszSystemMessage=NULL;
	}
	if (pszSystemMessage!=NULL) {
		try {
			m_pszSystemMessage=StdUtil::strdup(pszSystemMessage);
		} catch (std::bad_alloc&) {
			//m_pszSystemMessage=NULL;
		}
	}
}


void CBonException::Clear()
{
	if (m_pszText!=NULL) {
		delete [] m_pszText;
		m_pszText=NULL;
	}
	if (m_pszAdvise!=NULL) {
		delete [] m_pszAdvise;
		m_pszAdvise=NULL;
	}
	if (m_pszSystemMessage!=NULL) {
		delete [] m_pszSystemMessage;
		m_pszSystemMessage=NULL;
	}
	m_ErrorCode=0;
}




CBonErrorHandler::CBonErrorHandler()
{
}


CBonErrorHandler::CBonErrorHandler(const CBonErrorHandler &ErrorHandler)
	: m_Exception(ErrorHandler.m_Exception)
{
}


CBonErrorHandler::~CBonErrorHandler()
{
}


CBonErrorHandler &CBonErrorHandler::operator=(const CBonErrorHandler &ErrorHandler)
{
	if (&ErrorHandler!=this)
		m_Exception=ErrorHandler.m_Exception;
	return *this;
}


void CBonErrorHandler::SetErrorText(LPCTSTR pszText)
{
	m_Exception.SetText(pszText);
}


void CBonErrorHandler::SetErrorAdvise(LPCTSTR pszAdvise)
{
	m_Exception.SetAdvise(pszAdvise);
}


void CBonErrorHandler::SetErrorSystemMessage(LPCTSTR pszSystemMessage)
{
	m_Exception.SetSystemMessage(pszSystemMessage);
}


void CBonErrorHandler::SetErrorSystemMessageByErrorCode(DWORD ErrorCode)
{
	TCHAR szText[256];

	if (::FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
			ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			szText, _countof(szText), NULL) > 0) {
		m_Exception.SetSystemMessage(szText);
	} else {
		m_Exception.SetSystemMessage(NULL);
	}
}


void CBonErrorHandler::SetErrorCode(int ErrorCode)
{
	m_Exception.m_ErrorCode=ErrorCode;
}


void CBonErrorHandler::SetError(int ErrorCode,LPCTSTR pszText,LPCTSTR pszAdvise,LPCTSTR pszSystemMessage)
{
	m_Exception.m_ErrorCode=ErrorCode;
	m_Exception.SetText(pszText);
	m_Exception.SetAdvise(pszAdvise);
	m_Exception.SetSystemMessage(pszSystemMessage);
}


void CBonErrorHandler::SetError(LPCTSTR pszText,LPCTSTR pszAdvise,LPCTSTR pszSystemMessage)
{
	m_Exception.m_ErrorCode=0;
	m_Exception.SetText(pszText);
	m_Exception.SetAdvise(pszAdvise);
	m_Exception.SetSystemMessage(pszSystemMessage);
}


void CBonErrorHandler::SetError(const CBonException &Exception)
{
	m_Exception=Exception;
}


void CBonErrorHandler::ClearError()
{
	m_Exception.Clear();
}


LPCTSTR CBonErrorHandler::GetLastErrorText() const
{
	return m_Exception.GetText();
}


LPCTSTR CBonErrorHandler::GetLastErrorAdvise() const
{
	return m_Exception.GetAdvise();
}


LPCTSTR CBonErrorHandler::GetLastErrorSystemMessage() const
{
	return m_Exception.GetSystemMessage();
}


int CBonErrorHandler::GetLastErrorCode() const
{
	return m_Exception.GetErrorCode();
}


void CBonErrorHandler::FormatLastErrorText(LPTSTR pszText,int MaxLength,LPCTSTR pszLead) const
{
	StdUtil::snprintf(pszText,MaxLength,TEXT("%s%s%s%s"),
					  pszLead!=NULL?pszLead:TEXT(""),
					  m_Exception.GetText()!=NULL?m_Exception.GetText():TEXT(""),
					  m_Exception.GetText()!=NULL?TEXT("\n"):TEXT(""),
					  m_Exception.GetAdvise()!=NULL?m_Exception.GetAdvise():TEXT(""));
}
