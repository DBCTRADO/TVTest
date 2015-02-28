/*
	TVTest �C���^�[�t�F�[�X�w�b�_ ver.0.0.14-pre

	�� ver.0.0.14 �͂܂��J���r���ł��B����ύX�����\��������܂��B

	���̃t�@�C���͍Ĕz�z�E���ςȂǎ��R�ɍs���č\���܂���B
	�������A���ς����ꍇ�̓I���W�i���ƈႤ�|���L�ڂ��Ē�����ƁA�������Ȃ��Ă�
	���Ǝv���܂��B
*/

/*
	�v���O�C������ MESSAGE_REGISTERTSPROCESSOR �� ITSProcessor ��o�^����ƁA
	TS �ɑ΂��鏈�����s����悤�ɂȂ�܂��B

	ITSProcessor �͕K�v�ɉ����Ĉȉ��̃C���^�[�t�F�[�X���������A
	QueryInterface() �Ŏ擾�ł���悤�ɂ��܂��B

	�EIFilterManager
	�EIFilterModule
	�EIPersistPropertyBag
	�EISpecifyPropertyPages
	�EISpecifyPropertyPages2

	ITSProcessor �̏����̗���͈ȉ��̂悤�ɂȂ�܂��B

	  ITSProcessor::Initialize()           ����������
	    ��
	  ITSProcessor::StartStreaming() ����  �X�g���[�~���O�̊J�n
	    ��                             ��
	  ITSProcessor::InputPacket()      ��  �X�g���[���̓���(�J��Ԃ��Ă΂��)
	    ��                             ��
	  ITSProcessor::StopStreaming()  ����  �X�g���[�~���O�̏I��
	    ��
	  ITSProcessor::Finalize()             �I������

	ITSProcessor::InputPacket() �� ITSPacket ��ʂ���TS�f�[�^���n�����̂ŁA
	ITSProcessor::StartStreaming() �œn���ꂽ ITSOutput �� OutputPacket() ��
	�Ăяo����TS�f�[�^���o�͂��܂��B

	TS�v���Z�b�T��o�^����ۂɁA�ڑ��ʒu�Ƃ��� TS_PROCESSOR_CONNECT_POSITION_SOURCE ��
	�w�肵���ꍇ�AITSProcessor::InputPacket() �ɓ��͂����f�[�^�̓`���[�i�[������
	���͂��ꂽ�X�g���[�����̂܂܂ł���AITSPacket::GetSize() �Ŏ擾�����T�C�Y�͕s��ł��B
	����ȊO�̏ꍇ�AITSPacket ��188�o�C�g�̃p�P�b�g�f�[�^��\���܂��B

	�Ȃ��AITSPacket ���p�P�b�g�f�[�^��\�����A�p�P�b�g�̃w�b�_�������������ꍇ��
	ITSPacket::SetModified() ���Ăяo���āA�ύX���ꂽ���Ƃ�������悤�ɂ���K�v������܂��B

	MESSAGE_REGISTERTSPROCESSOR �� TSProcessorInfo::ConnectPosition �Ɏw�肷��
	�ڑ��ʒu�̊֌W�͈ȉ��̂悤�ɂȂ�܂��B

	  �`���[�i�[��
	    ��(*1)
	  TS_PROCESSOR_CONNECT_POSITION_SOURCE
	    ��(*1)
	  �p�P�b�g�̐؂�o��(�X�g���[�����p�P�b�g�P�ʂɐ؂�o����܂�)
	    ��
	  TS_PROCESSOR_CONNECT_POSITION_PREPROCESSING
	    ��
	  TS�̉��(PAT/PMT/SDT/NIT/EIT ���̉�͂��s���܂�)
	    ��
	  TS_PROCESSOR_CONNECT_POSITION_POSTPROCESSING
	    ����������������������������������������
	    ��                                    ��
	  TS_PROCESSOR_CONNECT_POSITION_VIEWER  TS_PROCESSOR_CONNECT_POSITION_RECORDER
	    ��                                    ��
	  �r���[�A                              ���R�[�_

	(*1) �̓T�C�Y���s��̃X�g���[���ŁA����ȊO��188�o�C�g��TS�p�P�b�g�ł��B
*/


#ifndef TVTEST_INTERFACE_H
#define TVTEST_INTERFACE_H


#include <pshpack1.h>


namespace TVTest
{

namespace Interface
{


typedef ULONG DeviceIDType;

enum LogType
{
	LOG_VERBOSE,
	LOG_INFO,
	LOG_WARNING,
	LOG_ERROR
};

enum NotifyType
{
	NOTIFY_INFO,
	NOTIFY_WARNING,
	NOTIFY_ERROR
};

struct ErrorInfo
{
	HRESULT hr;
	LPCWSTR pszText;
	LPCWSTR pszAdvise;
	LPCWSTR pszSystemMessage;
};

struct FilterModuleInfo
{
	BSTR Name;
	BSTR Version;
};

struct FilterDeviceInfo
{
	DeviceIDType DeviceID;
	ULONG Flags;
	BSTR Name;
	BSTR Text;
};

MIDL_INTERFACE("D513D10B-9438-4613-9758-2C17C434BE36") IStreamingClient : public IUnknown
{
	STDMETHOD(OnError)(const ErrorInfo *pInfo) = 0;
	STDMETHOD(OutLog)(LogType Type, LPCWSTR pszMessage) = 0;
	STDMETHOD(Notify)(NotifyType Type, LPCWSTR pszMessage) = 0;
};

MIDL_INTERFACE("558F0FBF-992E-490F-8BAE-6CB709801B1D") ITSPacket : public IUnknown
{
	STDMETHOD(GetData)(BYTE **ppData) = 0;
	STDMETHOD(GetSize)(ULONG *pSize) = 0;
	STDMETHOD(SetModified)(BOOL fModified) = 0;
	STDMETHOD(GetModified)() = 0;
};

MIDL_INTERFACE("F34436F9-35CD-4883-B4B4-E6F1CB33BE51") ITSOutput : public IUnknown
{
	STDMETHOD(OutputPacket)(ITSPacket *pPacket) = 0;
};

MIDL_INTERFACE("207D79AE-193B-4CD6-8228-456F032820F6") ITSProcessor : public IUnknown
{
	STDMETHOD(GetGuid)(GUID *pGuid) = 0;
	STDMETHOD(GetName)(BSTR *pName) = 0;
	STDMETHOD(Initialize)(IStreamingClient *pClient) = 0;
	STDMETHOD(Finalize)() = 0;
	STDMETHOD(StartStreaming)(ITSOutput *pOutput) = 0;
	STDMETHOD(StopStreaming)() = 0;
	STDMETHOD(InputPacket)(ITSPacket *pPacket) = 0;
	STDMETHOD(Reset)() = 0;
	STDMETHOD(SetEnableProcessing)(BOOL fEnable) = 0;
	STDMETHOD(GetEnableProcessing)(BOOL *pfEnable) = 0;
	STDMETHOD(SetActiveServiceID)(WORD ServiceID) = 0;
	STDMETHOD(GetActiveServiceID)(WORD *pServiceID) = 0;
};

MIDL_INTERFACE("57ECE161-0DC3-4309-8601-F2144B6D5A59") IFilterDevice : public IUnknown
{
	STDMETHOD(GetDeviceInfo)(FilterDeviceInfo *pInfo) = 0;
	STDMETHOD(GetFilterCount)(int *pCount) = 0;
	STDMETHOD(GetFilterName)(int Index, BSTR *pName) = 0;
	STDMETHOD(IsFilterAvailable)(LPCWSTR pszName, BOOL *pfAvailable) = 0;
};

MIDL_INTERFACE("83360AD2-B875-49d4-8EA6-3A4840AC2CC5") IFilter : public IUnknown
{
	STDMETHOD(GetDeviceID)(DeviceIDType *pDeviceID) = 0;
	STDMETHOD(GetName)(BSTR *pName) = 0;
	STDMETHOD(SendCommand)(const void *pSendData, DWORD SendSize, void *pRecvData, DWORD *pRecvSize) = 0;
};

MIDL_INTERFACE("F9D074A9-5730-489e-AC14-7A37D29E8B3E") IFilterModule : public IUnknown
{
	STDMETHOD(LoadModule)(LPCWSTR pszName) = 0;
	STDMETHOD(UnloadModule)() = 0;
	STDMETHOD(IsModuleLoaded)() = 0;
	STDMETHOD(GetModuleInfo)(FilterModuleInfo *pInfo) = 0;
	STDMETHOD(GetDeviceCount)(int *pCount) = 0;
	STDMETHOD(GetDevice)(int Device, IFilterDevice **ppDevice) = 0;
	STDMETHOD(IsDeviceAvailable)(int Device, BOOL *pfAvailable) = 0;
	STDMETHOD(CheckDeviceAvailability)(int Device, BOOL *pfAvailable, BSTR *pMessage) = 0;
	STDMETHOD(GetDefaultDevice)(int *pDevice) = 0;
	STDMETHOD(GetDeviceByID)(DeviceIDType DeviceID, int *pDevice) = 0;
	STDMETHOD(GetDeviceByName)(LPCWSTR pszName, int *pDevice) = 0;
	STDMETHOD(OpenFilter)(int Device, LPCWSTR pszName, IFilter **ppFilter) = 0;
};

MIDL_INTERFACE("ADABE110-5913-418d-8C31-BF6E39F67F28") IEnumFilterModule : public IUnknown
{
	STDMETHOD(Next)(BSTR *pName) = 0;
};

MIDL_INTERFACE("5CB26641-E1B3-4E8E-884A-8BEF5240B7EC") IFilterManager : public IUnknown
{
	STDMETHOD(EnumModules)(IEnumFilterModule **ppEnumModule) = 0;
	STDMETHOD(CreateModule)(IFilterModule **ppModule) = 0;
};


}	// namespace Interface

}	// namespace TVTest


#include <poppack.h>


#ifndef TVTEST_INTERFACE_NO_EXTERNAL

#include <ocidl.h>

MIDL_INTERFACE("03481710-D73E-4674-839F-03EDE2D60ED8") ISpecifyPropertyPages2 : public ISpecifyPropertyPages
{
	STDMETHOD(CreatePage)(const GUID &guid, IPropertyPage **ppPage) = 0;
};

#endif


#endif	// TVTEST_INTERFACE_H
