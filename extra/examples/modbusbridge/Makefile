BIN     = ../../riscv-embedded-gcc/bin
CC      = $(BIN)/riscv-none-embed-gcc
CFLAGS  = -march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8 -mno-save-restore -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -g
DEFINES =

OUTPUT_DIR = ./dist

TARGET     := $(addprefix $(OUTPUT_DIR)/, $(notdir $(CURDIR)))

Project_ROOT    = .
Project_INC_DIR = $(Project_ROOT)/inc
Project_SRC_DIR = $(Project_ROOT)/src
Project_SOURCES = $(notdir $(wildcard $(Project_SRC_DIR)/*.c))
Project_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(Project_SOURCES:.c=.o))

Common_ROOT = ../common
Common_INC_DIR = $(Common_ROOT)/inc
Common_SRC_DIR = $(Common_ROOT)/src
Common_SOURCES = gpio.c ringbuffer.c ssd1306spi.c at.c
Common_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(Common_SOURCES:.c=.o))

Startup_ROOT		= ../../EVT/EXAM/SRC/Startup
Startup_SOURCES = startup_CH583.S
Startup_OBJECTS = $(addprefix $(OUTPUT_DIR)/asm/, $(Startup_SOURCES:.S=.o))

RVMSIS_ROOT		 = ../../EVT/EXAM/SRC/RVMSIS
RVMSIS_SOURCES = core_riscv.c
RVMSIS_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(RVMSIS_SOURCES:.c=.o))

StdPeriphDriver_ROOT    = ../../EVT/EXAM/SRC/StdPeriphDriver
StdPeriphDriver_INC_DIR = $(StdPeriphDriver_ROOT)/inc
StdPeriphDriver_SRC_DIR = $(StdPeriphDriver_ROOT)
StdPeriphDriver_SOURCES = CH58x_uart0.c CH58x_uart1.c CH58x_sys.c CH58x_gpio.c
StdPeriphDriver_OBJECTS = $(addprefix $(OUTPUT_DIR)/, $(StdPeriphDriver_SOURCES:.c=.o))

LINK_FILE = ../../EVT/EXAM/SRC/Ld/Link.ld

INCLUDE = -I $(StdPeriphDriver_INC_DIR) -I $(RVMSIS_ROOT) -I $(Project_INC_DIR) -I $(Common_INC_DIR)

VPATH = $(StdPeriphDriver_SRC_DIR):$(RVMSIS_ROOT):$(Startup_ROOT):$(Project_SRC_DIR):$(Common_SRC_DIR)

LIBS = -lISP583 -lprintfloat

all: $(OUTPUT_DIR) $(TARGET).hex

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)/asm

version:
	$(CC) --version

clean:
	rm -rf $(OUTPUT_DIR)
	mkdir -p $(OUTPUT_DIR)/asm

$(OUTPUT_DIR)/asm/%.o: %.S
	$(CC) $(CFLAGS) -MMD -MP -x assembler-with-cpp -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(OUTPUT_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP $(INCLUDE) $(DEFINES) -std=gnu99 -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

$(TARGET).elf: $(StdPeriphDriver_OBJECTS) $(RVMSIS_OBJECTS) $(Startup_OBJECTS) $(Project_OBJECTS) $(Common_OBJECTS)
	$(CC) $(CFLAGS) \
	-T "$(LINK_FILE)" -nostartfiles -Xlinker --gc-sections \
	-L "$(StdPeriphDriver_ROOT)" \
	-Xlinker --print-memory-usage -Wl,-Map,"$(@:%.elf=%.map)" --specs=nano.specs --specs=nosys.specs -o $@ $^ $(LIBS)

$(TARGET).hex: $(TARGET).elf
	$(BIN)/riscv-none-embed-objcopy -O ihex $< $@

flash: $(TARGET).hex
	sudo ../../isp/WCHISPTool_CMD -p /dev/ch37x0 -c ./config.ini -o program -f $<

.PHONY: version clean flash
