################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../code/app_init.c \
../code/app_process.c \
../code/ble_bass.c \
../code/ble_custom.c \
../code/ble_diss.c \
../code/ble_std.c \
../code/uart.c 

OBJS += \
./code/app_init.o \
./code/app_process.o \
./code/ble_bass.o \
./code/ble_custom.o \
./code/ble_diss.o \
./code/ble_std.o \
./code/uart.o 

C_DEPS += \
./code/app_init.d \
./code/app_process.d \
./code/ble_bass.d \
./code/ble_custom.d \
./code/ble_diss.d \
./code/ble_std.d \
./code/uart.d 


# Each subdirectory must supply rules for building sources it contributes
code/%.o: ../code/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g -DRSL10_CID=101 -DCFG_BLE=1 -DCFG_ALLROLES=1 -DCFG_APP -DCFG_APP_BATT -DCFG_ATTS=1 -DCFG_CON=8 -DCFG_EMB=1 -DCFG_HOST=1 -DCFG_RF_ATLAS=1 -DCFG_ALLPRF=1 -DCFG_PRF=1 -DCFG_NB_PRF=8 -DCFG_CHNL_ASSESS=1 -DCFG_SEC_CON=1 -DCFG_EXT_DB -DCFG_PRF_BASS=1 -DCFG_PRF_DISS=1 -D_RTE_ -I"C:\A_Work\RFID\eclipse-workspace\peripheral_server_uart\include" -I"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include" -I"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/bb" -I"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/ble" -I"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/ble/profiles" -I"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/kernel" -I"C:\A_Work\RFID\eclipse-workspace\peripheral_server_uart/RTE" -I"C:\A_Work\RFID\eclipse-workspace\peripheral_server_uart/RTE/Device/RSL10" -isystem"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include" -isystem"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/bb" -isystem"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/ble" -isystem"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/ble/profiles" -isystem"C:/Users/h274519/ON_Semiconductor/ONSemiconductor/RSL10/3.0.534/include/kernel" -isystem"C:\A_Work\RFID\eclipse-workspace\peripheral_server_uart/RTE" -isystem"C:\A_Work\RFID\eclipse-workspace\peripheral_server_uart/RTE/Device/RSL10" -std=gnu11 -Wmissing-prototypes -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

