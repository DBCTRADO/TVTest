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

	CVideoParser();
	virtual ~CVideoParser();

	bool GetVideoInfo(VideoInfo *pInfo) const;
	void ResetVideoInfo();

	typedef void (CALLBACK *VideoInfoCallback)(const VideoInfo *pVideoInfo, const PVOID pParam);
	void SetVideoInfoCallback(VideoInfoCallback pCallback, const PVOID pParam = NULL);

	DWORD GetBitRate() const;

protected:
	void NotifyVideoInfo() const;

	VideoInfo m_VideoInfo;
	VideoInfoCallback m_pVideoInfoCallback;
	PVOID m_pCallbackParam;
	mutable CCritSec m_ParserLock;
	CBitRateCalculator m_BitRateCalculator;
};
