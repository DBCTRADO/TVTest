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

		CTextDrawClient();
		~CTextDrawClient();

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

		TextDrawEngine m_Engine;
		std::unique_ptr<CDirectWriteEngine> m_DirectWriteEngine;
	};

}	// namespace TVTest


#endif
