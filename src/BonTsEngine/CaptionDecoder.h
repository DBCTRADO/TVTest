#ifndef CAPTION_DECODER_H
#define CAPTION_DECODER_H


#include <vector>
#include "MediaDecoder.h"
#include "CaptionParser.h"


class CCaptionDecoder : public CMediaDecoder, protected CCaptionParser::ICaptionHandler
{
public:
	class ABSTRACT_CLASS_DECL IHandler {
	public:
		virtual ~IHandler() = 0;
		virtual void OnLanguageUpdate(CCaptionDecoder *pDecoder, CCaptionParser *pParser) {}
		virtual void OnCaption(CCaptionDecoder *pDecoder, CCaptionParser *pParser,
							   BYTE Language, LPCTSTR pszText,
							   const CAribString::FormatList *pFormatList) {}
	};

	typedef CCaptionParser::IDRCSMap IDRCSMap;

	CCaptionDecoder(IEventHandler *pEventHandler = NULL);
	virtual ~CCaptionDecoder();

// CMediaDecoder
	void Reset() override;
	const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

	bool SetTargetStream(WORD ServiceID, BYTE ComponentTag = 0xFF);
	void SetCaptionHandler(IHandler *pHandler);
	void SetDRCSMap(IDRCSMap *pDRCSMap);
	int GetLanguageNum();
	DWORD GetLanguageCode(int LanguageTag);

protected:
// CCaptionParser::IHandler
	virtual void OnLanguageUpdate(CCaptionParser *pParser);
	virtual void OnCaption(CCaptionParser *pParser, BYTE Language, LPCTSTR pszText, const CAribString::FormatList *pFormatList);

	int GetServiceIndexByID(WORD ServiceID) const;
	CCaptionParser *GetCurrentCaptionParser() const;

	CTsPidMapManager m_PidMapManager;

	struct CaptionEsInfo {
		WORD PID;
		BYTE ComponentTag;
	};
	struct ServiceInfo {
		WORD ServiceID;
		WORD PmtPID;
		std::vector<CaptionEsInfo> CaptionEsList;
	};
	std::vector<ServiceInfo> m_ServiceList;

	WORD m_TargetServiceID;
	BYTE m_TargetComponentTag;
	WORD m_TargetEsPID;
	IHandler *m_pCaptionHandler;
	IDRCSMap *m_pDRCSMap;

	friend class CCaptionStream;

private:
	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
};


#endif
