#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "TextDrawClient.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CTextDrawClient::CTextDrawClient()
	: m_Engine(TextDrawEngine::Undefined)
	, m_DirectWriteRenderer(GetAppClass().DirectWriteSystem)
	, m_DirectWriteEngine(m_DirectWriteRenderer)
{
}


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
		if (!m_DirectWriteRenderer.GetSystem().Initialize()) {
			GetAppClass().AddLog(CLogItem::LogType::Error, TEXT("DirectWrite の初期化ができません。"));
			return false;
		}
		if (!m_DirectWriteRenderer.Initialize(hwnd))
			return false;
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
		m_DirectWriteEngine.Finalize();
		m_DirectWriteRenderer.Finalize();
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
		pTextDraw->SetEngine(&m_DirectWriteEngine);
		break;

	default:
		return false;
	}

	return true;
}


bool CTextDrawClient::SetMaxFontCache(std::size_t MaxCache)
{
	return m_DirectWriteEngine.SetMaxFontCache(MaxCache);
}


bool CTextDrawClient::SetDirectWriteRenderingParams(const CDirectWriteRenderer::RenderingParams &Params)
{
	return m_DirectWriteRenderer.SetRenderingParams(Params);
}


}	// namespace TVTest
