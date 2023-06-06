################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include X-CUBE-AI/App/subdir.mk
-include Drivers/STM32L4xx_HAL_Driver/Src/subdir.mk
-include Drivers/Components/wm8994/subdir.mk
-include Drivers/Components/stmpe811/subdir.mk
-include Drivers/Components/stmpe1600/subdir.mk
-include Drivers/Components/st7789h2/subdir.mk
-include Drivers/Components/st7735/subdir.mk
-include Drivers/Components/st25dv/subdir.mk
-include Drivers/Components/ov9655/subdir.mk
-include Drivers/Components/mfxstm32l152/subdir.mk
-include Drivers/Components/m24sr/subdir.mk
-include Drivers/Components/lsm6dsl/subdir.mk
-include Drivers/Components/lsm303dlhc/subdir.mk
-include Drivers/Components/lsm303c/subdir.mk
-include Drivers/Components/ls016b8uy/subdir.mk
-include Drivers/Components/lps22hb/subdir.mk
-include Drivers/Components/lis3mdl/subdir.mk
-include Drivers/Components/l3gd20/subdir.mk
-include Drivers/Components/hx8347i/subdir.mk
-include Drivers/Components/hx8347g/subdir.mk
-include Drivers/Components/hts221/subdir.mk
-include Drivers/Components/ft6x06/subdir.mk
-include Drivers/Components/ft5336/subdir.mk
-include Drivers/Components/ft3x67/subdir.mk
-include Drivers/Components/cy8c4014lqi/subdir.mk
-include Drivers/Components/cs43l22/subdir.mk
-include Drivers/Components/cs42l51/subdir.mk
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


BUILD_ARTIFACT_NAME := Final Project G2
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
Final\ Project\ G2.elf \

MAP_FILES += \
Final\ Project\ G2.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
Final\ Project\ G2.list \


# All Target
all: main-build

# Main-build Target
main-build: Final\ Project\ G2.elf secondary-outputs

# Tool invocations
Final\ Project\ G2.elf Final\ Project\ G2.map: $(OBJS) $(USER_OBJS) C:\Users\rsaif\STM32CubeIDE\workspace_1.10.1\Final\ Project\ G2\STM32L4S5VITX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "Final Project G2.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Users\rsaif\STM32CubeIDE\workspace_1.10.1\Final Project G2\STM32L4S5VITX_FLASH.ld" --specs=nosys.specs -Wl,-Map="Final Project G2.map" -Wl,--gc-sections -static -L../Middlewares/ST/AI/Lib -L../Drivers/CMSIS/Include/Lib/ -L../Drivers/CMSIS/Include/Lib --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

Final\ Project\ G2.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "Final Project G2.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) Final\ Project\ G2.elf Final\ Project\ G2.list Final\ Project\ G2.map Final\\ Project\\ G2.elf default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
