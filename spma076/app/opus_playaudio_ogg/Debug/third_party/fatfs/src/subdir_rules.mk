################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
third_party/fatfs/src/ff.obj: D:/ti/TivaWare_C_Series-2.1.2.111/third_party/fatfs/src/ff.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"D:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.6/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 --include_path="D:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.6/include" --include_path="D:/ti/TivaWare_C_Series-2.1.2.111" --include_path="D:/ti/TivaWare_C_Series-2.1.2.111/third_party" --include_path="D:/ti/TivaWare_C_Series-2.1.2.111/examples/boards/dk-tm4c129x" --include_path="D:/ti/examples/OPUS/spma076" --include_path="D:/ti/examples/OPUS/opus-1.1.2/include" --include_path="D:/ti/examples/OPUS/opus-1.1.2/silk" --include_path="D:/ti/examples/OPUS/opus-1.1.2/src" --include_path="D:/ti/examples/OPUS/opus-1.1.2/celt" -g --gcc --define=ccs="ccs" --define=PART_TM4C129XNCZAD --define=TARGET_IS_TM4C129_RA0 --define=VAR_ARRAYS --define=FIXED_POINT --define=OPUS_BUILD --diag_warning=225 --display_error_number --diag_wrap=off --preproc_with_compile --preproc_dependency="third_party/fatfs/src/ff.pp" --obj_directory="third_party/fatfs/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


