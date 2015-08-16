/*
	TVTest �v���O�C���T���v��

	�M�����x���ƃr�b�g���[�g���O���t�\������
*/


#include <windows.h>
#include <tchar.h>
#include <gdiplus.h>
#include <shlwapi.h>
#include <cmath>
#include <cstddef>
#include <deque>
#include <strsafe.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// �N���X�Ƃ��Ď���
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")


// �O���t�̑傫��
#define GRAPH_WIDTH		300
#define GRAPH_HEIGHT	200


// �v���O�C���N���X
class CSignalGraph : public TVTest::CTVTestPlugin
{
public:
	CSignalGraph();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();

private:
	struct Position
	{
		int Left, Top, Width, Height;
		Position() : Left(0), Top(0), Width(0), Height(0) {}
	};

	struct SignalInfo
	{
		float SignalLevel;
		DWORD BitRate;

		SignalInfo() : SignalLevel(0.0f), BitRate(0) {}
		SignalInfo(float level, DWORD rate) : SignalLevel(level), BitRate(rate) {}
	};

	bool m_fInitialized;
	std::deque<SignalInfo> m_List;
	HWND m_hwnd;
	Position m_WindowPosition;
	Gdiplus::Color m_BackColor;
	Gdiplus::Color m_SignalLevelColor;
	Gdiplus::Color m_BitRateColor;
	Gdiplus::Color m_GridColor;
	Gdiplus::Pen *m_pGridPen;
	Gdiplus::Pen *m_pSignalLevelPen;
	Gdiplus::Pen *m_pBitRatePen;
	Gdiplus::SolidBrush *m_pBrush;
	LOGFONT m_Font;
	Gdiplus::Font *m_pFont;
	Gdiplus::Graphics *m_pOffscreen;
	Gdiplus::Bitmap *m_pOffscreenImage;
	float m_SignalLevelScale;
	float m_ActualSignalLevelScale;
	DWORD m_BitRateScale;

	static const LPCTSTR WINDOW_CLASS_NAME;

	bool EnablePlugin(bool fEnable);
	void DrawGraph(Gdiplus::Graphics &Graphics, int Width, int Height);
	void OnPaint(HWND hwnd);
	void FreeResources();
	void AdjustSignalLevelScale();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static CSignalGraph *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	template<typename T> static inline void SafeDelete(T *&p)
	{
		if (p != nullptr) {
			delete p;
			p = nullptr;
		}
	}
};


// �E�B���h�E�N���X��
const LPCTSTR CSignalGraph::WINDOW_CLASS_NAME = TEXT("TVTest Signal Graph Window");


CSignalGraph::CSignalGraph()
	: m_fInitialized(false)
	, m_hwnd(nullptr)
	, m_BackColor(255, 0, 0, 0)
	, m_SignalLevelColor(255, 0, 255, 128)
	, m_BitRateColor(192, 0, 160, 255)
	, m_GridColor(255, 64, 64, 64)
	, m_pGridPen(nullptr)
	, m_pSignalLevelPen(nullptr)
	, m_pBitRatePen(nullptr)
	, m_pBrush(nullptr)
	, m_pFont(nullptr)
	, m_pOffscreen(nullptr)
	, m_pOffscreenImage(nullptr)
	, m_SignalLevelScale(80.0f)
	, m_BitRateScale(40 * 1000 * 1000)
{
}


// �v���O�C���̏���Ԃ�
bool CSignalGraph::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Signal Graph";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�M�����x���ƃr�b�g���[�g���O���t�\�����܂��B";
	return true;
}


// ����������
bool CSignalGraph::Initialize()
{
	// �A�C�R����o�^
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// �I������
bool CSignalGraph::Finalize()
{
	// �E�B���h�E�̔j��
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);

	// �ݒ��ۑ�
	if (m_fInitialized) {
		TCHAR szIniFileName[MAX_PATH];

		::GetModuleFileName(g_hinstDLL, szIniFileName, _countof(szIniFileName));
		::PathRenameExtension(szIniFileName, TEXT(".ini"));

		struct IntString {
			IntString(int Value) { ::StringCchPrintf(m_szBuffer, _countof(m_szBuffer), TEXT("%d"), Value); }
			operator LPCTSTR() const { return m_szBuffer; }
			TCHAR m_szBuffer[16];
		};

		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowLeft"),
									IntString(m_WindowPosition.Left), szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowTop"),
									IntString(m_WindowPosition.Top), szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowWidth"),
									IntString(m_WindowPosition.Width), szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowHeight"),
									IntString(m_WindowPosition.Height), szIniFileName);
	}

	return true;
}


// �v���O�C���̗L��/�����̐؂�ւ�
bool CSignalGraph::EnablePlugin(bool fEnable)
{
	if (fEnable) {
		// �E�B���h�E�N���X�̓o�^
		if (!m_fInitialized) {
			WNDCLASS wc;

			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = g_hinstDLL;
			wc.hIcon = nullptr;
			wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName = nullptr;
			wc.lpszClassName = WINDOW_CLASS_NAME;
			if (::RegisterClass(&wc) == 0)
				return false;

			// �ݒ�̓ǂݍ���
			TCHAR szIniFileName[MAX_PATH];
			::GetModuleFileName(g_hinstDLL, szIniFileName, _countof(szIniFileName));
			::PathRenameExtension(szIniFileName, TEXT(".ini"));
			m_WindowPosition.Left =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowLeft"), m_WindowPosition.Left, szIniFileName);
			m_WindowPosition.Top =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowTop"), m_WindowPosition.Top, szIniFileName);
			m_WindowPosition.Width =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowWidth"), m_WindowPosition.Width, szIniFileName);
			m_WindowPosition.Height =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowHeight"), m_WindowPosition.Height, szIniFileName);

			m_fInitialized = true;
		}

		if (m_hwnd == nullptr) {
			static const DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			static const DWORD ExStyle = WS_EX_TOOLWINDOW;

			// �f�t�H���g�̃E�B���h�E�T�C�Y���擾
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				int Width = GRAPH_WIDTH, Height = GRAPH_HEIGHT;
				// DPI�ݒ�ɍ��킹�ăX�P�[�����O
				int DPI;
				if (m_pApp->GetSetting(L"DPI", &DPI) && DPI != 96) {
					Width = ::MulDiv(Width, DPI, 96);
					Height = ::MulDiv(Height, DPI, 96);
				}
				RECT rc = {0, 0, Width, Height};
				::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
				if (m_WindowPosition.Width <= 0)
					m_WindowPosition.Width = rc.right - rc.left;
				if (m_WindowPosition.Height <= 0)
					m_WindowPosition.Height = rc.bottom - rc.top;
			}

			// �E�B���h�E�̍쐬
			if (::CreateWindowEx(
					ExStyle, WINDOW_CLASS_NAME, TEXT("Signal Graph"), Style,
					0, 0, m_WindowPosition.Width, m_WindowPosition.Height,
					m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;

			WINDOWPLACEMENT wp;
			wp.length = sizeof(WINDOWPLACEMENT);
			::GetWindowPlacement(m_hwnd, &wp);
			wp.flags = 0;
			wp.showCmd = SW_SHOWNOACTIVATE;
			wp.rcNormalPosition.left = m_WindowPosition.Left;
			wp.rcNormalPosition.top = m_WindowPosition.Top;
			wp.rcNormalPosition.right = m_WindowPosition.Left + m_WindowPosition.Width;
			wp.rcNormalPosition.bottom = m_WindowPosition.Top + m_WindowPosition.Height;
			::SetWindowPlacement(m_hwnd, &wp);
		} else {
			AdjustSignalLevelScale();
			::ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
		}
		::UpdateWindow(m_hwnd);
	} else {
		if (m_hwnd != nullptr)
			::ShowWindow(m_hwnd, SW_HIDE);
	}

	return true;
}


// �O���t��`��
void CSignalGraph::DrawGraph(Gdiplus::Graphics &Graphics, int Width, int Height)
{
	Graphics.Clear(m_BackColor);

	const float PenScale = (float)Height / (float)GRAPH_HEIGHT;

	// �O���b�h��`��
	if (m_pGridPen == nullptr)
		m_pGridPen = new Gdiplus::Pen(m_GridColor, PenScale);
	else
		m_pGridPen->SetWidth(PenScale);
	for (int i = 0; i < 10; i++) {
		float y = (float)(Height * (i + 1)) / 10.0f - 1.0f;
		Graphics.DrawLine(m_pGridPen, 0.0f, y, (float)Width, y);
	}

	const int ListSize = (int)m_List.size();
	const int NumPoints = min(ListSize, GRAPH_WIDTH);
	Gdiplus::PointF *pPoints = new Gdiplus::PointF[NumPoints + 3];
	const float XScale = (float)Width / (float)GRAPH_WIDTH;
	const float YScale = (float)(Height - 1);
	int i, x;

	// �r�b�g���[�g��`��
	if (GRAPH_WIDTH > ListSize) {
		x = GRAPH_WIDTH - ListSize;
		i = 0;
	} else {
		x = 0;
		i = ListSize - GRAPH_WIDTH;
	}
	const float BitRateScale = YScale / (float)m_BitRateScale;
	for (int j = 0; i < ListSize; i++, j++) {
		float y = (float)min(m_List[i].BitRate, m_BitRateScale) * BitRateScale;
		pPoints[j].X = (float)x * XScale;
		if (j == 0) {
			pPoints[0].Y = (float)Height;
			pPoints[1].X = pPoints[0].X;
			j++;
		}
		pPoints[j].Y = YScale - y;
		x++;
	}
	pPoints[NumPoints + 1] = pPoints[NumPoints];
	pPoints[NumPoints + 1].X += XScale;
	pPoints[NumPoints + 2].X = pPoints[NumPoints + 1].X;
	pPoints[NumPoints + 2].Y = (float)Height;
	Gdiplus::GraphicsPath Path(Gdiplus::FillModeAlternate);
	Path.AddLines(pPoints, NumPoints + 3);
	if (m_pBrush == nullptr)
		m_pBrush = new Gdiplus::SolidBrush(m_BitRateColor);
	else
		m_pBrush->SetColor(m_BitRateColor);
	Graphics.FillPath(m_pBrush, &Path);
	if (m_pBitRatePen == nullptr) {
		m_pBitRatePen = new Gdiplus::Pen(m_BitRateColor, PenScale);
		m_pBitRatePen->SetStartCap(Gdiplus::LineCapRound);
		m_pBitRatePen->SetEndCap(Gdiplus::LineCapRound);
		m_pBitRatePen->SetLineJoin(Gdiplus::LineJoinMiter);
	} else {
		m_pBitRatePen->SetWidth(PenScale);
	}
	Graphics.DrawLines(m_pBitRatePen, &pPoints[1], NumPoints + 1);

	// �M�����x����`��
	if (GRAPH_WIDTH > ListSize) {
		x = GRAPH_WIDTH - ListSize;
		i = 0;
	} else {
		x = 0;
		i = ListSize - GRAPH_WIDTH;
	}
	const float SignalLevelScale = YScale / m_ActualSignalLevelScale;
	for (int j = 0; i < ListSize; i++, j++) {
		float y = min(m_List[i].SignalLevel, m_ActualSignalLevelScale) * SignalLevelScale;
		pPoints[j].X = (float)x * XScale;
		pPoints[j].Y = YScale - y;
		x++;
	}
	pPoints[NumPoints] = pPoints[NumPoints - 1];
	pPoints[NumPoints].X += XScale;
	if (m_pSignalLevelPen == nullptr) {
		m_pSignalLevelPen = new Gdiplus::Pen(m_SignalLevelColor, PenScale);
		m_pSignalLevelPen->SetStartCap(Gdiplus::LineCapRound);
		m_pSignalLevelPen->SetEndCap(Gdiplus::LineCapRound);
		m_pSignalLevelPen->SetLineJoin(Gdiplus::LineJoinMiter);
	} else {
		m_pSignalLevelPen->SetWidth(PenScale);
	}
	Graphics.DrawLines(m_pSignalLevelPen, pPoints, NumPoints + 1);

	delete [] pPoints;

	// �r�b�g���[�g�ƐM�����x���̕������`��
	const Gdiplus::REAL FontHeight = m_pFont->GetHeight(96.0f);
	const Gdiplus::REAL TextMargin = (Gdiplus::REAL)std::floor(FontHeight / 8.0f);
	Gdiplus::PointF Pos(TextMargin, TextMargin);
	const SignalInfo &Info = m_List.back();
	WCHAR szText[64];
	::StringCchPrintf(szText, _countof(szText), L"%.2f dB", Info.SignalLevel);
	m_pBrush->SetColor(Gdiplus::Color(m_SignalLevelColor.GetValue() | 0xFF000000U));
	Graphics.DrawString(szText, -1, m_pFont, Pos, m_pBrush);
	::StringCchPrintf(szText, _countof(szText), L"%.2f Mbps", (double)Info.BitRate / (double)(1000 * 1000));
	Pos.Y += FontHeight;
	m_pBrush->SetColor(Gdiplus::Color(m_BitRateColor.GetValue() | 0xFF000000U));
	Graphics.DrawString(szText, -1, m_pFont, Pos, m_pBrush);
}


// WM_PAINT �̏���
void CSignalGraph::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	RECT rcClient;

	::BeginPaint(hwnd, &ps);
	::GetClientRect(hwnd, &rcClient);

	if (m_pOffscreenImage == nullptr
			|| m_pOffscreenImage->GetWidth() < (UINT)rcClient.right
			|| m_pOffscreenImage->GetHeight() < (UINT)rcClient.bottom) {
		SafeDelete(m_pOffscreen);
		delete m_pOffscreenImage;
		m_pOffscreenImage = new Gdiplus::Bitmap(rcClient.right, rcClient.bottom, PixelFormat32bppPARGB);
		if (m_pOffscreenImage != nullptr)
			m_pOffscreen = new Gdiplus::Graphics(m_pOffscreenImage);
	}

	if (m_pFont == nullptr)
		m_pFont = new Gdiplus::Font(ps.hdc, &m_Font);

	const Gdiplus::Rect PaintRect(
		ps.rcPaint.left, ps.rcPaint.top,
		ps.rcPaint.right - ps.rcPaint.left,
		ps.rcPaint.bottom - ps.rcPaint.top);

	if (m_pOffscreen != nullptr) {
		m_pOffscreen->SetPageUnit(Gdiplus::UnitPixel);
		m_pOffscreen->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		m_pOffscreen->SetTextRenderingHint(Gdiplus::TextRenderingHintSystemDefault);
		m_pOffscreen->SetClip(PaintRect);

		DrawGraph(*m_pOffscreen, rcClient.right, rcClient.bottom);

		Gdiplus::Graphics Graphics(ps.hdc);

		Graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		Graphics.DrawImage(
			m_pOffscreenImage,
			PaintRect.X, PaintRect.Y,
			PaintRect.X, PaintRect.Y,
			PaintRect.Width, PaintRect.Height,
			Gdiplus::UnitPixel);
	} else {
		Gdiplus::Graphics Graphics(ps.hdc);

		Graphics.SetClip(PaintRect);
		DrawGraph(Graphics, rcClient.right, rcClient.bottom);
	}

	::EndPaint(hwnd, &ps);
}


// ���\�[�X�����
void CSignalGraph::FreeResources()
{
	SafeDelete(m_pGridPen);
	SafeDelete(m_pSignalLevelPen);
	SafeDelete(m_pBitRatePen);
	SafeDelete(m_pBrush);
	SafeDelete(m_pFont);
	SafeDelete(m_pOffscreen);
	SafeDelete(m_pOffscreenImage);
}


// �M�����x���̃X�P�[���𒲐�
void CSignalGraph::AdjustSignalLevelScale()
{
	double MaxLevel = 0.0;

	for (auto it = m_List.begin(); it != m_List.end(); ++it) {
		if (it->SignalLevel > MaxLevel)
			MaxLevel = it->SignalLevel;
	}

	if (m_SignalLevelScale > MaxLevel)
		m_ActualSignalLevelScale = m_SignalLevelScale;
	else
		m_ActualSignalLevelScale = (float)std::ceil(MaxLevel * 0.1f) * 10.0f;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CSignalGraph::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CSignalGraph *pThis = static_cast<CSignalGraph*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		return pThis->EnablePlugin(lParam1 != 0);

	case TVTest::EVENT_STANDBY:
		// �ҋ@��Ԃ��ω�����
		if (pThis->m_pApp->IsPluginEnabled()) {
			// �ҋ@��Ԃ̎��̓E�B���h�E���B��
			::ShowWindow(pThis->m_hwnd, lParam1 != 0 ? SW_HIDE : SW_SHOW);
		}
		return TRUE;
	}

	return 0;
}


// �E�B���h�E�n���h������this���擾����
CSignalGraph *CSignalGraph::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSignalGraph*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK CSignalGraph::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSignalGraph *pThis = static_cast<CSignalGraph*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			// �t�H���g���擾
			if (!pThis->m_pApp->GetSetting(L"StatusBarFont", &pThis->m_Font)) {
				NONCLIENTMETRICS ncm;
#if WINVER >= 0x0600
				ncm.cbSize = offsetof(NONCLIENTMETRICS, iPaddedBorderWidth);
#else
				ncm.cbSize = sizeof(NONCLIENTMETRICS);
#endif
				::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
				pThis->m_Font = ncm.lfStatusFont;
			}

			pThis->m_ActualSignalLevelScale = pThis->m_SignalLevelScale;

			pThis->m_List.clear();
			pThis->m_List.push_back(SignalInfo());

			// �X�V�p�^�C�}�̐ݒ�
			::SetTimer(hwnd, 1, 1000, nullptr);
		}
		return 0;

	case WM_PAINT:
		{
			CSignalGraph *pThis = GetThis(hwnd);

			pThis->OnPaint(hwnd);
		}
		return 0;

	case WM_TIMER:
		{
			CSignalGraph *pThis = GetThis(hwnd);

			// ���擾&�\���X�V
			TVTest::StatusInfo Status;
			pThis->m_pApp->GetStatus(&Status);
			if (Status.SignalLevel < 0.0f)
				Status.SignalLevel = 0.0f;
			if (pThis->m_List.size() == GRAPH_WIDTH)
				pThis->m_List.pop_front();
			pThis->m_List.push_back(SignalInfo(Status.SignalLevel, Status.BitRate));

			if (pThis->m_ActualSignalLevelScale < Status.SignalLevel)
				pThis->m_ActualSignalLevelScale = (float)std::ceil(Status.SignalLevel * 0.1f) * 10.0f;

			::InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// ���鎞�̓v���O�C���𖳌��ɂ���
			CSignalGraph *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CSignalGraph *pThis = GetThis(hwnd);

			pThis->FreeResources();

			// �E�B���h�E�ʒu�ۑ�
			WINDOWPLACEMENT wp;
			wp.length = sizeof (WINDOWPLACEMENT);
			if (::GetWindowPlacement(hwnd, &wp)) {
				pThis->m_WindowPosition.Left = wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Top = wp.rcNormalPosition.top;
				pThis->m_WindowPosition.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			}

			pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSignalGraph;
}
