################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Bluetooth_Process.c \
../Core/Src/DWIN_Process.c \
../Core/Src/EEPROM_Process.c \
../Core/Src/Flash_Process.c \
../Core/Src/InOut_Process.c \
../Core/Src/SEGGER_RTT.c \
../Core/Src/SEGGER_RTT_Syscalls_GCC.c \
../Core/Src/SEGGER_RTT_printf.c \
../Core/Src/TMP112.c \
../Core/Src/Temperature_Process.c \
../Core/Src/USART_Process.c \
../Core/Src/adc.c \
../Core/Src/dac.c \
../Core/Src/dma.c \
../Core/Src/furnace_pid.c \
../Core/Src/gpio.c \
../Core/Src/hdc1080.c \
../Core/Src/i2c.c \
../Core/Src/main.c \
../Core/Src/pid.c \
../Core/Src/rtc.c \
../Core/Src/sht40.c \
../Core/Src/spi.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c \
../Core/Src/tim.c \
../Core/Src/usart.c 

OBJS += \
./Core/Src/Bluetooth_Process.o \
./Core/Src/DWIN_Process.o \
./Core/Src/EEPROM_Process.o \
./Core/Src/Flash_Process.o \
./Core/Src/InOut_Process.o \
./Core/Src/SEGGER_RTT.o \
./Core/Src/SEGGER_RTT_Syscalls_GCC.o \
./Core/Src/SEGGER_RTT_printf.o \
./Core/Src/TMP112.o \
./Core/Src/Temperature_Process.o \
./Core/Src/USART_Process.o \
./Core/Src/adc.o \
./Core/Src/dac.o \
./Core/Src/dma.o \
./Core/Src/furnace_pid.o \
./Core/Src/gpio.o \
./Core/Src/hdc1080.o \
./Core/Src/i2c.o \
./Core/Src/main.o \
./Core/Src/pid.o \
./Core/Src/rtc.o \
./Core/Src/sht40.o \
./Core/Src/spi.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o \
./Core/Src/tim.o \
./Core/Src/usart.o 

C_DEPS += \
./Core/Src/Bluetooth_Process.d \
./Core/Src/DWIN_Process.d \
./Core/Src/EEPROM_Process.d \
./Core/Src/Flash_Process.d \
./Core/Src/InOut_Process.d \
./Core/Src/SEGGER_RTT.d \
./Core/Src/SEGGER_RTT_Syscalls_GCC.d \
./Core/Src/SEGGER_RTT_printf.d \
./Core/Src/TMP112.d \
./Core/Src/Temperature_Process.d \
./Core/Src/USART_Process.d \
./Core/Src/adc.d \
./Core/Src/dac.d \
./Core/Src/dma.d \
./Core/Src/furnace_pid.d \
./Core/Src/gpio.d \
./Core/Src/hdc1080.d \
./Core/Src/i2c.d \
./Core/Src/main.d \
./Core/Src/pid.d \
./Core/Src/rtc.d \
./Core/Src/sht40.d \
./Core/Src/spi.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d \
./Core/Src/tim.d \
./Core/Src/usart.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F105xC -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Bluetooth_Process.cyclo ./Core/Src/Bluetooth_Process.d ./Core/Src/Bluetooth_Process.o ./Core/Src/Bluetooth_Process.su ./Core/Src/DWIN_Process.cyclo ./Core/Src/DWIN_Process.d ./Core/Src/DWIN_Process.o ./Core/Src/DWIN_Process.su ./Core/Src/EEPROM_Process.cyclo ./Core/Src/EEPROM_Process.d ./Core/Src/EEPROM_Process.o ./Core/Src/EEPROM_Process.su ./Core/Src/Flash_Process.cyclo ./Core/Src/Flash_Process.d ./Core/Src/Flash_Process.o ./Core/Src/Flash_Process.su ./Core/Src/InOut_Process.cyclo ./Core/Src/InOut_Process.d ./Core/Src/InOut_Process.o ./Core/Src/InOut_Process.su ./Core/Src/SEGGER_RTT.cyclo ./Core/Src/SEGGER_RTT.d ./Core/Src/SEGGER_RTT.o ./Core/Src/SEGGER_RTT.su ./Core/Src/SEGGER_RTT_Syscalls_GCC.cyclo ./Core/Src/SEGGER_RTT_Syscalls_GCC.d ./Core/Src/SEGGER_RTT_Syscalls_GCC.o ./Core/Src/SEGGER_RTT_Syscalls_GCC.su ./Core/Src/SEGGER_RTT_printf.cyclo ./Core/Src/SEGGER_RTT_printf.d ./Core/Src/SEGGER_RTT_printf.o ./Core/Src/SEGGER_RTT_printf.su ./Core/Src/TMP112.cyclo ./Core/Src/TMP112.d ./Core/Src/TMP112.o ./Core/Src/TMP112.su ./Core/Src/Temperature_Process.cyclo ./Core/Src/Temperature_Process.d ./Core/Src/Temperature_Process.o ./Core/Src/Temperature_Process.su ./Core/Src/USART_Process.cyclo ./Core/Src/USART_Process.d ./Core/Src/USART_Process.o ./Core/Src/USART_Process.su ./Core/Src/adc.cyclo ./Core/Src/adc.d ./Core/Src/adc.o ./Core/Src/adc.su ./Core/Src/dac.cyclo ./Core/Src/dac.d ./Core/Src/dac.o ./Core/Src/dac.su ./Core/Src/dma.cyclo ./Core/Src/dma.d ./Core/Src/dma.o ./Core/Src/dma.su ./Core/Src/furnace_pid.cyclo ./Core/Src/furnace_pid.d ./Core/Src/furnace_pid.o ./Core/Src/furnace_pid.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/hdc1080.cyclo ./Core/Src/hdc1080.d ./Core/Src/hdc1080.o ./Core/Src/hdc1080.su ./Core/Src/i2c.cyclo ./Core/Src/i2c.d ./Core/Src/i2c.o ./Core/Src/i2c.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/pid.cyclo ./Core/Src/pid.d ./Core/Src/pid.o ./Core/Src/pid.su ./Core/Src/rtc.cyclo ./Core/Src/rtc.d ./Core/Src/rtc.o ./Core/Src/rtc.su ./Core/Src/sht40.cyclo ./Core/Src/sht40.d ./Core/Src/sht40.o ./Core/Src/sht40.su ./Core/Src/spi.cyclo ./Core/Src/spi.d ./Core/Src/spi.o ./Core/Src/spi.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su ./Core/Src/tim.cyclo ./Core/Src/tim.d ./Core/Src/tim.o ./Core/Src/tim.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su

.PHONY: clean-Core-2f-Src

