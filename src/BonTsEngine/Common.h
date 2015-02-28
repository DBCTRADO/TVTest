// Common.h: BonTsEngine���ʃw�b�_
//
//////////////////////////////////////////////////////////////////////

#ifndef BONTSENGINE_COMMON_H
#define BONTSENGINE_COMMON_H


// ���ۃN���X�p
#define ABSTRACT_CLASS_DECL __declspec(novtable)

// PID
#define PID_PAT		0x0000U	// PAT
#define PID_CAT		0x0001U	// CAT
#define PID_NIT		0x0010U	// NIT
#define PID_SDT		0x0011U	// SDT
#define PID_HEIT	0x0012U	// H-EIT
#define PID_TOT		0x0014U	// TOT
#define PID_SDTT	0x0023U	// SDTT
#define PID_BIT		0x0024U	// BIT
#define PID_MEIT	0x0026U	// M-EIT
#define PID_LEIT	0x0027U	// L-EIT
#define PID_CDT		0x0029U	// CDT

// stream_type
#define STREAM_TYPE_MPEG1_VIDEO						0x01	// ISO/IEC 11172-2 Video
#define STREAM_TYPE_MPEG2_VIDEO						0x02	// ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
#define STREAM_TYPE_MPEG1_AUDIO						0x03	// ISO/IEC 11172-3 Audio
#define STREAM_TYPE_MPEG2_AUDIO						0x04	// ISO/IEC 13818-3 Audio
#define STREAM_TYPE_PRIVATE_SECTIONS				0x05	// ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections
#define STREAM_TYPE_PRIVATE_DATA					0x06	// ITU-T Rec. H.222.0 | ISO/IEC 13818-1 PES packets containing private data
#define STREAM_TYPE_MHEG							0x07	// ISO/IEC 13522 MHEG
#define STREAM_TYPE_DSM_CC							0x08	// ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC
#define STREAM_TYPE_ITU_T_REC_H222_1				0x09	// ITU-T Rec. H.222.1
#define STREAM_TYPE_ISO_IEC_13818_6_TYPE_A			0x0A	// ISO/IEC 13818-6 type A
#define STREAM_TYPE_ISO_IEC_13818_6_TYPE_B			0x0B	// ISO/IEC 13818-6 type B
#define STREAM_TYPE_ISO_IEC_13818_6_TYPE_C			0x0C	// ISO/IEC 13818-6 type C
#define STREAM_TYPE_ISO_IEC_13818_6_TYPE_D			0x0D	// ISO/IEC 13818-6 type D
#define STREAM_TYPE_ISO_IEC_13818_1_AUXILIARY		0x0E	// ITU-T Rec. H.222.0 | ISO/IEC 13818-1 auxiliary
#define STREAM_TYPE_AAC								0x0F	// ISO/IEC 13818-7 Audio with ADTS transport syntax
#define STREAM_TYPE_MPEG4_VISUAL					0x10	// ISO/IEC 14496-2 Visual
#define STREAM_TYPE_MPEG4_AUDIO						0x11	// ISO/IEC 14496-3 Audio with the LATM transport syntax as defined in ISO/IEC 14496-3 / AMD 1
#define STREAM_TYPE_ISO_IEC_14496_1_IN_PES			0x12	// ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in PES packets
#define STREAM_TYPE_ISO_IEC_14496_1_IN_SECTIONS		0x13	// ISO/IEC 14496-1 SL-packetized stream or FlexMux stream carried in ISO/IEC 14496_sections
#define STREAM_TYPE_ISO_IEC_13818_6_DOWNLOAD		0x14	// ISO/IEC 13818-6 Synchronized Download Protocol
#define STREAM_TYPE_METADATA_IN_PES					0x15	// Metadata carried in PES packets
#define STREAM_TYPE_METADATA_IN_SECTIONS			0x16	// Metadata carried in metadata_sections
#define STREAM_TYPE_METADATA_IN_DATA_CAROUSEL		0x17	// Metadata carried in ISO/IEC 13818-6 Data Carousel
#define STREAM_TYPE_METADATA_IN_OBJECT_CAROUSEL		0x18	// Metadata carried in ISO/IEC 13818-6 Object Carousel
#define STREAM_TYPE_METADATA_IN_DOWNLOAD_PROTOCOL	0x19	// Metadata carried in ISO/IEC 13818-6 Synchronized Download Protocol
#define STREAM_TYPE_IPMP							0x1A	// ISO/IEC 13818-11 IPMP on MPEG-2 systems
#define STREAM_TYPE_H264							0x1B	// ITU-T Rec. H.264 | ISO/IEC 14496-10 Video
#define STREAM_TYPE_H265							0x24	// ITU-T Rec. H.265 | ISO/IEC 23008-2
#define STREAM_TYPE_USER_PRIVATE					0x80	// ISO/IEC User Private
#define STREAM_TYPE_AC3								0x81	// Dolby AC-3
#define STREAM_TYPE_DTS								0x82	// DTS
#define STREAM_TYPE_TRUEHD							0x83	// Dolby TrueHD
#define STREAM_TYPE_DOLBY_DIGITAL_PLUS				0x87	// Dolby Digital Plus

#define STREAM_TYPE_UNINITIALIZED					0x00	// ��������
#define STREAM_TYPE_INVALID							0xFF	// ����

#define STREAM_TYPE_CAPTION							STREAM_TYPE_PRIVATE_DATA			// ����
#define STREAM_TYPE_DATACARROUSEL					STREAM_TYPE_ISO_IEC_13818_6_TYPE_D	// �f�[�^����

// service_type
#define SERVICE_TYPE_DIGITALTV				0x01	// �f�W�^��TV�T�[�r�X
#define SERVICE_TYPE_DIGITALAUDIO			0x02	// �f�W�^�������T�[�r�X
// 0x03 - 0x7F ����`
// 0x80 - 0xA0 ���ƎҒ�`
#define SERVICE_TYPE_TEMPORARYVIDEO			0xA1	// �Վ��f���T�[�r�X
#define SERVICE_TYPE_TEMPORARYAUDIO			0xA2	// �Վ������T�[�r�X
#define SERVICE_TYPE_TEMPORARYDATA			0xA3	// �Վ��f�[�^�T�[�r�X
#define SERVICE_TYPE_ENGINEERING			0xA4	// �G���W�j�A�����O�T�[�r�X
#define SERVICE_TYPE_PROMOTIONVIDEO			0xA5	// �v�����[�V�����f���T�[�r�X
#define SERVICE_TYPE_PROMOTIONAUDIO			0xA6	// �v�����[�V���������T�[�r�X
#define SERVICE_TYPE_PROMOTIONDATA			0xA7	// �v�����[�V�����f�[�^�T�[�r�X
#define SERVICE_TYPE_ACCUMULATIONDATA		0xA8	// ���O�~�ϗp�f�[�^�T�[�r�X
#define SERVICE_TYPE_ACCUMULATIONONLYDATA	0xA9	// �~�ϐ�p�f�[�^�T�[�r�X
#define SERVICE_TYPE_BOOKMARKLISTDATA		0xAA	// �u�b�N�}�[�N�ꗗ�f�[�^�T�[�r�X
#define SERVICE_TYPE_SERVERTYPESIMULTANEOUS	0xAB	// �T�[�o�[�^�T�C�}���T�[�r�X
#define SERVICE_TYPE_INDEPENDENTFILE		0xAC	// �Ɨ��t�@�C���T�[�r�X
#define SERVICE_TYPE_4KTV					0xAD	// �������דx4K��pTV�T�[�r�X
// 0xAD - 0xBF ����`(�W�����@�֒�`�̈�)
#define SERVICE_TYPE_DATA					0xC0	// �f�[�^�T�[�r�X
#define SERVICE_TYPE_TLVACCUMULATION		0xC1	// TLV��p�����~�ό^�T�[�r�X
#define SERVICE_TYPE_MULTIMEDIA				0xC2	// �}���`���f�B�A�T�[�r�X
// 0xC3 - 0xFF ����`
#define SERVICE_TYPE_INVALID				0xFF	// ����

// ISO 639 language code
#define LANGUAGE_CODE_JPN	0x6A706EUL	// ���{��
#define LANGUAGE_CODE_ENG	0x656E67UL	// �p��
#define LANGUAGE_CODE_DEU	0x646575UL	// �h�C�c��
#define LANGUAGE_CODE_FRA	0x667261UL	// �t�����X��
#define LANGUAGE_CODE_ITA	0x697461UL	// �C�^���A��
#define LANGUAGE_CODE_RUS	0x727573UL	// ���V�A��
#define LANGUAGE_CODE_ZHO	0x7A686FUL	// ������
#define LANGUAGE_CODE_KOR	0x6B6F72UL	// �؍���
#define LANGUAGE_CODE_SPA	0x737061UL	// �X�y�C����
#define LANGUAGE_CODE_ETC	0x657463UL	// ���̑�

// �����Z�OPMT PID
#define ONESEG_PMT_PID_FIRST	0x1FC8
#define ONESEG_PMT_PID_LAST		0x1FCF
#define ONESEG_PMT_PID_NUM		(ONESEG_PMT_PID_LAST - ONESEG_PMT_PID_FIRST + 1)

inline bool Is1SegPmtPid(WORD Pid) {
	return (Pid >= ONESEG_PMT_PID_FIRST) && (Pid <= ONESEG_PMT_PID_LAST);
}


#endif
