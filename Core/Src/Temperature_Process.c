/*
 * Temperature_Process.c
 *
 *  Created on: Jan 20, 2025
 *      Author: Step
 */

// Test Commit

#include "Temperature_Process.h"
#include "SEGGER_RTT.h"
#include "math.h"
#include "TMP112.h"
#include "DWIN_Adress.h"
#include "DWIN_Process.h"

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;

uint8_t adcInitFlag = 0;

uint32_t adcBuffer[numOfChannel];
uint32_t tempBuffer[numOfChannel];
uint32_t sumAdcBuffer[numOfChannel];
uint32_t avgAdcBuffer[numOfChannel];
uint32_t avgCounter = 0;

uint16_t vref;

TemperatureData temp;
//HDC1080 hdcSensor;
TMP112 tmpSensor;

extern uint8_t templog_free;
extern uint16_t registerTable[REGISTER_TABLE_SIZE];
uint16_t templog_amount = 0;

//////////////////////////////////////////////////////////////////////////////////////////

int calculate_mcp9700(uint16_t adc_value);

/////////////////////////////////////////////////////////////////////////////////////////

int calculate_termocouple(uint16_t adc_value)
{
    float ref_adc_value = 500;    	// 23.5°C'deki referans ADC değeri
    float ref_temp = temp.TMP;    	// Referans sıcaklık (°C)
    float adc_coefficient; 			// ADC katsayısı (°C/LSB)

//    if(adc_value < 552)
//    	adc_coefficient = 4.325;

    if(adc_value < 620)
    	adc_coefficient = 5.80;

    else if((adc_value >= 620)&&(adc_value < 732))
    	adc_coefficient = 5.81;

    else if((adc_value >= 732)&&(adc_value < 848))
    	adc_coefficient = 5.85;

    else if((adc_value >= 848)&&(adc_value < 1025))
    	adc_coefficient = 5.87;

    else if((adc_value >= 1025)&&(adc_value < 1153))
    	adc_coefficient = 5.9;

    else if((adc_value >= 1153)&&(adc_value < 1334))
    	adc_coefficient = 5.93;

    else if((adc_value >= 1334)&&(adc_value < 1461))
    	adc_coefficient = 5.96;

    else if((adc_value >= 1461)&&(adc_value < 1649))
    	adc_coefficient = 5.98;

    else if((adc_value >= 1649)&&(adc_value < 1900))
    	adc_coefficient = 6.01;

    else if(adc_value >= 1900)
    	adc_coefficient = 6.07;

//    else if(adc_value >= 1849)
//    	adc_coefficient = 5.975;





    // 300 5.975
    // 260 5.935
    // 220 5.895
    // 190 5.825
    // 170 5.755
    // 140 5.685
    // 120 5.535
    // 90  5.365
    // 70  5.155
    // 50  4.825

    return (adc_value - ref_adc_value) / adc_coefficient + ref_temp;
}

int calculate_mcp9700(uint16_t adc_value) {
    // ADC değerinden voltajı hesapla
    float vout_mV = ((float)adc_value * vref) / 4095;

    // Sıcaklığı hesapla (Celsius)
    float temperature = (vout_mV - 500) / 10;

    return temperature;
}

uint16_t calculate_vref(uint16_t adc_vrefint) {
    // VREFINT için kalibrasyon değerini okuyoruz
    uint16_t vrefint_cal = VREFINT_CAL_VAL;

    // Gerçek referans voltajını hesaplıyoruz (mV cinsinden)
    float vref_actual = (float)vrefint_cal * VREF_CAL_VALUE / adc_vrefint;

    // mV'den Volt'a dönüştürüyoruz
    return vref_actual;
}


HAL_StatusTypeDef adc_Init(void)
{
	HAL_StatusTypeDef response = HAL_ERROR;

	if(HAL_ADCEx_Calibration_Start(&hadc1) == HAL_OK)
	{
		response = HAL_ADC_Start_DMA(&hadc1,adcBuffer,numOfChannel);
	}

	if(response == HAL_OK)
	{
		HAL_Delay(0);
		adcInitFlag = 1;
	}

	return response;

}

int ustOnSicaklik 		= 0;
int ustArkaSicaklik 	= 0;
int altSicaklik 		= 0;

void avgAdcProcess(void) // ms period function
{
	if(adcInitFlag)
	{

		for(int j=0;j<numOfChannel;j++)
		{
			sumAdcBuffer[j] += adcBuffer[j];
		}

		avgCounter++;

		if(avgCounter >= numOfSample)
		{
			for(int j=0;j<numOfChannel;j++)
			{
				avgAdcBuffer[j] = sumAdcBuffer[j] / numOfSample;
				sumAdcBuffer[j] = 0;
			}

			avgCounter = 0;
		}

	}
}

/* KALİBRASYON VERİLERİ
 * Referans 1: 75 Derece -> ADC 730
 * Referans 2: 150 Derece -> ADC 1075
 *
 * Hesaplanan Eğim (Slope): 4.6 ADC/Derece
 * Hesaplanan Offset (Sıfır Noktası): 500
 */
#define ADC_OFFSET_K      510

float ConvertValueforKType(uint16_t input)
{
    if (input <= 550)
        return 5.10f;

    if (input >= 2100)
        return 4.69f;

    float t = (float)(input - 550) / (2100.0f - 550.0f); // 0..1

    // %10 daha agresif ease-out
    float curved = 1.0f - powf((1.0f - t), 2.2f);

    return 5.10f + curved * (4.69f - 5.10f);
}

int calculateThermocouple_K(uint16_t adc_value) {
    // 1. Adım: ADC değerinden sanal sıfır noktasını çıkar (Negatif olabilir)
    int32_t adc_difference = (int32_t)adc_value - ADC_OFFSET_K;

    float ADC_PER_DEGREE = ConvertValueforKType(adc_value);

    // 2. Adım: ADC farkını sıcaklık farkına çevir
    // float işlemi ile hassas bölme yapıp int'e çeviriyoruz.
    int32_t delta_temp = (int32_t)(adc_difference / ADC_PER_DEGREE);

    // 3. Adım: Ortam sıcaklığını (Cold Junction) ekle
    int result_temp = delta_temp + temp.TMP;

    return result_temp;
}

#define ADC_OFFSET_J      515

float ConvertValueforJType(uint16_t input)
{
    if (input <= 550)
        return 5.80f;     // artık düşükte küçük

    if (input >= 2500)
        return 6.30f;     // yüksekte büyük

    float t = (float)(input - 550) / (2500.0f - 550.0f); // 0..1

    float curved = 1.0f - powf((1.0f - t), 1.5f);

    return 5.80f + curved * (6.30f - 5.80f);
}

int calculateThermocouple_J(uint16_t adc_value) {
    // 1. Adım: ADC değerinden sanal sıfır noktasını çıkar (Negatif olabilir)
    int32_t adc_difference = (int32_t)adc_value - ADC_OFFSET_J;

    float ADC_PER_DEGREE = ConvertValueforJType(adc_value);

    // 2. Adım: ADC farkını sıcaklık farkına çevir
    // float işlemi ile hassas bölme yapıp int'e çeviriyoruz.
    int32_t delta_temp = (int32_t)(adc_difference / ADC_PER_DEGREE);

    // 3. Adım: Ortam sıcaklığını (Cold Junction) ekle
    int result_temp = delta_temp + temp.TMP;

    return result_temp;
}

uint8_t intTempSampleCounter = 0;
//float sht40_temp,sht40_hum;
//float hdc_temp,hdc_hum;
float tmp_temp;
void calculate_temperature(void)
{

	if(intTempSampleCounter == 0)
	{
		//sht4xGetEvent(&sht40_temp, &sht40_hum);
		//temp.SHT40 = sht40_temp;

//		if(HDC1080_ReadTemperatureAndHumidity(&hdcSensor, &hdc_temp, &hdc_hum) == HAL_OK)
//			temp.HDC = hdc_temp;
//
//		else
//			SEGGER_RTT_printf(0," HDC1080 Read Temp and Humidty ERROR ! \r\n");

		if(TMP112_ReadTemperature(&tmpSensor, &tmp_temp) == HAL_OK)
			temp.TMP = tmp_temp;
		else
			SEGGER_RTT_printf(0," TMP112 Read Temp ERROR ! \r\n");
	}

	intTempSampleCounter++;

	if(intTempSampleCounter >= 5)
		intTempSampleCounter = 0;



	vref 			= calculate_vref(avgAdcBuffer[VREF_ROW_BUFFER]);

	if(registerTable[DW_TERMOKUPL_TYPE_PARAM_ADR] == DW_K_TYPE_TERMOKUP_VAL)
	{
		temp.TC1 		= calculateThermocouple_K(avgAdcBuffer[TC1_ROW_BUFFER]);
		temp.TC2 		= calculateThermocouple_K(avgAdcBuffer[TC2_ROW_BUFFER]);
		temp.TC3 		= calculateThermocouple_K(avgAdcBuffer[TC3_ROW_BUFFER]);
	}

	else if(registerTable[DW_TERMOKUPL_TYPE_PARAM_ADR] == DW_J_TYPE_TERMOKUP_VAL)
	{
		temp.TC1 		= calculateThermocouple_J(avgAdcBuffer[TC1_ROW_BUFFER]);
		temp.TC2 		= calculateThermocouple_J(avgAdcBuffer[TC2_ROW_BUFFER]);
		temp.TC3 		= calculateThermocouple_J(avgAdcBuffer[TC3_ROW_BUFFER]);
	}


	ustOnSicaklik 	= temp.TC1;
	ustArkaSicaklik = temp.TC3;
	altSicaklik 	= temp.TC2;


	#if DEBUG_Temp == 1
	SEGGER_RTT_printf(0,"--------------------------------------------------------- \r\n");
	SEGGER_RTT_printf(0,"VREF_mv :  %d  MCP9700 :     %d    TC1 Temp : %d   TC2 Temp : %d   TC3 Temp : %d \r\n",
						vref, temp.MCP9700, temp.TC1, temp.TC2, temp.TC3);

	SEGGER_RTT_printf(0,"VREF Val : %d  MPC9700 Val : %d  TC1 Val  : %d  TC2 Val  : %d  TC3 Val  : %d \r\n",
						avgAdcBuffer[VREF_ROW_BUFFER],avgAdcBuffer[MCP9700_ROW_BUFFER],avgAdcBuffer[TC1_ROW_BUFFER],avgAdcBuffer[TC2_ROW_BUFFER],avgAdcBuffer[TC3_ROW_BUFFER]);

	#endif



}
