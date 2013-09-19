/*
	字幕データ解析クラス
	ほとんど最低限の機能しか無い。
*/
#ifndef CAPTION_PARSER_H
#define CAPTION_PARSER_H


#include <vector>
#include "TsMedia.h"
#include "TsEncode.h"


class CCaptionParser : public CPesParser::IPacketHandler
{
public:
	class __declspec(novtable) ICaptionHandler {
	public:
		virtual ~ICaptionHandler() = 0;
		virtual void OnLanguageUpdate(CCaptionParser *pParser) {}
		virtual void OnCaption(CCaptionParser *pParser, BYTE Language, LPCTSTR pszText, const CAribString::FormatList *pFormatList) {}
	};

	struct DRCSBitmap {
		BYTE Width;
		BYTE Height;
		BYTE Depth;
		BYTE BitsPerPixel;
		const void *pBits;
		DWORD BitsSize;
	};

	class __declspec(novtable) IDRCSMap : public CAribString::IDRCSMap {
	public:
		virtual ~IDRCSMap() {}
		virtual bool SetDRCS(WORD Code, const DRCSBitmap *pBitmap) = 0;
	};

	CCaptionParser();
	~CCaptionParser();
	void Reset();
	bool StorePacket(const CTsPacket *pPacket);
	void SetCaptionHandler(ICaptionHandler *pHandler);
	void SetDRCSMap(IDRCSMap *pDRCSMap);
	int GetLanguageNum() const;
	bool GetLanguageCode(int LanguageTag, char *pCode) const;

private:
	CPesParser m_PesParser;
	ICaptionHandler *m_pHandler;
	IDRCSMap *m_pDRCSMap;
	struct LanguageInfo {
		BYTE LanguageTag;
		BYTE DMF;
		BYTE DC;
		char LanguageCode[4];
		BYTE Format;
		BYTE TCS;
		BYTE RollupMode;
		bool operator==(const LanguageInfo &Info) {
			return LanguageTag == Info.LanguageTag
				&& DMF == Info.DMF
				&& DC == Info.DC
				&& memcmp(LanguageCode, Info.LanguageCode, 4) == 0
				&& Format == Info.Format
				&& TCS == Info.TCS
				&& RollupMode == Info.RollupMode;
		}
		bool operator!=(const LanguageInfo &Info) { return !(*this == Info); }
	};
	std::vector<LanguageInfo> m_LanguageList;
	BYTE m_DataGroupVersion;
	BYTE m_DataGroupID;

// CPesParser::IPacketHandler
	virtual void OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket);

	bool ParseManagementData(const BYTE *pData, const DWORD DataSize);
	bool ParseCaptionData(const BYTE *pData, const DWORD DataSize);
	bool ParseUnitData(const BYTE *pData, DWORD *pDataSize);
	bool ParseDRCSUnitData(const BYTE *pData, const DWORD DataSize);
	int GetLanguageIndex(const BYTE LanguageTag) const;
	void OnCaption(LPCTSTR pszText, const CAribString::FormatList *pFormatList);
};


#endif
