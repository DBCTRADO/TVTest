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
#include "CommandLine.h"
//#include "AppMain.h"
#include "Common/DebugDef.h"


namespace TVTest
{


class CArgsParser
{
	LPWSTR *m_ppszArgList;
	int m_Args = 0;
	int m_CurPos = 0;

public:
	CArgsParser(LPCWSTR pszCmdLine);
	~CArgsParser();
	bool IsSwitch() const;
	bool IsOption(LPCWSTR pszOption) const;
	bool GetOption(LPCWSTR pszOption, bool *pValue);
	bool GetOption(LPCWSTR pszOption, String *pValue);
	bool GetOption(LPCWSTR pszOption, LPTSTR pszValue, int MaxLength);
	bool GetOption(LPCWSTR pszOption, int *pValue);
	bool GetOption(LPCWSTR pszOption, DWORD *pValue);
	bool GetOption(LPCWSTR pszOption, SYSTEMTIME *pValue);
	bool GetDurationOption(LPCWSTR pszOption, int *pValue);
	bool IsEnd() const { return m_CurPos >= m_Args; }
	bool Next();
	LPCWSTR GetText() const;
	bool GetText(LPWSTR pszText, int MaxLength) const;
	bool GetValue(int *pValue) const;
	bool GetValue(DWORD *pValue) const;
	bool GetValue(SYSTEMTIME *pValue) const;
	bool GetDurationValue(int *pValue) const;
};


CArgsParser::CArgsParser(LPCWSTR pszCmdLine)
{
	m_ppszArgList = ::CommandLineToArgvW(pszCmdLine, &m_Args);
	if (m_ppszArgList == nullptr)
		m_Args = 0;
}


CArgsParser::~CArgsParser()
{
	if (m_ppszArgList)
		::LocalFree(m_ppszArgList);
}


bool CArgsParser::IsSwitch() const
{
	if (IsEnd())
		return false;
	return m_ppszArgList[m_CurPos][0] == L'-'
		|| m_ppszArgList[m_CurPos][0] == L'/';
}


bool CArgsParser::IsOption(LPCWSTR pszOption) const
{
	if (IsEnd())
		return false;
	return ::lstrcmpi(m_ppszArgList[m_CurPos] + 1, pszOption) == 0;
}


bool CArgsParser::GetOption(LPCWSTR pszOption, bool *pValue)
{
	if (IsOption(pszOption)) {
		*pValue = true;
		return true;
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption, String *pValue)
{
	if (IsOption(pszOption)) {
		if (Next()) {
			StringUtility::Assign(*pValue, GetText());
			return true;
		}
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption, LPTSTR pszValue, int MaxLength)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetText(pszValue, MaxLength);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption, int *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption, DWORD *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption, SYSTEMTIME *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetDurationOption(LPCWSTR pszOption, int *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetDurationValue(pValue);
	}
	return false;
}


bool CArgsParser::Next()
{
	m_CurPos++;
	return m_CurPos < m_Args;
}


LPCWSTR CArgsParser::GetText() const
{
	if (IsEnd())
		return L"";
	return m_ppszArgList[m_CurPos];
}


bool CArgsParser::GetText(LPWSTR pszText, int MaxLength) const
{
	if (IsEnd())
		return false;
	if (::lstrlen(m_ppszArgList[m_CurPos]) >= MaxLength)
		return false;
	StringCopy(pszText, m_ppszArgList[m_CurPos]);
	return true;
}


bool CArgsParser::GetValue(int *pValue) const
{
	if (IsEnd())
		return false;
	*pValue = _wtoi(m_ppszArgList[m_CurPos]);
	return true;
}


bool CArgsParser::GetValue(DWORD *pValue) const
{
	if (IsEnd())
		return false;
	*pValue = wcstoul(m_ppszArgList[m_CurPos], nullptr, 0);
	return true;
}


bool CArgsParser::GetValue(SYSTEMTIME *pValue) const
{
	if (IsEnd())
		return false;

	/*
		日付と時刻のパースを行う
		Y-M-DTh:m:s や Y/M/D-h:m:s などをアバウトに受け付ける
		Y、M、sは省略可能
		日付のみを指定した場合、その日の0時0分0秒からとする
		時刻のみを指定した場合、次にその時刻が来る時とする
	*/
	SYSTEMTIME CurTime, Time = {};
	::GetLocalTime(&CurTime);

	WORD Date[3], TimeValue[3];
	int DateCount = 0, TimeCount = 0;
	UINT Value = 0;
	for (LPCWSTR p = m_ppszArgList[m_CurPos];; p++) {
		if (*p >= L'0' && *p <= L'9') {
			Value = Value * 10 + (*p - L'0');
			if (Value > 0xFFFF)
				return false;
		} else {
			if (*p == L'/' || *p == L'-' || *p == L'T') {
				if (DateCount == 3)
					return false;
				Date[DateCount++] = static_cast<WORD>(Value);
			} else if (*p == L':' || *p == L'\0') {
				if (TimeCount == 3)
					return false;
				TimeValue[TimeCount++] = static_cast<WORD>(Value);
				if (*p == L'\0')
					break;
			}
			Value = 0;
		}
	}

	if ((DateCount == 0 && TimeCount < 2) || TimeCount == 1)
		return false;

	int i = 0;
	if (DateCount > 2) {
		Time.wYear = Date[i++];
		if (Time.wYear < 100)
			Time.wYear += CurTime.wYear / 100 * 100;
	}
	if (DateCount > 1) {
		Time.wMonth = Date[i++];
		if (Time.wMonth < 1 || Time.wMonth > 12)
			return false;
	}
	if (DateCount > 0) {
		Time.wDay = Date[i];
		if (Time.wDay < 1 || Time.wDay > 31)
			return false;
	}
	if (Time.wYear == 0) {
		Time.wYear = CurTime.wYear;
		if (Time.wMonth == 0) {
			Time.wMonth = CurTime.wMonth;
			if (Time.wDay == 0) {
				Time.wDay = CurTime.wDay;
			} else if (Time.wDay < CurTime.wDay) {
				Time.wMonth++;
				if (Time.wMonth > 12) {
					Time.wMonth = 1;
					Time.wYear++;
				}
			}
		} else if (Time.wMonth < CurTime.wMonth) {
			Time.wYear++;
		}
	}

	if (TimeCount > 0) {
		Time.wHour = TimeValue[0];
		if (TimeCount > 1) {
			Time.wMinute = TimeValue[1];
			if (Time.wMinute > 59)
				return false;
			if (TimeCount > 2) {
				Time.wSecond = TimeValue[2];
				if (Time.wSecond > 59) // Windowsに閏秒は無いらしい
					return false;
			}
		}
	}
	if (DateCount == 0) {
		if (Time.wHour < CurTime.wHour)
			Time.wHour += 24;
	}

	SYSTEMTIME st = {};

	st.wYear = Time.wYear;
	st.wMonth = Time.wMonth;
	st.wDay = Time.wDay;

	OffsetSystemTime(
		&st,
		static_cast<LONGLONG>(Time.wHour) * TimeConsts::SYSTEMTIME_HOUR +
		static_cast<LONGLONG>(Time.wMinute) * TimeConsts::SYSTEMTIME_MINUTE +
		static_cast<LONGLONG>(Time.wSecond) * TimeConsts::SYSTEMTIME_SECOND);

	*pValue = st;

	return true;
}


bool CArgsParser::GetDurationValue(int *pValue) const
{
	if (IsEnd())
		return false;

	// ?h?m?s 形式の時間指定をパースする
	// 単位の指定が無い場合は秒単位と解釈する
	LPCWSTR p = m_ppszArgList[m_CurPos];
	int DurationSec = 0, Duration = 0;

	while (*p != L'\0') {
		if (*p == L'-' || (*p >= L'0' && *p <= L'9')) {
			wchar_t *pEnd;
			Duration = wcstol(p, &pEnd, 10);
			if (Duration == LONG_MAX || Duration == LONG_MIN)
				return false;
			p = pEnd;
		} else {
			switch (*p) {
			case L'h': case L'H':
				DurationSec += Duration * (60 * 60);
				break;
			case L'm': case L'M':
				DurationSec += Duration * 60;
				break;
			case L's': case L'S':
				DurationSec += Duration;
				break;
			}
			Duration = 0;
			p++;
		}
	}
	DurationSec += Duration;
	*pValue = DurationSec;

	return true;
}


static bool GetIniEntry(LPCWSTR pszText, CCommandLineOptions::IniEntry *pEntry)
{
	LPCWSTR p = pszText;

	if (*p == L'[') {
		p++;
		LPCWSTR pEnd = ::StrChrW(p, L']');
		if (pEnd == nullptr)
			return false;
		pEntry->Section.assign(p, pEnd - p);
		p = pEnd + 1;
	}

	LPCWSTR pEnd = ::StrChrW(p, L'=');
	if (pEnd == nullptr || pEnd - p < 1)
		return false;
	pEntry->Name.assign(p, pEnd - p);
	p = pEnd + 1;
	pEntry->Value.assign(p);

	return true;
}




/*
	利用可能なコマンドラインオプション

	/ch             物理チャンネル (e.g. /ch 13)
	/chi            チャンネルインデックス
	/chspace        チューニング空間 (e.g. /chspace 1)
	/d              ドライバの指定 (e.g. /d BonDriver.dll)
	/f /fullscreen  フルスクリーン
	/ini            INIファイル名
	/init           初期設定ダイアログを表示する
	/inikey         INIファイルの値を設定
	/log            終了時にログを保存する
	/max            最大化状態で起動
	/min            最小化状態で起動
	/tray           起動時にタスクトレイに格納
	/posx           ウィンドウの左位置の指定
	/posy           ウィンドウの上位置の指定
	/width          ウィンドウの幅の指定
	/height         ウィンドウの高さの指定
	/mute           消音
	/nd             TSプロセッサーを無効にする
	/nid            ネットワークID
	/nodriver       BonDriverを読み込まない
	/nodshow        DirectShowの初期化をしない
	/noplugin       プラグインを読み込まない
	/noview         プレビュー無効
	/mpeg2          MPEG-2を有効
	/h264           H.264を有効
	/h265           H.265を有効
	/1seg           ワンセグモード
	/p /port        UDP のポート番号 (e.g. /p 1234)
	/plugin-        指定されたプラグインを読み込まない
	/plugindir      プラグインのフォルダ
	/rch            リモコンチャンネル
	/rec            録画
	/reccurservice  現在のサービスのみ録画
	/recstarttime   録画開始日時
	/recdelay       録画までの時間
	/recduration    録画時間
	/recexit        録画終了時にプログラムを終了
	/recfile        録画ファイル名
	/reconly        録画専用モード
	/recstop        録画停止
	/s              複数起動しない
	/sid            サービスID
	/silent         エラー時にダイアログを表示しない
	/standby        待機状態で起動
	/tsid           トランスポートストリームID
	/volume         音量
	/noepg          EPG情報の取得を行わない
	/epg            EPG番組表を表示する
	/epgonly        EPG番組表のみ表示する
	/epgtuner       EPG番組表のデフォルトチューナー
	/epgspace       EPG番組表のデフォルトチューニング空間
	/home           ホーム画面表示
	/chdisplay      チャンネル選択画面表示
	/style          スタイルファイル名
	/command        コマンド実行
	/jumplist       ジャンプリストからの起動
*/
void CCommandLineOptions::Parse(LPCWSTR pszCmdLine)
{
	CArgsParser Args(pszCmdLine);

	if (Args.IsEnd())
		return;
	do {
		if (Args.IsSwitch()) {
			if (!Args.GetOption(TEXT("1seg"), &m_f1Seg)
					&& !Args.GetOption(TEXT("ch"), &m_Channel)
					&& !Args.GetOption(TEXT("chdisplay"), &m_fChannelDisplay)
					&& !Args.GetOption(TEXT("chi"), &m_ChannelIndex)
					&& !Args.GetOption(TEXT("chspace"), &m_TuningSpace)
					&& !Args.GetOption(TEXT("command"), &m_Command)
					&& !Args.GetOption(TEXT("d"), &m_DriverName)
					&& !Args.GetOption(TEXT("epg"), &m_fShowProgramGuide)
					&& !Args.GetOption(TEXT("epgonly"), &m_fProgramGuideOnly)
					&& !Args.GetOption(TEXT("epgspace"), &m_ProgramGuideSpace)
					&& !Args.GetOption(TEXT("epgtuner"), &m_ProgramGuideTuner)
					&& !Args.GetOption(TEXT("f"), &m_fFullscreen)
					&& !Args.GetOption(TEXT("fullscreen"), &m_fFullscreen)
					&& !Args.GetOption(TEXT("h264"), &m_fH264)
					&& !Args.GetOption(TEXT("h265"), &m_fH265)
					&& !Args.GetOption(TEXT("height"), &m_WindowHeight)
					&& !Args.GetOption(TEXT("home"), &m_fHomeDisplay)
					&& !Args.GetOption(TEXT("ini"), &m_IniFileName)
					&& !Args.GetOption(TEXT("init"), &m_fInitialSettings)
					&& !Args.GetOption(TEXT("jumplist"), &m_fJumpList)
					&& !Args.GetOption(TEXT("log"), &m_fSaveLog)
					&& !Args.GetOption(TEXT("max"), &m_fMaximize)
					&& !Args.GetOption(TEXT("min"), &m_fMinimize)
					&& !Args.GetOption(TEXT("mpeg2"), &m_fMpeg2)
					&& !Args.GetOption(TEXT("mute"), &m_fMute)
					&& !Args.GetOption(TEXT("nd"), &m_fNoTSProcessor)
					&& !Args.GetOption(TEXT("nodriver"), &m_fNoDriver)
					&& !Args.GetOption(TEXT("nodshow"), &m_fNoDirectShow)
					&& !Args.GetOption(TEXT("noepg"), &m_fNoEpg)
					&& !Args.GetOption(TEXT("noplugin"), &m_fNoPlugin)
					&& !Args.GetOption(TEXT("noview"), &m_fNoView)
					&& !Args.GetOption(TEXT("nid"), &m_NetworkID)
					&& !Args.GetOption(TEXT("p"), &m_UDPPort)
					&& !Args.GetOption(TEXT("port"), &m_UDPPort)
					&& !Args.GetOption(TEXT("posx"), &m_WindowLeft)
					&& !Args.GetOption(TEXT("posy"), &m_WindowTop)
					&& !Args.GetOption(TEXT("plugindir"), &m_PluginsDirectory)
					&& !Args.GetOption(TEXT("pluginsdir"), &m_PluginsDirectory)
					&& !Args.GetOption(TEXT("rec"), &m_fRecord)
					&& !Args.GetOption(TEXT("reccurservice"), &m_fRecordCurServiceOnly)
					&& !Args.GetOption(TEXT("recstarttime"), &m_RecordStartTime)
					&& !Args.GetDurationOption(TEXT("recdelay"), &m_RecordDelay)
					&& !Args.GetDurationOption(TEXT("recduration"), &m_RecordDuration)
					&& !Args.GetOption(TEXT("recexit"), &m_fExitOnRecordEnd)
					&& !Args.GetOption(TEXT("recfile"), &m_RecordFileName)
					&& !Args.GetOption(TEXT("reconly"), &m_fRecordOnly)
					&& !Args.GetOption(TEXT("recstop"), &m_fRecordStop)
					&& !Args.GetOption(TEXT("rch"), &m_ControllerChannel)
					&& !Args.GetOption(TEXT("s"), &m_fSingleTask)
					&& !Args.GetOption(TEXT("sid"), &m_ServiceID)
					&& !Args.GetOption(TEXT("silent"), &m_fSilent)
					&& !Args.GetOption(TEXT("standby"), &m_fStandby)
					&& !Args.GetOption(TEXT("style"), &m_StyleFileName)
					&& !Args.GetOption(TEXT("tray"), &m_fTray)
					&& !Args.GetOption(TEXT("tsid"), &m_TransportStreamID)
					&& !Args.GetOption(TEXT("volume"), &m_Volume)
					&& !Args.GetOption(TEXT("width"), &m_WindowWidth)) {
				if (Args.IsOption(TEXT("inikey"))) {
					if (Args.Next()) {
						IniEntry Entry;
						if (GetIniEntry(Args.GetText(), &Entry))
							m_IniValueList.push_back(Entry);
					}
				} else if (Args.IsOption(TEXT("plugin-"))) {
					if (Args.Next()) {
						TCHAR szPlugin[MAX_PATH];
						if (Args.GetText(szPlugin, MAX_PATH))
							m_NoLoadPlugins.emplace_back(szPlugin);
					}
				} else if (Args.IsOption(TEXT("did"))) {
					if (Args.Next()) {
						const WCHAR DID = Args.GetText()[0];

						if (DID >= L'A' && DID <= L'Z')
							m_TvRockDID = DID - L'A';
						else if (DID >= L'a' && DID <= L'z')
							m_TvRockDID = DID - L'a';
					}
				}
#ifdef _DEBUG
				else {
					TRACE(TEXT("Unknown command line option {}\n"), Args.GetText());
					// プラグインで解釈するオプションもあるので…
					//GetAppClass().AddLong(TEXT("不明なコマンドラインオプション {} を無視します。"), Args.GetText());
				}
#endif
			}
		} else {
			// なぜかudp://@:1234のようにポートを指定できると思っている人が多いので、対応しておく
			if (::wcsncmp(Args.GetText(), L"udp://@:", 8) == 0)
				m_UDPPort = ::_wtoi(Args.GetText() + 8);
		}
	} while (Args.Next());
	if (m_fRecordOnly) {
		m_fNoDirectShow = true;
	}

	/*
#ifdef _DEBUG
	// コマンドラインの解析のテスト
	{
		CArgsParser Args(L"/d 120 /d -45 /d 1h30m5s /t 2011/12/25-1:30:25 /t 1:30 /t 12/25 /t 12/25-1:30 /t 12/25-26:25");
		do {
			if (Args.IsSwitch()) {
				int Duration;
				SYSTEMTIME Time;
				if (Args.GetDurationOption(L"d", &Duration)) {
					TRACE(
						L"Commandline parse test : \"{}\" {}\n",
						Args.GetText(), Duration);
				} else if (Args.GetOption(L"t", &Time)) {
					TRACE(
						L"Commandline parse test : \"{}\" {}/{}/{} {}:{}:{}\n",
						Args.GetText(),
						Time.wYear, Time.wMonth, Time.wDay, Time.wHour, Time.wMinute, Time.wSecond);
				}
			}
		} while (Args.Next());
	}
#endif
	*/
}


bool CCommandLineOptions::IsChannelSpecified() const
{
	return m_Channel > 0 || m_ControllerChannel > 0
		|| (m_ChannelIndex >= 0 && m_TuningSpace >= 0)
		|| m_ServiceID > 0 || m_NetworkID > 0 || m_TransportStreamID > 0;
}


} // namespace TVTest
