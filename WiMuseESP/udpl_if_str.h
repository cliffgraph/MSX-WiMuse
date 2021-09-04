// UDPL

#pragma once
#include<string>


// WiFi PLL Mother、IoThief双方のUDPポート番号、
const uint16_t WPM_UDPPORT = 50003;

// WiFi PLL Mother ボードの情報
struct UDPL_BOARD_INFO
{
	uint32_t	version;				// ファームバージョン
	uint16_t	listen_port;			// 受信ポート番号
	uint16_t	send_port;				// 送信ポート番号
	uint16_t	wait;					// コマンド送信遅延[ms]
	uint32_t	bufsize;				// ボードのバッファサイズ[byte]
	uint8_t		name[32];				// ボードのニックネーム
	uint32_t	protocol;				// プロトコル
	char		board_address_ipv6[INET6_ADDRSTRLEN+1];	// ボードの ipv6	// INET6_ADDRSTRLEN (65) 以上
	char		board_address_ipv4[INET_ADDRSTRLEN+1];	// ボードの ipv6	// INET_ADDRSTRLEN (22) 以上
	uint32_t	slotnum;				// 総スロット数
	uint8_t		hbcycle;
	//
	UDPL_BOARD_INFO() :
		version(0), listen_port(0), send_port(0), wait(0), bufsize(0),
		protocol(0), slotnum(0), hbcycle(0)
	{
		return;
	}
};

// WiFi PLL Motherから受け取った音源モジュール設定
struct UDPL_CHIP_INFO
{
	bool		variable_clock;		// PLL クロックをつかってるか true=使ってる
	bool		hardware_reset;		// ハードウェアリセットかけるか true=使う
	uint32_t	clock;				// クロック[Hz]
	uint8_t		name[32];			// チップ名（chipname.h で定義している名称と同じもの）
	uint8_t		extinfo[32];		// 追加情報
	uint32_t	slotNo;				// 実装されているSLOTの番号（１～）
	const UDPL_BOARD_INFO *pBoard;	// このモジュールが載っているWiFi PLL Mother ボード情報へのポインタ
	//
	UDPL_CHIP_INFO() :
		variable_clock(false), hardware_reset(false), clock(0), slotNo(0), pBoard(nullptr)
	{
		return;
	}
};

enum INTERFACE_PROTOCOL : uint32_t
{	// プロトコル
	IPV6UDP = 0,	// IPv6 UDP
	IPV4UDP,		// IPv4 UDP
	IPV6TCP,		// IPv6 TCP
	IPV4TCP			// IPv4 TCP
};

struct YM2610_PCMDATA
{	// YM2610/B+H 用 ADPCM データ情報
	uint8_t *pdata;	// ADPCM データへのポインタ
	uint32_t ch;	// ADPCM-A か ADPCM-B か
	uint32_t size;	// データサイズ
	uint32_t offset;	// オフセット
};

enum : uint32_t
{	// ADPCM-A と ADPCM-B の指定
	ADPCM_A_CH = 1,	// ADPCM-A チャンネル
	ADPCM_B_CH = 0	// ADPCM-B チャンネル
};

enum : unsigned int
{	// 電子ボリュームで制御するアナログ入出力
	BOARD_OUTPUT = 0,	// 出力
	BOARD_INPUT			// 入力
};

enum : unsigned int
{	// 電子ボリュームで制御するステレオチャンネル
	LEFT_CH = 0,		// 左
	RIGHT_CH,			// 右
	STEREO_CH			// 左右両方
};

enum SETTING_LOCATION : uint8_t {
  SETTING_BOARD = 0,
  SETTING_SLOT_0,
  SETTING_SLOT_1,
  SETTING_SLOT_2,
  SETTING_SLOT_3
};

enum SETTING_BOARD_ITEM : uint8_t {
  SETTING_BOARD_VERSION = 0,
  SETTING_BOARD_SLOTNUM,
  SETTING_BOARD_ENABLE,
  SETTING_BOARD_LISTEN_PORT,
  SETTING_BOARD_WAIT,
  SETTING_BOARD_BUFSIZE,
  SETTING_BOARD_NAME,
  SETTING_BOARD_IPV4,
  SETTING_BOARD_IPV6,
  SETTING_BOARD_MDNS,
  SETTING_BOARD_SEND_PORT,
  SETTING_BOARD_HBCYCLE,
  SETTING_BOARD_I2C_ADDR
};

enum SETTING_CHIP_ITEM : uint8_t {
  SETTING_CHIP_ENABLE = 0,
  SETTING_CHIP_PLLEN,
  SETTING_CHIP_HWRESET,
  SETTING_CHIP_CLOCK,
  SETTING_CHIP_NAME,
  SETTING_CHIP_EXINFO
};

enum UDPL_COMMAND : uint8_t {
	CD_CMD_INQUIRY		= 0x80,			//!< インターフェイス問い合わせ
	CD_CMD_READ_SETTING	= 0x40,			//!< 設定読み込み
	CD_CMD_SET_CLOCK	= 0x08,			//!< クロック設定
	CD_CMD_WAIT			= 0x01,			//!< ウエイト
	CD_CMD_RESET		= 0x90,			//!< リセット
	CD_CMD_SET_REG		= 0x00,			//!< レジスタライト
	CD_CMD_GET_REG		= 0x04,			//!< レジスタリード
	CD_CMD_SET_BUS		= 0x02,			//!< レジスタリード
	CD_CMD_SET_PORT		= 0x50,			//!< 応答ポートの設定
	CD_CMD_CHECK_I2C	= 0x40,			//!< I2C デバイスの有無をチェック
	CD_CMD_WRITE_I2C	= 0x30,			//!< 
	CD_CMD_READ_I2C		= 0x20,			//!< 
};

#pragma pack(push, 1)	//  1[byte] (packing alignment for structure)

// インターフェース確認
struct CMD_INQUIRY
{
	uint8_t	payload;
	uint8_t	command;
	//
	CMD_INQUIRY() : 
		payload(0x01), command(CD_CMD_INQUIRY)
	{
		return;
	}
};

// CMD_INQUIRY に対する WiFiPLLMotherからの応答
struct RES_INQUIRY
{
	uint8_t	payload;
	uint8_t	command;
	uint8_t	data[2];
	RES_INQUIRY() 
	{
		return;
	}
	bool IsOK() const
	{
		if( payload == 0x03 &&
			command == CD_CMD_INQUIRY &&
			data[0] == 'O' && data[1] == 'K' ) {
			return true;
		}
		return false;
	}

};

struct CMD_HEADER
{
	uint8_t	payload;
	uint8_t	command;
};

// 設定の読み出し
struct CMD_READ_SETTING
{
	uint8_t				payload;	// 0x03+k
	uint8_t				command;	// CD_CMD_READ_SETTING
	SETTING_LOCATION	location;	// 取得対象 SETTING_LOCATION
	uint8_t				item;		// 取得項目の指定
	uint8_t				params[1];	// 付加情報（可変長 k）
	//
	CMD_READ_SETTING(const SETTING_LOCATION tg, const uint8_t it) :
		payload(0x03), command(CD_CMD_READ_SETTING),
		location(tg), item(it)
	{
		return;
	}
};

// 設定の読み出し（返信）
struct REP_READ_SETTING
{
	uint8_t	payload;	// 0x03+k
	uint8_t	command;	// CD_CMD_READ_SETTING
	uint8_t	location;	// 取得対象 0=ボード、1-4=Slot0-3
	uint8_t	item;		// 取得項目の指定
	uint8_t	data[1];	// 付加情報（可変長 k）
};

// レジスタ書き込み
struct CMD_SET_REG
{
	uint8_t	payload;
	uint8_t	command;
	uint8_t	slot_a3a0;	// bit7-4:[slot number], bit3-0:[A3][A2][A1][A0]
	uint8_t	address;
	uint8_t	data;
	uint8_t	wait1;
	uint8_t	wait2;
	//
	CMD_SET_REG(
		const uint8_t sa, const uint8_t addr, const uint8_t dt, const uint8_t w1, const uint8_t w2 ) :
			payload(0x06), command(CD_CMD_SET_REG),
			slot_a3a0(sa), address(addr), data(dt), wait1(w1), wait2(w2)
	{
		return;
	}
};

// レジスタ読み出し
struct CMD_GET_REG
{
	uint8_t	payload;
	uint8_t	command;
	uint8_t	slot_a3a0;	// bit7-4:[slot number], bit3-0:[A3][A2][A1][A0]
	uint8_t	address;
	uint8_t	wait;
	//
	CMD_GET_REG(
		const uint8_t sa, const uint8_t addr, const uint8_t w) :
			payload(0x04), command(CD_CMD_GET_REG),
			slot_a3a0(sa), address(addr), wait(w)
	{
		return;
	}
};

// レジスタ読み出し（返信）
struct REP_GET_REG
{
	uint8_t	payload;	// 0x06
	uint8_t	command;	// CD_CMD_GET_REG
	uint8_t	slot;		// slot no.
	uint8_t	data;
};

// バス書き込み
struct CMD_SET_BUS
{
	uint8_t	payload;
	uint8_t	command;
	uint8_t	slot_a3a0;	// bit7-4:[slot number], bit3-0:[A3][A2][A1][A0]
	uint8_t	data;
	uint8_t	wait;
	//
	CMD_SET_BUS() :
			payload(0x04),command(0x02),
			slot_a3a0(0), data(0), wait(0)
	{
		return;
	}

	CMD_SET_BUS(
		const uint8_t sa, const uint8_t dt, const uint8_t w) :
			payload(0x04),command(CD_CMD_SET_BUS),
			slot_a3a0(sa), data(dt), wait(w)
	{
		return;
	}
};

// インターフェースリセット
struct CMD_RESET
{
	uint8_t	payload;
	uint8_t	command;
	uint8_t	slot_bitmap; // 対象スロットをbitで指定する。
						 //	bit3: slot3
						 //	   2: slot2
						 //    1: slot1
			 			 //    0: slot0
	CMD_RESET() : 
		payload(0x02), command(CD_CMD_RESET), slot_bitmap(0x0f)
	{
		return;
	}
};

// CMD_RESET に対する WiFiPLLMotherからの応答
struct RES_RESET
{
	uint8_t	payload;
	uint8_t	command;
	uint8_t	data[2];
	RES_RESET() : payload(0), command(0) 
	{
		data[0] = 0x00;
		data[1] = 0x00;
		return;
	}
	bool IsOK() const
	{
		if( payload == 0x03 &&
			command == CD_CMD_RESET &&
			data[0] == 'O' &&
			data[1] == 'K' ){
			return true;
		}
		return false;
	}
};

// ウェイト
struct CMD_WAIT
{
	uint8_t	payload;
	uint8_t	command;
	uint8_t	mode;		// 0:us単位, 1:ms単位
	uint8_t count[2];	// ウェイト時間値（リトルエンディアン）
	//
	enum MODE : uint8_t { MICRO=0, MILLI=1 };
	//
	CMD_WAIT(const MODE m, const uint16_t c) :
		payload(0x04), command(0x01), mode(m)
	{
		count[0] = (c >> 0) & 0xff;
		count[1] = (c >> 8) & 0xff;
		return;
	}
};

#pragma pack(pop)

inline void makeSetReg(
	CMD_SET_REG *p,
	const uint8_t slot, const uint8_t addr, const uint8_t data, const uint8_t w1, const uint8_t w2)
{
	p->payload = 6;
	p->command = CD_CMD_SET_REG;
	p->slot_a3a0 = slot;
	p->address = addr;
	p->data = data;
	p->wait1 = w1;
	p->wait2 = w2;
	return;
}
