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
#define STREAM_TYPE_MPEG1			0x01	// MPEG-1
#define STREAM_TYPE_MPEG2			0x02	// MPEG-2
#define STREAM_TYPE_CAPTION			0x06	// 字幕
#define STREAM_TYPE_DATACARROUSEL	0x0D	// データ放送
#define STREAM_TYPE_AAC				0x0F	// AAC
#define STREAM_TYPE_H264			0x1B	// H.264
#define STREAM_TYPE_INVALID			0xFF	// 無効

// service_type
#define SERVICE_TYPE_DIGITALTV				0x01	// デジタルTVサービス
#define SERVICE_TYPE_DIGITALAUDIO			0x02	// デジタル音声サービス
#define SERVICE_TYPE_TEMPORARYVIDEO			0xA1	// 臨時映像サービス
#define SERVICE_TYPE_TEMPORARYAUDIO			0xA2	// 臨時音声サービス
#define SERVICE_TYPE_TEMPORARYDATA			0xA3	// 臨時データサービス
#define SERVICE_TYPE_ENGINEERING			0xA4	// エンジニアリングサービス
#define SERVICE_TYPE_PROMOTIONVIDEO			0xA5	// プロモーション映像サービス
#define SERVICE_TYPE_PROMOTIONAUDIO			0xA6	// プロモーション音声サービス
#define SERVICE_TYEP_PROMOTIONDATA			0xA7	// プロモーションデータサービス
#define SERVICE_TYPE_ACCUMULATIONDATA		0xA8	// 事前蓄積用データサービス
#define SERVICE_TYPE_ACCUMULATIONONLYDATA	0xA9	// 蓄積専用データサービス
#define SERVICE_TYPE_BOOKMARKLISTDATA		0xAA	// ブックマーク一覧データサービス
#define SERVICE_TYPE_DATA					0xC0	// データサービス
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


#endif
