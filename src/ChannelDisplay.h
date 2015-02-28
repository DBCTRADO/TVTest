#ifndef CHANNEL_DISPLAY_H
#define CHANNEL_DISPLAY_H


#include <vector>
#include "View.h"
#include "DriverManager.h"
#include "EpgProgramList.h"
#include "LogoManager.h"
#include "DrawUtil.h"
#include "Theme.h"
#include "WindowUtil.h"
#include "GUIUtil.h"


class CChannelDisplay : public CDisplayView
{
public:
	class ABSTRACT_CLASS(CChannelDisplayEventHandler) : public CDisplayView::CEventHandler
	{
	protected:
		class CChannelDisplay *m_pChannelDisplay;
	public:
		enum {
			SPACE_NOTSPECIFIED	=-2,
			SPACE_ALL			=-1
		};
		CChannelDisplayEventHandler() : m_pChannelDisplay(NULL) {}
		virtual void OnTunerSelect(LPCTSTR pszDriverFileName,int TuningSpace)=0;
		virtual void OnChannelSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)=0;
		virtual void OnClose()=0;
		friend class CChannelDisplay;
	};

	CChannelDisplay(CEpgProgramList *pEpgProgramList);
	~CChannelDisplay();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CDisplayView
	bool Close() override;
	bool IsMessageNeed(const MSG *pMsg) const override;
	bool OnMouseWheel(UINT Msg,WPARAM wParam,LPARAM lParam) override;

// CChannelDisplay
	void Clear();
	bool SetDriverManager(CDriverManager *pDriverManager);
	void SetLogoManager(CLogoManager *pLogoManager);
	void SetEventHandler(CChannelDisplayEventHandler *pEventHandler);
	bool SetSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo);
	bool SetFont(const LOGFONT *pFont,bool fAutoSize);

	static bool Initialize(HINSTANCE hinst);

private:
	class CTuner {
	public:
		class CChannel : public CChannelInfo {
			HBITMAP m_hbmSmallLogo;
			HBITMAP m_hbmBigLogo;
			CEventInfoData m_Event[2];
		public:
			CChannel(const CChannelInfo &Info)
				: CChannelInfo(Info)
				, m_hbmSmallLogo(NULL)
				, m_hbmBigLogo(NULL)
			{
			}
			void SetSmallLogo(HBITMAP hbm) { m_hbmSmallLogo=hbm; }
			HBITMAP GetSmallLogo() const { return m_hbmSmallLogo; }
			void SetBigLogo(HBITMAP hbm) { m_hbmBigLogo=hbm; }
			HBITMAP GetBigLogo() const { return m_hbmBigLogo; }
			bool HasLogo() const { return m_hbmSmallLogo!=NULL || m_hbmBigLogo!=NULL; }
			bool SetEvent(int Index,const CEventInfoData *pEvent);
			const CEventInfoData *GetEvent(int Index) const;
		};
		CTuner(const CDriverInfo *pDriverInfo);
		~CTuner();
		void Clear();
		LPCTSTR GetDriverFileName() const { return m_DriverFileName.c_str(); }
		LPCTSTR GetTunerName() const { return m_TunerName.c_str(); }
		LPCTSTR GetDisplayName() const;
		void SetDisplayName(LPCTSTR pszName);
		int NumSpaces() const;
		CTuningSpaceInfo *GetTuningSpaceInfo(int Index);
		const CTuningSpaceInfo *GetTuningSpaceInfo(int Index) const;
		void SetIcon(HICON hico);
		HICON GetIcon() const { return m_Icon; }
	private:
		std::vector<CTuningSpaceInfo*> m_TuningSpaceList;
		TVTest::String m_DriverFileName;
		TVTest::String m_TunerName;
		TVTest::String m_DisplayName;
		TVTest::CIcon m_Icon;
	};

	TVTest::Theme::BackgroundStyle m_TunerAreaBackStyle;
	TVTest::Theme::BackgroundStyle m_ChannelAreaBackStyle;
	TVTest::Theme::Style m_TunerItemStyle;
	TVTest::Theme::Style m_TunerItemSelStyle;
	TVTest::Theme::Style m_TunerItemCurStyle;
	TVTest::Theme::Style m_ChannelItemStyle[2];
	TVTest::Theme::Style m_ChannelItemCurStyle;
	TVTest::Theme::Style m_ClockStyle;
	DrawUtil::CFont m_Font;
	bool m_fAutoFontSize;
	int m_FontHeight;
	int m_TunerItemWidth;
	int m_TunerItemHeight;
	int m_TunerAreaWidth;
	int m_TunerItemLeft;
	int m_TunerItemTop;
	int m_ChannelItemWidth;
	int m_ChannelItemHeight;
	int m_ChannelItemLeft;
	int m_ChannelItemTop;
	int m_ChannelNameWidth;
	int m_VisibleTunerItems;
	int m_TunerScrollPos;
	int m_VisibleChannelItems;
	int m_ChannelScrollPos;
	HWND m_hwndTunerScroll;
	HWND m_hwndChannelScroll;
	CMouseWheelHandler m_MouseWheel;
	SYSTEMTIME m_EpgBaseTime;
	SYSTEMTIME m_ClockTime;
	enum { TIMER_CLOCK=1 };

	std::vector<CTuner*> m_TunerList;
	int m_TotalTuningSpaces;
	int m_CurTuner;
	int m_CurChannel;
	CEpgProgramList *m_pEpgProgramList;
	CLogoManager *m_pLogoManager;
	CChannelDisplayEventHandler *m_pChannelDisplayEventHandler;
	POINT m_LastCursorPos;

	struct TunerInfo {
		TCHAR DriverMasks[MAX_PATH];
		TCHAR szDisplayName[64];
		TCHAR szIconFile[MAX_PATH];
		int Index;
		bool fUseDriverChannel;
	};
	std::vector<TunerInfo> m_TunerInfoList;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

	void LoadSettings();
	void Layout();
	const CTuningSpaceInfo *GetTuningSpaceInfo(int Index) const;
	CTuningSpaceInfo *GetTuningSpaceInfo(int Index);
	const CTuner *GetTuner(int Index,int *pSpace=NULL) const;
	void GetTunerItemRect(int Index,RECT *pRect) const;
	void GetChannelItemRect(int Index,RECT *pRect) const;
	void UpdateTunerItem(int Index) const;
	void UpdateChannelItem(int Index) const;
	int TunerItemHitTest(int x,int y) const;
	int ChannelItemHitTest(int x,int y) const;
	bool SetCurTuner(int Index,bool fUpdate=false);
	bool UpdateChannelInfo(int Index);
	bool SetCurChannel(int Index);
	void SetTunerScrollPos(int Pos,bool fScroll);
	void SetChannelScrollPos(int Pos,bool fScroll);
	void Draw(HDC hdc,const RECT *pPaintRect);
	void DrawClock(HDC hdc) const;
	void NotifyTunerSelect() const;
	void NotifyChannelSelect() const;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
