// UDPL
#pragma once

// スロットピンと、ESP32のピンの対応
static uint8_t MSX_ESP_REQ	= 17;
static uint8_t MSX_SLOT_C2	= 13;
static uint8_t MSX_SLOT_C1	= 14;
static uint8_t MSX_SLOT_C0	= 12;
static uint8_t MSX_SLOT_A2	= 5;
static uint8_t MSX_SLOT_A1	= 4;
static uint8_t MSX_SLOT_A0	= 2;
// 
static uint8_t MSX_SLOT_D7	= 16;
static uint8_t MSX_SLOT_D6	= 15;
static uint8_t MSX_SLOT_D5	= 27;
static uint8_t MSX_SLOT_D4	= 26;
static uint8_t MSX_SLOT_D3	= 25;
static uint8_t MSX_SLOT_D2	= 23;
static uint8_t MSX_SLOT_D1	= 19;
static uint8_t MSX_SLOT_D0	= 18;
//
#define TEST_AD	 (0x00)
#define TEST_DT	 (0x01)
#define OPLL_AD	 (0x02)
#define OPLL_DT	 (0x03)
#define PSG_AD	 (0x04)
#define PSG_DT	 (0x05)
#define Y8950_AD (0x06)
#define Y8950_DT (0x07)


inline uint8_t getUpdateCode(const uint32_t m1)
{
  return
      ((((m1>>MSX_SLOT_C0) & 0x01 ) << 0) | 
       (((m1>>MSX_SLOT_C1) & 0x01 ) << 1) |
       (((m1>>MSX_SLOT_C2) & 0x01 ) << 2)) & 0x07;
}

inline uint8_t getAddressCode(const uint32_t m1)
{
  return
      ((((m1>>MSX_SLOT_A0) & 0x01 ) << 0) | 
       (((m1>>MSX_SLOT_A1) & 0x01 ) << 1) |
       (((m1>>MSX_SLOT_A2) & 0x01 ) << 2)) & 0x07;
}

inline uint8_t getDataBus(const uint32_t m1)
{
  return
      ((((m1>>MSX_SLOT_D0) & 0x01 ) << 0) | 
       (((m1>>MSX_SLOT_D1) & 0x01 ) << 1) | 
       (((m1>>MSX_SLOT_D2) & 0x01 ) << 2) | 
       (((m1>>MSX_SLOT_D3) & 0x01 ) << 3) | 
       (((m1>>MSX_SLOT_D4) & 0x01 ) << 4) | 
       (((m1>>MSX_SLOT_D5) & 0x01 ) << 5) | 
       (((m1>>MSX_SLOT_D6) & 0x01 ) << 6) |
       (((m1>>MSX_SLOT_D7) & 0x01 ) << 7)) & 0xff;
}

inline void setupGpio()
{
	const int pins[] = {
		MSX_SLOT_C2, MSX_SLOT_C1, MSX_SLOT_C0,
		MSX_SLOT_A1, MSX_SLOT_A0,
		MSX_SLOT_D7, MSX_SLOT_D6, MSX_SLOT_D5, MSX_SLOT_D4,
		MSX_SLOT_D3, MSX_SLOT_D2, MSX_SLOT_D1, MSX_SLOT_D0, };
	const int num = static_cast<int>(sizeof(pins)/sizeof(pins[0]));
	for( int t = 0; t < num; ++t) {
		pinMode(pins[t], INPUT);
	}
	//
	pinMode(MSX_ESP_REQ, OUTPUT);
	
}
