#ifndef TVTEST_DIRECTWRITE_H
#define TVTEST_DIRECTWRITE_H


#include <d2d1.h>
#include <dwrite.h>


namespace TVTest
{

	class CDirectWriteSystem
	{
	public:
		CDirectWriteSystem();
		~CDirectWriteSystem();
		bool Initialize();
		void Finalize();
		bool IsInitialized() const;
		ID2D1Factory *GetD2DFactory();
		IDWriteFactory *GetDWriteFactory();

	private:
		HMODULE m_hD2DLib;
		HMODULE m_hDWriteLib;
		ID2D1Factory *m_pD2DFactory;
		IDWriteFactory *m_pDWriteFactory;
	};

	class CDirectWriteRenderer;

	class CDirectWriteResource
	{
	public:
		virtual ~CDirectWriteResource() {}
		virtual void Destroy() = 0;
		virtual bool IsCreated() const = 0;
	};

	class CDirectWriteFont : public CDirectWriteResource
	{
	public:
		CDirectWriteFont();
		~CDirectWriteFont();
		bool Create(CDirectWriteRenderer &Renderer, const LOGFONT &lf);
		void Destroy() override;
		bool IsCreated() const override;
		IDWriteTextFormat *GetTextFormat();
		bool GetLogFont(LOGFONT *pLogFont) const;

	private:
		IDWriteTextFormat *m_pTextFormat;
		LOGFONT m_LogFont;
	};

	class CDirectWriteBrush : public CDirectWriteResource
	{
	public:
		CDirectWriteBrush();
		~CDirectWriteBrush();
		bool Create(CDirectWriteRenderer &Renderer, BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha = 255);
		void Destroy() override;
		bool IsCreated() const override;
		bool SetColor(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha = 255);
		ID2D1Brush *GetBrush();

	private:
		ID2D1SolidColorBrush *m_pBrush;
	};

	class CDirectWriteRenderer
	{
	public:
		struct RenderingParams
		{
			enum {
				PARAM_GAMMA             = 0x01U,
				PARAM_ENHANCED_CONTRAST = 0x02U,
				PARAM_CLEARTYPE_LEVEL   = 0x04U,
				PARAM_PIXEL_GEOMETRY    = 0x08U,
				PARAM_RENDERING_MODE    = 0x10U
			};

			unsigned int Mask;
			float Gamma;
			float EnhancedContrast;
			float ClearTypeLevel;
			DWRITE_PIXEL_GEOMETRY PixelGeometry;
			DWRITE_RENDERING_MODE RenderingMode;

			RenderingParams() : Mask(0) {}
		};

		struct FontMetrics
		{
			float Ascent;
			float Descent;
			float LineGap;
		};

		struct TextMetrics
		{
			float Width;
			float WidthIncludingTrailingWhitespace;
			float Height;
		};

		enum {
			DRAW_TEXT_ALIGN_HORZ_CENTER = 0x0001U,
			DRAW_TEXT_ALIGN_RIGHT       = 0x0002U,
			DRAW_TEXT_ALIGN_JUSTIFIED   = 0x0004U,
			DRAW_TEXT_ALIGN_VERT_CENTER = 0x0008U,
			DRAW_TEXT_ALIGN_BOTTOM      = 0x0010U
		};

		CDirectWriteRenderer(CDirectWriteSystem &System);
		~CDirectWriteRenderer();
		bool Initialize(HWND hwnd);
		void Finalize();
		bool IsInitialized() const;
		CDirectWriteSystem &GetSystem() { return m_System; }
		ID2D1RenderTarget *GetRenderTarget();
		bool BeginDraw(HDC hdc, const RECT &Rect);
		bool EndDraw();
		bool BindDC(HDC hdc, const RECT &Rect);
		HDC GetDC() const { return m_hdc; }
		bool Clear(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha = 255);
		bool SetClippingRect(const RECT &Rect);
		bool ResetClipping();
		bool SetRenderingParams(const RenderingParams &Params);
		bool OnWindowPosChanged();
		bool DrawText(
			LPCWSTR pText, int Length, const RECT &Rect,
			CDirectWriteFont &Font, CDirectWriteBrush &Brush, unsigned int Flags = 0);
		int GetFitCharCount(LPCWSTR pText, int Length, int Width, CDirectWriteFont &Font);
		bool IsNeedRecreate() const { return m_fNeedRecreate; }
		bool GetFontMetrics(CDirectWriteFont &Font, FontMetrics *pMetrics);
		bool GetTextMetrics(LPCWSTR pText, int Length, CDirectWriteFont &Font, TextMetrics *pMetrics);

	private:
		bool UpdateRenderingParams();

		CDirectWriteSystem &m_System;
		ID2D1DCRenderTarget *m_pRenderTarget;
		HWND m_hwnd;
		HDC m_hdc;
		HMONITOR m_hMonitor;
		RenderingParams m_RenderingParams;
		bool m_fNeedRecreate;
	};

}	// namespace TVTest


#endif
