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


#ifndef TVTEST_VARIABLE_STRING_H
#define TVTEST_VARIABLE_STRING_H


#include "ChannelList.h"
#include "LibISDB/LibISDB/EPG/EventInfo.hpp"
#include <vector>


namespace TVTest
{

	class CVariableStringMap
	{
	public:
		struct ParameterInfo
		{
			LPCWSTR pszParameter;
			LPCWSTR pszText;
		};

		struct ParameterGroup
		{
			String Text;
			std::vector<ParameterInfo> ParameterList;
		};

		typedef std::vector<ParameterGroup> ParameterGroupList;

		virtual bool BeginFormat() { return true; }
		virtual void EndFormat() {}
		virtual bool GetString(LPCWSTR pszKeyword, String *pString) = 0;
		virtual bool NormalizeString(String *pString) const { return false; }
		virtual bool GetParameterList(ParameterGroupList *pList) const = 0;
		bool InputParameter(HWND hDlg, int EditID, const POINT &MenuPos);

	protected:
		bool GetTimeString(LPCWSTR pszKeyword, const LibISDB::DateTime &Time, String *pString) const;
		bool IsDateTimeParameter(LPCTSTR pszKeyword);
	};

	bool FormatVariableString(CVariableStringMap *pVariableMap, LPCWSTR pszFormat, String *pString);

	class CBasicVariableStringMap
		: public CVariableStringMap
	{
	public:
		bool GetString(LPCWSTR pszKeyword, String *pString) override;
		bool GetParameterList(ParameterGroupList *pList) const override;

	protected:
		virtual bool GetLocalString(LPCWSTR pszKeyword, String *pString) = 0;
		bool GetPreferredGlobalString(LPCWSTR pszKeyword, String *pString);
		bool GetGlobalString(LPCWSTR pszKeyword, String *pString);
	};

	class CEventVariableStringMap
		: public CBasicVariableStringMap
	{
	public:
		enum class Flag : unsigned int {
			None          = 0x0000U,
			NoNormalize   = 0x0001U,
			NoCurrentTime = 0x0002U,
			NoTOTTime     = 0x0004U,
			NoSeparator   = 0x0008U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		struct EventInfo
		{
			CTunerChannelInfo Channel;
			LibISDB::EventInfo Event;
			String ServiceName;
			LibISDB::DateTime TOTTime;
		};

		CEventVariableStringMap() = default;
		CEventVariableStringMap(const EventInfo &Info);

		bool BeginFormat() override;
		bool NormalizeString(String *pString) const override;
		bool GetParameterList(ParameterGroupList *pList) const override;
		void SetCurrentTime(const LibISDB::DateTime *pTime);
		void SetSampleEventInfo();

		static bool GetSampleEventInfo(EventInfo *pInfo);

	protected:
		bool GetLocalString(LPCWSTR pszKeyword, String *pString) override;

		static void GetEventTitle(const String &EventName, String *pTitle);
		static void GetEventMark(const String &EventName, String *pMarks);

		Flag m_Flags = Flag::None;
		EventInfo m_EventInfo;
		bool m_fCurrentTimeSet = false;
		LibISDB::DateTime m_CurrentTime;
	};

}


#endif
