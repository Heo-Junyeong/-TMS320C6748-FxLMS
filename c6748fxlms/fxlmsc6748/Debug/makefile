################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-c6000_8.3.11

GEN_OPTS__FLAG := 
GEN_CMDS__FLAG := 

ORDERED_OBJS += \
"./aic31.obj" \
"./codecif.obj" \
"./main.obj" \
"../C6748.cmd" \
$(GEN_CMDS__FLAG) \
-llibc.a \
-l"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/utils/Release/utils.lib" \
-l"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/c6748/drivers/Release/drivers.lib" \
-l"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/c6748/system_config/Release/system_config.lib" \
-l"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/c6748/lcdkC6748/platform/Release/platform.lib" \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(INO_DEPS)),)
-include $(INO_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(PDE_DEPS)),)
-include $(PDE_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
fxlmsc6748.out \

EXE_OUTPUTS__QUOTED += \
"fxlmsc6748.out" \

BIN_OUTPUTS += \
fxlmsc6748.hex \

BIN_OUTPUTS__QUOTED += \
"fxlmsc6748.hex" \


# All Target
all: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "fxlmsc6748.out"

# Tool invocations
fxlmsc6748.out: $(OBJS) $(CMD_SRCS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: C6000 Linker'
	"C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-c6000_8.3.11/bin/cl6x" -mv6740 --define=c6748 -g --diag_warning=225 --diag_wrap=off --display_error_number -z -m"fxlmsc6748.map" --heap_size=0x800 --stack_size=0x800 -i"C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-c6000_8.3.11/lib" -i"C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-c6000_8.3.11/include" -i"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/utils/Release" -i"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/grlib/Release" -i"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/nandlib/Release" -i"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/c6748/drivers/Release" -i"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/c6748/system_config/Release" -i"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/c6748/usblib/Release" -i"C:/ti/C6748_StarterWare_1_20_04_01/binary/c674x/cgt_ccs/c6748/lcdkC6748/platform/Release" --reread_libs --diag_wrap=off --display_error_number --warn_sections --xml_link_info="fxlmsc6748_linkInfo.xml" --rom_model -o "fxlmsc6748.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

fxlmsc6748.hex: $(EXE_OUTPUTS)
	@echo 'Building secondary target: "$@"'
	@echo 'Invoking: C6000 Hex Utility'
	"C:/ti/ccs1110/ccs/tools/compiler/ti-cgt-c6000_8.3.11/bin/hex6x" --diag_wrap=off -o "fxlmsc6748.hex" $(EXE_OUTPUTS__QUOTED)
	@echo 'Finished building secondary target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(BIN_OUTPUTS__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "aic31.obj" "codecif.obj" "main.obj" 
	-$(RM) "aic31.d" "codecif.d" "main.d" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

