################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Lisa.axf

# Tool invocations
Lisa.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"D:\Career\4 - MS\Sem 3 - Fall 2016\CMPE 245\Codes\Workspace\CMSIS_CORE_LPC17xx\Debug" -L"D:\Career\4 - MS\Sem 3 - Fall 2016\CMPE 245\Codes\Workspace\CMSIS_DSPLIB_CM3\lib" -Xlinker -Map="Lisa.map" -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m3 -mthumb -T "Lisa_Debug.ld" -o "Lisa.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) Lisa.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "Lisa.axf"; # arm-none-eabi-objcopy -v -O binary "Lisa.axf" "Lisa.bin" ; # checksum -p LPC1769 -d "Lisa.bin";
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY: post-build

-include ../makefile.targets
