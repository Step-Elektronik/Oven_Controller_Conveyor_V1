/*
 * InOut_Process.h
 *
 *  Created on: Jan 21, 2025
 *      Author: Step
 */

#ifndef INC_INOUT_PROCESS_H_
#define INC_INOUT_PROCESS_H_

#include "main.h"

#define HIGH 	1
#define LOW		0

#define K17 	0x8
#define K16 	0x10
#define K15 	0x20
#define K14 	0x40
#define BUZZER 	0x80
#define K8 		0x200
#define K9 		0x400
#define K10 	0x800
#define K11 	0x1000
#define K12 	0x2000
#define K13 	0x4000
#define K18 	0x8000
#define K1 		0x20000//
#define K2 		0x40000
#define K3 		0x80000//
#define K4 		0x100000
#define K5 		0x200000//
#define K6 		0x400000//
#define K7 		0x800000

#define Q_SIRKULASYON_MOTORU 	K8
#define Q_SOGUTMA_FANI 			K9
#define Q_BRULOR_START			K10
#define Q_CONVEYOR_DIR 			K1
#define Q_CONVEYOR_EN 			K3
#define Q_BRULOR_RESET			K14



#define I_BRULOR_ARIZA			INPUT_AC_2


#define LATCH_PIN GPIO_PIN_2
#define LATCH_PORT GPIOD

#define OE_PIN	GPIO_PIN_7
#define OE_PORT GPIOA

#define RUN_LED GPIOC, GPIO_PIN_13
#define ESP32_EN GPIOC, GPIO_PIN_2

#define INPUT_6 GPIOB, GPIO_PIN_15
#define INPUT_5 GPIOB, GPIO_PIN_14
#define INPUT_4 GPIOB, GPIO_PIN_13
#define INPUT_3 GPIOB, GPIO_PIN_12
#define INPUT_2 GPIOB, GPIO_PIN_11
#define INPUT_1 GPIOB, GPIO_PIN_10

#define INPUT_AC_1 GPIOC, GPIO_PIN_6
#define INPUT_AC_2 GPIOC, GPIO_PIN_7

#define I_ASIRI_SICAKLIK 		INPUT_6
#define I_MOTOR_ASIRI_SICAKLIK 	INPUT_5

void setOut(uint32_t outputAddr, uint8_t status);
void resetBit(uint32_t *data, uint32_t bitMask);
void setBit(uint32_t *data, uint32_t bitMask);
void ShiftRegister_SendData(uint32_t data);
void setOutData(uint32_t outputAddr, uint8_t status);
void shiftRefresh(void);
void shiftDataRefresh(void);
uint8_t CheckBit(uint32_t bitAddr);
void I_AC_Check(void);


#endif /* INC_INOUT_PROCESS_H_ */
