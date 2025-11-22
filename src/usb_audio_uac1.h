#pragma once

#include <stdint.h>

/*
 * Minimal USB Audio Class 1.0 (UAC1) definitions for
 * a simple mono microphone (Type I PCM).
 *
 * Project-local, avoids pulling in libopencm3's usb/audio.h
 * to prevent name clashes.
 */

/* -------------------------------------------------------------------------- */
/* Class-specific descriptor types                                            */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_DT_CS_UNDEFINED      0x20
#define USB_AUDIO_DT_CS_DEVICE         0x21
#define USB_AUDIO_DT_CS_CONFIGURATION  0x22
#define USB_AUDIO_DT_CS_STRING         0x23
#define USB_AUDIO_DT_CS_INTERFACE      0x24
#define USB_AUDIO_DT_CS_ENDPOINT       0x25

/* Convenience aliases as used in descriptors */
#define USB_DT_CS_INTERFACE            USB_AUDIO_DT_CS_INTERFACE
#define USB_DT_CS_ENDPOINT             USB_AUDIO_DT_CS_ENDPOINT

/* -------------------------------------------------------------------------- */
/* Audio interface subclasses                                                 */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_SUBCLASS_UNDEFINED       0x00
#define USB_AUDIO_SUBCLASS_AUDIOCONTROL    0x01
#define USB_AUDIO_SUBCLASS_AUDIOSTREAMING  0x02
#define USB_AUDIO_SUBCLASS_MIDISTREAMING   0x03

/* -------------------------------------------------------------------------- */
/* AudioControl (AC) descriptor subtypes                                      */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_SUBTYPE_AC_UNDEFINED         0x00
#define USB_AUDIO_SUBTYPE_AC_HEADER            0x01
#define USB_AUDIO_SUBTYPE_AC_INPUT_TERMINAL    0x02
#define USB_AUDIO_SUBTYPE_AC_OUTPUT_TERMINAL   0x03
#define USB_AUDIO_SUBTYPE_AC_MIXER_UNIT        0x04
#define USB_AUDIO_SUBTYPE_AC_SELECTOR_UNIT     0x05
#define USB_AUDIO_SUBTYPE_AC_FEATURE_UNIT      0x06

/* -------------------------------------------------------------------------- */
/* AudioStreaming (AS) descriptor subtypes                                    */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_SUBTYPE_AS_UNDEFINED         0x00
#define USB_AUDIO_SUBTYPE_AS_GENERAL           0x01
#define USB_AUDIO_SUBTYPE_AS_FORMAT_TYPE       0x02

/* -------------------------------------------------------------------------- */
/* Format types (Type I PCM)                                                  */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_FORMAT_TYPE_UNDEFINED   0x00
#define USB_AUDIO_FORMAT_TYPE_I           0x01

#define USB_AUDIO_FORMAT_I_UNDEFINED      0x0000
#define USB_AUDIO_FORMAT_I_PCM            0x0001

/* -------------------------------------------------------------------------- */
/* Terminal types (subset)                                                    */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_TERMINAL_UNDEFINED      0x0100
#define USB_AUDIO_TERMINAL_STREAMING      0x0101

/* Input terminals (0x0200) */
#define USB_AUDIO_TERMINAL_IN_UNDEFINED   0x0200
#define USB_AUDIO_TERMINAL_MICROPHONE     0x0201

/* Output terminals (0x0300) */
#define USB_AUDIO_TERMINAL_OUT_UNDEFINED  0x0300
#define USB_AUDIO_TERMINAL_SPEAKER        0x0301

/* -------------------------------------------------------------------------- */
/* Feature Unit control bits                                                  */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_FU_MUTE                 (1 << 0)
#define USB_AUDIO_FU_VOLUME               (1 << 1)

/* Endpoint-specific subtype */
#define USB_AUDIO_SUBTYPE_EP_UNDEFINED    0x00
#define USB_AUDIO_SUBTYPE_EP_GENERAL      0x01

/* -------------------------------------------------------------------------- */
/* Device class code                                                          */
/* -------------------------------------------------------------------------- */
#define USB_CLASS_AUDIO                   0x01

/* -------------------------------------------------------------------------- */
/* UAC version                                                                */
/* -------------------------------------------------------------------------- */
#define USB_AUDIO_BCD_VERSION_1_00        0x0100

/* -------------------------------------------------------------------------- */
/* Project topology IDs                                                       */
/* -------------------------------------------------------------------------- */
#define AUDIO_INPUT_TERM_ID    0x01  /* Microphone IT */
#define AUDIO_FEATURE_UNIT_ID  0x02  /* Feature Unit   */
#define AUDIO_OUTPUT_TERM_ID   0x03  /* USB Streaming OT */

/* -------------------------------------------------------------------------- */
/* Audio format parameters                                                    */
/* -------------------------------------------------------------------------- */

/* 48 kHz / mono / 16-bit mic */
#define AUDIO_SAMPLE_RATE_HZ        48000
#define AUDIO_NUM_CHANNELS          1
#define AUDIO_BITS_PER_SAMPLE       16
#define AUDIO_BYTES_PER_SAMPLE      (AUDIO_BITS_PER_SAMPLE / 8)

/* Packet size for 1ms USB frames: Fs * channels * bytes_per_sample / 1000 */
#define AUDIO_PACKET_SIZE  ((AUDIO_SAMPLE_RATE_HZ * AUDIO_NUM_CHANNELS * AUDIO_BYTES_PER_SAMPLE) / 1000)
