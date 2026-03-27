/*
 * DWIN_Process.h
 *
 *  Created on: Jan 20, 2025
 *      Author: Step
 */

#ifndef INC_DWIN_PROCESS_H_
#define INC_DWIN_PROCESS_H_

#include "main.h"
#include "DWIN_Adress.h"


#define DEBUG_DWIN 1
#define TIMEOUT_MS 10
#define MAX_ATTEMPT 3

#define VERSION_ADDR 	0x000F
#define READ_CMD 		0x83
#define WRITE_CMD		0x82

#define REGISTER_TABLE_SIZE   9000

typedef enum
{
	NO_RESPONSE	= 	0,
	CRC_ERROR	= 	1,
	WRITE_OK	=	2,
	READ_OK		=	3,

} DWIN_Response;


typedef struct{
	uint32_t bluetoothCheck;
	uint32_t dwinCheck;
    uint32_t run;
    uint32_t buharSuresi;
    uint32_t lambaSuresi;
    uint32_t DwinAlarm;
    uint32_t ustOnPeriod;
    uint32_t ustArkaPeriod;
    uint32_t altPeriod;
    uint32_t turboCloseWait;
    uint32_t buharHazir;
    uint32_t PID_Run;
    uint32_t shiftRefreshWait;
    uint32_t input_AC_2;
    uint32_t brulorResetRelay;
}tickCounter;


uint16_t calculateCRC16Modbus(uint8_t *data, uint16_t length);
uint16_t combineBytes(uint8_t highByte, uint8_t lowByte);
void parse16BitTo8Bit(uint16_t value, uint8_t *highByte, uint8_t *lowByte);
HAL_StatusTypeDef DWIN_SetUsartChannel(UART_HandleTypeDef *huart, USART_TypeDef *Declaration, DMA_HandleTypeDef *hdma);
void DWIN_check(void);
DWIN_Response DWIN_readRegister(uint8_t* pBuffer, uint16_t addr, uint8_t len);
DWIN_Response DWIN_writeRegiser(uint16_t* pBuffer, uint16_t addr, uint8_t len);
DWIN_Response DWIN_changePage(uint8_t pageNumber);
void convert_u8_to_u16(const uint8_t src[], uint16_t dest[], uint16_t size);
void convert_u16_to_u8(const uint16_t src[], uint8_t dest[], uint16_t size);

DWIN_Response DWIN_receiveDataProcess(void);
void DWIN_answerProcess(void);
void DWIN_run(void);
void DWIN_manuelSayfa(void);
void DWIN_anaSayfa(void);
void DWIN_manuelPisirmeSuresi(void);
void DWIN_manuelBuharSuresi(void);
void DWIN_Alarm(void);
void DWIN_brulorArizaSet(uint8_t state);
void DWIN_resetManuelPisirme(void);
void DWIN_enterManuelProcess(void);
void DWIN_lambaSuresi(void);
void DWIN_manuelProcess(void);
void DWIN_manuelPeriodProcess(void);
void DWIN_changeMaxSetValue(uint16_t page, uint8_t controlID, uint16_t maxValue);
void DWIN_manuelTurboProcess(void);
void DWIN_arızaCheck(void);
void DWIN_buharHazirCheck(void);
void DWIN_receteSayfa(void);
void DWIN_manuelPisirmeSuresi_Calc(void);
void DWIN_manuelBuharSuresi_Calc(void);
void DWIN_recetePisirmeAdimProcess(void);
void DWIN_writeRTC(uint8_t saniye, uint8_t dakika, uint8_t saat, uint8_t gun, uint8_t ay, uint8_t yil, uint8_t writeEN);
void DWIN_readRTC(uint8_t* saniye, uint8_t* dakika, uint8_t* saat, uint8_t* hafta, uint8_t* gun, uint8_t* ay, uint8_t* yil);
void DWIN_otomatikSayfa(void);
void DWIN_otomatikAcmaCheck(void);
void DWIN_otomatikPisirmeBaslatmaCheck(void);
DWIN_Response DWIN_buzzerSet(uint8_t setLevel);
DWIN_Response DWIN_dokunmatik_aktif_msg(void);
DWIN_Response DWIN_buharActivePassive(uint8_t setMode);
void PWM_SetFreqAndDuty(uint32_t freq_hz, uint8_t duty_percent);
uint32_t calculate_frequency(uint32_t input);
void PWM_StartSmoothTransition(uint32_t new_freq, uint8_t duty_percent);
void PWM_SmoothTask_1ms(void);
void DWIN_brulorArizaReset_Relay(void);
void DWIN_changePopup(uint16_t dil);
void DWIN_changeIcon(uint16_t dil);
void DWIN_dilChange(void);
void DWIN_changeKeyboard(uint16_t dil);
void DWIN_testSayfa(void);
void DWIN_isitmaProcess(void);
void DWIN_sogutmaProcess(void);

#endif /* INC_DWIN_PROCESS_H_ */
