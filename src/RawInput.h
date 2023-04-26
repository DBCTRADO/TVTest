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


#ifndef TVTEST_RAW_INPUT_H
#define TVTEST_RAW_INPUT_H


namespace TVTest
{

	class CRawInput
	{
	public:
		class CEventHandler
		{
		public:
			virtual ~CEventHandler() = default;

			virtual void OnInput(int Type) = 0;
			virtual void OnUnknownInput(const BYTE *pData, int Size) {}
		};

	protected:
		CEventHandler *m_pEventHandler = nullptr;

	public:
		bool Initialize(HWND hwnd);
		LRESULT OnInput(HWND hwnd, WPARAM wParam, LPARAM lParam);
		void SetEventHandler(CEventHandler *pHandler);
		CEventHandler *GetEventHandler() const { return m_pEventHandler; }
		int NumKeyTypes() const;
		LPCTSTR GetKeyText(int Key) const;
		int GetKeyData(int Key) const;
		int KeyDataToIndex(int Data) const;
	};

}	// namespace TVTest


#endif
