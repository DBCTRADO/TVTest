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


#ifndef TVTEST_DRAW_UTIL_H
#define TVTEST_DRAW_UTIL_H


namespace TVTest
{

	namespace DrawUtil
	{

		struct RGBA
		{
			BYTE Red   = 0;
			BYTE Green = 0;
			BYTE Blue  = 0;
			BYTE Alpha = 0;

			RGBA() = default;
			RGBA(BYTE r, BYTE g, BYTE b, BYTE a = 255) : Red(r), Green(g), Blue(b), Alpha(a) {}
			RGBA(COLORREF c) : Red(GetRValue(c)), Green(GetGValue(c)), Blue(GetBValue(c)), Alpha(255) {}

			bool operator==(const RGBA &op) const noexcept = default;

			operator COLORREF() const { return GetCOLORREF(); }

			void Set(BYTE r, BYTE g, BYTE b, BYTE a = 255) { Red = r; Green = g; Blue = b; Alpha = a; }
			COLORREF GetCOLORREF() const { return RGB(Red, Green, Blue); }
		};

		// 塗りつぶしの方向
		enum class FillDirection {
			Horz,       // 水平方向
			Vert,       // 垂直方向
			HorzMirror, // 左右対称
			VertMirror, // 上下対称
		};

		bool Fill(HDC hdc, const RECT *pRect, COLORREF Color);
		bool FillGradient(
			HDC hdc, const RECT *pRect, COLORREF Color1, COLORREF Color2,
			FillDirection Direction = FillDirection::Horz);
		bool FillGradient(
			HDC hdc, const RECT *pRect, const RGBA &Color1, const RGBA &Color2,
			FillDirection Direction = FillDirection::Horz);
		bool FillGlossyGradient(
			HDC hdc, const RECT *pRect,
			COLORREF Color1, COLORREF Color2,
			FillDirection Direction = FillDirection::Horz,
			int GlossRatio1 = 96, int GlossRatio2 = 48);
		bool FillInterlacedGradient(
			HDC hdc, const RECT *pRect,
			COLORREF Color1, COLORREF Color2,
			FillDirection Direction = FillDirection::Horz,
			COLORREF LineColor = RGB(0, 0, 0), int LineOpacity = 48);
		bool GlossOverlay(
			HDC hdc, const RECT *pRect,
			int Highlight1 = 192, int Highlight2 = 32,
			int Shadow1 = 32, int Shadow2 = 0);
		bool ColorOverlay(HDC hdc, const RECT *pRect, COLORREF Color, BYTE Opacity = 128);
		bool FillBorder(HDC hdc, const RECT *pBorderRect, const RECT *pEmptyRect, const RECT *pPaintRect, HBRUSH hbr);
		bool FillBorder(HDC hdc, const RECT *pBorderRect, const RECT *pEmptyRect, const RECT *pPaintRect, COLORREF Color);
		bool FillBorder(HDC hdc, const RECT &BorderRect, int BorderWidth, const RECT *pPaintRect, COLORREF Color);
		inline bool FillBorder(HDC hdc, const RECT &BorderRect, int BorderWidth, COLORREF Color) {
			return FillBorder(hdc, BorderRect, BorderWidth, nullptr, Color);
		}

		bool DrawBitmap(
			HDC hdc, int DstX, int DstY, int DstWidth, int DstHeight,
			HBITMAP hbm, const RECT *pSrcRect = nullptr, BYTE Opacity = 255);
		bool DrawMonoColorDIB(
			HDC hdcDst, int DstX, int DstY,
			HDC hdcSrc, int SrcX, int SrcY, int Width, int Height, COLORREF Color);
		bool DrawMonoColorDIB(
			HDC hdcDst, int DstX, int DstY,
			HBITMAP hbm, int SrcX, int SrcY, int Width, int Height, COLORREF Color);
		HBITMAP CreateDIB(int Width, int Height, int BitCount, void **ppBits = nullptr);
		HBITMAP DuplicateDIB(HBITMAP hbmSrc);
		HBITMAP ResizeBitmap(HBITMAP hbmSrc, int Width, int Height, int BitCount = 24, int StretchMode = STRETCH_HALFTONE);

		enum class FontType {
			Default,
			Message,
			Menu,
			Caption,
			SmallCaption,
			Status,
		};

		bool GetSystemFont(FontType Type, LOGFONT *pLogFont);
		bool GetSystemFontWithDPI(FontType Type, LOGFONT *pLogFont, int DPI);
		bool GetDefaultUIFont(LOGFONT *pFont);
		bool IsFontAvailable(const LOGFONT &Font, HDC hdc = nullptr);
		bool IsFontSmoothingEnabled();
		bool IsClearTypeEnabled();

		class CFont
		{
			HFONT m_hfont = nullptr;

		public:
			CFont() = default;
			CFont(const CFont &Font);
			CFont(const LOGFONT &Font);
			CFont(FontType Type);
			~CFont();

			CFont &operator=(const CFont &Font);
			bool operator==(const CFont &Font) const;

			bool Create(const LOGFONT *pLogFont);
			bool Create(FontType Type);
			bool IsCreated() const { return m_hfont != nullptr; }
			void Destroy();
			bool GetLogFont(LOGFONT *pLogFont) const;
			HFONT GetHandle() const { return m_hfont; }
			int GetHeight(bool fCell = true) const;
			int GetHeight(HDC hdc, bool fCell = true) const;
		};

		bool DrawText(
			HDC hdc, LPCTSTR pszText, const RECT &Rect, UINT Format,
			const CFont *pFont = nullptr, COLORREF Color = CLR_INVALID);

		class CBrush
		{
			HBRUSH m_hbr = nullptr;

		public:
			CBrush() = default;
			CBrush(const CBrush &Brush);
			CBrush(COLORREF Color);
			~CBrush();

			CBrush &operator=(const CBrush &Brush);

			bool Create(COLORREF Color);
			bool IsCreated() const { return m_hbr != nullptr; }
			void Destroy();
			HBRUSH GetHandle() const { return m_hbr; }
		};

		class CBitmap
		{
			HBITMAP m_hbm = nullptr;

		public:
			CBitmap() = default;
			CBitmap(const CBitmap &Src);
			~CBitmap();

			CBitmap &operator=(const CBitmap &Src);

			bool Create(int Width, int Height, int BitCount, void **ppBits = nullptr);
			bool Create(const BITMAPINFO *pbmi, size_t Size, void **ppBits = nullptr);
			bool Load(HINSTANCE hinst, LPCTSTR pszName, UINT Flags = LR_CREATEDIBSECTION);
			bool Load(HINSTANCE hinst, int ID, UINT Flags = LR_CREATEDIBSECTION) {
				return Load(hinst, MAKEINTRESOURCE(ID), Flags);
			}
			bool Attach(HBITMAP hbm);
			bool IsCreated() const { return m_hbm != nullptr; }
			void Destroy();
			HBITMAP GetHandle() const { return m_hbm; }
			bool IsDIB() const;
			int GetWidth() const;
			int GetHeight() const;
		};

		inline HFONT SelectObject(HDC hdc, const CFont &Font) {
			return static_cast<HFONT>(::SelectObject(hdc, Font.GetHandle()));
		}
		inline HBRUSH SelectObject(HDC hdc, const CBrush &Brush) {
			return static_cast<HBRUSH>(::SelectObject(hdc, Brush.GetHandle()));
		}
		inline HBITMAP SelectObject(HDC hdc, const CBitmap &Bitmap) {
			return static_cast<HBITMAP>(::SelectObject(hdc, Bitmap.GetHandle()));
		}

		class CMonoColorBitmap
		{
		public:
			CMonoColorBitmap() = default;
			CMonoColorBitmap(const CMonoColorBitmap &Src);
			CMonoColorBitmap(CMonoColorBitmap &&Src) noexcept;
			~CMonoColorBitmap();

			CMonoColorBitmap &operator=(const CMonoColorBitmap &Src);
			CMonoColorBitmap &operator=(CMonoColorBitmap &&Src) noexcept;

			bool Load(HINSTANCE hinst, LPCTSTR pszName);
			bool Load(HINSTANCE hinst, int ID) { return Load(hinst, MAKEINTRESOURCE(ID)); }
			bool Create(HBITMAP hbm);
			void Destroy() noexcept;
			HBITMAP GetHandle() const { return m_hbm; }
			bool IsCreated() const { return m_hbm != nullptr; }
			bool Draw(
				HDC hdc,
				int DstX, int DstY, int DstWidth, int DstHeight,
				int SrcX, int SrcY, int SrcWidth, int SrcHeight,
				COLORREF Color, BYTE Opacity = 255);
			bool Draw(
				HDC hdc, int DstX, int DstY, COLORREF Color, BYTE Opacity = 255,
				int SrcX = 0, int SrcY = 0, int Width = 0, int Height = 0);
			HIMAGELIST CreateImageList(int IconWidth, COLORREF Color);
			HBITMAP ExtractBitmap(int x, int y, int Width, int Height, COLORREF Color);
			HICON ExtractIcon(int x, int y, int Width, int Height, COLORREF Color);
			HICON ExtractIcon(COLORREF Color);

		private:
			void SetColor(COLORREF Color);

			HBITMAP m_hbm = nullptr;
			HBITMAP m_hbmPremultiplied = nullptr;
			COLORREF m_Color = CLR_INVALID;
			bool m_fColorImage = false;
		};

		class CMonoColorIconList
		{
		public:
			struct ResourceInfo
			{
				LPCTSTR pszName;
				int Width;
				int Height;
			};

			bool Load(HINSTANCE hinst, LPCTSTR pszName, int Width, int Height);
			bool Load(HINSTANCE hinst, int ID, int Width, int Height) {
				return Load(hinst, MAKEINTRESOURCE(ID), Width, Height);
			}
			bool Load(
				HINSTANCE hinst, int Width, int Height,
				const ResourceInfo *pResourceList, int NumResources);
			bool Create(HBITMAP hbm, int Width, int Height);
			bool Create(HBITMAP hbm, int OrigWidth, int OrigHeight, int Width, int Height);
			void Destroy();
			bool IsCreated() const;
			bool Draw(
				HDC hdc, int DstX, int DstY, int DstWidth, int DstHeight,
				int IconIndex, COLORREF Color, BYTE Opacity = 255);
			int GetIconWidth() const { return m_IconWidth; }
			int GetIconHeight() const { return m_IconHeight; }
			HIMAGELIST CreateImageList(COLORREF Color);
			HBITMAP ExtractBitmap(int Index, COLORREF Color);
			HICON ExtractIcon(int Index, COLORREF Color);

		private:
			CMonoColorBitmap m_Bitmap;
			int m_IconWidth = 0;
			int m_IconHeight = 0;
		};

		class CMemoryDC
		{
		public:
			CMemoryDC() = default;
			CMemoryDC(HDC hdc);
			~CMemoryDC();

			CMemoryDC(const CMemoryDC &) = delete;
			CMemoryDC &operator=(const CMemoryDC &) = delete;

			bool Create(HDC hdc = nullptr);
			void Delete();
			bool IsCreated() const { return m_hdc != nullptr; }
			bool SetBitmap(HBITMAP hbmSrc);
			bool SetBitmap(CBitmap &Bitmap) { return SetBitmap(Bitmap.GetHandle()); }
			bool Draw(HDC hdc, int DstX, int DstY, int SrcX, int SrcY, int Width, int Height);
			bool DrawStretch(
				HDC hdc, int DstX, int DstY, int DstWidth, int DstHeight,
				int SrcX, int SrcY, int SrcWidth, int SrcHeight,
				int Mode = STRETCH_HALFTONE);
			bool DrawAlpha(HDC hdc, int DstX, int DstY, int SrcX, int SrcY, int Width, int Height);

		private:
			HDC m_hdc = nullptr;
			HBITMAP m_hbmOld = nullptr;
		};

		class COffscreen
		{
			HDC m_hdc = nullptr;
			HBITMAP m_hbm = nullptr;
			HBITMAP m_hbmOld = nullptr;
			int m_Width = 0;
			int m_Height = 0;

		public:
			COffscreen() = default;
			~COffscreen();

			COffscreen(const COffscreen &) = delete;
			COffscreen &operator=(const COffscreen &) = delete;

			bool Create(int Width, int Height, HDC hdc = nullptr);
			void Destroy();
			bool IsCreated() const { return m_hdc != nullptr; }
			HDC GetDC() const { return m_hdc; }
			int GetWidth() const { return m_Width; }
			int GetHeight() const { return m_Height; }
			bool CopyTo(HDC hdc, const RECT *pDstRect = nullptr);
		};

	}	// namespace DrawUtil

}	// namespace TVTest


#include <uxtheme.h>
#include <vssym32.h>

namespace TVTest
{

	class CUxTheme
	{
		HTHEME m_hTheme = nullptr;

	public:
		CUxTheme() = default;
		~CUxTheme();

		CUxTheme(const CUxTheme &) = delete;
		CUxTheme &operator=(const CUxTheme &) = delete;

		bool Initialize();
		bool Open(HWND hwnd, LPCWSTR pszClassList, int DPI = 0);
		void Close();
		bool IsOpen() const;
		bool IsActive();
		bool DrawBackground(HDC hdc, int PartID, int StateID, const RECT *pRect);
		bool DrawBackground(
			HDC hdc, int PartID, int StateID,
			int BackgroundPartID, int BackgroundStateID,
			const RECT *pRect);
		bool DrawText(
			HDC hdc, int PartID, int StateID, LPCWSTR pszText,
			DWORD TextFlags, const RECT *pRect);
		bool GetTextExtent(
			HDC hdc, int PartID, int StateID, LPCWSTR pszText,
			DWORD TextFlags, RECT *pExtentRect);
		bool GetMargins(int PartID, int StateID, int PropID, MARGINS *pMargins);
		bool GetColor(int PartID, int StateID, int PropID, COLORREF *pColor);
		bool GetFont(int PartID, int StateID, int PropID, LOGFONT *pFont);
		bool GetSysFont(int FontID, LOGFONT *pFont);
		bool GetInt(int PartID, int StateID, int PropID, int *pValue);
		bool GetPartSize(HDC hdc, int PartID, int StateID, SIZE *pSize);
		bool GetTransitionDuration(
			int PartID, int StateIDFrom, int StateIDTo, DWORD *pDuration);

		static void ScaleMargins(MARGINS *pMargins, int Num, int Denom);
	};

}	// namespace TVTest


#endif
