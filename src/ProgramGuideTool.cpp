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




class CEpgVariableStringMap : public TVTest::CEventVariableStringMap
{
public:
	CEpgVariableStringMap();
	CEpgVariableStringMap(const EventInfo &Info);
	bool GetString(LPCWSTR pszKeyword,TVTest::String *pString) override;
	bool NormalizeString(TVTest::String *pString) const override { return false; }
	bool GetParameterInfo(int Index,ParameterInfo *pInfo) const override;
	int GetParameterCount() const override;
	const TVTest::String &GetiEpgFileName() const { return m_iEpgFileName; }

private:
	static const ParameterInfo m_EpgParameterList[];

	TVTest::String m_iEpgFileName;
};


const CEpgVariableStringMap::ParameterInfo CEpgVariableStringMap::m_EpgParameterList[] = {
//	{TEXT("%eid%"),				TEXT("イベントID")},
	{TEXT("%nid%"),				TEXT("ネットワークID")},
	{TEXT("%tsid%"),			TEXT("ストリームID")},
//	{TEXT("%sid%"),				TEXT("サービスID")},
	{TEXT("%tvpid%"),			TEXT("iEPGファイル")},
	{TEXT("%duration-sec%"),	TEXT("番組の長さ(秒単位)")},
	{TEXT("%duration-min%"),	TEXT("番組の長さ(分単位)")},
};


CEpgVariableStringMap::CEpgVariableStringMap()
{
}


CEpgVariableStringMap::CEpgVariableStringMap(const EventInfo &Info)
	: CEventVariableStringMap(Info)
{
}


bool CEpgVariableStringMap::GetString(LPCWSTR pszKeyword,TVTest::String *pString)
{
	if (::lstrcmpi(pszKeyword,TEXT("tvpid"))==0) {
		if (m_iEpgFileName.empty()) {
			TCHAR sziEpgFileName[MAX_PATH+11];

			GetAppClass().GetAppDirectory(sziEpgFileName);
			::PathAppend(sziEpgFileName,TEXT("iepg.tvpid"));
			m_iEpgFileName=sziEpgFileName;
		}
		*pString=m_iEpgFileName;
	} else if (::lstrcmpi(pszKeyword,TEXT("eid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Event.m_EventID);
	} else if (::lstrcmpi(pszKeyword,TEXT("nid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Channel.GetNetworkID());
	} else if (::lstrcmpi(pszKeyword,TEXT("tsid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Channel.GetTransportStreamID());
	} else if (::lstrcmpi(pszKeyword,TEXT("sid"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Channel.GetServiceID());
	} else if (::lstrcmpi(pszKeyword,TEXT("duration-sec"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),m_EventInfo.Event.m_Duration);
	} else if (::lstrcmpi(pszKeyword,TEXT("duration-min"))==0) {
		TVTest::StringUtility::Format(*pString,TEXT("%d"),(m_EventInfo.Event.m_Duration+59)/60);
	} else {
		return CEventVariableStringMap::GetString(pszKeyword,pString);
	}

	return true;
}


bool CEpgVariableStringMap::GetParameterInfo(int Index,ParameterInfo *pInfo) const
{
	if (pInfo==nullptr)
		return false;

	if (CEventVariableStringMap::GetParameterInfo(Index,pInfo))
		return true;

	Index-=CEventVariableStringMap::GetParameterCount();
	if (Index>=0 && Index<lengthof(m_EpgParameterList)) {
		*pInfo=m_EpgParameterList[Index];
		return true;
	}

	return false;
}


int CEpgVariableStringMap::GetParameterCount() const
{
	return CEventVariableStringMap::GetParameterCount()+lengthof(m_EpgParameterList);
}




CProgramGuideTool::CProgramGuideTool()
{
}


CProgramGuideTool::CProgramGuideTool(const TVTest::String &Name,const TVTest::String &Command)
	: m_Name(Name)
	, m_Command(Command)
{
}


CProgramGuideTool::CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand)
{
	TVTest::StringUtility::Assign(m_Name,pszName);
	TVTest::StringUtility::Assign(m_Command,pszCommand);
}


bool CProgramGuideTool::GetPath(LPTSTR pszPath,int MaxLength) const
{
	LPCTSTR p=m_Command.c_str();

	return GetCommandFileName(&p,pszPath,MaxLength);
}


HICON CProgramGuideTool::GetIcon()
{
	if (!m_Icon && !m_Command.empty()) {
		TCHAR szFileName[MAX_PATH];
		LPCTSTR p=m_Command.c_str();

		if (GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
			SHFILEINFO shfi;
			if (::SHGetFileInfo(szFileName,0,&shfi,sizeof(shfi),
								SHGFI_ICON | SHGFI_SMALLICON))
				m_Icon.Attach(shfi.hIcon);
		}
	}
	return m_Icon;
}


bool CProgramGuideTool::Execute(const ProgramGuide::CServiceInfo *pServiceInfo,
								const CEventInfoData *pEventInfo,HWND hwnd)
{
	if (pServiceInfo==nullptr || pEventInfo==nullptr)
		return false;

	SYSTEMTIME stStart,stEnd;
	TCHAR szFileName[MAX_PATH];
	LPCTSTR p;

	pEventInfo->GetStartTime(&stStart);
	pEventInfo->GetEndTime(&stEnd);
	p=m_Command.c_str();
	if (!GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
		::MessageBox(hwnd,TEXT("パスが長すぎます。"),nullptr,MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	while (*p==_T(' '))
		p++;

	TVTest::CEventVariableStringMap::EventInfo Info;
	Info.Channel=pServiceInfo->GetChannelInfo();
	Info.Event=*pEventInfo;
	Info.ServiceName=pServiceInfo->GetServiceName();
	::GetLocalTime(&Info.TotTime);

	CEpgVariableStringMap VarStrMap(Info);
	TVTest::String Parameter;
	TVTest::FormatVariableString(&VarStrMap,p,&Parameter);
	const TVTest::String &iEpgFileName=VarStrMap.GetiEpgFileName();
	if (!iEpgFileName.empty()) {
		if (!pServiceInfo->SaveiEpgFile(pEventInfo,iEpgFileName.c_str(),true)) {
			::MessageBox(hwnd,TEXT("iEPGファイルの書き出しができません。"),nullptr,
						 MB_OK | MB_ICONEXCLAMATION);
			return false;
		}
	}

	TRACE(TEXT("外部ツール実行 : %s, %s\n"),szFileName,Parameter.c_str());

	return ::ShellExecute(nullptr,nullptr,szFileName,Parameter.c_str(),nullptr,SW_SHOWNORMAL)>=(HINSTANCE)32;
}


bool CProgramGuideTool::ShowDialog(HWND hwndOwner)
{
	CProgramGuideToolDialog Dialog(this);

	return Dialog.Show(hwndOwner);
}


bool CProgramGuideTool::GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName,int MaxFileName)
{
	LPCTSTR p;
	LPTSTR q;
	TCHAR cDelimiter;

	p=*ppszCommand;
	q=pszFileName;
	if (*p==_T('"')) {
		cDelimiter=_T('"');
		p++;
	} else {
		cDelimiter=_T(' ');
	}
	int Length=0;
	while (*p!=_T('\0') && *p!=cDelimiter) {
		int CharLength=StringCharLength(p);
		if (CharLength==0 || Length+CharLength>=MaxFileName) {
			pszFileName[0]=_T('\0');
			return false;
		}
		for (int i=0;i<CharLength;i++)
			*q++=*p++;
		Length+=CharLength;
	}
	*q=_T('\0');
	if (*p==cDelimiter)
		p++;
	*ppszCommand=p;
	return true;
}




CProgramGuideTool::CProgramGuideToolDialog::CProgramGuideToolDialog(CProgramGuideTool *pTool)
	: m_pTool(pTool)
{
}


bool CProgramGuideTool::CProgramGuideToolDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_PROGRAMGUIDETOOL))==IDOK;
}


INT_PTR CProgramGuideTool::CProgramGuideToolDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,m_pTool->m_Name.c_str());
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,m_pTool->m_Command.c_str());
			EnableDlgItem(hDlg,IDOK,!m_pTool->m_Name.empty() && !m_pTool->m_Command.empty());
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDETOOL_NAME:
		case IDC_PROGRAMGUIDETOOL_COMMAND:
			if (HIWORD(wParam)==EN_CHANGE) {
				EnableDlgItem(hDlg,IDOK,
					GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)>0 &&
					GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND)>0);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_BROWSE:
			{
				OPENFILENAME ofn;
				TVTest::String Command;
				TCHAR szFileName[MAX_PATH],szDirectory[MAX_PATH];

				GetDlgItemString(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,&Command);
				if (!Command.empty()) {
					LPCTSTR p=Command.c_str();
					GetCommandFileName(&p,szFileName,lengthof(szFileName));
				} else {
					szFileName[0]=_T('\0');
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("実行ファイル(*.exe;*.bat)\0*.exe;*.bat\0")
								TEXT("すべてのファイル\0*.*\0");
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=MAX_PATH;
				if (szFileName[0]!='\0') {
					CFilePath Path(szFileName);
					Path.GetDirectory(szDirectory);
					ofn.lpstrInitialDir=szDirectory;
					::lstrcpy(szFileName,Path.GetFileName());
				}
				ofn.Flags=OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::PathQuoteSpaces(szFileName);
					//::lstrcat(szFileName,TEXT(" \"%tvpid%\""));
					::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,szFileName);
					if (GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)==0) {
						::PathRemoveExtension(szFileName);
						::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
										 ::PathFindFileName(szFileName));
					}
				}
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_PARAMETER:
			{
				CEpgVariableStringMap VarStrMap;
				RECT rc;
				POINT pt;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_PARAMETER),&rc);
				pt.x=rc.left;
				pt.y=rc.bottom;
				VarStrMap.InputParameter(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,pt);
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_HELP:
			GetAppClass().UICore.ShowHelpContent(HELP_ID_PROGRAMGUIDETOOL);
			return TRUE;

		case IDOK:
			GetDlgItemString(hDlg,IDC_PROGRAMGUIDETOOL_NAME,&m_pTool->m_Name);
			GetDlgItemString(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,&m_pTool->m_Command);
			m_pTool->m_Icon.Destroy();
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}




CProgramGuideToolList::CProgramGuideToolList()
{
}


CProgramGuideToolList::CProgramGuideToolList(const CProgramGuideToolList &Src)
{
	*this=Src;
}


CProgramGuideToolList::~CProgramGuideToolList()
{
	Clear();
}


CProgramGuideToolList &CProgramGuideToolList::operator=(const CProgramGuideToolList &Src)
{
	if (&Src!=this) {
		Clear();
		if (!Src.m_ToolList.empty()) {
			m_ToolList.resize(Src.m_ToolList.size());
			for (size_t i=0;i<Src.m_ToolList.size();i++)
				m_ToolList[i]=new CProgramGuideTool(*Src.m_ToolList[i]);
		}
	}
	return *this;
}


void CProgramGuideToolList::Clear()
{
	for (size_t i=0;i<m_ToolList.size();i++)
		delete m_ToolList[i];
	m_ToolList.clear();
}


bool CProgramGuideToolList::Add(CProgramGuideTool *pTool)
{
	if (pTool==nullptr)
		return false;
	m_ToolList.push_back(pTool);
	return true;
}


CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index)
{
	if (Index>=m_ToolList.size())
		return nullptr;
	return m_ToolList[Index];
}


const CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index) const
{
	if (Index>=m_ToolList.size())
		return nullptr;
	return m_ToolList[Index];
}
