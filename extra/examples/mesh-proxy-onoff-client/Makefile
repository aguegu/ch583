BIN     = ../../riscv-embedded-gcc/bin
BIN_PREFIX = riscv-none-embed

LINTER = clang-format --style=LLVM -i

OUTPUT_DIR = ./dist

DEFINES := -DBLE_MEMHEAP_SIZE=4096

TARGET     := $(addprefix $(OUTPUT_DIR)/, $(notdir $(CURDIR)))

Project_ROOT = ./src
Project_INC_DIR = ./inc
Project_SOURCES = $(notdir $(wildcard $(Project_ROOT)/*.c))
Project_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(Project_SOURCES:.c=.o))

Common_ROOT = ../common
Common_INC_DIR = $(Common_ROOT)/inc
Common_SRC_DIR = $(Common_ROOT)/src
Common_SOURCES = gpio.c ringbuffer.c
Common_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(Common_SOURCES:.c=.o))

BleMeshModels_ROOT = ../blemesh-models
BleMeshModels_INC_DIR = $(BleMeshModels_ROOT)/inc
BleMeshModels_SRC_DIR = $(BleMeshModels_ROOT)/src
BleMeshModels_SOURCES = generic_onoff_client_model.c
BleMeshModels_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(BleMeshModels_SOURCES:.c=.o))

HAL_ROOT	=	./HAL
HAL_INC_DIR	 	= $(HAL_ROOT)/inc
HAL_SOURCES 	= MCU.c RTC.c
HAL_OBJECTS 	= $(addprefix $(OUTPUT_DIR)/HAL/, $(HAL_SOURCES:.c=.o))

APP_ROOT	= ./APP
APP_INC_DIR		= $(APP_ROOT)/inc
APP_SOURCES		=	$(notdir $(wildcard $(APP_ROOT)/*.c))
APP_OBJECTS = $(addprefix $(OUTPUT_DIR)/APP/, $(APP_SOURCES:.c=.o))
APP_HEADERS = $(addprefix $(APP_INC_DIR)/, $(notdir $(wildcard $(APP_INC_DIR)/*.h)))

Startup_ROOT	= ../../EVT/EXAM/SRC/Startup
Startup_SOURCES		= startup_CH583.S
Startup_OBJECTS 	= $(addprefix $(OUTPUT_DIR)/Startup/, $(Startup_SOURCES:.S=.o))

RVMSIS_ROOT	= ../../EVT/EXAM/SRC/RVMSIS
RVMSIS_SOURCES 	= core_riscv.c
RVMSIS_OBJECTS 	= $(addprefix $(OUTPUT_DIR)/RVMSIS/, $(RVMSIS_SOURCES:.c=.o))

StdPeriphDriver_ROOT = ../../EVT/EXAM/SRC/StdPeriphDriver
StdPeriphDriver_INC_DIR = $(StdPeriphDriver_ROOT)/inc
StdPeriphDriver_SOURCES = CH58x_sys.c CH58x_clk.c CH58x_gpio.c CH58x_adc.c CH58x_uart1.c
StdPeriphDriver_OBJECTS = $(addprefix $(OUTPUT_DIR)/StdPeriphDriver/, $(StdPeriphDriver_SOURCES:.c=.o))

VPATH = $(Startup_ROOT):$(RVMSIS_ROOT):$(StdPeriphDriver_ROOT):$(HAL_ROOT):$(Profile_ROOT):$(APP_ROOT):$(Project_ROOT):$(Common_SRC_DIR):$(BleMeshModels_SRC_DIR)

LIBS := -lISP583 -lCH58xBLE -lMESH

$(OUTPUT_DIR)/Startup/%.o: %.S
	$(BIN)/$(BIN_PREFIX)-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/RVMSIS/%.o: %.c
	$(BIN)/$(BIN_PREFIX)-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g $(DEFINES) \
		-I $(RVMSIS_ROOT) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/StdPeriphDriver/%.o: %.c
	$(BIN)/$(BIN_PREFIX)-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g $(DEFINES) \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC_DIR) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/HAL/%.o: %.c
	$(BIN)/$(BIN_PREFIX)-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g $(DEFINES) \
		-I ../../EVT/EXAM/BLE/LIB \
		-I $(HAL_INC_DIR) \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC_DIR) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/APP/%.o: %.c
	$(BIN)/$(BIN_PREFIX)-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g $(DEFINES) \
		-I ../../EVT/EXAM/BLE/LIB \
		-I ../../EVT/EXAM/BLE/MESH/MESH_LIB \
		-I $(HAL_INC_DIR) \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC_DIR) \
		-I $(APP_INC_DIR) \
		-I $(BleMeshModels_INC_DIR) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/%.o: %.c
	$(BIN)/$(BIN_PREFIX)-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g $(DEFINES) \
		-I ../../EVT/EXAM/BLE/LIB \
		-I ../../EVT/EXAM/BLE/MESH/MESH_LIB \
		-I $(RVMSIS_ROOT) \
		-I $(StdPeriphDriver_INC_DIR) \
		-I $(HAL_INC_DIR) \
		-I $(APP_INC_DIR) \
		-I $(Project_INC_DIR) \
		-I $(Common_INC_DIR) \
		-I $(BleMeshModels_INC_DIR) \
		-std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

all: $(OUTPUT_DIR) $(TARGET).hex $(TARGET).siz $(TARGET).lst

$(TARGET).elf: $(Startup_OBJECTS) $(RVMSIS_OBJECTS) $(StdPeriphDriver_OBJECTS) $(HAL_OBJECTS) $(Profile_OBJ) $(APP_OBJECTS) $(Project_OBJECTS) $(Common_OBJECTS) $(BleMeshModels_OBJECTS)
	$(BIN)/$(BIN_PREFIX)-gcc -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common  -g \
		-T "../../EVT/EXAM/SRC/Ld/Link.ld" \
		-nostartfiles -Xlinker --gc-sections \
		-L "../../EVT/EXAM/BLE/LIB" \
		-L "../../EVT/EXAM/BLE/MESH/MESH_LIB" \
		-L "$(StdPeriphDriver_ROOT)" \
		-Xlinker --print-memory-usage -Wl,-Map,"$(OUTPUT_DIR)/project.map" --specs=nano.specs --specs=nosys.specs -o "$@" $^ $(LIBS)

$(TARGET).hex: $(OUTPUT_DIR) $(TARGET).elf
	$(BIN)/$(BIN_PREFIX)-objcopy -O ihex "$(TARGET).elf" "$(TARGET).hex"

$(TARGET).siz: $(TARGET).elf
	$(BIN)/$(BIN_PREFIX)-size --format=berkeley "$(TARGET).elf"

$(TARGET).lst: $(TARGET).elf
	$(BIN)/$(BIN_PREFIX)-objdump --source --all-headers --demangle --line-numbers --wide $^ > $@

flash: $(TARGET).hex
	sudo ../../isp/WCHISPTool_CMD -p /dev/ch37x0 -c ./isp-config.ini -o program -f $<

lint: $(APP_HEADERS) $(APP_SOURCES) $(Project_SOURCES)
	$(foreach f, $^, $(LINTER) $(f);)

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/Startup
	mkdir -p $(OUTPUT_DIR)/RVMSIS
	mkdir -p $(OUTPUT_DIR)/StdPeriphDriver
	mkdir -p $(OUTPUT_DIR)/HAL
	mkdir -p $(OUTPUT_DIR)/APP

clean:
	rm -rf $(OUTPUT_DIR)

.PHONY: clean flash lint
