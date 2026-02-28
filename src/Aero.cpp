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
#include <dwmapi.h>
#include <uxtheme.h>
#include "TVTest.h"
#include "Aero.h"
#include "Common/DebugDef.h"

#pragma comment(lib,"dwmapi.lib")


namespace TVTest
{


// コンポジションが有効か取得する
bool CAeroGlass::IsEnabled()
{
	BOOL fEnabled = FALSE;

	return ::DwmIsCompositionEnabled(&fEnabled) == S_OK && fEnabled;
}


// クライアント領域を透けさせる
bool CAeroGlass::ApplyAeroGlass(HWND hwnd, const RECT *pRect)
{
	if (!IsEnabled())
		return false;

	MARGINS Margins;

	Margins.cxLeftWidth = pRect->left;
	Margins.cxRightWidth = pRect->right;
	Margins.cyTopHeight = pRect->top;
	Margins.cyBottomHeight = pRect->bottom;

	return ::DwmExtendFrameIntoClientArea(hwnd, &Margins) == S_OK;
}


// フレームの描画を無効にする
bool CAeroGlass::EnableNcRendering(HWND hwnd, bool fEnable)
{
	const DWMNCRENDERINGPOLICY ncrp = fEnable ? DWMNCRP_USEWINDOWSTYLE : DWMNCRP_DISABLED;

	return ::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp)) == S_OK;
}




class CBufferedPaintInitializer
{
public:
	~CBufferedPaintInitializer()
	{
		if (m_fInitialized)
			::BufferedPaintUnInit();
	}

	bool Initialize()
	{
		if (!m_fInitialized) {
			if (::BufferedPaintInit() != S_OK)
				return false;
			m_fInitialized = true;
		}
		return true;
	}

	bool IsInitialized() const { return m_fInitialized; }

private:
	bool m_fInitialized = false;
};

static CBufferedPaintInitializer BufferedPaintInitializer;


CBufferedPaint::~CBufferedPaint()
{
	End(false);
}


HDC CBufferedPaint::Begin(HDC hdc, const RECT *pRect, bool fErase)
{
	if (!BufferedPaintInitializer.IsInitialized())
		return nullptr;

	if (m_hPaintBuffer != nullptr) {
		if (!End(false))
			return nullptr;
	}

	BP_PAINTPARAMS Params = {sizeof(BP_PAINTPARAMS), 0, nullptr, nullptr};
	if (fErase)
		Params.dwFlags |= BPPF_ERASE;
	HDC hdcBuffer = nullptr;
	m_hPaintBuffer = ::BeginBufferedPaint(hdc, pRect, BPBF_TOPDOWNDIB, &Params, &hdcBuffer);
	if (m_hPaintBuffer == nullptr)
		return nullptr;
	return hdcBuffer;
}


bool CBufferedPaint::End(bool fUpdate)
{
	if (m_hPaintBuffer != nullptr) {
		::EndBufferedPaint(m_hPaintBuffer, fUpdate);
		m_hPaintBuffer = nullptr;
	}
	return true;
}


bool CBufferedPaint::Clear(const RECT *pRect)
{
	if (m_hPaintBuffer == nullptr)
		return false;
	return ::BufferedPaintClear(m_hPaintBuffer, pRect) == S_OK;
}


bool CBufferedPaint::SetAlpha(BYTE Alpha)
{
	if (m_hPaintBuffer == nullptr)
		return false;
	return ::BufferedPaintSetAlpha(m_hPaintBuffer, nullptr, Alpha) == S_OK;
}


bool CBufferedPaint::Initialize()
{
	return BufferedPaintInitializer.Initialize();
}


bool CBufferedPaint::IsSupported()
{
	return BufferedPaintInitializer.IsInitialized();
}




void CDoubleBufferingDraw::OnPaint(HWND hwnd)
{
	::PAINTSTRUCT ps;
	::BeginPaint(hwnd, &ps);
	{
		CBufferedPaint BufferedPaint;
		RECT rc;
		::GetClientRect(hwnd, &rc);
		const HDC hdc = BufferedPaint.Begin(ps.hdc, &rc);
		if (hdc != nullptr) {
			Draw(hdc, ps.rcPaint);
			BufferedPaint.End();
		} else {
			Draw(ps.hdc, ps.rcPaint);
		}
	}
	::EndPaint(hwnd, &ps);
}


} // namespace TVTest
