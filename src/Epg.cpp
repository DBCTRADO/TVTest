#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "resource.h"
#include "Common/DebugDef.h"


namespace TVTest
{


CEpg::CEpg(CEpgProgramList &EpgProgramList,CEventSearchOptions &EventSearchOptions)
	: ProgramGuide(EventSearchOptions)
	, ProgramGuideFrame(&ProgramGuide,&ProgramGuideFrameSettings)
	, ProgramGuideDisplay(&ProgramGuide,&ProgramGuideFrameSettings)
	, fShowProgramGuide(false)
{
	ProgramGuide.SetEpgProgramList(&EpgProgramList);
	ProgramGuide.SetEventHandler(&m_ProgramGuideEventHandler);
	ProgramGuide.SetProgramCustomizer(&m_ProgramCustomizer);
	ProgramGuide.SetChannelProviderManager(&m_ChannelProviderManager);

	ProgramGuideDisplay.SetEventHandler(&m_ProgramGuideDisplayEventHandler);
}


CEpg::CChannelProviderManager *CEpg::CreateChannelProviderManager(LPCTSTR pszDefaultTuner)
{
	m_ChannelProviderManager.Create(pszDefaultTuner);
	return &m_ChannelProviderManager;
}


CEpg::CChannelProviderManager::CChannelProviderManager()
	: m_CurChannelProvider(-1)
{
}


CEpg::CChannelProviderManager::~CChannelProviderManager()
{
	Clear();
}


size_t CEpg::CChannelProviderManager::GetChannelProviderCount() const
{
	return m_ChannelProviderList.size();
}


CProgramGuideChannelProvider *CEpg::CChannelProviderManager::GetChannelProvider(size_t Index) const
{
	if (Index>=m_ChannelProviderList.size())
		return NULL;
	return m_ChannelProviderList[Index];
}


bool CEpg::CChannelProviderManager::Create(LPCTSTR pszDefaultTuner)
{
	Clear();

	m_ChannelProviderList.push_back(new CFavoritesChannelProvider);
	if (pszDefaultTuner!=NULL && ::lstrcmpi(pszDefaultTuner,TEXT("favorites"))==0)
		m_CurChannelProvider=0;

	CAppMain &App=GetAppClass();

	CProgramGuideBaseChannelProvider *pCurChannelProvider=NULL;
	String DefaultTuner;
	if (m_CurChannelProvider<0) {
		if (!IsStringEmpty(pszDefaultTuner)) {
			pCurChannelProvider=new CBonDriverChannelProvider(pszDefaultTuner);
			DefaultTuner=pszDefaultTuner;
		} else if (pszDefaultTuner==NULL
				&& App.CoreEngine.IsDriverSpecified()
				&& !App.CoreEngine.IsNetworkDriver()) {
			DefaultTuner=App.CoreEngine.GetDriverFileName();
			pCurChannelProvider=new CProgramGuideBaseChannelProvider(
				App.ChannelManager.GetTuningSpaceList(),
				DefaultTuner.c_str());
		}
	}

	for (int i=0;i<App.DriverManager.NumDrivers();i++) {
		const CDriverInfo *pDriverInfo=App.DriverManager.GetDriverInfo(i);

		if (pDriverInfo!=NULL) {
			if (pCurChannelProvider!=NULL
					&& IsEqualFileName(DefaultTuner.c_str(),pDriverInfo->GetFileName())) {
				m_CurChannelProvider=(int)m_ChannelProviderList.size();
				m_ChannelProviderList.push_back(pCurChannelProvider);
				pCurChannelProvider=NULL;
			} else {
				CDriverManager::TunerSpec Spec;
				if (!App.DriverManager.GetTunerSpec(pDriverInfo->GetFileName(),&Spec)
						|| (Spec.Flags &
							(CDriverManager::TunerSpec::FLAG_NETWORK |
							 CDriverManager::TunerSpec::FLAG_FILE))==0) {
					CBonDriverChannelProvider *pDriverChannelProvider=
						new CBonDriverChannelProvider(pDriverInfo->GetFileName());

					m_ChannelProviderList.push_back(pDriverChannelProvider);
				}
			}
		}
	}

	if (pCurChannelProvider!=NULL) {
		auto itr=m_ChannelProviderList.begin();
		++itr;
		m_ChannelProviderList.insert(itr,pCurChannelProvider);
		m_CurChannelProvider=1;
	}

	return true;
}


void CEpg::CChannelProviderManager::Clear()
{
	for (size_t i=0;i<m_ChannelProviderList.size();i++)
		delete m_ChannelProviderList[i];
	m_ChannelProviderList.clear();
	m_CurChannelProvider=-1;
}


CEpg::CChannelProviderManager::CBonDriverChannelProvider::CBonDriverChannelProvider(LPCTSTR pszFileName)
	: CProgramGuideBaseChannelProvider(NULL,pszFileName)
{
}


bool CEpg::CChannelProviderManager::CBonDriverChannelProvider::Update()
{
	CDriverInfo DriverInfo(m_BonDriverFileName.c_str());

	if (!DriverInfo.LoadTuningSpaceList())
		return false;

	m_TuningSpaceList=*DriverInfo.GetTuningSpaceList();

	return true;
}


CEpg::CChannelProviderManager::CFavoritesChannelProvider::~CFavoritesChannelProvider()
{
	ClearGroupList();
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::Update()
{
	ClearGroupList();
	AddFavoritesChannels(GetAppClass().FavoritesManager.GetRootFolder(),String());
	m_GroupList.front()->Name=TEXT("���C�ɓ���");

	const int NumSpaces=static_cast<int>(m_GroupList.size());
	m_TuningSpaceList.Clear();
	m_TuningSpaceList.Reserve(NumSpaces);
	for (int i=0;i<NumSpaces;i++) {
		const GroupInfo *pGroup=m_GroupList[i];
		CTuningSpaceInfo *pTuningSpace=m_TuningSpaceList.GetTuningSpaceInfo(i);
		pTuningSpace->SetName(pGroup->Name.c_str());
		CChannelList *pChannelList=pTuningSpace->GetChannelList();
		for (auto itr=pGroup->ChannelList.begin();itr!=pGroup->ChannelList.end();++itr) {
			pChannelList->AddChannel(itr->GetChannelInfo());
		}
	}

	return true;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetName(
	LPTSTR pszName,int MaxName) const
{
	if (pszName==NULL || MaxName<1)
		return false;

	::lstrcpyn(pszName,TEXT("���C�ɓ���`�����l��"),MaxName);

	return true;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetGroupID(
	size_t Group,String *pID) const
{
	if (Group>=m_GroupList.size() || pID==NULL)
		return false;
	*pID=m_GroupList[Group]->ID;
	return true;
}


int CEpg::CChannelProviderManager::CFavoritesChannelProvider::ParseGroupID(
	LPCTSTR pszID) const
{
	if (IsStringEmpty(pszID))
		return -1;

	for (size_t i=0;i<m_GroupList.size();i++) {
		if (m_GroupList[i]->ID.compare(pszID)==0)
			return static_cast<int>(i);
	}

	// �ȑO�̃o�[�W�����Ƃ̌݊��p
	if (::lstrcmp(pszID,TEXT("0"))==0)
		return 0;

	return -1;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetBonDriver(
	LPTSTR pszFileName,int MaxLength) const
{
	if (pszFileName==NULL || MaxLength<1
			|| m_GroupList.empty()
			|| m_GroupList.front()->ChannelList.empty())
		return false;

	LPCTSTR pszBonDriver=m_GroupList.front()->ChannelList.front().GetBonDriverFileName();
	if (IsStringEmpty(pszBonDriver))
		return false;

	for (size_t i=0;i<m_GroupList.size();i++) {
		const GroupInfo *pGroup=m_GroupList[i];
		for (auto itr=pGroup->ChannelList.begin();
				itr!=pGroup->ChannelList.end();++itr) {
			if (!IsEqualFileName(itr->GetBonDriverFileName(),pszBonDriver))
				return false;
		}
	}

	::lstrcpyn(pszFileName,pszBonDriver,MaxLength);

	return false;
}


bool CEpg::CChannelProviderManager::CFavoritesChannelProvider::GetBonDriverFileName(
	size_t Group,size_t Channel,LPTSTR pszFileName,int MaxLength) const
{
	if (Group>=m_GroupList.size()
			|| Channel>=m_GroupList[Group]->ChannelList.size()
			|| pszFileName==NULL || MaxLength<1)
		return false;

	CAppMain &App=GetAppClass();
	const CFavoriteChannel &FavoriteChannel=m_GroupList[Group]->ChannelList[Channel];
	const CChannelInfo &ChannelInfo=FavoriteChannel.GetChannelInfo();

	if (!FavoriteChannel.GetForceBonDriverChange()
			&& App.CoreEngine.IsTunerOpen()) {
		int Space=App.ChannelManager.GetCurrentSpace();
		if (Space!=CChannelManager::SPACE_INVALID) {
			int Index=App.ChannelManager.FindChannelByIDs(Space,
				ChannelInfo.GetNetworkID(),
				ChannelInfo.GetTransportStreamID(),
				ChannelInfo.GetServiceID());
			if (Index<0 && Space!=CChannelManager::SPACE_ALL) {
				for (Space=0;Space<App.ChannelManager.NumSpaces();Space++) {
					Index=App.ChannelManager.FindChannelByIDs(Space,
						ChannelInfo.GetNetworkID(),
						ChannelInfo.GetTransportStreamID(),
						ChannelInfo.GetServiceID());
					if (Index>=0)
						break;
				}
			}
			if (Index>=0) {
				pszFileName[0]=_T('\0');
				return true;
			}
		}
	}

	::lstrcpyn(pszFileName,FavoriteChannel.GetBonDriverFileName(),MaxLength);

	return true;
}


void CEpg::CChannelProviderManager::CFavoritesChannelProvider::ClearGroupList()
{
	if (!m_GroupList.empty()) {
		for (auto itr=m_GroupList.begin();itr!=m_GroupList.end();++itr)
			delete *itr;
		m_GroupList.clear();
	}
}


void CEpg::CChannelProviderManager::CFavoritesChannelProvider::AddFavoritesChannels(
	const CFavoriteFolder &Folder,const String &Path)
{
	GroupInfo *pGroup=new GroupInfo;
	pGroup->Name=Folder.GetName();
	if (Path.empty())
		pGroup->ID=TEXT("\\");
	else
		pGroup->ID=Path;
	m_GroupList.push_back(pGroup);

	for (size_t i=0;i<Folder.GetItemCount();i++) {
		const CFavoriteItem *pItem=Folder.GetItem(i);

		if (pItem->GetType()==CFavoriteItem::ITEM_FOLDER) {
			const CFavoriteFolder *pFolder=static_cast<const CFavoriteFolder*>(pItem);
			String FolderPath,Name;
			StringUtility::Encode(pItem->GetName(),&Name);
			FolderPath=Path;
			FolderPath+=_T('\\');
			FolderPath+=Name;
			AddSubItems(pGroup,*pFolder);
			AddFavoritesChannels(*pFolder,FolderPath);
		} else if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL) {
			const CFavoriteChannel *pChannel=static_cast<const CFavoriteChannel*>(pItem);
			pGroup->ChannelList.push_back(*pChannel);
		}
	}
}


void CEpg::CChannelProviderManager::CFavoritesChannelProvider::AddSubItems(
	GroupInfo *pGroup,const CFavoriteFolder &Folder)
{
	for (size_t i=0;i<Folder.GetItemCount();i++) {
		const CFavoriteItem *pItem=Folder.GetItem(i);

		if (pItem->GetType()==CFavoriteItem::ITEM_FOLDER) {
			AddSubItems(pGroup,*static_cast<const CFavoriteFolder*>(pItem));
		} else if (pItem->GetType()==CFavoriteItem::ITEM_CHANNEL) {
			pGroup->ChannelList.push_back(*static_cast<const CFavoriteChannel*>(pItem));
		}
	}
}


bool CEpg::CProgramGuideEventHandler::OnClose()
{
	CAppMain &App=GetAppClass();

	App.Epg.fShowProgramGuide=false;
	App.UICore.SetCommandCheckedState(CM_PROGRAMGUIDE,false);
	return true;
}


void CEpg::CProgramGuideEventHandler::OnDestroy()
{
	m_pProgramGuide->Clear();

	CAppMain &App=GetAppClass();

	if (App.UICore.GetStandby()
			&& App.UICore.GetTransientStandby()
			&& !App.RecordManager.IsRecording()
			&& !App.RecordManager.IsReserved())
		App.UICore.DoCommandAsync(CM_EXIT);
}


int CEpg::CProgramGuideEventHandler::FindChannel(const CChannelList *pChannelList,const CServiceInfoData *pServiceInfo)
{
	for (int i=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);

		if (pChannelInfo->GetTransportStreamID()==pServiceInfo->m_TSID
				&& pChannelInfo->GetServiceID()==pServiceInfo->m_ServiceID
				&& pChannelInfo->IsEnabled())
			return i;
	}
	return -1;
}


void CEpg::CProgramGuideEventHandler::OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo)
{
	CAppMain &App=GetAppClass();

	if (!App.UICore.ConfirmChannelChange())
		return;

	const bool fSetBonDriver=!IsStringEmpty(pszDriverFileName);
	CMainWindow::ResumeInfo &ResumeInfo=App.MainWindow.GetResumeInfo();
	ResumeInfo.fSetChannel=false;
	ResumeInfo.fOpenTuner=!fSetBonDriver;

	App.UICore.DoCommand(CM_SHOW);

	if (fSetBonDriver) {
		if (!App.Core.OpenTuner(pszDriverFileName))
			return;
	}

	const CChannelList *pChannelList=App.ChannelManager.GetCurrentChannelList();
	if (pChannelList!=NULL) {
		int Index=FindChannel(pChannelList,pServiceInfo);
		if (Index>=0) {
			App.Core.SetChannel(App.ChannelManager.GetCurrentSpace(),Index);
			return;
		}
	}
	for (int i=0;i<App.ChannelManager.NumSpaces();i++) {
		pChannelList=App.ChannelManager.GetChannelList(i);
		if (pChannelList!=NULL) {
			int Index=FindChannel(pChannelList,pServiceInfo);
			if (Index>=0) {
				App.Core.SetChannel(i,Index);
				return;
			}
		}
	}
}


bool CEpg::CProgramGuideEventHandler::OnKeyDown(UINT KeyCode,UINT Flags)
{
	GetAppClass().MainWindow.SendMessage(WM_KEYDOWN,KeyCode,Flags);
	return true;
}


bool CEpg::CProgramGuideEventHandler::OnMenuInitialize(HMENU hmenu,UINT CommandBase)
{
	return GetAppClass().PluginManager.SendProgramGuideInitializeMenuEvent(hmenu,&CommandBase);
}


bool CEpg::CProgramGuideEventHandler::OnMenuSelected(UINT Command)
{
	return GetAppClass().PluginManager.SendProgramGuideMenuSelectedEvent(Command);
}


bool CEpg::CProgramGuideDisplayEventHandler::OnHide()
{
	CAppMain &App=GetAppClass();

	m_pProgramGuideDisplay->Destroy();
	App.Epg.fShowProgramGuide=false;
	App.UICore.SetCommandCheckedState(CM_PROGRAMGUIDE,App.Epg.fShowProgramGuide);
	return true;
}


bool CEpg::CProgramGuideDisplayEventHandler::SetAlwaysOnTop(bool fTop)
{
	return GetAppClass().UICore.SetAlwaysOnTop(fTop);
}


bool CEpg::CProgramGuideDisplayEventHandler::GetAlwaysOnTop() const
{
	return GetAppClass().UICore.GetAlwaysOnTop();
}


void CEpg::CProgramGuideDisplayEventHandler::OnMouseMessage(UINT Msg,int x,int y)
{
	RelayMouseMessage(m_pProgramGuideDisplay,Msg,x,y);
}


bool CEpg::CProgramGuideProgramCustomizer::Initialize()
{
	return GetAppClass().PluginManager.SendProgramGuideInitializeEvent(m_pProgramGuide->GetHandle());
}


void CEpg::CProgramGuideProgramCustomizer::Finalize()
{
	GetAppClass().PluginManager.SendProgramGuideFinalizeEvent(m_pProgramGuide->GetHandle());
}


bool CEpg::CProgramGuideProgramCustomizer::DrawBackground(
	const CEventInfoData &Event,HDC hdc,
	const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,
	COLORREF BackgroundColor)
{
	return GetAppClass().PluginManager.SendProgramGuideProgramDrawBackgroundEvent(
		Event,hdc,ItemRect,TitleRect,ContentRect,BackgroundColor);
}


bool CEpg::CProgramGuideProgramCustomizer::InitializeMenu(
	const CEventInfoData &Event,HMENU hmenu,UINT CommandBase,
	const POINT &CursorPos,const RECT &ItemRect)
{
	return GetAppClass().PluginManager.SendProgramGuideProgramInitializeMenuEvent(
		Event,hmenu,&CommandBase,CursorPos,ItemRect);
}


bool CEpg::CProgramGuideProgramCustomizer::ProcessMenu(
	const CEventInfoData &Event,UINT Command)
{
	return GetAppClass().PluginManager.SendProgramGuideProgramMenuSelectedEvent(Event,Command);
}


bool CEpg::CProgramGuideProgramCustomizer::OnLButtonDoubleClick(
	const CEventInfoData &Event,const POINT &CursorPos,const RECT &ItemRect)
{
	CAppMain &App=GetAppClass();

	LPCTSTR pszCommand=App.ProgramGuideOptions.GetProgramLDoubleClickCommand();
	if (IsStringEmpty(pszCommand))
		return false;
	int Command=App.ProgramGuideOptions.ParseCommand(pszCommand);
	if (Command>0) {
		m_pProgramGuide->SendMessage(WM_COMMAND,Command,0);
		return true;
	}
	return App.PluginManager.OnProgramGuideCommand(pszCommand,
		PROGRAMGUIDE_COMMAND_ACTION_MOUSE,&Event,&CursorPos,&ItemRect);
}


}	// namespace TVTest
