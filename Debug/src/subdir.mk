################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Functions_32bit.c \
../src/WaveletCodecBlockBased.c \
../src/audio.c 

SRC_OBJS += \
./src/Functions_32bit.doj \
./src/WaveletCodecBlockBased.doj \
./src/audio.doj 

C_DEPS += \
./src/Functions_32bit.d \
./src/WaveletCodecBlockBased.d \
./src/audio.d 


# Each subdirectory must supply rules for building sources it contributes
src/Functions_32bit.doj: ../src/Functions_32bit.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn -c -file-attr ProjectName="Wavelet_Codec_Block_Based_20_Samples_Packetizing_32bit" -proc ADSP-BF706 -flags-compiler --no_wrap_diagnostics -si-revision 1.1 -g -always-inline -D_DEBUG -DCORE0 @includes-08dae26ac8fb5ee75da9719eafb2ecc9.txt -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -D__PROCESSOR_SPEED__=400000000 -double-size-32 -decls-strong -cplbs -gnu-style-dependencies -MD -Mo "src/Functions_32bit.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/WaveletCodecBlockBased.doj: ../src/WaveletCodecBlockBased.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn -c -file-attr ProjectName="Wavelet_Codec_Block_Based_20_Samples_Packetizing_32bit" -proc ADSP-BF706 -flags-compiler --no_wrap_diagnostics -si-revision 1.1 -g -always-inline -D_DEBUG -DCORE0 @includes-08dae26ac8fb5ee75da9719eafb2ecc9.txt -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -D__PROCESSOR_SPEED__=400000000 -double-size-32 -decls-strong -cplbs -gnu-style-dependencies -MD -Mo "src/WaveletCodecBlockBased.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/audio.doj: ../src/audio.c
	@echo 'Building file: $<'
	@echo 'Invoking: CrossCore Blackfin C/C++ Compiler'
	ccblkfn -c -file-attr ProjectName="Wavelet_Codec_Block_Based_20_Samples_Packetizing_32bit" -proc ADSP-BF706 -flags-compiler --no_wrap_diagnostics -si-revision 1.1 -g -always-inline -D_DEBUG -DCORE0 @includes-08dae26ac8fb5ee75da9719eafb2ecc9.txt -structs-do-not-overlap -no-const-strings -no-multiline -warn-protos -D__PROCESSOR_SPEED__=400000000 -double-size-32 -decls-strong -cplbs -gnu-style-dependencies -MD -Mo "src/audio.d" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


