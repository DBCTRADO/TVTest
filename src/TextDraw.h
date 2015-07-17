#ifndef TVTEST_TEXT_DRAW_H
#define TVTEST_TEXT_DRAW_H


#include <vector>
#include <deque>
#include "DirectWrite.h"


namespace TVTest
{

	class CTextDrawEngine
	{
	public:
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

		virtual ~CTextDrawEngine() {}
		virtual void Finalize();
		virtual bool BeginDraw(HDC hdc,const RECT &Rect);
		virtual bool EndDraw();
		virtual bool BindDC(HDC hdc,const RECT &Rect);
		virtual bool SetFont(HFONT hfont) = 0;
		virtual bool SetTextColor(COLORREF Color) = 0;
		virtual bool DrawText(LPCWSTR pText,int Length,const RECT &Rect,unsigned int Flags) = 0;
		virtual int GetFitCharCount(LPCWSTR pText,int Length,int Width) = 0;
		virtual bool GetFontMetrics(FontMetrics *pMetrics) = 0;
		virtual bool GetTextMetrics(LPCWSTR pText,int Length,TextMetrics *pMetrics) = 0;
		virtual bool SetClippingRect(const RECT &Rect);
		virtual bool ResetClipping();
		virtual bool OnWindowPosChanged();
	};

	class CTextDrawEngine_GDI : public CTextDrawEngine
	{
	public:
		CTextDrawEngine_GDI();
		~CTextDrawEngine_GDI();
		void Finalize() override;
		bool BeginDraw(HDC hdc,const RECT &Rect) override;
		bool EndDraw() override;
		bool BindDC(HDC hdc,const RECT &Rect) override;
		bool SetFont(HFONT hfont) override;
		bool SetTextColor(COLORREF Color) override;
		bool DrawText(LPCWSTR pText,int Length,const RECT &Rect,unsigned int Flags) override;
		int GetFitCharCount(LPCWSTR pText,int Length,int Width) override;
		bool GetFontMetrics(FontMetrics *pMetrics) override;
		bool GetTextMetrics(LPCWSTR pText,int Length,TextMetrics *pMetrics) override;

	private:
		void UnbindDC();

		HDC m_hdc;
		HFONT m_hfontOld;
		COLORREF m_OldTextColor;
	};

	class CTextDrawEngine_DirectWrite : public CTextDrawEngine
	{
	public:
		CTextDrawEngine_DirectWrite(CDirectWriteRenderer &Renderer);
		~CTextDrawEngine_DirectWrite();
		void Finalize() override;
		bool BeginDraw(HDC hdc,const RECT &Rect) override;
		bool EndDraw() override;
		bool BindDC(HDC hdc,const RECT &Rect) override;
		bool SetFont(HFONT hfont) override;
		bool SetTextColor(COLORREF Color) override;
		bool DrawText(LPCWSTR pText,int Length,const RECT &Rect,unsigned int Flags) override;
		int GetFitCharCount(LPCWSTR pText,int Length,int Width) override;
		bool GetFontMetrics(FontMetrics *pMetrics) override;
		bool GetTextMetrics(LPCWSTR pText,int Length,TextMetrics *pMetrics) override;
		bool SetClippingRect(const RECT &Rect) override;
		bool ResetClipping() override;
		bool OnWindowPosChanged() override;
		void ClearFontCache();
		bool SetMaxFontCache(std::size_t MaxCache);

	private:
		CDirectWriteRenderer &m_Renderer;
		CDirectWriteFont *m_pFont;
		std::deque<CDirectWriteFont*> m_FontCache;
		std::size_t m_MaxFontCache;
		CDirectWriteBrush m_Brush;
	};

	class CTextDraw
	{
	public:
		enum {
			FLAG_END_ELLIPSIS			=0x0001U,	// é˚Ç‹ÇËÇ´ÇÁÇ»Ç¢èÍçáè»ó™ãLçÜÇïtâ¡
			FLAG_JAPANESE_HYPHNATION	=0x0002U	// ã÷ë•èàóù
		};

		enum {
			DRAW_FLAG_ALIGN_HORZ_CENTER  = 0x0001U,
			DRAW_FLAG_ALIGN_RIGHT        = 0x0002U,
			DRAW_FLAG_ALIGN_JUSTIFIED    = 0x0004U,
			DRAW_FLAG_ALIGN_VERT_CENTER  = 0x0008U,
			DRAW_FLAG_ALIGN_BOTTOM       = 0x0010U,
			DRAW_FLAG_JUSTIFY_MULTI_LINE = 0x0020U
		};

		typedef CTextDrawEngine::FontMetrics FontMetrics;
		typedef CTextDrawEngine::TextMetrics TextMetrics;

		CTextDraw();
		~CTextDraw();
		bool SetEngine(CTextDrawEngine *pEngine);
		bool Begin(HDC hdc,const RECT &Rect,unsigned int Flags=0);
		void End();
		bool BindDC(HDC hdc,const RECT &Rect);
		void SetFlags(unsigned int Flags) { m_Flags=Flags; }
		unsigned int GetFlags() const { return m_Flags; }
		bool SetFont(HFONT hfont);
		bool SetFont(const LOGFONT &Font);
		bool SetTextColor(COLORREF Color);
		int CalcLineCount(LPCWSTR pszText,int Width);
		bool Draw(LPCWSTR pszText,const RECT &Rect,int LineHeight,unsigned int Flags=0);
		bool GetFontMetrics(FontMetrics *pMetrics);
		bool GetTextMetrics(LPCWSTR pText,int Length,TextMetrics *pMetrics);
		bool SetClippingRect(const RECT &Rect);
		bool ResetClipping();

	private:
		CTextDrawEngine *m_pEngine;
		CTextDrawEngine_GDI m_DefaultEngine;
		unsigned int m_Flags;
		std::vector<WCHAR> m_StringBuffer;

		int GetLineLength(LPCWSTR pszText);
		int AdjustLineLength(LPCWSTR pszText,int Length);
		int GetFitCharCount(LPCWSTR pText,int Length,int Width);

		static const LPCWSTR m_pszStartProhibitChars;
		static const LPCWSTR m_pszEndProhibitChars;

		static bool IsStartProhibitChar(WCHAR Char);
		static bool IsEndProhibitChar(WCHAR Char);
	};

}	// namespace TVTest


#endif
