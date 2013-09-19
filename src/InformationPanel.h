#ifndef INFORMATION_PANEL_H
#define INFORMATION_PANEL_H


#include "PanelForm.h"
#include "Settings.h"
#include "DrawUtil.h"
#include "RichEditUtil.h"


class CInformationPanel : public CPanelForm::CPage, public CSettingsBase
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnProgramInfoUpdate(bool fNext) { return false; }
	};

	enum {
		ITEM_VIDEO,
		ITEM_DECODER,
		ITEM_VIDEORENDERER,
		ITEM_AUDIODEVICE,
		ITEM_SIGNALLEVEL,
		ITEM_MEDIABITRATE,
		ITEM_ERROR,
		ITEM_RECORD,
		ITEM_PROGRAMINFO,
		NUM_ITEMS,
	};

	static bool Initialize(HINSTANCE hinst);

	CInformationPanel();
	~CInformationPanel();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CInformationPanel
	void ResetStatistics();
	bool IsVisible() const;
	void SetColor(COLORREF crBackColor,COLORREF crTextColor);
	void SetProgramInfoColor(COLORREF crBackColor,COLORREF crTextColor);
	bool SetFont(const LOGFONT *pFont);
	bool SetItemVisible(int Item,bool fVisible);
	bool IsItemVisible(int Item) const;
	void UpdateItem(int Item);
	void SetVideoSize(int OriginalWidth,int OriginalHeight,int DisplayWidth,int DisplayHeight);
	void SetAspectRatio(int AspectX,int AspectY);
	void SetVideoDecoderName(LPCTSTR pszName);
	void SetVideoRendererName(LPCTSTR pszName);
	void SetAudioDeviceName(LPCTSTR pszName);
	void ShowSignalLevel(bool fShow);
	void SetMediaBitRate(DWORD VideoBitRate,DWORD AudioBitRate);
	void SetRecordStatus(bool fRecording,LPCTSTR pszFileName=NULL,
						 ULONGLONG WroteSize=0,unsigned int RecordTime=0,
						 ULONGLONG FreeSpace=0);
	void SetProgramInfo(LPCTSTR pszInfo);
	bool GetProgramInfoNext() const { return m_fNextProgramInfo; }
	bool SetProgramInfoRichEdit(bool fRichEdit);
	bool SetEventHandler(CEventHandler *pHandler);

private:
	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	HWND m_hwndProgramInfo;
	WNDPROC m_pOldProgramInfoProc;
	HWND m_hwndProgramInfoPrev;
	HWND m_hwndProgramInfoNext;
	CEventHandler *m_pEventHandler;
	CRichEditUtil m_RichEditUtil;
	bool m_fUseRichEdit;

	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	COLORREF m_crProgramInfoBackColor;
	COLORREF m_crProgramInfoTextColor;
	DrawUtil::CBrush m_BackBrush;
	DrawUtil::CBrush m_ProgramInfoBackBrush;
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	int m_LineMargin;
	DrawUtil::COffscreen m_Offscreen;
	unsigned int m_ItemVisibility;

	int m_OriginalVideoWidth;
	int m_OriginalVideoHeight;
	int m_DisplayVideoWidth;
	int m_DisplayVideoHeight;
	int m_AspectX;
	int m_AspectY;
	CDynamicString m_VideoDecoderName;
	CDynamicString m_VideoRendererName;
	CDynamicString m_AudioDeviceName;
	bool m_fSignalLevel;
	DWORD m_VideoBitRate;
	DWORD m_AudioBitRate;
	bool m_fRecording;
	ULONGLONG m_RecordWroteSize;
	unsigned int m_RecordTime;
	ULONGLONG m_DiskFreeSpace;
	CDynamicString m_ProgramInfo;
	CRichEditUtil::CharRangeList m_ProgramInfoLinkList;
	POINT m_ProgramInfoClickPos;
	bool m_fProgramInfoCursorOverLink;
	bool m_fNextProgramInfo;

	static const LPCTSTR m_pszItemNameList[];

	static LRESULT CALLBACK ProgramInfoHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void UpdateProgramInfoText();
	bool CreateProgramInfoEdit();
	void GetItemRect(int Item,RECT *pRect) const;
	void CalcFontHeight();
	void Draw(HDC hdc,const RECT &PaintRect);
	bool GetDrawItemRect(int Item,RECT *pRect,const RECT &PaintRect) const;
	void DrawItem(HDC hdc,LPCTSTR pszText,const RECT &Rect);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
