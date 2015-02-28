#include "stdafx.h"
#include "TsInformation.h"
#include "Common.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




namespace TsEngine
{


LPCTSTR GetStreamTypeText(const BYTE StreamType)
{
	switch (StreamType) {
	case STREAM_TYPE_MPEG1_VIDEO:					return TEXT("MPEG-1 Video");
	case STREAM_TYPE_MPEG2_VIDEO:					return TEXT("MPEG-2 Video");
	case STREAM_TYPE_MPEG1_AUDIO:					return TEXT("MPEG-1 Audio");
	case STREAM_TYPE_MPEG2_AUDIO:					return TEXT("MPEG-2 Audio");
	case STREAM_TYPE_PRIVATE_SECTIONS:				return TEXT("private_sections");
	case STREAM_TYPE_PRIVATE_DATA:					return TEXT("private data");
	case STREAM_TYPE_MHEG:							return TEXT("MHEG");
	case STREAM_TYPE_DSM_CC:						return TEXT("DSM-CC");
	case STREAM_TYPE_ITU_T_REC_H222_1:				return TEXT("H.222.1");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_A:		return TEXT("ISO/IEC 13818-6 type A");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_B:		return TEXT("ISO/IEC 13818-6 type B");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_C:		return TEXT("ISO/IEC 13818-6 type C");
	case STREAM_TYPE_ISO_IEC_13818_6_TYPE_D:		return TEXT("ISO/IEC 13818-6 type D");
	case STREAM_TYPE_ISO_IEC_13818_1_AUXILIARY:		return TEXT("auxiliary");
	case STREAM_TYPE_AAC:							return TEXT("AAC");
	case STREAM_TYPE_MPEG4_VISUAL:					return TEXT("MPEG-4 Visual");
	case STREAM_TYPE_MPEG4_AUDIO:					return TEXT("MPEG-4 Audio");
	case STREAM_TYPE_ISO_IEC_14496_1_IN_PES:		return TEXT("ISO/IEC 14496-1 in PES packets");
	case STREAM_TYPE_ISO_IEC_14496_1_IN_SECTIONS:	return TEXT("ISO/IEC 14496-1 in ISO/IEC 14496_sections");
	case STREAM_TYPE_ISO_IEC_13818_6_DOWNLOAD:		return TEXT("ISO/IEC 13818-6 Synchronized Download Protocol");
	case STREAM_TYPE_METADATA_IN_PES:				return TEXT("Metadata in PES packets");
	case STREAM_TYPE_METADATA_IN_SECTIONS:			return TEXT("Metadata in metadata_sections");
	case STREAM_TYPE_METADATA_IN_DATA_CAROUSEL:		return TEXT("Metadata in ISO/IEC 13818-6 Data Carousel");
	case STREAM_TYPE_METADATA_IN_OBJECT_CAROUSEL:	return TEXT("Metadata in ISO/IEC 13818-6 Object Carousel");
	case STREAM_TYPE_METADATA_IN_DOWNLOAD_PROTOCOL:	return TEXT("Metadata in ISO/IEC 13818-6 Synchronized Download Protocol");
	case STREAM_TYPE_IPMP:							return TEXT("IPMP");
	case STREAM_TYPE_H264:							return TEXT("H.264");
	case STREAM_TYPE_H265:							return TEXT("H.265");
	case STREAM_TYPE_USER_PRIVATE:					return TEXT("user private");
	case STREAM_TYPE_AC3:							return TEXT("AC-3");
	case STREAM_TYPE_DTS:							return TEXT("DTS");
	case STREAM_TYPE_TRUEHD:						return TEXT("TrueHD");
	case STREAM_TYPE_DOLBY_DIGITAL_PLUS:			return TEXT("Dolby Digital Plus");
	}
	return NULL;
}


LPCTSTR GetAreaText(const WORD AreaCode)
{
	switch (AreaCode) {
	// �L�敄��
	case 0x5A5:	return TEXT("�֓��L�挗");
	case 0x72A:	return TEXT("�����L�挗");
	case 0x8D5:	return TEXT("�ߋE�L�挗");
	case 0x699:	return TEXT("����E������");
	case 0x553:	return TEXT("���R�E���쌗");
	// ���敄��
	case 0x16B:	return TEXT("�k�C��");
	case 0x467:	return TEXT("�X");
	case 0x5D4:	return TEXT("���");
	case 0x758:	return TEXT("�{��");
	case 0xAC6:	return TEXT("�H�c");
	case 0xE4C:	return TEXT("�R�`");
	case 0x1AE:	return TEXT("����");
	case 0xC69:	return TEXT("���");
	case 0xE38:	return TEXT("�Ȗ�");
	case 0x98B:	return TEXT("�Q�n");
	case 0x64B:	return TEXT("���");
	case 0x1C7:	return TEXT("��t");
	case 0xAAC:	return TEXT("����");
	case 0x56C:	return TEXT("�_�ސ�");
	case 0x4CE:	return TEXT("�V��");
	case 0x539:	return TEXT("�x�R");
	case 0x6A6:	return TEXT("�ΐ�");
	case 0x92D:	return TEXT("����");
	case 0xD4A:	return TEXT("�R��");
	case 0x9D2:	return TEXT("����");
	case 0xA65:	return TEXT("��");
	case 0xA5A:	return TEXT("�É�");
	case 0x966:	return TEXT("���m");
	case 0x2DC:	return TEXT("�O�d");
	case 0xCE4:	return TEXT("����");
	case 0x59A:	return TEXT("���s");
	case 0xCB2:	return TEXT("���");
	case 0x674:	return TEXT("����");
	case 0xA93:	return TEXT("�ޗ�");
	case 0x396:	return TEXT("�a�̎R");
	case 0xD23:	return TEXT("����");
	case 0x31B:	return TEXT("����");
	case 0x2B5:	return TEXT("���R");
	case 0xB31:	return TEXT("�L��");
	case 0xB98:	return TEXT("�R��");
	case 0xE62:	return TEXT("����");
	case 0x9B4:	return TEXT("����");
	case 0x19D:	return TEXT("���Q");
	case 0x2E3:	return TEXT("���m");
	case 0x62D:	return TEXT("����");
	case 0x959:	return TEXT("����");
	case 0xA2B:	return TEXT("����");
	case 0x8A7:	return TEXT("�F�{");
	case 0xC8D:	return TEXT("�啪");
	case 0xD1C:	return TEXT("�{��");
	case 0xD45:	return TEXT("������");
	case 0x372:	return TEXT("����");
	}
	return NULL;
}


}	// namespace TsEngine
