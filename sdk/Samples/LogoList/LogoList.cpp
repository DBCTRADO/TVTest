/*
	TVTest �v���O�C���T���v��

	���S�̈ꗗ��\������

	���̃T���v���ł͎�Ɉȉ��̋@�\���������Ă��܂��B

	�E�T�[�r�X��񋓂���
	�E�ǃ��S���擾����
	�E�E�B���h�E��\������
	�E�z�F���擾���A�z�F�̕ύX�ɒǏ]����
	�EDPI �ɉ����ăX�P�[�����O����
*/


#include <windows.h>
#include <tchar.h>
#include <vector>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// �N���X�Ƃ��Ď���
#include "TVTestPlugin.h"


// �E�B���h�E�N���X��
#define LOGO_LIST_WINDOW_CLASS TEXT("TV Logo List Window")

// �X�V�p�^�C�}�[�̎��ʎq
#define TIMER_UPDATELOGO	1

// ���S�̑傫��
static const struct {
	int Width, Height;
} LogoSizeList[] = {
	{48, 24},	// logo_type 0
	{36, 24},	// logo_type 1
	{48, 27},	// logo_type 2
	{72, 36},	// logo_type 3
	{54, 36},	// logo_type 4
	{64, 36},	// logo_type 5
};


// �v���O�C���N���X
class CLogoList : public TVTest::CTVTestPlugin
{
	HWND m_hwnd;
	HWND m_hwndList;
	struct Position {
		int Left,Top,Width,Height;
		Position() : Left(0), Top(0), Width(0), Height(0) {}
	};
	Position m_WindowPosition;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	int m_DPI;
	int m_ItemMargin;
	int m_LogoMargin;
	int m_ServiceNameWidth;
	int m_ItemWidth;
	int m_ItemHeight;
	HFONT m_hfont;
	HBRUSH m_hbrBack;

	class CServiceInfo {
	public:
		TCHAR m_szServiceName[64];
		WORD m_NetworkID;
		WORD m_ServiceID;
		HBITMAP m_hbmLogo[6];
		CServiceInfo(const TVTest::ChannelInfo &ChInfo);
		~CServiceInfo();
	};
	std::vector<CServiceInfo*> m_ServiceList;

	bool Enable(bool fEnable);
	void GetServiceList();
	bool UpdateLogo();
	void ClearServiceList();
	void GetColors();
	void CalcMetrics();

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CLogoList *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CLogoList();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CLogoList::CLogoList()
	: m_hwnd(NULL)
	, m_hwndList(NULL)
	, m_hfont(NULL)
	, m_hbrBack(NULL)
{
}


// �v���O�C���̏���Ԃ�
bool CLogoList::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_DISABLEONSTART;
	pInfo->pszPluginName  = L"�ǃ��S�̈ꗗ";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�ǃ��S���ꗗ�\�����܂��B";
	return true;
}


// ����������
bool CLogoList::Initialize()
{
	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// �I������
bool CLogoList::Finalize()
{
	// �E�B���h�E��j������
	if (m_hwnd)
		::DestroyWindow(m_hwnd);

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CLogoList::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CLogoList *pThis=static_cast<CLogoList*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		return pThis->Enable(lParam1 != 0);

	case TVTest::EVENT_STANDBY:
		// �ҋ@��Ԃ��ω�����
		if (pThis->m_pApp->IsPluginEnabled()) {
			// �ҋ@��Ԃ̎��̓E�B���h�E���B��
			::ShowWindow(pThis->m_hwnd, lParam1 != 0 ? SW_HIDE : SW_SHOW);
		}
		return TRUE;

	case TVTest::EVENT_COLORCHANGE:
		// �F�̐ݒ肪�ω�����
		if (pThis->m_hwndList != NULL) {
			pThis->GetColors();
			::RedrawWindow(pThis->m_hwndList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;
	}

	return 0;
}


// �L����Ԃ��ς�������̏���
bool CLogoList::Enable(bool fEnable)
{
	if (fEnable) {
		static bool fInitialized = false;

		if (!fInitialized) {
			// �E�B���h�E�N���X�̓o�^
			WNDCLASS wc;

			wc.style         = 0;
			wc.lpfnWndProc   = WndProc;
			wc.cbClsExtra    = 0;
			wc.cbWndExtra    = 0;
			wc.hInstance     = g_hinstDLL;
			wc.hIcon         = NULL;
			wc.hCursor       = ::LoadCursor(NULL,IDC_ARROW);
			wc.hbrBackground = NULL;
			wc.lpszMenuName  = NULL;
			wc.lpszClassName = LOGO_LIST_WINDOW_CLASS;
			if (::RegisterClass(&wc) == 0)
				return false;
			fInitialized = true;
		}

		if (m_hwnd == NULL) {
			// �E�B���h�E�̍쐬
			const DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			const DWORD ExStyle = WS_EX_TOOLWINDOW;
			if (::CreateWindowEx(ExStyle, LOGO_LIST_WINDOW_CLASS,
								 TEXT("�ǃ��S�̈ꗗ"), Style,
								 0, 0, 320, 320,
								 m_pApp->GetAppWindow(), NULL, g_hinstDLL, this) == NULL)
				return false;

			// �f�t�H���g�T�C�Y�̌v�Z
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				RECT rc;
				rc.left = 0;
				rc.top = 0;
				rc.right = m_ItemWidth + ::GetSystemMetrics(SM_CXVSCROLL);
				rc.bottom = m_ItemHeight * 8;
				::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
				if (m_WindowPosition.Width <= 0)
					m_WindowPosition.Width = rc.right - rc.left;
				if (m_WindowPosition.Height <= 0)
					m_WindowPosition.Height = rc.bottom - rc.top;
			}

			// �E�B���h�E�ʒu�̕���
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
		}

		::ShowWindow(m_hwnd, SW_SHOWNORMAL);
	} else {
		// �E�B���h�E�̔j��
		if (m_hwnd)
			::DestroyWindow(m_hwnd);
	}

	return true;
}


// �e�T�[�r�X�̃��S�̎擾
void CLogoList::GetServiceList()
{
	ClearServiceList();

	// �T�[�r�X�̃��X�g���擾����
	int NumSpaces = 0;
	int CurTuningSpace = m_pApp->GetTuningSpace(&NumSpaces);

	TVTest::ChannelInfo ChInfo;
	CServiceInfo *pServiceInfo;
	if (CurTuningSpace >= 0) {
		// ���݂̃`���[�j���O��Ԃ̃`�����l�����擾����
		for (int Channel = 0; m_pApp->GetChannelInfo(CurTuningSpace, Channel, &ChInfo); Channel++) {
			pServiceInfo = new CServiceInfo(ChInfo);
			m_ServiceList.push_back(pServiceInfo);
		}
	} else {
		// �S�Ẵ`���[�j���O��Ԃ̃`�����l�����擾����
		for (int Space = 0; Space < NumSpaces; Space++) {
			for (int Channel = 0; m_pApp->GetChannelInfo(Space, Channel, &ChInfo); Channel++) {
				pServiceInfo = new CServiceInfo(ChInfo);
				m_ServiceList.push_back(pServiceInfo);
			}
		}
	}

	// ���S���擾����
	UpdateLogo();
}


// ���S�̍X�V
bool CLogoList::UpdateLogo()
{
	bool fUpdated = false;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		CServiceInfo *pServiceInfo = m_ServiceList[i];

		UINT ExistsType = 0;
		for (BYTE j = 0; j < 6; j++) {
			if (pServiceInfo->m_hbmLogo[j] != NULL)
				ExistsType |= 1U << j;
		}
		if ((ExistsType & 0x3F) != 0x3F) {
			// �܂��擾���Ă��Ȃ����S������
			UINT AvailableType =
				m_pApp->GetAvailableLogoType(pServiceInfo->m_NetworkID, pServiceInfo->m_ServiceID);
			if (AvailableType != ExistsType) {
				// �V�������S���擾���ꂽ�̂ōX�V����
				for (BYTE j = 0; j < 6; j++) {
					if (pServiceInfo->m_hbmLogo[j] == NULL
							&& (AvailableType & (1U << j)) != 0) {
						pServiceInfo->m_hbmLogo[j] =
							m_pApp->GetLogo(pServiceInfo->m_NetworkID, pServiceInfo->m_ServiceID, j);
						if (pServiceInfo->m_hbmLogo[j] != NULL)
							fUpdated = true;
					}
				}
			}
		}
	}

	return fUpdated;
}


// �T�[�r�X�̃��X�g���N���A����
void CLogoList::ClearServiceList()
{
	for (size_t i = 0; i < m_ServiceList.size(); i++)
		delete m_ServiceList[i];
	m_ServiceList.clear();
}


// �z�F���擾����
void CLogoList::GetColors()
{
	m_crBackColor = m_pApp->GetColor(L"PanelBack");
	m_crTextColor = m_pApp->GetColor(L"PanelText");

	if (m_hbrBack != NULL)
		::DeleteObject(m_hbrBack);
	m_hbrBack = ::CreateSolidBrush(m_crBackColor);
}


// ���@���v�Z����
void CLogoList::CalcMetrics()
{
	LOGFONT lf;
	m_pApp->GetFont(L"PanelFont", &lf, m_DPI);
	if (m_hfont != NULL)
		::DeleteObject(m_hfont);
	m_hfont = ::CreateFontIndirect(&lf);

	HDC hdc = ::GetDC(m_hwnd);
	HGDIOBJ hOldFont = ::SelectObject(hdc, m_hfont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	::SelectObject(hdc, hOldFont);
	::ReleaseDC(m_hwnd, hdc);

	m_ItemMargin = ::MulDiv(2, m_DPI, 96);
	m_LogoMargin = ::MulDiv(4, m_DPI, 96);
	m_ServiceNameWidth = tm.tmAveCharWidth * 20;

	int LogoWidth = 0;
	for (int i = 0; i < 6; i++)
		LogoWidth += LogoSizeList[i].Width;
	m_ItemWidth = m_ServiceNameWidth + (m_ItemMargin * 2) + (m_LogoMargin * 6) + LogoWidth;
	m_ItemHeight = max(36, tm.tmHeight) + (m_ItemMargin * 2);
}


// �E�B���h�E�n���h������this���擾����
CLogoList *CLogoList::GetThis(HWND hwnd)
{
	return reinterpret_cast<CLogoList*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK CLogoList::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CLogoList *pThis = static_cast<CLogoList*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			pThis->m_DPI = pThis->m_pApp->GetDPIFromWindow(hwnd);
			if (pThis->m_DPI == 0)
				pThis->m_DPI = 96;

			pThis->CalcMetrics();

			pThis->m_hwndList = ::CreateWindowEx(0, TEXT("LISTBOX"), NULL,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT,
				0, 0, 0, 0, hwnd, NULL, g_hinstDLL, NULL);

			pThis->GetColors();

			pThis->GetServiceList();

			// �A�C�e���̑傫����ݒ肷��
			::SendMessage(pThis->m_hwndList, LB_SETITEMHEIGHT, 0, pThis->m_ItemHeight);
			::SendMessage(pThis->m_hwndList, LB_SETHORIZONTALEXTENT, pThis->m_ItemWidth, 0);

			for (size_t i = 0; i < pThis->m_ServiceList.size(); i++)
				::SendMessage(pThis->m_hwndList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pThis->m_ServiceList[i]));

			// �X�V�p�^�C�}�[�ݒ�
			::SetTimer(hwnd, TIMER_UPDATELOGO, 60 * 1000, NULL);
		}
		return 0;

	case WM_SIZE:
		{
			CLogoList *pThis = GetThis(hwnd);

			::MoveWindow(pThis->m_hwndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		}
		return 0;

	case WM_DRAWITEM:
		// ���S�̃��X�g�̃A�C�e����`��
		{
			CLogoList *pThis = GetThis(hwnd);
			LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			::FillRect(pdis->hDC, &pdis->rcItem, pThis->m_hbrBack);
			if ((int)pdis->itemID < 0 || pdis->itemID >= pThis->m_ServiceList.size())
				return TRUE;

			const CServiceInfo *pService = pThis->m_ServiceList[pdis->itemID];

			HFONT hfontOld = static_cast<HFONT>(::SelectObject(pdis->hDC, pThis->m_hfont));
			int OldBkMode = ::SetBkMode(pdis->hDC, TRANSPARENT);
			COLORREF OldTextColor = ::SetTextColor(pdis->hDC, pThis->m_crTextColor);

			RECT rc;

			rc.left = pdis->rcItem.left + pThis->m_ItemMargin;
			rc.top = pdis->rcItem.top + pThis->m_ItemMargin;
			rc.right = rc.left + pThis->m_ServiceNameWidth;
			rc.bottom = pdis->rcItem.bottom - pThis->m_ItemMargin;
			::DrawText(pdis->hDC, pService->m_szServiceName, -1, &rc,
					   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

			::SetTextColor(pdis->hDC, OldTextColor);
			::SetBkMode(pdis->hDC, OldBkMode);
			::SelectObject(pdis->hDC, hfontOld);

			HDC hdcMemory = ::CreateCompatibleDC(pdis->hDC);
			HGDIOBJ hOldBitmap = ::GetCurrentObject(hdcMemory, OBJ_BITMAP);
			int x = rc.right + pThis->m_LogoMargin;
			for (int i = 0; i < 6; i++) {
				if (pService->m_hbmLogo[i] != NULL) {
					::SelectObject(hdcMemory, pService->m_hbmLogo[i]);
					::BitBlt(pdis->hDC,
							 x, rc.top + ((rc.bottom - rc.top) - LogoSizeList[i].Height) / 2,
							 LogoSizeList[i].Width, LogoSizeList[i].Height,
							 hdcMemory, 0, 0, SRCCOPY);
				}
				x += LogoSizeList[i].Width + pThis->m_LogoMargin;
			}
			if (::GetCurrentObject(hdcMemory, OBJ_BITMAP) != hOldBitmap)
				::SelectObject(hdcMemory, hOldBitmap);
			::DeleteDC(hdcMemory);
		}
		return TRUE;

	case WM_CTLCOLORLISTBOX:
		{
			CLogoList *pThis = GetThis(hwnd);

			return reinterpret_cast<LRESULT>(pThis->m_hbrBack);
		}

	case WM_TIMER:
		// ���S�̍X�V
		if (wParam == TIMER_UPDATELOGO) {
			CLogoList *pThis = GetThis(hwnd);

			if (pThis->UpdateLogo())
				::RedrawWindow(pThis->m_hwndList, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// ���鎞�̓v���O�C���𖳌��ɂ���
			CLogoList *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
	case WM_DPICHANGED:
		// DPI ���ς����
		{
			CLogoList *pThis = GetThis(hwnd);
			const RECT *prc = reinterpret_cast<const RECT*>(lParam);

			pThis->m_DPI = HIWORD(wParam);

			pThis->CalcMetrics();

			::SendMessage(pThis->m_hwndList, LB_SETITEMHEIGHT, 0, pThis->m_ItemHeight);
			::SendMessage(pThis->m_hwndList, LB_SETHORIZONTALEXTENT, pThis->m_ItemWidth, 0);

			::SetWindowPos(
				hwnd, NULL,
				prc->left, prc->top,
				prc->right - prc->left, prc->bottom - prc->top,
				SWP_NOZORDER | SWP_NOACTIVATE);
			::InvalidateRect(pThis->m_hwndList, NULL, TRUE);
		}
		break;

	case WM_NCDESTROY:
		{
			CLogoList *pThis = GetThis(hwnd);

			// �E�B���h�E�ʒu�̋L��
			WINDOWPLACEMENT wp;
			wp.length = sizeof (WINDOWPLACEMENT);
			if (::GetWindowPlacement(hwnd, &wp)) {
				pThis->m_WindowPosition.Left = wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Top = wp.rcNormalPosition.top;
				pThis->m_WindowPosition.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			}

			// ���\�[�X���
			if (pThis->m_hfont != NULL) {
				::DeleteObject(pThis->m_hfont);
				pThis->m_hfont = NULL;
			}
			if (pThis->m_hbrBack != NULL) {
				::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack = NULL;
			}

			pThis->m_hwnd = NULL;
			pThis->m_hwndList = NULL;
			pThis->ClearServiceList();
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




CLogoList::CServiceInfo::CServiceInfo(const TVTest::ChannelInfo &ChInfo)
{
	::lstrcpy(m_szServiceName, ChInfo.szChannelName);
	m_NetworkID = ChInfo.NetworkID;
	m_ServiceID = ChInfo.ServiceID;
	for (int i = 0; i < 6; i++)
		m_hbmLogo[i] = NULL;
}


CLogoList::CServiceInfo::~CServiceInfo()
{
	for (int i = 0; i < 6; i++) {
		if (m_hbmLogo[i] != NULL)
			::DeleteObject(m_hbmLogo[i]);
	}
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CLogoList;
}
