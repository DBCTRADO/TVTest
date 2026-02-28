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


#ifndef TVTEST_DIRECTWRITE_H
#define TVTEST_DIRECTWRITE_H


#include <d2d1.h>
#include <dwrite.h>


namespace TVTest
{

	class CDirectWriteSystem
	{
	public:
		CDirectWriteSystem() = default;
		~CDirectWriteSystem();

		CDirectWriteSystem(const CDirectWriteSystem &) = delete;
		CDirectWriteSystem &operator=(const CDirectWriteSystem &) = delete;

		bool Initialize();
		void Finalize();
		bool IsInitialized() const;
		ID2D1Factory *GetD2DFactory();
		IDWriteFactory *GetDWriteFactory();

	private:
		HMODULE m_hD2DLib = nullptr;
		HMODULE m_hDWriteLib = nullptr;
		ID2D1Factory *m_pD2DFactory = nullptr;
		IDWriteFactory *m_pDWriteFactory = nullptr;
	};

	class CDirectWriteRenderer;

	class CDirectWriteResource
	{
	public:
		CDirectWriteResource() = default;
		virtual ~CDirectWriteResource() = default;

		CDirectWriteResource(const CDirectWriteResource &) = delete;
		CDirectWriteResource &operator=(const CDirectWriteResource &) = delete;

		virtual void Destroy() = 0;
		virtual bool IsCreated() const = 0;
	};

	class CDirectWriteFont
		: public CDirectWriteResource
	{
	public:
		~CDirectWriteFont();

		bool Create(CDirectWriteRenderer &Renderer, const LOGFONT &lf);
		void Destroy() override;
		bool IsCreated() const override;
		IDWriteTextFormat *GetTextFormat();
		bool GetLogFont(LOGFONT *pLogFont) const;

	private:
		IDWriteTextFormat *m_pTextFormat = nullptr;
		LOGFONT m_LogFont{};
	};

	class CDirectWriteBrush
		: public CDirectWriteResource
	{
	public:
		~CDirectWriteBrush();

		bool Create(CDirectWriteRenderer &Renderer, BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha = 255);
		void Destroy() override;
		bool IsCreated() const override;
		bool SetColor(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha = 255);
		ID2D1Brush *GetBrush();

	private:
		ID2D1SolidColorBrush *m_pBrush = nullptr;
	};

	class CDirectWriteRenderer
	{
	public:
		struct RenderingParams
		{
			enum class ParamFlag : unsigned int {
				None             = 0x0000U,
				Gamma            = 0x0001U,
				EnhancedContrast = 0x0002U,
				ClearTypeLevel   = 0x0004U,
				PixelGeometry    = 0x0008U,
				RenderingMode    = 0x0010U,
				TVTEST_ENUM_FLAGS_TRAILER
			};

			ParamFlag Mask = ParamFlag::None;
			float Gamma = 2.2f;
			float EnhancedContrast = 0.5f;
			float ClearTypeLevel = 0.5f;
			DWRITE_PIXEL_GEOMETRY PixelGeometry = DWRITE_PIXEL_GEOMETRY_RGB;
			DWRITE_RENDERING_MODE RenderingMode = DWRITE_RENDERING_MODE_DEFAULT;
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

		enum class DrawTextFlag : unsigned int {
			None             = 0x0000U,
			Align_HorzCenter = 0x0001U,
			Align_Right      = 0x0002U,
			Align_Justified  = 0x0004U,
			Align_VertCenter = 0x0008U,
			Align_Bottom     = 0x0010U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		CDirectWriteRenderer(CDirectWriteSystem &System);
		~CDirectWriteRenderer();

		CDirectWriteRenderer(const CDirectWriteRenderer &) = delete;
		CDirectWriteRenderer &operator=(const CDirectWriteRenderer &) = delete;

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
			CDirectWriteFont &Font, CDirectWriteBrush &Brush, DrawTextFlag Flags = DrawTextFlag::None);
		int GetFitCharCount(LPCWSTR pText, int Length, int Width, CDirectWriteFont &Font);
		bool IsNeedRecreate() const { return m_fNeedRecreate; }
		bool GetFontMetrics(CDirectWriteFont &Font, FontMetrics *pMetrics);
		bool GetTextMetrics(LPCWSTR pText, int Length, CDirectWriteFont &Font, TextMetrics *pMetrics);

	private:
		bool UpdateRenderingParams();

		CDirectWriteSystem &m_System;
		ID2D1DCRenderTarget *m_pRenderTarget = nullptr;
		HWND m_hwnd = nullptr;
		HDC m_hdc = nullptr;
		HMONITOR m_hMonitor = nullptr;
		RenderingParams m_RenderingParams;
		bool m_fNeedRecreate = false;
	};

} // namespace TVTest


#endif
