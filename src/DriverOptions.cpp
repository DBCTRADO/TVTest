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
#include "DriverOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{

namespace
{

constexpr unsigned int DRIVER_FLAG_NOSIGNALLEVEL                = 0x00000001;
constexpr unsigned int DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE   = 0x00000004;
constexpr unsigned int DRIVER_FLAG_ALLCHANNELS                  = 0x00000008;
constexpr unsigned int DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT = 0x00000010;
constexpr unsigned int DRIVER_FLAG_NOTIGNOREINITIALSTREAM       = 0x00000020;
constexpr unsigned int DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK       = 0x00000040;

constexpr unsigned int DRIVER_FLAG_MASK                         = 0x0000007F;
constexpr unsigned int DRIVER_FLAG_DEFAULTMASK                  = 0x0000003F;

}




class CDriverSettings
{
	String m_FileName;
	int m_InitialChannelType = INITIALCHANNEL_LAST;
	int m_InitialSpace = 0;
	int m_InitialChannel = 0;
	int m_InitialServiceID = -1;
	bool m_fAllChannels = false;
	CDriverOptions::BonDriverOptions m_Options;

public:
	int m_LastSpace = -1;
	int m_LastChannel = -1;
	int m_LastServiceID = -1;
	int m_LastTransportStreamID = -1;
	bool m_fLastAllChannels = false;

	CDriverSettings(LPCTSTR pszFileName);

	LPCTSTR GetFileName() const { return m_FileName.c_str(); }
	void SetFileName(LPCTSTR pszFileName) { StringUtility::Assign(m_FileName, pszFileName); }
	enum {
		INITIALCHANNEL_NONE,
		INITIALCHANNEL_LAST,
		INITIALCHANNEL_CUSTOM
	};
	int GetInitialChannelType() const { return m_InitialChannelType; }
	bool SetInitialChannelType(int Type);
	int GetInitialSpace() const { return m_InitialSpace; }
	bool SetInitialSpace(int Space);
	int GetInitialChannel() const { return m_InitialChannel; }
	bool SetInitialChannel(int Channel);
	int GetInitialServiceID() const { return m_InitialServiceID; }
	void SetInitialServiceID(int ServiceID) { m_InitialServiceID = ServiceID; }
	bool GetAllChannels() const { return m_fAllChannels; }
	void SetAllChannels(bool fAll) { m_fAllChannels = fAll; }
	const CDriverOptions::BonDriverOptions &GetOptions() const { return m_Options; }
	bool GetNoSignalLevel() const { return m_Options.fNoSignalLevel; }
	void SetNoSignalLevel(bool fNoSignalLevel) { m_Options.fNoSignalLevel = fNoSignalLevel; }
	bool GetIgnoreInitialStream() const { return m_Options.fIgnoreInitialStream; }
	void SetIgnoreInitialStream(bool fIgnore) { m_Options.fIgnoreInitialStream = fIgnore; }
	bool GetPurgeStreamOnChannelChange() const { return m_Options.fPurgeStreamOnChannelChange; }
	void SetPurgeStreamOnChannelChange(bool fPurge) { m_Options.fPurgeStreamOnChannelChange = fPurge; }
	bool GetResetChannelChangeErrorCount() const { return m_Options.fResetChannelChangeErrorCount; }
	void SetResetChannelChangeErrorCount(bool fReset) { m_Options.fResetChannelChangeErrorCount = fReset; }
	bool GetPumpStreamSyncPlayback() const { return m_Options.fPumpStreamSyncPlayback; }
	void SetPumpStreamSyncPlayback(bool fSync) { m_Options.fPumpStreamSyncPlayback = fSync; }
	DWORD GetFirstChannelSetDelay() const { return m_Options.FirstChannelSetDelay; }
	void SetFirstChannelSetDelay(DWORD Delay) { m_Options.FirstChannelSetDelay = Delay; }
	DWORD GetMinChannelChangeInterval() const { return m_Options.MinChannelChangeInterval; }
	void SetMinChannelChangeInterval(DWORD Interval) { m_Options.MinChannelChangeInterval = Interval; }
};


CDriverSettings::CDriverSettings(LPCTSTR pszFileName)
	: m_FileName(pszFileName)
	, m_Options(pszFileName)
{
}


bool CDriverSettings::SetInitialChannelType(int Type)
{
	if (Type < INITIALCHANNEL_NONE || Type > INITIALCHANNEL_CUSTOM)
		return false;
	m_InitialChannelType = Type;
	return true;
}


bool CDriverSettings::SetInitialSpace(int Space)
{
	m_InitialSpace = Space;
	return true;
}


bool CDriverSettings::SetInitialChannel(int Channel)
{
	m_InitialChannel = Channel;
	return true;
}




CDriverSettingList::CDriverSettingList(const CDriverSettingList &Src)
{
	*this = Src;
}


CDriverSettingList &CDriverSettingList::operator=(const CDriverSettingList &Src)
{
	if (&Src != this) {
		Clear();
		if (Src.m_SettingList.size() > 0) {
			m_SettingList.resize(Src.m_SettingList.size());
			for (size_t i = 0; i < Src.m_SettingList.size(); i++)
				m_SettingList[i] = std::make_unique<CDriverSettings>(*Src.m_SettingList[i]);
		}
	}
	return *this;
}


void CDriverSettingList::Clear()
{
	m_SettingList.clear();
}


bool CDriverSettingList::Add(CDriverSettings *pSettings)
{
	if (pSettings == nullptr)
		return false;
	m_SettingList.emplace_back(pSettings);
	return true;
}


CDriverSettings *CDriverSettingList::GetDriverSettings(size_t Index)
{
	if (Index >= m_SettingList.size())
		return nullptr;
	return m_SettingList[Index].get();
}


const CDriverSettings *CDriverSettingList::GetDriverSettings(size_t Index) const
{
	if (Index >= m_SettingList.size())
		return nullptr;
	return m_SettingList[Index].get();
}


int CDriverSettingList::Find(LPCTSTR pszFileName) const
{
	if (pszFileName == nullptr)
		return -1;
	for (size_t i = 0; i < m_SettingList.size(); i++) {
		if (IsEqualFileName(m_SettingList[i]->GetFileName(), pszFileName))
			return static_cast<int>(i);
	}
	return -1;
}




CDriverOptions::CDriverOptions()
	: COptions(TEXT("DriverSettings"))
{
}


CDriverOptions::~CDriverOptions()
{
	Destroy();
}


bool CDriverOptions::ReadSettings(CSettings &Settings)
{
	int NumDrivers;

	if (Settings.Read(TEXT("DriverCount"), &NumDrivers) && NumDrivers > 0) {
		for (int i = 0; i < NumDrivers; i++) {
			TCHAR szName[64], szFileName[MAX_PATH];

			StringFormat(szName, TEXT("Driver{}_FileName"), i);
			if (!Settings.Read(szName, szFileName, lengthof(szFileName)))
				break;
			if (szFileName[0] != _T('\0')) {
				CDriverSettings *pSettings = new CDriverSettings(szFileName);
				int Value;

				StringFormat(szName, TEXT("Driver{}_InitChannelType"), i);
				if (Settings.Read(szName, &Value))
					pSettings->SetInitialChannelType(Value);
				StringFormat(szName, TEXT("Driver{}_InitSpace"), i);
				if (Settings.Read(szName, &Value))
					pSettings->SetInitialSpace(Value);
				StringFormat(szName, TEXT("Driver{}_InitChannel"), i);
				if (Settings.Read(szName, &Value))
					pSettings->SetInitialChannel(Value);
				StringFormat(szName, TEXT("Driver{}_InitServiceID"), i);
				if (Settings.Read(szName, &Value))
					pSettings->SetInitialServiceID(Value);
				StringFormat(szName, TEXT("Driver{}_Options"), i);
				if (Settings.Read(szName, &Value)) {
					unsigned int Mask;
					StringFormat(szName, TEXT("Driver{}_OptionsMask"), i);
					if (!Settings.Read(szName, &Mask))
						Mask = DRIVER_FLAG_DEFAULTMASK;
					if ((Mask & DRIVER_FLAG_NOSIGNALLEVEL) != 0)
						pSettings->SetNoSignalLevel((Value & DRIVER_FLAG_NOSIGNALLEVEL) != 0);
					if ((Mask & DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE) != 0)
						pSettings->SetPurgeStreamOnChannelChange((Value & DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE) != 0);
					if ((Mask & DRIVER_FLAG_ALLCHANNELS) != 0)
						pSettings->SetAllChannels((Value & DRIVER_FLAG_ALLCHANNELS) != 0);
					if ((Mask & DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT) != 0)
						pSettings->SetResetChannelChangeErrorCount((Value & DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT) != 0);
					if ((Mask & DRIVER_FLAG_NOTIGNOREINITIALSTREAM) != 0)
						pSettings->SetIgnoreInitialStream((Value & DRIVER_FLAG_NOTIGNOREINITIALSTREAM) == 0);
					if ((Mask & DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK) != 0)
						pSettings->SetPumpStreamSyncPlayback((Value & DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK) != 0);
				}
				StringFormat(szName, TEXT("Driver{}_FirstChSetDelay"), i);
				if (Settings.Read(szName, &Value))
					pSettings->SetFirstChannelSetDelay(Value);
				StringFormat(szName, TEXT("Driver{}_MinChChangeInterval"), i);
				if (Settings.Read(szName, &Value))
					pSettings->SetMinChannelChangeInterval(Value);
				StringFormat(szName, TEXT("Driver{}_LastSpace"), i);
				if (Settings.Read(szName, &Value))
					pSettings->m_LastSpace = Value;
				StringFormat(szName, TEXT("Driver{}_LastChannel"), i);
				if (Settings.Read(szName, &Value))
					pSettings->m_LastChannel = Value;
				StringFormat(szName, TEXT("Driver{}_LastServiceID"), i);
				if (Settings.Read(szName, &Value))
					pSettings->m_LastServiceID = Value;
				StringFormat(szName, TEXT("Driver{}_LastTSID"), i);
				if (Settings.Read(szName, &Value))
					pSettings->m_LastTransportStreamID = Value;
				StringFormat(szName, TEXT("Driver{}_LastStatus"), i);
				if (Settings.Read(szName, &Value))
					pSettings->m_fLastAllChannels = (Value & 1) != 0;

				m_SettingList.Add(pSettings);
			}
		}
	}
	return true;
}


bool CDriverOptions::WriteSettings(CSettings &Settings)
{
	const int NumDrivers = static_cast<int>(m_SettingList.NumDrivers());

	Settings.Write(TEXT("DriverCount"), NumDrivers);
	for (int i = 0; i < NumDrivers; i++) {
		const CDriverSettings *pSettings = m_SettingList.GetDriverSettings(i);
		TCHAR szName[64];

		StringFormat(szName, TEXT("Driver{}_FileName"), i);
		Settings.Write(szName, pSettings->GetFileName());
		StringFormat(szName, TEXT("Driver{}_InitChannelType"), i);
		Settings.Write(szName, pSettings->GetInitialChannelType());
		StringFormat(szName, TEXT("Driver{}_InitSpace"), i);
		Settings.Write(szName, pSettings->GetInitialSpace());
		StringFormat(szName, TEXT("Driver{}_InitChannel"), i);
		Settings.Write(szName, pSettings->GetInitialChannel());
		StringFormat(szName, TEXT("Driver{}_InitServiceID"), i);
		Settings.Write(szName, pSettings->GetInitialServiceID());
		StringFormat(szName, TEXT("Driver{}_Options"), i);
		int Flags = 0;
		if (pSettings->GetNoSignalLevel())
			Flags |= DRIVER_FLAG_NOSIGNALLEVEL;
		if (pSettings->GetPurgeStreamOnChannelChange())
			Flags |= DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE;
		if (pSettings->GetAllChannels())
			Flags |= DRIVER_FLAG_ALLCHANNELS;
		if (pSettings->GetResetChannelChangeErrorCount())
			Flags |= DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT;
		if (!pSettings->GetIgnoreInitialStream())
			Flags |= DRIVER_FLAG_NOTIGNOREINITIALSTREAM;
		if (pSettings->GetPumpStreamSyncPlayback())
			Flags |= DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK;
		Settings.Write(szName, Flags);
		StringFormat(szName, TEXT("Driver{}_OptionsMask"), i);
		Settings.Write(szName, DRIVER_FLAG_MASK);
		StringFormat(szName, TEXT("Driver{}_FirstChSetDelay"), i);
		Settings.Write(szName, static_cast<unsigned int>(pSettings->GetFirstChannelSetDelay()));
		StringFormat(szName, TEXT("Driver{}_MinChChangeInterval"), i);
		Settings.Write(szName, static_cast<unsigned int>(pSettings->GetMinChannelChangeInterval()));
		StringFormat(szName, TEXT("Driver{}_LastSpace"), i);
		Settings.Write(szName, pSettings->m_LastSpace);
		StringFormat(szName, TEXT("Driver{}_LastChannel"), i);
		Settings.Write(szName, pSettings->m_LastChannel);
		StringFormat(szName, TEXT("Driver{}_LastServiceID"), i);
		Settings.Write(szName, pSettings->m_LastServiceID);
		StringFormat(szName, TEXT("Driver{}_LastTSID"), i);
		Settings.Write(szName, pSettings->m_LastTransportStreamID);
		StringFormat(szName, TEXT("Driver{}_LastStatus"), i);
		Settings.Write(szName, pSettings->m_fLastAllChannels ? 0x01U : 0x00U);
	}
	return true;
}


bool CDriverOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(
		hwndOwner,
		GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE(IDD_OPTIONS_DRIVER));
}


bool CDriverOptions::Initialize(CDriverManager *pDriverManager)
{
	m_pDriverManager = pDriverManager;
	return true;
}


bool CDriverOptions::GetInitialChannel(LPCTSTR pszFileName, ChannelInfo *pChannelInfo) const
{
	if (pszFileName == nullptr || pChannelInfo == nullptr)
		return false;

	const int Index = m_SettingList.Find(pszFileName);

	if (Index >= 0) {
		const CDriverSettings *pSettings = m_SettingList.GetDriverSettings(Index);

		switch (pSettings->GetInitialChannelType()) {
		case CDriverSettings::INITIALCHANNEL_NONE:
			//pChannelInfo->Space = -1;
			pChannelInfo->Space = pSettings->m_LastSpace;
			pChannelInfo->Channel = -1;
			pChannelInfo->ServiceID = -1;
			pChannelInfo->TransportStreamID = -1;
			//pChannelInfo->fAllChannels = false;
			pChannelInfo->fAllChannels = pSettings->m_fLastAllChannels;
			return true;
		case CDriverSettings::INITIALCHANNEL_LAST:
			pChannelInfo->Space = pSettings->m_LastSpace;
			pChannelInfo->Channel = pSettings->m_LastChannel;
			pChannelInfo->ServiceID = pSettings->m_LastServiceID;
			pChannelInfo->TransportStreamID = pSettings->m_LastTransportStreamID;
			pChannelInfo->fAllChannels = pSettings->m_fLastAllChannels;
			return true;
		case CDriverSettings::INITIALCHANNEL_CUSTOM:
			pChannelInfo->Space = pSettings->GetInitialSpace();
			pChannelInfo->Channel = pSettings->GetInitialChannel();
			pChannelInfo->ServiceID = pSettings->GetInitialServiceID();
			pChannelInfo->TransportStreamID = -1;
			pChannelInfo->fAllChannels = pSettings->GetAllChannels();
			return true;
		}
	}
	return false;
}


bool CDriverOptions::SetLastChannel(LPCTSTR pszFileName, const ChannelInfo *pChannelInfo)
{
	if (IsStringEmpty(pszFileName) || pChannelInfo == nullptr)
		return false;

	const int Index = m_SettingList.Find(pszFileName);
	CDriverSettings *pSettings;

	if (Index < 0) {
		pSettings = new CDriverSettings(pszFileName);
		m_SettingList.Add(pSettings);
	} else {
		pSettings = m_SettingList.GetDriverSettings(Index);
	}
	pSettings->m_LastSpace = pChannelInfo->Space;
	pSettings->m_LastChannel = pChannelInfo->Channel;
	pSettings->m_LastServiceID = pChannelInfo->ServiceID;
	pSettings->m_LastTransportStreamID = pChannelInfo->TransportStreamID;
	pSettings->m_fLastAllChannels = pChannelInfo->fAllChannels;
	return true;
}


bool CDriverOptions::IsNoSignalLevel(LPCTSTR pszFileName) const
{
	const CDriverSettings *pSettings = GetBonDriverSettings(pszFileName);
	if (pSettings == nullptr)
		return false;
	return pSettings->GetNoSignalLevel();
}


bool CDriverOptions::IsResetChannelChangeErrorCount(LPCTSTR pszFileName) const
{
	const CDriverSettings *pSettings = GetBonDriverSettings(pszFileName);
	if (pSettings == nullptr)
		return false;
	return pSettings->GetResetChannelChangeErrorCount();
}


bool CDriverOptions::GetBonDriverOptions(LPCTSTR pszFileName, BonDriverOptions *pOptions) const
{
	if (pOptions == nullptr)
		return false;

	const CDriverSettings *pSettings = GetBonDriverSettings(pszFileName);
	if (pSettings == nullptr)
		return false;

	*pOptions = pSettings->GetOptions();

	return true;
}


CDriverSettings *CDriverOptions::GetBonDriverSettings(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return nullptr;

	const int Index = m_SettingList.Find(pszFileName);
	if (Index < 0)
		return nullptr;

	return m_SettingList.GetDriverSettings(Index);
}


const CDriverSettings *CDriverOptions::GetBonDriverSettings(LPCTSTR pszFileName) const
{
	if (IsStringEmpty(pszFileName))
		return nullptr;

	const int Index = m_SettingList.Find(pszFileName);
	if (Index < 0)
		return nullptr;

	return m_SettingList.GetDriverSettings(Index);
}


void CDriverOptions::InitDlgItem(int Driver)
{
	EnableDlgItems(m_hDlg, IDC_DRIVEROPTIONS_FIRST, IDC_DRIVEROPTIONS_LAST, Driver >= 0);
	DlgComboBox_Clear(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE);
	DlgComboBox_Clear(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);

	if (Driver >= 0) {
		CDriverInfo *pDriverInfo = m_pDriverManager->GetDriverInfo(Driver);
		const LPCTSTR pszFileName = pDriverInfo->GetFileName();
		const CDriverSettings *pSettings = reinterpret_cast<CDriverSettings*>(DlgComboBox_GetItemData(m_hDlg, IDC_DRIVEROPTIONS_DRIVERLIST, Driver));

		const int InitChannelType = pSettings->GetInitialChannelType();
		::CheckRadioButton(
			m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_NONE,
			IDC_DRIVEROPTIONS_INITCHANNEL_CUSTOM,
			IDC_DRIVEROPTIONS_INITCHANNEL_NONE + InitChannelType);
		EnableDlgItems(
			m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,
			IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,
			InitChannelType == CDriverSettings::INITIALCHANNEL_CUSTOM);
		const bool fCur = IsEqualFileName(
			pszFileName,
			::PathFindFileName(GetAppClass().CoreEngine.GetDriverFileName()));
		if (fCur || pDriverInfo->LoadTuningSpaceList(CDriverInfo::LoadTuningSpaceListMode::UseDriverNoOpen)) {
			const CTuningSpaceList *pTuningSpaceList;
			int NumSpaces;

			if (fCur) {
				pTuningSpaceList = GetAppClass().ChannelManager.GetDriverTuningSpaceList();
				NumSpaces = pTuningSpaceList->NumSpaces();
			} else {
				pTuningSpaceList = pDriverInfo->GetTuningSpaceList();
				NumSpaces = pTuningSpaceList->NumSpaces();
				if (NumSpaces < pDriverInfo->GetDriverSpaceList()->NumSpaces())
					NumSpaces = pDriverInfo->GetDriverSpaceList()->NumSpaces();
			}
			DlgComboBox_AddString(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE, TEXT("すべて"));
			for (int i = 0; i < NumSpaces; i++) {
				LPCTSTR pszName = pTuningSpaceList->GetTuningSpaceName(i);
				TCHAR szName[16];

				if (pszName == nullptr && !fCur)
					pszName = pDriverInfo->GetDriverSpaceList()->GetTuningSpaceName(i);
				if (pszName == nullptr) {
					StringFormat(szName, TEXT("Space {}"), i + 1);
					pszName = szName;
				}
				DlgComboBox_AddString(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE, pszName);
			}
			if (pSettings->GetAllChannels()) {
				DlgComboBox_SetCurSel(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE, 0);
			} else {
				const int Space = pSettings->GetInitialSpace();
				if (Space >= 0)
					DlgComboBox_SetCurSel(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE, Space + 1);
			}
			SetChannelList(Driver);
			int Sel = 0;
			if (pSettings->GetInitialSpace() >= 0
					&& pSettings->GetInitialChannel() >= 0) {
				const int Count = static_cast<int>(DlgComboBox_GetCount(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL));
				for (int i = 1; i < Count; i++) {
					const CChannelInfo *pChInfo = m_InitChannelList.GetChannelInfo(
						static_cast<int>(DlgComboBox_GetItemData(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, i)));
					if (pChInfo->GetSpace() == pSettings->GetInitialSpace()
							&& pChInfo->GetChannelIndex() == pSettings->GetInitialChannel()
							&& (pSettings->GetInitialServiceID() < 0
								|| pChInfo->GetServiceID() == pSettings->GetInitialServiceID())) {
						Sel = i;
						break;
					}
				}
			}
			DlgComboBox_SetCurSel(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, Sel);
		}

		DlgCheckBox_Check(
			m_hDlg, IDC_DRIVEROPTIONS_NOSIGNALLEVEL,
			pSettings->GetNoSignalLevel());
		DlgCheckBox_Check(
			m_hDlg, IDC_DRIVEROPTIONS_IGNOREINITIALSTREAM,
			pSettings->GetIgnoreInitialStream());
		DlgCheckBox_Check(
			m_hDlg, IDC_DRIVEROPTIONS_PURGESTREAMONCHANNELCHANGE,
			pSettings->GetPurgeStreamOnChannelChange());
		DlgCheckBox_Check(
			m_hDlg, IDC_DRIVEROPTIONS_RESETCHANNELCHANGEERRORCOUNT,
			pSettings->GetResetChannelChangeErrorCount());
		DlgCheckBox_Check(
			m_hDlg, IDC_DRIVEROPTIONS_PUMPSTREAMSYNCPLAYBACK,
			pSettings->GetPumpStreamSyncPlayback());

		::SetDlgItemInt(
			m_hDlg, IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY,
			pSettings->GetFirstChannelSetDelay(), FALSE);
		DlgUpDown_SetRange(
			m_hDlg, IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY_SPIN,
			0, LibISDB::BonDriverSourceFilter::FIRST_CHANNEL_SET_DELAY_MAX);
		TCHAR szText[64];
		StringFormat(szText, TEXT("ms (0～{})"), LibISDB::BonDriverSourceFilter::FIRST_CHANNEL_SET_DELAY_MAX);
		::SetDlgItemText(m_hDlg, IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY_UNIT, szText);

		::SetDlgItemInt(
			m_hDlg, IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL,
			pSettings->GetMinChannelChangeInterval(), FALSE);
		DlgUpDown_SetRange(
			m_hDlg, IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_SPIN,
			0, LibISDB::BonDriverSourceFilter::CHANNEL_CHANGE_INTERVAL_MAX);
		StringFormat(szText, TEXT("ms (0～{})"), LibISDB::BonDriverSourceFilter::CHANNEL_CHANGE_INTERVAL_MAX);
		::SetDlgItemText(m_hDlg, IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_UNIT, szText);
	} else {
		::SetDlgItemText(m_hDlg, IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY, TEXT(""));
		::SetDlgItemText(m_hDlg, IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL, TEXT(""));
	}
}


void CDriverOptions::SetChannelList(int Driver)
{
	const CDriverSettings *pSettings = reinterpret_cast<CDriverSettings*>(
		DlgComboBox_GetItemData(m_hDlg, IDC_DRIVEROPTIONS_DRIVERLIST, Driver));
	if (pSettings == nullptr)
		return;
	const bool fCur = IsEqualFileName(
		pSettings->GetFileName(),
		::PathFindFileName(GetAppClass().CoreEngine.GetDriverFileName()));
	const CDriverInfo *pDriverInfo;

	if (!fCur) {
		pDriverInfo = m_pDriverManager->GetDriverInfo(Driver);
	}
	if (pSettings->GetAllChannels()) {
		DlgComboBox_AddString(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, TEXT("指定なし"));
		DlgComboBox_SetItemData(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, 0, -1);
		for (int i = 0;; i++) {
			const CChannelList *pChannelList;

			if (fCur)
				pChannelList = GetAppClass().ChannelManager.GetFileChannelList(i);
			else
				pChannelList = pDriverInfo->GetChannelList(i);
			if (pChannelList == nullptr)
				break;
			AddChannelList(pChannelList);
		}
	} else {
		const int i = pSettings->GetInitialSpace();
		if (i >= 0) {
			const CChannelList *pChannelList;

			if (fCur)
				pChannelList = GetAppClass().ChannelManager.GetChannelList(i);
			else
				pChannelList = pDriverInfo->GetChannelList(i);
			if (pChannelList != nullptr) {
				DlgComboBox_AddString(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, TEXT("指定なし"));
				DlgComboBox_SetItemData(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, 0, -1);
				AddChannelList(pChannelList);
			}
		}
	}
}


void CDriverOptions::AddChannelList(const CChannelList *pChannelList)
{
	m_InitChannelList.Clear();

	for (int i = 0; i < pChannelList->NumChannels(); i++) {
		const CChannelInfo *pChannelInfo = pChannelList->GetChannelInfo(i);

		if (!pChannelInfo->IsEnabled())
			continue;
		m_InitChannelList.AddChannel(*pChannelInfo);
		const int Index = static_cast<int>(DlgComboBox_AddString(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, pChannelInfo->GetName()));
		DlgComboBox_SetItemData(m_hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, Index, m_InitChannelList.NumChannels() - 1);
	}
}


CDriverSettings *CDriverOptions::GetCurSelDriverSettings() const
{
	const int Sel = static_cast<int>(DlgComboBox_GetCurSel(m_hDlg, IDC_DRIVEROPTIONS_DRIVERLIST));

	if (Sel < 0)
		return nullptr;
	return reinterpret_cast<CDriverSettings*>(
		DlgComboBox_GetItemData(m_hDlg, IDC_DRIVEROPTIONS_DRIVERLIST, Sel));
}


INT_PTR CDriverOptions::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			if (m_pDriverManager != nullptr
					&& m_pDriverManager->NumDrivers() > 0) {

				m_CurSettingList = m_SettingList;
				for (int i = 0; i < m_pDriverManager->NumDrivers(); i++) {
					const LPCTSTR pszFileName = m_pDriverManager->GetDriverInfo(i)->GetFileName();
					CDriverSettings *pSettings;

					DlgComboBox_AddString(hDlg, IDC_DRIVEROPTIONS_DRIVERLIST, pszFileName);
					const int Index = m_CurSettingList.Find(pszFileName);
					if (Index < 0) {
						pSettings = new CDriverSettings(pszFileName);
						m_CurSettingList.Add(pSettings);
					} else {
						pSettings = m_CurSettingList.GetDriverSettings(Index);
					}
					DlgComboBox_SetItemData(hDlg, IDC_DRIVEROPTIONS_DRIVERLIST, i, reinterpret_cast<LPARAM>(pSettings));
				}
				int CurDriver = -1;
				const LPCTSTR pszCurDriverName = GetAppClass().CoreEngine.GetDriverFileName();
				if (pszCurDriverName[0] != '\0') {
					CurDriver = static_cast<int>(DlgComboBox_FindStringExact(
						hDlg, IDC_DRIVEROPTIONS_DRIVERLIST,
						-1, ::PathFindFileName(pszCurDriverName)));
				}
				DlgComboBox_SetCurSel(hDlg, IDC_DRIVEROPTIONS_DRIVERLIST, CurDriver);
				DlgComboBox_SetCueBanner(
					hDlg, IDC_DRIVEROPTIONS_DRIVERLIST,
					TEXT("設定する BonDriver を選択してください"));
				InitDlgItem(CurDriver);
			} else {
				EnableDlgItems(hDlg, IDC_DRIVEROPTIONS_FIRST, IDC_DRIVEROPTIONS_LAST, false);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DRIVEROPTIONS_DRIVERLIST:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				InitDlgItem(static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_DRIVEROPTIONS_DRIVERLIST)));
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_NONE:
		case IDC_DRIVEROPTIONS_INITCHANNEL_LAST:
		case IDC_DRIVEROPTIONS_INITCHANNEL_CUSTOM:
			{
				const int Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_DRIVEROPTIONS_DRIVERLIST));

				if (Sel >= 0) {
					CDriverSettings *pSettings = reinterpret_cast<CDriverSettings*>(
						DlgComboBox_GetItemData(hDlg, IDC_DRIVEROPTIONS_DRIVERLIST, Sel));

					pSettings->SetInitialChannelType(LOWORD(wParam) - IDC_DRIVEROPTIONS_INITCHANNEL_NONE);
					EnableDlgItems(
						hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,
						IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,
						pSettings->GetInitialChannelType() == CDriverSettings::INITIALCHANNEL_CUSTOM);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_SPACE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				DlgComboBox_Clear(hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);
				if (pSettings != nullptr) {
					const int Space = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_SPACE)) - 1;

					pSettings->SetInitialSpace(Space);
					pSettings->SetAllChannels(Space < 0);
					pSettings->SetInitialChannel(-1);
					pSettings->SetInitialServiceID(-1);
					SetChannelList(static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_DRIVEROPTIONS_DRIVERLIST)));
					DlgComboBox_SetCurSel(hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, 0);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					const int Sel = static_cast<int>(DlgComboBox_GetCurSel(hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL));
					int Channel = -1, ServiceID = -1;

					if (Sel > 0) {
						const CChannelInfo *pChInfo = m_InitChannelList.GetChannelInfo(
							static_cast<int>(DlgComboBox_GetItemData(hDlg, IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL, Sel)));
						if (pChInfo != nullptr) {
							pSettings->SetInitialSpace(pChInfo->GetSpace());
							Channel = pChInfo->GetChannelIndex();
							ServiceID = pChInfo->GetServiceID();
						}
					}
					pSettings->SetInitialChannel(Channel);
					pSettings->SetInitialServiceID(ServiceID);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_NOSIGNALLEVEL:
			{
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					pSettings->SetNoSignalLevel(DlgCheckBox_IsChecked(hDlg, IDC_DRIVEROPTIONS_NOSIGNALLEVEL));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_IGNOREINITIALSTREAM:
			{
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					pSettings->SetIgnoreInitialStream(DlgCheckBox_IsChecked(hDlg, IDC_DRIVEROPTIONS_IGNOREINITIALSTREAM));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_PURGESTREAMONCHANNELCHANGE:
			{
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					pSettings->SetPurgeStreamOnChannelChange(DlgCheckBox_IsChecked(hDlg, IDC_DRIVEROPTIONS_PURGESTREAMONCHANNELCHANGE));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_RESETCHANNELCHANGEERRORCOUNT:
			{
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					pSettings->SetResetChannelChangeErrorCount(DlgCheckBox_IsChecked(hDlg, IDC_DRIVEROPTIONS_RESETCHANNELCHANGEERRORCOUNT));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_PUMPSTREAMSYNCPLAYBACK:
			{
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					pSettings->SetPumpStreamSyncPlayback(
						DlgCheckBox_IsChecked(hDlg, IDC_DRIVEROPTIONS_PUMPSTREAMSYNCPLAYBACK));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY:
			if (HIWORD(wParam) == EN_CHANGE) {
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					const DWORD Delay = ::GetDlgItemInt(hDlg, IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY, nullptr, FALSE);
					if (Delay <= LibISDB::BonDriverSourceFilter::FIRST_CHANNEL_SET_DELAY_MAX)
						pSettings->SetFirstChannelSetDelay(Delay);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL:
			if (HIWORD(wParam) == EN_CHANGE) {
				CDriverSettings *pSettings = GetCurSelDriverSettings();

				if (pSettings != nullptr) {
					const DWORD Interval = ::GetDlgItemInt(hDlg, IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL, nullptr, FALSE);
					if (Interval <= LibISDB::BonDriverSourceFilter::CHANNEL_CHANGE_INTERVAL_MAX)
						pSettings->SetMinChannelChangeInterval(Interval);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case PSN_APPLY:
			m_SettingList = m_CurSettingList;
			GetAppClass().Core.ApplyBonDriverOptions();
			m_fChanged = true;
			break;
		}
		break;

	case WM_DESTROY:
		m_CurSettingList.Clear();
		return TRUE;
	}

	return FALSE;
}




CDriverOptions::BonDriverOptions::BonDriverOptions(LPCTSTR pszBonDriverName)
{
	CDriverManager::TunerSpec Spec;
	if (GetAppClass().DriverManager.GetTunerSpec(pszBonDriverName, &Spec)) {
		if (!!(Spec.Flags &
				(CDriverManager::TunerSpec::Flag::Network |
				 CDriverManager::TunerSpec::Flag::File)))
			fNoSignalLevel = true;
		if (!!(Spec.Flags &
				(CDriverManager::TunerSpec::Flag::Network |
				 CDriverManager::TunerSpec::Flag::File |
				 CDriverManager::TunerSpec::Flag::Volatile)))
			fIgnoreInitialStream = false;
		if (!!(Spec.Flags & CDriverManager::TunerSpec::Flag::File))
			fPumpStreamSyncPlayback = true;
	}
}


}	// namespace TVTest
