// "MSX-WiMuse" MSX WirelessLAN Music
// WiMuse ESP32-unit by @Harumakkin
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncUDP.h>

// Library: "ESP8266 and ESP32 OLED driver for SSD1306 displays ver 4.2.0"
// SSD1306: 128,63
// C:\Users\%USERNAME%\Documents\Arduino\libraries\ESP8266_and_ESP32_OLED_driver_for_SSD1306_displays\src
#include "Wire.h"		//I2C通信用ライブラリを読み込み
#include "SSD1306.h"	//ディスプレイ用ライブラリを読み込み

#include "udpl_if_str.h"
#include "iopins.h"

#define SOFTAP					// 定義＝アクセスポイントとして動作する
//#define OUTPUT_SERIAL_MESS
#define CHECK_UPDATECODE


static const char *APP_NAME = "MSX WiMu 1.1";
static const char *APP_SIGN = "@Harumakkin";

#ifdef SOFTAP
// アクセスポイントモードのSSDIとパスワード
static const char *SOFTAP_SSID = "MSXWIMUSE1";
static const char *SOFTAP_PASWD = "123456MSX";
#else
// 外部アクセスポイントに接続して通信する場合は、そのアクセスポイントのSSDIとパスワード
static const char *AP_SSID = "SSID";
static const char *AP_PASWD = "PASSWORD";
#endif

portMUX_TYPE g_Mut_InData = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE g_Mut_InDataNum = portMUX_INITIALIZER_UNLOCKED;

//---------------------------------------------------
struct INDATA
{
	volatile uint64_t  ck;
	volatile uint32_t  pin;
};

struct INDATASTR
{
	const static int SIZE = 256*2;
	volatile int num;
	volatile int top, btm;
	INDATA recs[SIZE];
	INDATASTR() : num(0), top(0), btm(0){return;}
};
INDATASTR g_InData;


//---------------------------------------------------
void IRAM_ATTR isr()
{
	// g_InData.top が bottom を追い抜くことはチェックしない。
	// その時はその時であきらめる

	// TangNanoからの1bytesを読み込む
	const uint32_t gpioDt = REG_READ(GPIO_IN_REG);

	digitalWrite(MSX_ESP_REQ, HIGH);

	INDATA &dt = g_InData.recs[g_InData.top];
	if( g_InData.SIZE <= ++g_InData.top )
		g_InData.top = 0;

	dt.ck = micros();
	dt.pin = gpioDt;

	portENTER_CRITICAL_ISR(&g_Mut_InDataNum);
	++g_InData.num;
	portEXIT_CRITICAL_ISR(&g_Mut_InDataNum);

	// TangNanoに対して次の1bytesを要求する
	digitalWrite(MSX_ESP_REQ, LOW);
	return;
}

//---------------------------------------------------
SSD1306  g_Lcd(0x3c, 21, 22); 	//SSD1306 作成（I2Cアドレス,SDA,SCL)
int g_lostCount;

static void printStatus(const int lc)
{
	g_Lcd.clear();
	g_Lcd.setFont(ArialMT_Plain_16);
	g_Lcd.drawString(0,0, APP_NAME);
	g_Lcd.setFont(ArialMT_Plain_10);
	g_Lcd.drawString(0,16, APP_SIGN);
	g_Lcd.drawHorizontalLine(0,29,128);
	g_Lcd.setFont(ArialMT_Plain_16);
	g_Lcd.drawString(0,31, "LOST:" + String(lc));
	g_Lcd.display();
	return;
}

//---------------------------------------------------
// WiFi PLL MOTHER
IPv6Address g_WPM_ADDR;
AsyncUDP g_udp;
enum WIFISTATE
{
	WIFISTATE_NONE,
	START_WIFI,
	STANDBY_WIFI,
	LOOKUP_WPM,
	RESET_WPM,
	WIFI_WPM_OK,
};
volatile WIFISTATE g_WiFiState = WIFISTATE_NONE;

void WiFiEvent(WiFiEvent_t event)
{
	switch(event) {
		case SYSTEM_EVENT_AP_START:
		{
			Serial.println("SYSTEM_EVENT_AP_START");
			WiFi.softAPenableIpV6();
			break;
		}
		case SYSTEM_EVENT_GOT_IP6:
		{
#ifdef SOFTAP
			Serial.println("SYSTEM_EVENT_GOT_IP6");
			IPAddress myIPv4 = WiFi.softAPIP();
			Serial.printf("IPv4: %s\n", myIPv4.toString().c_str());
			IPv6Address myIPv6 = WiFi.softAPIPv6();
			Serial.printf("IPv6: %s\n", myIPv6.toString().c_str());
			g_WiFiState = LOOKUP_WPM;
#else
			Serial.println("SYSTEM_EVENT_GOT_IP6");
			IPAddress myIPv4 = WiFi.localIP();
			Serial.printf("IPv4: %s\n", myIPv4.toString().c_str());
			IPv6Address myIPv6 = WiFi.localIPv6();
			Serial.printf("IPv6: %s\n", myIPv6.toString().c_str());
			g_WiFiState = LOOKUP_WPM;
#endif

			break;
		}
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to access point");
            WiFi.enableIpV6();
            break;
		// case SYSTEM_EVENT_STA_GOT_IP:
        //     Serial.print("Obtained IP address: ");
        //     Serial.println(WiFi.localIP());
        //     break;
	}
	return;
}

static const int BUFFSIZE = 9000;
struct SENDBUFF
{
	int     Index;
	uint8_t pBuff[BUFFSIZE];
	uint32_t KickTime;
};
static SENDBUFF g_Buffer[2];
static SENDBUFF *g_pB;
static uint8_t g_Plane;

uint32_t insertWait(const uint32_t def, uint8_t *pBuff, int index)
{
	// 10us 以下は無視する
	if( def <= 10 )
		return index;
	// WAITコマンドを作成
	const uint32_t ms = def / 1000U;
	const uint32_t us = def % 1000U;
	if( ms != 0 ){
		CMD_WAIT *p = reinterpret_cast<CMD_WAIT *>(&pBuff[index]);
		p->payload = 4;
		p->command = CD_CMD_WAIT;
		p->mode = 1; /* ms */
		p->count[0] = (ms >> 0) & 0xff;
		p->count[1] = (ms >> 8) & 0xff;
		index += sizeof(*p);
	}
	if( us != 0 ){
		CMD_WAIT *p = reinterpret_cast<CMD_WAIT *>(&pBuff[index]);
		p->payload = 4;
		p->command = CD_CMD_WAIT;
		p->mode = 0; /* us */
		p->count[0] = (us >> 0) & 0xff;
		p->count[1] = (us >> 8) & 0xff;
		index += sizeof(*p);
	}
	return index;
}

static bool makeWaitCmd(struct SENDBUFF *pB, const uint32_t def)
{
	bool bKick = false;
	if( pB->Index == 0 ){
		bKick = true;
	}
	else { 
		pB->Index = insertWait(def, pB->pBuff, pB->Index);
	}
	return bKick;
}

void steStandByWiFi()
{
	switch(g_WiFiState) {
		case START_WIFI:
		{
#ifdef SOFTAP
	#ifdef OUTPUT_SERIAL_MESS
			Serial.printf("START WiFi(softAP): %s\n", SOFTAP_SSID);
	#endif
			// 自信がアクセスポイントとして動作する
			WiFi.disconnect(true);
			WiFi.mode(WIFI_AP);
			WiFi.onEvent(WiFiEvent);
			WiFi.softAP(SOFTAP_SSID, SOFTAP_PASWD, 3, 0, 1);
			g_udp.listen(WPM_UDPPORT); 
#else
			// アクセスポイントに接続する
			WiFi.disconnect(true);
			WiFi.mode(WIFI_STA);
			WiFi.onEvent(WiFiEvent);
		    WiFi.begin(AP_SSID, AP_PASWD);
			g_udp.listen(WPM_UDPPORT); 
#endif
			g_WiFiState = STANDBY_WIFI;
			break;
		}
		case STANDBY_WIFI:
		{
			// do nothing
			break;
		}
		case LOOKUP_WPM:
		{
#ifdef OUTPUT_SERIAL_MESS
			Serial.print("LOOKUP WiFi PLL Mother board");
#endif
			g_udp.onPacket([](AsyncUDPPacket packet) {
				const uint8_t *pDt= packet.data();
				if( packet.length() == 4 &&
				  pDt[0] == 0x03 && pDt[1] == 0x80 && pDt[2] == 'W' && pDt[3] == 'P' ) {
					g_WPM_ADDR = packet.remoteIPv6();
#ifdef OUTPUT_SERIAL_MESS
 					Serial.printf("\nfound: %s\n", g_WPM_ADDR.toString().c_str());
#endif
					g_WiFiState = RESET_WPM;
				}
			});
			const uint8_t broad[] =
				{0xff,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01};
			IPv6Address addr(broad);
			while (g_WiFiState==LOOKUP_WPM){
#ifdef OUTPUT_SERIAL_MESS
				Serial.print(".");
#endif
				CMD_INQUIRY inq;
				g_udp.writeTo(reinterpret_cast<uint8_t*>(&inq), sizeof inq, addr, WPM_UDPPORT);
				delay(500);
			}
			break;
		}
		case RESET_WPM:
		{
			g_udp.connect(g_WPM_ADDR, WPM_UDPPORT);
			CMD_RESET reset;
			g_udp.write(reinterpret_cast<const uint8_t*>(&reset), sizeof reset);
#ifdef OUTPUT_SERIAL_MESS
			Serial.printf("Send RESET.\n");
#endif
			g_WiFiState = WIFI_WPM_OK;
			//
			attachInterrupt(MSX_SLOT_C0, isr, CHANGE );
			break;
		}
	}
	return;
}

const uint32_t POOLTIME = 3; // ms
const uint32_t POOLTIMEUS = POOLTIME*1000; // us

TaskHandle_t g_taskHandle;
void stsIoThief()
{
	static uint8_t opllAddr = 0, psgAddr = 0;
	static uint8_t y8950Addr = 0, testAddr = 0;
	int num = g_InData.num;
	if( num == 0 )
		return;

	bool bKick = false;
	bool bDisplay = false;
	for( int t = 0; t < num; ++t ){

		INDATA dt = g_InData.recs[g_InData.btm];
		if( g_InData.SIZE <= ++g_InData.btm )
			g_InData.btm = 0;

#ifdef CHECK_UPDATECODE
		const uint8_t newUpdate = getUpdateCode(dt.pin);
		static uint8_t lastUpdate = (newUpdate-1) & 0x07;
		const uint8_t defUpdate = (newUpdate-lastUpdate) & 0x07;
		static uint64_t lastCk1 = 0, lastCk2 = 0;
		if( defUpdate != 1){
#ifdef OUTPUT_SERIAL_MESS
			Serial.printf("old=%d new=%d oldCk2=%lld oldCk1=%lld newCk=%lld\n", lastUpdate, newUpdate, lastCk2, lastCk1, dt.ck);
#endif
			g_lostCount += (defUpdate-1) &  0x07;
			bDisplay = true;
		}
		lastCk2 = lastCk1;
		lastCk1 = dt.ck;
		lastUpdate = newUpdate;
#endif

		const uint8_t ioAddr = getAddressCode(dt.pin);
		const uint8_t ioData = getDataBus(dt.pin);
		static uint64_t lastTime = 0;
		const uint64_t nowTime = dt.ck;
		const uint32_t def = static_cast<uint32_t>((lastTime < nowTime) ? (nowTime - lastTime) : 0);

		switch(ioAddr) {  
			/*PSG A0h */
			case PSG_AD: {
				psgAddr = ioData;
				break;
			}
			/*PSG A1h */
			case PSG_DT: {
				if( 0x0e <= psgAddr )	// 汎用ポートA/Bへのアクセスは無視する
					break;
	#ifdef OUTPUT_SERIAL_MESS
				Serial.printf("PSG %02x:%02x %d %d\n", psgAddr, ioData, g_pB->Index, def);
	#endif
				portENTER_CRITICAL(&g_Mut_InData);
				bKick |= makeWaitCmd(g_pB, def);
				CMD_SET_REG *p = reinterpret_cast<CMD_SET_REG *>(&g_pB->pBuff[g_pB->Index]);
				const uint8_t SLOT = 0x00;
				makeSetReg(p, SLOT, psgAddr, ioData, 0, 0);	/* waitなし */
				g_pB->Index += sizeof(CMD_SET_REG);
				portEXIT_CRITICAL(&g_Mut_InData);
				lastTime = nowTime;
				break;
			}
			/* OPLL 7Ch */
			case OPLL_AD: {
				opllAddr = ioData;
				break;
			}
			/* OPLL 7Dh */
			case OPLL_DT: {
	#ifdef OUTPUT_SERIAL_MESS
				Serial.printf("FM  %02x:%02x %d %d\n", opllAddr, ioData, g_pB->Index, def);
	#endif
				portENTER_CRITICAL(&g_Mut_InData);
				bKick |= makeWaitCmd(g_pB, def);
				CMD_SET_REG *p = reinterpret_cast<CMD_SET_REG *>(&g_pB->pBuff[g_pB->Index]);
				const uint8_t SLOT = 0x10;
				makeSetReg(p, SLOT, opllAddr, ioData, 6, 27);	/*3.36us, 23.52us*/
				g_pB->Index += sizeof(CMD_SET_REG);
				portEXIT_CRITICAL(&g_Mut_InData);
				lastTime = nowTime;
				break;
			}
			/* MSX-AUDIO C0h */
			case Y8950_AD: {
				y8950Addr = ioData;
				break;
			}
			/* MSX-AUDIO C1h */
			case Y8950_DT: {
	#ifdef OUTPUT_SERIAL_MESS
				Serial.printf("AUD %02x:%02x %d %d\n", y8950Addr, ioData, g_pB->Index, def);
	#endif
				portENTER_CRITICAL(&g_Mut_InData);
				bKick |= makeWaitCmd(g_pB, def);
				CMD_SET_REG *p = reinterpret_cast<CMD_SET_REG *>(&g_pB->pBuff[g_pB->Index]);
				const uint8_t SLOT = 0x20;
				makeSetReg(p, SLOT, y8950Addr, ioData, 6, 27);	/*3.36us, 23.52us, Same as OPLL*/
				g_pB->Index += sizeof(CMD_SET_REG);
				portEXIT_CRITICAL(&g_Mut_InData);
				lastTime = nowTime;
				break;
			}
			/* TEST 00h */
			case TEST_AD: {
				testAddr = ioData;
				break;
			}
			/* TEST 01h */
			case TEST_DT: {
				break;
			}
		}
	}
	portENTER_CRITICAL(&g_Mut_InDataNum);
	g_InData.num -= num;    
	portEXIT_CRITICAL(&g_Mut_InDataNum);

	if (bKick){
		xTaskNotifyGive(g_taskHandle);
	}
	if (bDisplay){
		printStatus(g_lostCount);
	}
	return;
}

//---------------------------------------------------
void sendTask(void *pvParameters)
{
	for(;;) {
		uint32_t r = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));
		if (r == 0){	// timeoutの場合 NOPを送信する
			if (g_WiFiState == WIFI_WPM_OK){
				static uint8_t NOP = 0;
				g_udp.write(&NOP, 1);
			}
		}
		else
		{
			ets_delay_us(POOLTIMEUS);
			SENDBUFF *p = &g_Buffer[g_Plane];
			g_Plane ^= 0x01;
			portENTER_CRITICAL(&g_Mut_InData);
			g_pB = &g_Buffer[g_Plane];
			portEXIT_CRITICAL(&g_Mut_InData);
			g_udp.write(p->pBuff, p->Index);
			p->Index = 0;
		}
	}
}

//---------------------------------------------------
void loop()
{
	if( g_WiFiState != WIFI_WPM_OK ){
		steStandByWiFi();
	}
	else {
		stsIoThief();
	}
	return;
}

//---------------------------------------------------
void setup()
{
	g_Plane = 0;
	g_Buffer[0].Index = g_Buffer[1].Index = 0;
	g_pB = &g_Buffer[g_Plane];
	g_WiFiState = START_WIFI;
	g_lostCount = 0;

#ifdef OUTPUT_SERIAL_MESS
	Serial.begin(115200);
#endif
	g_Lcd.init();
	g_Lcd.flipScreenVertically();
	g_Lcd.setContrast(50);

	printStatus(g_lostCount);
	setupGpio();
	digitalWrite(MSX_ESP_REQ, HIGH);

	// 送信タスクの生成
	xTaskCreatePinnedToCore(
		sendTask, "sendTask",
		8192, NULL,
		4,				// priority, 2では音がよく停止した
		&g_taskHandle,
		1);				// Core 0:通信系,1:loop()
  return;
}
