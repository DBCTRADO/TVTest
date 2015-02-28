#ifndef STATUS_ITEMS_H
#define STATUS_ITEMS_H


#include "StatusView.h"
#include "Tooltip.h"
#include "EventInfoPopup.h"
#include "EpgUtil.h"


enum {
	STATUS_ITEM_CHANNEL,
	STATUS_ITEM_VIDEOSIZE,
	STATUS_ITEM_VOLUME,
	STATUS_ITEM_AUDIOCHANNEL,
	STATUS_ITEM_RECORD,
	STATUS_ITEM_CAPTURE,
	STATUS_ITEM_ERROR,
	STATUS_ITEM_SIGNALLEVEL,
	STATUS_ITEM_CLOCK,
	STATUS_ITEM_PROGRAMINFO,
	STATUS_ITEM_BUFFERING,
	STATUS_ITEM_TUNER,
	STATUS_ITEM_MEDIABITRATE,
	STATUS_ITEM_FAVORITES
};
#define STATUS_ITEM_FIRST	STATUS_ITEM_CHANNEL
#define STATUS_ITEM_LAST	STATUS_ITEM_FAVORITES


class CChannelStatusItem : public CStatusItem
{
public:
	CChannelStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Channel"); }
	LPCTSTR GetName() const override { return TEXT("�`�����l��"); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;
	bool OnMouseWheel(int x,int y,bool fHorz,int Delta,int *pCommand) override;
};

class CVideoSizeStatusItem : public CStatusItem
{
public:
	CVideoSizeStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Video"); }
	LPCTSTR GetName() const override { return TEXT("�f���T�C�Y"); }
	bool UpdateContent() override;
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;

private:
	int m_OriginalVideoWidth;
	int m_OriginalVideoHeight;
	int m_ZoomPercentage;
};

class CVolumeStatusItem : public CStatusItem
{
public:
	CVolumeStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Volume"); }
	LPCTSTR GetName() const override { return TEXT("����"); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;
	void OnMouseMove(int x,int y) override;
	bool OnMouseWheel(int x,int y,bool fHorz,int Delta,int *pCommand) override;
	void SetStyle(const TVTest::Style::CStyleManager *pStyleManager) override;
	void NormalizeStyle(const TVTest::Style::CStyleManager *pStyleManager) override;

private:
	TVTest::Style::IntValue m_BarHeight;
	TVTest::Style::Margins m_BarPadding;
	TVTest::Style::IntValue m_BarBorderWidth;
};

class CAudioChannelStatusItem : public CStatusItem
{
public:
	CAudioChannelStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Audio"); }
	LPCTSTR GetName() const override { return TEXT("����"); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;
	bool OnMouseWheel(int x,int y,bool fHorz,int Delta,int *pCommand) override;

private:
	DrawUtil::CMonoColorIconList m_Icons;
};

class CRecordStatusItem : public CStatusItem
{
	COLORREF m_CircleColor;
	bool m_fRemain;
	CTooltip m_Tooltip;

	int GetTipText(LPTSTR pszText,int MaxLength);

public:
	CRecordStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Record"); }
	LPCTSTR GetName() const override { return TEXT("�^��"); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;
	bool OnMouseHover(int x,int y) override;
	void OnFocus(bool fFocus) override;
	LRESULT OnNotifyMessage(LPNMHDR pnmh) override;
// CUIBase
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;
// CRecordStatusItem
	void ShowRemainTime(bool fRemain);
	void SetCircleColor(COLORREF Color);
};

class CCaptureStatusItem : public CIconStatusItem
{
public:
	CCaptureStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Capture"); }
	LPCTSTR GetName() const override { return TEXT("�L���v�`��"); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;

private:
	DrawUtil::CMonoColorIconList m_Icons;
};

class CErrorStatusItem : public CStatusItem
{
public:
	CErrorStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Error"); }
	LPCTSTR GetName() const override { return TEXT("�G���["); }
	bool UpdateContent() override;
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;

private:
	ULONGLONG m_ContinuityErrorPacketCount;
	ULONGLONG m_ErrorPacketCount;
	ULONGLONG m_ScramblePacketCount;
};

class CSignalLevelStatusItem : public CStatusItem
{
public:
	CSignalLevelStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Signal"); }
	LPCTSTR GetName() const override { return TEXT("�M�����x��"); }
	bool UpdateContent() override;
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
// CSignalLevelStatusItem
	void ShowSignalLevel(bool fShow);

private:
	bool m_fShowSignalLevel;
	float m_SignalLevel;
	DWORD m_BitRate;
};

class CClockStatusItem : public CStatusItem
{
public:
	CClockStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Clock"); }
	LPCTSTR GetName() const override { return TEXT("���v"); }
	bool UpdateContent() override;
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;
// CClockStatusItem
	void SetTOT(bool fTOT);
	bool IsTOT() const { return m_fTOT; }

private:
	void FormatTime(const SYSTEMTIME &Time,LPTSTR pszText,int MaxLength) const;

	bool m_fTOT;
	SYSTEMTIME m_Time;
};

class CProgramInfoStatusItem : public CStatusItem
{
	bool m_fNext;
	bool m_fShowProgress;
	bool m_fEnablePopupInfo;
	CEventInfoPopup m_EventInfoPopup;
	TVTest::String m_Text;
	CEpgTheme m_EpgTheme;
	TVTest::Theme::BackgroundStyle m_ProgressBackStyle;
	TVTest::Theme::BackgroundStyle m_ProgressElapsedStyle;
	bool m_fValidProgress;
	SYSTEMTIME m_EventStartTime;
	DWORD m_EventDuration;
	SYSTEMTIME m_CurTime;

	void ShowPopupInfo();

public:
	CProgramInfoStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Program"); }
	LPCTSTR GetName() const override { return TEXT("�ԑg���"); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	bool UpdateContent() override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;
	void OnLButtonDoubleClick(int x,int y) override;
	void OnFocus(bool fFocus) override;
	bool OnMouseHover(int x,int y) override;
// CUIBase
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;
// CProgramInfoStatusItem
	void SetShowProgress(bool fShow);
	bool GetShowProgress() const { return m_fShowProgress; }
	void EnablePopupInfo(bool fEnable);
	bool UpdateProgress();
	bool SetEventInfoFont(const LOGFONT *pFont) {
		return m_EventInfoPopup.SetFont(pFont);
	}
};

class CBufferingStatusItem : public CStatusItem
{
public:
	CBufferingStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Buffering"); }
	LPCTSTR GetName() const override { return TEXT("�o�b�t�@�����O"); }
	bool UpdateContent() override;
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;

private:
	DWORD m_StreamRemain;
	int m_PacketBufferUsedPercentage;
};

class CTunerStatusItem : public CStatusItem
{
public:
	CTunerStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Tuner"); }
	LPCTSTR GetName() const override { return TEXT("�`���[�i�["); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;
};

class CMediaBitRateStatusItem : public CStatusItem
{
public:
	CMediaBitRateStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Bitrate"); }
	LPCTSTR GetName() const override { return TEXT("�r�b�g���[�g"); }
	bool UpdateContent() override;
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;

private:
	DWORD m_VideoBitRate;
	DWORD m_AudioBitRate;
};

class CFavoritesStatusItem : public CIconStatusItem
{
public:
	CFavoritesStatusItem();
// CStatusItem
	LPCTSTR GetIDText() const override { return TEXT("Favorites"); }
	LPCTSTR GetName() const override { return TEXT("���C�ɓ���"); }
	void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
	void OnLButtonDown(int x,int y) override;
	void OnRButtonDown(int x,int y) override;

private:
	DrawUtil::CMonoColorIconList m_Icons;
};


#endif
