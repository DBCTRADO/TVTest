#ifndef TVTEST_GRAPHICS_H
#define TVTEST_GRAPHICS_H


#ifdef NOMINMAX
namespace Gdiplus
{
	using TVTest::min;
	using TVTest::max;
}
#endif
#include <gdiplus.h>
#include <memory>


namespace TVTest
{

	namespace Graphics
	{

		enum class GradientDirection {
			Horz,
			Vert,
		};

		enum class TextFlag : unsigned long {
			None                 = 0x00000000UL,
			Format_Left          = 0x00000000UL,
			Format_Right         = 0x00000001UL,
			Format_HorzCenter    = 0x00000002UL,
			Format_HorzAlignMask = 0x00000003UL,
			Format_Top           = 0x00000000UL,
			Format_Bottom        = 0x00000004UL,
			Format_VertCenter    = 0x00000008UL,
			Format_VertAlignMask = 0x0000000CUL,
			Format_NoWrap        = 0x00000010UL,
			Format_NoClip        = 0x00000020UL,
			Format_EndEllipsis   = 0x00000040UL,
			Format_WordEllipsis  = 0x00000080UL,
			Format_TrimChar      = 0x00000100UL,
			Draw_Antialias       = 0x00001000UL,
			Draw_NoAntialias     = 0x00002000UL,
			Draw_ClearType       = 0x00004000UL,
			Draw_Hinting         = 0x00008000UL,
			Draw_Path            = 0x00010000UL,
		};

		TVTEST_ENUM_FLAGS(TextFlag)

		class CColor
		{
		public:
			BYTE Red;
			BYTE Green;
			BYTE Blue;
			BYTE Alpha;

			CColor() : Red(0), Green(0), Blue(0), Alpha(0) {}
			CColor(BYTE r, BYTE g, BYTE b, BYTE a = 255) : Red(r), Green(g), Blue(b), Alpha(a) {}
			CColor(COLORREF cr) : Red(GetRValue(cr)), Green(GetGValue(cr)), Blue(GetBValue(cr)), Alpha(255) {}
		};

		class CImage
		{
		public:
			CImage() = default;
			CImage(const CImage &Src);

			CImage &operator=(const CImage &Src);

			void Free();
			bool LoadFromFile(LPCWSTR pszFileName);
			bool LoadFromResource(HINSTANCE hinst, LPCWSTR pszName);
			bool LoadFromResource(HINSTANCE hinst, LPCTSTR pszName, LPCTSTR pszType);
			bool Create(int Width, int Height, int BitsPerPixel);
			bool CreateFromBitmap(HBITMAP hbm, HPALETTE hpal = nullptr);
			bool CreateFromDIB(const BITMAPINFO *pbmi, const void *pBits);
			bool IsCreated() const;
			int GetWidth() const;
			int GetHeight() const;
			void Clear();

		private:
			std::unique_ptr<Gdiplus::Bitmap> m_Bitmap;

			friend class CCanvas;
		};

		class CBrush
		{
		public:
			CBrush() = default;
			CBrush(BYTE r, BYTE g, BYTE b, BYTE a = 255);
			CBrush(const CColor &Color);

			void Free();
			bool CreateSolidBrush(BYTE r, BYTE g, BYTE b, BYTE a = 255);
			bool CreateSolidBrush(const CColor &Color);

		private:
			std::unique_ptr<Gdiplus::SolidBrush> m_Brush;

			friend class CCanvas;
		};

		class CFont
		{
		public:
			CFont() = default;
			CFont(const LOGFONT &lf);
			void Free();

		private:
			std::unique_ptr<Gdiplus::Font> m_Font;

			friend class CCanvas;
		};

		class CCanvas
		{
		public:
			CCanvas(HDC hdc);
			CCanvas(CImage *pImage);

			bool Clear(BYTE r, BYTE g, BYTE b, BYTE a = 255);
			bool SetComposition(bool fComposite);
			bool DrawImage(CImage *pImage, int x, int y);
			bool DrawImage(
				int DstX, int DstY, int DstWidth, int DstHeight,
				CImage *pImage, int SrcX, int SrcY, int SrcWidth, int SrcHeight, float Opacity = 1.0f);
			bool FillRect(CBrush *pBrush, const RECT &Rect);
			bool FillGradient(
				const CColor &Color1, const CColor &Color2,
				const RECT &Rect, GradientDirection Direction);
			bool DrawText(
				LPCTSTR pszText, const LOGFONT &lf,
				const RECT &Rect, CBrush *pBrush, TextFlag Flags);
			bool GetTextSize(
				LPCTSTR pszText, const LOGFONT &lf, TextFlag Flags, SIZE *pSize);
			bool DrawOutlineText(
				LPCTSTR pszText, const LOGFONT &lf,
				const RECT &Rect, CBrush *pBrush,
				const CColor &OutlineColor, float OutlineWidth,
				TextFlag Flags);
			bool GetOutlineTextSize(
				LPCTSTR pszText, const LOGFONT &lf,
				float OutlineWidth, TextFlag Flags, SIZE *pSize);

		private:
			std::unique_ptr<Gdiplus::Graphics> m_Graphics;

			void SetStringFormat(Gdiplus::StringFormat *pFormat, TextFlag Flags);
			void SetTextRenderingHint(TextFlag Flags);
		};

		class CGraphicsCore
		{
		public:
			CGraphicsCore();
			~CGraphicsCore();

			bool Initialize();
			void Finalize();
			bool IsInitialized() const { return m_fInitialized; }

		private:
			bool m_fInitialized;
			ULONG_PTR m_Token;
		};

	}	// namespace Graphics

}	// namespace TVTest


#endif
