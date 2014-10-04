#ifndef INFORMATION_PANEL_H
#define INFORMATION_PANEL_H


#include "PanelForm.h"
#include "UIBase.h"
#include "Settings.h"
#include "DrawUtil.h"
#include "RichEditUtil.h"


class CInformationPanel
	: public CPanelForm::CPage
	, public TVTest::CUIBase
	, public CSettingsBase
{
public:
	enum {
		ITEM_VIDEOINFO,
		ITEM_VIDEODECODER,
		ITEM_VIDEORENDERER,
		ITEM_AUDIODEVICE,
		ITEM_SIGNALLEVEL,
		ITEM_MEDIABITRATE,
		ITEM_ERROR,
		ITEM_RECORD,
		ITEM_SERVICE,
		ITEM_PROGRAMINFO,
		NUM_ITEMS,
	};

	class CItem
	{
	public:
		CItem(CInformationPanel *pPanel,bool fVisible);
		virtual ~CItem() {}
		virtual int GetID() const = 0;
		virtual LPCTSTR GetName() const = 0;
		virtual void Reset() {}
		virtual bool Update() = 0;
		virtual void Draw(HDC hdc,const RECT &Rect) {}
		bool IsVisible() const { return m_fVisible; }
		void SetVisible(bool fVisible) { m_fVisible=fVisible; }

	protected:
		void DrawItem(HDC hdc,const RECT &Rect,LPCTSTR pszText);
		void UpdateItem();

		CInformationPanel *m_pPanel;
		bool m_fVisible;
	};

	template<int id> class CItemTemplate : public CItem
	{
	public:
		static const int ID=id;
		CItemTemplate(CInformationPanel *pPanel,bool fVisible) : CItem(pPanel,fVisible) {}
		int GetID() const override { return id; }
	};

	class CVideoInfoItem : public CItemTemplate<ITEM_VIDEOINFO>
	{
	public:
		CVideoInfoItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("VideoInfo"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		int m_OriginalVideoWidth;
		int m_OriginalVideoHeight;
		int m_DisplayVideoWidth;
		int m_DisplayVideoHeight;
		int m_AspectX;
		int m_AspectY;
	};

	class CVideoDecoderItem : public CItemTemplate<ITEM_VIDEODECODER>
	{
	public:
		CVideoDecoderItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("VideoDecoder"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		TVTest::String m_VideoDecoderName;
	};

	class CVideoRendererItem : public CItemTemplate<ITEM_VIDEORENDERER>
	{
	public:
		CVideoRendererItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("VideoRenderer"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		TVTest::String m_VideoRendererName;
	};

	class CAudioDeviceItem : public CItemTemplate<ITEM_AUDIODEVICE>
	{
	public:
		CAudioDeviceItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("AudioDevice"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		TVTest::String m_AudioDeviceName;
	};

	class CSignalLevelItem : public CItemTemplate<ITEM_SIGNALLEVEL>
	{
	public:
		CSignalLevelItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("SignalLevel"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;
		void ShowSignalLevel(bool fShow);

	private:
		bool m_fShowSignalLevel;
		float m_SignalLevel;
		DWORD m_BitRate;
	};

	class CMediaBitRateItem : public CItemTemplate<ITEM_MEDIABITRATE>
	{
	public:
		CMediaBitRateItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("MediaBitrate"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		DWORD m_VideoBitRate;
		DWORD m_AudioBitRate;
	};

	class CErrorItem : public CItemTemplate<ITEM_ERROR>
	{
	public:
		CErrorItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("Error"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		bool m_fShowScramble;
		ULONGLONG m_ErrorPacketCount;
		ULONGLONG m_ContinuityErrorPacketCount;
		ULONGLONG m_ScramblePacketCount;
	};

	class CRecordItem : public CItemTemplate<ITEM_RECORD>
	{
	public:
		CRecordItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("Record"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		bool m_fRecording;
		LONGLONG m_WroteSize;
		unsigned int m_RecordTime;
		LONGLONG m_DiskFreeSpace;
	};

	class CServiceItem : public CItemTemplate<ITEM_SERVICE>
	{
	public:
		CServiceItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("Service"); }
		void Reset() override;
		bool Update() override;
		void Draw(HDC hdc,const RECT &Rect) override;

	private:
		TVTest::String m_ServiceName;
	};

	class CProgramInfoItem : public CItemTemplate<ITEM_PROGRAMINFO>
	{
	public:
		CProgramInfoItem(CInformationPanel *pPanel,bool fVisible);
		LPCTSTR GetName() const override { return TEXT("ProgramInfo"); }
		void Reset() override;
		bool Update() override;
		const TVTest::String &GetInfoText() const { return m_InfoText; }
		void SetNext(bool fNext);
		bool IsNext() const { return m_fNext; }

	private:
		TVTest::String m_InfoText;
		bool m_fNext;
	};

	static bool Initialize(HINSTANCE hinst);

	CInformationPanel();
	~CInformationPanel();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CUIBase
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CInformationPanel
	bool IsVisible() const;
	void SetColor(COLORREF crBackColor,COLORREF crTextColor);
	void SetProgramInfoColor(COLORREF crBackColor,COLORREF crTextColor);
	bool SetFont(const LOGFONT *pFont);
	CItem *GetItem(int Item);
	const CItem *GetItem(int Item) const;
	template<typename T> T *GetItem() { return static_cast<T*>(GetItem(T::ID)); }
	template<typename T> const T *GetItem() const { return static_cast<const T*>(GetItem(T::ID)); }
	bool SetItemVisible(int Item,bool fVisible);
	bool IsItemVisible(int Item) const;
	bool UpdateItem(int Item);
	void RedrawItem(int Item);
	bool ResetItem(int Item);
	bool UpdateAllItems();
	bool SetProgramInfoRichEdit(bool fRichEdit);

private:
	struct InformationPanelStyle
	{
		TVTest::Style::Size ButtonSize;
		TVTest::Style::IntValue LineSpacing;

		InformationPanelStyle();
		void SetStyle(const TVTest::Style::CStyleManager *pStyleManager);
		void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager);
	};

	template<typename T> void RegisterItem(bool fVisible=true)
	{
		T *pItem=new T(this,fVisible);
		m_ItemList[pItem->GetID()]=pItem;
	}

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	CItem *m_ItemList[NUM_ITEMS];
	InformationPanelStyle m_Style;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	COLORREF m_crProgramInfoBackColor;
	COLORREF m_crProgramInfoTextColor;
	DrawUtil::CBrush m_BackBrush;
	DrawUtil::CBrush m_ProgramInfoBackBrush;
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	DrawUtil::COffscreen m_Offscreen;

	HWND m_hwndProgramInfo;
	WNDPROC m_pOldProgramInfoProc;
	HWND m_hwndProgramInfoPrev;
	HWND m_hwndProgramInfoNext;
	CRichEditUtil m_RichEditUtil;
	bool m_fUseRichEdit;
	CRichEditUtil::CharRangeList m_ProgramInfoLinkList;
	POINT m_ProgramInfoClickPos;
	bool m_fProgramInfoCursorOverLink;

	static LRESULT CALLBACK ProgramInfoHookProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void UpdateProgramInfoText();
	bool CreateProgramInfoEdit();
	void OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify);
	void GetItemRect(int Item,RECT *pRect) const;
	void CalcFontHeight();
	void Draw(HDC hdc,const RECT &PaintRect);
	bool GetDrawItemRect(int Item,RECT *pRect,const RECT &PaintRect) const;
	void DrawItem(HDC hdc,LPCTSTR pszText,const RECT &Rect);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
