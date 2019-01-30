/*
  TVTest
  Copyright(c) 2008-2019 DBCTRADO

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


#ifndef TVTEST_WINDOW_UTIL_H
#define TVTEST_WINDOW_UTIL_H


namespace TVTest
{

	void SnapWindow(HWND hwnd, RECT *prc, int Margin, HWND hwndExclude = nullptr);
	bool IsMessageInQueue(HWND hwnd, UINT Message);


	class CMouseLeaveTrack
	{
	public:
		CMouseLeaveTrack();

		CMouseLeaveTrack(const CMouseLeaveTrack &) = delete;
		CMouseLeaveTrack &operator=(const CMouseLeaveTrack &) = delete;

		void Initialize(HWND hwnd);
		bool OnMouseMove();
		bool OnMouseLeave();
		bool OnNcMouseMove();
		bool OnNcMouseLeave();
		bool OnMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
		bool IsClientTrack() const { return m_fClientTrack; }
		bool IsNonClientTrack() const { return m_fNonClientTrack; }

	protected:
		HWND m_hwnd;
		bool m_fClientTrack;
		bool m_fNonClientTrack;

		bool IsCursorInWindow() const;
	};

	class CMouseWheelHandler
	{
	public:
		CMouseWheelHandler();

		void Reset();
		void ResetDelta();
		int OnWheel(int Delta);
		int OnMouseWheel(WPARAM wParam, int ScrollLines = 0);
		int OnMouseHWheel(WPARAM wParam, int ScrollChars = 0);
		int GetDeltaSum() const { return m_DeltaSum; }
		int GetLastDelta() const { return m_LastDelta; }
		DWORD GetLastTime() const { return m_LastTime; }
		int GetDefaultScrollLines() const;
		int GetDefaultScrollChars() const;

	protected:
		int m_DeltaSum;
		int m_LastDelta;
		DWORD m_LastTime;
	};

	class CWindowTimerManager
	{
	public:
		CWindowTimerManager();

		CWindowTimerManager(const CWindowTimerManager &) = delete;
		CWindowTimerManager &operator=(const CWindowTimerManager &) = delete;

		void InitializeTimer(HWND hwnd);
		bool BeginTimer(unsigned int ID, DWORD Interval);
		void EndTimer(unsigned int ID);
		void EndAllTimers();
		bool IsTimerEnabled(unsigned int ID) const;

	protected:
		HWND m_hwndTimer;
		unsigned int m_TimerIDs;
	};

	class CWindowSubclass
	{
	public:
		CWindowSubclass();
		virtual ~CWindowSubclass();

		CWindowSubclass(const CWindowSubclass &) = delete;
		CWindowSubclass &operator=(const CWindowSubclass &) = delete;

		bool SetSubclass(HWND hwnd);
		void RemoveSubclass();

	protected:
		HWND m_hwnd;

		static LRESULT CALLBACK SubclassProc(
			HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
			UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
		virtual LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		virtual void OnSubclassRemoved() {}
	};

}	// namespace TVTest


#endif
