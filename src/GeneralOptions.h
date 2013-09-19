#ifndef GENERAL_OPTIONS_H
#define GENERAL_OPTIONS_H


#include "Options.h"
#include "CoreEngine.h"
#include "DirectShowFilter/VideoRenderer.h"


class CGeneralOptions : public COptions
{
public:
	enum DefaultDriverType {
		DEFAULT_DRIVER_NONE,
		DEFAULT_DRIVER_LAST,
		DEFAULT_DRIVER_CUSTOM
	};
	enum {
		MAX_MPEG2_DECODER_NAME=128
	};

	CGeneralOptions();
	~CGeneralOptions();

// COptions
	bool Apply(DWORD Flags) override;

// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

// CBasicDialog
	bool Create(HWND hwndOwner) override;

// CGeneralOptions
	DefaultDriverType GetDefaultDriverType() const;
	LPCTSTR GetDefaultDriverName() const;
	bool SetDefaultDriverName(LPCTSTR pszDriverName);
	bool GetFirstDriverName(LPTSTR pszDriverName) const;
	LPCTSTR GetMpeg2DecoderName() const;
	bool SetMpeg2DecoderName(LPCTSTR pszDecoderName);
	CVideoRenderer::RendererType GetVideoRendererType() const;
	bool SetVideoRendererType(CVideoRenderer::RendererType Renderer);
	int GetCasDevice(bool fUseName);
	bool SetCasDevice(int Device);
	void SetTemporaryNoDescramble(bool fNoDescramble);
	bool GetResident() const;
	bool GetKeepSingleTask() const;
	int GetDescrambleInstruction() const { return m_DescrambleInstruction; }
	bool GetDescrambleCurServiceOnly() const;
	bool GetEnableEmmProcess() const;

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void DescrambleBenchmarkTest(HWND hwndOwner);

	enum {
		UPDATE_DECODER				= 0x00000001UL,
		UPDATE_RENDERER				= 0x00000002UL,
		UPDATE_CARDREADER			= 0x00000004UL,
		UPDATE_RESIDENT				= 0x00000008UL,
		UPDATE_DESCRAMBLECURONLY	= 0x00000010UL,
		UPDATE_ENABLEEMMPROCESS		= 0x00000020UL
	};

	TVTest::String m_BonDriverDirectory;
	DefaultDriverType m_DefaultDriverType;
	TVTest::String m_DefaultBonDriverName;
	TVTest::String m_LastBonDriverName;
	TVTest::String m_Mpeg2DecoderName;
	CVideoRenderer::RendererType m_VideoRendererType;
	TVTest::String m_CasDeviceName;
	int m_CasDevice;
	bool m_fTemporaryNoDescramble;
	bool m_fResident;
	bool m_fKeepSingleTask;
	int m_DescrambleInstruction;
	bool m_fDescrambleCurServiceOnly;
	bool m_fEnableEmmProcess;
};


#endif
