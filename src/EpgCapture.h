#ifndef TVTEST_EPG_CAPTURE_H
#define TVTEST_EPG_CAPTURE_H


#include <vector>
#include "ChannelList.h"


namespace TVTest
{

	class CEpgCaptureManager
	{
	public:
		enum {
			BEGIN_NO_UI     = 0x0001U,
			BEGIN_STANDBY   = 0x0002U
		};
		enum {
			BEGIN_STATUS_STANDBY              = 0x0001U,
			BEGIN_STATUS_TUNER_ALREADY_OPENED = 0x0002U
		};
		enum {
			END_CLOSE_TUNER = 0x0001U,
			END_RESUME      = 0x0002U,
			END_DEFAULT     = END_CLOSE_TUNER | END_RESUME
		};

		class ABSTRACT_CLASS(CEventHandler)
		{
		public:
			virtual void OnBeginCapture(unsigned int Flags,unsigned int Status) {}
			virtual void OnEndCapture(unsigned int Flags) {}
			virtual void OnChannelChanged() {}
			virtual void OnChannelEnd(bool fComplete) {}
		};

		CEpgCaptureManager();
		~CEpgCaptureManager();
		bool BeginCapture(LPCTSTR pszTuner=nullptr,
						  const CChannelList *pChannelList=nullptr,
						  unsigned int Flags=0);
		void EndCapture(unsigned int Flags=END_DEFAULT);
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

}	// namespace TVTest


#endif
