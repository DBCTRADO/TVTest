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


#ifndef TVTEST_CHANNEL_SCAN_H
#define TVTEST_CHANNEL_SCAN_H


#include "CoreEngine.h"
#include "ChannelList.h"
#include "Options.h"


namespace TVTest
{

	class CChannelScan
		: public COptions
	{
	public:
		~CChannelScan();

	// COptions
		bool Apply(DWORD Flags) override;

	// CSettingsBase
		bool ReadSettings(CSettings &Settings) override;
		bool WriteSettings(CSettings &Settings) override;

	// CBasicDialog
		bool Create(HWND hwndOwner) override;

	// CChannelScan
		bool SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList);
		const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
		bool IsScanning() const { return m_hScanThread != nullptr; }

		bool AutoUpdateChannelList(CTuningSpaceList *pTuningSpaceList, std::vector<String> *pMessageList = nullptr);

	private:
		enum {
			UPDATE_CHANNELLIST = 0x0000001UL,
			UPDATE_PREVIEW     = 0x0000002UL
		};

		enum {
			COLUMN_NAME,
			COLUMN_SERVICETYPE,
			COLUMN_CHANNELNAME,
			COLUMN_SERVICEID,
			COLUMN_REMOTECONTROLKEYID,
			COLUMN_CHANNELINDEX,
			NUM_COLUMNS
		};

		// チャンネルリストのソートクラス
		class CChannelListSort
		{
			static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

		public:
			CChannelListSort() = default;
			CChannelListSort(int Column, bool fDescending = false);

			void Sort(HWND hwndList);
			bool UpdateChannelList(HWND hwndList, CChannelList *pList);

			int m_Column = COLUMN_CHANNELINDEX;
			bool m_fDescending = false;
		};

		class CScanSettingsDialog
			: public CBasicDialog
		{
		public:
			CScanSettingsDialog(CChannelScan *pChannelScan);

			bool Show(HWND hwndOwner) override;

		private:
			INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

			CChannelScan *m_pChannelScan;
		};

		class CScanDialog
			: public CBasicDialog
		{
		public:
			CScanDialog(CChannelScan *pChannelScan);

			bool Show(HWND hwndOwner) override;

		private:
			INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

			CChannelScan *m_pChannelScan;
		};

		int m_ScanSpace = -1;
		int m_ScanChannel = -1;
		const CTuningSpaceList *m_pOriginalTuningSpaceList = nullptr;
		CTuningSpaceList m_TuningSpaceList;
		CChannelList m_ScanningChannelList;
		std::vector<String> m_BonDriverChannelList;
		bool m_fScanService = true;          /**< サービスを検索 */
		bool m_fIgnoreSignalLevel = false;   /**< 信号レベルを無視 */
		float m_SignalLevelThreshold = 7.0f; /**< 信号レベルの閾値 */
		unsigned int m_ScanWait = 5000;      /**< チャンネル切り替え後の待ち時間(ms) */
		int m_RetryCount = 4;                /**< 情報取得の再試行回数 */
		unsigned int m_RetryInterval = 1000; /**< 再試行の間隔(ms) */
		bool m_fDetectDataService = true;    /**< データ放送サービスを検出 */
		bool m_fDetect1SegService = true;    /**< ワンセグサービスを検出 */
		bool m_fDetectAudioService = true;   /**< 音声サービスを検出 */
		bool m_fUpdated = false;
		bool m_fScaned = false;
		bool m_fRestorePreview = false;
		HWND m_hScanDlg = nullptr;
		HANDLE m_hScanThread = nullptr;
		HANDLE m_hCancelEvent = nullptr;
		bool m_fCancelled = false;
		int m_SortColumn = -1;
		bool m_fSortDescending = false;
		bool m_fChanging = false;
		float m_MaxSignalLevel = 0.0f;
		float m_ChannelMaxSignalLevel = 0.0f;
		DWORD m_MaxBitRate = 0;
		std::vector<float> m_ChannelSignalLevel;

	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		void InsertChannelInfo(int Index, const CChannelInfo *pChInfo, bool fServiceType);
		void SetChannelList(int Space);
		CChannelInfo *GetSelectedChannelInfo() const;
		bool LoadPreset(LPCTSTR pszFileName, CChannelList *pChannelList, int Space, bool *pfCorrupted);
		bool SetPreset(bool fAuto);
		void Scan();
		float GetSignalLevel();
		INT_PTR ScanDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
		bool IsScanService(const LibISDB::AnalyzerFilter::SDTServiceInfo &ServiceInfo, bool fData = true) const;
		bool IsScanServiceType(BYTE ServiceType, bool fData = true) const;

		static unsigned int __stdcall ScanProc(LPVOID lpParameter);
	};

}	// namespace TVTest


#endif
