BIN     = ../../riscv-embedded-gcc/bin

OUTPUT_DIR = ./dist

TARGET     := $(addprefix $(OUTPUT_DIR)/, $(notdir $(CURDIR)))

Startup_ROOT	= ../../EVT/EXAM/SRC/Startup
Startup_SRC		= startup_CH583.S
Startup_OBJ 	= $(addprefix $(OUTPUT_DIR)/Startup/, $(Startup_SRC:.S=.o))

RVMSIS_ROOT	= ../../EVT/EXAM/SRC/RVMSIS
RVMSIS_SRC 	= core_riscv.c
RVMSIS_OBJ 	= $(addprefix $(OUTPUT_DIR)/RVMSIS/, $(RVMSIS_SRC:.c=.o))

StdPeriphDriver_ROOT = ../../EVT/EXAM/SRC/StdPeriphDriver
StdPeriphDriver_INC = $(StdPeriphDriver_ROOT)/inc
StdPeriphDriver_SRC = $(notdir $(wildcard $(StdPeriphDriver_ROOT)/*.c))
StdPeriphDriver_OBJ = $(addprefix $(OUTPUT_DIR)/StdPeriphDriver/, $(StdPeriphDriver_SRC:.c=.o))

HAL_ROOT	=	../../EVT/EXAM/BLE/HAL
HAL_INC	 	= $(HAL_ROOT)/include
HAL_SRC 	= MCU.c RTC.c SLEEP.c
HAL_OBJ 	= $(addprefix $(OUTPUT_DIR)/HAL/, $(HAL_SRC:.c=.o))

Profile_ROOT	= ./Profile
Profile_INC		= $(Profile_ROOT)/include
Profile_SRC		=	$(notdir $(wildcard $(Profile_ROOT)/*.c))
Profile_OBJ = $(addprefix $(OUTPUT_DIR)/Profile/, $(Profile_SRC:.c=.o))

APP_ROOT	= ./APP
APP_INC		= $(APP_ROOT)/include
APP_SRC		=	$(notdir $(wildcard $(APP_ROOT)/*.c))
APP_OBJ = $(addprefix $(OUTPUT_DIR)/APP/, $(APP_SRC:.c=.o))

VPATH = $(Startup_ROOT):$(RVMSIS_ROOT):$(StdPeriphDriver_ROOT):$(HAL_ROOT):$(Profile_ROOT):$(APP_ROOT)

LIBS := -lISP583 -lCH58xBLE

$(OUTPUT_DIR)/Startup/%.o: %.S
	$(BIN)/riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/RVMSIS/%.o: %.c
	$(BIN)/riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DDEBUG=1 \
		-I $(RVMSIS_ROOT) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/StdPeriphDriver/%.o: %.c
	$(BIN)/riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DDEBUG=1 \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/HAL/%.o: %.c
	$(BIN)/riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DDEBUG=1 \
		-I ../../EVT/EXAM/BLE/LIB \
		-I $(HAL_INC) \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/Profile/%.o: %.c
	$(BIN)/riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DDEBUG=1 \
		-I ../../EVT/EXAM/BLE/LIB \
		-I $(HAL_INC) \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC) \
		-I $(Profile_INC) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/APP/%.o: %.c
	$(BIN)/riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -DDEBUG=1 \
		-I ../../EVT/EXAM/BLE/LIB \
		-I $(HAL_INC) \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC) \
		-I $(Profile_INC) \
		-I $(APP_INC) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

all: $(OUTPUT_DIR) $(TARGET).hex $(TARGET).siz $(TARGET).lst

$(TARGET).elf: $(Startup_OBJ) $(RVMSIS_OBJ) $(StdPeriphDriver_OBJ) $(HAL_OBJ) $(Profile_OBJ) $(APP_OBJ)
	$(BIN)/riscv-none-embed-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g \
		-T "../../EVT/EXAM/SRC/Ld/Link.ld" \
		-nostartfiles -Xlinker --gc-sections \
		-L"../../EVT/EXAM/BLE/LIB" \
		-L"$(StdPeriphDriver_ROOT)" \
		-Xlinker --print-memory-usage -Wl,-Map,"$(OUTPUT_DIR)/project.map" --specs=nano.specs --specs=nosys.specs -o "$@" $^ $(LIBS)

$(TARGET).hex: $(TARGET).elf
	$(BIN)/riscv-none-embed-objcopy -O ihex "$(TARGET).elf" "$(TARGET).hex"

$(TARGET).siz: $(TARGET).elf
	$(BIN)/riscv-none-embed-size --format=berkeley "$(TARGET).elf"

$(TARGET).lst: $(TARGET).elf
	$(BIN)/riscv-none-embed-objdump --source --all-headers --demangle --line-numbers --wide $^ > $@

flash: $(TARGET).hex
	sudo ../../isp/WCHISPTool_CMD -p /dev/ch37x0 -c ../../isp/config.ini -o program -f $<


$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/Startup
	mkdir -p $(OUTPUT_DIR)/RVMSIS
	mkdir -p $(OUTPUT_DIR)/StdPeriphDriver
	mkdir -p $(OUTPUT_DIR)/HAL
	mkdir -p $(OUTPUT_DIR)/Profile
	mkdir -p $(OUTPUT_DIR)/APP

clean:
	rm -rf $(OUTPUT_DIR)

.PHONY: clean flash
