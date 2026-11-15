#ifndef AUDIO_EP_NEXT_FRAME_H
#define AUDIO_EP_NEXT_FRAME_H

#include <stdint.h>
#include <libopencm3/cm3/common.h>
#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/dwc/otg_common.h>

/*
 * Queue one packet for the next USB frame on an OTG FS isochronous
 * IN endpoint configured with bInterval = 1.
 *
 * ep_addr is the full endpoint address, e.g. 0x82 for EP2 IN.
 * Returns the number of bytes queued, or zero if the endpoint is busy
 * or usbd_ep_write_packet() cannot queue the packet.
 *
 * Call from the same serialized USB context as usbd_poll(), normally
 * the IN completion callback. Also use this for the initial packet.
 * Service promptly: this does not implement missed-frame recovery.
 * This helper is specific to OTG FS, not the OTG HS peripheral.
 */
static inline uint16_t audio_ep_write_next_frame(
    usbd_device *dev,
    uint8_t ep_addr,
    const void *data,
    uint16_t length)
{
    const uint8_t ep = ep_addr & 0x7f;
    volatile uint32_t *ctl =
        &MMIO32(USB_OTG_FS_BASE + OTG_DIEPCTL(ep));

    /* Do not change the scheduling of an active transfer. */
    if (*ctl & OTG_DIEPCTL0_EPENA)
        return 0;

    const uint32_t dsts =
        MMIO32(USB_OTG_FS_BASE + OTG_DSTS);

    /* DSTS bit 8 is the current frame parity. Select the opposite. */
    const uint32_t select = (dsts & (1u << 8))
        ? (1u << 28)     /* SEVNFRM: next frame is even. */
        : (1u << 29);    /* SODDFRM: next frame is odd. */

    *ctl |= select;

    return usbd_ep_write_packet(dev, ep_addr, data, length);
}

#endif /* AUDIO_EP_NEXT_FRAME_H */
