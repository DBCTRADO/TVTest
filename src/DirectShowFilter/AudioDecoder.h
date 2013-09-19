#pragma once


class CAudioDecoder
{
public:
	enum {
		CHANNEL_2_L,
		CHANNEL_2_R
	};
	enum {
		CHANNEL_6_FL,
		CHANNEL_6_FR,
		CHANNEL_6_FC,
		CHANNEL_6_LFE,
		CHANNEL_6_BL,
		CHANNEL_6_BR
	};

	struct AudioInfo
	{
		DWORD Frequency;
		BYTE Channels;
		BYTE OrigChannels;
		bool bDualMono;
	};

	struct DecodeFrameInfo
	{
		const BYTE *pData;
		DWORD Samples;
		AudioInfo Info;
		bool bDiscontinuity;
	};

	struct DownmixInfo
	{
		double Center;
		double Front;
		double Rear;
		double LFE;
	};

	struct SpdifFrameInfo
	{
		WORD Pc;
		int FrameSize;
		int SamplesPerFrame;
	};

	CAudioDecoder();
	virtual ~CAudioDecoder();

	virtual bool Open() = 0;
	virtual void Close() = 0;
	virtual bool IsOpened() const = 0;
	virtual bool Reset() = 0;
	virtual bool Decode(const BYTE *pData, DWORD *pDataSize, DecodeFrameInfo *pInfo) = 0;

	virtual bool IsSpdifSupported() const { return false; }
	virtual bool GetSpdifFrameInfo(SpdifFrameInfo *pInfo) const { return false; }
	virtual int GetSpdifBurstPayload(BYTE *pBuffer, DWORD BufferSize) const { return false; }

	virtual bool GetChannelMap(int Channels, int *pMap) const { return false; }
	virtual bool GetDownmixInfo(DownmixInfo *pInfo) const { return false; }

	bool GetAudioInfo(AudioInfo *pInfo) const;

protected:
	void ClearAudioInfo();

	AudioInfo m_AudioInfo;
};
