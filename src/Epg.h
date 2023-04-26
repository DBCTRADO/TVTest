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


#ifndef TVTEST_EPG_H
#define TVTEST_EPG_H


#include "ProgramGuide.h"
#include "Favorites.h"


namespace TVTest
{

	class CEpg
	{
	public:
		class CChannelProviderManager
			: public CProgramGuideChannelProviderManager
		{
		public:
		// CProgramGuideChannelProviderManager
			size_t GetChannelProviderCount() const override;
			CProgramGuideChannelProvider *GetChannelProvider(size_t Index) const override;

		// CChannelProviderManager
			bool Create(LPCTSTR pszDefaultTuner = nullptr);
			void Clear();
			int GetCurChannelProvider() const { return m_CurChannelProvider; }

		private:
			class CBonDriverChannelProvider
				: public CProgramGuideBaseChannelProvider
			{
			public:
				CBonDriverChannelProvider(LPCTSTR pszFileName);
				bool Update() override;
			};

			class CFavoritesChannelProvider
				: public CProgramGuideBaseChannelProvider
			{
			public:
				bool Update() override;
				bool GetName(LPTSTR pszName, int MaxName) const override;
				bool GetGroupID(size_t Group, String *pID) const override;
				int ParseGroupID(LPCTSTR pszID) const override;
				bool GetBonDriver(LPTSTR pszFileName, int MaxLength) const override;
				bool GetBonDriverFileName(size_t Group, size_t Channel, LPTSTR pszFileName, int MaxLength) const override;

			private:
				struct GroupInfo
				{
					String Name;
					String ID;
					std::vector<CFavoriteChannel> ChannelList;
				};

				std::vector<std::unique_ptr<GroupInfo>> m_GroupList;

				void ClearGroupList();
				void AddFavoritesChannels(const CFavoriteFolder &Folder, const String &Path);
				void AddSubItems(GroupInfo *pGroup, const CFavoriteFolder &Folder);
			};

			std::vector<std::unique_ptr<CProgramGuideChannelProvider>> m_ChannelProviderList;
			int m_CurChannelProvider = -1;
		};

		CProgramGuide ProgramGuide;
		CProgramGuideFrameSettings ProgramGuideFrameSettings;
		CProgramGuideFrame ProgramGuideFrame;
		CProgramGuideDisplay ProgramGuideDisplay;
		bool fShowProgramGuide = false;

		CEpg(LibISDB::EPGDatabase &EPGDatabase, CEventSearchOptions &EventSearchOptions);
		CChannelProviderManager *CreateChannelProviderManager(LPCTSTR pszDefaultTuner = nullptr);

	private:
		class CProgramGuideEventHandler
			: public CProgramGuide::CEventHandler
		{
			bool OnClose() override;
			void OnDestroy() override;
			void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName, const LibISDB::EPGDatabase::ServiceInfo *pServiceInfo) override;
			bool OnKeyDown(UINT KeyCode, UINT Flags) override;
			bool OnMenuInitialize(HMENU hmenu, UINT CommandBase) override;
			bool OnMenuSelected(UINT Command) override;

			int FindChannel(const CChannelList *pChannelList, const LibISDB::EPGDatabase::ServiceInfo *pServiceInfo);
		};

		class CProgramGuideDisplayEventHandler
			: public CProgramGuideDisplay::CProgramGuideDisplayEventHandler
			, protected CDisplayEventHandlerBase
		{
		// CProgramGuideDisplay::CProgramGuideDisplayEventHandler
			bool OnHide() override;
			bool SetAlwaysOnTop(bool fTop) override;
			bool GetAlwaysOnTop() const override;
			void OnMouseMessage(UINT Msg, int x, int y) override;
		};

		class CProgramGuideProgramCustomizer
			: public CProgramGuide::CProgramCustomizer
		{
			bool Initialize() override;
			void Finalize() override;
			bool DrawBackground(
				const LibISDB::EventInfo &Event, HDC hdc,
				const RECT &ItemRect, const RECT &TitleRect, const RECT &ContentRect,
				COLORREF BackgroundColor) override;
			bool InitializeMenu(
				const LibISDB::EventInfo &Event, HMENU hmenu, UINT CommandBase,
				const POINT &CursorPos, const RECT &ItemRect) override;
			bool ProcessMenu(const LibISDB::EventInfo &Event, UINT Command) override;
			bool OnLButtonDoubleClick(
				const LibISDB::EventInfo &Event,
				const POINT &CursorPos, const RECT &ItemRect) override;
		};

		CProgramGuideEventHandler m_ProgramGuideEventHandler;
		CProgramGuideDisplayEventHandler m_ProgramGuideDisplayEventHandler;
		CProgramGuideProgramCustomizer m_ProgramCustomizer;
		CChannelProviderManager m_ChannelProviderManager;
	};

}	// namespace TVTest


#endif
