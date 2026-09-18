# --- Supported IPs ---
SUPPORT_IPS := usbfs

# --- Default IP ---
ifeq ($(IP),)
IP := usbfs
endif

# --- Compiler Flags ---
CFLAGS += \
	-DCONFIG_USB_MAX_BUS=1 \
	-DCONFIG_USBDEV_MAX_BUS=CONFIG_USB_MAX_BUS \
	-DCONFIG_USBHOST_MAX_BUS=CONFIG_USB_MAX_BUS \

ifeq ($(IP),usbfs)
CFLAGS += \
	-DUSBD_REG_BASE0=0x50000000 \
	-DUSBH_REG_BASE0=0x50000000
endif
