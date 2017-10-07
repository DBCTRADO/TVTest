#ifndef TVTEST_GRAPHICS_H
#define TVTEST_GRAPHICS_H


#include <gdiplus.h>
#include <memory>


namespace TVTest
{

	namespace Graphics
	{

		enum GradientDirection {
			GRADIENT_DIRECTION_HORZ,
			GRADIENT_DIRECTION_VERT
		};

		enum {
			TEXT_FORMAT_LEFT            = 0x00000000U,
			TEXT_FORMAT_RIGHT           = 0x00000001U,
			TEXT_FORMAT_HORZ_CENTER     = 0x00000002U,
			TEXT_FORMAT_HORZ_ALIGN_MASK = 0x00000003U,
			TEXT_FORMAT_TOP             = 0x00000000U,
			TEXT_FORMAT_BOTTOM          = 0x00000004U,
			TEXT_FORMAT_VERT_CENTER     = 0x00000008U,
			TEXT_FORMAT_VERT_ALIGN_MASK = 0x0000000CU,
			TEXT_FORMAT_NO_WRAP         = 0x00000010U,
			TEXT_FORMAT_NO_CLIP         = 0x00000020U,
			TEXT_FORMAT_END_ELLIPSIS    = 0x00000040U,
			TEXT_FORMAT_WORD_ELLIPSIS   = 0x00000080U,
			TEXT_FORMAT_TRIM_CHAR       = 0x00000100U,
			TEXT_DRAW_ANTIALIAS         = 0x00001000U,
			TEXT_DRAW_NO_ANTIALIAS      = 0x00002000U,
			TEXT_DRAW_CLEARTYPE         = 0x00004000U,
			TEXT_DRAW_HINTING           = 0x00008000U,
			TEXT_DRAW_PATH              = 0x00010000U
		};

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
				const RECT &Rect, CBrush *pBrush, UINT Flags);
			bool GetTextSize(
				LPCTSTR pszText, const LOGFONT &lf, UINT Flags, SIZE *pSize);
			bool DrawOutlineText(
				LPCTSTR pszText, const LOGFONT &lf,
				const RECT &Rect, CBrush *pBrush,
				const CColor &OutlineColor, float OutlineWidth,
				UINT Flags);
			bool GetOutlineTextSize(
				LPCTSTR pszText, const LOGFONT &lf,
				float OutlineWidth, UINT Flags, SIZE *pSize);

		private:
			std::unique_ptr<Gdiplus::Graphics> m_Graphics;

			void SetStringFormat(Gdiplus::StringFormat *pFormat, UINT Flags);
			void SetTextRenderingHint(UINT Flags);
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
