#include <stddef.h>
#include <stdbool.h>
#include <math.h>

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/usbstd.h>


#include <libopencm3/stm32/desig.h> // For getting device uniq id -> usb serial

#include "usb_audio_uac1.h"
#include "usb_descriptors.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static uint8_t audio_stream_cur_altsetting = 0;

/* -------------------------------------------------------------------------- */
/* DEVICE DESCRIPTOR                                                          */
/* -------------------------------------------------------------------------- */

const struct usb_device_descriptor dev_descriptor = {
    .bLength            = USB_DT_DEVICE_SIZE,
    .bDescriptorType    = USB_DT_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0,      /* class at interface level */
    .bDeviceSubClass    = 0,
    .bDeviceProtocol    = 0,
    .bMaxPacketSize0    = 64,
    .idVendor           = 0x1209, /* pid.codes community VID */
    .idProduct          = 0x0002, /* your PID */
    .bcdDevice          = 0x0100,
    .iManufacturer      = 1,
    .iProduct           = 2,
    .iSerialNumber      = 3,
    .bNumConfigurations = 1,
};

/* -------------------------------------------------------------------------- */
/* AUDIO CONTROL (AC) CLASS-SPECIFIC BLOCK                                    */
/* -------------------------------------------------------------------------- */

static const uint8_t audio_ac_cs[] = {
    /* Class-specific AC Interface Header */
    9,  USB_DT_CS_INTERFACE, USB_AUDIO_SUBTYPE_AC_HEADER,
    (USB_AUDIO_BCD_VERSION_1_00 & 0xFF),
    (USB_AUDIO_BCD_VERSION_1_00 >> 8),
    0x27, 0x00,      /* wTotalLength = 39 bytes */
    0x01,            /* bInCollection = 1 streaming IF */
    IFACE_AUDIO_STREAM,

    /* Input Terminal (Microphone) */
    12, USB_DT_CS_INTERFACE, USB_AUDIO_SUBTYPE_AC_INPUT_TERMINAL,
    AUDIO_INPUT_TERM_ID,
    (USB_AUDIO_TERMINAL_MICROPHONE & 0xFF),
    (USB_AUDIO_TERMINAL_MICROPHONE >> 8),
    0x00,            /* bAssocTerminal */
    0x01,            /* bNrChannels = 1 */
    0x00, 0x00,      /* wChannelConfig = mono */
    0x00,            /* iChannelNames */
    0x00,            /* iTerminal */

    /* Feature Unit (no controls exposed) */
    9,  USB_DT_CS_INTERFACE, USB_AUDIO_SUBTYPE_AC_FEATURE_UNIT,
    AUDIO_FEATURE_UNIT_ID,
    AUDIO_INPUT_TERM_ID,
    0x01,            /* bControlSize = 1 byte */
    0x00,            /* bmaControls(0) */
    0x00,            /* bmaControls(1) */
    0x00,            /* iFeature */

    /* Output Terminal (USB Streaming) */
    9,  USB_DT_CS_INTERFACE, USB_AUDIO_SUBTYPE_AC_OUTPUT_TERMINAL,
    AUDIO_OUTPUT_TERM_ID,
    (USB_AUDIO_TERMINAL_STREAMING & 0xFF),
    (USB_AUDIO_TERMINAL_STREAMING >> 8),
    0x00,            /* bAssocTerminal */
    AUDIO_FEATURE_UNIT_ID,
    0x00             /* iTerminal */
};

/* -------------------------------------------------------------------------- */
/* AUDIO STREAMING (AS) ALT 1: GENERAL + FORMAT                               */
/* -------------------------------------------------------------------------- */

static const uint8_t audio_as_alt1_cs[] = {
    /* AS General */
    7,  USB_DT_CS_INTERFACE, USB_AUDIO_SUBTYPE_AS_GENERAL,
    AUDIO_OUTPUT_TERM_ID,        /* bTerminalLink */
    0x00,                        /* bDelay */
    (USB_AUDIO_FORMAT_I_PCM & 0xFF),
    (USB_AUDIO_FORMAT_I_PCM >> 8),

    /* Type I Format Descriptor */
    11, USB_DT_CS_INTERFACE, USB_AUDIO_SUBTYPE_AS_FORMAT_TYPE,
    USB_AUDIO_FORMAT_TYPE_I,
    AUDIO_NUM_CHANNELS,
    AUDIO_BYTES_PER_SAMPLE,
    AUDIO_BITS_PER_SAMPLE,
    0x01,                        /* bSamFreqType = 1 discrete frequency */
    (AUDIO_SAMPLE_RATE_HZ & 0xFF),
    (AUDIO_SAMPLE_RATE_HZ >> 8) & 0xFF,
    (AUDIO_SAMPLE_RATE_HZ >> 16) & 0xFF
};

/* -------------------------------------------------------------------------- */
/* ISOCHRONOUS ENDPOINT (DATA EP + CS EP)                                     */
/* -------------------------------------------------------------------------- */

static const uint8_t audio_cs_ep[] = {
    7,  USB_DT_CS_ENDPOINT, USB_AUDIO_SUBTYPE_EP_GENERAL,
    0x00, /* bmAttributes: no pitch, no freq control */
    0x00, /* bLockDelayUnits */
    0x00, 0x00 /* wLockDelay */
};

static const struct usb_endpoint_descriptor audio_iso_ep[] = { {
    .bLength          = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType  = USB_DT_ENDPOINT,
    .bEndpointAddress = EP_AUDIO_IN,     /* 0x81 */
    .bmAttributes     = USB_ENDPOINT_ATTR_ISOCHRONOUS |
                        USB_ENDPOINT_ATTR_ADAPTIVE,    /* isoc async data */
    .wMaxPacketSize   = AUDIO_PACKET_SIZE,             /* 96 bytes */
    .bInterval        = 1,
    .extra            = audio_cs_ep,
    .extralen         = sizeof(audio_cs_ep),
} };

/* -------------------------------------------------------------------------- */
/* INTERFACES                                                                 */
/* -------------------------------------------------------------------------- */

static const struct usb_interface_descriptor audio_ac_iface[] = { {
    .bLength         = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber    = IFACE_AUDIO_CONTROL,
    .bAlternateSetting   = 0,
    .bNumEndpoints       = 0,
    .bInterfaceClass     = USB_CLASS_AUDIO,
    .bInterfaceSubClass  = USB_AUDIO_SUBCLASS_AUDIOCONTROL,
    .bInterfaceProtocol  = 0,
    .iInterface          = 0,
    .endpoint            = NULL,
    .extra               = audio_ac_cs,
    .extralen            = sizeof(audio_ac_cs),
} };

static const struct usb_interface_descriptor audio_as_iface[] = {
    {   /* Alt 0: zero bandwidth */
        .bLength         = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber    = IFACE_AUDIO_STREAM,
        .bAlternateSetting   = 0,
        .bNumEndpoints       = 0,
        .bInterfaceClass     = USB_CLASS_AUDIO,
        .bInterfaceSubClass  = USB_AUDIO_SUBCLASS_AUDIOSTREAMING,
        .bInterfaceProtocol  = 0,
        .iInterface          = 0,
        .endpoint            = NULL,
        .extra               = NULL,
        .extralen            = 0,
    },
    {   /* Alt 1: active streaming */
        .bLength         = USB_DT_INTERFACE_SIZE,
        .bDescriptorType = USB_DT_INTERFACE,
        .bInterfaceNumber    = IFACE_AUDIO_STREAM,
        .bAlternateSetting   = 1,
        .bNumEndpoints       = 1,
        .bInterfaceClass     = USB_CLASS_AUDIO,
        .bInterfaceSubClass  = USB_AUDIO_SUBCLASS_AUDIOSTREAMING,
        .bInterfaceProtocol  = 0,
        .iInterface          = 0,
        .endpoint            = audio_iso_ep,
        .extra               = audio_as_alt1_cs,
        .extralen            = sizeof(audio_as_alt1_cs),
    }
};

static const struct usb_interface interfaces[] = {
    {
        .num_altsetting = 1,
        .altsetting     = audio_ac_iface,
    },
    {
        .num_altsetting = 2,
        .altsetting     = audio_as_iface,
        .cur_altsetting = &audio_stream_cur_altsetting,   /* REQUIRED */
    },
};

/* -------------------------------------------------------------------------- */
/* CONFIG DESCRIPTOR                                                          */
/* -------------------------------------------------------------------------- */

const struct usb_config_descriptor config_descriptor = {
    .bLength             = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType     = USB_DT_CONFIGURATION,
    .wTotalLength        = 0,   /* filled in by library */
    .bNumInterfaces      = 2,
    .bConfigurationValue = 1,
    .iConfiguration      = 0,
    .bmAttributes        = 0x80, /* bus powered */
    .bMaxPower           = 50,   /* 100 mA */
    .interface           = interfaces,
};

/* -------------------------------------------------------------------------- */
/* STRING DESCRIPTORS                                                         */
/* -------------------------------------------------------------------------- */

/*  Space to populate STM32 uniw ID into as hex */
#define SERIAL_BUFFER_LEN = 25  /* Max 126 chars */

static char usb_serial[25] = "STM32 Unique ID";

const char *usb_strings[] = {
    "Your Manufacturer",
    "STM32F411 UAC1 Microphone",
    usb_serial,
};

/* -------------------------------------------------------------------------- */
/* STREAMING STATE + SOF-DRIVEN AUDIO                                         */
/* -------------------------------------------------------------------------- */

static volatile bool usb_configured          = false;
static volatile bool audio_streaming_enabled = false;
static usbd_device *audio_dev                = NULL;

/* SOF callback: generate a 1 kHz sine at 48 kHz mono 16-bit */
static void audio_sof_callback(void)
{
    if (!usb_configured || !audio_streaming_enabled || !audio_dev) {
        return;
    }

    static uint16_t phase = 0;
    static uint8_t  pcm[AUDIO_PACKET_SIZE];

    gpio_set(GPIOA, GPIO0);
    for (int i = 0; i < AUDIO_PACKET_SIZE / 2; i++) {
        /* simple phase -> time in cycles */
//g        float t = 2.0f * (float)M_PI * 1000.0f *
//g                  (float)phase / (float)AUDIO_SAMPLE_RATE_HZ; /* 1 kHz */
//g        float s = sinf(t) * 30000.0f;
//g        int16_t sample = (int16_t)s;
//g
        int16_t sample = 4096;
        pcm[2 * i + 0] = (uint8_t)(sample & 0xFF);
        pcm[2 * i + 1] = (uint8_t)((sample >> 8) & 0xFF);

 //       phase++;
        gpio_clear(GPIOA, GPIO0);

    }

    (void)usbd_ep_write_packet(audio_dev, EP_AUDIO_IN, pcm, AUDIO_PACKET_SIZE);
}

/* Altsetting callback: enable/disable streaming */
static void audio_set_interface(usbd_device *dev, uint16_t iface, uint16_t alt)
{
    (void)dev;

    if (iface == IFACE_AUDIO_STREAM) {
        audio_streaming_enabled = (alt == 1);
    }
}

/* Convert a work to 8 hex chars */
static void word_to_hex(uint32_t w, char *dst)
{
    for (int i = 0; i < 8; i++) {
        uint8_t nib = (w >> 28) & 0xF;
        w <<= 4;
        dst[i] = (nib < 10) ? ('0' + nib) : ('A' + (nib - 10));
    }
}

/* Copy STM32 uniqie id into serial localtion  */
void usb_set_unique_serial() 
{
    uint32_t uid[3];

    /* Get pointer to uniq id rehister  from libopencm3*/
    desig_get_unique_id(uid); 
    
    word_to_hex(uid[2], &usb_serial[0]);
    word_to_hex(uid[1], &usb_serial[8]);
    word_to_hex(uid[0], &usb_serial[16]);

    usb_serial[24] = '\0';
}

/* -------------------------------------------------------------------------- */
/* SET CONFIG CALLBACK                                                        */
/* -------------------------------------------------------------------------- */

void usb_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
    (void)wValue;

    audio_dev      = usbd_dev;
    usb_configured = true;

    /* Configure ISO IN endpoint (alt=1 will actually be used by host) */
    usbd_ep_setup(usbd_dev,
                  EP_AUDIO_IN,
                  USB_ENDPOINT_ATTR_ISOCHRONOUS | USB_ENDPOINT_ATTR_ASYNC,
                  AUDIO_PACKET_SIZE,
                  NULL);

    /* Register callbacks */
    usbd_register_set_altsetting_callback(usbd_dev, audio_set_interface);
    usbd_register_sof_callback(usbd_dev, audio_sof_callback);
}
