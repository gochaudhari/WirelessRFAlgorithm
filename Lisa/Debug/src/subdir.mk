################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Common.c \
../src/Lisa.c \
../src/ReceiverSource.c \
../src/TransmitterSource.c \
../src/cr_startup_lpc175x_6x.c \
../src/crp.c 

OBJS += \
./src/Common.o \
./src/Lisa.o \
./src/ReceiverSource.o \
./src/TransmitterSource.o \
./src/cr_startup_lpc175x_6x.o \
./src/crp.o 

C_DEPS += \
./src/Common.d \
./src/Lisa.d \
./src/ReceiverSource.d \
./src/TransmitterSource.d \
./src/cr_startup_lpc175x_6x.d \
./src/crp.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_CMSIS=CMSIS_CORE_LPC17xx -D__USE_CMSIS_DSPLIB=CMSIS_DSPLIB_CM3 -D__LPC17XX__ -D__REDLIB__ -I"C:\Users\Akshay\Desktop\245\testworkspace\workspace\Lisa\inc" -I"C:\Users\Akshay\Desktop\245\testworkspace\workspace\CMSIS_CORE_LPC17xx\inc" -I"C:\Users\Akshay\Desktop\245\testworkspace\workspace\CMSIS_DSPLIB_CM3\inc" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


