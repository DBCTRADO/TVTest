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
#include "Logger.h"
#include "DialogUtil.h"
#include "DPIUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr std::size_t MAX_LOG_TEXT_LENGTH = 1024;

}




CLogItem::CLogItem()
{
}


CLogItem::CLogItem(LogType Type, LPCTSTR pszText, DWORD SerialNumber)
	: m_Type(Type)
	, m_Text(pszText)
	, m_SerialNumber(SerialNumber)
{
	::GetSystemTimeAsFileTime(&m_Time);
}


CLogItem::~CLogItem()
{
}


void CLogItem::GetTime(SYSTEMTIME *pTime) const
{
	SYSTEMTIME stUTC;

	::FileTimeToSystemTime(&m_Time, &stUTC);
	::SystemTimeToTzSpecificLocalTime(nullptr, &stUTC, pTime);
}


int CLogItem::Format(char *pszText, int MaxLength) const
{
	int Length;

	Length = FormatTime(pszText, MaxLength);
	pszText[Length++] = '>';
	Length += ::WideCharToMultiByte(
		CP_ACP, 0, m_Text.data(), (int)m_Text.length(),
		pszText + Length, MaxLength - Length - 1, nullptr, nullptr);
	pszText[Length] = '\0';
	return Length;
}


int CLogItem::Format(WCHAR *pszText, int MaxLength) const
{
	int Length;

	Length = FormatTime(pszText, MaxLength);
	pszText[Length++] = L'>';
	StringCopy(pszText + Length, m_Text.c_str(), MaxLength - Length);
	Length += ::lstrlenW(pszText + Length);
	return Length;
}


int CLogItem::FormatTime(char *pszText, int MaxLength) const
{
	SYSTEMTIME st;
	int Length;

	GetTime(&st);
	Length = ::GetDateFormatA(
		LOCALE_USER_DEFAULT, DATE_SHORTDATE,
		&st, nullptr, pszText, MaxLength - 1);
	pszText[Length - 1] = ' ';
	Length += ::GetTimeFormatA(
		LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT,
		&st, nullptr, pszText + Length, MaxLength - Length);
	return Length - 1;
}


int CLogItem::FormatTime(WCHAR *pszText, int MaxLength) const
{
	SYSTEMTIME st;
	int Length;

	GetTime(&st);
	Length = ::GetDateFormatW(
		LOCALE_USER_DEFAULT, DATE_SHORTDATE,
		&st, nullptr, pszText, MaxLength - 1);
	pszText[Length - 1] = L' ';
	Length += ::GetTimeFormatW(
		LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT,
		&st, nullptr, pszText + Length, MaxLength - Length);
	return Length - 1;
}




CLogger::CLogger()
	: m_SerialNumber(0)
	, m_fOutputToFile(false)
{
	TCHAR szFileName[MAX_PATH];
	DWORD Result = ::GetModuleFileName(nullptr, szFileName, lengthof(szFileName));
	if ((Result > 0) && (Result < MAX_PATH)) {
		m_DefaultLogFileName = szFileName;
		m_DefaultLogFileName += TEXT(".log");
	}
}


CLogger::~CLogger()
{
	Destroy();
}


bool CLogger::ReadSettings(CSettings &Settings)
{
	BlockLock Lock(m_Lock);

	Settings.Read(TEXT("OutputLogToFile"), &m_fOutputToFile);
	if (m_fOutputToFile && m_LogList.size() > 0) {
		SaveToFile(nullptr, true);
	}
	return true;
}


bool CLogger::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("OutputLogToFile"), m_fOutputToFile);
	return true;
}


bool CLogger::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(), MAKEINTRESOURCE(IDD_OPTIONS_LOG));
}


bool CLogger::AddLog(CLogItem::LogType Type, LPCTSTR pszText, ...)
{
	if (pszText == nullptr)
		return false;

	va_list Args;
	va_start(Args, pszText);
	AddLogV(Type, pszText, Args);
	va_end(Args);
	return true;
}


bool CLogger::AddLogV(CLogItem::LogType Type, LPCTSTR pszText, va_list Args)
{
	if (pszText == nullptr)
		return false;

	TCHAR szText[MAX_LOG_TEXT_LENGTH];
	StringPrintfV(szText, pszText, Args);
	AddLogRaw(Type, szText);

	return true;
}


bool CLogger::AddLogRaw(CLogItem::LogType Type, LPCTSTR pszText)
{
	if (pszText == nullptr)
		return false;

	BlockLock Lock(m_Lock);

	CLogItem *pLogItem = new CLogItem(Type, pszText, m_SerialNumber++);
	m_LogList.emplace_back(pLogItem);
	TRACE(TEXT("Log : %s\n"), pszText);

	if (m_fOutputToFile && !m_DefaultLogFileName.empty()) {
		HANDLE hFile;

		hFile = ::CreateFile(
			m_DefaultLogFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
			OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile != INVALID_HANDLE_VALUE) {
			static const LARGE_INTEGER Zero = {};
			LARGE_INTEGER Pos;
			DWORD Size;

			if (::SetFilePointerEx(hFile, Zero, &Pos, FILE_END) && Pos.QuadPart == 0) {
				static const WORD BOM = 0xFEFF;

				::WriteFile(hFile, &BOM, 2, &Size, nullptr);
			}

			WCHAR szText[MAX_LOG_TEXT_LENGTH];
			DWORD Length;

			Length = pLogItem->Format(szText, lengthof(szText) - 1);
			szText[Length++] = L'\r';
			szText[Length++] = L'\n';
			::WriteFile(hFile, szText, Length * sizeof(WCHAR), &Size, nullptr);

			::CloseHandle(hFile);
		}
	}

	return true;
}


void CLogger::Clear()
{
	BlockLock Lock(m_Lock);

	m_LogList.clear();
}


std::size_t CLogger::GetLogCount() const
{
	BlockLock Lock(m_Lock);

	return m_LogList.size();
}


bool CLogger::GetLog(std::size_t Index, CLogItem *pItem) const
{
	if (pItem == nullptr)
		return false;

	BlockLock Lock(m_Lock);

	if (Index >= m_LogList.size())
		return false;

	*pItem = *m_LogList[Index];

	return true;
}


bool CLogger::GetLogBySerialNumber(DWORD SerialNumber, CLogItem *pItem) const
{
	if (pItem == nullptr)
		return false;

	BlockLock Lock(m_Lock);

	if (m_LogList.empty())
		return false;

	DWORD FirstSerial = m_LogList.front()->GetSerialNumber();
	if (SerialNumber < FirstSerial || SerialNumber >= FirstSerial + m_LogList.size())
		return false;

	*pItem = *m_LogList[SerialNumber - FirstSerial];

	return true;
}


bool CLogger::SetOutputToFile(bool fOutput)
{
	BlockLock Lock(m_Lock);

	m_fOutputToFile = fOutput;
	return true;
}


bool CLogger::SaveToFile(LPCTSTR pszFileName, bool fAppend)
{
	HANDLE hFile;

	hFile = ::CreateFile(
		pszFileName != nullptr ? pszFileName : m_DefaultLogFileName.c_str(),
		GENERIC_WRITE, FILE_SHARE_READ, nullptr,
		fAppend ? OPEN_ALWAYS : CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	bool fBOM = !fAppend;
	DWORD Size;

	if (fAppend) {
		static const LARGE_INTEGER Zero = {};
		LARGE_INTEGER Pos;

		if (::SetFilePointerEx(hFile, Zero, &Pos, FILE_END) && Pos.QuadPart == 0)
			fBOM = true;
	}

	if (fBOM) {
		static const WORD BOM = 0xFEFF;

		::WriteFile(hFile, &BOM, 2, &Size, nullptr);
	}

	String Text;

	GetLogText(&Text);
	::WriteFile(hFile, Text.data(), (DWORD)(Text.length() * sizeof(WCHAR)), &Size, nullptr);

	::CloseHandle(hFile);

	return true;
}


void CLogger::GetLogText(String *pText) const
{
	BlockLock Lock(m_Lock);

	for (const auto &e : m_LogList) {
		WCHAR szText[MAX_LOG_TEXT_LENGTH];

		e->Format(szText, lengthof(szText));
		*pText += szText;
		*pText += L"\r\n";
	}
}


void CLogger::GetLogText(AnsiString *pText) const
{
	BlockLock Lock(m_Lock);

	for (const auto &e : m_LogList) {
		char szText[MAX_LOG_TEXT_LENGTH];

		e->Format(szText, lengthof(szText));
		*pText += szText;
		*pText += "\r\n";
	}
}


bool CLogger::CopyToClipboard(HWND hwnd)
{
	String LogText;

	GetLogText(&LogText);

	return CopyTextToClipboard(hwnd, LogText.c_str());
}


INT_PTR CLogger::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndList = GetDlgItem(hDlg, IDC_LOG_LIST);

			ListView_SetExtendedListViewStyle(
				hwndList, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_SUBITEMIMAGES);

			static const LPCTSTR IconList[] = {
				IDI_INFORMATION,
				IDI_WARNING,
				IDI_ERROR
			};
			const int IconWidth = GetSystemMetricsWithDPI(SM_CXSMICON, m_CurrentDPI);
			const int IconHeight = GetSystemMetricsWithDPI(SM_CYSMICON, m_CurrentDPI);
			HIMAGELIST himl = ::ImageList_Create(
				IconWidth, IconHeight,
				ILC_COLOR32, lengthof(IconList), 1);
			for (LPCTSTR pszIcon : IconList) {
				HICON hico = LoadSystemIcon(pszIcon, IconWidth, IconHeight);
				::ImageList_AddIcon(himl, hico);
				::DestroyIcon(hico);
			}
			ListView_SetImageList(hwndList, himl, LVSIL_SMALL);

			// LVS_EX_SUBITEMIMAGES を指定すると、最初のカラムにもアイコン用のスペースが
			// 確保されてしまうので、幅0のダミーのカラムを作成して回避する
			LVCOLUMN lvc;
			lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt = LVCFMT_LEFT;
			lvc.cx = 0;
			lvc.pszText = const_cast<LPTSTR>(TEXT(""));
			ListView_InsertColumn(hwndList, COLUMN_DUMMY, &lvc);
			lvc.cx = 80;
			lvc.pszText = const_cast<LPTSTR>(TEXT("日時"));
			ListView_InsertColumn(hwndList, COLUMN_TIME, &lvc);
			lvc.pszText = const_cast<LPTSTR>(TEXT("内容"));
			ListView_InsertColumn(hwndList, COLUMN_TEXT, &lvc);

			LVITEM lvi;
			lvi.iItem = 0;
			m_Lock.Lock();
			ListView_SetItemCount(hwndList, (int)m_LogList.size());
			for (const auto &e : m_LogList) {
				TCHAR szTime[64];

				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = COLUMN_DUMMY;
				lvi.pszText = const_cast<LPTSTR>(TEXT(""));
				ListView_InsertItem(hwndList, &lvi);

				lvi.mask = LVIF_TEXT;
				lvi.iSubItem = COLUMN_TIME;
				e->FormatTime(szTime, lengthof(szTime));
				lvi.pszText = szTime;
				ListView_SetItem(hwndList, &lvi);

				lvi.mask = LVIF_TEXT | LVIF_IMAGE;
				lvi.iSubItem = COLUMN_TEXT;
				lvi.iImage = (int)e->GetType();
				lvi.pszText = const_cast<LPTSTR>(e->GetText());
				ListView_SetItem(hwndList, &lvi);

				lvi.iItem++;
			}
			for (int i = 1; i < NUM_COLUMNS; i++)
				ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);
			if (!m_LogList.empty())
				ListView_EnsureVisible(hwndList, (int)m_LogList.size() - 1, FALSE);
			m_Lock.Unlock();

			DlgCheckBox_Check(hDlg, IDC_LOG_OUTPUTTOFILE, m_fOutputToFile);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LOG_CLEAR:
			ListView_DeleteAllItems(GetDlgItem(hDlg, IDC_LOG_LIST));
			Clear();
			return TRUE;

		case IDC_LOG_COPY:
			CopyToClipboard(GetAppClass().UICore.GetMainWindow());
			return TRUE;

		case IDC_LOG_SAVE:
			{
				if (!SaveToFile(nullptr, false)) {
					::MessageBox(hDlg, TEXT("保存ができません。"), nullptr, MB_OK | MB_ICONEXCLAMATION);
				} else {
					TCHAR szMessage[MAX_PATH + 64];

					StringPrintf(
						szMessage,
						TEXT("ログを \"%s\" に保存しました。"), m_DefaultLogFileName.c_str());
					::MessageBox(hDlg, szMessage, TEXT("ログ保存"), MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case NM_CUSTOMDRAW:
			// ダミーの列が描画されないようにする
			{
				LPNMLVCUSTOMDRAW pnmlvcd = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

				if (pnmlvcd->nmcd.hdr.idFrom == IDC_LOG_LIST) {
					LRESULT Result = CDRF_DODEFAULT;

					switch (pnmlvcd->nmcd.dwDrawStage) {
					case CDDS_PREPAINT:
						Result = CDRF_NOTIFYITEMDRAW;
						break;

					case CDDS_ITEMPREPAINT:
						Result = CDRF_NOTIFYSUBITEMDRAW;
						break;

					case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
						if (pnmlvcd->iSubItem == 0)
							Result = CDRF_SKIPDEFAULT;
						break;
					}

					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, Result);
					return TRUE;
				}
			}
			break;

		case PSN_APPLY:
			{
				bool fOutput = DlgCheckBox_IsChecked(hDlg, IDC_LOG_OUTPUTTOFILE);

				if (fOutput != m_fOutputToFile) {
					BlockLock Lock(m_Lock);

					if (fOutput && m_LogList.size() > 0) {
						SaveToFile(nullptr, true);
					}
					m_fOutputToFile = fOutput;

					m_fChanged = true;
				}
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CLogger::RealizeStyle()
{
	CBasicDialog::RealizeStyle();

	if (m_hDlg != nullptr) {
		HWND hwndList = ::GetDlgItem(m_hDlg, IDC_LOG_LIST);

		for (int i = 1; i < NUM_COLUMNS; i++)
			ListView_SetColumnWidth(hwndList, i, LVSCW_AUTOSIZE_USEHEADER);
	}
}


}	// namespace TVTest
