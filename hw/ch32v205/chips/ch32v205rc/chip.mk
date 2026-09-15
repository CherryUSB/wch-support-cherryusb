# --- Include Directories ---
INCLUDES += \
	$(CHIP_DIR)/board \
	CherryUSB/port/wch/usbhs \

# --- Assembly Source Directories ---
ASM_DIR +=

# --- C Source Directories ---
SRC_DIR += \
	$(CHIP_DIR)/board \
	CherryUSB/port/wch/usbhs \

# --- Library Directories ---
LIB_DIR +=

# --- Assembly Source Files ---
ASMS +=

# --- C Source Files ---
SRCS +=

# --- Libraries ---
LIBS +=

# --- Compiler Flags ---
CFLAGS += \
	-DUSBD_REG_BASE0=0x40023400 \
	-DUSBH_REG_BASE0=0x40023500 \

# --- Linker Flags ---
LDFLAGS += -T "$(CHIP_DIR)/linker_script/Link.ld"
