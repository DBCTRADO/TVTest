/*
	TVTest �v���O�C���T���v��

	�`���[�i�[/�`�����l����I������p�l����\������
*/


#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <tchar.h>
#include <string>
#include <vector>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")


static void OffsetFileTime(FILETIME *pTime, LONGLONG Offset)
{
	ULARGE_INTEGER uli;

	uli.LowPart = pTime->dwLowDateTime;
	uli.HighPart = pTime->dwHighDateTime;
	uli.QuadPart += Offset;
	pTime->dwLowDateTime = uli.LowPart;
	pTime->dwHighDateTime = uli.HighPart;
}


// �v���O�C���N���X
class CTunerPanel : public TVTest::CTVTestPlugin
{
public:
	CTunerPanel();
	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;

private:
	enum ViewModeType {
		VIEW_MODE_LIST,
		VIEW_MODE_LOGO
	};
	static const ViewModeType VIEW_MODE_FIRST = VIEW_MODE_LIST;
	static const ViewModeType VIEW_MODE_LAST  = VIEW_MODE_LOGO;

	enum LogoSizeType {
		LOGO_SIZE_SMALL,
		LOGO_SIZE_MEDIUM,
		LOGO_SIZE_LARGE,
		LOGL_SIZE_EXTRALARGE
	};
	static const LogoSizeType LOGO_SIZE_FIRST = LOGO_SIZE_SMALL;
	static const LogoSizeType LOGO_SIZE_LAST  = LOGL_SIZE_EXTRALARGE;

	struct Bitmap
	{
		Bitmap() : m_hbm(NULL) {}
		~Bitmap() { Delete(); }
		Bitmap(const Bitmap &Src) : m_hbm(NULL) { *this = Src; }
		Bitmap &operator=(const Bitmap &Src) {
			if (&Src != this) {
				Delete();
				if (Src.m_hbm != NULL)
					m_hbm = (HBITMAP)::CopyImage(Src.m_hbm, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
			}
			return *this;
		}
		Bitmap &operator=(HBITMAP hbm) {
			Delete();
			m_hbm = hbm;
			return *this;
		}
		operator HBITMAP() const { return m_hbm; }
		operator bool() const { return m_hbm != NULL; }
		void Delete() {
			if (m_hbm != NULL) {
				::DeleteObject(m_hbm);
				m_hbm = NULL;
			}
		}

	private:
		HBITMAP m_hbm;
	};

	struct ChannelInfo
	{
		std::wstring Name;
		int Space;
		int Channel;
		int RemoteControlKeyID;
		WORD NetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
		Bitmap LogoBitmap;
	};

	struct TuningSpaceInfo
	{
		std::wstring Name;
		std::vector<ChannelInfo> ChannelList;
	};

	struct TunerInfo
	{
		std::wstring Name;
		std::vector<TuningSpaceInfo> TuningSpaceList;
		bool fExpandable;
		bool fExpanded;
	};

	struct Margins
	{
		int Left, Top, Right, Bottom;

		Margins() : Left(0), Top(0), Right(0), Bottom(0) {}
	};

	enum PartType {
		PART_NONE,
		PART_TUNER,
		PART_CHANNEL,
		PART_CHEVRON
	};

	struct HitTestInfo
	{
		PartType Part;
		int Tuner;
		int Space;
		int Channel;
	};

	static const int PANEL_ID = 1;
	static const LPCTSTR WINDOW_CLASS_NAME;

	WCHAR m_szIniFileName[MAX_PATH];
	std::vector<TunerInfo> m_TunerList;
	std::vector<std::wstring> m_ExpandedTunerList;
	ViewModeType m_ViewMode;
	LogoSizeType m_LogoSize;
	bool m_fEnableByPlugin;
	HWND m_hwnd;
	HWND m_hwndToolTips;
	HFONT m_hFont;
	int m_FontHeight;
	int m_ScrollPos;
	Margins m_TunerItemMargin;
	int m_TunerItemHeight;
	Margins m_ChannelItemMargin;
	int m_ChannelItemHeight;
	int m_ChevronWidth;
	int m_ChevronMargin;
	int m_LogoWidth;
	int m_LogoHeight;
	Margins m_LogoMargin;
	int m_ItemWidth;
	int m_ItemHeight;
	int m_ColumnCount;
	bool m_fPointCursor;
	HitTestInfo m_HotItem;
	WCHAR m_szToolTipBuffer[1024];

	void UpdateContent();
	void GetTunerList();
	void UpdateExpandedTunerList();
	void SetPanelItemState(DWORD Mask, DWORD State);
	bool IsPanelItemEnabled() const;
	LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnCommand(int ID);
	void InitializePanel();
	void Draw(HDC hdc, const RECT &PaintRect);
	int CalcVertExtent() const;
	int GetColumnCount() const;
	SIZE GetLogoSize(LogoSizeType Type) const;
	void UpdateItemSize();
	void UpdateScroll();
	void SetScrollPos(int Pos);
	bool GetItemRect(int Tuner, int Space, int Channel, RECT *pRect) const;
	void CalcChannelItemRect(int Channel, RECT *pRect) const;
	int CalcChannelItemRows(int Channels) const;
	bool HitTest(int x, int y, HitTestInfo *pInfo) const;
	void UpdateToolTips();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


const LPCTSTR CTunerPanel::WINDOW_CLASS_NAME = TEXT("Tuner Panel Window");


CTunerPanel::CTunerPanel()
	: m_ViewMode(VIEW_MODE_LIST)
	, m_LogoSize(LOGO_SIZE_MEDIUM)
	, m_fEnableByPlugin(false)
	, m_hwnd(NULL)
	, m_hwndToolTips(NULL)
{
}


// �v���O�C���̏���Ԃ�
bool CTunerPanel::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"�`���[�i�[�p�l��";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�`���[�i�[/�`�����l����I������p�l����\�����܂��B";
	return true;
}


// ����������
bool CTunerPanel::Initialize()
{
	// �p�l�����ڂ�o�^����
	TVTest::PanelItemInfo PanelInfo;

	PanelInfo.Size = sizeof(TVTest::PanelItemInfo);
	PanelInfo.Flags = 0;
	PanelInfo.Style = TVTest::PANEL_ITEM_STYLE_NEEDFOCUS;
	PanelInfo.ID = PANEL_ID;
	PanelInfo.pszIDText = L"Tuner";
	PanelInfo.pszTitle = L"�`���[�i�[";
	PanelInfo.hbmIcon = (HBITMAP)::LoadImage(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON),
											 IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

	bool fResult = m_pApp->RegisterPanelItem(&PanelInfo);
	::DeleteObject(PanelInfo.hbmIcon);
	if (!fResult) {
		m_pApp->AddLog(L"�p�l�����ڂ�o�^�ł��܂���B", TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// �E�B���h�E�N���X��o�^����
	WNDCLASS wc;

	wc.style = CS_HREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = g_hinstDLL;
	wc.hIcon = NULL;
	wc.hCursor = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASS_NAME;
	if (::RegisterClass(&wc) == 0)
		return false;

	// �ݒ�t�@�C�������擾����
	::GetModuleFileNameW(g_hinstDLL, m_szIniFileName, _countof(m_szIniFileName));
	::PathRenameExtensionW(m_szIniFileName, L".ini");

	// �ݒ��ǂݍ���
	int ViewMode = ::GetPrivateProfileIntW(L"Settings", L"ViewMode", (int)m_ViewMode, m_szIniFileName);
	if (ViewMode >= VIEW_MODE_FIRST && ViewMode <= VIEW_MODE_LAST)
		m_ViewMode = (ViewModeType)ViewMode;
	int LogoSize = ::GetPrivateProfileIntW(L"Settings", L"LogoSize", (int)m_LogoSize, m_szIniFileName);
	if (LogoSize >= LOGO_SIZE_FIRST && LogoSize <= LOGO_SIZE_LAST)
		m_LogoSize = (LogoSizeType)LogoSize;

	for (int i = 0; ; i++) {
		WCHAR szKey[32], szTuner[MAX_PATH];
		::wsprintfW(szKey, L"Tuner%d", i);
		if (::GetPrivateProfileStringW(
				L"ExpandedTuners", szKey, NULL,
				szTuner, _countof(szTuner), m_szIniFileName) < 1)
			break;
		m_ExpandedTunerList.push_back(std::wstring(szTuner));
	}

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// �I������
bool CTunerPanel::Finalize()
{
	// �E�B���h�E�̔j��
	if (m_hwnd != NULL)
		::DestroyWindow(m_hwnd);

	// �ݒ��ۑ�����
	struct IntString {
		IntString(int Value) { ::wsprintfW(m_szBuffer, L"%d", Value); }
		operator LPCWSTR() const { return m_szBuffer; }
		WCHAR m_szBuffer[16];
	};

	::WritePrivateProfileStringW(L"Settings", L"ViewMode", IntString((int)m_ViewMode), m_szIniFileName);
	::WritePrivateProfileStringW(L"Settings", L"LogoSize", IntString((int)m_LogoSize), m_szIniFileName);

	std::wstring ExpandedTuners;
	for (int i = 0; i < (int)m_ExpandedTunerList.size(); i++) {
		WCHAR szKey[32];
		::wsprintfW(szKey, L"Tuner%d=", i);
		ExpandedTuners += szKey;
		ExpandedTuners += m_ExpandedTunerList[i];
		ExpandedTuners += L'\0';
	}
	ExpandedTuners += L'\0';
	::WritePrivateProfileSectionW(L"ExpandedTuners", ExpandedTuners.data(), m_szIniFileName);

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CTunerPanel::EventCallback(
	UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CTunerPanel *pThis = static_cast<CTunerPanel*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		if (!pThis->m_fEnableByPlugin) {
			// �v���O�C���̗L����Ԃɍ��킹�ăp�l���̗L����Ԃ�ݒ肷��
			if (lParam1 != 0) {
				pThis->SetPanelItemState(
					TVTest::PANEL_ITEM_STATE_ENABLED |
					TVTest::PANEL_ITEM_STATE_ACTIVE,
					TVTest::PANEL_ITEM_STATE_ENABLED |
					TVTest::PANEL_ITEM_STATE_ACTIVE);
			} else {
				pThis->SetPanelItemState(
					TVTest::PANEL_ITEM_STATE_ENABLED,
					0);
			}
		}
		return TRUE;

	case TVTest::EVENT_DRIVERCHANGE:
		// �`���[�i�[���ύX���ꂽ
	case TVTest::EVENT_CHANNELCHANGE:
		// �`�����l�����ύX���ꂽ
	case TVTest::EVENT_SERVICECHANGE:
		// �T�[�r�X���ύX���ꂽ
	case TVTest::EVENT_COLORCHANGE:
		// �F�̐ݒ肪�ω�����
		if (pThis->m_hwnd != NULL)
			::InvalidateRect(pThis->m_hwnd, NULL, TRUE);
		return TRUE;

	case TVTest::EVENT_SETTINGSCHANGE:
		// �ݒ肪�ύX���ꂽ
		if (pThis->m_hwnd != NULL) {
			pThis->InitializePanel();
			pThis->UpdateContent();
		}
		return TRUE;

	case TVTest::EVENT_PANELITEM_NOTIFY:
		// �p�l�����ڂ̒ʒm
		{
			TVTest::PanelItemEventInfo *pInfo =
				reinterpret_cast<TVTest::PanelItemEventInfo*>(lParam1);

			switch (pInfo->Event) {
			case TVTest::PANEL_ITEM_EVENT_CREATE:
				// �p�l�����ڂ��쐬����
				{
					TVTest::PanelItemCreateEventInfo *pCreateInfo =
						reinterpret_cast<TVTest::PanelItemCreateEventInfo*>(lParam1);

					HWND hwnd = ::CreateWindowEx(
						0, WINDOW_CLASS_NAME, NULL,
						WS_CHILD | WS_VISIBLE | WS_VSCROLL,
						pCreateInfo->ItemRect.left,
						pCreateInfo->ItemRect.top,
						pCreateInfo->ItemRect.right - pCreateInfo->ItemRect.left,
						pCreateInfo->ItemRect.bottom - pCreateInfo->ItemRect.top,
						pCreateInfo->hwndParent, NULL, g_hinstDLL, pThis);
					if (hwnd == NULL) {
						return FALSE;
					}

					pCreateInfo->hwndItem = hwnd;

					// �p�l���̗L����Ԃɍ��킹�ăv���O�C���̗L����Ԃ�ݒ肷��
					pThis->m_fEnableByPlugin = true;
					pThis->m_pApp->EnablePlugin(pThis->IsPanelItemEnabled());
					pThis->m_fEnableByPlugin = false;
				}
				return TRUE;

			case TVTest::PANEL_ITEM_EVENT_ACTIVATE:
				// �p�l�����ڂ��A�N�e�B�u�ɂȂ�
				if (pThis->m_TunerList.empty())
					pThis->UpdateContent();
				return TRUE;

			case TVTest::PANEL_ITEM_EVENT_ENABLE:
			case TVTest::PANEL_ITEM_EVENT_DISABLE:
				// �p�l�����ڂ��L���ɂȂ�/�����ɂȂ�
				// �p�l���̗L����Ԃɍ��킹�ăv���O�C���̗L����Ԃ�ݒ肷��
				pThis->m_fEnableByPlugin = true;
				pThis->m_pApp->EnablePlugin(pInfo->Event == TVTest::PANEL_ITEM_EVENT_ENABLE);
				pThis->m_fEnableByPlugin = false;
				return TRUE;
			}
		}
		break;
	}

	return 0;
}


// ���e���X�V����
void CTunerPanel::UpdateContent()
{
	GetTunerList();
	UpdateScroll();
	::InvalidateRect(m_hwnd, NULL, TRUE);
}


// �`���[�i�[/�`�����l���̃��X�g���擾����
void CTunerPanel::GetTunerList()
{
	UpdateExpandedTunerList();
	m_TunerList.clear();

	WCHAR szDriver[MAX_PATH];

	for (int i = 0; m_pApp->EnumDriver(i, szDriver, _countof(szDriver)) > 0; i++) {
		m_TunerList.push_back(TunerInfo());
		TunerInfo &Tuner = m_TunerList.back();
		Tuner.Name = szDriver;
		Tuner.fExpandable = false;
		Tuner.fExpanded = false;
		for (auto it = m_ExpandedTunerList.begin(); it != m_ExpandedTunerList.end(); ++it) {
			if (::lstrcmpiW(it->c_str(), szDriver) == 0) {
				Tuner.fExpanded = true;
				break;
			}
		}

		TVTest::DriverTuningSpaceList TuningSpaceList;
		TuningSpaceList.Flags = 0;

		if (m_pApp->GetDriverTuningSpaceList(szDriver, &TuningSpaceList)) {
			Tuner.TuningSpaceList.resize(TuningSpaceList.NumSpaces);

			for (DWORD j = 0; j < TuningSpaceList.NumSpaces; j++) {
				const TVTest::DriverTuningSpaceInfo &DriverTuningSpace = *TuningSpaceList.SpaceList[j];
				TuningSpaceInfo &TuningSpace = Tuner.TuningSpaceList[j];
				TuningSpace.Name = DriverTuningSpace.pInfo->szName;
				TuningSpace.ChannelList.reserve(DriverTuningSpace.NumChannels);

				for (DWORD k = 0; k < DriverTuningSpace.NumChannels; k++) {
					const TVTest::ChannelInfo &DriverChannel = *DriverTuningSpace.ChannelList[k];

					if (!(DriverChannel.Flags & TVTest::CHANNEL_FLAG_DISABLED)) {
						TuningSpace.ChannelList.push_back(ChannelInfo());
						ChannelInfo &Channel = TuningSpace.ChannelList.back();

						Channel.Name = DriverChannel.szChannelName;
						Channel.Space = DriverChannel.Space;
						Channel.Channel = DriverChannel.Channel;
						Channel.RemoteControlKeyID = DriverChannel.RemoteControlKeyID;
						Channel.NetworkID = DriverChannel.NetworkID;
						Channel.TransportStreamID = DriverChannel.TransportStreamID;
						Channel.ServiceID = DriverChannel.ServiceID;

						// �ǃ��S���擾����
						UINT AvailLogos = m_pApp->GetAvailableLogoType(Channel.NetworkID, Channel.ServiceID);
						if (AvailLogos != 0) {
							static const BYTE LogoPriority[6] = {3, 5, 4, 2, 0, 1};	// �傫�����ɗD��
							for (BYTE LogoIndex = 0; LogoIndex <= 5; LogoIndex++) {
								BYTE LogoType = LogoPriority[LogoIndex];
								if (AvailLogos & (1U << LogoType)) {
									Channel.LogoBitmap = m_pApp->GetLogo(Channel.NetworkID, Channel.ServiceID, LogoType);
									if (Channel.LogoBitmap)
										break;
								}
							}
						}
					}
				}

				if (!TuningSpace.ChannelList.empty())
					Tuner.fExpandable = true;
			}

			m_pApp->FreeDriverTuningSpaceList(&TuningSpaceList);
		}
	}
}


// �W�J����Ă���`���[�i�[�̃��X�g���X�V
void CTunerPanel::UpdateExpandedTunerList()
{
	if (!m_TunerList.empty()) {
		m_ExpandedTunerList.clear();

		for (auto it = m_TunerList.begin(); it != m_TunerList.end(); ++it) {
			if (it->fExpanded)
				m_ExpandedTunerList.push_back(it->Name);
		}
	}
}


// �p�l�����ڂ̏�Ԃ�ݒ肷��
void CTunerPanel::SetPanelItemState(DWORD Mask, DWORD State)
{
	TVTest::PanelItemSetInfo Info;

	Info.Size = sizeof(TVTest::PanelItemSetInfo);
	Info.Mask = TVTest::PANEL_ITEM_SET_INFO_MASK_STATE;
	Info.ID = PANEL_ID;
	Info.StateMask = Mask;
	Info.State = State;

	m_pApp->SetPanelItem(&Info);
}


// �p�l�����ڂ��L���ł��邩�擾����
bool CTunerPanel::IsPanelItemEnabled() const
{
	TVTest::PanelItemGetInfo Info;

	Info.Size = sizeof(TVTest::PanelItemGetInfo);
	Info.Mask = TVTest::PANEL_ITEM_GET_INFO_MASK_STATE;
	Info.ID = PANEL_ID;

	return m_pApp->GetPanelItemInfo(&Info)
		&& (Info.State & TVTest::PANEL_ITEM_STATE_ENABLED) != 0;
}


// ���b�Z�[�W�n���h��
LRESULT CTunerPanel::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			m_hwndToolTips = ::CreateWindowEx(
				WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
				0, 0, 0, 0, hwnd, NULL, GetWindowInstance(hwnd), NULL);
			::SendMessage(m_hwndToolTips, TTM_ACTIVATE, TRUE, 0);
			int Delay = (int)::SendMessage(m_hwndToolTips, TTM_GETDELAYTIME, TTDT_AUTOPOP, 0);
			if (Delay < 5000)
				::SendMessage(m_hwndToolTips, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELONG(5000, 0));

			m_ScrollPos = 0;
			InitializePanel();
		}
		return 0;

	case WM_SIZE:
		UpdateItemSize();
		m_ColumnCount = GetColumnCount();
		UpdateScroll();
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd, &ps);
			Draw(ps.hdc, ps.rcPaint);
			::EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			::SetFocus(hwnd);

			HitTestInfo Info;

			if (HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &Info)) {
				switch (Info.Part) {
				case PART_TUNER:
					// �`���[�i�[
					m_pApp->SetDriverName(m_TunerList[Info.Tuner].Name.c_str());
					break;

				case PART_CHANNEL:
					// �`�����l��
					{
						const ChannelInfo &Channel =
							m_TunerList[Info.Tuner].TuningSpaceList[Info.Space].ChannelList[Info.Channel];
						TVTest::ChannelSelectInfo ChannelSel;

						ChannelSel.Size = sizeof (TVTest::ChannelSelectInfo);
						ChannelSel.Flags = 0;
						ChannelSel.pszTuner = m_TunerList[Info.Tuner].Name.c_str();
						ChannelSel.Space = Channel.Space;
						ChannelSel.Channel = Channel.Channel;
						ChannelSel.NetworkID = 0;
						ChannelSel.TransportStreamID = 0;
						ChannelSel.ServiceID = Channel.ServiceID;
						m_pApp->SelectChannel(&ChannelSel);
					}
					break;

				case PART_CHEVRON:
					// �V�F�u����
					{
						TunerInfo &Tuner = m_TunerList[Info.Tuner];
						if (Tuner.fExpandable) {
							Tuner.fExpanded = !Tuner.fExpanded;
							UpdateScroll();
							::InvalidateRect(hwnd, NULL ,TRUE);
						}
					}
					break;
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		::SetFocus(hwnd);
		return 0;

	case WM_RBUTTONUP:
		{
			HMENU hmenu = ::LoadMenu(g_hinstDLL, MAKEINTRESOURCE(IDM_MENU));

			::CheckMenuRadioItem(
				hmenu, CM_VIEW_LIST, CM_VIEW_LOGO,
				m_ViewMode == VIEW_MODE_LIST ? CM_VIEW_LIST : CM_VIEW_LOGO,
				MF_BYCOMMAND);
			::CheckMenuRadioItem(
				hmenu, CM_LOGOSIZE_SMALL, CM_LOGOSIZE_EXTRALARGE,
				CM_LOGOSIZE_SMALL + (int)m_LogoSize, MF_BYCOMMAND);

			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ClientToScreen(hwnd, &pt);

			::TrackPopupMenu(::GetSubMenu(hmenu, 0), TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
			::DestroyMenu(hmenu);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			HitTestInfo HotItem;

			m_fPointCursor = HitTest(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &HotItem);

			if (HotItem.Part != m_HotItem.Part
					|| HotItem.Tuner != m_HotItem.Tuner
					|| HotItem.Space != m_HotItem.Space
					|| HotItem.Channel != m_HotItem.Channel) {
				RECT rc;

				if (m_HotItem.Part == PART_CHANNEL && m_HotItem.Channel >= 0
						&& GetItemRect(m_HotItem.Tuner, m_HotItem.Space, m_HotItem.Channel, &rc))
					::InvalidateRect(hwnd, &rc, TRUE);
				m_HotItem = HotItem;
				if (m_HotItem.Part == PART_CHANNEL && m_HotItem.Channel >= 0
						&& GetItemRect(m_HotItem.Tuner, m_HotItem.Space, m_HotItem.Channel, &rc))
					::InvalidateRect(hwnd, &rc, TRUE);
			}

			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hwnd;
			::TrackMouseEvent(&tme);
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			RECT rc;

			if (m_HotItem.Part == PART_CHANNEL && m_HotItem.Channel >= 0
					&& GetItemRect(m_HotItem.Tuner, m_HotItem.Space, m_HotItem.Channel, &rc))
				::InvalidateRect(hwnd, &rc, TRUE);

			m_HotItem.Part = PART_NONE;
		}
		return 0;

	case WM_SETCURSOR:
		if ((HWND)wParam == hwnd && LOWORD(lParam) == HTCLIENT) {
			::SetCursor(::LoadCursor(NULL, m_fPointCursor ? IDC_HAND : IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_VSCROLL:
		{
			SCROLLINFO si;

			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
			::GetScrollInfo(m_hwnd, SB_VERT, &si);

			int Pos = si.nPos;

			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos -= m_FontHeight;	break;
			case SB_LINEDOWN:	Pos += m_FontHeight;	break;
			case SB_PAGEUP:		Pos -= si.nPage;		break;
			case SB_PAGEDOWN:	Pos += si.nPage;		break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK:	Pos = HIWORD(wParam);	break;
			case SB_TOP:		Pos = 0;				break;
			case SB_BOTTOM:		Pos = si.nMax;			break;
			default:	return 0;
			}

			if (Pos != si.nPos)
				SetScrollPos(Pos);
		}
		return 0;

	case WM_MOUSEWHEEL:
		{
			UINT ScrollLines;
			if (!::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &ScrollLines, 0))
				ScrollLines = 2;
			int Delta = GET_WHEEL_DELTA_WPARAM(wParam);
			int Pos = m_ScrollPos - ::MulDiv(m_FontHeight * (int)ScrollLines, Delta, WHEEL_DELTA);
			SetScrollPos(Pos);
		}
		return 0;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TTN_GETDISPINFO:
			{
				// �c�[���`�b�v�̃e�L�X�g��ݒ肷��
				NMTTDISPINFO *pttdi = reinterpret_cast<NMTTDISPINFO*>(lParam);
				const ChannelInfo *pChannel = reinterpret_cast<const ChannelInfo*>(pttdi->lParam);

				pttdi->lpszText = m_szToolTipBuffer;

				int Length = ::wnsprintfW(
					m_szToolTipBuffer, _countof(m_szToolTipBuffer),
					L"%d: %s",
					pChannel->RemoteControlKeyID,
					pChannel->Name.c_str());

				// ���݂̔ԑg�̏����擾����
				TVTest::EpgEventQueryInfo EventQuery;
				EventQuery.NetworkID = pChannel->NetworkID;
				EventQuery.TransportStreamID = pChannel->TransportStreamID;
				EventQuery.ServiceID = pChannel->ServiceID;
				EventQuery.Type = TVTest::EPG_EVENT_QUERY_TIME;
				EventQuery.Flags = 0;
				::GetSystemTimeAsFileTime(&EventQuery.Time);
				OffsetFileTime(&EventQuery.Time, 120LL * 1000LL * 10000LL);	// 2����
				TVTest::EpgEventInfo *pEventInfo = m_pApp->GetEpgEventInfo(&EventQuery);
				if (pEventInfo != NULL) {
					RECT rc;
					::GetClientRect(hwnd, &rc);
					::SendMessage(m_hwndToolTips, TTM_SETMAXTIPWIDTH, 0, rc.right);

					// �J�n����
					Length += ::wnsprintfW(
						m_szToolTipBuffer + Length, _countof(m_szToolTipBuffer) - Length,
						L"\r\n%d:%02d�`", pEventInfo->StartTime.wHour, pEventInfo->StartTime.wMinute);

					//  �I������
					if (pEventInfo->Duration != 0) {
						FILETIME ft;
						SYSTEMTIME st;
						::SystemTimeToFileTime(&pEventInfo->StartTime, &ft);
						OffsetFileTime(&ft, pEventInfo->Duration * (1000LL * 10000LL));
						::FileTimeToSystemTime(&ft, &st);
						Length += ::wnsprintfW(
							m_szToolTipBuffer + Length, _countof(m_szToolTipBuffer) - Length,
							L"%d:%02d", st.wHour, st.wMinute);
					}

					// �ԑg��
					if (pEventInfo->pszEventName != NULL) {
						::wnsprintfW(
							m_szToolTipBuffer + Length, _countof(m_szToolTipBuffer) - Length,
							L" %s", pEventInfo->pszEventName);
					}

					m_pApp->FreeEpgEventInfo(pEventInfo);
				}
			}
			return 0;
		}
		break;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		return 0;

	case WM_DESTROY:
		{
			if (m_hFont != NULL) {
				::DeleteObject(m_hFont);
				m_hFont = NULL;
			}

			UpdateExpandedTunerList();
			m_TunerList.clear();
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// �R�}���h�̏���
void CTunerPanel::OnCommand(int ID)
{
	switch (ID) {
	case CM_VIEW_LIST:
	case CM_VIEW_LOGO:
		{
			ViewModeType View = (ViewModeType)(ID - CM_VIEW_LIST);

			if (View != m_ViewMode) {
				m_ViewMode = View;
				UpdateItemSize();
				UpdateScroll();
				::InvalidateRect(m_hwnd, NULL, TRUE);
			}
		}
		return;

	case CM_LOGOSIZE_SMALL:
	case CM_LOGOSIZE_MEDIUM:
	case CM_LOGOSIZE_LARGE:
	case CM_LOGOSIZE_EXTRALARGE:
		{
			LogoSizeType Size = (LogoSizeType)(ID - CM_LOGOSIZE_SMALL);

			if (Size != m_LogoSize) {
				m_LogoSize = Size;
				if (m_ViewMode == VIEW_MODE_LOGO) {
					UpdateItemSize();
					UpdateScroll();
					::InvalidateRect(m_hwnd, NULL, TRUE);
				}
			}
		}
		return;
	}
}


// �p�l���̏�����
void CTunerPanel::InitializePanel()
{
	LOGFONT lf;
	m_pApp->GetSetting(L"PanelFont", &lf);
	m_hFont = ::CreateFontIndirect(&lf);

	HDC hdc = ::GetDC(m_hwnd);
	HGDIOBJ hOldFont = ::SelectObject(hdc, m_hFont);
	TEXTMETRIC tm;
	::GetTextMetrics(hdc, &tm);
	m_FontHeight = tm.tmHeight - tm.tmInternalLeading;
	::SelectObject(hdc, hOldFont);
	::ReleaseDC(m_hwnd, hdc);

	m_TunerItemMargin.Left   = m_FontHeight / 3;
	m_TunerItemMargin.Top    = m_FontHeight / 3;
	m_TunerItemMargin.Right  = 0;
	m_TunerItemMargin.Bottom = m_FontHeight / 3;
	m_pApp->GetStyleValuePixels(L"tuner-panel.tuner-name.margin.left",
								&m_TunerItemMargin.Left);
	m_pApp->GetStyleValuePixels(L"tuner-panel.tuner-name.margin.top",
								&m_TunerItemMargin.Top);
	m_pApp->GetStyleValuePixels(L"tuner-panel.tuner-name.margin.right",
								&m_TunerItemMargin.Right);
	m_pApp->GetStyleValuePixels(L"tuner-panel.tuner-name.margin.bottom",
								&m_TunerItemMargin.Bottom);

	m_TunerItemHeight = m_FontHeight + m_TunerItemMargin.Top + m_TunerItemMargin.Bottom;

	m_ChannelItemMargin.Left   = m_FontHeight / 4;
	m_ChannelItemMargin.Top    = m_FontHeight / 4;
	m_ChannelItemMargin.Right  = m_FontHeight / 4;
	m_ChannelItemMargin.Bottom = m_FontHeight / 4;
	m_pApp->GetStyleValuePixels(L"tuner-panel.channel-name.margin.left",
								&m_ChannelItemMargin.Left);
	m_pApp->GetStyleValuePixels(L"tuner-panel.channel-name.margin.top",
								&m_ChannelItemMargin.Top);
	m_pApp->GetStyleValuePixels(L"tuner-panel.channel-name.margin.right",
								&m_ChannelItemMargin.Right);
	m_pApp->GetStyleValuePixels(L"tuner-panel.channel-name.margin.bottom",
								&m_ChannelItemMargin.Bottom);

	m_ChannelItemHeight = m_FontHeight + m_ChannelItemMargin.Top + m_ChannelItemMargin.Bottom;

	m_ChevronWidth = m_FontHeight * 2;
	m_ChevronMargin = m_FontHeight;

	m_LogoMargin.Left   = m_FontHeight / 4;
	m_LogoMargin.Top    = m_FontHeight / 4;
	m_LogoMargin.Right  = m_FontHeight / 4;
	m_LogoMargin.Bottom = m_FontHeight / 4;
	m_pApp->GetStyleValuePixels(L"tuner-panel.logo.margin.left",
								&m_LogoMargin.Left);
	m_pApp->GetStyleValuePixels(L"tuner-panel.logo.margin.top",
								&m_LogoMargin.Top);
	m_pApp->GetStyleValuePixels(L"tuner-panel.logo.margin.right",
								&m_LogoMargin.Right);
	m_pApp->GetStyleValuePixels(L"tuner-panel.logo.margin.bottom",
								&m_LogoMargin.Bottom);

	UpdateItemSize();

	m_fPointCursor = false;
	m_HotItem.Part = PART_NONE;
}


// ���X�g��`�悷��
void CTunerPanel::Draw(HDC hdc, const RECT &PaintRect)
{
	HGDIOBJ hOldFont = ::SelectObject(hdc, m_hFont);
	int OldBkMode = ::SetBkMode(hdc, TRANSPARENT);

	RECT rcClient;
	::GetClientRect(m_hwnd, &rcClient);

	m_pApp->ThemeDrawBackground(L"panel.content", hdc, PaintRect);

	int LogoHeight = m_FontHeight;
	int LogoWidth = LogoHeight * 16 / 9;

	HDC hdcMemory = ::CreateCompatibleDC(hdc);
	HGDIOBJ hOldBmp = ::GetCurrentObject(hdc, OBJ_BITMAP);
	int OldStretchMode = ::SetStretchBltMode(hdc, STRETCH_HALFTONE);

	TCHAR szCurTuner[MAX_PATH];
	if (m_pApp->GetDriverName(szCurTuner, _countof(szCurTuner)) < 1)
		szCurTuner[0] = '\0';

	TVTest::ChannelInfo CurChannel;
	bool fValidCurChannel =
		m_pApp->GetCurrentChannelInfo(&CurChannel);
	if (fValidCurChannel) {
		int CurService = m_pApp->GetService();
		if (CurService >= 0) {
			TVTest::ServiceInfo ServiceInfo;
			if (m_pApp->GetServiceInfo(CurService, &ServiceInfo))
				CurChannel.ServiceID = ServiceInfo.ServiceID;
		}
	}

	RECT rcItem;
	rcItem.left = 0;
	rcItem.right = rcClient.right;
	rcItem.top = -m_ScrollPos;

	for (size_t i = 0; i < m_TunerList.size() && rcItem.top < PaintRect.bottom; i++) {
		const TunerInfo &Tuner = m_TunerList[i];
		const bool fCurrentTuner = ::lstrcmpiW(Tuner.Name.c_str(), szCurTuner) == 0;

		rcItem.bottom = rcItem.top + m_TunerItemHeight;

		if (rcItem.bottom > PaintRect.top) {
			RECT rc = rcItem;
			int Offset = m_TunerItemMargin.Top - m_TunerItemMargin.Bottom;
			if (Offset >= 0)
				rc.top += Offset;
			else
				rc.bottom += Offset;
			rc.left += m_TunerItemMargin.Left;
			rc.right -= m_TunerItemMargin.Right + m_ChevronMargin + m_ChevronWidth;

			LPCWSTR pszStyle = fCurrentTuner ?
				L"channel-list-panel.channel-name.current":
				L"channel-list-panel.channel-name";

			WCHAR szTunerName[MAX_PATH];
			::lstrcpynW(szTunerName, Tuner.Name.c_str(), _countof(szTunerName));
			::PathRemoveExtensionW(szTunerName);
			LPCWSTR pszTunerName = szTunerName;
			if (::StrCmpNI(pszTunerName, L"BonDriver_", 10) == 0)
				pszTunerName += 10;

			m_pApp->ThemeDrawBackground(pszStyle, hdc, rcItem);
			m_pApp->ThemeDrawText(
				pszStyle, hdc, pszTunerName, rc,
				DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
			if (Tuner.fExpandable) {
				rc.right = rcItem.right;
				rc.left = rc.right - m_ChevronWidth;
				m_pApp->ThemeDrawText(
					pszStyle, hdc, Tuner.fExpanded ? L"\u25BE" : L"\u25B4", rc,
					DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
		}

		if (Tuner.fExpanded) {
			for (size_t j = 0; j < Tuner.TuningSpaceList.size(); j++) {
				const TuningSpaceInfo &TuningSpace = Tuner.TuningSpaceList[j];
				const int ChannelCount = (int)TuningSpace.ChannelList.size();

				rcItem.top = rcItem.bottom;
				rcItem.bottom = rcItem.top + CalcChannelItemRows(ChannelCount) * m_ItemHeight;

				for (int k = 0; k < ChannelCount; k++) {
					const ChannelInfo &Channel = TuningSpace.ChannelList[k];

					RECT rc = rcItem;
					CalcChannelItemRect(k, &rc);
					if (rc.bottom <= PaintRect.top)
						continue;

					const bool fCurrentChannel =
						fCurrentTuner && fValidCurChannel
							&& CurChannel.Space == Channel.Space
							&& CurChannel.Channel == Channel.Channel
							&& CurChannel.ServiceID == Channel.ServiceID;

					LPCWSTR pszStyle;
					if (m_HotItem.Part == PART_CHANNEL
							&& m_HotItem.Tuner == (int)i
							&& m_HotItem.Space == (int)j
							&& m_HotItem.Channel == k) {
						pszStyle = L"control-panel.item.hot";
						m_pApp->ThemeDrawBackground(pszStyle, hdc, rc);
					} else if (fCurrentChannel) {
						pszStyle = L"control-panel.item.checked";
						m_pApp->ThemeDrawBackground(pszStyle, hdc, rc);
					} else {
						pszStyle = L"panel.content";
					}

					if (m_ViewMode == VIEW_MODE_LIST) {
						int Offset = m_ChannelItemMargin.Top - m_ChannelItemMargin.Bottom;
						if (Offset >= 0)
							rc.top += Offset;
						else
							rc.bottom += Offset;
						rc.left += m_ChannelItemMargin.Left;
						rc.right -= m_ChannelItemMargin.Right;
					}

					if (Channel.LogoBitmap) {
						HBITMAP hbmLogo = Channel.LogoBitmap;
						BITMAP bm;
						::GetObject(hbmLogo, sizeof(BITMAP), &bm);
						::SelectObject(hdcMemory, hbmLogo);
						if (m_ViewMode == VIEW_MODE_LIST) {
							::StretchBlt(hdc,
								rc.left, rc.top + ((rc.bottom - rc.top) - LogoHeight) / 2,
								LogoWidth, LogoHeight,
								hdcMemory, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
							rc.left += LogoWidth + 4;
						} else if (m_ViewMode == VIEW_MODE_LOGO) {
							::StretchBlt(hdc,
								rc.left + m_LogoMargin.Left, rc.top + m_LogoMargin.Top,
								m_LogoWidth, m_LogoHeight,
								hdcMemory, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
						}
					}

					if (m_ViewMode == VIEW_MODE_LIST) {
						WCHAR szText[256];
						::wnsprintfW(szText, _countof(szText), L"%d: %s",
									 Channel.RemoteControlKeyID, Channel.Name.c_str());
						m_pApp->ThemeDrawText(
							pszStyle, hdc, szText, rc,
							DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
					} else if (m_ViewMode == VIEW_MODE_LOGO) {
						if (!Channel.LogoBitmap) {
							WCHAR szText[16];
							::wnsprintfW(szText, _countof(szText), L"%d", Channel.RemoteControlKeyID);
							m_pApp->ThemeDrawText(
								pszStyle, hdc, szText, rc,
								DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
						}
					}
				}
			}
		}

		rcItem.top = rcItem.bottom;
	}

	::SetStretchBltMode(hdc, OldStretchMode);
	::SelectObject(hdcMemory, hOldBmp);
	::DeleteDC(hdcMemory);

	::SetBkMode(hdc, OldBkMode);
	::SelectObject(hdc, hOldFont);
}


// ���X�g�S�̂̍������擾����
int CTunerPanel::CalcVertExtent() const
{
	int Extent = (int)m_TunerList.size() * m_TunerItemHeight;

	for (size_t i = 0; i < m_TunerList.size(); i++) {
		const TunerInfo &Tuner = m_TunerList[i];

		if (Tuner.fExpanded) {
			for (size_t j = 0; j < Tuner.TuningSpaceList.size(); j++) {
				const int ChannelCount = (int)Tuner.TuningSpaceList[j].ChannelList.size();

				Extent += CalcChannelItemRows(ChannelCount) * m_ItemHeight;
			}
		}
	}

	return Extent;
}


// �`�����l�����ڂ̃J���������擾����
int CTunerPanel::GetColumnCount() const
{
	if (m_ViewMode == VIEW_MODE_LOGO) {
		RECT rc;

		::GetClientRect(m_hwnd, &rc);
		int Columns = rc.right / m_ItemWidth;
		return max(Columns, 1);
	}

	return 1;
}


// ���S�̑傫�����擾����
SIZE CTunerPanel::GetLogoSize(LogoSizeType Type) const
{
	SIZE Size;

	Size.cy = m_FontHeight + (m_FontHeight * (int)Type / 3);
	Size.cx = Size.cy * 16 / 9;

	return Size;
}


// �`�����l�����ڂ̑傫�����X�V����
void CTunerPanel::UpdateItemSize()
{
	if (m_ViewMode == VIEW_MODE_LIST) {
		RECT rc;
		::GetClientRect(m_hwnd, &rc);
		m_ItemWidth = max(rc.right, 1);
		m_ItemHeight = m_ChannelItemHeight;
	} else if (m_ViewMode == VIEW_MODE_LOGO) {
		SIZE LogoSize = GetLogoSize(m_LogoSize);
		m_LogoWidth = LogoSize.cx;
		m_LogoHeight = LogoSize.cy;
		m_ItemWidth = m_LogoWidth + m_LogoMargin.Left + m_LogoMargin.Right;
		m_ItemHeight = m_LogoHeight + m_LogoMargin.Top + m_LogoMargin.Bottom;
	}

	m_ColumnCount = GetColumnCount();
}


// �X�N���[���͈�/�ʒu���X�V����
void CTunerPanel::UpdateScroll()
{
	int VertExtent = CalcVertExtent();
	RECT rc;

	::GetClientRect(m_hwnd, &rc);

	SCROLLINFO si;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = VertExtent - 1;
	si.nPage = rc.bottom;

	if (rc.bottom >= VertExtent) {
		si.nPos = 0;
	} else if (m_ScrollPos + rc.bottom <= VertExtent) {
		si.nPos = m_ScrollPos;
	} else {
		si.nPos = VertExtent - rc.bottom;
	}

	::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);

	if (si.nPos != m_ScrollPos) {
		::InvalidateRect(m_hwnd, NULL, TRUE);
		m_ScrollPos = si.nPos;
	}

	UpdateToolTips();
}


// �X�N���[���ʒu��ݒ肷��
void CTunerPanel::SetScrollPos(int Pos)
{
	RECT rc;

	::GetClientRect(m_hwnd, &rc);

	if (Pos < 0) {
		Pos = 0;
	} else {
		int Height = CalcVertExtent();
		int Max = max(Height - rc.bottom, 0);
		if (Pos > Max)
			Pos = Max;
	}

	if (Pos != m_ScrollPos) {
		int Offset = Pos - m_ScrollPos;

		m_ScrollPos = Pos;
		if (abs(Offset) < rc.bottom) {
			::ScrollWindowEx(m_hwnd, 0, -Offset,
							 NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
		} else {
			::InvalidateRect(m_hwnd, NULL, TRUE);
		}

		SCROLLINFO si;
		si.cbSize = sizeof (SCROLLINFO);
		si.fMask = SIF_POS;
		si.nPos = Pos;
		::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);

		UpdateToolTips();
	}
}


// ���ڂ̈ʒu���擾����
bool CTunerPanel::GetItemRect(int Tuner, int Space, int Channel, RECT *pRect) const
{
	RECT rc;

	::GetClientRect(m_hwnd, &rc);
	rc.top = -m_ScrollPos;

	for (int i = 0; i < (int)m_TunerList.size() && i <= Tuner; i++) {
		rc.bottom = rc.top + m_TunerItemHeight;

		if (Tuner == i && Space < 0) {
			*pRect = rc;
			return true;
		}

		const TunerInfo &TunerInf = m_TunerList[i];

		if (TunerInf.fExpanded) {
			for (int j = 0; j < (int)TunerInf.TuningSpaceList.size(); j++) {
				const TuningSpaceInfo &TuningSpace = TunerInf.TuningSpaceList[j];
				const int ChannelCount = (int)TuningSpace.ChannelList.size();

				rc.top = rc.bottom;
				rc.bottom = rc.top + CalcChannelItemRows(ChannelCount) * m_ItemHeight;

				if (Tuner == i && Space == j) {
					if (Channel < 0 || Channel >= ChannelCount)
						return false;
					CalcChannelItemRect(Channel, &rc);
					*pRect = rc;
					return true;
				}
			}
		}

		rc.top = rc.bottom;
	}

	return false;
}


// �`�����l�����ڂ̈ʒu���v�Z����
void CTunerPanel::CalcChannelItemRect(int Channel, RECT *pRect) const
{
	pRect->top += (Channel / m_ColumnCount) * m_ItemHeight;
	pRect->bottom = pRect->top + m_ItemHeight;
	pRect->left += (Channel % m_ColumnCount) * m_ItemWidth;
	pRect->right = pRect->left + m_ItemWidth;
}


// �`�����l�����ڂ̍s�����擾����
int CTunerPanel::CalcChannelItemRows(int Channels) const
{
	return (Channels + (m_ColumnCount - 1)) / m_ColumnCount;
}


// �w��ʒu�̍��ڂ��擾����
bool CTunerPanel::HitTest(int x, int y, HitTestInfo *pInfo) const
{
	pInfo->Part = PART_NONE;
	pInfo->Tuner = -1;
	pInfo->Space = -1;
	pInfo->Channel = -1;

	POINT pt = {x, y};
	RECT rc;

	::GetClientRect(m_hwnd, &rc);
	rc.top = -m_ScrollPos;

	for (size_t i = 0; i < m_TunerList.size() && rc.top < y; i++) {
		rc.bottom = rc.top + m_TunerItemHeight;

		if (::PtInRect(&rc, pt)) {
			if (m_TunerList[i].fExpandable && x >= rc.right - m_ChevronWidth)
				pInfo->Part = PART_CHEVRON;
			else if (x < rc.right - (m_ChevronMargin + m_ChevronWidth))
				pInfo->Part = PART_TUNER;
			else
				return false;
			pInfo->Tuner = (int)i;
			return true;
		}

		const TunerInfo &Tuner = m_TunerList[i];

		if (Tuner.fExpanded) {
			for (size_t j = 0; j < Tuner.TuningSpaceList.size(); j++) {
				const TuningSpaceInfo &TuningSpace = Tuner.TuningSpaceList[j];
				const int ChannelCount = (int)TuningSpace.ChannelList.size();

				rc.top = rc.bottom;
				rc.bottom = rc.top + CalcChannelItemRows(ChannelCount) * m_ItemHeight;

				if (x < m_ColumnCount * m_ItemWidth) {
					if (::PtInRect(&rc, pt)) {
						int Item = (((y - rc.top) / m_ItemHeight) * m_ColumnCount) + ((x - rc.left) / m_ItemWidth);
						if (Item >= ChannelCount)
							return false;
						pInfo->Part = PART_CHANNEL;
						pInfo->Tuner = (int)i;
						pInfo->Space = (int)j;
						pInfo->Channel = Item;
						return true;
					}
				}
			}
		}

		rc.top = rc.bottom;
	}

	return false;
}


// �c�[���`�b�v���X�V����
void CTunerPanel::UpdateToolTips()
{
	if (m_hwndToolTips == NULL)
		return;

	int NumTools = (int)::SendMessage(m_hwndToolTips, TTM_GETTOOLCOUNT, 0, 0);
	TOOLINFO ti = {sizeof(TOOLINFO)};
	ti.hwnd = m_hwnd;
	for (int i = NumTools - 1; i >= 0; i--) {
		ti.uId = i;
		::SendMessage(m_hwndToolTips, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&ti));
	}

	int ToolCount = 0;

	RECT rcClient;
	::GetClientRect(m_hwnd, &rcClient);
	RECT rc = rcClient;
	rc.top = -m_ScrollPos;

	for (int i = 0; i < (int)m_TunerList.size() && rc.top < rcClient.bottom; i++) {
		rc.bottom = rc.top + m_TunerItemHeight;

		TunerInfo &TunerInf = m_TunerList[i];

		if (TunerInf.fExpanded) {
			for (int j = 0; j < (int)TunerInf.TuningSpaceList.size(); j++) {
				TuningSpaceInfo &TuningSpace = TunerInf.TuningSpaceList[j];
				const int ChannelCount = (int)TuningSpace.ChannelList.size();

				rc.top = rc.bottom;
				rc.bottom = rc.top + CalcChannelItemRows(ChannelCount) * m_ItemHeight;

				for (int k = 0; k < ChannelCount; k++) {
					ti.rect = rc;
					CalcChannelItemRect(k, &ti.rect);
					if (ti.rect.bottom > 0) {
						ti.uFlags = TTF_SUBCLASS | TTF_TRANSPARENT;
						ti.uId = ToolCount;
						ti.lpszText = LPSTR_TEXTCALLBACK;
						ti.lParam = reinterpret_cast<LPARAM>(&TuningSpace.ChannelList[k]);
						::SendMessage(m_hwndToolTips, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
						ToolCount++;
					}
				}
			}
		}

		rc.top = rc.bottom;
	}

}


// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK CTunerPanel::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CTunerPanel *pThis;

	if (uMsg == WM_NCCREATE) {
		LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);

		pThis = static_cast<CTunerPanel*>(pcs->lpCreateParams);
		pThis->m_hwnd = hwnd;
		::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	} else {
		pThis = reinterpret_cast<CTunerPanel*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (pThis == NULL)
			return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
		if (uMsg == WM_NCDESTROY) {
			LRESULT Result = pThis->OnMessage(hwnd, uMsg, wParam, lParam);
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(nullptr));
			pThis-> m_hwnd = NULL;
			return Result;
		}
	}

	return pThis->OnMessage(hwnd, uMsg, wParam, lParam);
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CTunerPanel;
}
