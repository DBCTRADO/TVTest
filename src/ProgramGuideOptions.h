#ifndef PROGRAM_GUIDE_OPTIONS_H
#define PROGRAM_GUIDE_OPTIONS_H


#include "ProgramGuide.h"
#include "Options.h"
#include "Plugin.h"
#include "Tooltip.h"


class CProgramGuideOptions
	: public COptions
{
public:
	enum {
		UPDATE_EVENTICONS	= 0x00000001UL
	};

	CProgramGuideOptions(CProgramGuide *pProgramGuide, CPluginManager *pPluginManager);
	~CProgramGuideOptions();

// CSettingsBase
	bool LoadSettings(CSettings &Settings) override;
	bool SaveSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CProgramGuideOptions
	bool GetTimeRange(LibISDB::DateTime *pFirst, LibISDB::DateTime *pLast);
	bool GetOnScreen() const { return m_fOnScreen; }
	bool ScrollToCurChannel() const { return m_fScrollToCurChannel; }
	const TVTest::Style::Font &GetFont() const { return m_Font; }
	UINT GetVisibleEventIcons() const { return m_VisibleEventIcons; }
	LPCTSTR GetProgramLDoubleClickCommand() const { return TVTest::StringUtility::GetCStrOrNull(m_ProgramLDoubleClickCommand); }
	int ParseCommand(LPCTSTR pszCommand) const;

private:
	static constexpr int MIN_VIEW_HOURS = 1;
	static constexpr int MAX_VIEW_HOURS = 24 * 8;

	CProgramGuide *m_pProgramGuide;
	CPluginManager *m_pPluginManager;
	bool m_fOnScreen;
	bool m_fScrollToCurChannel;
	int m_BeginHour;
	int m_ViewHours;
	int m_ItemWidth;
	int m_LinesPerHour;
	TVTest::Style::Font m_Font;
	TVTest::Style::Font m_CurSettingFont;
	bool m_fUseDirectWrite;
	TVTest::CDirectWriteRenderer::RenderingParams m_DirectWriteRenderingParams;
	UINT m_VisibleEventIcons;
	CProgramGuideToolList m_ToolList;
	int m_WheelScrollLines;
	TVTest::String m_ProgramLDoubleClickCommand;
	CTooltip m_Tooltip;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

// CUIBase
	void RealizeStyle() override;

	void SetDlgItemState();
	void DeleteAllTools();
};


#endif
