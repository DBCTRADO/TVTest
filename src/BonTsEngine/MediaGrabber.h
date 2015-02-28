// MediaGrabber.h: CMediaGrabber �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include <vector>


/////////////////////////////////////////////////////////////////////////////
// �T���v���O���o(�O���t��ʉ߂���CMediaData�ɃA�N�Z�X�����i��񋟂���)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		���̓f�[�^
// Output	#0	: CMediaData		�o�̓f�[�^
/////////////////////////////////////////////////////////////////////////////

class CMediaGrabber : public CMediaDecoder
{
public:
	class ABSTRACT_CLASS_DECL IGrabber
	{
	public:
		virtual ~IGrabber() {}
		virtual bool OnInputMedia(CMediaData *pMediaData) { return true; }
		virtual void OnReset() {}
	};

	CMediaGrabber(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaGrabber();

// IMediaDecoder
	virtual void Reset(void) override;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

// CMediaGrabber
	bool AddGrabber(IGrabber *pGrabber);
	bool RemoveGrabber(IGrabber *pGrabber);

protected:
	std::vector<IGrabber*> m_GrabberList;
};
