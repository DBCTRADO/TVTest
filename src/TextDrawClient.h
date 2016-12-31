#ifndef TVTEST_TEXT_DRAW_CLIENT_H
#define TVTEST_TEXT_DRAW_CLIENT_H


#include "TextDraw.h"


namespace TVTest
{

	class CTextDrawClient
	{
	public:
		enum TextDrawEngine {
			ENGINE_UNDEFINED,
			ENGINE_GDI,
			ENGINE_DIRECTWRITE
		};

		CTextDrawClient();
		~CTextDrawClient();
		bool Initialize(TextDrawEngine Engine,HWND hwnd);
		void Finalize();
		bool InitializeTextDraw(CTextDraw *pTextDraw);
		bool SetMaxFontCache(std::size_t MaxCache);
		bool SetDirectWriteRenderingParams(const CDirectWriteRenderer::RenderingParams &Params);

	private:
		TextDrawEngine m_Engine;
		CDirectWriteRenderer m_DirectWriteRenderer;
		CTextDrawEngine_DirectWrite m_DirectWriteEngine;
	};

}	// namespace TVTest


#endif
