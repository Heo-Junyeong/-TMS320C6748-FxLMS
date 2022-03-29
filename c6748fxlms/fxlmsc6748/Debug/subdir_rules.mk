################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C6000 Compiler'
	"C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-c6000_8.3.11/bin/cl6x" -mv6740 --include_path="C:/Users/jyheo/Desktop/[project]/2022RMS/c6748fxlms/fxlmsc6748" --include_path="C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-c6000_8.3.11/include" --include_path="C:/ti/C6748_StarterWare_1_20_04_01/include" --include_path="C:/ti/C6748_StarterWare_1_20_04_01/include/hw" --include_path="C:/ti/C6748_StarterWare_1_20_04_01/include/c674x" --include_path="C:/ti/C6748_StarterWare_1_20_04_01/include/c674x/c6748" --define=c6748 -g --diag_warning=225 --diag_wrap=off --display_error_number --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


