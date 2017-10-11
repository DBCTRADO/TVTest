#ifndef TVTEST_EPG_CAPTURE_H
#define TVTEST_EPG_CAPTURE_H


#include <vector>
#include "ChannelList.h"


namespace TVTest
{

	class CEpgCaptureManager
	{
	public:
		enum class BeginFlag : unsigned int {
			None    = 0x0000U,
			NoUI    = 0x0001U,
			Standby = 0x0002U,
		};

		enum class BeginStatus : unsigned int {
			None               = 0x0000U,
			Standby            = 0x0001U,
			TunerAlreadyOpened = 0x0002U,
		};

		enum class EndFlag : unsigned int  {
			None       = 0x0000U,
			CloseTuner = 0x0001U,
			Resume     = 0x0002U,
			Default    = CloseTuner | Resume,
		};

		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual void OnBeginCapture(BeginFlag Flags, BeginStatus Status) {}
			virtual void OnEndCapture(EndFlag Flags) {}
			virtual void OnChannelChanged() {}
			virtual void OnChannelEnd(bool fComplete) {}
		};

		CEpgCaptureManager();
		~CEpgCaptureManager();

		bool BeginCapture(
			LPCTSTR pszTuner = nullptr,
			const CChannelList *pChannelList = nullptr,
			BeginFlag Flags = BeginFlag::None);
		void EndCapture(EndFlag Flags = EndFlag::Default);
		bool IsCapturing() const { return m_fCapturing; }
		bool ProcessCapture();
		void SetEventHandler(CEventHandler *pEventHandler);
		int GetChannelCount() const { return (int)m_ChannelList.size(); }
		int GetCurChannel() const { return m_CurChannel; }
		DWORD GetRemainingTime() const;
		bool IsChannelChanging() const { return m_fChannelChanging; }

	private:
		bool NextChannel();

		struct ChannelGroup
		{
			int Space;
			int Channel;
			DWORD Time;
			CChannelList ChannelList;
		};

		bool m_fCapturing;
		int m_CurChannel;
		std::vector<ChannelGroup> m_ChannelList;
		Util::CClock m_AccumulateClock;
		bool m_fChannelChanging;
		CEventHandler *m_pEventHandler;
	};

	TVTEST_ENUM_FLAGS(CEpgCaptureManager::BeginFlag)
	TVTEST_ENUM_FLAGS(CEpgCaptureManager::BeginStatus)
	TVTEST_ENUM_FLAGS(CEpgCaptureManager::EndFlag)

}	// namespace TVTest


#endif
