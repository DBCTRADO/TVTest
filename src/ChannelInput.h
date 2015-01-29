#ifndef TVTEST_CHANNEL_INPUT_H
#define TVTEST_CHANNEL_INPUT_H


#include "Dialog.h"


namespace TVTest
{

	class CChannelInputOptions
	{
	public:
		enum KeyInputModeType {
			KEYINPUTMODE_DISABLED,
			KEYINPUTMODE_SINGLEKEY,
			KEYINPUTMODE_MULTIPLEKEYS
		};
		static const KeyInputModeType KEYINPUTMODE_FIRST = KEYINPUTMODE_DISABLED;
		static const KeyInputModeType KEYINPUTMODE_LAST  = KEYINPUTMODE_MULTIPLEKEYS;

		enum KeyType {
			KEY_DIGIT,
			KEY_NUMPAD,
			KEY_FUNCTION
		};
		static const KeyType KEY_FIRST = KEY_DIGIT;
		static const KeyType KEY_LAST  = KEY_FUNCTION;

		KeyInputModeType KeyInputMode[KEY_LAST+1];
		unsigned int KeyTimeout;
		bool fKeyTimeoutCancel;

		CChannelInputOptions();
	};

	class CChannelInput
	{
	public:
		enum KeyDownResult {
			KEYDOWN_NOTPROCESSED,
			KEYDOWN_BEGIN,
			KEYDOWN_COMPLETED,
			KEYDOWN_CANCELLED,
			KEYDOWN_CONTINUE
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
		bool m_fInputting;
		int m_MaxDigits;
		int m_CurDigits;
		int m_Number;
	};

	class CChannelInputOptionsDialog : public CBasicDialog
	{
	public:
		CChannelInputOptionsDialog(CChannelInputOptions &Options);

	// CBasicDialog
		bool Show(HWND hwndOwner) override;

	private:
	// CBasicDialog
		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

		CChannelInputOptions &m_Options;
	};

}	// namespace TVTest

#endif
