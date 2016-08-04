#ifndef CAPTION_PANEL_H
#define CAPTION_PANEL_H


#include <deque>
#include <map>
#include "BonTsEngine/CaptionDecoder.h"
#include "PanelForm.h"
#include "UIBase.h"
#include "Settings.h"
#include "DrawUtil.h"
#include "WindowUtil.h"


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

class CCaptionPanel
	: public CPanelForm::CPage
	, protected CCaptionDecoder::IHandler
	, public CSettingsBase
{
public:
	CCaptionPanel();
	~CCaptionPanel();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CUIBase
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CPage
	bool SetFont(const TVTest::Style::Font &Font) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CCaptionPanel
	void SetColor(COLORREF BackColor,COLORREF TextColor);
	void Reset();
	bool LoadDRCSMap(LPCTSTR pszFileName);

	static bool Initialize(HINSTANCE hinst);

private:
	class CEditSubclass : public CWindowSubclass
	{
	public:
		CEditSubclass(CCaptionPanel *pCaptionPanel);

	private:
		LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		CCaptionPanel *m_pCaptionPanel;
	};

	struct LanguageInfo
	{
		std::deque<TVTest::String> CaptionList;
		TVTest::String NextCaption;
		DWORD LanguageCode;
		bool fClearLast;
		bool fContinue;
	};
	enum { MAX_QUEUE_TEXT=10000 };

	enum CharEncoding
	{
		CHARENCODING_UTF16,
		CHARENCODING_UTF8,
		CHARENCODING_SHIFTJIS
	};
	static const CharEncoding CHARENCODING_FIRST = CHARENCODING_UTF16;
	static const CharEncoding CHARENCODING_LAST  = CHARENCODING_SHIFTJIS;

	COLORREF m_BackColor;
	COLORREF m_TextColor;
	DrawUtil::CBrush m_BackBrush;
	TVTest::Style::Font m_CaptionFont;
	DrawUtil::CFont m_Font;
	HWND m_hwndEdit;
	CEditSubclass m_EditSubclass;
	bool m_fActive;
	bool m_fEnable;
	bool m_fAutoScroll;
	bool m_fIgnoreSmall;
	bool m_fHalfWidthAlnum;
	bool m_fHalfWidthEuroLanguagesOnly;
	BYTE m_CurLanguage;
	std::vector<LanguageInfo> m_LanguageList;
	CCriticalLock m_Lock;
	CCaptionDRCSMap m_DRCSMap;
	CharEncoding m_SaveCharEncoding;

// CCaptionDecoder::IHandler
	virtual void OnLanguageUpdate(CCaptionDecoder *pDecoder,CCaptionParser *pParser) override;
	virtual void OnCaption(CCaptionDecoder *pDecoder,CCaptionParser *pParser,
						   BYTE Language,LPCTSTR pszText,
						   const CAribString::FormatList *pFormatList) override;

	void ClearCaptionList();
	void AppendText(LPCTSTR pszText);
	void AppendQueuedText(BYTE Language);
	void AddNextCaption(BYTE Language);
	bool SetLanguage(BYTE Language);
	void OnCommand(int Command);

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

// CUIBase
	void ApplyStyle() override;
	void RealizeStyle() override;

// CPanelForm::CPage
	void OnActivate() override;
	void OnDeactivate() override;

};


#endif
