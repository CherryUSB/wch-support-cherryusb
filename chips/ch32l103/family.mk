# --- Toolchain ---
PREFIX  := riscv32-wch-elf
CC      := $(PREFIX)-gcc
AR      := $(PREFIX)-ar
OBJCOPY := $(PREFIX)-objcopy
OBJDUMP := $(PREFIX)-objdump
SIZE    := $(PREFIX)-size

# --- Include Directories ---
INCLUDES += \
	$(FAMILY_DIR)/sdk/Core \
	$(FAMILY_DIR)/sdk/Debug \
	$(FAMILY_DIR)/sdk/Peripheral/inc \
	$(FAMILY_DIR)/board \
	$(FAMILY_DIR)/libcpu \

# --- Assembly Source Directories ---
ASM_DIR += \
	$(FAMILY_DIR)/sdk/Startup \
	$(FAMILY_DIR)/libcpu \

# --- C Source Directories ---
SRC_DIR += \
	$(FAMILY_DIR)/sdk/Core \
	$(FAMILY_DIR)/sdk/Debug \
	$(FAMILY_DIR)/sdk/Peripheral/src \
	$(FAMILY_DIR)/board \
	$(FAMILY_DIR)/libcpu \

# --- Library Directories ---
LIB_DIR +=

# --- Assembly Source Files ---
ASMS += \
	startup_ch32l103.S \
	context_gcc.S \
	interrupt_gcc.S \

# --- C Source Files ---
SRCS += $(foreach dir,$(SRC_DIR),$(wildcard $(dir)/*.c))

# --- Libraries ---
LIBS += -lm

# --- Compiler Flags ---
CFLAGS += \
	-march=rv32imac_xw \
	-mabi=ilp32 \
	-msmall-data-limit=8 \
	-msave-restore \
	-Os \
	-fmessage-length=0 \
	-fsigned-char \
	-ffunction-sections \
	-fdata-sections \
	-fno-common \
	-Wunused \
	-Wuninitialized \
	-g \
	-gdwarf-4 \
	-std=gnu11 \
	$(addprefix -I,$(INCLUDES)) \
	$(addprefix -L,$(LIB_DIR)) \

# --- Linker Flags ---
LDFLAGS += \
	$(CFLAGS) \
	$(LIBS) \
	-nostartfiles \
	-Xlinker \
	--gc-sections \
	-Wl,--print-memory-usage \
	-Wl,-Map,$(MAP_FILE) \
	--specs=nano.specs \
	--specs=nosys.specs \
	-T "$(FAMILY_DIR)/sdk/Ld/Link.ld" \
