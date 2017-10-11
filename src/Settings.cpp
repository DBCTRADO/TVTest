#include "stdafx.h"
#include "TVTest.h"
#include "Settings.h"
#include <utility>
#include "Common/DebugDef.h"

using namespace TVTest;


static unsigned int StrToUInt(LPCTSTR pszValue)
{
	return (unsigned int)_tcstoul(pszValue, nullptr, 0);
}


CSettings::CSettings()
	: m_OpenFlags(OpenFlag::None)
{
}


CSettings::~CSettings()
{
	Close();
}


bool CSettings::Open(LPCTSTR pszFileName, OpenFlag Flags)
{
	Close();

	if (IsStringEmpty(pszFileName) || ::lstrlen(pszFileName) >= MAX_PATH
			|| !(Flags & (OpenFlag::Read | OpenFlag::Write))
			|| (Flags & (OpenFlag::Write | OpenFlag::WriteVolatile)) == (OpenFlag::Write | OpenFlag::WriteVolatile))
		return false;

	UINT IniFlags = 0;
	if (!!(Flags & OpenFlag::Read))
		IniFlags |= CIniFile::OPEN_READ;
	if (!!(Flags & OpenFlag::Write))
		IniFlags |= CIniFile::OPEN_WRITE;
	if (!!(Flags & OpenFlag::WriteVolatile))
		IniFlags |= CIniFile::OPEN_WRITE_VOLATILE;
	if (!m_IniFile.Open(pszFileName, IniFlags))
		return false;

	m_OpenFlags = Flags;
	if (!!(Flags & OpenFlag::WriteVolatile))
		m_OpenFlags |= OpenFlag::Write;

	return true;
}


void CSettings::Close()
{
	m_IniFile.Close();
	m_OpenFlags = OpenFlag::None;
}


bool CSettings::IsOpened() const
{
	return !!m_OpenFlags;
}


bool CSettings::Clear()
{
	if (!(m_OpenFlags & OpenFlag::Write))
		return false;

	return m_IniFile.ClearSection();
}


bool CSettings::SetSection(LPCTSTR pszSection)
{
	if (!m_OpenFlags)
		return false;

	return m_IniFile.SelectSection(pszSection);
}


bool CSettings::IsSectionExists(LPCTSTR pszSection)
{
	return m_IniFile.IsSectionExists(pszSection);
}


bool CSettings::GetEntries(EntryList *pEntries)
{
	return m_IniFile.GetEntries(pEntries);
}


bool CSettings::IsValueExists(LPCTSTR pszValueName)
{
	return m_IniFile.IsValueExists(pszValueName);
}


bool CSettings::DeleteValue(LPCTSTR pszValueName)
{
	if (!(m_OpenFlags & OpenFlag::Write))
		return false;

	return m_IniFile.DeleteValue(pszValueName);
}


bool CSettings::Read(LPCTSTR pszValueName, int *pData)
{
	TCHAR szValue[16];

	if (!Read(pszValueName, szValue, lengthof(szValue)) || szValue[0] == _T('\0'))
		return false;
	*pData = ::StrToInt(szValue);
	return true;
}


bool CSettings::Write(LPCTSTR pszValueName, int Data)
{
	TCHAR szValue[16];

	wsprintf(szValue, TEXT("%d"), Data);
	return Write(pszValueName, szValue);
}


bool CSettings::Read(LPCTSTR pszValueName, unsigned int *pData)
{
	TCHAR szValue[16];

	if (!Read(pszValueName, szValue, 16) || szValue[0] == _T('\0'))
		return false;
	*pData = StrToUInt(szValue);
	return true;
}


bool CSettings::Write(LPCTSTR pszValueName, unsigned int Data)
{
	TCHAR szValue[16];

	wsprintf(szValue, TEXT("%u"), Data);
	return Write(pszValueName, szValue);
}


bool CSettings::Read(LPCTSTR pszValueName, LPTSTR pszData, unsigned int Max)
{
	if (!(m_OpenFlags & OpenFlag::Read))
		return false;

	if (pszData == nullptr)
		return false;

	String Value;
	if (!m_IniFile.GetValue(pszValueName, &Value))
		return false;

	::lstrcpyn(pszData, Value.c_str(), Max);

	return true;
}


bool CSettings::Write(LPCTSTR pszValueName, LPCTSTR pszData)
{
	if (!(m_OpenFlags & OpenFlag::Write))
		return false;

	if (pszData == nullptr)
		return false;

	// 文字列が ' か " で囲まれていると読み込み時に除去されてしまうので、
	// 余分に " で囲っておく。
	if (pszData[0] == _T('"') || pszData[0] == _T('\'')) {
		String Buff;
		Buff = TEXT("\"");
		Buff += pszData;
		Buff += TEXT("\"");
		return m_IniFile.SetValue(pszValueName, Buff.c_str());
	}

	return m_IniFile.SetValue(pszValueName, pszData);
}


bool CSettings::Read(LPCTSTR pszValueName, TVTest::String *pValue)
{
	if (!(m_OpenFlags & OpenFlag::Read))
		return false;

	if (pValue == nullptr)
		return false;

	TVTest::String Value;
	if (!m_IniFile.GetValue(pszValueName, &Value))
		return false;

	*pValue = std::move(Value);

	return true;
}


bool CSettings::Write(LPCTSTR pszValueName, const TVTest::String &Value)
{
	return Write(pszValueName, Value.c_str());
}


bool CSettings::Read(LPCTSTR pszValueName, bool *pfData)
{
	TCHAR szData[8];

	if (!Read(pszValueName, szData, lengthof(szData)))
		return false;
	if (lstrcmpi(szData, TEXT("yes")) == 0 || lstrcmpi(szData, TEXT("true")) == 0)
		*pfData = true;
	else if (lstrcmpi(szData, TEXT("no")) == 0 || lstrcmpi(szData, TEXT("false")) == 0)
		*pfData = false;
	else
		return false;
	return true;
}


bool CSettings::Write(LPCTSTR pszValueName, bool fData)
{
	// よく考えたら否定文もあるので yes/no は変だが…
	// (その昔、iniファイルを直接編集して設定するようにしていた頃の名残)
	return Write(pszValueName, fData ? TEXT("yes") : TEXT("no"));
}


bool CSettings::Read(LPCTSTR pszValueName, double *pData)
{
	if (pData == nullptr)
		return false;

	TVTest::String Value;

	if (!Read(pszValueName, &Value))
		return false;

	*pData = std::_tcstod(Value.c_str(), nullptr);

	return true;
}


bool CSettings::Write(LPCTSTR pszValueName, double Data, int Digits)
{
	TCHAR szText[64];

	StdUtil::snprintf(szText, lengthof(szText), TEXT("%.*f"), Digits, Data);
	return Write(pszValueName, szText);
}


bool CSettings::Read(LPCTSTR pszValueName, float *pData)
{
	if (pData == nullptr)
		return false;

	double Value;

	if (!Read(pszValueName, &Value))
		return false;

	*pData = static_cast<float>(Value);

	return true;
}


bool CSettings::ReadColor(LPCTSTR pszValueName, COLORREF *pcrData)
{
	TCHAR szText[8];

	if (!Read(pszValueName, szText, lengthof(szText)) || szText[0] != _T('#') || lstrlen(szText) < 7)
		return false;
	*pcrData = RGB(
		HexStringToUInt(&szText[1], 2),
		HexStringToUInt(&szText[3], 2),
		HexStringToUInt(&szText[5], 2));
	return true;
}


bool CSettings::WriteColor(LPCTSTR pszValueName, COLORREF crData)
{
	TCHAR szText[8];

	wsprintf(
		szText, TEXT("#%02x%02x%02x"),
		GetRValue(crData), GetGValue(crData), GetBValue(crData));
	return Write(pszValueName, szText);
}


#define FONT_FLAG_ITALIC    0x0001U
#define FONT_FLAG_UNDERLINE 0x0002U
#define FONT_FLAG_STRIKEOUT 0x0004U

bool CSettings::Read(LPCTSTR pszValueName, LOGFONT *pFont)
{
	TCHAR szData[LF_FACESIZE + 32];

	if (!Read(pszValueName, szData, sizeof(szData) / sizeof(TCHAR)) || szData[0] == '\0')
		return false;

	LPTSTR p = szData, q;
	for (int i = 0; *p != _T('\0'); i++) {
		while (*p == _T(' '))
			p++;
		q = p;
		while (*p != _T('\0') && *p != _T(','))
			p++;
		if (*p != _T('\0'))
			*p++ = _T('\0');
		if (*q != _T('\0')) {
			switch (i) {
			case 0:
				::lstrcpyn(pFont->lfFaceName, q, LF_FACESIZE);
				pFont->lfWidth = 0;
				pFont->lfEscapement = 0;
				pFont->lfOrientation = 0;
				pFont->lfWeight = FW_NORMAL;
				pFont->lfItalic = 0;
				pFont->lfUnderline = 0;
				pFont->lfStrikeOut = 0;
				pFont->lfCharSet = DEFAULT_CHARSET;
				pFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
				pFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
				pFont->lfQuality = DRAFT_QUALITY;
				pFont->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
				break;
			case 1:
				pFont->lfHeight = ::StrToInt(q);
				break;
			case 2:
				pFont->lfWeight = ::StrToInt(q);
				break;
			case 3:
				{
					unsigned int Flags = StrToUInt(q);
					pFont->lfItalic = (Flags & FONT_FLAG_ITALIC) != 0;
					pFont->lfUnderline = (Flags & FONT_FLAG_UNDERLINE) != 0;
					pFont->lfStrikeOut = (Flags & FONT_FLAG_STRIKEOUT) != 0;
				}
				break;
			}
		} else if (i == 0) {
			return false;
		}
	}
	return true;
}


bool CSettings::Write(LPCTSTR pszValueName, const LOGFONT *pFont)
{
	TCHAR szData[LF_FACESIZE + 32];
	unsigned int Flags = 0;

	if (pFont->lfItalic)
		Flags |= FONT_FLAG_ITALIC;
	if (pFont->lfUnderline)
		Flags |= FONT_FLAG_UNDERLINE;
	if (pFont->lfStrikeOut)
		Flags |= FONT_FLAG_STRIKEOUT;
	::wsprintf(
		szData, TEXT("%s,%d,%d,%u"),
		pFont->lfFaceName, pFont->lfHeight, pFont->lfWeight, Flags);
	return Write(pszValueName, szData);
}




CSettingsBase::CSettingsBase()
	: m_pszSection(TEXT("Settings"))
	, m_fChanged(false)
{
}


CSettingsBase::CSettingsBase(LPCTSTR pszSection)
	: m_pszSection(pszSection)
	, m_fChanged(false)
{
}


bool CSettingsBase::LoadSettings(CSettings &Settings)
{
	if (!Settings.SetSection(m_pszSection))
		return false;
	return ReadSettings(Settings);
}


bool CSettingsBase::SaveSettings(CSettings &Settings)
{
	if (!Settings.SetSection(m_pszSection))
		return false;
	return WriteSettings(Settings);
}


bool CSettingsBase::LoadSettings(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Read))
		return false;
	return LoadSettings(Settings);
}


bool CSettingsBase::SaveSettings(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (!Settings.Open(pszFileName, CSettings::OpenFlag::Write))
		return false;
	return SaveSettings(Settings);
}
