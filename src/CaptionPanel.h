#ifndef CAPTION_PANEL_H
#define CAPTION_PANEL_H


#include <deque>
#include <map>
#include "BonTsEngine/CaptionDecoder.h"
#include "PanelForm.h"
#include "DrawUtil.h"


class CCaptionDRCSMap : public CCaptionDecoder::IDRCSMap
{
	class CHashCmp {
	public:
		bool operator()(const PBYTE &Key1,const PBYTE &Key2) const {
			return memcmp(Key1,Key2,16)<0;
		}
	};
	typedef std::map<PBYTE,LPCTSTR,CHashCmp> HashMap;
	typedef std::map<WORD,LPCTSTR> CodeMap;
	HashMap m_HashMap;
	CodeMap m_CodeMap;
	LPTSTR m_pBuffer;
	bool m_fSaveBMP;
	bool m_fSaveRaw;
	TCHAR m_szSaveDirectory[MAX_PATH];
	CCriticalLock m_Lock;

	static bool SaveBMP(const CCaptionParser::DRCSBitmap *pBitmap,LPCTSTR pszFileName);
	static bool SaveRaw(const CCaptionParser::DRCSBitmap *pBitmap,LPCTSTR pszFileName);

// CCaptionDecoder::IDRCSMap
	LPCTSTR GetString(WORD Code) override;
	bool SetDRCS(WORD Code, const CCaptionParser::DRCSBitmap *pBitmap) override;

public:
	CCaptionDRCSMap();
	~CCaptionDRCSMap();
	void Clear();
	void Reset();
	bool Load(LPCTSTR pszFileName);
};

class CCaptionPanel : public CPanelForm::CPage, protected CCaptionDecoder::IHandler
{
	COLORREF m_BackColor;
	COLORREF m_TextColor;
	DrawUtil::CBrush m_BackBrush;
	DrawUtil::CFont m_Font;
	HWND m_hwndEdit;
	WNDPROC m_pOldEditProc;
	bool m_fEnable;
	bool m_fAutoScroll;
#ifndef TVH264_FOR_1SEG
	bool m_fIgnoreSmall;
#endif
	BYTE m_Language;
	bool m_fClearLast;
	bool m_fContinue;
	std::deque<LPTSTR> m_CaptionList;
	enum { MAX_QUEUE_TEXT=10000 };
	TVTest::String m_NextCaption;
	CCriticalLock m_Lock;
	CCaptionDRCSMap m_DRCSMap;

// CCaptionDecoder::IHandler
	virtual void OnLanguageUpdate(CCaptionDecoder *pDecoder) override;
	virtual void OnCaption(CCaptionDecoder *pDecoder,BYTE Language,LPCTSTR pszText,
						   const CAribString::FormatList *pFormatList) override;

	void ClearCaptionList();
	void AppendText(LPCTSTR pszText);

	static const LPCTSTR m_pszClassName;
	static const LPCTSTR m_pszPropName;
	static HINSTANCE m_hinst;

	static LRESULT CALLBACK EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

public:
	CCaptionPanel();
	~CCaptionPanel();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
	void SetVisible(bool fVisible) override;
// CCaptionPanel
	void SetColor(COLORREF BackColor,COLORREF TextColor);
	bool SetFont(const LOGFONT *pFont);
	void Clear();
	bool LoadDRCSMap(LPCTSTR pszFileName);

	static bool Initialize(HINSTANCE hinst);
};


#endif
