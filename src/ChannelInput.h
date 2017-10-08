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
		unsigned int KeyTimeout;
		bool fKeyTimeoutCancel;

		CChannelInputOptions();
	};

	class CChannelInput
	{
	public:
		enum KeyDownResult {
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
		bool m_fInputting;
		int m_MaxDigits;
		int m_CurDigits;
		int m_Number;
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

}	// namespace TVTest

#endif
