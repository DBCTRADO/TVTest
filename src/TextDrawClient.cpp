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
#include "TVTest.h"
#include "AppMain.h"
#include "TextDrawClient.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CTextDrawClient::~CTextDrawClient()
{
	Finalize();
}


bool CTextDrawClient::Initialize(TextDrawEngine Engine, HWND hwnd)
{
	Finalize();

	if (hwnd == nullptr)
		return false;

	switch (Engine) {
	case TextDrawEngine::GDI:
		break;

	case TextDrawEngine::DirectWrite:
		if (!GetDirectWriteEngine()->Initialize(hwnd)) {
			GetAppClass().AddLog(CLogItem::LogType::Error, TEXT("DirectWrite の初期化ができません。"));
			return false;
		}
		break;

	default:
		return false;
	}

	m_Engine = Engine;

	return true;
}


void CTextDrawClient::Finalize()
{
	switch (m_Engine) {
	case TextDrawEngine::DirectWrite:
		if (m_DirectWriteEngine)
			m_DirectWriteEngine->Finalize();
		break;
	}

	m_Engine = TextDrawEngine::Undefined;
}


bool CTextDrawClient::InitializeTextDraw(CTextDraw *pTextDraw)
{
	if (pTextDraw == nullptr)
		return false;

	switch (m_Engine) {
	case TextDrawEngine::GDI:
		pTextDraw->SetEngine(nullptr);
		break;

	case TextDrawEngine::DirectWrite:
		pTextDraw->SetEngine(&GetDirectWriteEngine()->Engine);
		break;

	default:
		return false;
	}

	return true;
}


bool CTextDrawClient::SetMaxFontCache(std::size_t MaxCache)
{
	return GetDirectWriteEngine()->Engine.SetMaxFontCache(MaxCache);
}


bool CTextDrawClient::SetDirectWriteRenderingParams(const CDirectWriteRenderer::RenderingParams &Params)
{
	return GetDirectWriteEngine()->Renderer.SetRenderingParams(Params);
}


CTextDrawClient::CDirectWriteEngine *CTextDrawClient::GetDirectWriteEngine()
{
	if (!m_DirectWriteEngine)
		m_DirectWriteEngine = std::make_unique<CDirectWriteEngine>(GetAppClass().DirectWriteSystem);
	return m_DirectWriteEngine.get();
}




CTextDrawClient::CDirectWriteEngine::CDirectWriteEngine(CDirectWriteSystem &System)
	: Renderer(System)
	, Engine(Renderer)
{
}


bool CTextDrawClient::CDirectWriteEngine::Initialize(HWND hwnd)
{
	if (!Renderer.GetSystem().Initialize())
		return false;
	if (!Renderer.Initialize(hwnd))
		return false;
	return true;
}


void CTextDrawClient::CDirectWriteEngine::Finalize()
{
	Engine.Finalize();
	Renderer.Finalize();
}


}	// namespace TVTest
