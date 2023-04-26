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


#ifndef TVTEST_TEXT_DRAW_CLIENT_H
#define TVTEST_TEXT_DRAW_CLIENT_H


#include "TextDraw.h"


namespace TVTest
{

	class CTextDrawClient
	{
	public:
		enum class TextDrawEngine {
			Undefined,
			GDI,
			DirectWrite,
		};

		CTextDrawClient() = default;
		~CTextDrawClient();

		CTextDrawClient(const CTextDrawClient &) = delete;
		CTextDrawClient &operator=(const CTextDrawClient &) = delete;

		bool Initialize(TextDrawEngine Engine, HWND hwnd);
		void Finalize();
		bool InitializeTextDraw(CTextDraw *pTextDraw);
		bool SetMaxFontCache(std::size_t MaxCache);
		bool SetDirectWriteRenderingParams(const CDirectWriteRenderer::RenderingParams &Params);

	private:
		class CDirectWriteEngine
		{
		public:
			CDirectWriteEngine(CDirectWriteSystem &System);

			bool Initialize(HWND hwnd);
			void Finalize();

			CDirectWriteRenderer Renderer;
			CTextDrawEngine_DirectWrite Engine;
		};

		CDirectWriteEngine *GetDirectWriteEngine();

		TextDrawEngine m_Engine = TextDrawEngine::Undefined;
		std::unique_ptr<CDirectWriteEngine> m_DirectWriteEngine;
	};

}	// namespace TVTest


#endif
