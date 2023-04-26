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


#ifndef TVTEST_TEXT_DRAW_H
#define TVTEST_TEXT_DRAW_H


#include <vector>
#include <deque>
#include <memory>
#include "DirectWrite.h"


namespace TVTest
{

	class CTextDrawEngine;

	class CTextDraw
	{
	public:
		enum class Flag : unsigned int {
			None               = 0x0000U,
			EndEllipsis        = 0x0001U,	// 収まりきらない場合省略記号を付加
			JapaneseHyphnation = 0x0002U,	// 禁則処理
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class DrawFlag : unsigned int {
			None             = 0x0000U,
			Align_HorzCenter = 0x0001U,
			Align_Right      = 0x0002U,
			Align_Justified  = 0x0004U,
			Align_VertCenter = 0x0008U,
			Align_Bottom     = 0x0010U,
			JustifyMultiLine = 0x0020U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		struct FontMetrics
		{
			int Height;
			int LineGap;
		};

		struct TextMetrics
		{
			int Width;
			int Height;
		};

		CTextDraw() = default;

		CTextDraw(const CTextDraw &) = delete;
		CTextDraw &operator=(const CTextDraw &) = delete;

		bool SetEngine(CTextDrawEngine *pEngine);
		bool Begin(HDC hdc, const RECT &Rect, Flag Flags = Flag::None);
		void End();
		bool BindDC(HDC hdc, const RECT &Rect);
		void SetFlags(Flag Flags) { m_Flags = Flags; }
		Flag GetFlags() const { return m_Flags; }
		bool SetFont(HFONT hfont);
		bool SetFont(const LOGFONT &Font);
		bool SetTextColor(COLORREF Color);
		int CalcLineCount(LPCWSTR pszText, int Width);
		bool Draw(LPCWSTR pszText, const RECT &Rect, int LineHeight, DrawFlag Flags = DrawFlag::None);
		bool GetFontMetrics(FontMetrics *pMetrics);
		bool GetTextMetrics(LPCWSTR pText, int Length, TextMetrics *pMetrics);
		bool SetClippingRect(const RECT &Rect);
		bool ResetClipping();

	private:
		CTextDrawEngine *m_pEngine = nullptr;
		std::unique_ptr<CTextDrawEngine> m_DefaultEngine;
		Flag m_Flags = Flag::None;
		std::vector<WCHAR> m_StringBuffer;

		int GetLineLength(LPCWSTR pszText);
		int AdjustLineLength(LPCWSTR pszText, int Length);
		int GetFitCharCount(LPCWSTR pText, int Length, int Width);

		static const LPCWSTR m_pszStartProhibitChars;
		static const LPCWSTR m_pszEndProhibitChars;

		static bool IsStartProhibitChar(WCHAR Char);
		static bool IsEndProhibitChar(WCHAR Char);
	};

	class CTextDrawEngine
	{
	public:
		typedef CTextDraw::FontMetrics FontMetrics;
		typedef CTextDraw::TextMetrics TextMetrics;

		CTextDrawEngine() = default;
		virtual ~CTextDrawEngine() = default;

		CTextDrawEngine(const CTextDrawEngine &) = delete;
		CTextDrawEngine &operator=(const CTextDrawEngine &) = delete;

		virtual void Finalize();
		virtual bool BeginDraw(HDC hdc, const RECT &Rect);
		virtual bool EndDraw();
		virtual bool BindDC(HDC hdc, const RECT &Rect);
		virtual bool SetFont(HFONT hfont) = 0;
		virtual bool SetTextColor(COLORREF Color) = 0;
		virtual bool DrawText(LPCWSTR pText, int Length, const RECT &Rect, CTextDraw::DrawFlag Flags) = 0;
		virtual int GetFitCharCount(LPCWSTR pText, int Length, int Width) = 0;
		virtual bool GetFontMetrics(FontMetrics *pMetrics) = 0;
		virtual bool GetTextMetrics(LPCWSTR pText, int Length, TextMetrics *pMetrics) = 0;
		virtual bool SetClippingRect(const RECT &Rect);
		virtual bool ResetClipping();
		virtual bool OnWindowPosChanged();
	};

	class CTextDrawEngine_GDI
		: public CTextDrawEngine
	{
	public:
		~CTextDrawEngine_GDI();

		void Finalize() override;
		bool BeginDraw(HDC hdc, const RECT &Rect) override;
		bool EndDraw() override;
		bool BindDC(HDC hdc, const RECT &Rect) override;
		bool SetFont(HFONT hfont) override;
		bool SetTextColor(COLORREF Color) override;
		bool DrawText(LPCWSTR pText, int Length, const RECT &Rect, CTextDraw::DrawFlag Flags) override;
		int GetFitCharCount(LPCWSTR pText, int Length, int Width) override;
		bool GetFontMetrics(FontMetrics *pMetrics) override;
		bool GetTextMetrics(LPCWSTR pText, int Length, TextMetrics *pMetrics) override;

	private:
		void UnbindDC();

		HDC m_hdc = nullptr;
		HFONT m_hfontOld = nullptr;
		COLORREF m_OldTextColor;
	};

	class CTextDrawEngine_DirectWrite
		: public CTextDrawEngine
	{
	public:
		CTextDrawEngine_DirectWrite(CDirectWriteRenderer &Renderer);
		~CTextDrawEngine_DirectWrite();

		void Finalize() override;
		bool BeginDraw(HDC hdc, const RECT &Rect) override;
		bool EndDraw() override;
		bool BindDC(HDC hdc, const RECT &Rect) override;
		bool SetFont(HFONT hfont) override;
		bool SetTextColor(COLORREF Color) override;
		bool DrawText(LPCWSTR pText, int Length, const RECT &Rect, CTextDraw::DrawFlag Flags) override;
		int GetFitCharCount(LPCWSTR pText, int Length, int Width) override;
		bool GetFontMetrics(FontMetrics *pMetrics) override;
		bool GetTextMetrics(LPCWSTR pText, int Length, TextMetrics *pMetrics) override;
		bool SetClippingRect(const RECT &Rect) override;
		bool ResetClipping() override;
		bool OnWindowPosChanged() override;
		void ClearFontCache();
		bool SetMaxFontCache(std::size_t MaxCache);

	private:
		CDirectWriteRenderer &m_Renderer;
		CDirectWriteFont *m_pFont = nullptr;
		std::deque<std::unique_ptr<CDirectWriteFont>> m_FontCache;
		std::size_t m_MaxFontCache = 4;
		CDirectWriteBrush m_Brush;
	};

}	// namespace TVTest


#endif
