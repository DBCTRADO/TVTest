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
#include "CaptionPanel.h"
#include "EpgUtil.h"
#include "Settings.h"
#include "DarkMode.h"
#include "resource.h"
#include <utility>
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr UINT WM_APP_ADD_CAPTION = WM_APP;

constexpr int IDC_EDIT = 1000;

}




const LPCTSTR CCaptionPanel::m_pszClassName = APP_NAME TEXT(" Caption Panel");
HINSTANCE CCaptionPanel::m_hinst = nullptr;


bool CCaptionPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst == nullptr) {
		WNDCLASS wc;

		wc.style = 0;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hinst;
		wc.hIcon = nullptr;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = nullptr;
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = m_pszClassName;
		if (::RegisterClass(&wc) == 0)
			return false;
		m_hinst = hinst;
	}
	return true;
}


CCaptionPanel::CCaptionPanel()
{
	GetDefaultFont(&m_CaptionFont);
}


CCaptionPanel::~CCaptionPanel()
{
	Destroy();
}


bool CCaptionPanel::Create(HWND hwndParent, DWORD Style, DWORD ExStyle, int ID)
{
	return CreateBasicWindow(
		hwndParent, Style, ExStyle, ID,
		m_pszClassName, TEXT("字幕"), m_hinst);
}


void CCaptionPanel::SetTheme(const Theme::CThemeManager *pThemeManager)
{
	SetColor(
		pThemeManager->GetColor(CColorScheme::COLOR_CAPTIONPANELBACK),
		pThemeManager->GetColor(CColorScheme::COLOR_CAPTIONPANELTEXT));
}


bool CCaptionPanel::SetFont(const Style::Font &Font)
{
	m_CaptionFont = Font;
	if (m_hwnd != nullptr) {
		ApplyStyle();
		RealizeStyle();
	}
	return true;
}


bool CCaptionPanel::ReadSettings(CSettings &Settings)
{
	int Value;

	Settings.Read(TEXT("CaptionPanel.AutoScroll"), &m_fAutoScroll);
	Settings.Read(TEXT("CaptionPanel.IgnoreSmall"), &m_fIgnoreSmall);
	Settings.Read(TEXT("CaptionPanel.HalfWidthAlnum"), &m_fHalfWidthAlnum);
	Settings.Read(TEXT("CaptionPanel.HalfWidthEuroLanguagesOnly"), &m_fHalfWidthEuroLanguagesOnly);
	if (Settings.Read(TEXT("CaptionPanel.SaveCharEncoding"), &Value)
			&& CheckEnumRange(static_cast<CharEncoding>(Value)))
		m_SaveCharEncoding = static_cast<CharEncoding>(Value);
	return true;
}


bool CCaptionPanel::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("CaptionPanel.AutoScroll"), m_fAutoScroll);
	Settings.Write(TEXT("CaptionPanel.IgnoreSmall"), m_fIgnoreSmall);
	Settings.Write(TEXT("CaptionPanel.HalfWidthAlnum"), m_fHalfWidthAlnum);
	Settings.Write(TEXT("CaptionPanel.HalfWidthEuroLanguagesOnly"), m_fHalfWidthEuroLanguagesOnly);
	Settings.Write(TEXT("CaptionPanel.SaveCharEncoding"), static_cast<int>(m_SaveCharEncoding));
	return true;
}


void CCaptionPanel::SetColor(COLORREF BackColor, COLORREF TextColor)
{
	m_BackColor = BackColor;
	m_TextColor = TextColor;
	m_BackBrush.Destroy();
	if (m_hwnd != nullptr) {
		m_BackBrush.Create(BackColor);

		if (IsDarkThemeSupported()) {
			SetWindowDarkTheme(m_hwndEdit, IsDarkThemeColor(m_BackColor));
		}

		::InvalidateRect(m_hwndEdit, nullptr, TRUE);
	}
}


void CCaptionPanel::Reset()
{
	BlockLock Lock(m_Lock);

	ClearCaptionList();
	if (m_hwndEdit != nullptr)
		::SetWindowText(m_hwndEdit, TEXT(""));
	m_DRCSMap.Reset();
}


bool CCaptionPanel::LoadDRCSMap(LPCTSTR pszFileName)
{
	return m_DRCSMap.Load(pszFileName);
}


void CCaptionPanel::ClearCaptionList()
{
	for (auto &e : m_LanguageList) {
		e.CaptionList.clear();
		e.NextCaption.clear();
		e.fClearLast = true;
		e.fContinue = false;
	}
}


void CCaptionPanel::AppendText(LPCTSTR pszText)
{
	bool fScroll = false;
	DWORD SelStart, SelEnd;

	if (m_fAutoScroll) {
		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
		::GetScrollInfo(m_hwndEdit, SB_VERT, &si);
		if (si.nPos >= si.nMax - static_cast<int>(si.nPage))
			fScroll = true;
	}
	::SendMessage(
		m_hwndEdit, EM_GETSEL,
		reinterpret_cast<WPARAM>(&SelStart),
		reinterpret_cast<LPARAM>(&SelEnd));
	::SendMessage(m_hwndEdit, EM_SETSEL, ::GetWindowTextLength(m_hwndEdit), -1);
	::SendMessage(m_hwndEdit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(pszText));
	::SendMessage(m_hwndEdit, EM_SETSEL, SelStart, SelEnd);
	if (fScroll)
		::SendMessage(m_hwndEdit, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
}


void CCaptionPanel::AppendQueuedText(BYTE Language)
{
	if (Language >= m_LanguageList.size())
		return;

	LanguageInfo &Lang = m_LanguageList[Language];

	if (!Lang.CaptionList.empty() || !Lang.NextCaption.empty()) {
		String Text;

		for (const auto &e : Lang.CaptionList)
			Text += e;
		Text += Lang.NextCaption;
		AppendText(Text.c_str());
		Lang.CaptionList.clear();
		Lang.NextCaption.clear();
	}
}


void CCaptionPanel::AddNextCaption(BYTE Language)
{
	LanguageInfo &Lang = m_LanguageList[Language];

	if (m_fHalfWidthAlnum) {
		if (!m_fHalfWidthEuroLanguagesOnly
				|| (Lang.LanguageCode != LibISDB::LANGUAGE_CODE_JPN &&
					Lang.LanguageCode != LibISDB::LANGUAGE_CODE_ZHO &&
					Lang.LanguageCode != LibISDB::LANGUAGE_CODE_KOR)) {
			StringUtility::ToHalfWidthNoKatakana(Lang.NextCaption);
		}
	}

	if (Language == m_CurLanguage) {
		::PostMessage(m_hwnd, WM_APP_ADD_CAPTION, Language, 0);
	} else {
		Lang.CaptionList.push_back(Lang.NextCaption);
		Lang.NextCaption.clear();
	}
}


bool CCaptionPanel::SetLanguage(BYTE Language)
{
	BlockLock Lock(m_Lock);

	if (Language >= m_LanguageList.size())
		return false;
	if (Language == m_CurLanguage)
		return true;

	if (m_CurLanguage < m_LanguageList.size()) {
		const int Length = ::GetWindowTextLength(m_hwndEdit);

		if (Length > 0) {
			LanguageInfo &Lang = m_LanguageList[m_CurLanguage];
			String Buffer(Length + 1, _T('\0'));

			::GetWindowText(m_hwndEdit, Buffer.data(), Length + 1);
			int End = Length - 1;
			for (int i = Length - 2; Lang.CaptionList.size() < MAX_QUEUE_TEXT; i--) {
				if (i < 0 || Buffer[i] == _T('\n')) {
					Lang.CaptionList.emplace_front(Buffer.substr(i + 1, End - i));
					if (i < 0)
						break;
					End = i;
				}
			}
		}
	}

	m_CurLanguage = Language;

	::SetWindowText(m_hwndEdit, TEXT(""));
	AppendQueuedText(m_CurLanguage);

	return true;
}


void CCaptionPanel::OnCommand(int Command)
{
	switch (Command) {
	case CM_CAPTIONPANEL_COPY:
		{
			const HWND hwndEdit = m_hwndEdit;
			DWORD Start, End;

			::SendMessage(hwndEdit, WM_SETREDRAW, FALSE, 0);
			::SendMessage(hwndEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&Start), reinterpret_cast<LPARAM>(&End));
			if (Start == End)
				::SendMessage(hwndEdit, EM_SETSEL, 0, -1);
			::SendMessage(hwndEdit, WM_COPY, 0, 0);
			if (Start == End)
				::SendMessage(hwndEdit, EM_SETSEL, Start, End);
			::SendMessage(hwndEdit, WM_SETREDRAW, TRUE, 0);
		}
		break;

	case CM_CAPTIONPANEL_SELECTALL:
		::SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
		break;

	case CM_CAPTIONPANEL_CLEAR:
		{
			BlockLock Lock(m_Lock);

			ClearCaptionList();
			::SetWindowText(m_hwndEdit, TEXT(""));
		}
		break;

	case CM_CAPTIONPANEL_SAVE:
		{
			const int Length = ::GetWindowTextLengthW(m_hwndEdit);
			if (Length > 0) {
				std::wstring Text(Length + 1, L'\0');
				DWORD Start, End;

				::GetWindowTextW(m_hwndEdit, Text.data(), Length + 1);
				::SendMessageW(m_hwndEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&Start), reinterpret_cast<LPARAM>(&End));

				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];

				szFileName[0] = _T('\0');
				InitOpenFileName(&ofn);
				ofn.hwndOwner = m_hwnd;
				ofn.lpstrFilter =
					TEXT("テキストファイル(UTF-16 LE)(*.*)\0*.*\0")
					TEXT("テキストファイル(UTF-8)(*.*)\0*.*\0")
					TEXT("テキストファイル(Shift_JIS)(*.*)\0*.*\0");
				ofn.nFilterIndex = static_cast<int>(m_SaveCharEncoding) + 1;
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = lengthof(szFileName);
				ofn.lpstrTitle = TEXT("字幕の保存");
				ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER;
				if (FileSaveDialog(&ofn)) {
					m_SaveCharEncoding = static_cast<CharEncoding>(ofn.nFilterIndex - 1);

					bool fOK = false;
					const HANDLE hFile = ::CreateFile(
						szFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

					if (hFile != INVALID_HANDLE_VALUE) {
						LPCWSTR pSrcText;
						DWORD SrcLength;
						DWORD Write;

						if (Start < End) {
							pSrcText = Text.c_str() + Start;
							SrcLength = End - Start;
						} else {
							pSrcText = Text.c_str();
							SrcLength = Length;
						}
						if (m_SaveCharEncoding == CharEncoding::UTF16) {
							const WCHAR BOM = 0xFEFF;
							fOK = ::WriteFile(hFile, &BOM, sizeof(BOM), &Write, nullptr)
								&& Write == sizeof(BOM)
								&& ::WriteFile(hFile, pSrcText, SrcLength * sizeof(WCHAR), &Write, nullptr)
								&& Write == SrcLength * sizeof(WCHAR);
						} else {
							const UINT CodePage =
								m_SaveCharEncoding == CharEncoding::UTF8 ? CP_UTF8 : 932;
							const int EncodedLen = ::WideCharToMultiByte(CodePage, 0, pSrcText, SrcLength, nullptr, 0, nullptr, nullptr);
							if (EncodedLen > 0) {
								std::string EncodedText(EncodedLen, '\0');
								::WideCharToMultiByte(CodePage, 0, pSrcText, SrcLength, EncodedText.data(), EncodedLen, nullptr, nullptr);
								fOK = ::WriteFile(hFile, EncodedText.data(), EncodedLen, &Write, nullptr)
									&& Write == static_cast<DWORD>(EncodedLen);
							}
						}

						::CloseHandle(hFile);
					}

					if (!fOK) {
						::MessageBox(
							m_hwnd, TEXT("ファイルを保存できません。"), nullptr,
							MB_OK | MB_ICONEXCLAMATION);
					}
				}
			}
		}
		break;

	case CM_CAPTIONPANEL_ENABLE:
		m_Lock.Lock();
		m_fEnable = !m_fEnable;
		for (auto &e : m_LanguageList) {
			e.fClearLast = false;
			e.fContinue = false;
		}
		m_Lock.Unlock();
		break;

	case CM_CAPTIONPANEL_AUTOSCROLL:
		m_fAutoScroll = !m_fAutoScroll;
		break;

	case CM_CAPTIONPANEL_IGNORESMALL:
		m_Lock.Lock();
		m_fIgnoreSmall = !m_fIgnoreSmall;
		m_Lock.Unlock();
		break;

	case CM_CAPTIONPANEL_HALFWIDTHALNUM:
		m_Lock.Lock();
		m_fHalfWidthAlnum = !m_fHalfWidthAlnum;
		m_Lock.Unlock();
		break;

	case CM_CAPTIONPANEL_HALFWIDTHEUROLANGS:
		m_Lock.Lock();
		m_fHalfWidthEuroLanguagesOnly = !m_fHalfWidthEuroLanguagesOnly;
		m_Lock.Unlock();
		break;

	default:
		if (Command >= CM_CAPTIONPANEL_LANGUAGE_FIRST && Command <= CM_CAPTIONPANEL_LANGUAGE_LAST) {
			SetLanguage(static_cast<BYTE>(Command - CM_CAPTIONPANEL_LANGUAGE_FIRST));
		}
		break;
	}
}


LRESULT CCaptionPanel::OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			InitializeUI();

			if (!m_BackBrush.IsCreated())
				m_BackBrush.Create(m_BackColor);

			m_hwndEdit = CreateWindowEx(
				0, TEXT("EDIT"), TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL |
					ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
				0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT)), m_hinst, nullptr);
			Edit_LimitText(m_hwndEdit, 8 * 1024 * 1024);
			SetWindowFont(m_hwndEdit, m_Font.GetHandle(), FALSE);
			m_EditSubclass.SetSubclass(m_hwndEdit);

			if (IsDarkThemeSupported()) {
				SetWindowDarkTheme(m_hwndEdit, IsDarkThemeColor(m_BackColor));
			}

			for (auto &e : m_LanguageList) {
				e.fClearLast = true;
				e.fContinue = false;
			}

			LibISDB::CaptionFilter *pCaptionFilter = GetAppClass().CoreEngine.GetFilter<LibISDB::CaptionFilter>();
			if (pCaptionFilter != nullptr) {
				pCaptionFilter->SetCaptionHandler(this);
				pCaptionFilter->SetDRCSMap(&m_DRCSMap);
			}
		}
		return 0;

	case WM_SIZE:
		::MoveWindow(m_hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;

	case WM_CTLCOLORSTATIC:
		{
			const HDC hdc = reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc, m_TextColor);
			::SetBkColor(hdc, m_BackColor);
			return reinterpret_cast<LRESULT>(m_BackBrush.GetHandle());
		}

	case WM_APP_ADD_CAPTION:
		{
			BlockLock Lock(m_Lock);
			const int LangIndex = static_cast<int>(wParam);

			if (LangIndex >= 0 && static_cast<size_t>(LangIndex) < m_LanguageList.size()) {
				LanguageInfo &Lang = m_LanguageList[LangIndex];

				if (!Lang.NextCaption.empty()) {
					if (m_fEnable) {
						if (m_fActive && LangIndex == m_CurLanguage) {
							::SendMessage(m_hwndEdit, WM_SETREDRAW, FALSE, 0);
							AppendText(Lang.NextCaption.c_str());
							::SendMessage(m_hwndEdit, WM_SETREDRAW, TRUE, 0);
							::InvalidateRect(m_hwndEdit, nullptr, TRUE);
						} else {
							// 非アクティブの場合はキューに溜める
							if (Lang.CaptionList.size() >= MAX_QUEUE_TEXT) {
								Lang.CaptionList.pop_front();
								if (LangIndex == m_CurLanguage)
									::SetWindowText(m_hwndEdit, TEXT(""));
							}
							Lang.CaptionList.push_back(Lang.NextCaption);
						}
					}
					Lang.NextCaption.clear();
				}
			}
		}
		return 0;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		return 0;

	case WM_DESTROY:
		{
			LibISDB::CaptionFilter *pCaptionFilter = GetAppClass().CoreEngine.GetFilter<LibISDB::CaptionFilter>();
			if (pCaptionFilter != nullptr) {
				pCaptionFilter->SetCaptionHandler(nullptr);
				pCaptionFilter->SetDRCSMap(nullptr);
			}

			ClearCaptionList();
			m_hwndEdit = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}


void CCaptionPanel::ApplyStyle()
{
	if (m_hwnd != nullptr)
		CreateDrawFont(m_CaptionFont, &m_Font);
}


void CCaptionPanel::RealizeStyle()
{
	if (m_hwndEdit != nullptr)
		SetWindowFont(m_hwndEdit, m_Font.GetHandle(), TRUE);
}


void CCaptionPanel::OnActivate()
{
	BlockLock Lock(m_Lock);

	AppendQueuedText(m_CurLanguage);

	m_fActive = true;
}


void CCaptionPanel::OnDeactivate()
{
	m_fActive = false;
}


void CCaptionPanel::OnLanguageUpdate(LibISDB::CaptionFilter *pFilter, LibISDB::CaptionParser *pParser)
{
	BlockLock Lock(m_Lock);

	const int LanguageNum = pFilter->GetLanguageCount();
	const int OldLanguageNum = static_cast<int>(m_LanguageList.size());

	m_LanguageList.resize(LanguageNum);

	for (int i = 0; i < LanguageNum; i++) {
		m_LanguageList[i].LanguageCode = pFilter->GetLanguageCode(i);
		if (i >= OldLanguageNum) {
			m_LanguageList[i].fClearLast = true;
			m_LanguageList[i].fContinue = false;
		}
	}

	if (m_CurLanguage >= LanguageNum)
		m_CurLanguage = 0;
}


void CCaptionPanel::OnCaption(
	LibISDB::CaptionFilter *pFilter, LibISDB::CaptionParser *pParser,
	uint8_t Language, const LibISDB::CharType *pText,
	const LibISDB::ARIBStringDecoder::FormatList *pFormatList)
{
	BlockLock Lock(m_Lock);

	if (Language >= m_LanguageList.size() || m_hwnd == nullptr || !m_fEnable)
		return;

	const int Length = ::lstrlen(pText);

	if (Length > 0) {
		LanguageInfo &Lang = m_LanguageList[Language];
		int i;

		for (i = 0; i < Length; i++) {
			if (pText[i] != '\f')
				break;
		}
		if (i == Length) {
			if (Lang.fClearLast || Lang.fContinue)
				return;
			Lang.fClearLast = true;
			Lang.NextCaption += TEXT("\r\n");
			AddNextCaption(Language);
			return;
		} else {
			Lang.fClearLast = false;
		}

		String Buff(pText);

		if (m_fIgnoreSmall && !pParser->Is1Seg() && Lang.LanguageCode == LibISDB::LANGUAGE_CODE_JPN) {
			for (int i = static_cast<int>(pFormatList->size()) - 1; i >= 0; i--) {
				if ((*pFormatList)[i].Size == LibISDB::ARIBStringDecoder::CharSize::Small) {
					const size_t Pos = (*pFormatList)[i].Pos;
					if (Pos < Buff.length()) {
						if (i + 1 < static_cast<int>(pFormatList->size())) {
							const size_t NextPos = std::min(Buff.length(), (*pFormatList)[i + 1].Pos);
							TRACE(TEXT("Caption exclude : {}\n"), StringView(&Buff[Pos], NextPos - Pos));
							Buff.erase(Pos, NextPos - Pos);
						} else {
							Buff.erase(Pos);
						}
					}
				}
			}
		}

		for (size_t i = 0; i < Buff.length(); i++) {
			if (Buff[i] == '\f') {
				if (i == 0 && !Lang.fContinue) {
					Buff.replace(0, 1, TEXT("\r\n"));
					i++;
				} else {
					Buff.erase(i, 1);
				}
			}
		}
		Lang.fContinue =
#ifdef UNICODE
			Buff.length() > 1 && Buff.back() == L'→';
#else
			Buff.length() > 2 && Buff[Buff.length() - 2] == "→"[0] && Buff[Buff.length() - 1] == "→"[1];
#endif
		if (Lang.fContinue)
#ifdef UNICODE
			Buff.pop_back();
#else
			Buff.erase(Buff.length() - 2);
#endif
		if (!Buff.empty()) {
			Lang.NextCaption += Buff;
			AddNextCaption(Language);
		}
	}
}


CCaptionPanel::CEditSubclass::CEditSubclass(CCaptionPanel *pCaptionPanel)
	: m_pCaptionPanel(pCaptionPanel)
{
}


LRESULT CCaptionPanel::CEditSubclass::OnMessage(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_RBUTTONDOWN:
		return 0;

	case WM_RBUTTONUP:
		{
			CPopupMenu Menu(GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDM_CAPTIONPANEL));

			Menu.CheckItem(CM_CAPTIONPANEL_ENABLE, m_pCaptionPanel->m_fEnable);
			Menu.CheckItem(CM_CAPTIONPANEL_AUTOSCROLL, m_pCaptionPanel->m_fAutoScroll);
			Menu.CheckItem(CM_CAPTIONPANEL_IGNORESMALL, m_pCaptionPanel->m_fIgnoreSmall);
			Menu.CheckItem(CM_CAPTIONPANEL_HALFWIDTHALNUM, m_pCaptionPanel->m_fHalfWidthAlnum);
			Menu.CheckItem(CM_CAPTIONPANEL_HALFWIDTHEUROLANGS, m_pCaptionPanel->m_fHalfWidthEuroLanguagesOnly);
			Menu.EnableItem(CM_CAPTIONPANEL_HALFWIDTHEUROLANGS, m_pCaptionPanel->m_fHalfWidthAlnum);

			// 言語選択
			m_pCaptionPanel->m_Lock.Lock();
			if (!m_pCaptionPanel->m_LanguageList.empty()) {
				Menu.AppendSeparator();
				int LanguageNum = static_cast<int>(m_pCaptionPanel->m_LanguageList.size());
				if (LanguageNum > CM_CAPTIONPANEL_LANGUAGE_LAST - CM_CAPTIONPANEL_LANGUAGE_FIRST + 1)
					LanguageNum = CM_CAPTIONPANEL_LANGUAGE_LAST - CM_CAPTIONPANEL_LANGUAGE_FIRST + 1;
				for (int i = 0; i < LanguageNum; i++) {
					TCHAR szText[LibISDB::MAX_LANGUAGE_TEXT_LENGTH];
					LibISDB::GetLanguageText_ja(
						m_pCaptionPanel->m_LanguageList[i].LanguageCode,
						szText, lengthof(szText));
					if (szText[0] == _T('\0'))
						StringFormat(szText, TEXT("言語{}"), i + 1);
					Menu.Append(CM_CAPTIONPANEL_LANGUAGE_FIRST + i, szText);
				}
				Menu.CheckRadioItem(
					CM_CAPTIONPANEL_LANGUAGE_FIRST,
					CM_CAPTIONPANEL_LANGUAGE_FIRST + LanguageNum - 1,
					CM_CAPTIONPANEL_LANGUAGE_FIRST + m_pCaptionPanel->m_CurLanguage);
			}
			m_pCaptionPanel->m_Lock.Unlock();

			POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
			::ClientToScreen(hwnd, &pt);
			Menu.Show(m_pCaptionPanel->m_hwnd, &pt, TPM_RIGHTBUTTON);
		}
		return 0;

	/*
		複数行(リッチ)エディットコントロールは Esc キーが押されると親ウィンドウに WM_CLOSE をポストするため、
		それによってウィンドウが破棄されてしまうので、Esc キーを処理させないようにする。
	*/
	case WM_KEYDOWN:
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
			return 0;
		break;

	case WM_NCDESTROY:
		m_pCaptionPanel->m_hwndEdit = nullptr;
		break;
	}

	return CWindowSubclass::OnMessage(hwnd, uMsg, wParam, lParam);
}




void CCaptionDRCSMap::Clear()
{
	BlockLock Lock(m_Lock);

	m_HashMap.clear();
	m_CodeMap.clear();
}


void CCaptionDRCSMap::Reset()
{
	BlockLock Lock(m_Lock);

	m_CodeMap.clear();
}


bool CCaptionDRCSMap::Load(LPCTSTR pszFileName)
{
	BlockLock Lock(m_Lock);

	Clear();

	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Read))
		return false;

	if (Settings.SetSection(TEXT("Settings"))) {
		Settings.Read(TEXT("SaveBMP"), &m_fSaveBMP);
		Settings.Read(TEXT("SaveRaw"), &m_fSaveRaw);
		String Dir;
		if (Settings.Read(TEXT("SaveDirectory"), &Dir) && !Dir.empty()) {
			GetAbsolutePath(Dir, &m_SaveDirectory);
		}
	}

	if (Settings.SetSection(TEXT("DRCSMap"))) {
		CSettings::EntryList Entries;

		Settings.GetEntries(&Entries);

		for (const auto &Entry : Entries) {
			if (Entry.Name.length() < 32)
				continue;

			LibISDB::MD5Value MD5;
			int i;

			for (i = 0; i < 32; i++) {
				BYTE v;

				if (Entry.Name[i] >= '0' && Entry.Name[i] <= '9')
					v = Entry.Name[i] - '0';
				else if (Entry.Name[i] >= 'A' && Entry.Name[i] <= 'F')
					v = Entry.Name[i] - 'A' + 10;
				else if (Entry.Name[i] >= 'a' && Entry.Name[i] <= 'f')
					v = Entry.Name[i] - 'a' + 10;
				else
					break;
				v <<= 4;
				i++;
				if (Entry.Name[i] >= '0' && Entry.Name[i] <= '9')
					v |= Entry.Name[i] - '0';
				else if (Entry.Name[i] >= 'A' && Entry.Name[i] <= 'F')
					v |= Entry.Name[i] - 'A' + 10;
				else if (Entry.Name[i] >= 'a' && Entry.Name[i] <= 'f')
					v |= Entry.Name[i] - 'a' + 10;
				else
					break;
				MD5.Value[i / 2] = v;
			}
	
			if (i == 32) {
				m_HashMap.emplace(MD5, Entry.Value);
				/*
				TRACE(
					TEXT("DRCS map : {:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X} = {}\n"),
					MD5.Value[0], MD5.Value[1], MD5.Value[2], MD5.Value[3], MD5.Value[4], MD5.Value[5], MD5.Value[6], MD5.Value[7],
					MD5.Value[8], MD5.Value[9], MD5.Value[10], MD5.Value[11], MD5.Value[12], MD5.Value[13], MD5.Value[14], MD5.Value[15],
					Entry.Value.c_str());
				*/
			}
		}
	}

	return true;
}


LPCTSTR CCaptionDRCSMap::GetString(WORD Code)
{
	BlockLock Lock(m_Lock);
	CodeMap::iterator itr = m_CodeMap.find(Code);

	if (itr != m_CodeMap.end()) {
		TRACE(TEXT("DRCS : Code {} {}\n"), Code, itr->second);
		return itr->second.c_str();
	}
	return nullptr;
}


static void MakeSaveFileName(const LibISDB::MD5Value &MD5, LPTSTR pszFileName, LPCTSTR pszExtension)
{
	static const TCHAR Hex[] = TEXT("0123456789ABCDEF");
	for (int i = 0; i < 16; i++) {
		pszFileName[i * 2 + 0] = Hex[MD5.Value[i] >> 4];
		pszFileName[i * 2 + 1] = Hex[MD5.Value[i] & 0x0F];
	}
	StringCopy(pszFileName + 32, pszExtension);
}

bool CCaptionDRCSMap::SetDRCS(uint16_t Code, const DRCSBitmap *pBitmap)
{
	BlockLock Lock(m_Lock);

	const LibISDB::MD5Value MD5 = LibISDB::CalcMD5(pBitmap->pData, pBitmap->DataSize);
	TRACE(
		TEXT("DRCS : Code {}, {} x {} ({}), MD5 {:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}\n"),
		Code, pBitmap->Width, pBitmap->Height, pBitmap->Depth,
		MD5.Value[0], MD5.Value[1], MD5.Value[2], MD5.Value[3], MD5.Value[4], MD5.Value[5], MD5.Value[6], MD5.Value[7],
		MD5.Value[8], MD5.Value[9], MD5.Value[10], MD5.Value[11], MD5.Value[12], MD5.Value[13], MD5.Value[14], MD5.Value[15]);
	HashMap::iterator itr = m_HashMap.find(MD5);
	if (itr != m_HashMap.end()) {
		TRACE(TEXT("DRCS assign {} = {}\n"), Code, itr->second);
		m_CodeMap[Code] = itr->second;
	}

	if (m_fSaveBMP || m_fSaveRaw) {
		if (m_SaveDirectory.empty()) {
			GetAppClass().GetAppDirectory(&m_SaveDirectory);
			m_SaveDirectory.Append(TEXT("DRCS"));
		}
	}

	if (m_fSaveBMP) {
		TCHAR szFileName[40];
		MakeSaveFileName(MD5, szFileName, TEXT(".bmp"));
		CFilePath FilePath(m_SaveDirectory);
		FilePath.Append(szFileName);
		if (!FilePath.IsExists())
			SaveBMP(pBitmap, FilePath.c_str());
	}
	if (m_fSaveRaw) {
		TCHAR szFileName[40];
		MakeSaveFileName(MD5, szFileName, TEXT(".drcs"));
		CFilePath FilePath(m_SaveDirectory);
		FilePath.Append(szFileName);
		if (!FilePath.IsExists())
			SaveRaw(pBitmap, FilePath.c_str());
	}
	return true;
}


bool CCaptionDRCSMap::SaveBMP(const DRCSBitmap *pBitmap, LPCTSTR pszFileName)
{
	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD Write;
	const int BitCount = pBitmap->BitsPerPixel == 1 ? 1 : 8;
	const DWORD DIBRowBytes = (pBitmap->Width * BitCount + 31) / 32 * 4;
	const DWORD BitsSize = DIBRowBytes * pBitmap->Height;
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = 0x4D42;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + (1UL << BitCount) * static_cast<DWORD>(sizeof(RGBQUAD));
	bmfh.bfSize = bmfh.bfOffBits + BitsSize;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	if (!::WriteFile(hFile, &bmfh, sizeof(bmfh), &Write, nullptr) || Write != sizeof(bmfh)) {
		::CloseHandle(hFile);
		return false;
	}

	BITMAPINFOHEADER bmih;
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = pBitmap->Width;
	bmih.biHeight = pBitmap->Height;
	bmih.biPlanes = 1;
	bmih.biBitCount = BitCount;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	if (!::WriteFile(hFile, &bmih, sizeof(bmih), &Write, nullptr) || Write != sizeof(bmih)) {
		::CloseHandle(hFile);
		return false;
	}

	RGBQUAD Colormap[256];
	for (int i = 0; i < 1 << BitCount; i++) {
		const BYTE v = static_cast<BYTE>(i * 255 / ((1 << BitCount) - 1));
		Colormap[i].rgbBlue = v;
		Colormap[i].rgbGreen = v;
		Colormap[i].rgbRed = v;
		Colormap[i].rgbReserved = 0;
	}
	const DWORD PalSize = (1UL << BitCount) * static_cast<DWORD>(sizeof(RGBQUAD));
	if (!::WriteFile(hFile, Colormap, PalSize, &Write, nullptr) || Write != PalSize) {
		::CloseHandle(hFile);
		return false;
	}

	std::unique_ptr<BYTE[]> DIBBits(new BYTE[BitsSize]);
	const BYTE *p = static_cast<const BYTE*>(pBitmap->pData);
	BYTE *q = DIBBits.get() + (pBitmap->Height - 1) * DIBRowBytes;
	if (BitCount == 1) {
		BYTE Mask;

		::ZeroMemory(DIBBits.get(), BitsSize);
		Mask = 0x80;
		for (int y = 0; y < pBitmap->Height; y++) {
			for (int x = 0; x < pBitmap->Width; x++) {
				if ((*p & Mask) != 0)
					q[x >> 3] |= 0x80 >> (x & 7);
				Mask >>= 1;
				if (Mask == 0) {
					Mask = 0x80;
					p++;
				}
			}
			q -= DIBRowBytes;
		}
	} else {
		const unsigned int Max = pBitmap->Depth + 1;
		unsigned int Mask;
		int Shift;

		Shift = 16 - pBitmap->BitsPerPixel;
		Mask = (1 << pBitmap->BitsPerPixel) - 1;
		for (int y = 0; y < pBitmap->Height; y++) {
			for (int x = 0; x < pBitmap->Width; x++) {
				unsigned int Pixel = *p;
				if (Shift < 8)
					Pixel = (Pixel << (8 - Shift)) | (*p >> Shift);
				else
					Pixel >>= Shift - 8;
				Pixel = (Pixel & Mask) * 255 / Max;
				q[x] = static_cast<BYTE>(std::min(Pixel, 255U));
				Shift -= pBitmap->BitsPerPixel;
				if (Shift < 0) {
					Shift += 16;
					p++;
				}
				if (Shift + pBitmap->BitsPerPixel <= 8) {
					Shift += 8;
					p++;
				}
			}
			q -= DIBRowBytes;
		}
	}
	if (!::WriteFile(hFile, DIBBits.get(), BitsSize, &Write, nullptr) || Write != BitsSize) {
		::CloseHandle(hFile);
		return false;
	}

	::CloseHandle(hFile);
	return true;
}


bool CCaptionDRCSMap::SaveRaw(const DRCSBitmap *pBitmap, LPCTSTR pszFileName)
{
	const HANDLE hFile = ::CreateFile(
		pszFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	DWORD Write;
	if (!::WriteFile(hFile, pBitmap->pData, pBitmap->DataSize, &Write, nullptr)
			|| Write != pBitmap->DataSize) {
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);
	return true;
}


} // namespace TVTest
