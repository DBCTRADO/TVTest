#pragma once


#include "ITVTestVideoDecoder.h"


// 映像デコーダークラス
class CVideoDecoder
	: protected ITVTestVideoDecoderFrameCapture
{
public:
	// インターレース解除方法(TVTVIDEODEC_DeinterlaceMethod と同じ)
	enum class DeinterlaceMethod
	{
		Weave,
		Blend,
		Bob,
		ELA,
		Yadif,
		Yadif_Bob,
	};

	// フレームのフラグ(TVTVIDEODEC_FRAME_* と同じ)
	enum {
		FrameFlag_TopFieldFirst    = 0x00000001,
		FrameFlag_RepeatFirstField = 0x00000002,
		FrameFlag_Progressive      = 0x00000004,
		FrameFlag_Weave            = 0x00000008,
		FrameFlag_IFrame           = 0x00000010,
		FrameFlag_PFrame           = 0x00000020,
		FrameFlag_BFrame           = 0x00000040
	};

	struct FrameInfo
	{
		int Width;
		int Height;
		int BitsPerPixel;
		int AspectRatioX;
		int AspectRatioY;
		unsigned int Flags;
		const BYTE *Buffer;
		int Pitch;
	};

	class CFrameCapture
	{
	public:
		virtual bool OnFrame(const FrameInfo &Frame) = 0;
	};

	~CVideoDecoder();

	bool Initialize();
	void Finalize();
	bool Open(DWORD Format);
	void Close();
	void SetFrameCapture(CFrameCapture *pFrameCapture);
	bool InputStream(const void *pData, SIZE_T Size);
	void SetDeinterlaceMethod(DeinterlaceMethod Deinterlace);

private:
	HMODULE m_hLib = nullptr;
	ITVTestVideoFrameDecoder *m_pDecoder = nullptr;
	CFrameCapture *m_pFrameCapture = nullptr;
	decltype(TVTestVideoDecoder_CreateInstance) *m_pCreateInstance = nullptr;
	DeinterlaceMethod m_Deinterlace = DeinterlaceMethod::Blend;
	LONG m_RefCount = 1;

	STDMETHOD_(ULONG, AddRef)() override;
	STDMETHOD_(ULONG, Release)() override;
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) override;
	STDMETHOD(OnFrame)(const TVTVIDEODEC_FrameInfo *pFrameInfo) override;
};
