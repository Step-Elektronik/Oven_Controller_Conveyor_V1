################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F1xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := Oven_Controller_Conveyor_V1
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Oven_Controller_Conveyor_V1.elf \

MAP_FILES += \
Oven_Controller_Conveyor_V1.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Oven_Controller_Conveyor_V1.list \

OBJCOPY_HEX += \
Oven_Controller_Conveyor_V1.hex \


# All Target
all:
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: Oven_Controller_Conveyor_V1.elf secondary-outputs

# Tool invocations
Oven_Controller_Conveyor_V1.elf Oven_Controller_Conveyor_V1.map: $(OBJS) $(USER_OBJS) C:\Users\Step\Documents\GitHub\Oven_Controller_Conveyor_V1\STM32F105RCTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Oven_Controller_Conveyor_V1.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m3 -T"C:\Users\Step\Documents\GitHub\Oven_Controller_Conveyor_V1\STM32F105RCTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Oven_Controller_Conveyor_V1.map" -Wl,--gc-sections -static --specs=nano.specs -mfloat-abi=soft -mthumb -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Oven_Controller_Conveyor_V1.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Oven_Controller_Conveyor_V1.list"
	@echo 'Finished building: $@'
	@echo ' '

Oven_Controller_Conveyor_V1.hex: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objcopy  -O ihex $(EXECUTABLES) "Oven_Controller_Conveyor_V1.hex"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Oven_Controller_Conveyor_V1.elf Oven_Controller_Conveyor_V1.hex Oven_Controller_Conveyor_V1.list Oven_Controller_Conveyor_V1.map default.size.stdout
	-@echo ' '

post-build:
	rm -f "Oven_Controller_Conveyor_V1"*.bin
	rm -f "../Oven_Controller_Conveyor_V1"*.bin
	rm -f "Oven_Controller_Conveyor_V1"*.hex
	rm -f "../Oven_Controller_Conveyor_V1"*.hex
	arm-none-eabi-objcopy -O binary "Oven_Controller_Conveyor_V1.elf" "Oven_Controller_Conveyor_V1.bin"
	arm-none-eabi-objcopy -O ihex "Oven_Controller_Conveyor_V1.elf" "Oven_Controller_Conveyor_V1.hex"
	cp "Oven_Controller_Conveyor_V1.bin" "../Oven_Controller_Conveyor_V1_$$(awk '/FW_VERSION/ {print $$3}' "C:/Users/Step/Documents/GitHub/Oven_Controller_Conveyor_V1/Core/Inc/version.h" | tr -d '\r\n').bin"
	cp "Oven_Controller_Conveyor_V1.hex" "../Oven_Controller_Conveyor_V1_$$(awk '/FW_VERSION/ {print $$3}' "C:/Users/Step/Documents/GitHub/Oven_Controller_Conveyor_V1/Core/Inc/version.h" | tr -d '\r\n').hex"
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_HEX)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified post-build

-include ../makefile.targets
