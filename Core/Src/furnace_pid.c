/*
 * Fırın Sıcaklık Kontrolü - Soft PID Örneği
 *
 * Çalışma Mantığı:
 *   - Hedef sıcaklığın %80'ine kadar → tam güç (DAC: 3500)
 *   - %80 ve üzeri                    → PID devreye girer (DAC: 1900–3500)
 *
 * Ölçüm periyodu: 1 saniye
 * Sıcaklık değişkeni: temp.TC3 (int türünde okunuyor)
 */

#include "main.h"
#include "pid.h"
#include "dac.h"
#include "DWIN_Process.h"
#include "Temperature_Process.h"
#include "SEGGER_RTT.h"
#include "InOut_Process.h"
#include "math.h"

/* ─────────────────────────────────────────────
   Global Değişkenler
───────────────────────────────────────────── */
extern uint16_t registerTable[REGISTER_TABLE_SIZE];
extern TemperatureData temp;
uint16_t pwm_counter = 0; // Ortak sayaç (1000 ms için)
uint8_t pwm1_duty = 0;


/* ─────────────────────────────────────────────
   Kullanıcı Ayarları
───────────────────────────────────────────── */
#define SOFT_START_RATIO    0.70    /* Tam güç → PID geçiş eşiği (%70)              */
double DAC_MAX 	= 	100.0;  /* Maksimum DAC değeri (tam güç)                 */
double DAC_MIN	= 	0.0;  /* Minimum DAC değeri                            */

/*
 * PID Katsayıları — Yüksek ateletli fırın, DAC_MIN'den başlayan PID
 *
 *  Kp = 12.0 → DAC_MIN'den başladığı için hedefe ulaşmak için daha
 *               agresif itme gerekir, overshoot riski artık düşük
 *
 *  Ki = 0.08 → Steadystate hatayı kapatır, windup riski azaldı çünkü
 *               PID artık 1900'den yukarı çıkarak çalışıyor
 *
 *  Kd = 8.0  → Artık frenleme görevi SOFT_START mantığında yapıldı,
 *               Kd sadece ince stabilizasyon için kullanılıyor
 *
 *  NOT: Sisteminize göre ince ayar sırası: önce Kp, sonra Ki, en son Kd
 */
double PID_KP 	= 	5.0;
double PID_KI 	=	0.05;
double PID_KD 	=	0.0;

/* ─────────────────────────────────────────────
   Forward Declaration
───────────────────────────────────────────── */
void Set_Heater_DAC(uint32_t value);
float CalculateSoftThreshold(float setTemp, float currentTemp);
void type1_oven_heating_process(void);
void type2_oven_heating_process(void);
/* ─────────────────────────────────────────────
   Global Değişkenler
───────────────────────────────────────────── */
static PID_TypeDef furnacePID;

static int    pid_input    = 0;      /* PID girişi  → temp.TC3 buraya kopyalanır  */
static double pid_output   = 0.0;    /* PID çıkışı  → DAC değeri (1900–3500)      */
static double pid_setpoint = 0.0;    /* Hedef sıcaklık → registerTable'dan okunur */

uint8_t  		pid_active   = 0;    /* PID henüz başlatıldı mı?      */
static uint32_t tick_counter = 0;    /* Saniye sayacı (ms cinsinden)  */

double soft_threshold;

/* ─────────────────────────────────────────────
   Başlangıç — bir kez çağrılır (main veya init)
───────────────────────────────────────────── */
void Furnace_Init(void)
{

	switch(registerTable[DW_FIRIN_TYPE_ADR])
	{
		case DW_ELEKTRIKLI_FIRIN_TYPE_VAL:

			DAC_MAX = 100.0;
			DAC_MIN = 0.0;

			PID_KP 	= 	6.0;
			PID_KI 	=	0.05;
			PID_KD 	=	0.0;

			if((registerTable[DW_UST_SICAKLIK_SET_ADR] - temp.TC3) > 30)
				soft_threshold = CalculateSoftThreshold(registerTable[DW_UST_SICAKLIK_SET_ADR],temp.TC3);

		break;

		case DW_LPG_FIRIN_TYPE_VAL:

			DAC_MAX = 2200.0;
			DAC_MIN = 0.0;

			PID_KP 	= 	40.0;
			PID_KI 	=	0.25;
			PID_KD 	=	0.0;

			soft_threshold = 0;

		break;

		case DW_DOGALGAZ_AO_FIRIN_TYPE_VAL:

			DAC_MAX = 1600.0;
			DAC_MIN = 0.0;

			PID_KP 	= 	40.0;
			PID_KI 	=	0.25;
			PID_KD 	=	0.0;

			soft_threshold = 0;

		break;
	}
    /* PID nesnesini yapılandır */

    PID(&furnacePID,
        &pid_input,          /* Giriş  : int*  (temp.TC3 buraya yazılacak) */
        &pid_output,         /* Çıkış  : double*           					*/
        &pid_setpoint,       /* Hedef  : double*                            */
        PID_KP, PID_KI, PID_KD,
        _PID_P_ON_E,         /* Proportional on Error — fırın için uygun   	*/
        _PID_CD_DIRECT);

    PID_SetOutputLimits(&furnacePID, DAC_MIN, DAC_MAX);
    PID_SetSampleTime(&furnacePID, 1000); /* 1000 ms = 1 saniye              */

    /* Başlangıçta PID manuel modda, tam güç uyguluyoruz */
    PID_SetMode(&furnacePID, _PID_MODE_MANUAL);
    pid_output  = DAC_MAX;
    pid_active  = 0;
    tick_counter = 0;
}

/* ─────────────────────────────────────────────
   Ana Döngü — Her 1 saniyede bir çağrılır
   (Timer interrupt, RTOS task veya while döngüsü)
───────────────────────────────────────────── */

void Furnace_Control_1s(void)
{

	if(registerTable[DW_FIRIN_TYPE_ADR] == DW_DOGALGAZ_DO_FIRIN_TYPE_VAL)
	{
		if((temp.TC3 < (registerTable[DW_UST_SICAKLIK_SET_ADR] - 4))&&(CheckBit(K16) == 0))
		{
			setOut(K16, 1);
		}

		if((temp.TC3 > registerTable[DW_UST_SICAKLIK_SET_ADR])&&(CheckBit(K16) == 1))
		{
			setOut(K16, 0);
		}
	}
	else
	{

		/* 1. Set sıcaklığını register'dan oku (her döngüde güncellenir) */
		pid_setpoint = (double)registerTable[DW_UST_SICAKLIK_SET_ADR];

		/* 2. Mevcut sıcaklığı oku */
		pid_input = temp.TC3;

		/* 3. Soft-start eşiğini dinamik hesapla (%80 × set değeri) */

		SEGGER_RTT_printf(0,"sicaklik : %d pid output :%d soft_threshold :  %d\r\n",temp.TC3, (uint32_t)pid_output, (uint32_t)soft_threshold);

		/* 4. Soft-start kontrolü */
		if (!pid_active)
		{
			if ((double)pid_input >= soft_threshold)
			{
				/*
				 * Eşiğe ulaşıldı → Hemen fren uygula.
				 * pid_output DAC_MIN'e çekiliyor ki PID_Init() OutputSum'u
				 * oradan başlatsın. Fırının kendi ataletini hesaba katarak
				 * PID sıfırdan yukarı doğru kontrol eder, overshoot engellenir.
				 */

				pid_output = DAC_MIN;
				PID_SetMode(&furnacePID, _PID_MODE_AUTOMATIC);
				pid_active = 1;

			}
			else
			{
				/* Henüz eşiğe gelmedik → tam güç */
				pid_output = DAC_MAX;

				if((registerTable[DW_FIRIN_TYPE_ADR] == DW_LPG_FIRIN_TYPE_VAL)||(registerTable[DW_FIRIN_TYPE_ADR] == DW_DOGALGAZ_AO_FIRIN_TYPE_VAL))
					Set_Heater_DAC((uint32_t)pid_output);

				return;
			}
		}




		if(registerTable[DW_FIRIN_TYPE_ADR] == DW_ELEKTRIKLI_FIRIN_TYPE_VAL)
		{
			if(pid_active == 1)
			{
				if(registerTable[DW_FIRIN_GUC_TYPE_PARAM_ADR] == 1)
				{
					type1_oven_heating_process();

				}
				else
				{
					type2_oven_heating_process();
				}
			}
		}

		else
		{
			/* 5. PID hesapla */
			tick_counter += 1000;
			PID_Compute(&furnacePID, tick_counter);
		}

		/* 6. Çıkışı ısıtıcıya uygula */
		if((registerTable[DW_FIRIN_TYPE_ADR] == DW_LPG_FIRIN_TYPE_VAL)||(registerTable[DW_FIRIN_TYPE_ADR] == DW_DOGALGAZ_AO_FIRIN_TYPE_VAL))
			Set_Heater_DAC((uint32_t)pid_output);

	}
}

/* ─────────────────────────────────────────────
   Yardımcı: Isıtıcı DAC yazma
   Kanal ve hizalama ayarını donanımınıza göre düzenleyin
───────────────────────────────────────────── */
void Set_Heater_DAC(uint32_t value)
{
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);
}

void manual_pwm_update(void)
{
	if((registerTable[DW_PISIRME_START_ADR] == 1)&&(registerTable[DW_ARIZA_PAGE_NUM] != 1)&&(registerTable[DW_FIRIN_TYPE_ADR] == DW_ELEKTRIKLI_FIRIN_TYPE_VAL))
	{
		// PWM1
		if (pwm_counter < ((uint32_t)pid_output * 10))
			setOutData(K2, 1);
		else
			setOutData(K2, 0);


		// Sayaç güncelleme
		pwm_counter++;
		if (pwm_counter >= 1000) {
			pwm_counter = 0;
		}
	}
}

float CalculateSoftThreshold(float setTemp, float currentTemp)
{
	float softThreshold;

	if((setTemp-currentTemp)<150)
	{
		if(currentTemp>setTemp)
			softThreshold = setTemp + ((setTemp - (double)temp.TC3) * 0.30);
		else
			softThreshold = setTemp - ((setTemp - (double)temp.TC3) * 0.30);

		return softThreshold;
	}

    float diff = fabsf(setTemp - currentTemp);

    if (diff <= 0.01f)
        return setTemp;

    const float alpha = 0.046f;   // Ayar katsayısı

    float k = 1.0f / (1.0f + alpha * diff);

    softThreshold = setTemp - (diff * k);

    return softThreshold;
}

double type1_outputCalculate(uint16_t setTemp)
{
    double output;

    // lineer yaklaşık (220 -> 60, 460 -> 100)
    output = 60.0 + ( (double)(setTemp - 220) * (40.0 / (460.0 - 220.0)) );

    // sınırlandırma (clamp)
    if(output > 100.0) output = 100.0;
    if(output < 50.0)  output = 50.0;

    return output;
}

void type1_oven_heating_process(void)
{
	if(temp.TC3<(registerTable[DW_UST_SICAKLIK_SET_ADR] - 6))
	{

		pid_output = 100;//(uint16_t)type1_outputCalculate(registerTable[DW_UST_SICAKLIK_SET_ADR]);//100

		tick_counter = 0;
		furnacePID.OutputSum = 0;
	}

	else
	{

		if(tick_counter == 0)
		{
			furnacePID.OutputSum = pid_output - 30;

			if(pid_output == 100)
				pid_output = 90;

			PID_SetOutputLimits(&furnacePID, 20, pid_output + 10);
			PID_SetTunings(&furnacePID, 5.0, 0.03, 0.0);

		}


		tick_counter += 1000;
		PID_Compute(&furnacePID, tick_counter);
	}
}

double type2_outputCalculate(uint16_t setTemp)
{
    double output;

    // 320 -> 60, 450 -> 90
    output = 60.0 + ((double)(setTemp - 320) * (30.0 / (450.0 - 320.0)));

    // clamp
    if(output > 100.0) output = 100.0;
    if(output < 50.0) output = 50.0;

    return output;
}

void type2_oven_heating_process(void)
{

	if(temp.TC3<(registerTable[DW_UST_SICAKLIK_SET_ADR] - 6))
	{

		pid_output = (uint16_t)type2_outputCalculate(registerTable[DW_UST_SICAKLIK_SET_ADR]);

		tick_counter = 0;
		furnacePID.OutputSum = 0;
	}

	else
	{

		if(tick_counter == 0)
		{
			furnacePID.OutputSum = pid_output - 30;

			if(pid_output == 100)
				pid_output = 90;

			PID_SetOutputLimits(&furnacePID, 20, pid_output + 10);
			PID_SetTunings(&furnacePID, 5.0, 0.03, 0.0);

		}

		tick_counter += 1000;
		PID_Compute(&furnacePID, tick_counter);
	}

}
