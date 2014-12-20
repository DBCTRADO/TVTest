#ifndef TVTEST_AUDIO_OPTIONS_H
#define TVTEST_AUDIO_OPTIONS_H


#include "Options.h"
#include "BonTsEngine/MediaViewer.h"


class CAudioOptions : public COptions
{
public:
	CAudioOptions();
	~CAudioOptions();

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CAudioOptions
	bool ApplyMediaViewerOptions();
	LPCTSTR GetAudioDeviceName() const { return TVTest::StringUtility::GetCStrOrNull(m_AudioDeviceName); }
	LPCTSTR GetAudioFilterName() const { return TVTest::StringUtility::GetCStrOrNull(m_AudioFilterName); }
	const CAudioDecFilter::SpdifOptions &GetSpdifOptions() const { return m_SpdifOptions; }
	bool SetSpdifOptions(const CAudioDecFilter::SpdifOptions &Options);
	bool GetDownMixSurround() const { return m_fDownMixSurround; }

private:
	class CSurroundOptionsDialog : public CBasicDialog
	{
	public:
		CSurroundOptionsDialog(CAudioOptions *pOptions);
		bool Show(HWND hwndOwner) override;

	private:
		CAudioOptions *m_pOptions;

		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		void SetDownMixMatrix(const CAudioDecFilter::DownMixMatrix &Matrix);
	};

	static const CAudioDecFilter::SurroundMixingMatrix m_DefaultSurroundMixingMatrix;
	static const CAudioDecFilter::DownMixMatrix m_DefaultDownMixMatrix;

	TVTest::String m_AudioDeviceName;
	TVTest::String m_AudioFilterName;

	CAudioDecFilter::SpdifOptions m_SpdifOptions;
	bool m_fDownMixSurround;
	bool m_fUseCustomSurroundMixingMatrix;
	CAudioDecFilter::SurroundMixingMatrix m_SurroundMixingMatrix;
	bool m_fUseCustomDownMixMatrix;
	CAudioDecFilter::DownMixMatrix m_DownMixMatrix;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
