#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "DriverOptions.h"
#include "DialogUtil.h"
#include "resource.h"
#include "Common/DebugDef.h"


#define DRIVER_FLAG_NOSIGNALLEVEL					0x00000001
#define DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE		0x00000004
#define DRIVER_FLAG_ALLCHANNELS						0x00000008
#define DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT	0x00000010
#define DRIVER_FLAG_NOTIGNOREINITIALSTREAM			0x00000020
#define DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK			0x00000040

#define DRIVER_FLAG_MASK							0x0000007F
#define DRIVER_FLAG_DEFAULTMASK						0x0000003F




class CDriverSettings
{
	TVTest::String m_FileName;
	int m_InitialChannelType;
	int m_InitialSpace;
	int m_InitialChannel;
	int m_InitialServiceID;
	bool m_fAllChannels;
	CDriverOptions::BonDriverOptions m_Options;

public:
	int m_LastSpace;
	int m_LastChannel;
	int m_LastServiceID;
	int m_LastTransportStreamID;
	bool m_fLastAllChannels;

	CDriverSettings(LPCTSTR pszFileName);
	LPCTSTR GetFileName() const { return m_FileName.c_str(); }
	void SetFileName(LPCTSTR pszFileName) { TVTest::StringUtility::Assign(m_FileName,pszFileName); }
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
	void SetInitialServiceID(int ServiceID) { m_InitialServiceID=ServiceID; }
	bool GetAllChannels() const { return m_fAllChannels; }
	void SetAllChannels(bool fAll) { m_fAllChannels=fAll; }
	const CDriverOptions::BonDriverOptions &GetOptions() const { return m_Options; }
	bool GetNoSignalLevel() const { return m_Options.fNoSignalLevel; }
	void SetNoSignalLevel(bool fNoSignalLevel) { m_Options.fNoSignalLevel=fNoSignalLevel; }
	bool GetIgnoreInitialStream() const { return m_Options.fIgnoreInitialStream; }
	void SetIgnoreInitialStream(bool fIgnore) { m_Options.fIgnoreInitialStream=fIgnore; }
	bool GetPurgeStreamOnChannelChange() const { return m_Options.fPurgeStreamOnChannelChange; }
	void SetPurgeStreamOnChannelChange(bool fPurge) { m_Options.fPurgeStreamOnChannelChange=fPurge; }
	bool GetResetChannelChangeErrorCount() const { return m_Options.fResetChannelChangeErrorCount; }
	void SetResetChannelChangeErrorCount(bool fReset) { m_Options.fResetChannelChangeErrorCount=fReset; }
	bool GetPumpStreamSyncPlayback() const { return m_Options.fPumpStreamSyncPlayback; }
	void SetPumpStreamSyncPlayback(bool fSync) { m_Options.fPumpStreamSyncPlayback=fSync; }
	DWORD GetFirstChannelSetDelay() const { return m_Options.FirstChannelSetDelay; }
	void SetFirstChannelSetDelay(DWORD Delay) { m_Options.FirstChannelSetDelay=Delay; }
	DWORD GetMinChannelChangeInterval() const { return m_Options.MinChannelChangeInterval; }
	void SetMinChannelChangeInterval(DWORD Interval) { m_Options.MinChannelChangeInterval=Interval; }
};


CDriverSettings::CDriverSettings(LPCTSTR pszFileName)
	: m_FileName(pszFileName)
	, m_InitialChannelType(INITIALCHANNEL_LAST)
	, m_InitialSpace(0)
	, m_InitialChannel(0)
	, m_InitialServiceID(-1)
	, m_fAllChannels(false)
	, m_Options(pszFileName)

	, m_LastSpace(-1)
	, m_LastChannel(-1)
	, m_LastServiceID(-1)
	, m_LastTransportStreamID(-1)
	, m_fLastAllChannels(false)
{
}


bool CDriverSettings::SetInitialChannelType(int Type)
{
	if (Type<INITIALCHANNEL_NONE || Type>INITIALCHANNEL_CUSTOM)
		return false;
	m_InitialChannelType=Type;
	return true;
}


bool CDriverSettings::SetInitialSpace(int Space)
{
	m_InitialSpace=Space;
	return true;
}


bool CDriverSettings::SetInitialChannel(int Channel)
{
	m_InitialChannel=Channel;
	return true;
}




CDriverSettingList::CDriverSettingList()
{
}


CDriverSettingList::CDriverSettingList(const CDriverSettingList &Src)
{
	*this=Src;
}


CDriverSettingList::~CDriverSettingList()
{
	Clear();
}


CDriverSettingList &CDriverSettingList::operator=(const CDriverSettingList &Src)
{
	if (&Src!=this) {
		Clear();
		if (Src.m_SettingList.size()>0) {
			m_SettingList.resize(Src.m_SettingList.size());
			for (size_t i=0;i<Src.m_SettingList.size();i++)
				m_SettingList[i]=new CDriverSettings(*Src.m_SettingList[i]);
		}
	}
	return *this;
}


void CDriverSettingList::Clear()
{
	for (size_t i=0;i<m_SettingList.size();i++)
		delete m_SettingList[i];
	m_SettingList.clear();
}


bool CDriverSettingList::Add(CDriverSettings *pSettings)
{
	if (pSettings==NULL)
		return false;
	m_SettingList.push_back(pSettings);
	return true;
}


CDriverSettings *CDriverSettingList::GetDriverSettings(size_t Index)
{
	if (Index>=m_SettingList.size())
		return NULL;
	return m_SettingList[Index];
}


const CDriverSettings *CDriverSettingList::GetDriverSettings(size_t Index) const
{
	if (Index>=m_SettingList.size())
		return NULL;
	return m_SettingList[Index];
}


int CDriverSettingList::Find(LPCTSTR pszFileName) const
{
	if (pszFileName==NULL)
		return -1;
	for (size_t i=0;i<m_SettingList.size();i++) {
		if (IsEqualFileName(m_SettingList[i]->GetFileName(),pszFileName))
			return (int)i;
	}
	return -1;
}




CDriverOptions::CDriverOptions()
	: COptions(TEXT("DriverSettings"))
	, m_pDriverManager(NULL)
{
}


CDriverOptions::~CDriverOptions()
{
	Destroy();
}


bool CDriverOptions::ReadSettings(CSettings &Settings)
{
	int NumDrivers;

	if (Settings.Read(TEXT("DriverCount"),&NumDrivers) && NumDrivers>0) {
		for (int i=0;i<NumDrivers;i++) {
			TCHAR szName[64],szFileName[MAX_PATH];

			::wsprintf(szName,TEXT("Driver%d_FileName"),i);
			if (!Settings.Read(szName,szFileName,lengthof(szFileName)))
				break;
			if (szFileName[0]!=_T('\0')) {
				CDriverSettings *pSettings=new CDriverSettings(szFileName);
				int Value;

				::wsprintf(szName,TEXT("Driver%d_InitChannelType"),i);
				if (Settings.Read(szName,&Value))
					pSettings->SetInitialChannelType(Value);
				::wsprintf(szName,TEXT("Driver%d_InitSpace"),i);
				if (Settings.Read(szName,&Value))
					pSettings->SetInitialSpace(Value);
				::wsprintf(szName,TEXT("Driver%d_InitChannel"),i);
				if (Settings.Read(szName,&Value))
					pSettings->SetInitialChannel(Value);
				::wsprintf(szName,TEXT("Driver%d_InitServiceID"),i);
				if (Settings.Read(szName,&Value))
					pSettings->SetInitialServiceID(Value);
				::wsprintf(szName,TEXT("Driver%d_Options"),i);
				if (Settings.Read(szName,&Value)) {
					unsigned int Mask;
					::wsprintf(szName,TEXT("Driver%d_OptionsMask"),i);
					if (!Settings.Read(szName,&Mask))
						Mask=DRIVER_FLAG_DEFAULTMASK;
					if ((Mask & DRIVER_FLAG_NOSIGNALLEVEL)!=0)
						pSettings->SetNoSignalLevel((Value & DRIVER_FLAG_NOSIGNALLEVEL)!=0);
					if ((Mask & DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE)!=0)
						pSettings->SetPurgeStreamOnChannelChange((Value & DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE)!=0);
					if ((Mask & DRIVER_FLAG_ALLCHANNELS)!=0)
						pSettings->SetAllChannels((Value & DRIVER_FLAG_ALLCHANNELS)!=0);
					if ((Mask & DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT)!=0)
						pSettings->SetResetChannelChangeErrorCount((Value & DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT)!=0);
					if ((Mask & DRIVER_FLAG_NOTIGNOREINITIALSTREAM)!=0)
						pSettings->SetIgnoreInitialStream((Value & DRIVER_FLAG_NOTIGNOREINITIALSTREAM)==0);
					if ((Mask & DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK)!=0)
						pSettings->SetPumpStreamSyncPlayback((Value & DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK)!=0);
				}
				::wsprintf(szName,TEXT("Driver%d_FirstChSetDelay"),i);
				if (Settings.Read(szName,&Value))
					pSettings->SetFirstChannelSetDelay(Value);
				::wsprintf(szName,TEXT("Driver%d_MinChChangeInterval"),i);
				if (Settings.Read(szName,&Value))
					pSettings->SetMinChannelChangeInterval(Value);
				::wsprintf(szName,TEXT("Driver%d_LastSpace"),i);
				if (Settings.Read(szName,&Value))
					pSettings->m_LastSpace=Value;
				::wsprintf(szName,TEXT("Driver%d_LastChannel"),i);
				if (Settings.Read(szName,&Value))
					pSettings->m_LastChannel=Value;
				::wsprintf(szName,TEXT("Driver%d_LastServiceID"),i);
				if (Settings.Read(szName,&Value))
					pSettings->m_LastServiceID=Value;
				::wsprintf(szName,TEXT("Driver%d_LastTSID"),i);
				if (Settings.Read(szName,&Value))
					pSettings->m_LastTransportStreamID=Value;
				::wsprintf(szName,TEXT("Driver%d_LastStatus"),i);
				if (Settings.Read(szName,&Value))
					pSettings->m_fLastAllChannels=(Value&1)!=0;

				m_SettingList.Add(pSettings);
			}
		}
	}
	return true;
}


bool CDriverOptions::WriteSettings(CSettings &Settings)
{
	int NumDrivers=(int)m_SettingList.NumDrivers();

	Settings.Write(TEXT("DriverCount"),NumDrivers);
	for (int i=0;i<NumDrivers;i++) {
		const CDriverSettings *pSettings=m_SettingList.GetDriverSettings(i);
		TCHAR szName[64];

		::wsprintf(szName,TEXT("Driver%d_FileName"),i);
		Settings.Write(szName,pSettings->GetFileName());
		::wsprintf(szName,TEXT("Driver%d_InitChannelType"),i);
		Settings.Write(szName,pSettings->GetInitialChannelType());
		::wsprintf(szName,TEXT("Driver%d_InitSpace"),i);
		Settings.Write(szName,pSettings->GetInitialSpace());
		::wsprintf(szName,TEXT("Driver%d_InitChannel"),i);
		Settings.Write(szName,pSettings->GetInitialChannel());
		::wsprintf(szName,TEXT("Driver%d_InitServiceID"),i);
		Settings.Write(szName,pSettings->GetInitialServiceID());
		::wsprintf(szName,TEXT("Driver%d_Options"),i);
		int Flags=0;
		if (pSettings->GetNoSignalLevel())
			Flags|=DRIVER_FLAG_NOSIGNALLEVEL;
		if (pSettings->GetPurgeStreamOnChannelChange())
			Flags|=DRIVER_FLAG_PURGESTREAMONCHANNELCHANGE;
		if (pSettings->GetAllChannels())
			Flags|=DRIVER_FLAG_ALLCHANNELS;
		if (pSettings->GetResetChannelChangeErrorCount())
			Flags|=DRIVER_FLAG_RESETCHANNELCHANGEERRORCOUNT;
		if (!pSettings->GetIgnoreInitialStream())
			Flags|=DRIVER_FLAG_NOTIGNOREINITIALSTREAM;
		if (pSettings->GetPumpStreamSyncPlayback())
			Flags|=DRIVER_FLAG_PUMPSTREAMSYNCPLAYBACK;
		Settings.Write(szName,Flags);
		::wsprintf(szName,TEXT("Driver%d_OptionsMask"),i);
		Settings.Write(szName,DRIVER_FLAG_MASK);
		::wsprintf(szName,TEXT("Driver%d_FirstChSetDelay"),i);
		Settings.Write(szName,(unsigned int)pSettings->GetFirstChannelSetDelay());
		::wsprintf(szName,TEXT("Driver%d_MinChChangeInterval"),i);
		Settings.Write(szName,(unsigned int)pSettings->GetMinChannelChangeInterval());
		::wsprintf(szName,TEXT("Driver%d_LastSpace"),i);
		Settings.Write(szName,pSettings->m_LastSpace);
		::wsprintf(szName,TEXT("Driver%d_LastChannel"),i);
		Settings.Write(szName,pSettings->m_LastChannel);
		::wsprintf(szName,TEXT("Driver%d_LastServiceID"),i);
		Settings.Write(szName,pSettings->m_LastServiceID);
		::wsprintf(szName,TEXT("Driver%d_LastTSID"),i);
		Settings.Write(szName,pSettings->m_LastTransportStreamID);
		::wsprintf(szName,TEXT("Driver%d_LastStatus"),i);
		Settings.Write(szName,pSettings->m_fLastAllChannels?0x01U:0x00U);
	}
	return true;
}


bool CDriverOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),
							  MAKEINTRESOURCE(IDD_OPTIONS_DRIVER));
}


bool CDriverOptions::Initialize(CDriverManager *pDriverManager)
{
	m_pDriverManager=pDriverManager;
	return true;
}


bool CDriverOptions::GetInitialChannel(LPCTSTR pszFileName,ChannelInfo *pChannelInfo) const
{
	if (pszFileName==NULL || pChannelInfo==NULL)
		return false;

	int Index=m_SettingList.Find(pszFileName);

	if (Index>=0) {
		const CDriverSettings *pSettings=m_SettingList.GetDriverSettings(Index);

		switch (pSettings->GetInitialChannelType()) {
		case CDriverSettings::INITIALCHANNEL_NONE:
			//pChannelInfo->Space=-1;
			pChannelInfo->Space=pSettings->m_LastSpace;
			pChannelInfo->Channel=-1;
			pChannelInfo->ServiceID=-1;
			pChannelInfo->TransportStreamID=-1;
			//pChannelInfo->fAllChannels=false;
			pChannelInfo->fAllChannels=pSettings->m_fLastAllChannels;
			return true;
		case CDriverSettings::INITIALCHANNEL_LAST:
			pChannelInfo->Space=pSettings->m_LastSpace;
			pChannelInfo->Channel=pSettings->m_LastChannel;
			pChannelInfo->ServiceID=pSettings->m_LastServiceID;
			pChannelInfo->TransportStreamID=pSettings->m_LastTransportStreamID;
			pChannelInfo->fAllChannels=pSettings->m_fLastAllChannels;
			return true;
		case CDriverSettings::INITIALCHANNEL_CUSTOM:
			pChannelInfo->Space=pSettings->GetInitialSpace();
			pChannelInfo->Channel=pSettings->GetInitialChannel();
			pChannelInfo->ServiceID=pSettings->GetInitialServiceID();
			pChannelInfo->TransportStreamID=-1;
			pChannelInfo->fAllChannels=pSettings->GetAllChannels();
			return true;
		}
	}
	return false;
}


bool CDriverOptions::SetLastChannel(LPCTSTR pszFileName,const ChannelInfo *pChannelInfo)
{
	if (IsStringEmpty(pszFileName) || pChannelInfo==NULL)
		return false;

	int Index=m_SettingList.Find(pszFileName);
	CDriverSettings *pSettings;

	if (Index<0) {
		pSettings=new CDriverSettings(pszFileName);
		m_SettingList.Add(pSettings);
	} else {
		pSettings=m_SettingList.GetDriverSettings(Index);
	}
	pSettings->m_LastSpace=pChannelInfo->Space;
	pSettings->m_LastChannel=pChannelInfo->Channel;
	pSettings->m_LastServiceID=pChannelInfo->ServiceID;
	pSettings->m_LastTransportStreamID=pChannelInfo->TransportStreamID;
	pSettings->m_fLastAllChannels=pChannelInfo->fAllChannels;
	return true;
}


bool CDriverOptions::IsNoSignalLevel(LPCTSTR pszFileName) const
{
	const CDriverSettings *pSettings=GetBonDriverSettings(pszFileName);
	if (pSettings==NULL)
		return false;
	return pSettings->GetNoSignalLevel();
}


bool CDriverOptions::IsResetChannelChangeErrorCount(LPCTSTR pszFileName) const
{
	const CDriverSettings *pSettings=GetBonDriverSettings(pszFileName);
	if (pSettings==NULL)
		return false;
	return pSettings->GetResetChannelChangeErrorCount();
}


bool CDriverOptions::GetBonDriverOptions(LPCTSTR pszFileName,BonDriverOptions *pOptions) const
{
	if (pOptions==NULL)
		return false;

	const CDriverSettings *pSettings=GetBonDriverSettings(pszFileName);
	if (pSettings==NULL)
		return false;

	*pOptions = pSettings->GetOptions();

	return true;
}


CDriverSettings *CDriverOptions::GetBonDriverSettings(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return NULL;

	int Index=m_SettingList.Find(pszFileName);
	if (Index<0)
		return NULL;

	return m_SettingList.GetDriverSettings(Index);
}


const CDriverSettings *CDriverOptions::GetBonDriverSettings(LPCTSTR pszFileName) const
{
	if (IsStringEmpty(pszFileName))
		return NULL;

	int Index=m_SettingList.Find(pszFileName);
	if (Index<0)
		return NULL;

	return m_SettingList.GetDriverSettings(Index);
}


void CDriverOptions::InitDlgItem(int Driver)
{
	EnableDlgItems(m_hDlg,IDC_DRIVEROPTIONS_FIRST,IDC_DRIVEROPTIONS_LAST,Driver>=0);
	DlgComboBox_Clear(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE);
	DlgComboBox_Clear(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);

	if (Driver>=0) {
		CDriverInfo *pDriverInfo=m_pDriverManager->GetDriverInfo(Driver);
		LPCTSTR pszFileName=pDriverInfo->GetFileName();
		CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(DlgComboBox_GetItemData(m_hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Driver));

		int InitChannelType=pSettings->GetInitialChannelType();
		::CheckRadioButton(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_NONE,
								  IDC_DRIVEROPTIONS_INITCHANNEL_CUSTOM,
						IDC_DRIVEROPTIONS_INITCHANNEL_NONE+InitChannelType);
		EnableDlgItems(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,
							  IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,
					InitChannelType==CDriverSettings::INITIALCHANNEL_CUSTOM);
		bool fCur=IsEqualFileName(pszFileName,
			::PathFindFileName(GetAppClass().CoreEngine.GetDriverFileName()));
		if (fCur || pDriverInfo->LoadTuningSpaceList(CDriverInfo::LOADTUNINGSPACE_USEDRIVER_NOOPEN)) {
			const CTuningSpaceList *pTuningSpaceList;
			int NumSpaces,i;

			if (fCur) {
				pTuningSpaceList=GetAppClass().ChannelManager.GetDriverTuningSpaceList();
				NumSpaces=pTuningSpaceList->NumSpaces();
			} else {
				pTuningSpaceList=pDriverInfo->GetTuningSpaceList();
				NumSpaces=pTuningSpaceList->NumSpaces();
				if (NumSpaces<pDriverInfo->GetDriverSpaceList()->NumSpaces())
					NumSpaces=pDriverInfo->GetDriverSpaceList()->NumSpaces();
			}
			DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,TEXT("���ׂ�"));
			for (i=0;i<NumSpaces;i++) {
				LPCTSTR pszName=pTuningSpaceList->GetTuningSpaceName(i);
				TCHAR szName[16];

				if (pszName==NULL && !fCur)
					pszName=pDriverInfo->GetDriverSpaceList()->GetTuningSpaceName(i);
				if (pszName==NULL) {
					::wsprintf(szName,TEXT("Space %d"),i+1);
					pszName=szName;
				}
				DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,pszName);
			}
			if (pSettings->GetAllChannels()) {
				DlgComboBox_SetCurSel(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,0);
			} else {
				i=pSettings->GetInitialSpace();
				if (i>=0)
					DlgComboBox_SetCurSel(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,i+1);
			}
			SetChannelList(Driver);
			int Sel=0;
			if (pSettings->GetInitialSpace()>=0
					&& pSettings->GetInitialChannel()>=0) {
				int Count=(int)DlgComboBox_GetCount(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);
				for (i=1;i<Count;i++) {
					const CChannelInfo *pChInfo=m_InitChannelList.GetChannelInfo(
						(int)DlgComboBox_GetItemData(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,i));
					if (pChInfo->GetSpace()==pSettings->GetInitialSpace()
							&& pChInfo->GetChannelIndex()==pSettings->GetInitialChannel()
							&& (pSettings->GetInitialServiceID()<0
								|| pChInfo->GetServiceID()==pSettings->GetInitialServiceID())) {
						Sel=i;
						break;
					}
				}
			}
			DlgComboBox_SetCurSel(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,Sel);
		}

		DlgCheckBox_Check(m_hDlg,IDC_DRIVEROPTIONS_NOSIGNALLEVEL,
						  pSettings->GetNoSignalLevel());
		DlgCheckBox_Check(m_hDlg,IDC_DRIVEROPTIONS_IGNOREINITIALSTREAM,
						  pSettings->GetIgnoreInitialStream());
		DlgCheckBox_Check(m_hDlg,IDC_DRIVEROPTIONS_PURGESTREAMONCHANNELCHANGE,
						  pSettings->GetPurgeStreamOnChannelChange());
		DlgCheckBox_Check(m_hDlg,IDC_DRIVEROPTIONS_RESETCHANNELCHANGEERRORCOUNT,
						  pSettings->GetResetChannelChangeErrorCount());
		DlgCheckBox_Check(m_hDlg,IDC_DRIVEROPTIONS_PUMPSTREAMSYNCPLAYBACK,
						  pSettings->GetPumpStreamSyncPlayback());

		::SetDlgItemInt(m_hDlg,IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY,
						pSettings->GetFirstChannelSetDelay(),FALSE);
		DlgUpDown_SetRange(m_hDlg,IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY_SPIN,
						   0,CBonSrcDecoder::FIRST_CHANNEL_SET_DELAY_MAX);
		TCHAR szText[64];
		::wsprintf(szText,TEXT("ms (0�`%u)"),CBonSrcDecoder::FIRST_CHANNEL_SET_DELAY_MAX);
		::SetDlgItemText(m_hDlg,IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY_UNIT,szText);

		::SetDlgItemInt(m_hDlg,IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL,
						pSettings->GetMinChannelChangeInterval(),FALSE);
		DlgUpDown_SetRange(m_hDlg,IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_SPIN,
						   0,CBonSrcDecoder::CHANNEL_CHANGE_INTERVAL_MAX);
		::wsprintf(szText,TEXT("ms (0�`%u)"),CBonSrcDecoder::CHANNEL_CHANGE_INTERVAL_MAX);
		::SetDlgItemText(m_hDlg,IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL_UNIT,szText);
	} else {
		::SetDlgItemText(m_hDlg,IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY,TEXT(""));
		::SetDlgItemText(m_hDlg,IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL,TEXT(""));
	}
}


void CDriverOptions::SetChannelList(int Driver)
{
	const CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(
		DlgComboBox_GetItemData(m_hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Driver));
	if (pSettings==NULL)
		return;
	bool fCur=IsEqualFileName(
		pSettings->GetFileName(),
		::PathFindFileName(GetAppClass().CoreEngine.GetDriverFileName()));
	const CDriverInfo *pDriverInfo;
	int i;

	if (!fCur) {
		pDriverInfo=m_pDriverManager->GetDriverInfo(Driver);
	}
	if (pSettings->GetAllChannels()) {
		DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,TEXT("�w��Ȃ�"));
		DlgComboBox_SetItemData(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,0,-1);
		for (i=0;;i++) {
			const CChannelList *pChannelList;

			if (fCur)
				pChannelList=GetAppClass().ChannelManager.GetFileChannelList(i);
			else
				pChannelList=pDriverInfo->GetChannelList(i);
			if (pChannelList==NULL)
				break;
			AddChannelList(pChannelList);
		}
	} else {
		i=pSettings->GetInitialSpace();
		if (i>=0) {
			const CChannelList *pChannelList;

			if (fCur)
				pChannelList=GetAppClass().ChannelManager.GetChannelList(i);
			else
				pChannelList=pDriverInfo->GetChannelList(i);
			if (pChannelList!=NULL) {
				DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,TEXT("�w��Ȃ�"));
				DlgComboBox_SetItemData(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,0,-1);
				AddChannelList(pChannelList);
			}
		}
	}
}


void CDriverOptions::AddChannelList(const CChannelList *pChannelList)
{
	m_InitChannelList.Clear();

	for (int i=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);

		if (!pChannelInfo->IsEnabled())
			continue;
		m_InitChannelList.AddChannel(*pChannelInfo);
		int Index=(int)DlgComboBox_AddString(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,pChannelInfo->GetName());
		DlgComboBox_SetItemData(m_hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,Index,m_InitChannelList.NumChannels()-1);
	}
}


CDriverSettings *CDriverOptions::GetCurSelDriverSettings() const
{
	int Sel=(int)DlgComboBox_GetCurSel(m_hDlg,IDC_DRIVEROPTIONS_DRIVERLIST);

	if (Sel<0)
		return NULL;
	return reinterpret_cast<CDriverSettings*>(
			DlgComboBox_GetItemData(m_hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Sel));
}


INT_PTR CDriverOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			if (m_pDriverManager!=NULL
					&& m_pDriverManager->NumDrivers()>0) {

				m_CurSettingList=m_SettingList;
				for (int i=0;i<m_pDriverManager->NumDrivers();i++) {
					LPCTSTR pszFileName=m_pDriverManager->GetDriverInfo(i)->GetFileName();
					CDriverSettings *pSettings;

					DlgComboBox_AddString(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,pszFileName);
					int Index=m_CurSettingList.Find(pszFileName);
					if (Index<0) {
						pSettings=new CDriverSettings(pszFileName);
						m_CurSettingList.Add(pSettings);
					} else {
						pSettings=m_CurSettingList.GetDriverSettings(Index);
					}
					DlgComboBox_SetItemData(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,i,(LPARAM)pSettings);
				}
				int CurDriver=-1;
				LPCTSTR pszCurDriverName=GetAppClass().CoreEngine.GetDriverFileName();
				if (pszCurDriverName[0]!='\0') {
					CurDriver=(int)DlgComboBox_FindStringExact(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,
						-1,::PathFindFileName(pszCurDriverName));
				}
				DlgComboBox_SetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,CurDriver);
				if (Util::OS::IsWindowsVistaOrLater()) {
					DlgComboBox_SetCueBanner(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,
											 TEXT("�ݒ肷�� BonDriver ��I�����Ă�������"));
				}
				InitDlgItem(CurDriver);
			} else {
				EnableDlgItems(hDlg,IDC_DRIVEROPTIONS_FIRST,IDC_DRIVEROPTIONS_LAST,false);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DRIVEROPTIONS_DRIVERLIST:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				InitDlgItem((int)DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST));
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_NONE:
		case IDC_DRIVEROPTIONS_INITCHANNEL_LAST:
		case IDC_DRIVEROPTIONS_INITCHANNEL_CUSTOM:
			{
				int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST);

				if (Sel>=0) {
					CDriverSettings *pSettings=reinterpret_cast<CDriverSettings*>(
						DlgComboBox_GetItemData(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST,Sel));

					pSettings->SetInitialChannelType(LOWORD(wParam)-IDC_DRIVEROPTIONS_INITCHANNEL_NONE);
					EnableDlgItems(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE,
						IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,
						pSettings->GetInitialChannelType()==CDriverSettings::INITIALCHANNEL_CUSTOM);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_SPACE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				DlgComboBox_Clear(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);
				if (pSettings!=NULL) {
					int Space=(int)DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_SPACE)-1;

					pSettings->SetInitialSpace(Space);
					pSettings->SetAllChannels(Space<0);
					pSettings->SetInitialChannel(-1);
					pSettings->SetInitialServiceID(-1);
					SetChannelList((int)DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_DRIVERLIST));
					DlgComboBox_SetCurSel(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,0);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL);
					int Channel=-1,ServiceID=-1;

					if (Sel>0) {
						const CChannelInfo *pChInfo=m_InitChannelList.GetChannelInfo(
							(int)DlgComboBox_GetItemData(hDlg,IDC_DRIVEROPTIONS_INITCHANNEL_CHANNEL,Sel));
						if (pChInfo!=NULL) {
							pSettings->SetInitialSpace(pChInfo->GetSpace());
							Channel=pChInfo->GetChannelIndex();
							ServiceID=pChInfo->GetServiceID();
						}
					}
					pSettings->SetInitialChannel(Channel);
					pSettings->SetInitialServiceID(ServiceID);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_NOSIGNALLEVEL:
			{
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					pSettings->SetNoSignalLevel(DlgCheckBox_IsChecked(hDlg,IDC_DRIVEROPTIONS_NOSIGNALLEVEL));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_IGNOREINITIALSTREAM:
			{
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					pSettings->SetIgnoreInitialStream(DlgCheckBox_IsChecked(hDlg,IDC_DRIVEROPTIONS_IGNOREINITIALSTREAM));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_PURGESTREAMONCHANNELCHANGE:
			{
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					pSettings->SetPurgeStreamOnChannelChange(DlgCheckBox_IsChecked(hDlg,IDC_DRIVEROPTIONS_PURGESTREAMONCHANNELCHANGE));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_RESETCHANNELCHANGEERRORCOUNT:
			{
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					pSettings->SetResetChannelChangeErrorCount(DlgCheckBox_IsChecked(hDlg,IDC_DRIVEROPTIONS_RESETCHANNELCHANGEERRORCOUNT));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_PUMPSTREAMSYNCPLAYBACK:
			{
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					pSettings->SetPumpStreamSyncPlayback(
						DlgCheckBox_IsChecked(hDlg,IDC_DRIVEROPTIONS_PUMPSTREAMSYNCPLAYBACK));
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY:
			if (HIWORD(wParam)==EN_CHANGE) {
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					DWORD Delay=::GetDlgItemInt(hDlg,IDC_DRIVEROPTIONS_FIRSTCHANNELSETDELAY,NULL,FALSE);
					if (Delay<=CBonSrcDecoder::FIRST_CHANNEL_SET_DELAY_MAX)
						pSettings->SetFirstChannelSetDelay(Delay);
				}
			}
			return TRUE;

		case IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL:
			if (HIWORD(wParam)==EN_CHANGE) {
				CDriverSettings *pSettings=GetCurSelDriverSettings();

				if (pSettings!=NULL) {
					DWORD Interval=::GetDlgItemInt(hDlg,IDC_DRIVEROPTIONS_MINCHANNELCHANGEINTERVAL,NULL,FALSE);
					if (Interval<=CBonSrcDecoder::CHANNEL_CHANGE_INTERVAL_MAX)
						pSettings->SetMinChannelChangeInterval(Interval);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			m_SettingList=m_CurSettingList;
			GetAppClass().Core.ApplyBonDriverOptions();
			m_fChanged=true;
			break;
		}
		break;

	case WM_DESTROY:
		m_CurSettingList.Clear();
		return TRUE;
	}

	return FALSE;
}




CDriverOptions::BonDriverOptions::BonDriverOptions()
	: fNoSignalLevel(false)
	, fIgnoreInitialStream(true)
	, fPurgeStreamOnChannelChange(true)
	, fResetChannelChangeErrorCount(true)
	, fPumpStreamSyncPlayback(false)
	, FirstChannelSetDelay(0)
	, MinChannelChangeInterval(0)
{
}


CDriverOptions::BonDriverOptions::BonDriverOptions(LPCTSTR pszBonDriverName)
{
	*this=BonDriverOptions();

	CDriverManager::TunerSpec Spec;
	if (GetAppClass().DriverManager.GetTunerSpec(pszBonDriverName,&Spec)) {
		if ((Spec.Flags &
				(CDriverManager::TunerSpec::FLAG_NETWORK |
				 CDriverManager::TunerSpec::FLAG_FILE))!=0)
			fNoSignalLevel=true;
		if ((Spec.Flags &
				(CDriverManager::TunerSpec::FLAG_NETWORK |
				 CDriverManager::TunerSpec::FLAG_FILE |
				 CDriverManager::TunerSpec::FLAG_VOLATILE))!=0)
			fIgnoreInitialStream=false;
		if ((Spec.Flags & CDriverManager::TunerSpec::FLAG_FILE)!=0)
			fPumpStreamSyncPlayback=true;
	}
}
