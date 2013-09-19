// BonSrcDecoder.h: CBonSrcDecoder クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <deque>
#include "MediaDecoder.h"
#include "IBonDriver.h"
#include "IBonDriver2.h"


/////////////////////////////////////////////////////////////////////////////
// Bonソースデコーダ(チューナからTS平分ストリームを受信する)
/////////////////////////////////////////////////////////////////////////////
// Output	#0	: CMediaData		平分TSストリーム
/////////////////////////////////////////////////////////////////////////////

class CBonSrcDecoder : public CMediaDecoder
{
public:
	// エラーコード
	enum {
		ERR_NOERROR,		// エラーなし
		ERR_NOTLOADED,		// ライブラリが読み込まれていない
		ERR_DRIVER,			// ドライバエラー
		ERR_TUNEROPEN,		// チューナオープンエラー
		ERR_TUNER,			// チューナエラー
		ERR_NOTOPEN,		// チューナが開かれていない
		ERR_ALREADYOPEN,	// チューナが既に開かれている
		ERR_NOTPLAYING,		// 再生されていない
		ERR_ALREADYPLAYING,	// 既に再生されている
		ERR_TIMEOUT,		// タイムアウト
		ERR_PENDING,		// 前の処理が終わっていない
		ERR_INTERNAL		// 内部エラー
	};

	enum {
		FIRST_CHANNEL_SET_DELAY_MAX = 5000UL,
		CHANNEL_CHANGE_INTERVAL_MAX = 5000UL
	};

	CBonSrcDecoder(IEventHandler *pEventHandler = NULL);
	virtual ~CBonSrcDecoder();

// IMediaDecoder
	virtual void Reset(void) override;
	virtual void ResetGraph(void) override;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

// CBonSrcDecoder
	bool LoadBonDriver(LPCTSTR pszFileName);
	bool UnloadBonDriver();
	bool IsBonDriverLoaded() const;
	bool OpenTuner();
	bool CloseTuner(void);
	bool IsOpen() const;

	bool Play(void);
	bool Stop(void);

	bool SetChannel(const BYTE byChannel);
	bool SetChannel(const DWORD dwSpace, const DWORD dwChannel);
	bool SetChannelAndPlay(const DWORD dwSpace, const DWORD dwChannel);

	bool IsBonDriver2(void) const;
	LPCTSTR GetSpaceName(const DWORD dwSpace) const;
	LPCTSTR GetChannelName(const DWORD dwSpace, const DWORD dwChannel) const;

	bool PurgeStream(void);

	int NumSpaces() const;
	LPCTSTR GetTunerName() const;
	int GetCurSpace() const;
	int GetCurChannel() const;

	float GetSignalLevel(void);
	DWORD GetBitRate() const;
	DWORD GetStreamRemain() const;

	bool SetStreamThreadPriority(int Priority);
	int GetStreamThreadPriority() const { return m_StreamThreadPriority; }
	void SetPurgeStreamOnChannelChange(bool bPurge);
	bool IsPurgeStreamOnChannelChange() const { return m_bPurgeStreamOnChannelChange; }
	bool SetFirstChannelSetDelay(const DWORD Delay);
	DWORD GetFirstChannelSetDelay() const { return m_FirstChannelSetDelay; }
	bool SetMinChannelChangeInterval(const DWORD Interval);
	DWORD GetMinChannelChangeInterval() const { return m_MinChannelChangeInterval; }

private:
	struct StreamingRequest
	{
		enum RequestType
		{
			TYPE_SETCHANNEL,
			TYPE_SETCHANNEL2,
			TYPE_PURGESTREAM,
			TYPE_RESET
		};

		RequestType Type;
		bool bProcessing;

		union
		{
			struct {
				BYTE Channel;
			} SetChannel;
			struct {
				DWORD Space;
				DWORD Channel;
			} SetChannel2;
		};
	};

	static unsigned int __stdcall StreamingThread(LPVOID pParam);
	void StreamingMain();

	void ResetStatus();
	void AddRequest(const StreamingRequest &Request);
	void AddRequests(const StreamingRequest *pRequestList, int RequestCount);
	bool WaitAllRequests(DWORD Timeout);
	bool HasPendingRequest();
	void SetRequestTimeoutError();
	void SetChannelWait();

	HMODULE m_hBonDriverLib;
	IBonDriver *m_pBonDriver;
	IBonDriver2 *m_pBonDriver2;	

	HANDLE m_hStreamRecvThread;
	CLocalEvent m_EndEvent;
	std::deque<StreamingRequest> m_RequestQueue;
	CCriticalLock m_RequestLock;
	CLocalEvent m_RequestEvent;
	DWORD m_RequestTimeout;
	BOOL m_bRequestResult;

	volatile bool m_bIsPlaying;

	float m_SignalLevel;
	CBitRateCalculator m_BitRateCalculator;
	DWORD m_StreamRemain;

	int m_StreamThreadPriority;
	bool m_bPurgeStreamOnChannelChange;

	DWORD m_FirstChannelSetDelay;
	DWORD m_MinChannelChangeInterval;
	DWORD m_TunerOpenTime;
	DWORD m_SetChannelTime;
	UINT m_SetChannelCount;
};
