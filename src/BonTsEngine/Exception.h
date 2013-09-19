#pragma once


class CBonException
{
public:
	CBonException();
	CBonException(LPCTSTR pszText,LPCTSTR pszAdvise=NULL,LPCTSTR pszSystemMessage=NULL);
	CBonException(int ErrorCode,LPCTSTR pszText,LPCTSTR pszAdvise=NULL,LPCTSTR pszSystemMessage=NULL);
	CBonException(const CBonException &Exception);
	virtual ~CBonException();
	CBonException &operator=(const CBonException &Exception);
	LPCTSTR GetText() const { return m_pszText; }
	LPCTSTR GetAdvise() const { return m_pszAdvise; }
	LPCTSTR GetSystemMessage() const { return m_pszSystemMessage; }
	int GetErrorCode() const { return m_ErrorCode; }

	friend class CBonErrorHandler;

protected:
	LPTSTR m_pszText;
	LPTSTR m_pszAdvise;
	LPTSTR m_pszSystemMessage;
	int m_ErrorCode;
	void SetText(LPCTSTR pszText);
	void SetAdvise(LPCTSTR pszAdvise);
	void SetSystemMessage(LPCTSTR pszSystemMessage);
	void Clear();
};

class CBonErrorHandler
{
public:
	CBonErrorHandler();
	CBonErrorHandler(const CBonErrorHandler &ErrorHandler);
	virtual ~CBonErrorHandler();
	CBonErrorHandler &operator=(const CBonErrorHandler &ErrorHandler);
	LPCTSTR GetLastErrorText() const;
	LPCTSTR GetLastErrorAdvise() const;
	LPCTSTR GetLastErrorSystemMessage() const;
	int GetLastErrorCode() const;
	const CBonException &GetLastErrorException() const { return m_Exception; }
	void FormatLastErrorText(LPTSTR pszText,int MaxLength,LPCTSTR pszLead=NULL) const;

protected:
	void SetErrorText(LPCTSTR pszText);
	void SetErrorAdvise(LPCTSTR pszAdvise);
	void SetErrorSystemMessage(LPCTSTR pszSystemMessage);
	void SetErrorSystemMessageByErrorCode(DWORD ErrorCode);
	void SetErrorCode(int ErrorCode);
	void SetError(int ErrorCode,LPCTSTR pszText,LPCTSTR pszAdvise=NULL,LPCTSTR pszSystemMessage=NULL);
	void SetError(LPCTSTR pszText,LPCTSTR pszAdvise=NULL,LPCTSTR pszSystemMessage=NULL);
	void SetError(const CBonException &Exception);
	void ClearError();

private:
	CBonException m_Exception;
};
