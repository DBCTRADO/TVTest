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


#include "stdafx.h"
#include <vector>
#include <cmath>
#include "TVTest.h"
#include "DirectWrite.h"
#include "ComUtility.h"
#include "Common/DebugDef.h"


namespace TVTest
{


static inline D2D1_COLOR_F D2DColorF(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha)
{
	return D2D1::ColorF(
		static_cast<float>(Red)   / 255.0f,
		static_cast<float>(Green) / 255.0f,
		static_cast<float>(Blue)  / 255.0f,
		static_cast<float>(Alpha) / 255.0f);
}


static inline D2D1_RECT_F D2DRectF(const RECT &Rect)
{
	return D2D1::RectF(
		static_cast<float>(Rect.left),
		static_cast<float>(Rect.top),
		static_cast<float>(Rect.right),
		static_cast<float>(Rect.bottom));
}




CDirectWriteSystem::~CDirectWriteSystem()
{
	Finalize();
}


bool CDirectWriteSystem::Initialize()
{
	if (m_hD2DLib == nullptr) {
		m_hD2DLib = Util::LoadSystemLibrary(TEXT("d2d1.dll"));
		if (m_hD2DLib == nullptr)
			return false;
	}

	if (m_hDWriteLib == nullptr) {
		m_hDWriteLib = Util::LoadSystemLibrary(TEXT("dwrite.dll"));
		if (m_hDWriteLib == nullptr)
			return false;
	}

	if (m_pD2DFactory == nullptr) {
		typedef HRESULT (WINAPI * D2D1CreateFactoryFunc)(
			D2D1_FACTORY_TYPE factoryType,
			REFIID riid,
			const D2D1_FACTORY_OPTIONS * pFactoryOptions,
			void **ppIFactory);
		D2D1CreateFactoryFunc pD2D1CreateFactory =
			reinterpret_cast<D2D1CreateFactoryFunc>(::GetProcAddress(m_hD2DLib, "D2D1CreateFactory"));
		if (pD2D1CreateFactory == nullptr)
			return false;
		const HRESULT hr = pD2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			__uuidof(ID2D1Factory),
			nullptr,
			reinterpret_cast<void**>(&m_pD2DFactory));
		if (FAILED(hr))
			return false;
	}

	if (m_pDWriteFactory == nullptr) {
		auto pDWriteCreateFactory =
			GET_LIBRARY_FUNCTION(m_hDWriteLib, DWriteCreateFactory);
		if (pDWriteCreateFactory == nullptr)
			return false;
		const HRESULT hr = pDWriteCreateFactory(
			DWRITE_FACTORY_TYPE_SHARED,
			__uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (FAILED(hr))
			return false;
	}

	return true;
}


void CDirectWriteSystem::Finalize()
{
	SafeRelease(&m_pDWriteFactory);
	SafeRelease(&m_pD2DFactory);

	if (m_hDWriteLib != nullptr) {
		::FreeLibrary(m_hDWriteLib);
		m_hDWriteLib = nullptr;
	}

	if (m_hD2DLib != nullptr) {
		::FreeLibrary(m_hD2DLib);
		m_hD2DLib = nullptr;
	}
}


bool CDirectWriteSystem::IsInitialized() const
{
	return m_pDWriteFactory != nullptr;
}


ID2D1Factory *CDirectWriteSystem::GetD2DFactory()
{
	if (m_pD2DFactory == nullptr)
		return nullptr;

	m_pD2DFactory->AddRef();

	return m_pD2DFactory;
}


IDWriteFactory *CDirectWriteSystem::GetDWriteFactory()
{
	if (m_pDWriteFactory == nullptr)
		return nullptr;

	m_pDWriteFactory->AddRef();

	return m_pDWriteFactory;
}




CDirectWriteFont::~CDirectWriteFont()
{
	Destroy();
}


bool CDirectWriteFont::Create(CDirectWriteRenderer &Renderer, const LOGFONT &lf)
{
	Destroy();

	IDWriteFactory *pFactory = Renderer.GetSystem().GetDWriteFactory();

	if (pFactory == nullptr)
		return false;

	float FontSize;
	if (lf.lfHeight < 0 || Renderer.GetDC() == nullptr) {
		FontSize = static_cast<float>(std::abs(lf.lfHeight));
	} else {
		const HDC hdc = Renderer.GetDC();
		const HFONT hfont = ::CreateFontIndirect(&lf);
		const HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
		TEXTMETRIC tm;
		::GetTextMetrics(hdc, &tm);
		FontSize = static_cast<float>(tm.tmHeight - tm.tmInternalLeading);
		::SelectObject(hdc, hOldFont);
		::DeleteObject(hfont);
	}

	IDWriteTextFormat *pTextFormat;
	const HRESULT hr = pFactory->CreateTextFormat(
		lf.lfFaceName,
		nullptr,
		static_cast<DWRITE_FONT_WEIGHT>(lf.lfWeight),
		lf.lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		FontSize,
		L"",
		&pTextFormat);
	if (SUCCEEDED(hr)) {
		m_pTextFormat = pTextFormat;
		m_LogFont = lf;
	}

	pFactory->Release();

	return true;
}


void CDirectWriteFont::Destroy()
{
	SafeRelease(&m_pTextFormat);
}


bool CDirectWriteFont::IsCreated() const
{
	return m_pTextFormat != nullptr;
}


IDWriteTextFormat *CDirectWriteFont::GetTextFormat()
{
	if (m_pTextFormat == nullptr)
		return nullptr;

	m_pTextFormat->AddRef();

	return m_pTextFormat;
}


bool CDirectWriteFont::GetLogFont(LOGFONT *pLogFont) const
{
	if (m_pTextFormat == nullptr || pLogFont == nullptr)
		return false;

	*pLogFont = m_LogFont;

	return true;
}




CDirectWriteBrush::~CDirectWriteBrush()
{
	Destroy();
}


bool CDirectWriteBrush::Create(CDirectWriteRenderer &Renderer, BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha)
{
	Destroy();

	ID2D1RenderTarget *pRenderTarget = Renderer.GetRenderTarget();

	if (pRenderTarget == nullptr)
		return false;

	ID2D1SolidColorBrush *pBrush;
	const HRESULT hr = pRenderTarget->CreateSolidColorBrush(
		D2DColorF(Red, Green, Blue, Alpha),
		&pBrush);
	if (SUCCEEDED(hr))
		m_pBrush = pBrush;

	pRenderTarget->Release();

	return true;
}


void CDirectWriteBrush::Destroy()
{
	SafeRelease(&m_pBrush);
}


bool CDirectWriteBrush::IsCreated() const
{
	return m_pBrush != nullptr;
}


bool CDirectWriteBrush::SetColor(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha)
{
	if (m_pBrush == nullptr)
		return false;

	m_pBrush->SetColor(D2DColorF(Red, Green, Blue, Alpha));

	return true;
}


ID2D1Brush *CDirectWriteBrush::GetBrush()
{
	if (m_pBrush == nullptr)
		return nullptr;

	m_pBrush->AddRef();

	return m_pBrush;
}




CDirectWriteRenderer::CDirectWriteRenderer(CDirectWriteSystem &System)
	: m_System(System)
{
}


CDirectWriteRenderer::~CDirectWriteRenderer()
{
	Finalize();
}


bool CDirectWriteRenderer::Initialize(HWND hwnd)
{
	Finalize();

	if (hwnd == nullptr)
		return false;

	ID2D1Factory *pFactory = m_System.GetD2DFactory();

	if (pFactory == nullptr)
		return false;

	D2D1_RENDER_TARGET_PROPERTIES Props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(
			DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_IGNORE),
		96.0f,
		96.0f,
		D2D1_RENDER_TARGET_USAGE_NONE,
		D2D1_FEATURE_LEVEL_DEFAULT);
	ID2D1DCRenderTarget *pRenderTarget;
	const HRESULT hr = pFactory->CreateDCRenderTarget(&Props, &pRenderTarget);
	if (SUCCEEDED(hr)) {
		m_pRenderTarget = pRenderTarget;
		m_hwnd = hwnd;
		OnWindowPosChanged();
	}

	pFactory->Release();

	return SUCCEEDED(hr);
}


void CDirectWriteRenderer::Finalize()
{
	if (m_pRenderTarget != nullptr) {
		m_pRenderTarget->Release();
		m_pRenderTarget = nullptr;
	}

	m_hwnd = nullptr;
	m_hMonitor = nullptr;
	m_fNeedRecreate = false;
}


bool CDirectWriteRenderer::IsInitialized() const
{
	return m_pRenderTarget != nullptr;
}


ID2D1RenderTarget *CDirectWriteRenderer::GetRenderTarget()
{
	if (m_pRenderTarget == nullptr)
		return nullptr;

	m_pRenderTarget->AddRef();

	return m_pRenderTarget;
}


bool CDirectWriteRenderer::BeginDraw(HDC hdc, const RECT &Rect)
{
	if (hdc == nullptr)
		return false;

	if (m_pRenderTarget == nullptr) {
		if (!m_fNeedRecreate)
			return false;
		if (!Initialize(m_hwnd))
			return false;
	}

	m_pRenderTarget->BindDC(hdc, &Rect);
	m_pRenderTarget->BeginDraw();

	return true;
}


bool CDirectWriteRenderer::EndDraw()
{
	if (m_pRenderTarget == nullptr)
		return false;

	const HRESULT hr = m_pRenderTarget->EndDraw();

	m_fNeedRecreate = hr == D2DERR_RECREATE_TARGET;
	if (m_fNeedRecreate) {
		m_pRenderTarget->Release();
		m_pRenderTarget = nullptr;
	}

	return hr == S_OK;
}


bool CDirectWriteRenderer::BindDC(HDC hdc, const RECT &Rect)
{
	if (m_pRenderTarget == nullptr)
		return false;

	if (FAILED(m_pRenderTarget->BindDC(hdc, &Rect))) {
		m_hdc = nullptr;
		return false;
	}

	m_hdc = hdc;

	return true;
}


bool CDirectWriteRenderer::Clear(BYTE Red, BYTE Green, BYTE Blue, BYTE Alpha)
{
	if (m_pRenderTarget == nullptr)
		return false;

	m_pRenderTarget->Clear(D2DColorF(Red, Green, Blue, Alpha));

	return true;
}


bool CDirectWriteRenderer::SetClippingRect(const RECT &Rect)
{
	if (m_pRenderTarget == nullptr)
		return false;

	m_pRenderTarget->PushAxisAlignedClip(
		D2DRectF(Rect),
		D2D1_ANTIALIAS_MODE_ALIASED);

	return true;
}


bool CDirectWriteRenderer::ResetClipping()
{
	if (m_pRenderTarget == nullptr)
		return false;

	m_pRenderTarget->PopAxisAlignedClip();

	return true;
}


bool CDirectWriteRenderer::SetRenderingParams(const RenderingParams &Params)
{
	m_RenderingParams.Mask = RenderingParams::ParamFlag::None;

	if (!!(Params.Mask & RenderingParams::ParamFlag::Gamma)
			&& Params.Gamma > 0.0f
			&& Params.Gamma <= 256.0f) {
		m_RenderingParams.Mask |= RenderingParams::ParamFlag::Gamma;
		m_RenderingParams.Gamma = Params.Gamma;
	}

	if (!!(Params.Mask & RenderingParams::ParamFlag::EnhancedContrast)
			&& Params.EnhancedContrast >= 0.0f) {
		m_RenderingParams.Mask |= RenderingParams::ParamFlag::EnhancedContrast;
		m_RenderingParams.EnhancedContrast = Params.EnhancedContrast;
	}

	if (!!(Params.Mask & RenderingParams::ParamFlag::ClearTypeLevel)
			&& Params.ClearTypeLevel >= 0.0f
			&& Params.ClearTypeLevel <= 1.0f) {
		m_RenderingParams.Mask |= RenderingParams::ParamFlag::ClearTypeLevel;
		m_RenderingParams.ClearTypeLevel = Params.ClearTypeLevel;
	}

	if (!!(Params.Mask & RenderingParams::ParamFlag::PixelGeometry)
			&& Params.PixelGeometry >= DWRITE_PIXEL_GEOMETRY_FLAT
			&& Params.PixelGeometry <= DWRITE_PIXEL_GEOMETRY_BGR) {
		m_RenderingParams.Mask |= RenderingParams::ParamFlag::PixelGeometry;
		m_RenderingParams.PixelGeometry = Params.PixelGeometry;
	}

	if (!!(Params.Mask & RenderingParams::ParamFlag::RenderingMode)
			&& Params.RenderingMode >= DWRITE_RENDERING_MODE_ALIASED
			&& Params.RenderingMode <= DWRITE_RENDERING_MODE_OUTLINE) {
		m_RenderingParams.Mask |= RenderingParams::ParamFlag::RenderingMode;
		m_RenderingParams.RenderingMode = Params.RenderingMode;
	}

	if (m_pRenderTarget != nullptr)
		UpdateRenderingParams();

	return true;
}


bool CDirectWriteRenderer::OnWindowPosChanged()
{
	if (m_pRenderTarget == nullptr)
		return false;

	bool fUpdated = false;
	const HMONITOR hMonitor = ::MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONULL);

	if (hMonitor != nullptr && hMonitor != m_hMonitor) {
		m_hMonitor = hMonitor;

		fUpdated = UpdateRenderingParams();
	}

	return fUpdated;
}


bool CDirectWriteRenderer::DrawText(
	LPCWSTR pText, int Length, const RECT &Rect,
	CDirectWriteFont &Font, CDirectWriteBrush &Brush, DrawTextFlag Flags)
{
	if (pText == nullptr)
		return false;
	if (m_pRenderTarget == nullptr)
		return false;

	bool fOK = false;

	IDWriteTextFormat *pTextFormat = Font.GetTextFormat();

	if (pTextFormat != nullptr) {
		ID2D1Brush *pBrush = Brush.GetBrush();

		if (pBrush != nullptr) {
			pTextFormat->SetTextAlignment(
				!!(Flags & DrawTextFlag::Align_HorzCenter) ?
					DWRITE_TEXT_ALIGNMENT_CENTER :
				!!(Flags & DrawTextFlag::Align_Right) ?
					DWRITE_TEXT_ALIGNMENT_TRAILING :
				(!!(Flags & DrawTextFlag::Align_Justified) && Util::OS::IsWindows8OrLater()) ?
					DWRITE_TEXT_ALIGNMENT_JUSTIFIED :
					DWRITE_TEXT_ALIGNMENT_LEADING);
			pTextFormat->SetParagraphAlignment(
				!!(Flags & DrawTextFlag::Align_VertCenter) ?
					DWRITE_PARAGRAPH_ALIGNMENT_CENTER :
				!!(Flags & DrawTextFlag::Align_Bottom) ?
					DWRITE_PARAGRAPH_ALIGNMENT_FAR :
					DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
			pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			const DWRITE_TRIMMING Trimming = {DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0};
			pTextFormat->SetTrimming(&Trimming, nullptr);

			m_pRenderTarget->DrawText(
				pText,
				Length >= 0 ? Length : ::lstrlenW(pText),
				pTextFormat,
				D2DRectF(Rect),
				pBrush);
			fOK = true;

			pBrush->Release();
		}

		pTextFormat->Release();
	}

	return fOK;
}


int CDirectWriteRenderer::GetFitCharCount(LPCWSTR pText, int Length, int Width, CDirectWriteFont &Font)
{
	if (pText == nullptr || Length == 0)
		return 0;
	if (m_pRenderTarget == nullptr)
		return 0;

	int FitCharCount = 0;

	IDWriteFactory *pFactory = m_System.GetDWriteFactory();

	if (pFactory != nullptr) {
		IDWriteTextFormat *pTextFormat = Font.GetTextFormat();

		if (pTextFormat != nullptr) {
			IDWriteTextLayout *pTextLayout;

			pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			if (Length < 0)
				Length = ::lstrlenW(pText);
			HRESULT hr = pFactory->CreateTextLayout(
				pText,
				Length,
				pTextFormat,
				static_cast<float>(Width),
				m_pRenderTarget->GetSize().height,
				&pTextLayout);
			if (SUCCEEDED(hr)) {
				Util::CTempBuffer<DWRITE_CLUSTER_METRICS, 256> ClusterMetrics(Length);
				UINT32 ClusterCount;

				hr = pTextLayout->GetClusterMetrics(ClusterMetrics.GetBuffer(), Length, &ClusterCount);
				if (SUCCEEDED(hr)) {
					float Pos = 0.0f;

					for (UINT32 i = 0; i < ClusterCount; i++) {
						Pos += ClusterMetrics[i].width;
						if (static_cast<int>(std::ceil(Pos)) > Width)
							break;
						FitCharCount += ClusterMetrics[i].length;
					}
				}

				pTextLayout->Release();
			}

			pTextFormat->Release();
		}

		pFactory->Release();
	}

	return FitCharCount;
}


bool CDirectWriteRenderer::GetFontMetrics(CDirectWriteFont &Font, FontMetrics *pMetrics)
{
	if (pMetrics == nullptr)
		return false;
	if (m_pRenderTarget == nullptr)
		return false;

	HRESULT hr = E_UNEXPECTED;
	IDWriteTextFormat *pTextFormat = Font.GetTextFormat();

	if (pTextFormat != nullptr) {
		IDWriteFontCollection *pFontCollection;

		hr = pTextFormat->GetFontCollection(&pFontCollection);
		if (SUCCEEDED(hr)) {
			WCHAR szName[256];

			hr = pTextFormat->GetFontFamilyName(szName, lengthof(szName));
			if (SUCCEEDED(hr)) {
				UINT32 Index;
				BOOL fExists;

				hr = pFontCollection->FindFamilyName(szName, &Index, &fExists);
				if (SUCCEEDED(hr)) {
					IDWriteFontFamily *pFontFamily;

					hr = pFontCollection->GetFontFamily(Index, &pFontFamily);
					if (SUCCEEDED(hr)) {
						IDWriteFont *pFont;

						hr = pFontFamily->GetFirstMatchingFont(
							pTextFormat->GetFontWeight(),
							pTextFormat->GetFontStretch(),
							pTextFormat->GetFontStyle(),
							&pFont);
						if (SUCCEEDED(hr)) {
							DWRITE_FONT_METRICS Metrics;

							pFont->GetMetrics(&Metrics);
							const float Ratio = pTextFormat->GetFontSize() / static_cast<float>(Metrics.designUnitsPerEm);
							pMetrics->Ascent = Metrics.ascent * Ratio;
							pMetrics->Descent = Metrics.descent * Ratio;
							pMetrics->LineGap = Metrics.lineGap * Ratio;

							pFont->Release();
						}

						pFontFamily->Release();
					}
				}
			}
		}

		pFontCollection->Release();
	}

	return SUCCEEDED(hr);
}


bool CDirectWriteRenderer::GetTextMetrics(
	LPCWSTR pText, int Length, CDirectWriteFont &Font, TextMetrics *pMetrics)
{
	if (pText == nullptr || pMetrics == nullptr)
		return false;
	if (m_pRenderTarget == nullptr)
		return false;

	HRESULT hr = E_UNEXPECTED;
	IDWriteFactory *pFactory = m_System.GetDWriteFactory();

	if (pFactory != nullptr) {
		IDWriteTextFormat *pTextFormat = Font.GetTextFormat();

		if (pTextFormat != nullptr) {
			IDWriteTextLayout *pTextLayout;

			pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			if (Length < 0)
				Length = ::lstrlenW(pText);
			const D2D1_SIZE_F Size = m_pRenderTarget->GetSize();
			hr = pFactory->CreateTextLayout(
				pText,
				Length,
				pTextFormat,
				Size.width,
				Size.height,
				&pTextLayout);
			if (SUCCEEDED(hr)) {
				DWRITE_TEXT_METRICS Metrics;

				hr = pTextLayout->GetMetrics(&Metrics);
				if (SUCCEEDED(hr)) {
					pMetrics->Width = Metrics.width;
					pMetrics->WidthIncludingTrailingWhitespace = Metrics.widthIncludingTrailingWhitespace;
					pMetrics->Height = Metrics.height;
				}

				pTextLayout->Release();
			}

			pTextFormat->Release();
		}

		pFactory->Release();
	}

	return SUCCEEDED(hr);
}


bool CDirectWriteRenderer::UpdateRenderingParams()
{
	if (m_pRenderTarget == nullptr)
		return false;

	bool fUpdated = false;
	IDWriteFactory *pFactory = m_System.GetDWriteFactory();

	if (pFactory != nullptr) {
		IDWriteRenderingParams *pRenderingParams;

		HRESULT hr = pFactory->CreateMonitorRenderingParams(m_hMonitor, &pRenderingParams);
		if (SUCCEEDED(hr)) {
			if (m_RenderingParams.Mask != RenderingParams::ParamFlag::None) {
				IDWriteRenderingParams *pCustomRenderingParams;
				hr = pFactory->CreateCustomRenderingParams(
					!!(m_RenderingParams.Mask & RenderingParams::ParamFlag::Gamma) ?
						m_RenderingParams.Gamma : pRenderingParams->GetGamma(),
					!!(m_RenderingParams.Mask & RenderingParams::ParamFlag::EnhancedContrast) ?
						m_RenderingParams.EnhancedContrast : pRenderingParams->GetEnhancedContrast(),
					!!(m_RenderingParams.Mask & RenderingParams::ParamFlag::ClearTypeLevel) ?
						m_RenderingParams.ClearTypeLevel : pRenderingParams->GetClearTypeLevel(),
					!!(m_RenderingParams.Mask & RenderingParams::ParamFlag::PixelGeometry) ?
						m_RenderingParams.PixelGeometry : pRenderingParams->GetPixelGeometry(),
					!!(m_RenderingParams.Mask & RenderingParams::ParamFlag::RenderingMode) ?
						m_RenderingParams.RenderingMode : pRenderingParams->GetRenderingMode(),
					&pCustomRenderingParams);
				if (SUCCEEDED(hr)) {
					m_pRenderTarget->SetTextRenderingParams(pCustomRenderingParams);
					pCustomRenderingParams->Release();
				}
			} else {
				m_pRenderTarget->SetTextRenderingParams(pRenderingParams);
			}

			if (SUCCEEDED(hr))
				fUpdated = true;

			pRenderingParams->Release();
		}

		pFactory->Release();
	}

	return fUpdated;
}



} // namespace TVTest
