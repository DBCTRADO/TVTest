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


#ifndef TVTEST_COMMAND_H
#define TVTEST_COMMAND_H


#include <functional>
#include <vector>
#include <map>
#include <unordered_map>
#include <array>


namespace TVTest
{

	class CCommandManager
	{
	public:
		static constexpr size_t MAX_COMMAND_ID_TEXT = MAX_PATH;
		static constexpr size_t MAX_COMMAND_TEXT    = MAX_PATH + 16;

		enum class CommandState : std::uint8_t {
			None     = 0x00_u8,
			Disabled = 0x01_u8,
			Checked  = 0x02_u8,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class InvokeFlag : unsigned int {
			None  = 0x0000U,
			Mouse = 0x0001U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		struct InvokeParameters
		{
			int ID;
			InvokeFlag Flags;
		};

		typedef std::function<bool(InvokeParameters &Params)> CommandHandler;
		template<typename T> static CommandHandler BindHandler(
			bool (T::*pFunc)(InvokeParameters &Params), T *pObject)
		{
			return std::bind(pFunc, pObject, std::placeholders::_1);
		}

		class CCommandLister
		{
		public:
			CCommandLister(const CCommandManager &Manager);

			int Next();
			void Reset();

		private:
			const CCommandManager &m_Manager;
			size_t m_Index = 0;
			int m_ID = 0;
		};

		class ABSTRACT_CLASS(CEventListener)
		{
		public:
			virtual ~CEventListener() = default;
			virtual void OnCommandStateChanged(int ID, CommandState OldState, CommandState NewState) {}
			virtual void OnCommandRadioCheckedStateChanged(int FirstID, int LastID, int CheckedID) {}
		};

		class ABSTRACT_CLASS(CCommandCustomizer)
		{
		protected:
			int m_FirstID;
			int m_LastID;

		public:
			CCommandCustomizer(int FirstID, int LastID)
				: m_FirstID(FirstID), m_LastID(LastID) {}
			virtual ~CCommandCustomizer() = default;
			virtual bool IsCommandValid(int Command) { return Command >= m_FirstID && Command <= m_LastID; }
			virtual bool GetCommandText(int Command, LPTSTR pszText, size_t MaxLength) { return false; }
		};

		CCommandManager();

		bool RegisterCommand(
			int FirstID, int LastID, int LastListingID,
			LPCTSTR pszIDText,
			const CommandHandler &Handler,
			LPCTSTR pszText = nullptr, LPCTSTR pszShortText = nullptr,
			CommandState State = CommandState::None);
		bool RegisterCommand(
			int ID, LPCTSTR pszIDText,
			const CommandHandler &Handler,
			LPCTSTR pszText = nullptr, LPCTSTR pszShortText = nullptr,
			CommandState State = CommandState::None)
		{
			return RegisterCommand(ID, ID, ID, pszIDText, Handler, pszText, pszShortText, State);
		}
		bool InvokeCommand(int ID, InvokeFlag Flags = InvokeFlag::None) const;

		bool IsCommandValid(int ID) const;
		String GetCommandIDText(int ID) const;
		size_t GetCommandText(int ID, LPTSTR pszText, size_t MaxLength) const;
		size_t GetCommandShortText(int ID, LPTSTR pszText, size_t MaxLength) const;
		int ParseIDText(const String &Text) const;
		int ParseIDText(LPCTSTR pszText) const;

		bool SetCommandState(int ID, CommandState State);
		bool SetCommandState(int ID, CommandState Mask, CommandState State);
		CommandState GetCommandState(int ID) const;
		bool SetCommandRadioCheckedState(int FirstID, int LastID, int CheckedID);

		bool AddCommandCustomizer(CCommandCustomizer *pCustomizer);
		bool AddEventListener(CEventListener *pEventListener);
		bool RemoveEventListener(CEventListener *pEventListener);

	private:
		int IDToIndex(int ID) const;
		String GetNumberedText(const String &Text, int Number) const;

		struct CommandInfo
		{
			int FirstID;
			int LastID;
			int LastListingID;
			String IDText;
			String Text;
			String ShortText;
			CommandHandler Handler;
		};

		std::vector<CommandInfo> m_CommandList;
		std::map<int, size_t> m_CommandIDMap;
		std::unordered_map<String, int,
			StringFunctional::HashNoCase,
			StringFunctional::EqualNoCase> m_CommandTextMap;
		std::vector<CommandState> m_CommandStateList;
		std::vector<CCommandCustomizer*> m_CustomizerList;
		std::vector<CEventListener*> m_EventListenerList;
	};

} // namespace TVTest


#endif
