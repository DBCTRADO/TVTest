#ifndef TVTEST_VARIABLE_STRING_H
#define TVTEST_VARIABLE_STRING_H


#include "ChannelList.h"
#include "EpgProgramList.h"


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

		virtual bool BeginFormat() { return true; }
		virtual void EndFormat() {}
		virtual bool GetString(LPCWSTR pszKeyword,String *pString) = 0;
		virtual bool NormalizeString(String *pString) const { return false; }
		virtual bool GetParameterInfo(int Index,ParameterInfo *pInfo) const = 0;
		virtual int GetParameterCount() const = 0;
		bool InputParameter(HWND hDlg,int EditID,const POINT &MenuPos);

	protected:
		bool GetTimeString(LPCWSTR pszKeyword,const SYSTEMTIME &Time,String *pString) const;
		bool IsDateTimeParameter(LPCTSTR pszKeyword);
	};

	bool FormatVariableString(CVariableStringMap *pVariableMap,LPCWSTR pszFormat,String *pString);

	class CEventVariableStringMap : public CVariableStringMap
	{
	public:
		struct EventInfo
		{
			CChannelInfo Channel;
			CEventInfo Event;
			String ServiceName;
			SYSTEMTIME TotTime;
		};

		CEventVariableStringMap();
		CEventVariableStringMap(const EventInfo &Info);
		bool BeginFormat() override;
		bool GetString(LPCWSTR pszKeyword,String *pString) override;
		bool NormalizeString(String *pString) const override;
		bool GetParameterInfo(int Index,ParameterInfo *pInfo) const override;
		int GetParameterCount() const override;
		void SetCurrentTime(const SYSTEMTIME *pTime);
		void SetSampleEventInfo();

		static bool GetSampleEventInfo(EventInfo *pInfo);

	protected:
		static void GetEventTitle(const String &EventName,String *pTitle);

		static const ParameterInfo m_ParameterList[];

		EventInfo m_EventInfo;
		bool m_fCurrentTimeSet;
		SYSTEMTIME m_CurrentTime;
	};

}


#endif
