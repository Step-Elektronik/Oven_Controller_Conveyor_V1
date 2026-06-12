/*
 * DWIN_Process.c
 *
 *  Created on: Jan 20, 2025
 *      Author: Step
 */


#include "usart.h"
#include "USART_Process.h"
#include "DWIN_Process.h"
#include "SEGGER_RTT.h"
#include "Temperature_Process.h"
#include "string.h"
#include "InOut_Process.h"
#include "rtc.h"
#include "EEPROM_Process.h"
#include "hdc1080.h"
#include "tim.h"
#include "dac.h"
#include "version.h"
#include "stdio.h"

extern TemperatureData temp;
extern uint8_t DWIN_rxBuffer[DWIN_rxBufferSize];
extern uint8_t main_DWIN_rxBuffer[DWIN_rxBufferSize];

extern USART_TypeDef *DWIN_usartDeclaration;
extern UART_HandleTypeDef *DWIN_huart_channel;
extern DMA_HandleTypeDef *DWIN_hdma_usart_purpose;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern I2C_HandleTypeDef hi2c1;

extern usartInfo DWIN;

extern uint16_t eepromAddrTable[EEPROM_TABLE_LEN];

extern uint8_t rxBusyFlag;

tickCounter counterTick;

uint16_t registerTable[REGISTER_TABLE_SIZE] = {0};


uint16_t pisirmeManuelDownCounter 	= 0;
uint16_t buharManuelDownCounter 	= 0;

uint8_t rtcSecondPisirme 	= 0;
uint8_t rtcSecondBuhar 		= 0;

uint8_t DwinAlarmFlag = 0;
uint8_t DwinAlarmBuzzer = 0;

uint8_t wrongmsg_flag = 0;
uint8_t wrongmsg_buffer[20];

uint8_t ustSicaklikProcess = 99;
uint8_t altSicaklikProcess = 99;

uint8_t ustOnTurbo 		= 0;
uint8_t ustArkaTurbo 	= 0;
uint8_t altTurbo		= 0;
uint8_t turboCloseFlag	= 0;

uint16_t alarmBuzzerPeriod = 1000;

uint16_t islemdekiRecete 			= 0;
uint16_t islemdekiReceteAdim 		= 1;
uint16_t islemdekiOtomatikGun		= 0;
uint16_t islemdekiOtomatikAktifIkon	= 0;

uint8_t otomatikPisirmeBaslatmaFlag = 0;

uint32_t conveyor_freq = 0;

extern uint8_t  pid_active;
extern uint32_t outputData;

extern uint8_t brulor_ariza_resetleme_gormgelme_check;
extern uint8_t brulor_ariza_resetleme_gormgelme_cnt;

volatile uint32_t start_freq = 0;      // Geçiş başladığındaki frekans
volatile uint32_t target_freq = 0;     // Hedeflenen frekans
volatile uint32_t current_freq = 0;    // Anlık hesaplanan frekans
volatile uint16_t tick_counter = 0;    // 0'dan 1000'e kadar sayacak
volatile uint8_t  current_duty = 0;    // Sabit kalacak duty cycle
volatile uint8_t  is_transitioning = 0; // Geçiş devam ediyor mu?

void Furnace_Init(void);
void Furnace_Control_1s(void);
void Set_Heater_DAC(uint32_t value);

extern double soft_threshold;

float CalculateSoftThreshold(float setTemp, float currentTemp);
void Set_Heater_DAC(uint32_t value);

// 8 bitlik iki sayıyı 16 bitlik bir sayıya birleştiren fonksiyon
uint16_t combineBytes(uint8_t highByte, uint8_t lowByte) {
    return ((uint16_t)highByte << 8) | lowByte;
}

void parse16BitTo8Bit(uint16_t value, uint8_t *highByte, uint8_t *lowByte) {
    *highByte = (uint8_t)(value >> 8);  // Yüksek byte (MSB)
    *lowByte = (uint8_t)(value & 0xFF); // Düşük byte (LSB)
}

void convert_u8_to_u16(const uint8_t src[], uint16_t dest[], uint16_t size)
{

    for (int i = 0; i < size/2 ; i++)
    {
        dest[i] = ((uint16_t)src[2 * i] << 8) | src[2 * i + 1];
    }
}

void convert_u16_to_u8(const uint16_t src[], uint8_t dest[], uint16_t size)
{
    for (int i = 0; i < size/2 ; i++)
    {
        dest[2 * i + 1]     = (uint8_t)(src[i] & 0xFF);       	// Lower byte
        dest[2 * i] 		= (uint8_t)((src[i] >> 8) & 0xFF);  // Higher byte
    }
}

uint16_t calculateCRC16Modbus(uint8_t *data, uint16_t length) {
    uint16_t crc = 0xFFFF; // Başlangıç değeri

    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i]; // İlk veri byte'ı ile XOR işlemi
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001; // Polinom: x^16 + x^15 + x^2 + 1
            } else {
                crc >>= 1;
            }
        }
    }

    return crc;
}

static inline void UserApp_IWDG_Refresh(void)
{
    IWDG->KR = 0xAAAA;
}

void set_FWversion(void)
{
	uint8_t version[10] = {0};

    uint16_t major = FW_VERSION / 100;
    uint16_t minor = FW_VERSION % 100;

    sprintf((char*)version, "VER %d.%02d", major, minor);

    uint16_t version_u16[sizeof(version)/2] = {0};

    convert_u8_to_u16(version, version_u16, sizeof(version));

    registerTable[DW_FW_VERSION_ADR] = FW_VERSION;

    DWIN_writeRegiser(version_u16, DW_FW_VERSION_ADR, sizeof(version_u16));
}


void setAnalogVoltage(float target_voltage, uint32_t Channel)
{

	// 2. Sabit Değerler
	const float V_REF = 3.3f;               // MCU besleme gerilimi (DAC referansı)
	float 		GAIN = 0; 					// Opamp Kazancı
	const float MAX_DAC_VAL = 4095.0f;      // 12-bit DAC maksimum değeri

	if(Channel == DAC_CHANNEL_1)
	{
		GAIN = 6.0;

		if (target_voltage > 15.0f) {
			target_voltage = 15.0f;
		} else if (target_voltage < 0.0f) {
			target_voltage = 0.0f;
		}
	}
	else if(Channel == DAC_CHANNEL_2)
	{
		GAIN = 3.26;

		if (target_voltage > 10.0f) {
			target_voltage = 10.0f;
		} else if (target_voltage < 0.0f) {
			target_voltage = 0.0f;
		}
	}

	// 3. İstenen 10V çıkış için, DAC'tan çıkması gereken hedef voltajı bul
	float required_dac_voltage = target_voltage / GAIN;

	// 4. Bu voltajı 0-4095 arasında bir dijital değere (RAW) çevir
	// Formül: (İstenen_DAC_Voltajı / V_REF) * 4095
	uint32_t dac_raw_value = (uint32_t)((required_dac_voltage / V_REF) * MAX_DAC_VAL);

	// 5. Değeri DAC kanalına yazdır (Kanal 1 kullanıldığını varsayıyoruz, Kanal 2 ise değiştirin)
	HAL_DAC_SetValue(&hdac, Channel, DAC_ALIGN_12B_R, dac_raw_value);
}


DWIN_Response DWIN_receiveDataProcess(void)
{
	DWIN_Response response = NO_RESPONSE;

	if((DWIN_rxBuffer[0] == 0x5A) && (DWIN_rxBuffer[1] == 0xA5))
	{
		uint8_t size = DWIN_rxBuffer[2]+3;

		if((DWIN_rxBuffer[size] == 0x5A) && (DWIN_rxBuffer[size + 3] == READ_CMD))
		{
			size = DWIN_rxBuffer[size + 2] + 3;

			uint8_t secondBufferstartAdr = DWIN_rxBuffer[2]+3;

			for(int i=secondBufferstartAdr;i<secondBufferstartAdr+size;i++)
			{
				DWIN_rxBuffer[i - secondBufferstartAdr] = DWIN_rxBuffer[i];
			}

			#if DEBUG_DWIN == 1
			SEGGER_RTT_printf(0," write and read ! \r\n");
			#endif
		}

		uint8_t crcBuffer[(uint8_t)(DWIN_rxBuffer[2]-2)];

		for(int i=0;i<DWIN_rxBuffer[2]-2;i++)
			crcBuffer[i] = DWIN_rxBuffer[i+3];

		uint16_t crc = calculateCRC16Modbus(crcBuffer, sizeof(crcBuffer));

		uint16_t crcRx = combineBytes(DWIN_rxBuffer[size-1], DWIN_rxBuffer[size-2]);

		if(crc == crcRx)
		{
			if(DWIN_rxBuffer[3] == WRITE_CMD)
				response = WRITE_OK;

			if(DWIN_rxBuffer[3] == READ_CMD)
			{
				uint16_t addr = combineBytes(DWIN_rxBuffer[4], DWIN_rxBuffer[5]);

				if((addr != 0x979A) && (addr != 0x0084) && (addr != 0x7200))
				{
					#if DEBUG_DWIN == 1
					SEGGER_RTT_printf(0,"---------------------------\r\nAddr   : %02X %02X \r\n",DWIN_rxBuffer[4],DWIN_rxBuffer[5]);

					HAL_Delay(0);

					for(int i=0;i<DWIN_rxBuffer[6];i++)
						SEGGER_RTT_printf(0,"Data %d : %02X %02X \r\n",i+1,DWIN_rxBuffer[7+(i*2)],DWIN_rxBuffer[8+(i*2)]);

					#endif


					response = READ_OK;
				}
			}
		}

		else
		{
			#if DEBUG_DWIN == 1
			SEGGER_RTT_printf(0," CRC ERROR ! \r\n");
			#endif

			response = CRC_ERROR;
		}

	}
	else
	{

		#if DEBUG_DWIN == 1
		SEGGER_RTT_printf(0," False Message ! \r\n");
		wrongmsg_flag = 1;

		for(int i=0;i<20;i++)
			wrongmsg_buffer[i] = DWIN_rxBuffer[i];

		#endif
	}

	return response;
}

HAL_StatusTypeDef DWIN_SetUsartChannel(UART_HandleTypeDef *huart, USART_TypeDef *Declaration, DMA_HandleTypeDef *hdma)
{
	HAL_StatusTypeDef response;

	DWIN_usartDeclaration = Declaration;
	DWIN_huart_channel = huart;
	DWIN_hdma_usart_purpose = hdma;


//	response = HAL_UARTEx_ReceiveToIdle_DMA(huart, DWIN_rxBuffer, DWIN_rxBufferSize);
//	__HAL_DMA_DISABLE_IT(DWIN_hdma_usart_purpose, DMA_IT_HT);

	  response = HAL_UART_Receive_DMA(huart, main_DWIN_rxBuffer, DWIN_rxBufferSize);

	//__HAL_UART_CLEAR_IT(huart, UART_CLEAR_IDLEF);
	__HAL_UART_CLEAR_IDLEFLAG(huart);
	__HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);


	return response;
}

DWIN_Response DWIN_readRegister(uint8_t* pBuffer, uint16_t addr, uint8_t len)
{
	DWIN_Response response = NO_RESPONSE;

	uint8_t numOfRegister = len / 2;

	uint8_t txBuffer[numOfRegister + 8];

	txBuffer[0] = 0x5A;
	txBuffer[1] = 0xA5;
	txBuffer[2] = 0x06;
	txBuffer[3] = 0x83;

	uint8_t highByte, lowByte;
    highByte 	= (addr >> 8) & 0xFF;
    lowByte 	= addr & 0xFF;

	txBuffer[4] = highByte;
	txBuffer[5] = lowByte;
	txBuffer[6] = numOfRegister;

	uint8_t crcBuffer[4];

	for(int i=3;i<7;i++)
		crcBuffer[i-3] = txBuffer[i];

	uint16_t crc = calculateCRC16Modbus(crcBuffer, sizeof(crcBuffer));

	txBuffer[7] = crc & 0xFF;
	txBuffer[8] = (crc >> 8) & 0xFF;

	uint8_t numOfAttempts = 0;

	sendPoint:

	//DWIN.receiveStatus = NO_RESPONSE;
	memset(DWIN_rxBuffer,0,sizeof(DWIN_rxBuffer));

	DWIN.rxDoneFlag = 0;
	HAL_UART_Transmit_IT(DWIN_huart_channel, txBuffer, sizeof(txBuffer));
	numOfAttempts++;

	uint32_t timeOut = HAL_GetTick();
	while((DWIN.rxDoneFlag != 1)&&((HAL_GetTick() - timeOut)<=TIMEOUT_MS));

	if(((HAL_GetTick() - timeOut) <= TIMEOUT_MS)&&(numOfAttempts <= MAX_ATTEMPT))
	{
		response = DWIN_receiveDataProcess();

		if(response == READ_OK)
		{
			for(int i=0;i<(numOfRegister*2);i++)
				pBuffer[i] = DWIN_rxBuffer[i+7];

			DWIN.rxDoneFlag = 0;
		}
		else
		{
			#if DEBUG_DWIN == 1
			SEGGER_RTT_printf(0,"DWIN CRC ERROR \r\n");
			#endif

			goto sendPoint;
		}

	}

	else
	{
		if(numOfAttempts <= MAX_ATTEMPT)
		{
			#if DEBUG_DWIN == 1
			SEGGER_RTT_printf(0,"DWIN NO RESPONSE \r\n");
			#endif

			goto sendPoint;
		}

		DWIN.rxDoneFlag = 0;
		DWIN.Init = 0;
	}

	return response;

}

void dwin_popup_change_structure(uint16_t page, uint8_t controlID, uint16_t picNext)
{
	uint16_t tx[4];


	tx[0]=(uint16_t)(0x5A<<0x08)+(uint16_t)0xA5;
	tx[1]=page;
	tx[2]=(uint16_t)(controlID<<0x08)+0x01;
	tx[3]=0x0002;

	DWIN_writeRegiser(tx, 0x00b0, sizeof(tx));

	HAL_Delay(50);

	uint8_t readData[48] = {0};
	DWIN_readRegister(readData, 0x00B4, sizeof(readData));

	uint16_t data[28];

	data[0]=(uint16_t)(0x5A<<0x08)+(uint16_t)0xA5;
	data[1]=page;
	data[2]=(uint16_t)(controlID<<0x08)+0x01;
	data[3]=0x0003;

	for(int i=0;i<sizeof(readData)/2;i++)
		data[i+4] = combineBytes(readData[i*2], readData[(i*2)+1]);

    data[14]=picNext;

    DWIN_writeRegiser(data, 0x00b0, sizeof(data));

    HAL_Delay(50);


}

void dwin_pop_up_change_all_structure(uint16_t popPage)
{
	for(int i=8; i<25; i++)
	{
		for(int j=0; j<6; j++)
		{
			if(i != 28)
				dwin_popup_change_structure(i,(uint8_t)j,popPage);
			else if(j<4)
			{
				dwin_popup_change_structure(i,(uint8_t)j,popPage);
			}

		}

	}
}

void DWIN_changePopup(uint16_t dil)
{

	switch(dil)
	{
		case DW_DIL_TURKCE_VAL:
			dwin_pop_up_change_all_structure(7);
		break;
		case DW_DIL_INGILIZCE_VAL:
			dwin_pop_up_change_all_structure(48);
		break;
		case DW_DIL_RUSCA_VAL:
			dwin_pop_up_change_all_structure(49);
		break;
		case DW_DIL_ALMANCA_VAL:
			dwin_pop_up_change_all_structure(57);
		break;
	}
}

void dwin_icon_change_structure(uint16_t sp,uint16_t iconMin, uint16_t iconMax, uint16_t file)
{
	uint16_t writeData[3] = {iconMin, iconMax, file};
	DWIN_writeRegiser(writeData, sp+0x05, sizeof(writeData));
}

void DWIN_changeIcon(uint16_t dil)
{
	switch(dil)
	{
		case DW_DIL_TURKCE_VAL:

			dwin_icon_change_structure( 0x4700, 0x0035, 0x0036, 0x3701);
			dwin_icon_change_structure( 0x470A, 0x0035, 0x0036, 0x3701);
			dwin_icon_change_structure( 0x4714, 0x0051, 0x0054, 0x3701);
			dwin_icon_change_structure( 0x471E, 0x0035, 0x0036, 0x3701);
			dwin_icon_change_structure( 0x4732, 0x0040, 0x0041, 0x3701);
			dwin_icon_change_structure( 0x473C, 0x0040, 0x0042, 0x3701);
			dwin_icon_change_structure( 0x4746, 0x0040, 0x0043, 0x3701);
			dwin_icon_change_structure( 0x4750, 0x0040, 0x0044, 0x3701);
			dwin_icon_change_structure( 0x475A, 0x0040, 0x0046, 0x3701);
			dwin_icon_change_structure( 0x4764, 0x0040, 0x0045, 0x3701);
			dwin_icon_change_structure( 0x476E, 0x0040, 0x0047, 0x3701);
			dwin_icon_change_structure( 0x4778, 0x0040, 0x0050, 0x3701);
			dwin_icon_change_structure( 0x4782, 0x0049, 0x0049, 0x3701);
			dwin_icon_change_structure( 0x478C, 0x0048, 0x0048, 0x3701);


		break;

		case DW_DIL_INGILIZCE_VAL:

			dwin_icon_change_structure( 0x4700, 0x0035, 0x0036, 0x3701);
			dwin_icon_change_structure( 0x470A, 0x0035, 0x0036, 0x3701);
			dwin_icon_change_structure( 0x4714, 0x003A, 0x003D, 0x3A01);
			dwin_icon_change_structure( 0x471E, 0x0035, 0x0036, 0x3701);
			dwin_icon_change_structure( 0x4732, 0x0061, 0x004F, 0x3A01);
			dwin_icon_change_structure( 0x473C, 0x0061, 0x0050, 0x3A01);
			dwin_icon_change_structure( 0x4746, 0x0061, 0x0051, 0x3A01);
			dwin_icon_change_structure( 0x4750, 0x0061, 0x0052, 0x3A01);
			dwin_icon_change_structure( 0x475A, 0x0061, 0x0053, 0x3A01);
			dwin_icon_change_structure( 0x4764, 0x0061, 0x0054, 0x3A01);
			dwin_icon_change_structure( 0x476E, 0x0061, 0x0055, 0x3A01);
			dwin_icon_change_structure( 0x4778, 0x0061, 0x0056, 0x3A01);
			dwin_icon_change_structure( 0x4782, 0x0062, 0x0062, 0x3A01);
			dwin_icon_change_structure( 0x478C, 0x0060, 0x0060, 0x3701);

		break;

		case DW_DIL_RUSCA_VAL:

			dwin_icon_change_structure( 0x4700, 0x0036, 0x0037, 0x3A01);
			dwin_icon_change_structure( 0x470A, 0x0036, 0x0037, 0x3A01);
			dwin_icon_change_structure( 0x4714, 0x0049, 0x004C, 0x3A01);
			dwin_icon_change_structure( 0x471E, 0x0036, 0x0037, 0x3A01);
			dwin_icon_change_structure( 0x4732, 0x0061, 0x0057, 0x3A01);
			dwin_icon_change_structure( 0x473C, 0x0061, 0x0058, 0x3A01);
			dwin_icon_change_structure( 0x4746, 0x0061, 0x0059, 0x3A01);
			dwin_icon_change_structure( 0x4750, 0x0061, 0x005A, 0x3A01);
			dwin_icon_change_structure( 0x475A, 0x0061, 0x005B, 0x3A01);
			dwin_icon_change_structure( 0x4764, 0x0061, 0x005C, 0x3A01);
			dwin_icon_change_structure( 0x476E, 0x0061, 0x005D, 0x3A01);
			dwin_icon_change_structure( 0x4778, 0x0061, 0x005E, 0x3A01);
			dwin_icon_change_structure( 0x4782, 0x0063, 0x0063, 0x3A01);
			dwin_icon_change_structure( 0x478C, 0x005F, 0x005F, 0x3A01);

		break;

		case DW_DIL_ALMANCA_VAL:

			dwin_icon_change_structure( 0x4700, 0x0074, 0x0075, 0x3A01);
			dwin_icon_change_structure( 0x470A, 0x0074, 0x0075, 0x3A01);
			dwin_icon_change_structure( 0x4714, 0x006E, 0x0071, 0x3A01);
			dwin_icon_change_structure( 0x471E, 0x0074, 0x0075, 0x3A01);
			dwin_icon_change_structure( 0x4732, 0x0061, 0x0076, 0x3A01);
			dwin_icon_change_structure( 0x473C, 0x0061, 0x0077, 0x3A01);
			dwin_icon_change_structure( 0x4746, 0x0061, 0x0078, 0x3A01);
			dwin_icon_change_structure( 0x4750, 0x0061, 0x0079, 0x3A01);
			dwin_icon_change_structure( 0x475A, 0x0061, 0x007A, 0x3A01);
			dwin_icon_change_structure( 0x4764, 0x0061, 0x007B, 0x3A01);
			dwin_icon_change_structure( 0x476E, 0x0061, 0x007C, 0x3A01);
			dwin_icon_change_structure( 0x4778, 0x0061, 0x007D, 0x3A01);
			dwin_icon_change_structure( 0x4782, 0x0064, 0x0064, 0x3A01);
			dwin_icon_change_structure( 0x478C, 0x0060, 0x0060, 0x3A01);

		break;
	}
}

void DWIN_changeMaxSetValue(uint16_t page, uint8_t controlID, uint16_t maxValue)
{
	uint16_t tx[4];

	tx[0]=(uint16_t)(0x5A<<0x08)+(uint16_t)0xA5;
	tx[1]=page;
	tx[2]=(uint16_t)(controlID<<0x08)+0x02;
	tx[3]=0x0002;

	DWIN_writeRegiser(tx, 0x00b0, sizeof(tx));

	HAL_Delay(50);

	uint8_t readData[32] = {0};
	DWIN_readRegister(readData, 0x00B4, sizeof(readData));

	uint16_t data[20] = {0};

	data[0]=(uint16_t)(0x5A<<0x08)+(uint16_t)0xA5;
	data[1]=page;
	data[2]=(uint16_t)(controlID<<0x08)+0x02;
	data[3]=0x0003;

	for(int i=0;i<sizeof(readData)/2;i++)
		data[i+4] = combineBytes(readData[i*2], readData[(i*2)+1]);

    data[17]=maxValue;

    DWIN_writeRegiser(data, 0x00b0, sizeof(data));

    HAL_Delay(50);

}

void keyboard_change_structure(uint16_t page,uint8_t touchID, uint8_t libID, uint8_t xDots, uint8_t yDots, uint8_t picture)
{
	uint16_t tx[4];

	tx[0]=0x5AA5;
	tx[1]=page;
	tx[2]=(uint16_t)(touchID<<0x08)+0x06;
	tx[3]=0x0002;

	DWIN_writeRegiser(tx, 0x00b0, sizeof(tx));
	HAL_Delay(50);

	uint8_t readData[64] = {0};
	DWIN_readRegister(readData, 0x00B4, sizeof(readData));


	uint16_t data[36];

	data[0]=0x5AA5;
	data[1]=page;
	data[2]=(uint16_t)(touchID<<0x08)+0x06;
	data[3]=0x0003;

	for(int i=0;i<sizeof(readData)/2;i++)
		data[i+4] = combineBytes(readData[i*2], readData[(i*2)+1]);

    data[0xa+0x04]=(data[0x0a+0x04] & 0xff00) | (uint16_t)(libID);
    data[0xb+0x04]=(uint16_t)(xDots<<0x08) | (uint16_t)(yDots);
    data[0x13+0x04]=(data[0x13+0x04] & 0xFf00) | (uint16_t)(picture);


    DWIN_writeRegiser(data, 0x00b0, sizeof(data));

    HAL_Delay(50);

}

void DWIN_changeKeyboard(uint16_t dil)
{
	if(dil == DW_DIL_RUSCA_VAL)
	{
		keyboard_change_structure(0x0019,0x00,0x0B,0x32,0x46,0x35);
		HAL_Delay(100);
		keyboard_change_structure(0x0032,0x01,0x0B,0x32,0x46,0x35);
		HAL_Delay(100);
	}
	else if(dil != DW_DIL_ALMANCA_VAL)
	{
		keyboard_change_structure(0x0019,0x00,0x0F,0x23,0x46,0x1A);
		HAL_Delay(100);
		keyboard_change_structure(0x0032,0x01,0x0F,0x1A,0x34,0x1A);
		HAL_Delay(100);
	}
	else
	{
		keyboard_change_structure(0x0019,0x00,0x0A,0x32,0x46,0x38);
		HAL_Delay(100);
		keyboard_change_structure(0x0032,0x01,0x0A,0x32,0x46,0x38);
		HAL_Delay(100);
	}


}

void DWIN_changeWord(uint16_t dil)
{
	switch(dil)
	{
		case DW_DIL_TURKCE_VAL:

			for(int i=0; i<100; i++) //turkish
			{
				DWIN_writeRegiser((uint16_t[]){0x0F0F,0x2346},0x4000+((i*0x0D)+0x09),4);
			}

			DWIN_writeRegiser((uint16_t[]){0x0F0F,0x2346},0x4514+0x09,4); //recete duzenleme

			DWIN_writeRegiser((uint16_t[]){0},0x4514+0x0001,2);
			DWIN_writeRegiser((uint16_t[]){0x4000},0x4514+0x000B,2);

			DWIN_writeRegiser((uint16_t[]){0x0F0F,0x2346},0x4521+0x09,4); //recete pisirme
			DWIN_writeRegiser((uint16_t[]){0},0x4521+0x0001,2);
			DWIN_writeRegiser((uint16_t[]){0x4000},0x4521+0x000B,2);

			//DWIN_writeRegiser(noName_u16,0x17c0,sizeof(noName_u16)); //bt isim

			DWIN_writeRegiser((uint16_t[]){0x0F0F,0x1A34},0x452E + 0x09,4);

		break;

		case DW_DIL_INGILIZCE_VAL:
			for(int i=0; i<100; i++) //english
			{
				DWIN_writeRegiser((uint16_t[]){0x0F0F,0x2346},0x4000+((i*0x0D)+0x09),4);
			} //recete isimleri

			DWIN_writeRegiser((uint16_t[]){0x0F0F,0x2346},0x4514+0x09,4); //recete duzenleme

			DWIN_writeRegiser((uint16_t[]){0},0x4514+0x0001,2);

			DWIN_writeRegiser((uint16_t[]){0x4000},0x4514+0x000B,2);

			DWIN_writeRegiser((uint16_t[]){0x0F0F,0x2346},0x4521+0x09,4); //recete pisirme
			DWIN_writeRegiser((uint16_t[]){0},0x4521+0x0001,2);
			DWIN_writeRegiser((uint16_t[]){0x4000},0x4521+0x000B,2);

			//DWIN_writeRegiser(noName_u16,0x17c0,sizeof(noName_u16)); 	//bt isim

			DWIN_writeRegiser((uint16_t[]){0x0F0F,0x1A34},0x452E + 0x09,4);
		break;

		case DW_DIL_RUSCA_VAL:

			for(int i=0; i<100; i++) //russian
			{
				DWIN_writeRegiser((uint16_t[]){0x0b0b,0x3246},0x4000+((i*0x0D)+0x09),4);
			}

			DWIN_writeRegiser((uint16_t[]){0x0B0B,0x3246},0x4514+0x09,4); //recete duzenleme
			DWIN_writeRegiser((uint16_t[]){0x0020},0x4514+0x0001,2);

			DWIN_writeRegiser((uint16_t[]){0x8000},0x4514+0x000B,2);

			DWIN_writeRegiser((uint16_t[]){0x0B0B,0x3246},0x4521+0x09,4); //recete pisirme
			DWIN_writeRegiser((uint16_t[]){0x0020},0x4521+0x0001,2);
			DWIN_writeRegiser((uint16_t[]){0x8000},0x4521+0x000B,2);

			//DWIN_writeRegiser(0x17c0,Без_Имени_u16,10); //bt isim
			DWIN_writeRegiser((uint16_t[]){0x0B0B,0x3246},(0x452E)+0x09,4);
		break;

		case DW_DIL_ALMANCA_VAL:

			for(int i=0; i<100; i++) //russian
			{
				DWIN_writeRegiser((uint16_t[]){0x0A0A,0x3246},0x4000+((i*0x0D)+0x09),4);
			}

			DWIN_writeRegiser((uint16_t[]){0x0A0A,0x3246},0x4514+0x09,4); //recete duzenleme
			DWIN_writeRegiser((uint16_t[]){0x0020},0x4514+0x0001,2);

			DWIN_writeRegiser((uint16_t[]){0x8000},0x4514+0x000B,2);

			DWIN_writeRegiser((uint16_t[]){0x0A0A,0x3246},0x4521+0x09,4); //recete pisirme
			DWIN_writeRegiser((uint16_t[]){0x0020},0x4521+0x0001,2);
			DWIN_writeRegiser((uint16_t[]){0x8000},0x4521+0x000B,2);

			//DWIN_writeRegiser(0x17c0,Без_Имени_u16,10); //bt isim
			DWIN_writeRegiser((uint16_t[]){0x0A0A,0x3246},(0x452E)+0x09,4);
		break;
	}
}

void DWIN_dilChange(void)
{
	uint16_t writeData = registerTable[DW_DIL_PARAM_ADR];

	DWIN_writeRegiser(&writeData, DW_DIL_SABIT_YAZI_ADR, sizeof(writeData));

	DWIN_changePopup(registerTable[DW_DIL_PARAM_ADR]);

	DWIN_changeIcon(registerTable[DW_DIL_PARAM_ADR]);

	DWIN_changeWord(registerTable[DW_DIL_PARAM_ADR]);

	DWIN_changeKeyboard(registerTable[DW_DIL_PARAM_ADR]);

}


DWIN_Response DWIN_writeRegiser(uint16_t* pBuffer, uint16_t addr, uint8_t len)
{
	DWIN_Response response = NO_RESPONSE;

	uint8_t numOfRegister = len / 2;

	uint8_t txBuffer[(numOfRegister*2) + 8];


	txBuffer[0] = 0x5A;
	txBuffer[1] = 0xA5;
	txBuffer[2] = 0x05 + (numOfRegister*2);
	txBuffer[3] = 0x82;

	uint8_t highByte, lowByte;
    highByte 	= (addr >> 8) & 0xFF;
    lowByte 	= addr & 0xFF;

	txBuffer[4] = highByte;
	txBuffer[5] = lowByte;

	for(int i=0;i<numOfRegister;i++)
	{
		uint16_t writeData = pBuffer[i];

		uint8_t highByte, lowByte;

	    highByte 	= (writeData >> 8) & 0xFF;
	    lowByte 	= writeData & 0xFF;

		txBuffer[6+(i*2)] = highByte;
		txBuffer[7+(i*2)] = lowByte;

	}

	uint8_t crcBuffer[3+(numOfRegister*2)];

	for(int i=3;i<6+(numOfRegister*2);i++)
		crcBuffer[i-3] = txBuffer[i];


	uint16_t crc = calculateCRC16Modbus(crcBuffer,3+(numOfRegister*2));

	txBuffer[8+((numOfRegister-1)*2)] = crc & 0xFF;
	txBuffer[9+((numOfRegister-1)*2)] = (crc >> 8) & 0xFF;

	uint8_t numOfAttempts = 0;

	sendPoint:

	if(DWIN.rxDoneFlag != 1)
	{
		if(__HAL_UART_GET_FLAG(&huart3,UART_FLAG_IDLE) == 0)
		{
			if (huart3.gState == HAL_UART_STATE_READY)
			{

				memset(DWIN_rxBuffer,0,sizeof(DWIN_rxBuffer));

				DWIN.rxDoneFlag = 0;
				HAL_UART_Transmit_IT(DWIN_huart_channel, txBuffer, sizeof(txBuffer));
				numOfAttempts++;
			}
		}


		uint32_t timeOut = HAL_GetTick();
		while((DWIN.rxDoneFlag != 1)&&((HAL_GetTick() - timeOut)<=TIMEOUT_MS));

		if(((HAL_GetTick() - timeOut) <= TIMEOUT_MS)&&(numOfAttempts <= MAX_ATTEMPT))
		{
			response = DWIN_receiveDataProcess();

			if(response == WRITE_OK)
				DWIN.rxDoneFlag = 0;

			else if(response != READ_OK)
			{
				#if DEBUG_DWIN == 1
				SEGGER_RTT_printf(0,"DWIN WRITE ERROR \r\n");
				#endif


				goto sendPoint;
			}
		}
		else
		{
			if(numOfAttempts <= MAX_ATTEMPT)
			{
				#if DEBUG_DWIN == 1
				SEGGER_RTT_printf(0,"DWIN NO RESPONSE \r\n");
				#endif

				goto sendPoint;
			}

			DWIN.Init = 0;
			DWIN.rxDoneFlag = 0;
		}

	}


	return response;
}

DWIN_Response DWIN_changePage(uint8_t pageNumber)
{
	DWIN_Response response = NO_RESPONSE;

	uint8_t txBuffer[12];

	txBuffer[0] = 0x5A;
	txBuffer[1] = 0xA5;
	txBuffer[2] = 0x09;
	txBuffer[3] = 0x82;
	txBuffer[4] = 0x00;
	txBuffer[5] = 0x84;
	txBuffer[6] = 0x5A;
	txBuffer[7] = 0x01;

	uint8_t highByte, lowByte;
    highByte 	= (pageNumber >> 8) & 0xFF;
    lowByte 	= pageNumber & 0xFF;

	txBuffer[8] = highByte;
	txBuffer[9] = lowByte;

	uint8_t crcBuffer[7];

	for(int i=3;i<10;i++)
		crcBuffer[i-3] = txBuffer[i];

	uint16_t crc = calculateCRC16Modbus(crcBuffer,7);

	txBuffer[10] = crc & 0xFF;
	txBuffer[11] = (crc >> 8) & 0xFF;

	uint8_t numOfAttempts = 0;

	sendPoint:

	//DWIN.receiveStatus = NO_RESPONSE;
	memset(DWIN_rxBuffer,0,sizeof(DWIN_rxBuffer));

	DWIN.rxDoneFlag = 0;
	HAL_UART_Transmit(DWIN_huart_channel, txBuffer, sizeof(txBuffer), 100);
	numOfAttempts++;

	uint32_t timeOut = HAL_GetTick();
	while((DWIN.rxDoneFlag != 1)&&((HAL_GetTick() - timeOut)<=TIMEOUT_MS));

	if(((HAL_GetTick() - timeOut) <= TIMEOUT_MS)&&(numOfAttempts <= MAX_ATTEMPT))
	{
		response = DWIN_receiveDataProcess();

		if(response == WRITE_OK)
			DWIN.rxDoneFlag = 0;

		else if(response != READ_OK)
		{
			#if DEBUG_DWIN == 1
			SEGGER_RTT_printf(0,"DWIN WRITE ERROR \r\n");
			#endif


			goto sendPoint;
		}
	}
	else
	{
		if(numOfAttempts <= MAX_ATTEMPT)
		{
			#if DEBUG_DWIN == 1
			SEGGER_RTT_printf(0,"DWIN NO RESPONSE \r\n");
			#endif

			goto sendPoint;
		}

		DWIN.Init = 0;
		DWIN.rxDoneFlag = 0;
	}

	return response;

}


DWIN_Response touchSetOnOff_structure(uint16_t mode, uint16_t page , uint8_t controlID, uint8_t keyCode)
{
	DWIN_Response response = NO_RESPONSE;

	uint16_t tx[4];

	tx[0]=0x5AA5;
	tx[1]=page;
	tx[2]=(uint16_t)(controlID<<0x08)+keyCode;
	tx[3]=mode;

	response = DWIN_writeRegiser(tx, 0x00b0, sizeof(tx));

	return response;
}

DWIN_Response DWIN_dokunmatik_aktif_msg(void)
{
	DWIN_Response response = NO_RESPONSE;

	for(int j=0;j<3;j++)
	{
		response = touchSetOnOff_structure(1,0,j,5);
		HAL_Delay(50);
	}


	return response;

}

void DWIN_run(void)
{

	if((HAL_GetTick() - counterTick.run) >= 1000)
	{
		counterTick.run = HAL_GetTick();

		HAL_GPIO_TogglePin(RUN_LED);

		calculate_temperature();

		UserApp_IWDG_Refresh();

		if(DWIN.Init == 1)
		{
			registerTable[DW_TMP112_ADR] 		= temp.TMP;
			registerTable[DW_UST_SICAKLIK_ADR] 	= temp.TC3;

			uint16_t sendData[2] = {(uint16_t)temp.TC3,temp.TMP};
			DWIN_writeRegiser(sendData, DW_UST_SICAKLIK_ADR,sizeof(sendData));


			if((registerTable[REG_DW_MODE_INFO_ADR] == DW_MANUEL_MODE_ENTER)||(registerTable[REG_DW_MODE_INFO_ADR] == DW_RECETE_PISIRME_SAYFA_ENTER))
			{
				I_AC_Check();
				DWIN_brulorArizaReset_Relay();
				DWIN_arızaCheck();

			}
			else if(registerTable[REG_DW_MODE_INFO_ADR] == DW_CIHAZ_TEST_SAYFA_ENTER)
			{
				uint8_t in1,in2,in3,in4,in5,in6,in_AC_1 = 0,in_AC_2 = 0;

				in1 	= HAL_GPIO_ReadPin(INPUT_1);
				in2 	= HAL_GPIO_ReadPin(INPUT_2);
				in3 	= HAL_GPIO_ReadPin(INPUT_3);
				in4 	= HAL_GPIO_ReadPin(INPUT_4);
				in5 	= HAL_GPIO_ReadPin(INPUT_5);
				in6 	= HAL_GPIO_ReadPin(INPUT_6);

				for(int i=0;i<25;i++)
				{
					if(HAL_GPIO_ReadPin(INPUT_AC_1) == 1)
						in_AC_1 = 1;

					if(HAL_GPIO_ReadPin(INPUT_AC_2) == 1)
						in_AC_2 = 1;

					HAL_Delay(0);
				}

				uint16_t testSendData[11] = {temp.TC1,temp.TC2,temp.TC3,
											in1,in2,in3,in4,in5,in6,in_AC_1,in_AC_2};

				DWIN_writeRegiser(testSendData, DW_TEST_TC1_ADR,sizeof(testSendData));
			}

			if(registerTable[DW_ARIZA_PAGE_NUM] != 1)
				DWIN_isitmaProcess();

			DWIN_sogutmaProcess();

		}


	}

	shiftRefresh();
	DWIN_check();
	DWIN_answerProcess();
	DWIN_Alarm();

}

static uint8_t dwin_check_counter = 0;

void DWIN_check(void)
{
	if((HAL_GetTick() - counterTick.dwinCheck) >= 500)
	{
		if((DWIN.Init != 1) && (dwin_check_counter < 5) )
		{
			uint8_t version[2];

			if(DWIN_readRegister(version, DWIN_VERSION_ADDR, sizeof(version)) == READ_OK)
			{
				dwin_check_counter = 0;

				SEGGER_RTT_printf(0,"DWIN OK ! Version : %x%x\r\n",version[0],version[1]);


				registerTable[DWIN_VERSION_ADDR] = combineBytes(version[0], version[1]);

				registerTable[APP_FIRIN_MODEL_ADR] = DW_EKRAN_PROG_CONV_VAL;

				///////////////////////////////////////////////////////////////////
				uint8_t saniye,dakika,saat,gun,hafta,ay,yil;
				DWIN_readRTC(&saniye, &dakika, &saat, &hafta, &gun, &ay, &yil);
				RTC_SetDateTime(saat, dakika, saniye, gun, ay, yil);
				///////////////////////////////////////////////////////////////////

				uint8_t dw_ekran_prg_check[2] = {0};

				DWIN_readRegister(dw_ekran_prg_check, DW_EKRAN_PROG_CHECK_ADR, sizeof(dw_ekran_prg_check));


				if(dw_ekran_prg_check[1] != DW_EKRAN_PROG_CONV_VAL)
				{
					uint16_t data = 0;
					DWIN_writeRegiser(&data, DW_EKRAN_PROG_CHECK_ADR, sizeof(data));
				}

				set_FWversion();

				for(int i=0;i<EEPROM_TABLE_LEN;i++)
				{
					uint16_t writeDwin = registerTable[eepromAddrTable[i]];
					DWIN_writeRegiser(&writeDwin, eepromAddrTable[i], sizeof(writeDwin));
				}

				setOut(Q_CONVEYOR_DIR,registerTable[DW_CONVEYOR_YON_ADR]-1);

				if(registerTable[DW_DIL_PARAM_ADR] != DW_DIL_TURKCE_VAL)
					DWIN_dilChange();

				DWIN_changeMaxSetValue(3,0,registerTable[DW_SICAKLIK_MAX_SET_PARAM_ADR]);
				DWIN_changeMaxSetValue(3,1,registerTable[DW_SICAKLIK_MAX_SET_PARAM_ADR]);

				DWIN_resetManuelPisirme();

				conveyor_freq = calculate_frequency((registerTable[DW_PISIRME_SURESI_DK_ADR]*60) + registerTable[DW_PISIRME_SURESI_SN_ADR]);

				if(registerTable[DW_BUTTON_SOUND_ADR] == 1)
					DWIN_buzzerSet(1);
				else
					DWIN_buzzerSet(0);

				DWIN_dokunmatik_aktif_msg();

				uint16_t writeData = 0;
				DWIN_writeRegiser(&writeData, DW_LOADING_PAGE_ADR, sizeof(writeData));

				DWIN_changePage(0);

				DWIN.Init = 1;

			}

			else
			{

		    	HAL_UART_DMAStop(DWIN_huart_channel);

				if (HAL_UART_DeInit(&huart3) != HAL_OK)
				{
					SEGGER_RTT_printf(0,"DeInit ERROR !!! \r\n");
					Error_Handler();
				}

				HAL_Delay(0);

				if (HAL_UART_Init(&huart3) != HAL_OK)
				{
					SEGGER_RTT_printf(0,"Init ERROR !!! \r\n");
					Error_Handler();
				}

				if(DWIN_SetUsartChannel(&huart3, USART3, &hdma_usart3_rx) != HAL_OK)
				{
				  SEGGER_RTT_printf(0,"DWIN Set Usart Channel Error ! \r\n");
				  Error_Handler();
				}

				dwin_check_counter++;

			}

			HAL_GPIO_WritePin(ESP32_EN, 0);
		}

		counterTick.dwinCheck = HAL_GetTick();
	}
}





void DWIN_answerProcess(void)
{
	if((DWIN.Init) && (DWIN.rxDoneFlag == 1))
	{
		DWIN.rxDoneFlag = 0;

		if(DWIN_receiveDataProcess() == READ_OK)
		{
			DWIN_anaSayfa();

			if((registerTable[REG_DW_MODE_INFO_ADR] == DW_MANUEL_MODE_ENTER)||(registerTable[REG_DW_MODE_INFO_ADR] == DW_RECETE_PISIRME_SAYFA_ENTER))
				DWIN_manuelSayfa();

			else if(registerTable[REG_DW_MODE_INFO_ADR] == DW_CIHAZ_TEST_SAYFA_ENTER)
				DWIN_testSayfa();


			else
				DWIN_receteSayfa();


		}

	}
}

void DWIN_testSayfa(void)
{
	uint16_t addr = combineBytes(DWIN_rxBuffer[4], DWIN_rxBuffer[5]);
	uint16_t data = combineBytes(DWIN_rxBuffer[7], DWIN_rxBuffer[8]);

	if((addr >= DW_TEST_K1_ADR)&&(addr <= DW_TEST_BUZZER_ADR))
	{
		uint32_t outputList[18] = {K1,K2,K3,K4,K5,K6,K8,K9,K10,K11,K12,K13,K14,K15,K16,K17,K18,BUZZER};

		setOut(outputList[addr-DW_TEST_K1_ADR],data);
	}
	else
	{
		switch(addr)
		{
			case DW_TEST_HEPSINIAC_ADR:

				setOut(K1|K2|K3|K4|K5|K6|K8|K9|K10|K11|K12|K13|K14|K15|K16|K17|K18,1);

				uint16_t writeData[17] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
				DWIN_writeRegiser(writeData, DW_TEST_K1_ADR, sizeof(writeData));
			break;

			case DW_TEST_HEPSINIKAPAT_ADR:

				setOut(K1|K2|K3|K4|K5|K6|K8|K9|K10|K11|K12|K13|K14|K15|K16|K17|K18|BUZZER,0);

				uint16_t writeData2[19] = {0};
				DWIN_writeRegiser(writeData2, DW_TEST_K1_ADR, sizeof(writeData2));
			break;

			case DW_TEST_EXIT_ADR:

				registerTable[REG_DW_MODE_INFO_ADR] = 0;

				setOut(K1|K2|K3|K4|K5|K6|K8|K9|K10|K11|K12|K13|K14|K15|K16|K17|K18|BUZZER,0);
				PWM_StartSmoothTransition(0, 50);
				setAnalogVoltage(0, DAC_CHANNEL_1);
				setAnalogVoltage(0, DAC_CHANNEL_2);

				uint16_t writeData3[30] = {0};
				DWIN_writeRegiser(writeData3, DW_TEST_K1_ADR, sizeof(writeData3));

			break;
		}
	}

	if((addr >= DW_TEST_1KHZ_ADR)&&(addr <= DW_TEST_4KHZ_ADR))
	{
		uint16_t writeData[4] = {0};

		writeData[addr - DW_TEST_1KHZ_ADR] = data;

		PWM_StartSmoothTransition((addr - DW_TEST_1KHZ_ADR + 1)*1000*data, 50);

		DWIN_writeRegiser(writeData, DW_TEST_1KHZ_ADR, sizeof(writeData));
	}
	else if((addr >= DW_TEST_OUT2_3V_ADR)&&(addr <= DW_TEST_OUT2_9V_ADR))
	{
		uint16_t writeData[3] = {0};

		setAnalogVoltage((addr - DW_TEST_OUT2_3V_ADR + 1) * 3.0 * data , DAC_CHANNEL_2);

		writeData[addr - DW_TEST_OUT2_3V_ADR] = data;

		DWIN_writeRegiser(writeData, DW_TEST_OUT2_3V_ADR, sizeof(writeData));
	}
	else if((addr >= DW_TEST_OUT1_5V_ADR)&&(addr <= DW_TEST_OUT1_15V_ADR))
	{
		uint16_t writeData[3] = {0};

		setAnalogVoltage((addr - DW_TEST_OUT1_5V_ADR + 1) * 5.0 * data , DAC_CHANNEL_1);

		writeData[addr - DW_TEST_OUT1_5V_ADR] = data;

		DWIN_writeRegiser(writeData, DW_TEST_OUT1_5V_ADR, sizeof(writeData));
	}
}

void DWIN_isitmaProcess(void)
{
	if(registerTable[DW_PISIRME_START_ADR] == 1)
	{
		if(registerTable[DW_UST_SICAKLIK_ADR]>registerTable[DW_UST_SICAKLIK_SET_ADR])
		{

			if(registerTable[DW_SICAKLIK_ANIM_ADR] == 1)
			{
				registerTable[DW_SICAKLIK_ANIM_ADR] = 0;

				uint16_t data = registerTable[DW_SICAKLIK_ANIM_ADR];
				DWIN_writeRegiser(&data, DW_SICAKLIK_ANIM_ADR, sizeof(data));
			}


		}
		else if((registerTable[DW_UST_SICAKLIK_ADR])<=registerTable[DW_UST_SICAKLIK_SET_ADR])
		{

			if(registerTable[DW_SICAKLIK_ANIM_ADR] == 0)
			{
				registerTable[DW_SICAKLIK_ANIM_ADR] = 1;

				uint16_t data = registerTable[DW_SICAKLIK_ANIM_ADR];
				DWIN_writeRegiser(&data, DW_SICAKLIK_ANIM_ADR, sizeof(data));
			}

		}

		Furnace_Control_1s();
	}


}

void DWIN_sogutmaProcess(void)
{
	if((registerTable[DW_PISIRME_START_ADR] == 0)&&(registerTable[REG_DW_MODE_INFO_ADR] != DW_CIHAZ_TEST_SAYFA_ENTER)&&(registerTable[DW_ARIZA_PAGE_NUM] != 1)&&(HAL_GPIO_ReadPin(I_MOTOR_ASIRI_SICAKLIK) != 0))
	{
		if(registerTable[DW_UST_SICAKLIK_ADR] > 50)
		{
			if(CheckBit(Q_SIRKULASYON_MOTORU) == 0)
				setOut(Q_SIRKULASYON_MOTORU, 1);
		}
		else
		{
			if(CheckBit(Q_SIRKULASYON_MOTORU) == 1)
				setOut(Q_SIRKULASYON_MOTORU, 0);
		}
	}
	else if((CheckBit(Q_SIRKULASYON_MOTORU) == 1)&&(HAL_GPIO_ReadPin(I_MOTOR_ASIRI_SICAKLIK) == 0)&&(registerTable[REG_DW_MODE_INFO_ADR] != DW_CIHAZ_TEST_SAYFA_ENTER))
	{
		setOut(Q_SIRKULASYON_MOTORU, 0);
	}
}

void DWIN_anaSayfa(void)
{
	uint16_t addr = combineBytes(DWIN_rxBuffer[4], DWIN_rxBuffer[5]);
	uint16_t data = combineBytes(DWIN_rxBuffer[7], DWIN_rxBuffer[8]);
	uint8_t data2[2];

	switch(addr)
	{

		case DW_MANUEL_MOD_GIRIS_ADR:

			if(data == 0)
			{
				registerTable[REG_DW_MODE_INFO_ADR] = DW_ANA_SAYFA_ENTER;

				DWIN_resetManuelPisirme();

			}


			else if(data == 1)
			{
				registerTable[REG_DW_MODE_INFO_ADR] = DW_MANUEL_MODE_ENTER;

				setOut(Q_SOGUTMA_FANI, data);


			}


		break;


		case DW_PARAMETRE_PAGE_ADR:

			if((data == registerTable[DW_PSW_PARAM_ADR])||(data == DW_PARAMETRE_MK_PSW))
			{

				DWIN_changePage(DW_PARAMETRE_PAGE_NUM);
			}

			else if(data == DW_CIHAZ_TEST_PAGE_PSW)
			{
				outputData = 0;
				DWIN_changePage(DW_CIHAZ_TEST_PAGE_ADR);
				registerTable[REG_DW_MODE_INFO_ADR] = DW_CIHAZ_TEST_SAYFA_ENTER;
			}

			else
				DWIN_changePage(DW_AYARLAR_PAGE_NUM);


		break;

		case DW_PARAMETRE_EXIT_PAGE_ADR:

			uint8_t readData[2];
			DWIN_readRegister(readData, DW_DIL_PARAM_ADR, sizeof(readData));

			if(readData[1] != registerTable[DW_DIL_PARAM_ADR])
			{
				registerTable[DW_DIL_PARAM_ADR] = readData[1];

				EEPROM_Write(&hi2c1, DW_DIL_PARAM_ADR, readData, sizeof(readData));

				DWIN_changePage(DW_EMPTY_PAGE_NUM);

				uint16_t writeData = 1;
				DWIN_writeRegiser(&writeData, DW_LOADING_PAGE_ADR, sizeof(writeData));

				DWIN_dilChange();

				EEPROM_Recete_DefaultWrite(&hi2c1);
				EEPROM_Recete_Read(&hi2c1);

				writeData = 0;
				DWIN_writeRegiser(&writeData, DW_LOADING_PAGE_ADR, sizeof(writeData));

				DWIN_changePage(0);

			}

		break;

		case DW_BUTTON_SOUND_ADR:

			registerTable[DW_BUTTON_SOUND_ADR] = data;

			if(data == 1)
				DWIN_buzzerSet(1);
			else
				DWIN_buzzerSet(0);

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_BUTTON_SOUND_ADR, data2, sizeof(data2));

		break;

		case DW_ALARM_PARAM_ADR:

			registerTable[DW_ALARM_PARAM_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_ALARM_PARAM_ADR, data2, sizeof(data2));

		break;

//		case DW_DIL_PARAM_ADR:
//
//			registerTable[DW_DIL_PARAM_ADR] = data;
//
//			parse16BitTo8Bit(data, &data2[0], &data2[1]);
//
//			EEPROM_Write(&hi2c1, DW_DIL_PARAM_ADR, data2, sizeof(data2));
//
//		break;

		case DW_PSW_PARAM_ADR:

			registerTable[DW_PSW_PARAM_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_PSW_PARAM_ADR, data2, sizeof(data2));

		break;

		case DW_FIRIN_TYPE_ADR:

			registerTable[DW_FIRIN_TYPE_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_FIRIN_TYPE_ADR, data2, sizeof(data2));

		break;

		case DW_SICAKLIK_MAX_SET_PARAM_ADR:

			registerTable[DW_SICAKLIK_MAX_SET_PARAM_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_SICAKLIK_MAX_SET_PARAM_ADR, data2, sizeof(data2));

			DWIN_changeMaxSetValue(3,0,registerTable[DW_SICAKLIK_MAX_SET_PARAM_ADR]);
			DWIN_changeMaxSetValue(3,1,registerTable[DW_SICAKLIK_MAX_SET_PARAM_ADR]);

		break;

		case DW_BRULOR_RST_PARAM_ADR:

			registerTable[DW_BRULOR_RST_PARAM_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_BRULOR_RST_PARAM_ADR, data2, sizeof(data2));

		break;

		case DW_MOTOR_FREQ_ADR:

			registerTable[DW_MOTOR_FREQ_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_MOTOR_FREQ_ADR, data2, sizeof(data2));

			conveyor_freq = calculate_frequency((registerTable[DW_PISIRME_SURESI_DK_ADR]*60) + registerTable[DW_PISIRME_SURESI_SN_ADR]);

		break;



		case DW_LOGO_PARAM_ADR:

			registerTable[DW_LOGO_PARAM_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_LOGO_PARAM_ADR, data2, sizeof(data2));

		break;


		case DW_TERMOKUPL_TYPE_PARAM_ADR:

			registerTable[DW_TERMOKUPL_TYPE_PARAM_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_TERMOKUPL_TYPE_PARAM_ADR, data2, sizeof(data2));

		break;


		case DW_ARIZA_ALARM_SUSTURMA_ADR:

			data = 1;

			registerTable[DW_ARIZA_ALARM_SUSTURMA_ADR] = 1;

			DwinAlarmFlag = 0;
			DwinAlarmBuzzer = 0;
			setOut(BUZZER, 0);

		break;

		case DW_FARBRIKA_AYAR_PARAM_ADR:

			if(data == 1)
			{
				uint8_t eraseWrite = 0;
				EEPROM_Write(&hi2c1, EEPROM_USAGE_CHECK_ADDR, &eraseWrite, 1);
				HAL_Delay(0);

				DWIN_writeRegiser((uint16_t[]){0x55aa,0x5aa5},0x0004,4);

				HAL_Delay(1000);

				NVIC_SystemReset();
			}

		break;

		case DW_FIRIN_GUC_TYPE_PARAM_ADR:

			registerTable[DW_FIRIN_GUC_TYPE_PARAM_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_FIRIN_GUC_TYPE_PARAM_ADR, data2, sizeof(data2));

		break;

		case DW_TARIH_SAAT_PAGE_ENTER_ADR:


			if(data == 1)
			{
				uint8_t saniye, dakika, saat, gun, hafta ,ay, yil;

				DWIN_readRTC(&saniye, &dakika, &saat, &hafta, &gun, &ay, &yil);
				DWIN_writeRTC(saniye, dakika, saat, gun, ay, yil, 0);
			}

		break;


		case DW_FIRST_WRITE_RTC_ADR:

			if(data == DW_WRITE_RTC_DONE_MSG)
			{
				HAL_Delay(1000);
				uint8_t saniye,dakika,saat,gun,hafta,ay,yil;
				DWIN_readRTC(&saniye, &dakika, &saat, &hafta, &gun, &ay, &yil);
				RTC_SetDateTime(saat, dakika, saniye, gun, ay, yil);
			}

		break;

	}
}
void DWIN_manuelSayfa(void)
{

	uint16_t addr = combineBytes(DWIN_rxBuffer[4], DWIN_rxBuffer[5]);
	uint16_t data = combineBytes(DWIN_rxBuffer[7], DWIN_rxBuffer[8]);
	uint8_t data2[2] = {0};

	switch(addr)
	{

		case DW_UST_SICAKLIK_SET_ONAY_ADR:

			if(data == 1)
			{
				DWIN_readRegister(data2, DW_UST_SICAKLIK_SET_ORT_ADR, sizeof(data2));

				data = combineBytes(data2[0], data2[1]);

				registerTable[DW_UST_SICAKLIK_SET_ADR] = data;

				if((registerTable[DW_UST_SICAKLIK_SET_ADR] < (soft_threshold + 5)) && (pid_active != 1))
					soft_threshold = CalculateSoftThreshold(registerTable[DW_UST_SICAKLIK_SET_ADR],temp.TC3);

				DWIN_writeRegiser(&data, DW_UST_SICAKLIK_SET_ADR, sizeof(data));

				EEPROM_Write(&hi2c1, DW_UST_SICAKLIK_SET_ADR, data2, sizeof(data2));

				if(registerTable[DW_ELEKTRIKLI_FIRIN_TYPE_VAL] == DW_ELEKTRIKLI_FIRIN_TYPE_VAL)
					Furnace_Init();


			}

		break;

		case DW_PISIRME_SURESI_DK_ONAY_ADR:

			if(data == 1)
			{
				DWIN_readRegister(data2, DW_PISIRME_SURESI_SET_DK_ADR, sizeof(data2));

				data = combineBytes(data2[0], data2[1]);
				registerTable[DW_PISIRME_SURESI_DK_ADR] = data;

				DWIN_writeRegiser(&data, DW_PISIRME_SURESI_DK_ADR, sizeof(data));

				conveyor_freq = calculate_frequency((registerTable[DW_PISIRME_SURESI_DK_ADR]*60) + registerTable[DW_PISIRME_SURESI_SN_ADR]);

				if(registerTable[DW_CONVEYOR_START_ADR] == 1)
				{
					PWM_StartSmoothTransition(conveyor_freq,50);
				}

				EEPROM_Write(&hi2c1, DW_PISIRME_SURESI_DK_ADR, data2, sizeof(data2));
			}

		break;

		case DW_PISIRME_SURESI_SN_ONAY_ADR:

			if(data == 1)
			{
				DWIN_readRegister(data2, DW_PISIRME_SURESI_SET_SN_ADR, sizeof(data2));

				data = combineBytes(data2[0], data2[1]);
				registerTable[DW_PISIRME_SURESI_SN_ADR] = data;


				DWIN_writeRegiser(&data, DW_PISIRME_SURESI_SN_ADR, sizeof(data));

				conveyor_freq = calculate_frequency((registerTable[DW_PISIRME_SURESI_DK_ADR]*60) + registerTable[DW_PISIRME_SURESI_SN_ADR]);

				if(registerTable[DW_CONVEYOR_START_ADR] == 1)
				{
					PWM_StartSmoothTransition(conveyor_freq,50);
				}

				EEPROM_Write(&hi2c1, DW_PISIRME_SURESI_SN_ADR, data2, sizeof(data2));
			}

		break;

		case DW_PISIRME_START_ADR:

			registerTable[DW_PISIRME_START_ADR] = data;
			registerTable[DW_CONVEYOR_START_ADR] = data;

			DWIN_writeRegiser(&data, DW_CONVEYOR_START_ADR, sizeof(data));

			if (data == 1)
			{
				Furnace_Init();

				setOut(Q_BRULOR_START|Q_SIRKULASYON_MOTORU,1);
				setOut(Q_CONVEYOR_EN, 0);

				if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SOL)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
				}
				else if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SAG)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
				}

				DWIN_writeRegiser(&data, DW_SICAKLIK_ANIM_ADR, sizeof(data));

				PWM_StartSmoothTransition(conveyor_freq,50);
			}

			else
			{

				setOut(Q_BRULOR_START|K2|K16,0);
				setOut(Q_CONVEYOR_EN, 1);

				HAL_DAC_SetValue(&hdac,
							   DAC_CHANNEL_1,
							   DAC_ALIGN_12B_R,
							   0);

				PWM_StartSmoothTransition(0,50);

				if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SOL)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));

					data = 2;

					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
				}
				else if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SAG)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));

					data = 2;

					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
				}


				registerTable[DW_SICAKLIK_ANIM_ADR] = 0;

				data = 0;
				DWIN_writeRegiser(&data, DW_SICAKLIK_ANIM_ADR, sizeof(data));
			}


		break;

		case DW_CONVEYOR_START_ADR:

			registerTable[DW_CONVEYOR_START_ADR] = data;

			if (data == 1)
			{

				setOut(Q_CONVEYOR_EN, 0);

				if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SOL)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
					data = 0;
					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
				}
				else if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SAG)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
					data = 0;
					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
				}

				PWM_StartSmoothTransition(conveyor_freq,50);
			}

			else
			{

				setOut(Q_CONVEYOR_EN, 1);

				if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SOL)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));

					data = 2;

					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
				}
				else if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SAG)
				{
					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));

					data = 2;

					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
				}

				PWM_StartSmoothTransition(0,50);
			}


		break;

		case DW_CONVEYOR_YON_ADR:

			registerTable[DW_CONVEYOR_YON_ADR] = data;

			parse16BitTo8Bit(data, &data2[0], &data2[1]);

			EEPROM_Write(&hi2c1, DW_CONVEYOR_YON_ADR, data2, sizeof(data2));

			setOut(Q_CONVEYOR_DIR,data-1);

			if(registerTable[DW_CONVEYOR_START_ADR] == 1)
			{
				if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SOL)
				{
					data = 1;
					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
					data = 0;
					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
				}
				else if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SAG)
				{
					data = 1;
					DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
					data = 0;
					DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
				}
			}

		break;

		case DW_BRULOR_RESET_ICON_ADR:

			if(data == 1)
			{
				setOut(Q_BRULOR_RESET, 1);
			}
			else if(data == 2)
			{
				setOut(Q_BRULOR_RESET, 0);
				DWIN_brulorArizaSet(0);
				brulor_ariza_resetleme_gormgelme_check = 1;

			}

			//counterTick.brulorResetRelay = HAL_GetTick();

		break;

	}

}

void DWIN_receteSayfa(void)
{
	uint16_t addr = combineBytes(DWIN_rxBuffer[4], DWIN_rxBuffer[5]);

	if((addr >= DW_RECETE_ILK_ADR)&&(addr <= DW_RECETE_SON_ADR))
	{

		islemdekiRecete = addr;

		uint16_t data = combineBytes(DWIN_rxBuffer[7], DWIN_rxBuffer[8]);

		uint16_t recete_num = addr - DW_RECETE_ILK_ADR + 1;


		uint8_t recete_all_data_u8[EE_RECETE_DATA_SIZE + DW_RECETE_ISIM_SIZE] = {0};

		EEPROM_Read_Safe(&hi2c1, EE_RECETE_ILK_ADR + ((recete_num - 1)*(EE_RECETE_DATA_SIZE + DW_RECETE_ISIM_SIZE)), recete_all_data_u8, sizeof(recete_all_data_u8));

		uint16_t receteResmi		=	combineBytes(recete_all_data_u8[0], recete_all_data_u8[1]);
		uint16_t receteAdi[DW_RECETE_ISIM_SIZE/2];

		for(int i=0;i<(DW_RECETE_ISIM_SIZE/2);i++)
			receteAdi[i] = combineBytes(recete_all_data_u8[EE_RECETE_DATA_SIZE + (i*2)], recete_all_data_u8[EE_RECETE_DATA_SIZE + (i*2) + 1]);

		DWIN_writeRegiser(&recete_num, DW_RECETE_DUZ_NUM_ADR, sizeof(recete_num));
		DWIN_writeRegiser(&receteResmi, DW_RECETE_DUZ_RESIM_ADR, sizeof(receteResmi));
		DWIN_writeRegiser(receteAdi, DW_RECETE_DUZ_ISIM_ADR, sizeof(receteAdi));

		switch(data)
		{

			case DW_RECETE_PISIRME_CMD:

//				islemdekiReceteAdim = 1;
//
				registerTable[REG_DW_MODE_INFO_ADR] = DW_RECETE_PISIRME_SAYFA_ENTER;
//
				for(int i=0;i<(EE_RECETE_DATA_SIZE/2)-1;i++)
				{
					uint16_t recete_pisirme_param = combineBytes(recete_all_data_u8[(i*2)+2], recete_all_data_u8[(i*2)+3]);
					DWIN_writeRegiser(&recete_pisirme_param, DW_UST_SICAKLIK_SET_ADR + (i*2), sizeof(recete_pisirme_param));
					registerTable[DW_UST_SICAKLIK_SET_ADR + (i*2)] = recete_pisirme_param;
					SEGGER_RTT_printf(0,"recete_pisirme_param :%d  \r\n", recete_pisirme_param);
				}

				conveyor_freq = calculate_frequency((registerTable[DW_PISIRME_SURESI_DK_ADR]*60) + registerTable[DW_PISIRME_SURESI_SN_ADR]);

				setOut(Q_SOGUTMA_FANI, 1);
				DWIN_changePage(DW_RECETE_PISIRME_PAGE_NUM);


			break;

			case DW_RECETE_DUZENLEME_CMD:


				for(int i=0;i<(EE_RECETE_DATA_SIZE / 2) - 1;i++)
				{
					uint16_t writeData = combineBytes(recete_all_data_u8[(i*2)+2], recete_all_data_u8[(i*2) + 3]);
					DWIN_writeRegiser(&writeData, DW_RECETE_DUZ_UST_SIC_SET_ADR + (i*2), sizeof(writeData));
				}

				DWIN_changePage(DW_PISIRME_DUZEN_PAGE1_ADR);


			break;

		}
	}

	else if(addr == DW_RECETE_CIKIS_CMD)
	{
		uint16_t data = combineBytes(DWIN_rxBuffer[7], DWIN_rxBuffer[8]);
		registerTable[REG_DW_MODE_INFO_ADR] = DW_ANA_SAYFA_ENTER;

		switch(data)
		{
			case 1:

				uint16_t recete_num = islemdekiRecete - DW_RECETE_ILK_ADR + 1;

				uint8_t Recete_data1_u8[EE_RECETE_DATA_SIZE] 						= {0};
				uint8_t Recete_data2_u8[EE_RECETE_DATA_SIZE + DW_RECETE_ISIM_SIZE] 	= {0};

				uint8_t recete_resim[2] 											= {0};
				uint16_t recete_resim_u16[1] 										= {0};
				uint8_t recete_isim[DW_RECETE_ISIM_SIZE] 							= {0};
				uint16_t recete_isim_u16[DW_RECETE_ISIM_SIZE/2] 					= {0};

				//////////////////////////////////////////////////////////////////////////////////////////////////////
				DWIN_readRegister(recete_isim, DW_RECETE_DUZ_ISIM_ADR, sizeof(recete_isim));

				uint8_t ff_check = 0;

				for(int i=0;i<DW_RECETE_ISIM_SIZE;i++)
				{
					if(ff_check != 1)
					{
						if(recete_isim[i] == 0xFF)
						{
							recete_isim[i] = 0;
							ff_check = 1;
						}
					}
					else
						recete_isim[i] = 0;
				}
				////////////////////////////////////////////////////////////////////////////////////////////////////
				for(int i=0;i<(EE_RECETE_DATA_SIZE/2)-1;i++)
				{
					uint8_t readData[2];
					DWIN_readRegister(readData, DW_RECETE_DUZ_UST_SIC_SET_ADR + (i*2), sizeof(readData));
					Recete_data1_u8[(i*2)+2] = readData[0];
					Recete_data1_u8[(i*2)+3] = readData[1];
				}
				//////////////////////////////////////////////////////////////////////////////////////////////////////

				DWIN_readRegister(recete_resim, DW_RECETE_DUZ_RESIM_ADR, sizeof(recete_resim));


				for(int i=0;i<(EE_RECETE_DATA_SIZE + DW_RECETE_ISIM_SIZE);i++)
				{
					if(i<EE_RECETE_DATA_SIZE)
						Recete_data2_u8[i] = Recete_data1_u8[i];
					else
						Recete_data2_u8[i] = recete_isim[i - EE_RECETE_DATA_SIZE];

				}

				Recete_data2_u8[0] = recete_resim[0];
				Recete_data2_u8[1] = recete_resim[1];

				convert_u8_to_u16(recete_isim, recete_isim_u16, sizeof(recete_isim));
				DWIN_writeRegiser(recete_isim_u16, DW_RECETE_ISIM_ILK_ADR + ((recete_num-1)*(DW_RECETE_ISIM_SIZE)), sizeof(recete_isim_u16));

				convert_u8_to_u16(recete_resim, recete_resim_u16, sizeof(recete_resim));
				DWIN_writeRegiser(recete_resim_u16, DW_RECETE_RESIM_ILK_ADR + (recete_num-1), sizeof(recete_resim_u16));

				EEPROM_Write(&hi2c1, EE_RECETE_ILK_ADR + ((recete_num - 1)*(EE_RECETE_DATA_SIZE + DW_RECETE_ISIM_SIZE)), Recete_data2_u8, sizeof(Recete_data2_u8));



			break;

		}
		islemdekiRecete = 0;
	}

	else if((addr >= DW_UST_SICAKLIK_SET_ONAY_ADR)&&(addr <= DW_PISIRME_SURESI_SN_ONAY_ADR))
	{
		uint16_t data = combineBytes(DWIN_rxBuffer[7], DWIN_rxBuffer[8]);

		if(data == 1)
		{
			uint16_t ortak_adress[3] ={	DW_UST_SICAKLIK_SET_ORT_ADR,
										DW_PISIRME_SURESI_SET_DK_ADR,
										DW_PISIRME_SURESI_SET_SN_ADR
									};

			uint8_t readBytes[4];

			DWIN_readRegister(readBytes, ortak_adress[addr - DW_UST_SICAKLIK_SET_ONAY_ADR], sizeof(readBytes));
			HAL_Delay(0);

			uint16_t addres 		= combineBytes(readBytes[2], readBytes[3]);
			uint16_t data_recete	= combineBytes(readBytes[0], readBytes[1]);

			DWIN_writeRegiser(&data_recete, addres, sizeof(data_recete));
		}
	}
}


DWIN_Response DWIN_buzzerSet(uint8_t setLevel)
{
	DWIN_Response response = NO_RESPONSE;

	if(setLevel)
	{
		uint16_t writeData[2] = {0x5AFF,0x90B9};
		response = DWIN_writeRegiser(writeData, 0x0080, sizeof(writeData));
	}
	else
	{
		uint16_t writeData[2] = {0x5AFF,0x90B1};
		response = DWIN_writeRegiser(writeData, 0x0080, sizeof(writeData));
	}

	return response;
}


void DWIN_resetManuelPisirme(void)
{

	DwinAlarmFlag = 0;
	DwinAlarmBuzzer = 0;

	PWM_StartSmoothTransition(0,50);

	setOut(BUZZER|Q_SOGUTMA_FANI|Q_BRULOR_START|K2|K16|Q_BRULOR_RESET, 0);
	setOut(Q_CONVEYOR_EN, 1);


	registerTable[DW_SICAKLIK_ANIM_ADR] 	= 0;
	registerTable[DW_PISIRME_START_ADR] 	= 0;
	registerTable[DW_CONVEYOR_START_ADR] 	= 0;
	registerTable[DW_BRULOR_ARIZA_ADR] 		= 0;

	uint16_t data = 0;

	if(registerTable[DW_ARIZA_PAGE_NUM] == 1)
	{
		registerTable[DW_ARIZA_PAGE_NUM]		= 0;

		data = 0;
		DWIN_writeRegiser(&data, DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR, sizeof(data));
		DWIN_writeRegiser(&data, DW_ASIRI_SICAKLIK_ARIZA_ADR, sizeof(data));
		DWIN_writeRegiser(&data, DW_TC3_ARIZA_ADR, sizeof(data));

		registerTable[DW_TC3_ARIZA_ADR] = 0;
		registerTable[DW_ASIRI_SICAKLIK_ARIZA_ADR] = 0;
		registerTable[DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR] = 0;
	}

	HAL_DAC_SetValue(&hdac,
				   DAC_CHANNEL_1,
				   DAC_ALIGN_12B_R,
				   0);



	DWIN_writeRegiser(&data, DW_SICAKLIK_ANIM_ADR, sizeof(data));
	DWIN_writeRegiser(&data, DW_CONVEYOR_START_ADR, sizeof(data));
	DWIN_writeRegiser(&data, DW_PISIRME_START_ADR, sizeof(data));
	DWIN_writeRegiser(&data, DW_ARIZA_ALARM_SUSTURMA_ADR, sizeof(data));

	if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SOL)
	{
		DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));

		data = 2;

		DWIN_writeRegiser(&data, DW_CONVEYOR_SOL_ANIM_ADR, sizeof(data));
	}
	else if(registerTable[DW_CONVEYOR_YON_ADR] == DW_CONVEYOR_SAG)
	{
		DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));

		data = 2;

		DWIN_writeRegiser(&data, DW_CONVEYOR_SAG_ANIM_ADR, sizeof(data));
	}

	DWIN_brulorArizaSet(0);

}

void DWIN_brulorArizaSet(uint8_t state)
{
	uint16_t writeData[4] = {0x5AA5,0x0052,0x0002,(uint16_t)state};
	DWIN_writeRegiser(writeData, 0x00B0, sizeof(writeData)); // Reset icon dokunmatikliği

	uint16_t writeData2 = (uint16_t)state;
	DWIN_writeRegiser(&writeData2, DW_BRULOR_RESET_ICON_ADR, sizeof(writeData2));
	writeData2 = (uint16_t)state;
	DWIN_writeRegiser(&writeData2, DW_BRULOR_ARIZA_ADR, sizeof(writeData2));

	registerTable[DW_BRULOR_ARIZA_ADR] = state;

}



void DWIN_brulorArizaReset_Relay(void)
{
	if(brulor_ariza_resetleme_gormgelme_check == 1)
	{
		if(brulor_ariza_resetleme_gormgelme_cnt > 5)
		{
			brulor_ariza_resetleme_gormgelme_cnt = 0;
			brulor_ariza_resetleme_gormgelme_check = 0;
		}

		brulor_ariza_resetleme_gormgelme_cnt++;


	}
}

void DWIN_arızaCheck(void)
{
//	if((temp.TC2 >= 500) && (registerTable[DW_TC2_ARIZA_ADR] != 1))
//	{
//		registerTable[DW_TC2_ARIZA_ADR] = 1;
//		uint16_t data = 1;
//		DWIN_writeRegiser(&data, DW_TC2_ARIZA_ADR, sizeof(data));
//	}
//
//	if((temp.TC2 < 500) && (registerTable[DW_TC2_ARIZA_ADR] != 0))
//	{
//		registerTable[DW_TC2_ARIZA_ADR] = 0;
//		uint16_t data = 0;
//		DWIN_writeRegiser(&data, DW_TC2_ARIZA_ADR, sizeof(data));
//	}

	if((temp.TC3 >= 600) && (registerTable[DW_TC3_ARIZA_ADR] != 1))
	{
		registerTable[DW_TC3_ARIZA_ADR] = 1;
		uint16_t data = 1;
		DWIN_writeRegiser(&data, DW_TC3_ARIZA_ADR, sizeof(data));
	}

//	if((temp.TC3 < 600) && (registerTable[DW_TC3_ARIZA_ADR] != 0))
//	{
//		registerTable[DW_TC3_ARIZA_ADR] = 0;
//		uint16_t data = 0;
//		DWIN_writeRegiser(&data, DW_TC3_ARIZA_ADR, sizeof(data));
//	}

	if((HAL_GPIO_ReadPin(I_ASIRI_SICAKLIK) == 0)&&(registerTable[DW_ASIRI_SICAKLIK_ARIZA_ADR] != 1))
	{
		registerTable[DW_ASIRI_SICAKLIK_ARIZA_ADR] = 1;
		uint16_t data = 1;
		DWIN_writeRegiser(&data, DW_ASIRI_SICAKLIK_ARIZA_ADR, sizeof(data));
	}
//	if((HAL_GPIO_ReadPin(I_ASIRI_SICAKLIK) == 1)&&(registerTable[DW_ASIRI_SICAKLIK_ARIZA_ADR] != 0))
//	{
//		registerTable[DW_ASIRI_SICAKLIK_ARIZA_ADR] = 0;
//		uint16_t data = 0;
//		DWIN_writeRegiser(&data, DW_ASIRI_SICAKLIK_ARIZA_ADR, sizeof(data));
//	}

	if((HAL_GPIO_ReadPin(I_MOTOR_ASIRI_SICAKLIK) == 0)&&(registerTable[DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR] != 1))
	{
		registerTable[DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR] = 1;
		uint16_t data = 1;
		DWIN_writeRegiser(&data, DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR, sizeof(data));
	}
//	if((HAL_GPIO_ReadPin(I_MOTOR_ASIRI_SICAKLIK) == 1)&&(registerTable[DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR] != 0))
//	{
//		registerTable[DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR] = 0;
//		uint16_t data = 0;
//		DWIN_writeRegiser(&data, DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR, sizeof(data));
//	}

	if(((registerTable[DW_TC2_ARIZA_ADR] == 1) ||
		(registerTable[DW_TC3_ARIZA_ADR] == 1) ||
		(registerTable[DW_BRULOR_ARIZA_ADR] == 1)||
		(registerTable[DW_ASIRI_SICAKLIK_ARIZA_ADR] == 1)||
		(registerTable[DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR] == 1))&&(registerTable[DW_ARIZA_PAGE_NUM] != 1))
	{
		//DWIN_resetManuelPisirme();

		PWM_StartSmoothTransition(0, 50);
		Set_Heater_DAC(0);

		registerTable[DW_ARIZA_PAGE_NUM] = 1;
		DWIN_changePage(DW_ARIZA_PAGE_NUM);

		setOut(K2|Q_SIRKULASYON_MOTORU|K16, 0);

		if(registerTable[DW_FIRIN_TYPE_ADR] == DW_ELEKTRIKLI_FIRIN_TYPE_VAL)
			setOut(Q_BRULOR_START, 0);
		else if(registerTable[DW_BRULOR_ARIZA_ADR] != 1)
		{
			setOut(Q_BRULOR_START, 0);
		}

		DwinAlarmFlag = 1;
		alarmBuzzerPeriod = 200;
	}

	if((registerTable[DW_TC2_ARIZA_ADR] == 0) &&
		(registerTable[DW_TC3_ARIZA_ADR] == 0) &&
		(registerTable[DW_BRULOR_ARIZA_ADR] == 0) &&
		(registerTable[DW_ASIRI_SICAKLIK_ARIZA_ADR] == 0) &&
		(registerTable[DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR] == 0) &&
		(registerTable[DW_ARIZA_PAGE_NUM] != 0))
	{
		registerTable[DW_ARIZA_PAGE_NUM] = 0;
		registerTable[DW_ARIZA_ALARM_SUSTURMA_ADR] = 0;

		uint16_t data = 0;
		DWIN_writeRegiser(&data, DW_ARIZA_ALARM_SUSTURMA_ADR, sizeof(data));

		if(registerTable[REG_DW_MODE_INFO_ADR] == DW_MANUEL_MODE_ENTER)
			DWIN_changePage(DW_PISIRME_PAGE_NUM);
		else if(registerTable[REG_DW_MODE_INFO_ADR] == DW_RECETE_PISIRME_SAYFA_ENTER)
			DWIN_changePage(DW_RECETE_PISIRME_PAGE_NUM);

		if(registerTable[MANUEL_SURE_SONU_ADR] != 1)
		{
			DwinAlarmFlag = 0;
			DwinAlarmBuzzer = 0;
			setOut(BUZZER, 0);
		}

		alarmBuzzerPeriod = 1000;

		if(registerTable[DW_PISIRME_START_ADR] == 1)
		{
			if(CheckBit(Q_BRULOR_START) == 0)
			{
				Furnace_Init();
				setOut(Q_BRULOR_START, 1);
			}
			if(CheckBit(Q_SIRKULASYON_MOTORU) == 0)
			{
				setOut(Q_SIRKULASYON_MOTORU, 1);
			}

		}

		if(registerTable[DW_CONVEYOR_START_ADR] == 1)
		{
			PWM_StartSmoothTransition(conveyor_freq, 50);
		}

	}
}

void DWIN_Alarm(void)
{
	if(DwinAlarmFlag)
	{
		if((HAL_GetTick() - counterTick.DwinAlarm) >= alarmBuzzerPeriod)
		{
			counterTick.DwinAlarm = HAL_GetTick();

			DwinAlarmBuzzer = !DwinAlarmBuzzer;
			setOut(BUZZER, DwinAlarmBuzzer);

		}
	}
}



void DWIN_writeRTC(uint8_t saniye, uint8_t dakika, uint8_t saat, uint8_t gun, uint8_t ay, uint8_t yil, uint8_t writeEN)
{
	uint16_t rtcEnable_msg = DW_WRITE_RTC_DONE_MSG;

	uint16_t dk_sn 		= combineBytes(dakika, saniye);
	uint16_t gun_saat 	= combineBytes(gun, saat);
	uint16_t yil_ay		= combineBytes(yil, ay);

	if(writeEN)
	{
		uint16_t writeBuffer[4] = {rtcEnable_msg, yil_ay, gun_saat, dk_sn};
		DWIN_writeRegiser(writeBuffer, DW_FIRST_WRITE_RTC_ADR, sizeof(writeBuffer));
	}
	else
	{
		uint16_t writeBuffer[3] = {yil_ay, gun_saat, dk_sn};
		DWIN_writeRegiser(writeBuffer, DW_FIRST_WRITE_RTC_ADR + 1, sizeof(writeBuffer));
	}

}

void DWIN_readRTC(uint8_t* saniye, uint8_t* dakika, uint8_t* saat, uint8_t* hafta, uint8_t* gun, uint8_t* ay, uint8_t* yil)
{
	uint8_t readBuffer[8] = {0};

	DWIN_readRegister(readBuffer, DW_FIRST_READ_RTC_ADR, sizeof(readBuffer));

	*yil 	= readBuffer[0];
	*ay	 	= readBuffer[1];
	*gun 	= readBuffer[2];
	*hafta	= readBuffer[3];
	*saat 	= readBuffer[4];
	*dakika = readBuffer[5];
	*saniye = readBuffer[6];
}

void PWM_SetFreqAndDuty(uint32_t freq_hz, uint8_t duty_percent)
{
    uint32_t timer_clk = HAL_RCC_GetPCLK2Freq();

    uint32_t arr = 0;

    if ((RCC->CFGR & RCC_CFGR_PPRE2) != RCC_CFGR_PPRE2_DIV1)
        timer_clk *= 2;

    uint32_t prescaler = htim1.Init.Prescaler + 1;

    if(freq_hz != 0)
    	arr = (timer_clk / (prescaler * freq_hz)) - 1;

    uint32_t ccr = ((arr + 1) * duty_percent) / 100;

    __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, ccr);
}

uint32_t calculate_frequency(uint32_t duration_sec)
{
    if(duration_sec == 0)
        return 0;   // bölme hatası koruması

    uint32_t regValue = registerTable[DW_MOTOR_FREQ_ADR];

    uint64_t temp = 1400 * regValue;

    temp /= duration_sec;

    return (uint32_t)temp;
}

void PWM_StartSmoothTransition(uint32_t new_freq, uint8_t duty_percent) {
    start_freq = current_freq; // Şu an kaçtaysak oradan başla
    target_freq = new_freq;
    current_duty = duty_percent;
    tick_counter = 0;          // Sayacı sıfırla
    is_transitioning = 1;      // Süreci başlat
}

void PWM_SmoothTask_1ms(void) {
    if (is_transitioning) {
        tick_counter++;

        if (tick_counter <= 1000) {
            // Ara frekans hesaplama: start + (fark * tick / 1000)
            // Not: Çarpma işlemi uint32 sınırını aşmasın diye (int64_t) cast ediyoruz.
            // STM32 için bu işlem float'tan çok daha hızlıdır.
            int64_t delta = (int64_t)target_freq - (int64_t)start_freq;
            current_freq = (uint32_t)((int64_t)start_freq + (delta * tick_counter) / 1000);

            // PWM güncelleme
            PWM_SetFreqAndDuty(current_freq, current_duty);
        }

        if (tick_counter >= 1000) {
            // Garantiye al: Tam hedef değere eşitle ve dur
            current_freq = target_freq;
            PWM_SetFreqAndDuty(current_freq, current_duty);
            is_transitioning = 0;
            tick_counter = 0;
        }
    }
}

