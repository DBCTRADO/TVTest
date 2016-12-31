#pragma once

#include "../BonTsEngine/TsUtilClass.h"


class CVideoParser
{
public:
	class VideoInfo
	{
	public:
		struct FrameRate {
			DWORD Num,Denom;
		};

		WORD m_OrigWidth,m_OrigHeight;
		WORD m_DisplayWidth,m_DisplayHeight;
		WORD m_PosX,m_PosY;
		BYTE m_AspectRatioX,m_AspectRatioY;
		FrameRate m_FrameRate;

		VideoInfo()
		{
			Reset();
		}

		VideoInfo(WORD OrigWidth,WORD OrigHeight,WORD DisplayWidth,WORD DisplayHeight,BYTE AspectX,BYTE AspectY)
		{
			m_OrigWidth=OrigWidth;
			m_OrigHeight=OrigHeight;
			m_DisplayWidth=DisplayWidth;
			m_DisplayHeight=DisplayHeight;
			m_PosX=(OrigWidth-DisplayWidth)/2;
			m_PosY=(OrigHeight-DisplayHeight)/2;
			m_AspectRatioX=AspectX;
			m_AspectRatioY=AspectY;
			m_FrameRate.Num=0;
			m_FrameRate.Denom=0;
		}

		bool operator==(const VideoInfo &Info) const
		{
			return m_OrigWidth==Info.m_OrigWidth
				&& m_OrigHeight==Info.m_OrigHeight
				&& m_DisplayWidth==Info.m_DisplayWidth
				&& m_DisplayHeight==Info.m_DisplayHeight
				&& m_AspectRatioX==Info.m_AspectRatioX
				&& m_AspectRatioY==Info.m_AspectRatioY
				&& m_FrameRate.Num==Info.m_FrameRate.Num
				&& m_FrameRate.Denom==Info.m_FrameRate.Denom;
		}

		bool operator!=(const VideoInfo &Info) const
		{
			return !(*this==Info);
		}

		void Reset()
		{
			m_OrigWidth=0;
			m_OrigHeight=0;
			m_DisplayWidth=0;
			m_DisplayHeight=0;
			m_PosX=0;
			m_PosY=0;
			m_AspectRatioX=0;
			m_AspectRatioY=0;
			m_FrameRate.Num=0;
			m_FrameRate.Denom=0;
		}
	};

	class IStreamCallback
	{
	public:
		virtual ~IStreamCallback() {}
		virtual void OnStream(DWORD Format, const void *pData, SIZE_T Size) = 0;
	};

	enum {
		ADJUST_SAMPLE_TIME       = 0x0001U,
		ADJUST_SAMPLE_FRAME_RATE = 0x0002U,
		ADJUST_SAMPLE_1SEG       = 0x0004U
	};

	CVideoParser();
	virtual ~CVideoParser();

	bool GetVideoInfo(VideoInfo *pInfo) const;
	void ResetVideoInfo();

	typedef void (CALLBACK *VideoInfoCallback)(const VideoInfo *pVideoInfo, const PVOID pParam);
	void SetVideoInfoCallback(VideoInfoCallback pCallback, const PVOID pParam = NULL);

	void SetStreamCallback(IStreamCallback *pCallback);

	void SetAttachMediaType(bool bAttach);
	virtual bool SetAdjustSampleOptions(unsigned int Flags) { return false; }

protected:
	void NotifyVideoInfo() const;
	static bool SARToDAR(WORD SarX, WORD SarY, WORD Width, WORD Height,
						 BYTE *pDarX, BYTE *pDarY);

	VideoInfo m_VideoInfo;
	VideoInfoCallback m_pVideoInfoCallback;
	PVOID m_pCallbackParam;
	IStreamCallback *m_pStreamCallback;
	mutable CCritSec m_ParserLock;
	bool m_bAttachMediaType;
};
