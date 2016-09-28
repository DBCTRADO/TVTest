#ifndef TVTEST_VARIABLE_STRING_H
#define TVTEST_VARIABLE_STRING_H


#include "ChannelList.h"
#include "EpgProgramList.h"
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
		virtual bool GetString(LPCWSTR pszKeyword,String *pString) = 0;
		virtual bool NormalizeString(String *pString) const { return false; }
		virtual bool GetParameterList(ParameterGroupList *pList) const = 0;
		bool InputParameter(HWND hDlg,int EditID,const POINT &MenuPos);

	protected:
		bool GetTimeString(LPCWSTR pszKeyword,const SYSTEMTIME &Time,String *pString) const;
		bool IsDateTimeParameter(LPCTSTR pszKeyword);
	};

	bool FormatVariableString(CVariableStringMap *pVariableMap,LPCWSTR pszFormat,String *pString);

	class CBasicVariableStringMap : public CVariableStringMap
	{
	public:
		bool GetString(LPCWSTR pszKeyword,String *pString) override;
		bool GetParameterList(ParameterGroupList *pList) const override;

	protected:
		virtual bool GetLocalString(LPCWSTR pszKeyword,String *pString) = 0;
		bool GetPreferredGlobalString(LPCWSTR pszKeyword,String *pString);
		bool GetGlobalString(LPCWSTR pszKeyword,String *pString);
	};

	class CEventVariableStringMap : public CBasicVariableStringMap
	{
	public:
		enum {
			FLAG_NO_NORMALIZE    = 0x0001U,
			FLAG_NO_CURRENT_TIME = 0x0002U,
			FLAG_NO_TOT_TIME     = 0x0004U,
			FLAG_NO_SEPARATOR    = 0x0008U
		};

		struct EventInfo
		{
			CTunerChannelInfo Channel;
			CEventInfo Event;
			String ServiceName;
			SYSTEMTIME TotTime;
		};

		CEventVariableStringMap();
		CEventVariableStringMap(const EventInfo &Info);
		bool BeginFormat() override;
		bool NormalizeString(String *pString) const override;
		bool GetParameterList(ParameterGroupList *pList) const override;
		void SetCurrentTime(const SYSTEMTIME *pTime);
		void SetSampleEventInfo();

		static bool GetSampleEventInfo(EventInfo *pInfo);

	protected:
		bool GetLocalString(LPCWSTR pszKeyword,String *pString) override;

		static void GetEventTitle(const String &EventName,String *pTitle);
		static void GetEventMark(const String &EventName,String *pMarks);

		unsigned int m_Flags;
		EventInfo m_EventInfo;
		bool m_fCurrentTimeSet;
		SYSTEMTIME m_CurrentTime;
	};

}


#endif
