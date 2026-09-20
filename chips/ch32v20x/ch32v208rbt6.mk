# --- Supported IPs ---
SUPPORT_IPS := usbfs

# --- Default IP ---
ifeq ($(IP),)
IP := usbfs
endif

# --- Assembly Source Files ---
ASMS += startup_ch32v20x_D8W.S

# --- Compiler Flags ---
CFLAGS += \
	-DCH32V20x_D8W \
	-DCONFIG_USB_MAX_BUS=1 \
	-DCONFIG_USBDEV_MAX_BUS=CONFIG_USB_MAX_BUS \
	-DCONFIG_USBHOST_MAX_BUS=CONFIG_USB_MAX_BUS \

ifeq ($(IP),usbfs)
CFLAGS += \
	-DUSBD_REG_BASE0=0x50000000 \
	-DUSBH_REG_BASE0=0x50000000
endif

# --- Linker Flags ---
LDFLAGS += -T "$(FAMILY_DIR)/sdk/Ld/Link_xb.ld"
