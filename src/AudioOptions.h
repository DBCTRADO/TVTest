#ifndef TVTEST_AUDIO_OPTIONS_H
#define TVTEST_AUDIO_OPTIONS_H


#include "Options.h"
#include "LibISDB/LibISDB/Windows/Viewer/ViewerFilter.hpp"
#include <vector>


class CAudioOptions : public COptions
{
public:
	struct AudioLanguageInfo
	{
		DWORD Language;
		bool fSub;

		bool operator==(const AudioLanguageInfo &Op) const {
			return Language==Op.Language && fSub==Op.fSub;
		}
		bool operator!=(const AudioLanguageInfo &Op) const { return !(*this==Op); }
	};

	typedef std::vector<AudioLanguageInfo> AudioLanguageList;

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
	const LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions &GetSpdifOptions() const { return m_SPDIFOptions; }
	bool SetSpdifOptions(const LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions &Options);
	bool GetDownMixSurround() const { return m_fDownMixSurround; }
	bool GetEnableLanguagePriority() const { return m_fEnableLanguagePriority; }
	const AudioLanguageList &GetLanguagePriority() const { return m_LanguagePriority; }
	bool GetResetAudioDelayOnChannelChange() const { return m_fResetAudioDelayOnChannelChange; }

private:
	class CSurroundOptionsDialog : public CBasicDialog
	{
	public:
		CSurroundOptionsDialog(CAudioOptions *pOptions);
		bool Show(HWND hwndOwner) override;

	private:
		CAudioOptions *m_pOptions;

		INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
		void SetDownMixMatrix(const LibISDB::DirectShow::AudioDecoderFilter::DownMixMatrix &Matrix);
	};

	static const LibISDB::DirectShow::AudioDecoderFilter::SurroundMixingMatrix m_DefaultSurroundMixingMatrix;
	static const LibISDB::DirectShow::AudioDecoderFilter::DownMixMatrix m_DefaultDownMixMatrix;
	static const DWORD m_AudioLanguageList[];
	static const DWORD LANGUAGE_FLAG_SUB=0x01000000;

	TVTest::String m_AudioDeviceName;
	TVTest::String m_AudioFilterName;

	LibISDB::DirectShow::AudioDecoderFilter::SPDIFOptions m_SPDIFOptions;
	bool m_fDownMixSurround;
	bool m_fUseCustomSurroundMixingMatrix;
	LibISDB::DirectShow::AudioDecoderFilter::SurroundMixingMatrix m_SurroundMixingMatrix;
	bool m_fUseCustomDownMixMatrix;
	LibISDB::DirectShow::AudioDecoderFilter::DownMixMatrix m_DownMixMatrix;
	bool m_fEnableLanguagePriority;
	AudioLanguageList m_LanguagePriority;
	bool m_fResetAudioDelayOnChannelChange;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void GetLanguageText(DWORD Language,bool fSub,LPTSTR pszText,int MaxText) const;
	void UpdateLanguagePriorityControls();
};


#endif
