/**
 * @file main.c
 * @author Links (lhd@wch.cn)
 * @brief Main file.
 * @version 0.1
 * @date 2026-09-05
 *
 * @copyright Copyright (c) 2026
 *
 */

/* @include */
#include "usbh_core.h"

#define CONFIG_TEST_USBH_HID

#include "usb_host.c"

int main(void)
{
    usbh_initialize(0, REG_BASE0, NULL);
}
