#include "stdafx.h"
#include <algorithm>
#include "TVTest.h"
#include "AppMain.h"
#include "ColorScheme.h"
#include "Settings.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "ThemeManager.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

using namespace TVTest;


#define MAX_COLORSCHEME_NAME 128

static const LPCTSTR GradientDirectionList[] = {
	TEXT("horizontal"),
	TEXT("vertical"),
	TEXT("horizontal-mirror"),
	TEXT("vertical-mirror"),
};


#define HEXRGB(hex) RGB((hex)>>16,((hex)>>8)&0xFF,(hex)&0xFF)

const CColorScheme::ColorInfo CColorScheme::m_ColorInfoList[NUM_COLORS] = {
	{HEXRGB(0x333333),	TEXT("StatusBack"),							TEXT("�X�e�[�^�X�o�[ �w�i1")},
	{HEXRGB(0x111111),	TEXT("StatusBack2"),						TEXT("�X�e�[�^�X�o�[ �w�i2")},
	{HEXRGB(0x999999),	TEXT("StatusText"),							TEXT("�X�e�[�^�X�o�[ ����")},
	{HEXRGB(0x777777),	TEXT("StatusItemBorder"),					TEXT("�X�e�[�^�X�o�[ ���ڊO�g")},
	{HEXRGB(0x111111),	TEXT("StatusBottomItemBack"),				TEXT("�X�e�[�^�X�o�[ ���i�w�i1")},
	{HEXRGB(0x111111),	TEXT("StatusBottomItemBack2"),				TEXT("�X�e�[�^�X�o�[ ���i�w�i2")},
	{HEXRGB(0x999999),	TEXT("StatusBottomItemText"),				TEXT("�X�e�[�^�X�o�[ ���i����")},
	{HEXRGB(0x111111),	TEXT("StatusBottomItemBorder"),				TEXT("�X�e�[�^�X�o�[ ���i�O�g")},
	{HEXRGB(0x4486E8),	TEXT("StatusHighlightBack"),				TEXT("�X�e�[�^�X�o�[ �I��w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("StatusHighlightBack2"),				TEXT("�X�e�[�^�X�o�[ �I��w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("StatusHighlightText"),				TEXT("�X�e�[�^�X�o�[ �I�𕶎�")},
	{HEXRGB(0x3C76CC),	TEXT("StatusHighlightBorder"),				TEXT("�X�e�[�^�X�o�[ �I���O�g")},
	{HEXRGB(0x111111),	TEXT("StatusBorder"),						TEXT("�X�e�[�^�X�o�[ �O�g")},
	{HEXRGB(0xDF3F00),	TEXT("StatusRecordingCircle"),				TEXT("�X�e�[�^�X�o�[ �^�恜")},
	{HEXRGB(0x444444),	TEXT("StatusEventProgressBack"),			TEXT("�X�e�[�^�X�o�[ �ԑg�o�ߎ��Ԕw�i1")},
	{HEXRGB(0x444444),	TEXT("StatusEventProgressBack2"),			TEXT("�X�e�[�^�X�o�[ �ԑg�o�ߎ��Ԕw�i2")},
	{HEXRGB(0x444444),	TEXT("StatusEventProgressBorder"),			TEXT("�X�e�[�^�X�o�[ �ԑg�o�ߎ��ԊO�g")},
	{HEXRGB(0x3465B0),	TEXT("StatusEventProgressElapsed"),			TEXT("�X�e�[�^�X�o�[ �ԑg�o�ߎ��ԃo�[1")},
	{HEXRGB(0x3465B0),	TEXT("StatusEventProgressElapsed2"),		TEXT("�X�e�[�^�X�o�[ �ԑg�o�ߎ��ԃo�[2")},
	{HEXRGB(0x3465B0),	TEXT("StatusEventProgressElapsedBorder"),	TEXT("�X�e�[�^�X�o�[ �ԑg�o�ߎ��ԃo�[�O�g")},
	{HEXRGB(0x222222),	TEXT("Splitter"),							TEXT("������")},
	{HEXRGB(0x000000),	TEXT("ScreenBorder"),						TEXT("��ʂ̊O�g")},
	{HEXRGB(0x555555),	TEXT("WindowFrame"),						TEXT("�E�B���h�E �טg")},
	{HEXRGB(0x555555),	TEXT("WindowFrameBorder"),					TEXT("�E�B���h�E �טg�̋��E")},
	{HEXRGB(0x666666),	TEXT("WindowActiveFrame"),					TEXT("�E�B���h�E �A�N�e�B�u�טg")},
	{HEXRGB(0x666666),	TEXT("WindowActiveFrameBorder"),			TEXT("�E�B���h�E �A�N�e�B�u�טg�̋��E")},
	{HEXRGB(0x333333),	TEXT("PanelBack"),							TEXT("�p�l�� �w�i")},
	{HEXRGB(0x999999),	TEXT("PanelText"),							TEXT("�p�l�� ����")},
	{HEXRGB(0x000000),	TEXT("PanelTabBack"),						TEXT("�p�l�� �^�u�w�i1")},
	{HEXRGB(0x222222),	TEXT("PanelTabBack2"),						TEXT("�p�l�� �^�u�w�i2")},
	{HEXRGB(0x888888),	TEXT("PanelTabText"),						TEXT("�p�l�� �^�u����")},
	{HEXRGB(0x000000),	TEXT("PanelTabBorder"),						TEXT("�p�l�� �^�u�O�g")},
	{HEXRGB(0x555555),	TEXT("PanelCurTabBack"),					TEXT("�p�l�� �I���^�u�w�i1")},
	{HEXRGB(0x333333),	TEXT("PanelCurTabBack2"),					TEXT("�p�l�� �I���^�u�w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("PanelCurTabText"),					TEXT("�p�l�� �I���^�u����")},
	{HEXRGB(0x444444),	TEXT("PanelCurTabBorder"),					TEXT("�p�l�� �I���^�u�O�g")},
	{HEXRGB(0x000000),	TEXT("PanelTabMargin"),						TEXT("�p�l�� �^�u�]��1")},
	{HEXRGB(0x222222),	TEXT("PanelTabMargin2"),					TEXT("�p�l�� �^�u�]��2")},
	{HEXRGB(0x888888),	TEXT("PanelTabMarginBorder"),				TEXT("�p�l�� �^�u�]���O�g")},
	{HEXRGB(0x444444),	TEXT("PanelTabLine"),						TEXT("�p�l�� �^�u��")},
	{HEXRGB(0x333333),	TEXT("PanelTitleBack"),						TEXT("�p�l�� �^�C�g���w�i1")},
	{HEXRGB(0x111111),	TEXT("PanelTitleBack2"),					TEXT("�p�l�� �^�C�g���w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("PanelTitleText"),						TEXT("�p�l�� �^�C�g������")},
	{HEXRGB(0x111111),	TEXT("PanelTitleBorder"),					TEXT("�p�l�� �^�C�g���O�g")},
	{HEXRGB(0x111111),	TEXT("ProgramInfoBack"),					TEXT("���p�l�� �ԑg���w�i")},
	{HEXRGB(0xAAAAAA),	TEXT("ProgramInfoText"),					TEXT("���p�l�� �ԑg��񕶎�")},
	{HEXRGB(0x111111),	TEXT("ProgramInfoBorder"),					TEXT("���p�l�� �ԑg���O�g")},
	{HEXRGB(0x333333),	TEXT("InformationPanelButtonBack"),			TEXT("���p�l�� �{�^���w�i1")},
	{HEXRGB(0x333333),	TEXT("InformationPanelButtonBack2"),		TEXT("���p�l�� �{�^���w�i2")},
	{HEXRGB(0x999999),	TEXT("InformationPanelButtonText"),			TEXT("���p�l�� �{�^������")},
	{HEXRGB(0x333333),	TEXT("InformationPanelButtonBorder"),		TEXT("���p�l�� �{�^�����E")},
	{HEXRGB(0x4486E8),	TEXT("InformationPanelHotButtonBack"),		TEXT("���p�l�� �I���{�^���w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("InformationPanelHotButtonBack2"),		TEXT("���p�l�� �I���{�^���w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("InformationPanelHotButtonText"),		TEXT("���p�l�� �I���{�^������")},
	{HEXRGB(0x3C76CC),	TEXT("InformationPanelHotButtonBorder"),	TEXT("���p�l�� �I���{�^�����E")},
	{HEXRGB(0x333333),	TEXT("ProgramListChannelBack"),				TEXT("�ԑg�\�p�l�� �ǖ��w�i1")},
	{HEXRGB(0x000000),	TEXT("ProgramListChannelBack2"),			TEXT("�ԑg�\�p�l�� �ǖ��w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("ProgramListChannelText"),				TEXT("�ԑg�\�p�l�� �ǖ�����")},
	{HEXRGB(0x000000),	TEXT("ProgramListChannelBorder"),			TEXT("�ԑg�\�p�l�� �ǖ��O�g")},
	{HEXRGB(0x4486E8),	TEXT("ProgramListCurChannelBack"),			TEXT("�ԑg�\�p�l�� ���݋ǖ��w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("ProgramListCurChannelBack2"),			TEXT("�ԑg�\�p�l�� ���݋ǖ��w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramListCurChannelText"),			TEXT("�ԑg�\�p�l�� ���݋ǖ�����")},
	{HEXRGB(0x000000),	TEXT("ProgramListCurChannelBorder"),		TEXT("�ԑg�\�p�l�� ���݋ǖ��O�g")},
	{HEXRGB(0x333333),	TEXT("ProgramListChannelButtonBack"),		TEXT("�ԑg�\�p�l�� �{�^���w�i1")},
	{HEXRGB(0x000000),	TEXT("ProgramListChannelButtonBack2"),		TEXT("�ԑg�\�p�l�� �{�^���w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("ProgramListChannelButtonText"),		TEXT("�ԑg�\�p�l�� �{�^������")},
	{HEXRGB(0x000000),	TEXT("ProgramListChannelButtonBorder"),		TEXT("�ԑg�\�p�l�� �{�^���O�g")},
	{HEXRGB(0x4486E8),	TEXT("ProgramListChannelButtonHotBack"),	TEXT("�ԑg�\�p�l�� �I���{�^���w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("ProgramListChannelButtonHotBack2"),	TEXT("�ԑg�\�p�l�� �I���{�^���w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramListChannelButtonHotText"),	TEXT("�ԑg�\�p�l�� �I���{�^������")},
	{HEXRGB(0x3C76CC),	TEXT("ProgramListChannelButtonHotBorder"),	TEXT("�ԑg�\�p�l�� �I���{�^���O�g")},
	{HEXRGB(0x333333),	TEXT("ProgramListBack"),					TEXT("�ԑg�\�p�l�� �ԑg���e�w�i1")},
	{HEXRGB(0x333333),	TEXT("ProgramListBack2"),					TEXT("�ԑg�\�p�l�� �ԑg���e�w�i2")},
	{HEXRGB(0x999999),	TEXT("ProgramListText"),					TEXT("�ԑg�\�p�l�� �ԑg���e����")},
	{HEXRGB(0x444444),	TEXT("ProgramListBorder"),					TEXT("�ԑg�\�p�l�� �ԑg���e�O�g")},
	{HEXRGB(0x222222),	TEXT("ProgramListCurBack"),					TEXT("�ԑg�\�p�l�� ���ݔԑg���e�w�i1")},
	{HEXRGB(0x333333),	TEXT("ProgramListCurBack2"),				TEXT("�ԑg�\�p�l�� ���ݔԑg���e�w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("ProgramListCurText"),					TEXT("�ԑg�\�p�l�� ���ݔԑg���e����")},
	{HEXRGB(0x555555),	TEXT("ProgramListCurBorder"),				TEXT("�ԑg�\�p�l�� ���ݔԑg���e�O�g")},
	{HEXRGB(0x333333),	TEXT("ProgramListTitleBack"),				TEXT("�ԑg�\�p�l�� �ԑg���w�i1")},
	{HEXRGB(0x000000),	TEXT("ProgramListTitleBack2"),				TEXT("�ԑg�\�p�l�� �ԑg���w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("ProgramListTitleText"),				TEXT("�ԑg�\�p�l�� �ԑg������")},
	{HEXRGB(0x000000),	TEXT("ProgramListTitleBorder"),				TEXT("�ԑg�\�p�l�� �ԑg���O�g")},
	{HEXRGB(0x4486E8),	TEXT("ProgramListCurTitleBack"),			TEXT("�ԑg�\�p�l�� ���ݔԑg���w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("ProgramListCurTitleBack2"),			TEXT("�ԑg�\�p�l�� ���ݔԑg���w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramListCurTitleText"),			TEXT("�ԑg�\�p�l�� ���ݔԑg������")},
	{HEXRGB(0x000000),	TEXT("ProgramListCurTitleBorder"),			TEXT("�ԑg�\�p�l�� ���ݔԑg���O�g")},
	{HEXRGB(0x333333),	TEXT("ChannelPanelChannelNameBack"),		TEXT("�`�����l���p�l�� �ǖ��w�i1")},
	{HEXRGB(0x000000),	TEXT("ChannelPanelChannelNameBack2"),		TEXT("�`�����l���p�l�� �ǖ��w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("ChannelPanelChannelNameText"),		TEXT("�`�����l���p�l�� �ǖ�����")},
	{HEXRGB(0x000000),	TEXT("ChannelPanelChannelNameBorder"),		TEXT("�`�����l���p�l�� �ǖ��O�g")},
	{HEXRGB(0x4486E8),	TEXT("ChannelPanelCurChannelNameBack"),		TEXT("�`�����l���p�l�� ���݋ǖ��w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("ChannelPanelCurChannelNameBack2"),	TEXT("�`�����l���p�l�� ���݋ǖ��w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("ChannelPanelCurChannelNameText"),		TEXT("�`�����l���p�l�� ���݋ǖ�����")},
	{HEXRGB(0x000000),	TEXT("ChannelPanelCurChannelNameBorder"),	TEXT("�`�����l���p�l�� ���݋ǖ��O�g")},
	{HEXRGB(0x444444),	TEXT("ChannelPanelEventNameBack"),			TEXT("�`�����l���p�l�� �ԑg��1�w�i1")},
	{HEXRGB(0x333333),	TEXT("ChannelPanelEventNameBack2"),			TEXT("�`�����l���p�l�� �ԑg��1�w�i2")},
	{HEXRGB(0x999999),	TEXT("ChannelPanelEventNameText"),			TEXT("�`�����l���p�l�� �ԑg��1����")},
	{HEXRGB(0x444444),	TEXT("ChannelPanelEventNameBorder"),		TEXT("�`�����l���p�l�� �ԑg��1�O�g")},
	{HEXRGB(0x222222),	TEXT("ChannelPanelEventName2Back"),			TEXT("�`�����l���p�l�� �ԑg��2�w�i1")},
	{HEXRGB(0x222222),	TEXT("ChannelPanelEventName2Back2"),		TEXT("�`�����l���p�l�� �ԑg��2�w�i2")},
	{HEXRGB(0x999999),	TEXT("ChannelPanelEventName2Text"),			TEXT("�`�����l���p�l�� �ԑg��2����")},
	{HEXRGB(0x333333),	TEXT("ChannelPanelEventName2Border"),		TEXT("�`�����l���p�l�� �ԑg��2�O�g")},
	{HEXRGB(0x444444),	TEXT("ChannelPanelCurEventNameBack"),		TEXT("�`�����l���p�l�� �I��ԑg��1�w�i1")},
	{HEXRGB(0x333333),	TEXT("ChannelPanelCurEventNameBack2"),		TEXT("�`�����l���p�l�� �I��ԑg��1�w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("ChannelPanelCurEventNameText"),		TEXT("�`�����l���p�l�� �I��ԑg��1����")},
	{HEXRGB(0x444444),	TEXT("ChannelPanelCurEventNameBorder"),		TEXT("�`�����l���p�l�� �I��ԑg��1�O�g")},
	{HEXRGB(0x222222),	TEXT("ChannelPanelCurEventName2Back"),		TEXT("�`�����l���p�l�� �I��ԑg��2�w�i1")},
	{HEXRGB(0x222222),	TEXT("ChannelPanelCurEventName2Back2"),		TEXT("�`�����l���p�l�� �I��ԑg��2�w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("ChannelPanelCurEventName2Text"),		TEXT("�`�����l���p�l�� �I��ԑg��2����")},
	{HEXRGB(0x333333),	TEXT("ChannelPanelCurEventName2Border"),	TEXT("�`�����l���p�l�� �I��ԑg��2�O�g")},
	{HEXRGB(0x00FF00),	TEXT("ChannelPanelFeaturedMark"),			TEXT("�`�����l���p�l�� ���ڃ}�[�N�w�i1")},
	{HEXRGB(0x00FF00),	TEXT("ChannelPanelFeaturedMark2"),			TEXT("�`�����l���p�l�� ���ڃ}�[�N�w�i2")},
	{HEXRGB(0x00BF00),	TEXT("ChannelPanelFeaturedMarkBorder"),		TEXT("�`�����l���p�l�� ���ڃ}�[�N�O�g")},
	{HEXRGB(0x2D5899),	TEXT("ChannelPanelProgress"),				TEXT("�`�����l���p�l�� �ԑg�o�ߎ��ԃo�[1")},
	{HEXRGB(0x2D5899),	TEXT("ChannelPanelProgress2"),				TEXT("�`�����l���p�l�� �ԑg�o�ߎ��ԃo�[2")},
	{HEXRGB(0x2D5899),	TEXT("ChannelPanelProgressBorder"),			TEXT("�`�����l���p�l�� �ԑg�o�ߎ��ԃo�[�O�g")},
	{HEXRGB(0x3465B0),	TEXT("ChannelPanelCurProgress"),			TEXT("�`�����l���p�l�� �I��ԑg�o�ߎ��ԃo�[1")},
	{HEXRGB(0x3465B0),	TEXT("ChannelPanelCurProgress2"),			TEXT("�`�����l���p�l�� �I��ԑg�o�ߎ��ԃo�[2")},
	{HEXRGB(0x3465B0),	TEXT("ChannelPanelCurProgressBorder"),		TEXT("�`�����l���p�l�� �I��ԑg�o�ߎ��ԃo�[�O�g")},
	{HEXRGB(0x333333),	TEXT("ControlPanelBack"),					TEXT("����p�l�� �w�i1")},
	{HEXRGB(0x333333),	TEXT("ControlPanelBack2"),					TEXT("����p�l�� �w�i2")},
	{HEXRGB(0x999999),	TEXT("ControlPanelText"),					TEXT("����p�l�� ����")},
	{HEXRGB(0x666666),	TEXT("ControlPanelItemBorder"),				TEXT("����p�l�� ���ڊO�g")},
	{HEXRGB(0x4486E8),	TEXT("ControlPanelHighlightBack"),			TEXT("����p�l�� �I��w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("ControlPanelHighlightBack2"),			TEXT("����p�l�� �I��w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("ControlPanelHighlightText"),			TEXT("����p�l�� �I�𕶎�")},
	{HEXRGB(0x3C76CC),	TEXT("ControlPanelHighlightBorder"),		TEXT("����p�l�� �I�����ڊO�g")},
	{HEXRGB(0x444444),	TEXT("ControlPanelCheckedBack"),			TEXT("����p�l�� �`�F�b�N�w�i1")},
	{HEXRGB(0x555555),	TEXT("ControlPanelCheckedBack2"),			TEXT("����p�l�� �`�F�b�N�w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("ControlPanelCheckedText"),			TEXT("����p�l�� �`�F�b�N����")},
	{HEXRGB(0x333333),	TEXT("ControlPanelCheckedBorder"),			TEXT("����p�l�� �`�F�b�N���ڊO�g")},
	{HEXRGB(0x333333),	TEXT("ControlPanelMargin"),					TEXT("����p�l�� �]��")},
	{HEXRGB(0x333333),	TEXT("CaptionPanelBack"),					TEXT("�����p�l�� �w�i")},
	{HEXRGB(0x999999),	TEXT("CaptionPanelText"),					TEXT("�����p�l�� ����")},
	{HEXRGB(0x333333),	TEXT("TitleBarBack"),						TEXT("�^�C�g���o�[ �w�i1")},
	{HEXRGB(0x111111),	TEXT("TitleBarBack2"),						TEXT("�^�C�g���o�[ �w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("TitleBarText"),						TEXT("�^�C�g���o�[ ����")},
	{HEXRGB(0x777777),	TEXT("TitleBarTextBorder"),					TEXT("�^�C�g���o�[ �����O�g")},
	{HEXRGB(0x333333),	TEXT("TitleBarIconBack"),					TEXT("�^�C�g���o�[ �A�C�R���w�i1")},
	{HEXRGB(0x111111),	TEXT("TitleBarIconBack2"),					TEXT("�^�C�g���o�[ �A�C�R���w�i2")},
	{HEXRGB(0x999999),	TEXT("TitleBarIcon"),						TEXT("�^�C�g���o�[ �A�C�R��")},
	{HEXRGB(0x777777),	TEXT("TitleBarIconBorder"),					TEXT("�^�C�g���o�[ �A�C�R���O�g")},
	{HEXRGB(0x4486E8),	TEXT("TitleBarHighlightBack"),				TEXT("�^�C�g���o�[ �I��w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("TitleBarHighlightBack2"),				TEXT("�^�C�g���o�[ �I��w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("TitleBarHighlightIcon"),				TEXT("�^�C�g���o�[ �I���A�C�R��")},
	{HEXRGB(0x3C76CC),	TEXT("TitleBarHighlightIconBorder"),		TEXT("�^�C�g���o�[ �I���A�C�R���O�g")},
	{HEXRGB(0x111111),	TEXT("TitleBarBorder"),						TEXT("�^�C�g���o�[ �O�g")},
	{HEXRGB(0x333333),	TEXT("SideBarBack"),						TEXT("�T�C�h�o�[ �w�i1")},
	{HEXRGB(0x111111),	TEXT("SideBarBack2"),						TEXT("�T�C�h�o�[ �w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("SideBarIcon"),						TEXT("�T�C�h�o�[ �A�C�R��")},
	{HEXRGB(0x777777),	TEXT("SideBarItemBorder"),					TEXT("�T�C�h�o�[ ���ڊO�g")},
	{HEXRGB(0x4486E8),	TEXT("SideBarHighlightBack"),				TEXT("�T�C�h�o�[ �I��w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("SideBarHighlightBack2"),				TEXT("�T�C�h�o�[ �I��w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("SideBarHighlightIcon"),				TEXT("�T�C�h�o�[ �I���A�C�R��")},
	{HEXRGB(0x3C76CC),	TEXT("SideBarHighlightBorder"),				TEXT("�T�C�h�o�[ �I���O�g")},
	{HEXRGB(0x333333),	TEXT("SideBarCheckBack"),					TEXT("�T�C�h�o�[ �`�F�b�N�w�i1")},
	{HEXRGB(0x444444),	TEXT("SideBarCheckBack2"),					TEXT("�T�C�h�o�[ �`�F�b�N�w�i2")},
	{HEXRGB(0xAAAAAA),	TEXT("SideBarCheckIcon"),					TEXT("�T�C�h�o�[ �`�F�b�N�A�C�R��")},
	{HEXRGB(0x222222),	TEXT("SideBarCheckBorder"),					TEXT("�T�C�h�o�[ �`�F�b�N�O�g")},
	{HEXRGB(0x111111),	TEXT("SideBarBorder"),						TEXT("�T�C�h�o�[ �O�g")},
	{HEXRGB(0x222222),	TEXT("NotificationBarBack"),				TEXT("�ʒm�o�[ �w�i1")},
	{HEXRGB(0x333333),	TEXT("NotificationBarBack2"),				TEXT("�ʒm�o�[ �w�i2")},
	{HEXRGB(0xBBBBBB),	TEXT("NotificationBarText"),				TEXT("�ʒm�o�[ ����")},
	{HEXRGB(0xFF9F44),	TEXT("NotificationBarWarningText"),			TEXT("�ʒm�o�[ �x������")},
	{HEXRGB(0xFF4444),	TEXT("NotificationBarErrorText"),			TEXT("�ʒm�o�[ �G���[����")},
	{HEXRGB(0x333333),	TEXT("ProgramGuideBack"),					TEXT("EPG�ԑg�\ �w�i")},
	{HEXRGB(0x222222),	TEXT("ProgramGuideText"),					TEXT("EPG�ԑg�\ �ԑg���e")},
	{HEXRGB(0x000000),	TEXT("ProgramGuideEventTitle"),				TEXT("EPG�ԑg�\ �ԑg��")},
	{HEXRGB(0x0000BF),	TEXT("ProgramGuideHighlightText"),			TEXT("EPG�ԑg�\ �����ԑg���e")},
	{HEXRGB(0x0000FF),	TEXT("ProgramGuideHighlightTitle"),			TEXT("EPG�ԑg�\ �����ԑg��")},
	{HEXRGB(0x6666FF),	TEXT("ProgramGuideHighlightBorder"),		TEXT("EPG�ԑg�\ �����ԑg�g")},
	{HEXRGB(0xCCFFCC),	TEXT("ProgramGuideFeaturedMark"),			TEXT("EPG�ԑg�\ ���ڃ}�[�N�w�i1")},
	{HEXRGB(0x99FF99),	TEXT("ProgramGuideFeaturedMark2"),			TEXT("EPG�ԑg�\ ���ڃ}�[�N�w�i2")},
	{HEXRGB(0x00EF00),	TEXT("ProgramGuideFeaturedMarkBorder"),		TEXT("EPG�ԑg�\ ���ڃ}�[�N�O�g")},
	{HEXRGB(0x333333),	TEXT("ProgramGuideChannelBack"),			TEXT("EPG�ԑg�\ �`�����l�����w�i1")},
	{HEXRGB(0x111111),	TEXT("ProgramGuideChannelBack2"),			TEXT("EPG�ԑg�\ �`�����l�����w�i2")},
	{HEXRGB(0x999999),	TEXT("ProgramGuideChannelText"),			TEXT("EPG�ԑg�\ �`�����l��������")},
	{HEXRGB(0x4486E8),	TEXT("ProgramGuideCurChannelBack"),			TEXT("EPG�ԑg�\ �`�����l�����I��w�i1")},
	{HEXRGB(0x3C76CC),	TEXT("ProgramGuideCurChannelBack2"),		TEXT("EPG�ԑg�\ �`�����l�����I��w�i2")},
	{HEXRGB(0xDDDDDD),	TEXT("ProgramGuideCurChannelText"),			TEXT("EPG�ԑg�\ �`�����l�����I�𕶎�")},
	{HEXRGB(0x333333),	TEXT("ProgramGuideTimeBack"),				TEXT("EPG�ԑg�\ �����w�i1")},
	{HEXRGB(0x111111),	TEXT("ProgramGuideTimeBack2"),				TEXT("EPG�ԑg�\ �����w�i2")},
	{HEXRGB(0x00337F),	TEXT("ProgramGuideTime0To2Back"),			TEXT("EPG�ԑg�\ 0�`2���w�i1")},
	{HEXRGB(0x00193F),	TEXT("ProgramGuideTime0To2Back2"),			TEXT("EPG�ԑg�\ 0�`2���w�i2")},
	{HEXRGB(0x00667F),	TEXT("ProgramGuideTime3To5Back"),			TEXT("EPG�ԑg�\ 3�`5���w�i1")},
	{HEXRGB(0x00333F),	TEXT("ProgramGuideTime3To5Back2"),			TEXT("EPG�ԑg�\ 3�`5���w�i2")},
	{HEXRGB(0x007F66),	TEXT("ProgramGuideTime6To8Back"),			TEXT("EPG�ԑg�\ 6�`8���w�i1")},
	{HEXRGB(0x003F33),	TEXT("ProgramGuideTime6To8Back2"),			TEXT("EPG�ԑg�\ 6�`8���w�i2")},
	{HEXRGB(0x667F00),	TEXT("ProgramGuideTime9To11Back"),			TEXT("EPG�ԑg�\ 9�`11���w�i1")},
	{HEXRGB(0x333F00),	TEXT("ProgramGuideTime9To11Back2"),			TEXT("EPG�ԑg�\ 9�`11���w�i2")},
	{HEXRGB(0x7F6600),	TEXT("ProgramGuideTime12To14Back"),			TEXT("EPG�ԑg�\ 12�`14���w�i1")},
	{HEXRGB(0x3F3300),	TEXT("ProgramGuideTime12To14Back2"),		TEXT("EPG�ԑg�\ 12�`14���w�i2")},
	{HEXRGB(0x7F3300),	TEXT("ProgramGuideTime15To17Back"),			TEXT("EPG�ԑg�\ 15�`17���w�i1")},
	{HEXRGB(0x3F1900),	TEXT("ProgramGuideTime15To17Back2"),		TEXT("EPG�ԑg�\ 15�`17�Ԕw�i2")},
	{HEXRGB(0x7F0066),	TEXT("ProgramGuideTime18To20Back"),			TEXT("EPG�ԑg�\ 18�`20���w�i1")},
	{HEXRGB(0x3F0033),	TEXT("ProgramGuideTime18To20Back2"),		TEXT("EPG�ԑg�\ 18�`20���w�i2")},
	{HEXRGB(0x66007F),	TEXT("ProgramGuideTime21To23Back"),			TEXT("EPG�ԑg�\ 21�`23���w�i1")},
	{HEXRGB(0x33003F),	TEXT("ProgramGuideTime21To23Back2"),		TEXT("EPG�ԑg�\ 21�`23���w�i2")},
	{HEXRGB(0xBBBBBB),	TEXT("ProgramGuideTimeText"),				TEXT("EPG�ԑg�\ ���ԕ���")},
	{HEXRGB(0x888888),	TEXT("ProgramGuideTimeLine"),				TEXT("EPG�ԑg�\ ���Ԑ�")},
	{HEXRGB(0xFF6600),	TEXT("ProgramGuideCurTimeLine"),			TEXT("EPG�ԑg�\ ���ݎ�����")},
	{HEXRGB(0xFFFFE0),	TEXT("EPGContentNews"),						TEXT("EPG�ԑg�\ �j���[�X�ԑg")},
	{HEXRGB(0xE0E0FF),	TEXT("EPGContentSports"),					TEXT("EPG�ԑg�\ �X�|�[�c�ԑg")},
	{HEXRGB(0xFFE0F0),	TEXT("EPGContentInformation"),				TEXT("EPG�ԑg�\ ���ԑg")},
	{HEXRGB(0xFFE0E0),	TEXT("EPGContentDrama"),					TEXT("EPG�ԑg�\ �h���}")},
	{HEXRGB(0xE0FFE0),	TEXT("EPGContentMusic"),					TEXT("EPG�ԑg�\ ���y�ԑg")},
	{HEXRGB(0xE0FFFF),	TEXT("EPGContentVariety"),					TEXT("EPG�ԑg�\ �o���G�e�B�ԑg")},
	{HEXRGB(0xFFF0E0),	TEXT("EPGContentMovie"),					TEXT("EPG�ԑg�\ �f��")},
	{HEXRGB(0xFFE0FF),	TEXT("EPGContentAnime"),					TEXT("EPG�ԑg�\ �A�j��/���B")},
	{HEXRGB(0xFFFFE0),	TEXT("EPGContentDocumentary"),				TEXT("EPG�ԑg�\ �h�L�������^���[/���{�ԑg")},
	{HEXRGB(0xFFF0E0),	TEXT("EPGContentTheater"),					TEXT("EPG�ԑg�\ ����/����")},
	{HEXRGB(0xE0F0FF),	TEXT("EPGContentEducation"),				TEXT("EPG�ԑg�\ �/����ԑg")},
	{HEXRGB(0xE0F0FF),	TEXT("EPGContentWelfare"),					TEXT("EPG�ԑg�\ �����ԑg")},
	{HEXRGB(0xF0F0F0),	TEXT("EPGContentOther"),					TEXT("EPG�ԑg�\ ���̑��̔ԑg")},
};

const CColorScheme::GradientInfo CColorScheme::m_GradientInfoList[NUM_GRADIENTS] = {
	{TEXT("StatusBackGradient"),						Theme::DIRECTION_VERT,	false,
		COLOR_STATUSBACK1,						COLOR_STATUSBACK2},
	{TEXT("StatusBottomItemBackGradient"),				Theme::DIRECTION_VERT,	false,
		COLOR_STATUSBOTTOMITEMBACK1,			COLOR_STATUSBOTTOMITEMBACK2},
	{TEXT("StatusHighlightBackGradient"),				Theme::DIRECTION_VERT,	true,
		COLOR_STATUSHIGHLIGHTBACK1,				COLOR_STATUSHIGHLIGHTBACK2},
	{TEXT("StatusEventProgressBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_STATUSEVENTPROGRESSBACK1,			COLOR_STATUSEVENTPROGRESSBACK2},
	{TEXT("StatusEventProgressElapsedGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_STATUSEVENTPROGRESSELAPSED1,		COLOR_STATUSEVENTPROGRESSELAPSED2},
	{TEXT("PanelTabBackGradient"),						Theme::DIRECTION_VERT,	true,
		COLOR_PANELTABBACK1,					COLOR_PANELTABBACK2},
	{TEXT("PanelCurTabBackGradient"),					Theme::DIRECTION_VERT,	true,
		COLOR_PANELCURTABBACK1,					COLOR_PANELCURTABBACK2},
	{TEXT("PanelTabMarginGradient"),					Theme::DIRECTION_VERT,	false,
		COLOR_PANELTABMARGIN1,					COLOR_PANELTABMARGIN2},
	{TEXT("PanelTitleBackGradient"),					Theme::DIRECTION_VERT,	true,
		COLOR_PANELTITLEBACK1,					COLOR_PANELTITLEBACK2},
	{TEXT("InformationPanelButtonBackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_INFORMATIONPANEL_BUTTONBACK1,		COLOR_INFORMATIONPANEL_BUTTONBACK2},
	{TEXT("InformationPanelHotButtonBackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_INFORMATIONPANEL_HOTBUTTONBACK1,	COLOR_INFORMATIONPANEL_HOTBUTTONBACK2},
	{TEXT("ProgramListChannelBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_CHANNELBACK1,	COLOR_PROGRAMLISTPANEL_CHANNELBACK2},
	{TEXT("ProgramListCurChannelBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_CURCHANNELBACK1,	COLOR_PROGRAMLISTPANEL_CURCHANNELBACK2},
	{TEXT("ProgramListChannelButtonBackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBACK1,	COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBACK2},
	{TEXT("ProgramListChannelButtonHotBackGradient"),	Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK1,	COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK2},
	{TEXT("ProgramListBackGradient"),					Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_EVENTBACK1,		COLOR_PROGRAMLISTPANEL_EVENTBACK2},
	{TEXT("ProgramListCurBackGradient"),				Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_CUREVENTBACK1,	COLOR_PROGRAMLISTPANEL_CUREVENTBACK2},
	{TEXT("ProgramListTitleBackGradient"),				Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_TITLEBACK1,		COLOR_PROGRAMLISTPANEL_TITLEBACK2},
	{TEXT("ProgramListCurTitleBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMLISTPANEL_CURTITLEBACK1,	COLOR_PROGRAMLISTPANEL_CURTITLEBACK2},
	{TEXT("ChannelPanelChannelNameBackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_CHANNELNAMEBACK1,	COLOR_CHANNELPANEL_CHANNELNAMEBACK2},
	{TEXT("ChannelPanelCurChannelNameBackGradient"),	Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_CURCHANNELNAMEBACK1,	COLOR_CHANNELPANEL_CURCHANNELNAMEBACK2},
	{TEXT("ChannelPanelEventNameBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_EVENTNAME1BACK1,		COLOR_CHANNELPANEL_EVENTNAME1BACK2},
	{TEXT("ChannelPanelEventName2BackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_EVENTNAME2BACK1,		COLOR_CHANNELPANEL_EVENTNAME2BACK2},
	{TEXT("ChannelPanelCurEventNameBackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_CUREVENTNAME1BACK1,	COLOR_CHANNELPANEL_CUREVENTNAME1BACK2},
	{TEXT("ChannelPanelCurEventName2BackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_CUREVENTNAME2BACK1,	COLOR_CHANNELPANEL_CUREVENTNAME2BACK2},
	{TEXT("ChannelPanelFeaturedMarkGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_FEATUREDMARK1,		COLOR_CHANNELPANEL_FEATUREDMARK2},
	{TEXT("ChannelPanelProgressGradient"),				Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_PROGRESS1,			COLOR_CHANNELPANEL_PROGRESS2},
	{TEXT("ChannelPanelCurProgressGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_CHANNELPANEL_CURPROGRESS1,		COLOR_CHANNELPANEL_CURPROGRESS2},
	{TEXT("ControlPanelBackGradient"),					Theme::DIRECTION_VERT,	true,
		COLOR_CONTROLPANELBACK1,				COLOR_CONTROLPANELBACK2},
	{TEXT("ControlPanelHighlightBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_CONTROLPANELHIGHLIGHTBACK1,		COLOR_CONTROLPANELHIGHLIGHTBACK2},
	{TEXT("ControlPanelCheckedBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_CONTROLPANELCHECKEDBACK1,			COLOR_CONTROLPANELCHECKEDBACK2},
	{TEXT("TitleBarBackGradient"),						Theme::DIRECTION_VERT,	true,
		COLOR_TITLEBARBACK1,					COLOR_TITLEBARBACK2},
	{TEXT("TitleBarIconBackGradient"),					Theme::DIRECTION_VERT,	true,
		COLOR_TITLEBARICONBACK1,				COLOR_TITLEBARICONBACK2},
	{TEXT("TitleBarHighlightBackGradient"),				Theme::DIRECTION_VERT,	true,
		COLOR_TITLEBARHIGHLIGHTBACK1,			COLOR_TITLEBARHIGHLIGHTBACK2},
	{TEXT("SideBarBackGradient"),						Theme::DIRECTION_HORZ,	true,
		COLOR_SIDEBARBACK1,						COLOR_SIDEBARBACK2},
	{TEXT("SideBarHighlightBackGradient"),				Theme::DIRECTION_HORZ,	true,
		COLOR_SIDEBARHIGHLIGHTBACK1,			COLOR_SIDEBARHIGHLIGHTBACK2},
	{TEXT("SideBarCheckBackGradient"),					Theme::DIRECTION_HORZ,	true,
		COLOR_SIDEBARCHECKBACK1,				COLOR_SIDEBARCHECKBACK2},
	{TEXT("NotificationBarBackGradient"),				Theme::DIRECTION_VERT,	true,
		COLOR_NOTIFICATIONBARBACK1,				COLOR_NOTIFICATIONBARBACK2},
	{TEXT("ProgramGuideFeaturedMarkGradient"),			Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_FEATUREDMARK1,		COLOR_PROGRAMGUIDE_FEATUREDMARK2},
	{TEXT("ProgramGuideChannelBackGradient"),			Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMGUIDE_CHANNELBACK1,		COLOR_PROGRAMGUIDE_CHANNELBACK2},
	{TEXT("ProgramGuideCurChannelBackGradient"),		Theme::DIRECTION_VERT,	true,
		COLOR_PROGRAMGUIDE_CURCHANNELBACK1,		COLOR_PROGRAMGUIDE_CURCHANNELBACK2},
	{TEXT("ProgramGuideTimeBackGradient"),				Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK1,			COLOR_PROGRAMGUIDE_TIMEBACK2},
	{TEXT("ProgramGuideTime0To2BackGradient"),			Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_0TO2_1,		COLOR_PROGRAMGUIDE_TIMEBACK_0TO2_2},
	{TEXT("ProgramGuideTime3To5BackGradient"),			Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_3TO5_1,		COLOR_PROGRAMGUIDE_TIMEBACK_3TO5_2},
	{TEXT("ProgramGuideTime6To8BackGradient"),			Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_6TO8_1,		COLOR_PROGRAMGUIDE_TIMEBACK_6TO8_2},
	{TEXT("ProgramGuideTime9To11BackGradient"),			Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_9TO11_1,	COLOR_PROGRAMGUIDE_TIMEBACK_9TO11_2},
	{TEXT("ProgramGuideTime12To14BackGradient"),		Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_12TO14_1,	COLOR_PROGRAMGUIDE_TIMEBACK_12TO14_2},
	{TEXT("ProgramGuideTime15To17BackGradient"),		Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_15TO17_1,	COLOR_PROGRAMGUIDE_TIMEBACK_15TO17_2},
	{TEXT("ProgramGuideTime18To20BackGradient"),		Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_18TO20_1,	COLOR_PROGRAMGUIDE_TIMEBACK_18TO20_2},
	{TEXT("ProgramGuideTime21To23BackGradient"),		Theme::DIRECTION_HORZ,	true,
		COLOR_PROGRAMGUIDE_TIMEBACK_21TO23_1,	COLOR_PROGRAMGUIDE_TIMEBACK_21TO23_2},
};

const CColorScheme::BorderInfo CColorScheme::m_BorderInfoList[NUM_BORDERS] = {
	{TEXT("ScreenBorder"),						Theme::BORDER_SUNKEN,
		COLOR_SCREENBORDER,							true},
	{TEXT("WindowFrameBorder"),					Theme::BORDER_NONE,
		COLOR_WINDOWFRAMEBORDER,					false},
	{TEXT("WindowActiveFrameBorder"),			Theme::BORDER_NONE,
		COLOR_WINDOWACTIVEFRAMEBORDER,				false},
	{TEXT("StatusBorder"),						Theme::BORDER_RAISED,
		COLOR_STATUSBORDER,							false},
	{TEXT("StatusItemBorder"),					Theme::BORDER_NONE,
		COLOR_STATUSITEMBORDER,						false},
	{TEXT("StatusBottomItemBorder"),			Theme::BORDER_NONE,
		COLOR_STATUSBOTTOMITEMBORDER,				false},
	{TEXT("StatusHighlightBorder"),				Theme::BORDER_NONE,
		COLOR_STATUSHIGHLIGHTBORDER,				false},
	{TEXT("StatusEventProgressBorder"),			Theme::BORDER_NONE,
		COLOR_STATUSEVENTPROGRESSBORDER,			false},
	{TEXT("StatusEventProgressElapsedBorder"),	Theme::BORDER_NONE,
		COLOR_STATUSEVENTPROGRESSELAPSEDBORDER,		false},
	{TEXT("TitleBarBorder"),					Theme::BORDER_RAISED,
		COLOR_TITLEBARBORDER,						true},
	{TEXT("TitleBarCaptionBorder"),				Theme::BORDER_NONE,
		COLOR_TITLEBARTEXTBORDER,					false},
	{TEXT("TitleBarIconBorder"),				Theme::BORDER_NONE,
		COLOR_TITLEBARICONBORDER,					false},
	{TEXT("TitleBarHighlightBorder"),			Theme::BORDER_NONE,
		COLOR_TITLEBARHIGHLIGHTBORDER,				false},
	{TEXT("SideBarBorder"),						Theme::BORDER_RAISED,
		COLOR_SIDEBARBORDER,						true},
	{TEXT("SideBarItemBorder"),					Theme::BORDER_NONE,
		COLOR_SIDEBARITEMBORDER,					false},
	{TEXT("SideBarHighlightBorder"),			Theme::BORDER_NONE,
		COLOR_SIDEBARHIGHLIGHTBORDER,				false},
	{TEXT("SideBarCheckBorder"),				Theme::BORDER_SUNKEN,
		COLOR_SIDEBARCHECKBORDER,					false},
	{TEXT("ProgramGuideStatusBorder"),			Theme::BORDER_SUNKEN,
		COLOR_STATUSBORDER,							true},
	{TEXT("PanelTabBorder"),					Theme::BORDER_SOLID,
		COLOR_PANELTABBORDER,						false},
	{TEXT("PanelCurTabBorder"),					Theme::BORDER_SOLID,
		COLOR_PANELCURTABBORDER,					false},
	{TEXT("PanelTabMarginBorder"),				Theme::BORDER_NONE,
		COLOR_PANELTABMARGINBORDER,					false},
	{TEXT("PanelTitleBorder"),					Theme::BORDER_RAISED,
		COLOR_PANELTITLEBORDER,						false},
	{TEXT("InformationPanelEventInfoBorder"),	Theme::BORDER_NONE,
		COLOR_INFORMATIONPANEL_EVENTINFOBORDER,		false},
	{TEXT("InformationPanelButtonBorder"),		Theme::BORDER_NONE,
		COLOR_INFORMATIONPANEL_BUTTONBORDER,		false},
	{TEXT("InformationPanelHotButtonBorder"),	Theme::BORDER_NONE,
		COLOR_INFORMATIONPANEL_HOTBUTTONBORDER,		false},
	{TEXT("ProgramListPanelChannelBorder"),		Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_CHANNELBORDER,		false},
	{TEXT("ProgramListPanelCurChannelBorder"),	Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_CURCHANNELBORDER,	false},
	{TEXT("ProgramListPanelChannelButtonBorder"),	Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_CHANNELBUTTONBORDER,	false},
	{TEXT("ProgramListPanelChannelButtonHotBorder"),	Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTBORDER,	false},
	{TEXT("ProgramListPanelEventBorder"),		Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_EVENTBORDER,			false},
	{TEXT("ProgramListPanelCurEventBorder"),	Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_CUREVENTBORDER,		false},
	{TEXT("ProgramListPanelTitleBorder"),		Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_TITLEBORDER,			false},
	{TEXT("ProgramListPanelCurTitleBorder"),	Theme::BORDER_NONE,
		COLOR_PROGRAMLISTPANEL_CURTITLEBORDER,		false},
	{TEXT("ChannelPanelChannelNameBorder"),		Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_CHANNELNAMEBORDER,		false},
	{TEXT("ChannelPanelCurChannelNameBorder"),	Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_CURCHANNELNAMEBORDER,	false},
	{TEXT("ChannelPanelEventNameBorder"),		Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_EVENTNAME1BORDER,		false},
	{TEXT("ChannelPanelEventName2Border"),		Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_EVENTNAME2BORDER,		false},
	{TEXT("ChannelPanelCurEventNameBorder"),	Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_CUREVENTNAME1BORDER,		false},
	{TEXT("ChannelPanelCurEventName2Border"),	Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_CUREVENTNAME2BORDER,		false},
	{TEXT("ChannelPanelFeaturedMarkBorder"),	Theme::BORDER_SOLID,
		COLOR_CHANNELPANEL_FEATUREDMARKBORDER,		false},
	{TEXT("ChannelPanelProgressBorder"),		Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_PROGRESSBORDER,			false},
	{TEXT("ChannelPanelCurProgressBorder"),		Theme::BORDER_NONE,
		COLOR_CHANNELPANEL_CURPROGRESSBORDER,		false},
	{TEXT("ControlPanelItemBorder"),			Theme::BORDER_NONE,
		COLOR_CONTROLPANELITEMBORDER,				false},
	{TEXT("ControlPanelHighlightBorder"),		Theme::BORDER_NONE,
		COLOR_CONTROLPANELHIGHLIGHTBORDER,			false},
	{TEXT("ControlPanelCheckedBorder"),			Theme::BORDER_NONE,
		COLOR_CONTROLPANELCHECKEDBORDER,			false},
	{TEXT("ProgramGuideFeaturedMarkBorder"),	Theme::BORDER_SOLID,
		COLOR_PROGRAMGUIDE_FEATUREDMARKBORDER,		false},
};

const Theme::BorderType CColorScheme::m_CustomDefaultBorderList[NUM_BORDERS] = {
	Theme::BORDER_SOLID,
	Theme::BORDER_NONE,
	Theme::BORDER_NONE,
	Theme::BORDER_RAISED,
	Theme::BORDER_NONE,
	Theme::BORDER_NONE,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_NONE,
	Theme::BORDER_NONE,
	Theme::BORDER_RAISED,
	Theme::BORDER_NONE,
	Theme::BORDER_NONE,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_RAISED,
	Theme::BORDER_NONE,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_NONE,
	Theme::BORDER_SOLID,
	Theme::BORDER_NONE,
	Theme::BORDER_RAISED,
	Theme::BORDER_NONE,
	Theme::BORDER_NONE,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_NONE,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_SOLID,
	Theme::BORDER_NONE,
	Theme::BORDER_NONE,
	Theme::BORDER_NONE,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_SUNKEN,
	Theme::BORDER_SOLID,
};


CColorScheme::CColorScheme()
{
	SetDefault();
	::ZeroMemory(m_LoadedFlags,sizeof(m_LoadedFlags));
}


CColorScheme::CColorScheme(const CColorScheme &ColorScheme)
{
	*this=ColorScheme;
}


CColorScheme::~CColorScheme()
{
}


CColorScheme &CColorScheme::operator=(const CColorScheme &ColorScheme)
{
	if (&ColorScheme!=this) {
		::CopyMemory(m_ColorList,ColorScheme.m_ColorList,sizeof(m_ColorList));
		::CopyMemory(m_GradientList,ColorScheme.m_GradientList,sizeof(m_GradientList));
		::CopyMemory(m_BorderList,ColorScheme.m_BorderList,sizeof(m_BorderList));
		m_Name=ColorScheme.m_Name;
		m_FileName=ColorScheme.m_FileName;
		::CopyMemory(m_LoadedFlags,ColorScheme.m_LoadedFlags,sizeof(m_LoadedFlags));
	}
	return *this;
}


COLORREF CColorScheme::GetColor(int Type) const
{
	if (Type<0 || Type>=NUM_COLORS)
		return CLR_INVALID;
	return m_ColorList[Type];
}


COLORREF CColorScheme::GetColor(LPCTSTR pszText) const
{
	for (int i=0;i<NUM_COLORS;i++) {
		if (::lstrcmpi(m_ColorInfoList[i].pszText,pszText)==0)
			return m_ColorList[i];
	}
	return CLR_INVALID;
}


bool CColorScheme::SetColor(int Type,COLORREF Color)
{
	if (Type<0 || Type>=NUM_COLORS)
		return false;
	m_ColorList[Type]=Color;
	return true;
}


Theme::GradientType CColorScheme::GetGradientType(int Gradient) const
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return Theme::GRADIENT_NORMAL;
	return m_GradientList[Gradient].Type;
}


Theme::GradientType CColorScheme::GetGradientType(LPCTSTR pszText) const
{
	for (int i=0;i<NUM_GRADIENTS;i++) {
		if (::lstrcmpi(m_GradientInfoList[i].pszText,pszText)==0)
			return m_GradientList[i].Type;
	}
	return Theme::GRADIENT_NORMAL;
}


bool CColorScheme::SetGradientStyle(int Gradient,const GradientStyle &Style)
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return false;
	m_GradientList[Gradient].Type=Style.Type;
	m_GradientList[Gradient].Direction=Style.Direction;
	return true;
}


bool CColorScheme::GetGradientStyle(int Gradient,GradientStyle *pStyle) const
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return false;
	*pStyle=m_GradientList[Gradient];
	return true;
}


bool CColorScheme::GetGradientStyle(int Gradient,Theme::GradientStyle *pStyle) const
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return false;
	pStyle->Type=m_GradientList[Gradient].Type;
	pStyle->Direction=m_GradientList[Gradient].Direction;
	pStyle->Color1=m_ColorList[m_GradientInfoList[Gradient].Color1];
	pStyle->Color2=m_ColorList[m_GradientInfoList[Gradient].Color2];
	return true;
}


Theme::BorderType CColorScheme::GetBorderType(int Border) const
{
	if (Border<0 || Border>=NUM_BORDERS)
		return Theme::BORDER_NONE;
	return m_BorderList[Border];
}


bool CColorScheme::SetBorderType(int Border,Theme::BorderType Type)
{
	if (Border<0 || Border>=NUM_BORDERS
			|| Type<Theme::BORDER_NONE || Type>Theme::BORDER_RAISED)
		return false;
	m_BorderList[Border]=Type;
	return true;
}


bool CColorScheme::GetBorderStyle(int Border,Theme::BorderStyle *pStyle) const
{
	if (Border<0 || Border>=NUM_BORDERS)
		return false;
	pStyle->Type=m_BorderList[Border];
	pStyle->Color=m_ColorList[m_BorderInfoList[Border].Color];
	return true;
}


void CColorScheme::SetName(LPCTSTR pszName)
{
	TVTest::StringUtility::Assign(m_Name,pszName);
}


bool CColorScheme::Load(CSettings &Settings)
{
	TCHAR szText[MAX_COLORSCHEME_NAME];

	if (!Settings.SetSection(TEXT("ColorScheme")))
		return false;
	if (Settings.Read(TEXT("Name"),szText,lengthof(szText)))
		SetName(szText);
	::ZeroMemory(m_LoadedFlags,sizeof(m_LoadedFlags));
	for (int i=0;i<NUM_COLORS;i++) {
		if (Settings.ReadColor(m_ColorInfoList[i].pszText,&m_ColorList[i]))
			SetLoadedFlag(i);
	}

	for (int i=0;i<NUM_GRADIENTS;i++) {
		if (IsLoaded(m_GradientInfoList[i].Color1)
				&& !IsLoaded(m_GradientInfoList[i].Color2)) {
			m_ColorList[m_GradientInfoList[i].Color2]=m_ColorList[m_GradientInfoList[i].Color1];
			SetLoadedFlag(m_GradientInfoList[i].Color2);
			m_GradientList[i].Type=Theme::GRADIENT_NORMAL;
		}
	}

	static const struct {
		int To,From;
	} ColorMap[] = {
	//	{COLOR_STATUSBORDER,							COLOR_STATUSBACK1},
		{COLOR_STATUSBOTTOMITEMBACK1,					COLOR_STATUSBACK2},
		{COLOR_STATUSBOTTOMITEMBACK2,					COLOR_STATUSBOTTOMITEMBACK1},
		{COLOR_STATUSBOTTOMITEMTEXT,					COLOR_STATUSTEXT},
		{COLOR_STATUSBOTTOMITEMBORDER,					COLOR_STATUSBOTTOMITEMBACK1},
		{COLOR_STATUSEVENTPROGRESSBORDER,				COLOR_STATUSEVENTPROGRESSBACK1},
		{COLOR_STATUSEVENTPROGRESSELAPSEDBORDER,		COLOR_STATUSEVENTPROGRESSELAPSED1},
		{COLOR_WINDOWFRAMEBORDER,						COLOR_WINDOWFRAMEBACK},
		{COLOR_WINDOWACTIVEFRAMEBACK,					COLOR_WINDOWFRAMEBACK},
		{COLOR_WINDOWACTIVEFRAMEBORDER,					COLOR_WINDOWACTIVEFRAMEBACK},
		{COLOR_INFORMATIONPANEL_EVENTINFOBORDER,		COLOR_PROGRAMINFOBACK},
		{COLOR_INFORMATIONPANEL_BUTTONBACK1,			COLOR_PANELBACK},
		{COLOR_INFORMATIONPANEL_BUTTONBACK2,			COLOR_INFORMATIONPANEL_BUTTONBACK1},
		{COLOR_INFORMATIONPANEL_BUTTONTEXT,				COLOR_PANELTEXT},
		{COLOR_INFORMATIONPANEL_HOTBUTTONBACK1,			COLOR_PANELBACK},
		{COLOR_INFORMATIONPANEL_HOTBUTTONBACK2,			COLOR_INFORMATIONPANEL_HOTBUTTONBACK1},
		{COLOR_INFORMATIONPANEL_HOTBUTTONTEXT,			COLOR_PANELTEXT},
		{COLOR_PROGRAMLISTPANEL_CUREVENTTEXT,			COLOR_PROGRAMLISTPANEL_EVENTTEXT},
		{COLOR_PROGRAMLISTPANEL_CURTITLETEXT,			COLOR_PROGRAMLISTPANEL_TITLETEXT},
		{COLOR_PANELTABLINE,							COLOR_PANELTABBORDER},
	//	{COLOR_PANELTITLEBORDER,						COLOR_PANELTITLEBACK1},
		{COLOR_CHANNELPANEL_CURCHANNELNAMETEXT,			COLOR_CHANNELPANEL_CHANNELNAMETEXT},
		{COLOR_CHANNELPANEL_EVENTNAME2TEXT,				COLOR_CHANNELPANEL_EVENTNAME1TEXT},
		{COLOR_CHANNELPANEL_EVENTNAME2BORDER,			COLOR_CHANNELPANEL_EVENTNAME1BORDER},
		{COLOR_CHANNELPANEL_CUREVENTNAME1TEXT,			COLOR_CHANNELPANEL_EVENTNAME1TEXT},
		{COLOR_CHANNELPANEL_CUREVENTNAME1BORDER,		COLOR_CHANNELPANEL_EVENTNAME1BORDER},
		{COLOR_CHANNELPANEL_CUREVENTNAME2TEXT,			COLOR_CHANNELPANEL_CUREVENTNAME1TEXT},
		{COLOR_CHANNELPANEL_CUREVENTNAME2BORDER,		COLOR_CHANNELPANEL_CUREVENTNAME1BORDER},
		{COLOR_CHANNELPANEL_PROGRESSBORDER,				COLOR_CHANNELPANEL_PROGRESS1},
		{COLOR_CHANNELPANEL_CURPROGRESSBORDER,			COLOR_CHANNELPANEL_CURPROGRESS1},
		{COLOR_PROGRAMLISTPANEL_CHANNELTEXT,			COLOR_CHANNELPANEL_CHANNELNAMETEXT},
		{COLOR_PROGRAMLISTPANEL_CURCHANNELTEXT,			COLOR_CHANNELPANEL_CURCHANNELNAMETEXT},
		{COLOR_PROGRAMLISTPANEL_CHANNELBUTTONTEXT,		COLOR_PROGRAMLISTPANEL_CHANNELTEXT},
		{COLOR_PROGRAMLISTPANEL_CHANNELBUTTONHOTTEXT,	COLOR_PROGRAMLISTPANEL_CURCHANNELTEXT},
		{COLOR_CONTROLPANELBACK1,						COLOR_PANELBACK},
		{COLOR_CONTROLPANELBACK2,						COLOR_PANELBACK},
		{COLOR_CONTROLPANELTEXT,						COLOR_PANELTEXT},
		{COLOR_CONTROLPANELCHECKEDTEXT,					COLOR_CONTROLPANELHIGHLIGHTTEXT},
		{COLOR_CONTROLPANELMARGIN,						COLOR_PANELBACK},
		{COLOR_CAPTIONPANELBACK,						COLOR_PROGRAMINFOBACK},
		{COLOR_CAPTIONPANELTEXT,						COLOR_PROGRAMINFOTEXT},
		{COLOR_TITLEBARTEXT,							COLOR_STATUSTEXT},
		{COLOR_TITLEBARICON,							COLOR_TITLEBARTEXT},
		{COLOR_TITLEBARHIGHLIGHTICON,					COLOR_STATUSHIGHLIGHTTEXT},
	//	{COLOR_TITLEBARBORDER,							COLOR_TITLEBARBACK1},
		{COLOR_SIDEBARICON,								COLOR_STATUSTEXT},
		{COLOR_SIDEBARHIGHLIGHTICON,					COLOR_STATUSHIGHLIGHTTEXT},
		{COLOR_SIDEBARCHECKICON,						COLOR_SIDEBARICON},
	//	{COLOR_SIDEBARCHECKBORDER,						COLOR_SIDEBARCHECKBACK2},
	//	{COLOR_SIDEBARBORDER,							COLOR_SIDEBARBACK1},
		{COLOR_PROGRAMGUIDE_CURCHANNELTEXT,				COLOR_PROGRAMGUIDE_CHANNELTEXT},
		{COLOR_PROGRAMGUIDE_TIMELINE,					COLOR_PROGRAMGUIDE_TIMETEXT},
	};

	for (int i=0;i<lengthof(ColorMap);i++) {
		const int To=ColorMap[i].To;
		if (!IsLoaded(To) && IsLoaded(ColorMap[i].From)) {
			m_ColorList[To]=m_ColorList[ColorMap[i].From];
			SetLoadedFlag(To);
		}
	}

	static const struct {
		int To,From1,From2;
	} MixMap[] = {
		{COLOR_STATUSBORDER,				COLOR_STATUSBACK1,			COLOR_STATUSBACK2},
		{COLOR_PANELTITLEBORDER,			COLOR_PANELTITLEBACK1,		COLOR_PANELTITLEBACK2},
		{COLOR_TITLEBARBORDER,				COLOR_TITLEBARBACK1,		COLOR_TITLEBARBACK2},
		{COLOR_SIDEBARCHECKBORDER,			COLOR_SIDEBARCHECKBACK1,	COLOR_SIDEBARCHECKBACK2},
		{COLOR_SIDEBARBORDER,				COLOR_SIDEBARBACK1,			COLOR_SIDEBARBACK2},
	};

	for (int i=0;i<lengthof(MixMap);i++) {
		const int To=MixMap[i].To;
		if (!IsLoaded(To) && IsLoaded(MixMap[i].From1) && IsLoaded(MixMap[i].From2)) {
			m_ColorList[To]=MixColor(m_ColorList[MixMap[i].From1],m_ColorList[MixMap[i].From2]);
			SetLoadedFlag(To);
		}
	}

	for (int i=0;i<NUM_GRADIENTS;i++) {
		if (Settings.Read(m_GradientInfoList[i].pszText,szText,lengthof(szText))) {
			if (szText[0]=='\0' || ::lstrcmpi(szText,TEXT("normal"))==0)
				m_GradientList[i].Type=Theme::GRADIENT_NORMAL;
			else if (::lstrcmpi(szText,TEXT("glossy"))==0)
				m_GradientList[i].Type=Theme::GRADIENT_GLOSSY;
			else if (::lstrcmpi(szText,TEXT("interlaced"))==0)
				m_GradientList[i].Type=Theme::GRADIENT_INTERLACED;
		} else {
			switch (i) {
			case GRADIENT_TITLEBARICON:
				m_GradientList[i].Type=m_GradientList[GRADIENT_TITLEBARBACK].Type;
				break;
			case GRADIENT_SIDEBARCHECKBACK:
				m_GradientList[i].Type=m_GradientList[GRADIENT_SIDEBARBACK].Type;
				break;
			}
		}

		TCHAR szName[128];
		::wsprintf(szName,TEXT("%sDirection"),m_GradientInfoList[i].pszText);
		m_GradientList[i].Direction=m_GradientInfoList[i].Direction;
		if (Settings.Read(szName,szText,lengthof(szText))) {
			for (int j=0;j<lengthof(GradientDirectionList);j++) {
				if (::lstrcmpi(szText,GradientDirectionList[j])==0) {
					m_GradientList[i].Direction=(Theme::GradientDirection)j;
					break;
				}
			}
		} else {
			switch (i) {
			case GRADIENT_TITLEBARICON:
				m_GradientList[i].Direction=m_GradientList[GRADIENT_TITLEBARBACK].Direction;
				break;
			case GRADIENT_SIDEBARCHECKBACK:
				m_GradientList[i].Direction=m_GradientList[GRADIENT_SIDEBARBACK].Direction;
				break;
			}
		}
	}

	static const struct {
		int To,From;
	} GradientMap[] = {
		{GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK,			GRADIENT_CHANNELPANEL_CHANNELNAMEBACK},
		{GRADIENT_CHANNELPANEL_EVENTNAMEBACK2,				GRADIENT_CHANNELPANEL_EVENTNAMEBACK1},
		{GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK1,			GRADIENT_CHANNELPANEL_EVENTNAMEBACK1},
		{GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK2,			GRADIENT_CHANNELPANEL_CUREVENTNAMEBACK1},
		{GRADIENT_CHANNELPANEL_PROGRESS,					GRADIENT_STATUSEVENTPROGRESSELAPSED},
		{GRADIENT_CHANNELPANEL_CURPROGRESS,					GRADIENT_CHANNELPANEL_PROGRESS},
		{GRADIENT_PROGRAMLISTPANEL_CHANNELBACK,				GRADIENT_CHANNELPANEL_CHANNELNAMEBACK},
		{GRADIENT_PROGRAMLISTPANEL_CURCHANNELBACK,			GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK},
		{GRADIENT_PROGRAMLISTPANEL_CHANNELBUTTONBACK,		GRADIENT_CHANNELPANEL_CHANNELNAMEBACK},
	//	{GRADIENT_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK,	GRADIENT_CHANNELPANEL_CURCHANNELNAMEBACK},
		{GRADIENT_PROGRAMLISTPANEL_CHANNELBUTTONHOTBACK,	GRADIENT_CONTROLPANELHIGHLIGHTBACK},
		{GRADIENT_PROGRAMLISTPANEL_CUREVENTBACK,			GRADIENT_PROGRAMLISTPANEL_EVENTBACK},
		{GRADIENT_PROGRAMLISTPANEL_CURTITLEBACK,			GRADIENT_PROGRAMLISTPANEL_TITLEBACK},
		{GRADIENT_CONTROLPANELCHECKEDBACK,					GRADIENT_CONTROLPANELHIGHLIGHTBACK},
		{GRADIENT_TITLEBARBACK,								GRADIENT_STATUSBACK},
		{GRADIENT_TITLEBARICON,								GRADIENT_TITLEBARBACK},
		{GRADIENT_TITLEBARHIGHLIGHTBACK,					GRADIENT_STATUSHIGHLIGHTBACK},
		{GRADIENT_SIDEBARBACK,								GRADIENT_STATUSBACK},
		{GRADIENT_SIDEBARHIGHLIGHTBACK,						GRADIENT_STATUSHIGHLIGHTBACK},
		{GRADIENT_SIDEBARCHECKBACK,							GRADIENT_SIDEBARBACK},
		{GRADIENT_PROGRAMGUIDECURCHANNELBACK,				GRADIENT_PROGRAMGUIDECHANNELBACK},
	};

	for (int i=0;i<lengthof(GradientMap);i++) {
		const int To=GradientMap[i].To,From=GradientMap[i].From;
		if (!IsLoaded(m_GradientInfoList[To].Color1)
				&& IsLoaded(m_GradientInfoList[From].Color1)) {
			m_ColorList[m_GradientInfoList[To].Color1]=m_ColorList[m_GradientInfoList[From].Color1];
			m_ColorList[m_GradientInfoList[To].Color2]=m_ColorList[m_GradientInfoList[From].Color2];
			SetLoadedFlag(m_GradientInfoList[To].Color1);
			SetLoadedFlag(m_GradientInfoList[To].Color2);
			m_GradientList[To]=m_GradientList[From];
		}
	}

	bool BorderLoaded[NUM_BORDERS];

	for (int i=0;i<NUM_BORDERS;i++) {
		m_BorderList[i]=m_BorderInfoList[i].DefaultType;
		BorderLoaded[i]=false;
	}
	if (Settings.SetSection(TEXT("Style"))) {
		for (int i=0;i<NUM_BORDERS;i++) {
			if (Settings.Read(m_BorderInfoList[i].pszText,szText,lengthof(szText))) {
				bool fLoaded=true;
				if (::lstrcmpi(szText,TEXT("none"))==0) {
					if (!m_BorderInfoList[i].fAlways)
						m_BorderList[i]=Theme::BORDER_NONE;
				} else if (::lstrcmpi(szText,TEXT("solid"))==0)
					m_BorderList[i]=Theme::BORDER_SOLID;
				else if (::lstrcmpi(szText,TEXT("sunken"))==0)
					m_BorderList[i]=Theme::BORDER_SUNKEN;
				else if (::lstrcmpi(szText,TEXT("raised"))==0)
					m_BorderList[i]=Theme::BORDER_RAISED;
				else
					fLoaded=false;
				BorderLoaded[i]=fLoaded;
			}
		}
	}

	static const struct {
		int To,From;
	} BorderMap[] = {
		{BORDER_PROGRAMLISTPANEL_CHANNEL,			BORDER_CHANNELPANEL_CHANNELNAME},
		{BORDER_PROGRAMLISTPANEL_CURCHANNEL,		BORDER_CHANNELPANEL_CURCHANNELNAME},
		{BORDER_PROGRAMLISTPANEL_CHANNELBUTTONHOT,	BORDER_CONTROLPANELHIGHLIGHTITEM},
		{BORDER_SIDEBARITEM,						BORDER_STATUSITEM},
		{BORDER_SIDEBARHIGHLIGHT,					BORDER_STATUSHIGHLIGHT},
	};

	for (int i=0;i<lengthof(BorderMap);i++) {
		const int To=BorderMap[i].To;
		const int From=BorderMap[i].From;
		if (!BorderLoaded[To] && BorderLoaded[From]) {
			m_BorderList[To]=m_BorderList[From];
		}
		const int ColorTo=m_BorderInfoList[To].Color;
		const int ColorFrom=m_BorderInfoList[From].Color;
		if (!IsLoaded(ColorTo) && IsLoaded(ColorFrom)) {
			m_ColorList[ColorTo]=m_ColorList[ColorFrom];
			SetLoadedFlag(ColorTo);
		}
	}

	return true;
}


bool CColorScheme::Save(CSettings &Settings,unsigned int Flags) const
{
	if (!Settings.SetSection(TEXT("ColorScheme")))
		return false;

	bool fSaveAllColors=true,fSaveGradients=true;

	if ((Flags & SAVE_NODEFAULT)!=0) {
		/*
			SAVE_NODEFAULT ���w�肳��Ă���ꍇ�A�f�t�H���g����ύX����Ă��Ȃ����̂ݐݒ��ۑ�����B
			Load�̏��������G�Ȃ̂ŁA�P���ɕύX����Ă�����̂̂ݕۑ�����΂����Ƃ�����ł͂Ȃ����߁A
			�f�t�H���g����ύX����Ă���ꍇ�͑S�Ă̐ݒ��ۑ�����B
		*/
		CColorScheme DefaultColorScheme;
		int i;

		for (i=0;i<NUM_COLORS;i++) {
			if ((i<COLOR_PROGRAMGUIDE_CONTENT_FIRST || i>COLOR_PROGRAMGUIDE_CONTENT_LAST)
					&& m_ColorList[i]!=DefaultColorScheme.m_ColorList[i])
				break;
		}
		if (i==NUM_COLORS)
			fSaveAllColors=false;

		for (i=0;i<NUM_GRADIENTS;i++) {
			if (m_GradientList[i]!=DefaultColorScheme.m_GradientList[i])
				break;
		}
		if (i==NUM_GRADIENTS)
			fSaveGradients=false;

		if (!fSaveAllColors || !fSaveGradients)
			Settings.Clear();
	}

	if ((Flags & SAVE_NONAME)==0)
		Settings.Write(TEXT("Name"),m_Name);

	for (int i=0;i<NUM_COLORS;i++) {
		if (fSaveAllColors || m_ColorList[i]!=m_ColorInfoList[i].DefaultColor)
			Settings.WriteColor(m_ColorInfoList[i].pszText,m_ColorList[i]);
	}

	if (fSaveGradients) {
		for (int i=0;i<NUM_GRADIENTS;i++) {
			static const LPCTSTR pszTypeName[] = {
				TEXT("normal"),	TEXT("glossy"), TEXT("interlaced")
			};
			TCHAR szName[128];

			Settings.Write(m_GradientInfoList[i].pszText,pszTypeName[m_GradientList[i].Type]);
			::wsprintf(szName,TEXT("%sDirection"),m_GradientInfoList[i].pszText);
			Settings.Write(szName,GradientDirectionList[m_GradientList[i].Direction]);
		}
	}

	if (Settings.SetSection(TEXT("Style"))) {
		static const LPCTSTR pszTypeName[] = {
			TEXT("none"),	TEXT("solid"),	TEXT("sunken"),	TEXT("raised")
		};

		for (int i=0;i<NUM_BORDERS;i++)
			Settings.Write(m_BorderInfoList[i].pszText,pszTypeName[m_BorderList[i]]);
	}

	return true;
}


bool CColorScheme::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (!Settings.Open(pszFileName,CSettings::OPEN_READ))
		return false;

	if (!Load(Settings))
		return false;

	SetFileName(pszFileName);

	if (m_Name.empty()) {
		TCHAR szName[MAX_COLORSCHEME_NAME];
		::lstrcpyn(szName,::PathFindFileName(pszFileName),lengthof(szName));
		::PathRemoveExtension(szName);
		SetName(szName);
	}

	return true;
}


bool CColorScheme::Save(LPCTSTR pszFileName,unsigned int Flags) const
{
	CSettings Settings;

	if (!Settings.Open(pszFileName,CSettings::OPEN_WRITE))
		return false;

	return Save(Settings,Flags);
}


bool CColorScheme::SetFileName(LPCTSTR pszFileName)
{
	TVTest::StringUtility::Assign(m_FileName,pszFileName);
	return true;
}


void CColorScheme::SetDefault()
{
	int i;

	for (i=0;i<NUM_COLORS;i++)
		m_ColorList[i]=m_ColorInfoList[i].DefaultColor;
	for (i=0;i<NUM_GRADIENTS;i++) {
		m_GradientList[i].Type=Theme::GRADIENT_NORMAL;
		m_GradientList[i].Direction=m_GradientInfoList[i].Direction;
	}
	for (i=0;i<NUM_BORDERS;i++)
		m_BorderList[i]=m_CustomDefaultBorderList[i];
}


LPCTSTR CColorScheme::GetColorName(int Type)
{
	if (Type<0 || Type>=NUM_COLORS)
		return NULL;
	return m_ColorInfoList[Type].pszName;
}


COLORREF CColorScheme::GetDefaultColor(int Type)
{
	if (Type<0 || Type>=NUM_COLORS)
		return CLR_INVALID;
	return m_ColorInfoList[Type].DefaultColor;
}


Theme::GradientType CColorScheme::GetDefaultGradientType(int Gradient)
{
	return Theme::GRADIENT_NORMAL;
}


bool CColorScheme::GetDefaultGradientStyle(int Gradient,GradientStyle *pStyle)
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return false;
	pStyle->Type=Theme::GRADIENT_NORMAL;
	pStyle->Direction=m_GradientInfoList[Gradient].Direction;
	return true;
}


bool CColorScheme::IsGradientDirectionEnabled(int Gradient)
{
	if (Gradient<0 || Gradient>=NUM_GRADIENTS)
		return false;
	return m_GradientInfoList[Gradient].fEnableDirection;
}


Theme::BorderType CColorScheme::GetDefaultBorderType(int Border)
{
	if (Border<0 || Border>=NUM_BORDERS)
		return Theme::BORDER_NONE;
	return m_BorderInfoList[Border].DefaultType;
}


bool CColorScheme::IsLoaded(int Type) const
{
	if (Type<0 || Type>=NUM_COLORS)
		return false;
	return (m_LoadedFlags[Type/32]&(1<<(Type%32)))!=0;
}


void CColorScheme::SetLoaded()
{
	::FillMemory(m_LoadedFlags,sizeof(m_LoadedFlags),0xFF);
}


int CColorScheme::GetColorGradient(int Type)
{
	for (int i=0;i<NUM_GRADIENTS;i++) {
		if (m_GradientInfoList[i].Color1==Type
				|| m_GradientInfoList[i].Color2==Type)
			return i;
	}
	return -1;
}


int CColorScheme::GetColorBorder(int Type)
{
	for (int i=0;i<NUM_BORDERS;i++) {
		if (m_BorderInfoList[i].Color==Type)
			return i;
	}
	return -1;
}


bool CColorScheme::IsBorderAlways(int Border)
{
	if (Border<0 || Border>=NUM_BORDERS)
		return false;
	return m_BorderInfoList[Border].fAlways;
}


void CColorScheme::SetLoadedFlag(int Color)
{
	m_LoadedFlags[Color/32]|=1<<(Color%32);
}




CColorSchemeList::CColorSchemeList()
{
}


CColorSchemeList::~CColorSchemeList()
{
	Clear();
}


bool CColorSchemeList::Add(CColorScheme *pColorScheme)
{
	if (pColorScheme==NULL)
		return false;
	m_List.push_back(pColorScheme);
	return true;
}


bool CColorSchemeList::Insert(int Index,CColorScheme *pColorScheme)
{
	if (Index<0)
		return false;
	if ((size_t)Index>=m_List.size())
		return Add(pColorScheme);
	auto i=m_List.begin();
	std::advance(i,Index);
	m_List.insert(i,pColorScheme);
	return true;
}


bool CColorSchemeList::Load(LPCTSTR pszDirectory)
{
	HANDLE hFind;
	WIN32_FIND_DATA wfd;
	TCHAR szFileName[MAX_PATH];

	::PathCombine(szFileName,pszDirectory,TEXT("*.httheme"));
	hFind=::FindFirstFile(szFileName,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			CColorScheme *pColorScheme;

			::PathCombine(szFileName,pszDirectory,wfd.cFileName);
			pColorScheme=new CColorScheme;
			if (pColorScheme->Load(szFileName))
				Add(pColorScheme);
			else
				delete pColorScheme;
		} while (::FindNextFile(hFind,&wfd));
		::FindClose(hFind);
	}
	return true;
}


void CColorSchemeList::Clear()
{
	for (auto i=m_List.begin();i!=m_List.end();i++)
		delete *i;
	m_List.clear();
}


CColorScheme *CColorSchemeList::GetColorScheme(int Index)
{
	if (Index<0 || (size_t)Index>=m_List.size())
		return NULL;
	return m_List[Index];
}


bool CColorSchemeList::SetColorScheme(int Index,const CColorScheme *pColorScheme)
{
	if (Index<0 || (size_t)Index>=m_List.size() || pColorScheme==NULL)
		return false;
	*m_List[Index]=*pColorScheme;
	return true;
}


int CColorSchemeList::FindByName(LPCTSTR pszName,int FirstIndex) const
{
	if (pszName==NULL)
		return -1;

	for (int i=max(FirstIndex,0);i<(int)m_List.size();i++) {
		if (!IsStringEmpty(m_List[i]->GetName())
				&& ::lstrcmpi(m_List[i]->GetName(),pszName)==0)
			return i;
	}
	return -1;
}


void CColorSchemeList::SortByName()
{
	if (m_List.size()>1) {
		std::sort(m_List.begin(),m_List.end(),
			[](const CColorScheme *pColorScheme1,const CColorScheme *pColorScheme2) -> bool {
				return ::lstrcmpi(pColorScheme1->GetName(),pColorScheme2->GetName())<0;
			});
	}
}




const LPCTSTR CColorSchemeOptions::m_pszExtension=TEXT(".httheme");


CColorSchemeOptions::CColorSchemeOptions()
	: m_pColorScheme(new CColorScheme)
	, m_pPreviewColorScheme(NULL)
	, m_pEventHandler(NULL)
{
}


CColorSchemeOptions::~CColorSchemeOptions()
{
	Destroy();
	delete m_pColorScheme;
	delete m_pPreviewColorScheme;
}


bool CColorSchemeOptions::LoadSettings(CSettings &Settings)
{
	return m_pColorScheme->Load(Settings);
}


bool CColorSchemeOptions::SaveSettings(CSettings &Settings)
{
	return m_pColorScheme->Save(Settings,
								CColorScheme::SAVE_NODEFAULT |
								CColorScheme::SAVE_NONAME);
}


bool CColorSchemeOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_COLORSCHEME));
}


bool CColorSchemeOptions::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
	return true;
}


bool CColorSchemeOptions::ApplyColorScheme() const
{
	return Apply(m_pColorScheme);
}


bool CColorSchemeOptions::Apply(const CColorScheme *pColorScheme) const
{
	if (m_pEventHandler==NULL)
		return false;
	return m_pEventHandler->ApplyColorScheme(pColorScheme);
}


COLORREF CColorSchemeOptions::GetColor(int Type) const
{
	if (m_pPreviewColorScheme!=NULL)
		return m_pPreviewColorScheme->GetColor(Type);
	return m_pColorScheme->GetColor(Type);
}


COLORREF CColorSchemeOptions::GetColor(LPCTSTR pszText) const
{
	if (m_pPreviewColorScheme!=NULL)
		return m_pPreviewColorScheme->GetColor(pszText);
	return m_pColorScheme->GetColor(pszText);
}


void CColorSchemeOptions::GetCurrentSettings(CColorScheme *pColorScheme)
{
	int i;

	for (i=0;i<CColorScheme::NUM_COLORS;i++)
		pColorScheme->SetColor(i,
							   (COLORREF)DlgListBox_GetItemData(m_hDlg,IDC_COLORSCHEME_LIST,i));
	for (int i=0;i<CColorScheme::NUM_GRADIENTS;i++)
		pColorScheme->SetGradientStyle(i,m_GradientList[i]);
	for (int i=0;i<CColorScheme::NUM_BORDERS;i++)
		pColorScheme->SetBorderType(i,m_BorderList[i]);
}


bool CColorSchemeOptions::GetThemesDirectory(LPTSTR pszDirectory,int MaxLength,bool fCreate)
{
	GetAppClass().GetAppDirectory(pszDirectory);
	::PathAppend(pszDirectory,TEXT("Themes"));
	if (fCreate && !::PathIsDirectory(pszDirectory))
		::CreateDirectory(pszDirectory,NULL);
	return true;
}


INT_PTR CColorSchemeOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			TCHAR szDirectory[MAX_PATH];
			int i;

			GetThemesDirectory(szDirectory,lengthof(szDirectory));
			m_PresetList.Load(szDirectory);
			//m_PresetList.SortByName();
			CColorScheme *pColorScheme=new CColorScheme(*m_pColorScheme);
			pColorScheme->SetName(TEXT("���݂̃e�[�}"));
			pColorScheme->SetLoaded();
			m_PresetList.Insert(0,pColorScheme);
			pColorScheme=new CColorScheme;
			pColorScheme->SetName(TEXT("�f�t�H���g�̃e�[�}"));
			pColorScheme->SetLoaded();
			m_PresetList.Insert(1,pColorScheme);
			for (i=0;i<m_PresetList.NumColorSchemes();i++) {
				DlgComboBox_AddItem(hDlg,IDC_COLORSCHEME_PRESET,i);
			}
			int Height=7*HIWORD(::GetDialogBaseUnits())/8+6;
			DlgComboBox_SetItemHeight(hDlg,IDC_COLORSCHEME_PRESET,0,Height);
			DlgComboBox_SetItemHeight(hDlg,IDC_COLORSCHEME_PRESET,-1,Height);
			DlgComboBox_SetCurSel(hDlg,IDC_COLORSCHEME_PRESET,0);
			EnableDlgItem(hDlg,IDC_COLORSCHEME_DELETE,false);

			for (i=0;i<CColorScheme::NUM_COLORS;i++) {
				DlgListBox_AddItem(hDlg,IDC_COLORSCHEME_LIST,m_pColorScheme->GetColor(i));
			}
			HDC hdc=GetDC(GetDlgItem(hDlg,IDC_COLORSCHEME_LIST));
			HFONT hfontOld=
				SelectFont(hdc,(HFONT)SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,WM_GETFONT,0,0));
			long MaxWidth=0;
			for (i=0;i<CColorScheme::NUM_COLORS;i++) {
				LPCTSTR pszName=CColorScheme::GetColorName(i);
				RECT rc={0,0,0,0};
				::DrawText(hdc,pszName,-1,&rc,DT_SINGLELINE | DT_NOPREFIX | DT_CALCRECT);
				if (rc.right>MaxWidth)
					MaxWidth=rc.right;
			}
			SelectFont(hdc,hfontOld);
			ReleaseDC(GetDlgItem(hDlg,IDC_COLORSCHEME_LIST),hdc);
			DlgListBox_SetItemHeight(hDlg,IDC_COLORSCHEME_LIST,0,
									 7*HIWORD(::GetDialogBaseUnits())/8);
			DlgListBox_SetHorizontalExtent(hDlg,IDC_COLORSCHEME_LIST,
										   DlgListBox_GetItemHeight(hDlg,IDC_COLORSCHEME_LIST,0)*2+MaxWidth+2);
			ExtendListBox(GetDlgItem(hDlg,IDC_COLORSCHEME_LIST));

			for (int i=0;i<CColorScheme::NUM_GRADIENTS;i++)
				m_pColorScheme->GetGradientStyle(i,&m_GradientList[i]);
			for (int i=0;i<CColorScheme::NUM_BORDERS;i++)
				m_BorderList[i]=m_pColorScheme->GetBorderType(i);

			RECT rc;
			static const RGBQUAD BaseColors[18] = {
				{0x00, 0x00, 0xFF},
				{0x00, 0x66, 0xFF},
				{0x00, 0xCC, 0xFF},
				{0x00, 0xFF, 0xFF},
				{0x00, 0xFF, 0xCC},
				{0x00, 0xFF, 0x66},
				{0x00, 0xFF, 0x00},
				{0x66, 0xFF, 0x00},
				{0xCC, 0xFF, 0x00},
				{0xFF, 0xFF, 0x00},
				{0xFF, 0xCC, 0x00},
				{0xFF, 0x66, 0x00},
				{0xFF, 0x00, 0x00},
				{0xFF, 0x00, 0x66},
				{0xFF, 0x00, 0xCC},
				{0xFF, 0x00, 0xFF},
				{0xCC, 0x00, 0xFF},
				{0x66, 0x00, 0xFF},
			};
			RGBQUAD Palette[256];
			int j,k;

			CColorPalette::Initialize(GetWindowInstance(hDlg));
			m_ColorPalette.Create(hDlg,WS_CHILD | WS_VISIBLE,0,IDC_COLORSCHEME_PALETTE);
			GetWindowRect(GetDlgItem(hDlg,IDC_COLORSCHEME_PALETTEPLACE),&rc);
			MapWindowPoints(NULL,hDlg,(LPPOINT)&rc,2);
			m_ColorPalette.SetPosition(&rc);
			for (i=0;i<lengthof(BaseColors);i++) {
				RGBQUAD Color=BaseColors[i%2*(lengthof(BaseColors)/2)+i/2];

				for (j=0;j<4;j++) {
					Palette[i*8+j].rgbBlue=(Color.rgbBlue*(j+1))/4;
					Palette[i*8+j].rgbGreen=(Color.rgbGreen*(j+1))/4;
					Palette[i*8+j].rgbRed=(Color.rgbRed*(j+1))/4;
				}
				for (;j<8;j++) {
					Palette[i*8+j].rgbBlue=Color.rgbBlue+(255-Color.rgbBlue)*(j-3)/5;
					Palette[i*8+j].rgbGreen=Color.rgbGreen+(255-Color.rgbGreen)*(j-3)/5;
					Palette[i*8+j].rgbRed=Color.rgbRed+(255-Color.rgbRed)*(j-3)/5;
				}
			}
			i=lengthof(BaseColors)*8;
			for (j=0;j<16;j++) {
				Palette[i].rgbBlue=(255*j)/15;
				Palette[i].rgbGreen=(255*j)/15;
				Palette[i].rgbRed=(255*j)/15;
				i++;
			}
			for (j=0;j<CColorScheme::NUM_COLORS;j++) {
				COLORREF cr=m_pColorScheme->GetColor(j);

				for (k=0;k<i;k++) {
					if (cr==RGB(Palette[k].rgbRed,Palette[k].rgbGreen,Palette[k].rgbBlue))
						break;
				}
				if (k==i) {
					Palette[i].rgbBlue=GetBValue(cr);
					Palette[i].rgbGreen=GetGValue(cr);
					Palette[i].rgbRed=GetRValue(cr);
					i++;
				}
			}
			if (i<lengthof(Palette))
				ZeroMemory(&Palette[i],(lengthof(Palette)-i)*sizeof(RGBQUAD));
			m_ColorPalette.SetPalette(Palette,lengthof(Palette));
		}
		return TRUE;

	/*
	case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

			pmis->itemHeight=7*HIWORD(GetDialogBaseUnits())/8;
			if (pmis->CtlID==IDC_COLORSCHEME_PRESET)
				pmis->itemHeight+=6;
		}
		return TRUE;
	*/

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

			if (pdis->CtlID==IDC_COLORSCHEME_PRESET) {
				switch (pdis->itemAction) {
				case ODA_DRAWENTIRE:
				case ODA_SELECT:
					if ((int)pdis->itemID<0) {
						::FillRect(pdis->hDC,&pdis->rcItem,
								   reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
					} else {
						const CColorScheme *pColorScheme=m_PresetList.GetColorScheme((int)pdis->itemData);
						if (pColorScheme==NULL)
							break;
						bool fSelected=(pdis->itemState & ODS_SELECTED)!=0
									&& (pdis->itemState & ODS_COMBOBOXEDIT)==0;
						Theme::CThemeManager ThemeManager(pColorScheme);
						Theme::Style Style;

						ThemeManager.GetStyle(fSelected?
											  Theme::CThemeManager::STYLE_STATUSBAR_ITEM_HOT:
											  Theme::CThemeManager::STYLE_STATUSBAR_ITEM,
											  &Style);
						Theme::Draw(pdis->hDC,pdis->rcItem,Style.Back);
						if (!IsStringEmpty(pColorScheme->GetName())) {
							int OldBkMode;
							RECT rc;
							HFONT hfont,hfontOld;

							if (fSelected) {
								LOGFONT lf;

								hfontOld=static_cast<HFONT>(::GetCurrentObject(pdis->hDC,OBJ_FONT));
								::GetObject(hfontOld,sizeof(LOGFONT),&lf);
								lf.lfWeight=FW_BOLD;
								hfont=::CreateFontIndirect(&lf);
								SelectFont(pdis->hDC,hfont);
							} else {
								hfont=NULL;
							}
							OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
							rc=pdis->rcItem;
							rc.left+=4;
							Theme::Draw(pdis->hDC,rc,Style.Fore,pColorScheme->GetName(),
								DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
							::SetBkMode(pdis->hDC,OldBkMode);
							if (hfont!=NULL) {
								::SelectObject(pdis->hDC,hfontOld);
								::DeleteObject(hfont);
							}
						}
					}
					if ((pdis->itemState & ODS_FOCUS)==0)
						break;
				case ODA_FOCUS:
					if ((pdis->itemState & ODS_NOFOCUSRECT)==0
							&& (pdis->itemState & ODS_COMBOBOXEDIT)!=0)
						::DrawFocusRect(pdis->hDC,&pdis->rcItem);
					break;
				}
			} else if (pdis->CtlID==IDC_COLORSCHEME_LIST) {
				switch (pdis->itemAction) {
				case ODA_DRAWENTIRE:
				case ODA_SELECT:
					{
						int BackSysColor;
						COLORREF BackColor,TextColor,OldTextColor;
						HBRUSH hbr,hbrOld;
						HPEN hpenOld;
						RECT rc;
						int OldBkMode;

						if ((pdis->itemState & ODS_SELECTED)==0) {
							BackSysColor=COLOR_WINDOW;
							TextColor=::GetSysColor(COLOR_WINDOWTEXT);
						} else {
							BackSysColor=COLOR_HIGHLIGHT;
							TextColor=::GetSysColor(COLOR_HIGHLIGHTTEXT);
						}
						BackColor=::GetSysColor(BackSysColor);
						int Border=CColorScheme::GetColorBorder((int)pdis->itemID);
						if (Border>=0 && m_BorderList[Border]==Theme::BORDER_NONE)
							TextColor=MixColor(TextColor,BackColor);
						::FillRect(pdis->hDC,&pdis->rcItem,reinterpret_cast<HBRUSH>(BackSysColor+1));
						hbr=::CreateSolidBrush((COLORREF)pdis->itemData);
						hbrOld=SelectBrush(pdis->hDC,hbr);
						hpenOld=SelectPen(pdis->hDC,::GetStockObject(BLACK_PEN));
						rc.left=pdis->rcItem.left+2;
						rc.top=pdis->rcItem.top+2;
						rc.bottom=pdis->rcItem.bottom-2;
						rc.right=rc.left+(rc.bottom-rc.top)*2;
						::Rectangle(pdis->hDC,rc.left,rc.top,rc.right,rc.bottom);
						::SelectObject(pdis->hDC,hpenOld);
						::SelectObject(pdis->hDC,hbrOld);
						::DeleteObject(hbr);
						OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
						OldTextColor=::SetTextColor(pdis->hDC,TextColor);
						rc.left=rc.right+2;
						rc.top=pdis->rcItem.top;
						rc.right=pdis->rcItem.right;
						rc.bottom=pdis->rcItem.bottom;
						::DrawText(pdis->hDC,CColorScheme::GetColorName(pdis->itemID),-1,&rc,
								   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
						::SetTextColor(pdis->hDC,OldTextColor);
						::SetBkMode(pdis->hDC,OldBkMode);
					}
					if ((pdis->itemState & ODS_FOCUS)==0)
						break;
				case ODA_FOCUS:
					if ((pdis->itemState & ODS_NOFOCUSRECT)==0)
						::DrawFocusRect(pdis->hDC,&pdis->rcItem);
					break;
				}
			}
		}
		return TRUE;

	case WM_COMPAREITEM:
		{
			COMPAREITEMSTRUCT *pcis=reinterpret_cast<COMPAREITEMSTRUCT*>(lParam);

			if (pcis->CtlID==IDC_COLORSCHEME_PRESET) {
				if (pcis->itemData1<2 || pcis->itemData2<2)
					return (int)pcis->itemData1-(int)pcis->itemData2;

				const CColorScheme *pColorScheme1=m_PresetList.GetColorScheme((int)pcis->itemData1);
				const CColorScheme *pColorScheme2=m_PresetList.GetColorScheme((int)pcis->itemData2);
				if (pColorScheme1==NULL || pColorScheme2==NULL)
					return 0;
				int Cmp=::CompareString(pcis->dwLocaleId,NORM_IGNORECASE,
										pColorScheme1->GetName(),-1,
										pColorScheme2->GetName(),-1);
				if (Cmp!=0)
					Cmp-=CSTR_EQUAL;
				return Cmp;
			}
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_COLORSCHEME_PRESET:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_COLORSCHEME_PRESET);

				if (Sel>=0) {
					int Index=(int)DlgComboBox_GetItemData(hDlg,IDC_COLORSCHEME_PRESET,Sel);
					const CColorScheme *pColorScheme=m_PresetList.GetColorScheme(Index);

					if (pColorScheme!=NULL) {
						int i;

						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (pColorScheme->IsLoaded(i))
								SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,
									LB_SETITEMDATA,i,pColorScheme->GetColor(i));
						}
						SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_SETSEL,FALSE,-1);
						InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);

						for (i=0;i<CColorScheme::NUM_GRADIENTS;i++)
							pColorScheme->GetGradientStyle(i,&m_GradientList[i]);
						for (i=0;i<CColorScheme::NUM_BORDERS;i++)
							m_BorderList[i]=pColorScheme->GetBorderType(i);

						m_ColorPalette.SetSel(-1);
						::SendMessage(hDlg,WM_COMMAND,IDC_COLORSCHEME_PREVIEW,0);
					}
				}

				EnableDlgItem(hDlg,IDC_COLORSCHEME_DELETE,Sel>=2);
			}
			return TRUE;

		case IDC_COLORSCHEME_SAVE:
			{
				CColorScheme *pColorScheme;
				TCHAR szName[MAX_COLORSCHEME_NAME];
				szName[0]=_T('\0');
				LRESULT Sel=DlgComboBox_GetCurSel(hDlg,IDC_COLORSCHEME_PRESET);
				if (Sel>=2) {
					pColorScheme=m_PresetList.GetColorScheme(
						(int)DlgComboBox_GetItemData(hDlg,IDC_COLORSCHEME_PRESET,Sel));
					if (pColorScheme!=NULL && !IsStringEmpty(pColorScheme->GetName()))
						::lstrcpyn(szName,pColorScheme->GetName(),lengthof(szName));
				}
				if (::DialogBoxParam(GetAppClass().GetResourceInstance(),
									 MAKEINTRESOURCE(IDD_SAVECOLORSCHEME),
									 hDlg,SaveDlgProc,reinterpret_cast<LPARAM>(szName))!=IDOK)
					return TRUE;

				pColorScheme=NULL;
				int Index=m_PresetList.FindByName(szName,2);
				if (Index>=2) {
					pColorScheme=m_PresetList.GetColorScheme(Index);
				}
				bool fNewColorScheme;
				TCHAR szFileName[MAX_PATH];
				if (pColorScheme!=NULL && !IsStringEmpty(pColorScheme->GetFileName())) {
					::lstrcpy(szFileName,pColorScheme->GetFileName());
					fNewColorScheme=false;
				} else {
					GetThemesDirectory(szFileName,lengthof(szFileName),true);
					if (::lstrlen(szFileName)+1+::lstrlen(szName)+::lstrlen(m_pszExtension)>=MAX_PATH) {
						MessageBox(hDlg,TEXT("���O���������܂��B"),NULL,MB_OK | MB_ICONEXCLAMATION);
						break;
					}
					::PathAppend(szFileName,szName);
					::lstrcat(szFileName,m_pszExtension);
					pColorScheme=new CColorScheme;
					pColorScheme->SetFileName(szFileName);
					fNewColorScheme=true;
				}
				pColorScheme->SetName(szName);
				pColorScheme->SetLoaded();

				GetCurrentSettings(pColorScheme);
				if (!pColorScheme->Save(szFileName)) {
					if (fNewColorScheme)
						delete pColorScheme;
					::MessageBox(hDlg,TEXT("�ۑ����ł��܂���B"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}

				if (fNewColorScheme) {
					m_PresetList.Add(pColorScheme);
					Index=(int)DlgComboBox_AddItem(hDlg,IDC_COLORSCHEME_PRESET,m_PresetList.NumColorSchemes()-1);
				} else {
					InvalidateDlgItem(hDlg,IDC_COLORSCHEME_PRESET);
					int ItemCount=(int)DlgComboBox_GetCount(hDlg,IDC_COLORSCHEME_PRESET);
					for (int i=2;i<ItemCount;i++) {
						if (DlgComboBox_GetItemData(hDlg,IDC_COLORSCHEME_PRESET,i)==Index) {
							Index=i;
							break;
						}
					}
				}
				DlgComboBox_SetCurSel(hDlg,IDC_COLORSCHEME_PRESET,Index);

				::MessageBox(hDlg,TEXT("�e�[�}��ۑ����܂����B"),TEXT("�ۑ�"),MB_OK | MB_ICONINFORMATION);
			}
			return TRUE;

		case IDC_COLORSCHEME_DELETE:
			{
				LRESULT Sel=DlgComboBox_GetCurSel(hDlg,IDC_COLORSCHEME_PRESET);
				if (Sel<2)
					break;
				CColorScheme *pColorScheme=m_PresetList.GetColorScheme(
					(int)DlgComboBox_GetItemData(hDlg,IDC_COLORSCHEME_PRESET,Sel));
				if (pColorScheme==NULL || IsStringEmpty(pColorScheme->GetFileName()))
					break;
				if (::MessageBox(hDlg,TEXT("�I�����ꂽ�e�[�}���폜���܂���?"),TEXT("�폜�̊m�F"),
								 MB_OKCANCEL | MB_ICONQUESTION)!=IDOK)
					break;
				if (!::DeleteFile(pColorScheme->GetFileName())) {
					::MessageBox(hDlg,TEXT("�t�@�C�����폜�ł��܂���B"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				DlgComboBox_DeleteItem(hDlg,IDC_COLORSCHEME_PRESET,Sel);
			}
			return TRUE;

		case IDC_COLORSCHEME_LIST:
			switch (HIWORD(wParam)) {
			case LBN_SELCHANGE:
				{
					int SelCount=(int)DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST);
					int i;
					COLORREF SelColor=CLR_INVALID,Color;

					if (SelCount==0) {
						m_ColorPalette.SetSel(-1);
						break;
					}
					if (SelCount==1) {
						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i)) {
								SelColor=(COLORREF)DlgListBox_GetItemData(hDlg,IDC_COLORSCHEME_LIST,i);
								break;
							}
						}
					} else {
						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i)) {
								Color=(COLORREF)DlgListBox_GetItemData(hDlg,IDC_COLORSCHEME_LIST,i);
								if (SelColor==CLR_INVALID)
									SelColor=Color;
								else if (Color!=SelColor)
									break;
							}
						}
						if (i<CColorScheme::NUM_COLORS) {
							m_ColorPalette.SetSel(-1);
							break;
						}
					}
					if (SelColor!=CLR_INVALID)
						m_ColorPalette.SetSel(m_ColorPalette.FindColor(SelColor));
				}
				break;

			case LBN_EX_RBUTTONUP:
				{
					HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDM_COLORSCHEME));
					POINT pt;

					::EnableMenuItem(hmenu,IDC_COLORSCHEME_SELECTSAMECOLOR,
						MF_BYCOMMAND | (m_ColorPalette.GetSel()>=0?MFS_ENABLED:MFS_GRAYED));
					if (DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST)==1) {
						int Sel,Gradient,Border;

						DlgListBox_GetSelItems(hDlg,IDC_COLORSCHEME_LIST,&Sel,1);
						Gradient=CColorScheme::GetColorGradient(Sel);
						if (Gradient>=0) {
							::EnableMenuItem(::GetSubMenu(hmenu,0),2,MF_BYPOSITION | MFS_ENABLED);
							::CheckMenuRadioItem(hmenu,
												 IDC_COLORSCHEME_GRADIENT_NORMAL,IDC_COLORSCHEME_GRADIENT_INTERLACED,
												 IDC_COLORSCHEME_GRADIENT_NORMAL+(int)m_GradientList[Gradient].Type,
												 MF_BYCOMMAND);
							::EnableMenuItem(::GetSubMenu(hmenu,0),3,MF_BYPOSITION | MFS_ENABLED);
							::CheckMenuRadioItem(hmenu,
												 IDC_COLORSCHEME_DIRECTION_HORZ,IDC_COLORSCHEME_DIRECTION_VERTMIRROR,
												 IDC_COLORSCHEME_DIRECTION_HORZ+(int)m_GradientList[Gradient].Direction,
												 MF_BYCOMMAND);
							if (!CColorScheme::IsGradientDirectionEnabled(Gradient)) {
								if (m_GradientList[Gradient].Direction==Theme::DIRECTION_HORZ
										|| m_GradientList[Gradient].Direction==Theme::DIRECTION_HORZMIRROR) {
									::EnableMenuItem(hmenu,IDC_COLORSCHEME_DIRECTION_VERT,MF_BYCOMMAND | MFS_GRAYED);
									::EnableMenuItem(hmenu,IDC_COLORSCHEME_DIRECTION_VERTMIRROR,MF_BYCOMMAND | MFS_GRAYED);
								} else {
									::EnableMenuItem(hmenu,IDC_COLORSCHEME_DIRECTION_HORZ,MF_BYCOMMAND | MFS_GRAYED);
									::EnableMenuItem(hmenu,IDC_COLORSCHEME_DIRECTION_HORZMIRROR,MF_BYCOMMAND | MFS_GRAYED);
								}
							}
						}
						Border=CColorScheme::GetColorBorder(Sel);
						if (Border>=0) {
							::EnableMenuItem(::GetSubMenu(hmenu,0),4,MF_BYPOSITION | MFS_ENABLED);
							::EnableMenuItem(hmenu,IDC_COLORSCHEME_BORDER_NONE,
											 MF_BYCOMMAND | (CColorScheme::IsBorderAlways(Border)?MFS_GRAYED:MFS_ENABLED));
							::CheckMenuRadioItem(hmenu,
												 IDC_COLORSCHEME_BORDER_NONE,IDC_COLORSCHEME_BORDER_RAISED,
												 IDC_COLORSCHEME_BORDER_NONE+(int)m_BorderList[Border],
												 MF_BYCOMMAND);
						}
					}
					::GetCursorPos(&pt);
					::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,hDlg,NULL);
					::DestroyMenu(hmenu);
				}
				break;
			}
			return TRUE;

		case IDC_COLORSCHEME_PALETTE:
			switch (HIWORD(wParam)) {
			case CColorPalette::NOTIFY_SELCHANGE:
				{
					int Sel=m_ColorPalette.GetSel();
					COLORREF Color=m_ColorPalette.GetColor(Sel);
					int i;

					for (i=0;i<CColorScheme::NUM_COLORS;i++) {
						if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i))
							DlgListBox_SetItemData(hDlg,IDC_COLORSCHEME_LIST,i,Color);
					}
					InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
				}
				break;

			case CColorPalette::NOTIFY_DOUBLECLICK:
				{
					int Sel=m_ColorPalette.GetSel();
					COLORREF Color=m_ColorPalette.GetColor(Sel);

					if (ChooseColorDialog(hDlg,&Color)) {
						m_ColorPalette.SetColor(Sel,Color);
						int i;

						for (i=0;i<CColorScheme::NUM_COLORS;i++) {
							if (DlgListBox_GetSel(hDlg,IDC_COLORSCHEME_LIST,i))
								DlgListBox_SetItemData(hDlg,IDC_COLORSCHEME_LIST,i,Color);
						}
						InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
					}
				}
				break;
			}
			return TRUE;

		case IDC_COLORSCHEME_PREVIEW:
			if (m_pPreviewColorScheme==NULL)
				m_pPreviewColorScheme=new CColorScheme;
			GetCurrentSettings(m_pPreviewColorScheme);
			Apply(m_pPreviewColorScheme);
			return TRUE;

		case IDC_COLORSCHEME_SELECTSAMECOLOR:
			{
				int Sel=m_ColorPalette.GetSel();

				if (Sel>=0) {
					COLORREF Color=m_ColorPalette.GetColor(Sel);
					int TopIndex=(int)DlgListBox_GetTopIndex(hDlg,IDC_COLORSCHEME_LIST);

					::SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,WM_SETREDRAW,FALSE,0);
					for (int i=0;i<CColorScheme::NUM_COLORS;i++) {
						DlgListBox_SetSel(hDlg,IDC_COLORSCHEME_LIST,i,
							(COLORREF)DlgListBox_GetItemData(hDlg,IDC_COLORSCHEME_LIST,i)==Color);
					}
					DlgListBox_SetTopIndex(hDlg,IDC_COLORSCHEME_LIST,TopIndex);
					::SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,WM_SETREDRAW,TRUE,0);
					::InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST);
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_GRADIENT_NORMAL:
		case IDC_COLORSCHEME_GRADIENT_GLOSSY:
		case IDC_COLORSCHEME_GRADIENT_INTERLACED:
			if (DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST)==1) {
				int Sel,Gradient;

				DlgListBox_GetSelItems(hDlg,IDC_COLORSCHEME_LIST,&Sel,1);
				Gradient=CColorScheme::GetColorGradient(Sel);
				if (Gradient>=0) {
					m_GradientList[Gradient].Type=
						(Theme::GradientType)(LOWORD(wParam)-IDC_COLORSCHEME_GRADIENT_NORMAL);
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_DIRECTION_HORZ:
		case IDC_COLORSCHEME_DIRECTION_VERT:
		case IDC_COLORSCHEME_DIRECTION_HORZMIRROR:
		case IDC_COLORSCHEME_DIRECTION_VERTMIRROR:
			if (DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST)==1) {
				int Sel,Gradient;

				DlgListBox_GetSelItems(hDlg,IDC_COLORSCHEME_LIST,&Sel,1);
				Gradient=CColorScheme::GetColorGradient(Sel);
				if (Gradient>=0) {
					m_GradientList[Gradient].Direction=
						(Theme::GradientDirection)(LOWORD(wParam)-IDC_COLORSCHEME_DIRECTION_HORZ);
				}
			}
			return TRUE;

		case IDC_COLORSCHEME_BORDER_NONE:
		case IDC_COLORSCHEME_BORDER_SOLID:
		case IDC_COLORSCHEME_BORDER_SUNKEN:
		case IDC_COLORSCHEME_BORDER_RAISED:
			if (DlgListBox_GetSelCount(hDlg,IDC_COLORSCHEME_LIST)==1) {
				int Sel,Border;

				DlgListBox_GetSelItems(hDlg,IDC_COLORSCHEME_LIST,&Sel,1);
				Border=CColorScheme::GetColorBorder(Sel);
				if (Border>=0) {
					m_BorderList[Border]=
						(Theme::BorderType)(LOWORD(wParam)-IDC_COLORSCHEME_BORDER_NONE);
					RECT rc;
					::SendDlgItemMessage(hDlg,IDC_COLORSCHEME_LIST,LB_GETITEMRECT,
										 Sel,reinterpret_cast<LPARAM>(&rc));
					InvalidateDlgItem(hDlg,IDC_COLORSCHEME_LIST,&rc);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			GetCurrentSettings(m_pColorScheme);
			Apply(m_pColorScheme);
			m_fChanged=true;
			break;

		case PSN_RESET:
			if (m_pPreviewColorScheme!=NULL)
				Apply(m_pColorScheme);
			break;
		}
		break;

// �J���p�@�\
#ifdef _DEBUG
	case WM_RBUTTONUP:
		{
			HMENU hmenu=::CreatePopupMenu();
			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,1,TEXT("�z�F�R�[�h���R�s�[(&C)"));
			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,2,TEXT("�{�[�_�[�ݒ���R�s�[(&B)"));

			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			::ClientToScreen(hDlg,&pt);

			switch (::TrackPopupMenu(hmenu,TPM_RETURNCMD | TPM_RIGHTBUTTON,pt.x,pt.y,0,hDlg,NULL)) {
			case 1:
				{
					TVTest::String Buffer;

					for (int i=0;i<CColorScheme::NUM_COLORS;i++) {
						COLORREF cr=(COLORREF)DlgListBox_GetItemData(hDlg,IDC_COLORSCHEME_LIST,i);
						TCHAR szColor[32];
						StdUtil::snprintf(szColor,lengthof(szColor),
										  TEXT("HEXRGB(0x%02X%02X%02X)\r\n"),
										  GetRValue(cr),GetGValue(cr),GetBValue(cr));
						Buffer+=szColor;
					}

					CopyTextToClipboard(hDlg,Buffer.c_str());
				}
				break;

			case 2:
				{
					TVTest::String Buffer;

					for (int i=0;i<CColorScheme::NUM_BORDERS;i++) {
						switch (m_BorderList[i]) {
						case Theme::BORDER_NONE:
							Buffer+=TEXT("Theme::BORDER_NONE,\r\n");
							break;
						case Theme::BORDER_SOLID:
							Buffer+=TEXT("Theme::BORDER_SOLID,\r\n");
							break;
						case Theme::BORDER_SUNKEN:
							Buffer+=TEXT("Theme::BORDER_SUNKEN,\r\n");
							break;
						case Theme::BORDER_RAISED:
							Buffer+=TEXT("Theme::BORDER_RAISED,\r\n");
							break;
						}
					}

					CopyTextToClipboard(hDlg,Buffer.c_str());
				}
				break;
			}
		}
		return TRUE;
#endif

	case WM_DESTROY:
		SAFE_DELETE(m_pPreviewColorScheme);
		m_PresetList.Clear();
		break;
	}

	return FALSE;
}


INT_PTR CALLBACK CColorSchemeOptions::SaveDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static LPTSTR pszName;

	switch (uMsg) {
	case WM_INITDIALOG:
		pszName=reinterpret_cast<LPTSTR>(lParam);
		DlgEdit_SetText(hDlg,IDC_SAVECOLORSCHEME_NAME,pszName);
		DlgEdit_LimitText(hDlg,IDC_SAVECOLORSCHEME_NAME,MAX_COLORSCHEME_NAME);
		EnableDlgItem(hDlg,IDOK,pszName[0]!=_T('\0'));
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SAVECOLORSCHEME_NAME:
			if (HIWORD(wParam)==EN_CHANGE) {
				EnableDlgItem(hDlg,IDOK,GetDlgItemTextLength(hDlg,IDC_SAVECOLORSCHEME_NAME)>0);
			}
			return TRUE;

		case IDOK:
			::GetDlgItemText(hDlg,IDC_SAVECOLORSCHEME_NAME,pszName,MAX_COLORSCHEME_NAME);
			if (pszName[0]==_T('\0'))
				return TRUE;
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}
