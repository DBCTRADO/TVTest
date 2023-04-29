/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef TVTEST_CAPTION_PANEL_H
#define TVTEST_CAPTION_PANEL_H


#include <deque>
#include <map>
#include "LibISDB/LibISDB/Filters/CaptionFilter.hpp"
#include "LibISDB/LibISDB/Utilities/MD5.hpp"
#include "PanelForm.h"
#include "UIBase.h"
#include "Settings.h"
#include "DrawUtil.h"
#include "WindowUtil.h"


namespace TVTest
{

	class CCaptionDRCSMap
		: public LibISDB::CaptionFilter::DRCSMap
	{
		struct MD5Less
		{
			bool operator()(const LibISDB::MD5Value &Key1, const LibISDB::MD5Value &Key2) const
			{
				return std::memcmp(Key1.Value, Key2.Value, 16) < 0;
			}
		};
		typedef std::map<LibISDB::MD5Value, String, MD5Less> HashMap;
		typedef std::map<WORD, String> CodeMap;

		typedef LibISDB::CaptionParser::DRCSBitmap DRCSBitmap;

		HashMap m_HashMap;
		CodeMap m_CodeMap;
		bool m_fSaveBMP = false;
		bool m_fSaveRaw = false;
		CFilePath m_SaveDirectory;
		MutexLock m_Lock;

		static bool SaveBMP(const DRCSBitmap *pBitmap, LPCTSTR pszFileName);
		static bool SaveRaw(const DRCSBitmap *pBitmap, LPCTSTR pszFileName);

	// LibISDB::CaptionFilter::DRCSMap
		LPCTSTR GetString(uint16_t Code) override;
		bool SetDRCS(uint16_t Code, const DRCSBitmap *pBitmap) override;

	public:
		void Clear();
		void Reset();
		bool Load(LPCTSTR pszFileName);
	};

	class CCaptionPanel
		: public CPanelForm::CPage
		, protected LibISDB::CaptionFilter::Handler
		, public CSettingsBase
	{
	public:
		CCaptionPanel();
		~CCaptionPanel();

	// CBasicWindow
		bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) override;

	// CUIBase
		void SetTheme(const Theme::CThemeManager *pThemeManager) override;

	// CPage
		bool SetFont(const Style::Font &Font) override;

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CCaptionPanel
		void SetColor(COLORREF BackColor, COLORREF TextColor);
		void Reset();
		bool LoadDRCSMap(LPCTSTR pszFileName);

		static bool Initialize(HINSTANCE hinst);

	private:
		class CEditSubclass
			: public CWindowSubclass
		{
		public:
			CEditSubclass(CCaptionPanel *pCaptionPanel);

		private:
			LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

			CCaptionPanel *m_pCaptionPanel;
		};

		struct LanguageInfo
		{
			std::deque<String> CaptionList;
			String NextCaption;
			DWORD LanguageCode;
			bool fClearLast;
			bool fContinue;
		};
		static constexpr size_t MAX_QUEUE_TEXT = 10000;

		enum class CharEncoding {
			UTF16,
			UTF8,
			Shift_JIS,
			TVTEST_ENUM_CLASS_TRAILER
		};

		COLORREF m_BackColor = RGB(0, 0, 0);
		COLORREF m_TextColor = RGB(255, 255, 255);
		DrawUtil::CBrush m_BackBrush;
		Style::Font m_CaptionFont;
		DrawUtil::CFont m_Font;
		HWND m_hwndEdit = nullptr;
		CEditSubclass m_EditSubclass{this};
		bool m_fActive = false;
		bool m_fEnable = true;
		bool m_fAutoScroll = true;
		bool m_fIgnoreSmall = true;
		bool m_fHalfWidthAlnum = true;
		bool m_fHalfWidthEuroLanguagesOnly = true;
		BYTE m_CurLanguage = 0;
		std::vector<LanguageInfo> m_LanguageList;
		MutexLock m_Lock;
		CCaptionDRCSMap m_DRCSMap;
		CharEncoding m_SaveCharEncoding = CharEncoding::UTF16;

	// LibISDB::CaptionFilter::Handler
		virtual void OnLanguageUpdate(LibISDB::CaptionFilter *pFilter, LibISDB::CaptionParser *pParser) override;
		virtual void OnCaption(
			LibISDB::CaptionFilter *pFilter, LibISDB::CaptionParser *pParser,
			uint8_t Language, const LibISDB::CharType *pText,
			const LibISDB::ARIBStringDecoder::FormatList *pFormatList) override;

		void ClearCaptionList();
		void AppendText(LPCTSTR pszText);
		void AppendQueuedText(BYTE Language);
		void AddNextCaption(BYTE Language);
		bool SetLanguage(BYTE Language);
		void OnCommand(int Command);

		static const LPCTSTR m_pszClassName;
		static HINSTANCE m_hinst;

	// CCustomWindow
		LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	// CUIBase
		void ApplyStyle() override;
		void RealizeStyle() override;

	// CPanelForm::CPage
		void OnActivate() override;
		void OnDeactivate() override;

	};

}	// namespace TVTest


#endif
