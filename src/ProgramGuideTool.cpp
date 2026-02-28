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
#include "ProgramGuideTool.h"
#include "ProgramGuide.h"
#include "DialogUtil.h"
#include "VariableString.h"
#include "Help/HelpID.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


class CEpgVariableStringMap
	: public CEventVariableStringMap
{
public:
	CEpgVariableStringMap() = default;
	CEpgVariableStringMap(const EventInfo &Info);
	bool NormalizeString(String *pString) const override { return false; }
	bool GetParameterList(ParameterGroupList *pList) const override;
	const String &GetiEpgFileName() const { return m_iEpgFileName; }

private:
	bool GetLocalString(LPCWSTR pszKeyword, String *pString) override;

	static const ParameterInfo m_EpgParameterList[];

	String m_iEpgFileName;
};


const CEpgVariableStringMap::ParameterInfo CEpgVariableStringMap::m_EpgParameterList[] = {
	//{TEXT("eid"),          TEXT("イベントID")},
	{TEXT("nid"),          TEXT("ネットワークID")},
	{TEXT("tsid"),         TEXT("ストリームID")},
	//{TEXT("sid"),          TEXT("サービスID")},
	{TEXT("tvpid"),        TEXT("iEPGファイル")},
	{TEXT("duration-sec"), TEXT("番組の長さ(秒単位)")},
	{TEXT("duration-min"), TEXT("番組の長さ(分単位)")},
};


CEpgVariableStringMap::CEpgVariableStringMap(const EventInfo &Info)
	: CEventVariableStringMap(Info)
{
}


bool CEpgVariableStringMap::GetLocalString(LPCWSTR pszKeyword, String *pString)
{
	if (::lstrcmpi(pszKeyword, TEXT("tvpid")) == 0) {
		if (m_iEpgFileName.empty()) {
			TCHAR sziEpgFileName[MAX_PATH + 11];

			GetAppClass().GetAppDirectory(sziEpgFileName);
			::PathAppend(sziEpgFileName, TEXT("iepg.tvpid"));
			m_iEpgFileName = sziEpgFileName;
		}
		*pString = m_iEpgFileName;
	} else if (::lstrcmpi(pszKeyword, TEXT("eid")) == 0) {
		StringFormat(pString, TEXT("{}"), m_EventInfo.Event.EventID);
	} else if (::lstrcmpi(pszKeyword, TEXT("nid")) == 0) {
		StringFormat(pString, TEXT("{}"), m_EventInfo.Channel.GetNetworkID());
	} else if (::lstrcmpi(pszKeyword, TEXT("tsid")) == 0) {
		StringFormat(pString, TEXT("{}"), m_EventInfo.Channel.GetTransportStreamID());
	} else if (::lstrcmpi(pszKeyword, TEXT("sid")) == 0) {
		StringFormat(pString, TEXT("{}"), m_EventInfo.Channel.GetServiceID());
	} else if (::lstrcmpi(pszKeyword, TEXT("duration-sec")) == 0) {
		StringFormat(pString, TEXT("{}"), m_EventInfo.Event.Duration);
	} else if (::lstrcmpi(pszKeyword, TEXT("duration-min")) == 0) {
		StringFormat(pString, TEXT("{}"), (m_EventInfo.Event.Duration + 59) / 60);
	} else {
		return CEventVariableStringMap::GetLocalString(pszKeyword, pString);
	}

	return true;
}


bool CEpgVariableStringMap::GetParameterList(ParameterGroupList *pList) const
{
	if (!CEventVariableStringMap::GetParameterList(pList))
		return false;

	ParameterGroup &Group = pList->emplace_back();
	Group.ParameterList.insert(
		Group.ParameterList.end(),
		m_EpgParameterList,
		m_EpgParameterList + lengthof(m_EpgParameterList));

	return true;
}




CProgramGuideTool::CProgramGuideTool(const String &Name, const String &Command)
	: m_Name(Name)
	, m_Command(Command)
{
}


CProgramGuideTool::CProgramGuideTool(LPCTSTR pszName, LPCTSTR pszCommand)
{
	StringUtility::Assign(m_Name, pszName);
	StringUtility::Assign(m_Command, pszCommand);
}


bool CProgramGuideTool::GetPath(LPTSTR pszPath, int MaxLength) const
{
	LPCTSTR p = m_Command.c_str();

	return GetCommandFileName(&p, pszPath, MaxLength);
}


HICON CProgramGuideTool::GetIcon()
{
	if (!m_Icon && !m_Command.empty()) {
		TCHAR szFileName[MAX_PATH];
		LPCTSTR p = m_Command.c_str();

		if (GetCommandFileName(&p, szFileName, lengthof(szFileName))) {
			SHFILEINFO shfi;
			if (::SHGetFileInfo(szFileName, 0, &shfi, sizeof(shfi), SHGFI_ICON | SHGFI_SMALLICON))
				m_Icon.Attach(shfi.hIcon);
		}
	}
	return m_Icon;
}


bool CProgramGuideTool::Execute(
	const ProgramGuide::CServiceInfo *pServiceInfo,
	const LibISDB::EventInfo *pEventInfo, HWND hwnd)
{
	if (pServiceInfo == nullptr || pEventInfo == nullptr)
		return false;

	TCHAR szFileName[MAX_PATH];
	LPCTSTR p = m_Command.c_str();
	if (!GetCommandFileName(&p, szFileName, lengthof(szFileName))) {
		::MessageBox(hwnd, TEXT("パスが長すぎます。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	while (*p == _T(' '))
		p++;

	CEventVariableStringMap::EventInfo Info;
	Info.Channel = pServiceInfo->GetChannelInfo();
	Info.Event = *pEventInfo;
	Info.ServiceName = pServiceInfo->GetServiceName();
	Info.TOTTime.NowLocal();

	CEpgVariableStringMap VarStrMap(Info);
	String Parameter;
	FormatVariableString(&VarStrMap, p, &Parameter);
	const String &iEpgFileName = VarStrMap.GetiEpgFileName();
	if (!iEpgFileName.empty()) {
		if (!pServiceInfo->SaveiEpgFile(pEventInfo, iEpgFileName.c_str(), true)) {
			::MessageBox(
				hwnd, TEXT("iEPGファイルの書き出しができません。"), nullptr,
				MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	TRACE(TEXT("外部ツール実行 : {}, {}\n"), szFileName, Parameter);

	return ::ShellExecute(nullptr, nullptr, szFileName, Parameter.c_str(), nullptr, SW_SHOWNORMAL) >= (HINSTANCE)32;
}


bool CProgramGuideTool::ShowDialog(HWND hwndOwner)
{
	CProgramGuideToolDialog Dialog(this);

	return Dialog.Show(hwndOwner);
}


bool CProgramGuideTool::GetCommandFileName(LPCTSTR *ppszCommand, LPTSTR pszFileName, int MaxFileName)
{
	LPCTSTR p = *ppszCommand;
	LPTSTR q = pszFileName;
	TCHAR cDelimiter;

	if (*p == _T('"')) {
		cDelimiter = _T('"');
		p++;
	} else {
		cDelimiter = _T(' ');
	}
	int Length = 0;
	while (*p != _T('\0') && *p != cDelimiter) {
		const int CharLength = StringCharLength(p);
		if (CharLength == 0 || Length + CharLength >= MaxFileName) {
			pszFileName[0] = _T('\0');
			return false;
		}
		for (int i = 0; i < CharLength; i++)
			*q++ = *p++;
		Length += CharLength;
	}
	*q = _T('\0');
	if (*p == cDelimiter)
		p++;
	*ppszCommand = p;
	return true;
}




CProgramGuideTool::CProgramGuideToolDialog::CProgramGuideToolDialog(CProgramGuideTool *pTool)
	: m_pTool(pTool)
{
}


bool CProgramGuideTool::CProgramGuideToolDialog::Show(HWND hwndOwner)
{
	return ShowDialog(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_PROGRAMGUIDETOOL)) == IDOK;
}


INT_PTR CProgramGuideTool::CProgramGuideToolDialog::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::SetDlgItemText(hDlg, IDC_PROGRAMGUIDETOOL_NAME, m_pTool->m_Name.c_str());
			::SetDlgItemText(hDlg, IDC_PROGRAMGUIDETOOL_COMMAND, m_pTool->m_Command.c_str());
			EnableDlgItem(hDlg, IDOK, !m_pTool->m_Name.empty() && !m_pTool->m_Command.empty());
			AdjustDialogPos(::GetParent(hDlg), hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDETOOL_NAME:
		case IDC_PROGRAMGUIDETOOL_COMMAND:
			if (HIWORD(wParam) == EN_CHANGE) {
				EnableDlgItem(
					hDlg, IDOK,
					GetDlgItemTextLength(hDlg, IDC_PROGRAMGUIDETOOL_NAME) > 0 &&
					GetDlgItemTextLength(hDlg, IDC_PROGRAMGUIDETOOL_COMMAND) > 0);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_BROWSE:
			{
				OPENFILENAME ofn;
				String Command;
				TCHAR szFileName[MAX_PATH];
				String Directory;

				GetDlgItemString(hDlg, IDC_PROGRAMGUIDETOOL_COMMAND, &Command);
				if (!Command.empty()) {
					LPCTSTR p = Command.c_str();
					GetCommandFileName(&p, szFileName, lengthof(szFileName));
				} else {
					szFileName[0] = _T('\0');
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter =
					TEXT("実行ファイル(*.exe;*.bat)\0*.exe;*.bat\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.nFilterIndex = 1;
				ofn.lpstrFile = szFileName;
				ofn.nMaxFile = MAX_PATH;
				if (szFileName[0] != '\0') {
					String Name;
					if (PathUtil::Split(szFileName, &Directory, &Name)) {
						ofn.lpstrInitialDir = Directory.c_str();
						StringCopy(szFileName, Name.c_str());
					}
				}
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
				if (FileOpenDialog(&ofn)) {
					::PathQuoteSpaces(szFileName);
					::SetDlgItemText(hDlg, IDC_PROGRAMGUIDETOOL_COMMAND, szFileName);
					if (GetDlgItemTextLength(hDlg, IDC_PROGRAMGUIDETOOL_NAME) == 0) {
						::PathRemoveExtension(szFileName);
						::SetDlgItemText(hDlg, IDC_PROGRAMGUIDETOOL_NAME, ::PathFindFileName(szFileName));
					}
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_PARAMETER:
			{
				RECT rc;
				::GetWindowRect(::GetDlgItem(hDlg, IDC_PROGRAMGUIDETOOL_PARAMETER), &rc);
				CEpgVariableStringMap VarStrMap;
				VarStrMap.InputParameter(hDlg, IDC_PROGRAMGUIDETOOL_COMMAND, rc);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_PROGRAMGUIDETOOL);
			return TRUE;

		case IDOK:
			GetDlgItemString(hDlg, IDC_PROGRAMGUIDETOOL_NAME, &m_pTool->m_Name);
			GetDlgItemString(hDlg, IDC_PROGRAMGUIDETOOL_COMMAND, &m_pTool->m_Command);
			m_pTool->m_Icon.Destroy();
			[[fallthrough]];
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




CProgramGuideToolList::CProgramGuideToolList(const CProgramGuideToolList &Src)
{
	*this = Src;
}


CProgramGuideToolList &CProgramGuideToolList::operator=(const CProgramGuideToolList &Src)
{
	if (&Src != this) {
		Clear();
		if (!Src.m_ToolList.empty()) {
			m_ToolList.resize(Src.m_ToolList.size());
			for (size_t i = 0; i < Src.m_ToolList.size(); i++)
				m_ToolList[i] = std::make_unique<CProgramGuideTool>(*Src.m_ToolList[i]);
		}
	}
	return *this;
}


void CProgramGuideToolList::Clear()
{
	m_ToolList.clear();
}


bool CProgramGuideToolList::Add(CProgramGuideTool *pTool)
{
	if (pTool == nullptr)
		return false;
	m_ToolList.emplace_back(pTool);
	return true;
}


CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index)
{
	if (Index >= m_ToolList.size())
		return nullptr;
	return m_ToolList[Index].get();
}


const CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index) const
{
	if (Index >= m_ToolList.size())
		return nullptr;
	return m_ToolList[Index].get();
}


} // namespace TVTest
