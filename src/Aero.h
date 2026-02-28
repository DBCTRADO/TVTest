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


#ifndef TVTEST_AERO_H
#define TVTEST_AERO_H


namespace TVTest
{

	class CAeroGlass
	{
	public:
		bool IsEnabled();
		bool ApplyAeroGlass(HWND hwnd, const RECT *pRect);
		bool EnableNcRendering(HWND hwnd, bool fEnable);
	};

	class CBufferedPaint
	{
	public:
		CBufferedPaint() = default;
		~CBufferedPaint();

		CBufferedPaint(const CBufferedPaint &) = delete;
		CBufferedPaint &operator=(const CBufferedPaint &) = delete;

		HDC Begin(HDC hdc, const RECT *pRect, bool fErase = false);
		bool End(bool fUpdate = true);
		bool Clear(const RECT *pRect = nullptr);
		bool SetAlpha(BYTE Alpha);
		bool SetOpaque() { return SetAlpha(255); }

		static bool Initialize();
		static bool IsSupported();

	private:
		HPAINTBUFFER m_hPaintBuffer = nullptr;
	};

	class CDoubleBufferingDraw
	{
	public:
		virtual void Draw(HDC hdc, const RECT &PaintRect) = 0;
		void OnPaint(HWND hwnd);
	};

}


#endif
