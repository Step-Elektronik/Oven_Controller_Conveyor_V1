/*
 * DWIN_Adress.h
 *
 *  Created on: Jan 23, 2025
 *      Author: Step
 */

#ifndef INC_DWIN_ADRESS_H_
#define INC_DWIN_ADRESS_H_

#include "main.h"

#define STM32_OTA_BEGIN_ADR 100

/*---------------------------- SAYFALAR ------------------------------*/

#define MANUEL_SURE_SONU_ADR			0x53
#define RECETE_SURE_SONU_ADR			0x54

#define DW_ARIZA_PAGE_NUM				82
#define DW_EMPTY_PAGE_NUM				81

#define DW_PISIRME_PAGE_NUM				2
#define DW_AYARLAR_PAGE_NUM				6
#define DW_PARAMETRE_PAGE_NUM			40
#define DW_RECETE_PISIRME_PAGE_NUM		41

#define DW_PISIRME_DUZEN_PAGE1_ADR		25

#define DW_CIHAZ_TEST_PAGE_ADR			85
#define DW_CIHAZ_TEST_PAGE_PSW			9905

#define DW_LOADING_PAGE_ADR				0xF100

#define DW_EKRAN_PROG_CHECK_ADR				0x7995

#define DW_EKRAN_PROG_MASTER_VAL			1
#define DW_EKRAN_PROG_CONV_VAL				2

/*---------------------------- TEST SAYFASI ------------------------------*/

#define DW_TEST_TC1_ADR				0x2900
#define DW_TEST_TC2_ADR				0x2901
#define DW_TEST_TC3_ADR				0x2902

#define DW_TEST_K1_ADR 				0x290B
#define DW_TEST_BUZZER_ADR 			0x291C

#define DW_TEST_HEPSINIAC_ADR 		0x291D
#define DW_TEST_HEPSINIKAPAT_ADR 	0x291E

#define DW_TEST_1KHZ_ADR			0x291F
#define DW_TEST_2KHZ_ADR			0x2920
#define DW_TEST_3KHZ_ADR			0x2921
#define DW_TEST_4KHZ_ADR			0x2922

#define DW_TEST_OUT2_3V_ADR			0x2923
#define DW_TEST_OUT2_6V_ADR			0x2924
#define DW_TEST_OUT2_9V_ADR			0x2925

#define DW_TEST_OUT1_5V_ADR			0x2926
#define DW_TEST_OUT1_10V_ADR		0x2927
#define DW_TEST_OUT1_15V_ADR		0x2928

#define DW_TEST_EXIT_ADR			0x2929

/*--------------------------------- MODE INFO -------------------------------*/
#define REG_DW_MODE_INFO_ADR			0x2000

#define DW_ANA_SAYFA_ENTER				0
#define DW_MANUEL_MODE_ENTER			1
#define DW_RECETE_PISIRME_SAYFA_ENTER	3

#define DW_CIHAZ_TEST_SAYFA_ENTER		6

/*---------------------------- MANUEL SAYFASI ------------------------------*/

#define DW_MANUEL_MOD_GIRIS_ADR			0x2000

#define DW_ILK_KULLANILAN_ADR			DW_UST_SICAKLIK_SET_ADR

#define DW_UST_SICAKLIK_SET_ADR			0x1000

#define DW_PISIRME_SURESI_DK_ADR		0x1002
#define DW_PISIRME_SURESI_SN_ADR		0x1004

#define DW_UST_SICAKLIK_ADR				0x1006
#define DW_TMP112_ADR					0x1007
#define DW_PISIRME_START_ADR			0x1008
#define DW_CONVEYOR_START_ADR			0x100A
#define DW_CONVEYOR_YON_ADR				0x100B
#define DW_CONVEYOR_SOL_ANIM_ADR		0x100C
#define DW_CONVEYOR_SAG_ANIM_ADR		0x100E


#define DW_CONVEYOR_SAG	2
#define DW_CONVEYOR_SOL	1

/*------------------------------- ANIMSAYON ADDRESS  --------------------------------*/
#define DW_SICAKLIK_ANIM_ADR			0x1010

/*---------------------------- BLUETOOTH SAYFASI	 ------------------------------*/

#define DW_BLE_NAME_START_ADR			0x17C4
#define DW_BLE_PSW_START_ADR			0x17DE
#define DW_QR_CODE_ADR					0x75BB
#define DW_BLE_IKON_ADR					0x17D0


/*---------------------------- ORTAK AYARLAMA SAYFASI	 ------------------------------*/


#define DW_UST_SICAKLIK_SET_ONAY_ADR	0x1569
#define DW_PISIRME_SURESI_DK_ONAY_ADR	0x156A
#define DW_PISIRME_SURESI_SN_ONAY_ADR	0x156B


#define DW_UST_SICAKLIK_SET_ORT_ADR		0x155B
#define DW_PISIRME_SURESI_SET_DK_ADR	0x155D
#define DW_PISIRME_SURESI_SET_SN_ADR	0x155F




/*---------------------------- PARAMETRELER SAYFASI	 ------------------------------*/

#define BRULOR_RESET_RELAY_DELAY_MS 	2000

#define DW_PARAMETRE_PAGE_ADR			0x17A8
#define DW_PARAMETRE_EXIT_PAGE_ADR      0x17FA

#define DW_FW_VERSION_ADR				0x17E0
#define APP_FIRIN_MODEL_ADR				0x2320
#define DW_PARAMETRE_MK_PSW				9128

#define DW_BUTTON_SOUND_ADR				0x17A9
#define DW_ALARM_PARAM_ADR				0x17AB
#define DW_DIL_PARAM_ADR				0x17AD
#define DW_PSW_PARAM_ADR				0x17AF
#define DW_FIRIN_TYPE_ADR				0x17B1
#define DW_SICAKLIK_MAX_SET_PARAM_ADR	0x17B3
#define DW_BRULOR_RST_PARAM_ADR			0x17B5
#define DW_MOTOR_FREQ_ADR				0x17B7
#define DW_LOGO_PARAM_ADR				0x17B9
#define DW_TERMOKUPL_TYPE_PARAM_ADR		0x17BB

#define DW_FARBRIKA_AYAR_PARAM_ADR		0x17BF

#define DW_FIRIN_GUC_TYPE_PARAM_ADR		0x1803

#define DW_K_TYPE_TERMOKUP_VAL			1
#define DW_J_TYPE_TERMOKUP_VAL			0

#define DW_ELEKTRIKLI_FIRIN_TYPE_VAL 	0
#define DW_LPG_FIRIN_TYPE_VAL 			1
#define DW_DOGALGAZ_AO_FIRIN_TYPE_VAL 	2
#define DW_DOGALGAZ_DO_FIRIN_TYPE_VAL 	3


/*---------------------------- ARIZA ALARM SAYFASI	 ------------------------------*/

#define DW_TC1_ARIZA_ADR					0x178E
#define DW_TC2_ARIZA_ADR					0x178F
#define DW_TC3_ARIZA_ADR					0x1790
#define DW_MCP9700_ARIZA_ADR				0x1791
#define DW_EEPROM_ARIZA_ADR					0x1792
#define DW_ASIRI_SICAKLIK_ARIZA_ADR			0x1793
#define DW_BRULOR_ARIZA_ADR					0x1794
#define DW_ARIZA_ALARM_SUSTURMA_ADR			0x1795
#define DW_BRULOR_RESET_ICON_ADR			0x1796
#define DW_MOTOR_ASIRI_SICAKLIK_ARIZA_ADR	0x1797

/*--------------------------------- RECETELER -----------------------------------*/

#define DW_RECETE_SAYFA_ENTER_ADR		0x2800
#define DW_RECETE_CIKIS_CMD				0x107F
#define DW_RECETE_AMOUNT				100
#define DW_RECETE_ISIM_SIZE				10
#define DW_RECETE_RESIM_ILK_ADR			0x10FB
#define DW_RECETE_ISIM_ILK_ADR			0x115F

#define DW_RECETE_ILK_ADR				0x1097
#define DW_RECETE_SON_ADR				0x10FA

#define DW_RECETE_DUZ_RESIM_ADR			0x1071
#define DW_RECETE_DUZ_NUM_ADR			0x1072
#define DW_RECETE_DUZ_ISIM_ADR			0x1073

#define DW_RECETE_DUZ_UST_SIC_SET_ADR	0x1039

#define DW_RECETE_PISIR_UST_SIC_SET_ADR	0x1000


#define DW_RECETE_PISIRME_CMD			1
#define DW_RECETE_DUZENLEME_CMD			2


/*--------------------------------- TARIH/SAAT -----------------------------------*/
#define DW_TARIH_SAAT_PAGE_ENTER_ADR	0x2500
#define DW_FIRST_WRITE_RTC_ADR			0x009C
#define DW_WRITE_RTC_DONE_MSG			0x5AA5
#define DW_FIRST_READ_RTC_ADR			0x0010

/*---------------------------- DIL ------------------------------*/

#define DW_DIL_SABIT_YAZI_ADR			0x843A

#define DW_DIL_TURKCE_VAL 				0
#define DW_DIL_INGILIZCE_VAL 			1
#define DW_DIL_RUSCA_VAL 				2
#define DW_DIL_ALMANCA_VAL 				3


#endif /* INC_DWIN_ADRESS_H_ */
