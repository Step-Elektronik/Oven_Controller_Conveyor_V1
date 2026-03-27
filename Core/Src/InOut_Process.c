/*
 * InOut_Process.c
 *
 *  Created on: Jan 21, 2025
 *      Author: Step
 */


#include "InOut_Process.h"
#include "spi.h"
#include "SEGGER_RTT.h"
#include "DWIN_Process.h"

uint32_t outputData = 0;
extern tickCounter counterTick;
extern uint16_t registerTable[REGISTER_TABLE_SIZE];

void ShiftRegister_SendData(uint32_t data)
{
    HAL_GPIO_WritePin(LATCH_PORT, LATCH_PIN, GPIO_PIN_RESET);

    uint8_t bytes[4];
    bytes[0] = (data >> 24) & 0xFF;
    bytes[1] = (data >> 16) & 0xFF;
    bytes[2] = (data >> 8) & 0xFF;
    bytes[3] = data & 0xFF;

    HAL_SPI_Transmit(&hspi1, bytes, 4, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(LATCH_PORT, LATCH_PIN, GPIO_PIN_SET);

    for(int i=0;i<10;i++);

    HAL_GPIO_WritePin(LATCH_PORT, LATCH_PIN, GPIO_PIN_RESET);
}

void ShiftRegister_RefreshData(uint32_t data)
{
    uint8_t bytes[4];
    bytes[0] = (data >> 24) & 0xFF;
    bytes[1] = (data >> 16) & 0xFF;
    bytes[2] = (data >> 8) & 0xFF;
    bytes[3] = data & 0xFF;

    HAL_SPI_Transmit(&hspi1, bytes, 4, HAL_MAX_DELAY);
}

uint8_t CheckBit(uint32_t bitAddr)
{
    return (outputData & bitAddr) ? 1U : 0U;
}

void setBit(uint32_t *data, uint32_t bitMask)
{
    *data |= bitMask;
}

// Belirtilen bit maskesine göre biti reset eder (0 yapar)
void resetBit(uint32_t *data, uint32_t bitMask)
{
    *data &= ~bitMask;
}

void setOut(uint32_t outputAddr, uint8_t status)
{
	(status == 1) ? setBit(&outputData,outputAddr) : resetBit(&outputData,outputAddr);
	ShiftRegister_SendData(outputData);
}

void setOutData(uint32_t outputAddr, uint8_t status)
{
	(status == 1) ? setBit(&outputData,outputAddr) : resetBit(&outputData,outputAddr);
}

void shiftRefresh(void)
{
	ShiftRegister_SendData(outputData);
}

void shiftDataRefresh(void)
{
	ShiftRegister_RefreshData(outputData);
}

void I_AC_Check(void) // 1 saniyelik periyot da kontrol edilmeli
{
	if((counterTick.input_AC_2 >= 45)&&(CheckBit(Q_BRULOR_RESET) == 0))
	{
		if(registerTable[DW_BRULOR_ARIZA_ADR] == 0)
			DWIN_brulorArizaSet(1);

		counterTick.input_AC_2 = 0;
	}
	else if(registerTable[DW_BRULOR_ARIZA_ADR] == 1)
	{
		DWIN_brulorArizaSet(0);
		counterTick.input_AC_2 = 0;
	}
	else
	{
		counterTick.input_AC_2 = 0;
	}
}
