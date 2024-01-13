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


#ifndef TVTEST_CHANNEL_INPUT_H
#define TVTEST_CHANNEL_INPUT_H


#include "Dialog.h"


namespace TVTest
{

	class CChannelInputOptions
	{
	public:
		enum class KeyInputModeType {
			Disabled,
			SingleKey,
			MultipleKeys,
			TVTEST_ENUM_CLASS_TRAILER
		};

		enum class KeyType {
			Digit,
			NumPad,
			Function,
			TVTEST_ENUM_CLASS_TRAILER
		};

		KeyInputModeType KeyInputMode[static_cast<size_t>(KeyType::Trailer_)];
		unsigned int KeyTimeout = 2000;
		bool fKeyTimeoutCancel = false;

		CChannelInputOptions();
	};

	class CChannelInput
	{
	public:
		enum class KeyDownResult {
			NotProcessed,
			Begin,
			Completed,
			Cancelled,
			Continue,
		};

		CChannelInput(const CChannelInputOptions &Options);

		bool BeginInput(int MaxDigits);
		void EndInput();
		bool IsInputting() const { return m_fInputting; }
		int GetMaxDigits() const { return m_MaxDigits; }
		int GetCurDigits() const { return m_CurDigits; }
		int GetNumber() const { return m_Number; }
		KeyDownResult OnKeyDown(WPARAM wParam);
		bool IsKeyNeeded(WPARAM wParam) const;

	private:
		const CChannelInputOptions &m_Options;
		bool m_fInputting = false;
		int m_MaxDigits = 0;
		int m_CurDigits = 0;
		int m_Number = 0;
	};

	class CChannelInputOptionsDialog
		: public CBasicDialog
	{
	public:
		CChannelInputOptionsDialog(CChannelInputOptions &Options);

	// CBasicDialog
		bool Show(HWND hwndOwner) override;

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

		CChannelInputOptions &m_Options;
	};

} // namespace TVTest

#endif
