#pragma once


#include "../EVRenderer.h"


class CVideoRenderer_EVRCustomPresenter : public CVideoRenderer_EVR
{
public:
	CVideoRenderer_EVRCustomPresenter();
	~CVideoRenderer_EVRCustomPresenter();
	bool Finalize() override;
	bool HasProperty() override { return false; }	// �v���p�e�B�̐��l���s��l�ɂȂ�

private:
	HRESULT InitializePresenter(IBaseFilter *pFilter) override;

	struct IMFVideoPresenter *m_pPresenter;
};
