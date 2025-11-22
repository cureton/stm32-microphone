#pragma once

#include <libopencm3/usb/usbd.h>

/* Interface numbers */
enum {
    IFACE_AUDIO_CONTROL = 0,
    IFACE_AUDIO_STREAM  = 1,
};

/* Endpoint address (iso IN) */
#define EP_AUDIO_IN 0x81

/* Descriptors exposed to main.c */
extern const struct usb_device_descriptor  dev_descriptor;
extern const struct usb_config_descriptor config_descriptor;
extern const char *usb_strings[];

/* Called from main.c after usbd_init() */
void usb_set_config(usbd_device *dev, uint16_t wValue);
void usb_set_unique_serial(void);

