/*
	TVTest �v���O�C���T���v��

	�p�P�b�g�J�E���^�[

	�X�e�[�^�X�o�[�Ƀp�P�b�g�𐔂��鍀�ڂ�ǉ����܂��B
	���̃T���v���ł́A�v���O�C���̗L��/�����̏�Ԃ�
	�X�e�[�^�X�o�[�̍��ڂ̕\��/��\���̏�Ԃ̓������Ƃ��Ă��܂��B
*/


#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"


// �X�e�[�^�X���ڂ̎��ʎq
#define STATUS_ITEM_ID 1


// �v���O�C���N���X
class CPacketCounter : public TVTest::CTVTestPlugin
{
	LONG m_PacketCount;

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static BOOL CALLBACK StreamCallback(BYTE *pData, void *pClientData);
	void ShowItem(bool fShow);

public:
	CPacketCounter()
		: m_PacketCount(0)
	{
	}

	bool GetPluginInfo(TVTest::PluginInfo *pInfo) override;
	bool Initialize() override;
	bool Finalize() override;
};


bool CPacketCounter::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"�p�P�b�g�J�E���^�[";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�X�e�[�^�X�o�[�Ƀp�P�b�g�𐔂��鍀�ڂ�ǉ����܂��B";
	return true;
}


bool CPacketCounter::Initialize()
{
	// ����������

	// �X�e�[�^�X���ڂ�o�^
	TVTest::StatusItemInfo Item;
	Item.Size         = sizeof(Item);
	Item.Flags        = TVTest::STATUS_ITEM_FLAG_TIMERUPDATE;
	Item.Style        = 0;
	Item.ID           = STATUS_ITEM_ID;
	Item.pszIDText    = L"PacketCounter";
	Item.pszName      = L"�p�P�b�g�J�E���^�[";
	Item.MinWidth     = 0;
	Item.MaxWidth     = -1;
	Item.DefaultWidth = TVTest::StatusItemWidthByFontSize(6);
	Item.MinHeight    = 0;
	if (!m_pApp->RegisterStatusItem(&Item)) {
		m_pApp->AddLog(L"�X�e�[�^�X���ڂ�o�^�ł��܂���B", TVTest::LOG_TYPE_ERROR);
		return false;
	}

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback, this);

	// �X�g���[���R�[���o�b�N�֐���o�^
	m_pApp->SetStreamCallback(0, StreamCallback, this);

	return true;
}


bool CPacketCounter::Finalize()
{
	// �I������

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CPacketCounter::EventCallback(
	UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CPacketCounter *pThis = static_cast<CPacketCounter*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		// �v���O�C���̗L����Ԃƍ��ڂ̕\����Ԃ𓯊�����
		pThis->ShowItem(lParam1 != 0);
		return TRUE;

	case TVTest::EVENT_STATUSITEM_DRAW:
		// �X�e�[�^�X���ڂ̕`��
		{
			const TVTest::StatusItemDrawInfo *pInfo =
				reinterpret_cast<const TVTest::StatusItemDrawInfo *>(lParam1);
			WCHAR szText[32];

			if ((pInfo->Flags & TVTest::STATUS_ITEM_DRAW_FLAG_PREVIEW) == 0) {
				// �ʏ�̍��ڂ̕`��
				::_itow_s(pThis->m_PacketCount, szText, 10);
			} else {
				// �v���r���[(�ݒ�_�C�A���O)�̍��ڂ̕`��
				::lstrcpyW(szText, L"123456");
			}
			pThis->m_pApp->ThemeDrawText(pInfo->pszStyle, pInfo->hdc, szText,
										 pInfo->DrawRect,
										 DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS,
										 pInfo->Color);
		}
		return TRUE;

	case TVTest::EVENT_STATUSITEM_NOTIFY:
		// �X�e�[�^�X���ڂ̒ʒm
		{
			const TVTest::StatusItemEventInfo *pInfo =
				reinterpret_cast<const TVTest::StatusItemEventInfo *>(lParam1);

			switch (pInfo->Event) {
			case TVTest::STATUS_ITEM_EVENT_CREATED:
				// ���ڂ��쐬���ꂽ
				// �v���O�C�����L���ł���΍��ڂ�\����Ԃɂ���
				pThis->ShowItem(pThis->m_pApp->IsPluginEnabled());
				return TRUE;

			case TVTest::STATUS_ITEM_EVENT_VISIBILITYCHANGED:
				// ���ڂ̕\����Ԃ��ς����
				// ���ڂ̕\����Ԃƃv���O�C���̗L����Ԃ𓯊�����
				pThis->m_pApp->EnablePlugin(pInfo->Param != 0);
				return TRUE;

			case TVTest::STATUS_ITEM_EVENT_UPDATETIMER:
				// �X�V�^�C�}�[
				return TRUE;	// TRUE ��Ԃ��ƍĕ`�悳���
			}
		}
		return FALSE;

	case TVTest::EVENT_STATUSITEM_MOUSE:
		// �X�e�[�^�X���ڂ̃}�E�X����
		{
			const TVTest::StatusItemMouseEventInfo *pInfo =
				reinterpret_cast<const TVTest::StatusItemMouseEventInfo *>(lParam1);

			// ���{�^���������ꂽ�烊�Z�b�g����
			if (pInfo->Action == TVTest::STATUS_ITEM_MOUSE_ACTION_LDOWN) {
				::InterlockedExchange(&pThis->m_PacketCount, 0);
				// ���ڂ��ĕ`��
				pThis->m_pApp->StatusItemNotify(STATUS_ITEM_ID, TVTest::STATUS_ITEM_NOTIFY_REDRAW);
				return TRUE;
			}
		}
		return FALSE;
	}

	return 0;
}


// �X�g���[���R�[���o�b�N�֐�
// 188�o�C�g�̃p�P�b�g�f�[�^���n�����
BOOL CALLBACK CPacketCounter::StreamCallback(BYTE *pData, void *pClientData)
{
	CPacketCounter *pThis = static_cast<CPacketCounter*>(pClientData);

	::InterlockedIncrement(&pThis->m_PacketCount);

	return TRUE;	// FALSE��Ԃ��ƃp�P�b�g���j�������
}


// �X�e�[�^�X���ڂ̕\��/��\����؂�ւ���
void CPacketCounter::ShowItem(bool fShow)
{
	TVTest::StatusItemSetInfo Info;

	Info.Size      = sizeof(Info);
	Info.Mask      = TVTest::STATUS_ITEM_SET_INFO_MASK_STATE;
	Info.ID        = STATUS_ITEM_ID;
	Info.StateMask = TVTest::STATUS_ITEM_STATE_VISIBLE;
	Info.State     = fShow ? TVTest::STATUS_ITEM_STATE_VISIBLE : 0;

	m_pApp->SetStatusItem(&Info);
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CPacketCounter;
}
