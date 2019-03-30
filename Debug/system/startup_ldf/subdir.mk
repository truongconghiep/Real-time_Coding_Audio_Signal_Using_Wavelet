################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
LDF_SRCS += \
../system/startup_ldf/app.ldf 

S_SRCS += \
../system/startup_ldf/app_startup.s 

C_SRCS += \
../system/startup_ldf/app_cplbtab.c \
../system/startup_ldf/app_handler_table.c \
../system/startup_ldf/app_heaptab.c 

SRC_OBJS += \
./system/startup_ldf/app_cplbtab.doj \
./system/startup_ldf/app_handler_table.doj \
./system/startup_ldf/app_heaptab.doj \
./system/startup_ldf/app_startup.doj 

S_DEPS += \
./system/startup_ldf/app_startup.d 

C_DEPS += \
./system/startup_ldf/app_cplbtab.d \
./system/startup_ldf/app_handler_table.d \
./system/startup_ldf/app_heaptab.d 


# Each subdirectory must supply rules for building sources it contributes
system/startup_ldf/app_cplbtab.doj: ../system/startup_ldf/app_cplbtab.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn -c -file-attr ProjectName="Wavelet_Codec_Block_Based_20_Samples_Packetizing_32bit" -proc ADSP-BF706 -flags-compiler --no_wrap_diagnostics -si-revision 1.1 -g -always-inline -D_DEBUG -DCORE0 @includes-08dae26ac8fb5ee75da9719eafb2ecc9.txt -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -D__PROCESSOR_SPEED__=400000000 -double-size-32 -decls-strong -cplbs -gnu-style-dependencies -MD -Mo "system/startup_ldf/app_cplbtab.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

system/startup_ldf/app_handler_table.doj: ../system/startup_ldf/app_handler_table.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn -c -file-attr ProjectName="Wavelet_Codec_Block_Based_20_Samples_Packetizing_32bit" -proc ADSP-BF706 -flags-compiler --no_wrap_diagnostics -si-revision 1.1 -g -always-inline -D_DEBUG -DCORE0 @includes-08dae26ac8fb5ee75da9719eafb2ecc9.txt -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -D__PROCESSOR_SPEED__=400000000 -double-size-32 -decls-strong -cplbs -gnu-style-dependencies -MD -Mo "system/startup_ldf/app_handler_table.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

system/startup_ldf/app_heaptab.doj: ../system/startup_ldf/app_heaptab.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn -c -file-attr ProjectName="Wavelet_Codec_Block_Based_20_Samples_Packetizing_32bit" -proc ADSP-BF706 -flags-compiler --no_wrap_diagnostics -si-revision 1.1 -g -always-inline -D_DEBUG -DCORE0 @includes-08dae26ac8fb5ee75da9719eafb2ecc9.txt -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -D__PROCESSOR_SPEED__=400000000 -double-size-32 -decls-strong -cplbs -gnu-style-dependencies -MD -Mo "system/startup_ldf/app_heaptab.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

system/startup_ldf/app_startup.doj: ../system/startup_ldf/app_startup.s
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin Assembler'
	easmblkfn -file-attr ProjectName="Wavelet_Codec_Block_Based_20_Samples_Packetizing_32bit" -proc ADSP-BF706 -si-revision 1.1 -g -D_DEBUG -DCORE0 @includes-6ba5347410d25f447e6881137e504628.txt -gnu-style-dependencies -MM -Mo "system/startup_ldf/app_startup.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


