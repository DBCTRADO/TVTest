// Common.h: BonTsEngine共通ヘッダ
//
//////////////////////////////////////////////////////////////////////

#ifndef BONTSENGINE_COMMON_H
#define BONTSENGINE_COMMON_H


// 抽象クラス用
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

#define STREAM_TYPE_UNINITIALIZED					0x00	// 未初期化
#define STREAM_TYPE_INVALID							0xFF	// 無効

#define STREAM_TYPE_CAPTION							STREAM_TYPE_PRIVATE_DATA			// 字幕
#define STREAM_TYPE_DATACARROUSEL					STREAM_TYPE_ISO_IEC_13818_6_TYPE_D	// データ放送

// service_type
#define SERVICE_TYPE_DIGITALTV				0x01	// デジタルTVサービス
#define SERVICE_TYPE_DIGITALAUDIO			0x02	// デジタル音声サービス
// 0x03 - 0x7F 未定義
// 0x80 - 0xA0 事業者定義
#define SERVICE_TYPE_TEMPORARYVIDEO			0xA1	// 臨時映像サービス
#define SERVICE_TYPE_TEMPORARYAUDIO			0xA2	// 臨時音声サービス
#define SERVICE_TYPE_TEMPORARYDATA			0xA3	// 臨時データサービス
#define SERVICE_TYPE_ENGINEERING			0xA4	// エンジニアリングサービス
#define SERVICE_TYPE_PROMOTIONVIDEO			0xA5	// プロモーション映像サービス
#define SERVICE_TYPE_PROMOTIONAUDIO			0xA6	// プロモーション音声サービス
#define SERVICE_TYPE_PROMOTIONDATA			0xA7	// プロモーションデータサービス
#define SERVICE_TYPE_ACCUMULATIONDATA		0xA8	// 事前蓄積用データサービス
#define SERVICE_TYPE_ACCUMULATIONONLYDATA	0xA9	// 蓄積専用データサービス
#define SERVICE_TYPE_BOOKMARKLISTDATA		0xAA	// ブックマーク一覧データサービス
#define SERVICE_TYPE_SERVERTYPESIMULTANEOUS	0xAB	// サーバー型サイマルサービス
#define SERVICE_TYPE_INDEPENDENTFILE		0xAC	// 独立ファイルサービス
#define SERVICE_TYPE_4KTV					0xAD	// 超高精細度4K専用TVサービス
// 0xAD - 0xBF 未定義(標準化機関定義領域)
#define SERVICE_TYPE_DATA					0xC0	// データサービス
#define SERVICE_TYPE_TLVACCUMULATION		0xC1	// TLVを用いた蓄積型サービス
#define SERVICE_TYPE_MULTIMEDIA				0xC2	// マルチメディアサービス
// 0xC3 - 0xFF 未定義
#define SERVICE_TYPE_INVALID				0xFF	// 無効

// ISO 639 language code
#define LANGUAGE_CODE_JPN	0x6A706EUL	// 日本語
#define LANGUAGE_CODE_ENG	0x656E67UL	// 英語
#define LANGUAGE_CODE_DEU	0x646575UL	// ドイツ語
#define LANGUAGE_CODE_FRA	0x667261UL	// フランス語
#define LANGUAGE_CODE_ITA	0x697461UL	// イタリア語
#define LANGUAGE_CODE_RUS	0x727573UL	// ロシア語
#define LANGUAGE_CODE_ZHO	0x7A686FUL	// 中国語
#define LANGUAGE_CODE_KOR	0x6B6F72UL	// 韓国語
#define LANGUAGE_CODE_SPA	0x737061UL	// スペイン語
#define LANGUAGE_CODE_ETC	0x657463UL	// その他

// ワンセグPMT PID
#define ONESEG_PMT_PID_FIRST	0x1FC8
#define ONESEG_PMT_PID_LAST		0x1FCF
#define ONESEG_PMT_PID_NUM		(ONESEG_PMT_PID_LAST - ONESEG_PMT_PID_FIRST + 1)

inline bool Is1SegPmtPid(WORD Pid) {
	return (Pid >= ONESEG_PMT_PID_FIRST) && (Pid <= ONESEG_PMT_PID_LAST);
}


#endif
