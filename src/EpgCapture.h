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
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class BeginStatus : unsigned int {
			None               = 0x0000U,
			Standby            = 0x0001U,
			TunerAlreadyOpened = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER
		};

		enum class EndFlag : unsigned int  {
			None       = 0x0000U,
			CloseTuner = 0x0001U,
			Resume     = 0x0002U,
			TVTEST_ENUM_FLAGS_TRAILER,
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

		bool BeginCapture(
			LPCTSTR pszTuner = nullptr,
			const CChannelList *pChannelList = nullptr,
			BeginFlag Flags = BeginFlag::None);
		void EndCapture(EndFlag Flags = EndFlag::Default);
		bool IsCapturing() const { return m_fCapturing; }
		bool ProcessCapture();
		void SetEventHandler(CEventHandler *pEventHandler);
		int GetChannelCount() const { return static_cast<int>(m_ChannelList.size()); }
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

		bool m_fCapturing = false;
		int m_CurChannel = -1;
		std::vector<ChannelGroup> m_ChannelList;
		Util::CClock m_AccumulateClock;
		bool m_fChannelChanging = false;
		CEventHandler *m_pEventHandler = nullptr;
	};

}	// namespace TVTest


#endif
