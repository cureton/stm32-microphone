#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>

#include "usb_descriptors.h"

/* Global USB device handle (only used here) */
static usbd_device *usbdev;
static uint8_t control_buffer[128];

/* -------------------------------------------------------------------------- */
/* CLOCK SETUP                                                                */
/* -------------------------------------------------------------------------- */
static void clock_setup(void)
{
    /* 25 MHz HSE → 96 MHz SYSCLK, USB at 48 MHz */
    rcc_clock_setup_pll(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_96MHZ]);
}

/* -------------------------------------------------------------------------- */
/* GPIO: USB pins + fake VBUS + LED                                           */
/* -------------------------------------------------------------------------- */
static void gpio_setup(void)
{
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOC);

    /* USB FS: PA11 = DM, PA12 = DP (AF10) */
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
    gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

    /* Fake VBUS: drive PA9 high */
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO9);
    gpio_set(GPIOA, GPIO9);

    /* LED on PC13 (optional) */
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
    gpio_set(GPIOC, GPIO13);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
    gpio_clear(GPIOA, GPIO0);
}

/* -------------------------------------------------------------------------- */
/* USB SETUP                                                                  */
/* -------------------------------------------------------------------------- */
static void usb_setup(void)
{
    rcc_periph_clock_enable(RCC_OTGFS);
    rcc_periph_reset_pulse(RST_OTGFS);

    usbdev = usbd_init(&otgfs_usb_driver,
                       &dev_descriptor,
                       &config_descriptor,
                       usb_strings,
                       3,
                       control_buffer,
                       sizeof(control_buffer));

    usbd_register_set_config_callback(usbdev, usb_set_config);
}

/* -------------------------------------------------------------------------- */
/* HARD FAULT HANDLER (blinks LED)                                            */
/* -------------------------------------------------------------------------- */
void hard_fault_handler(void);

void hard_fault_handler(void)
{
    while (1) {
        gpio_toggle(GPIOC, GPIO13);
        for (volatile int i = 0; i < 200000; i++) {
            __asm__("nop");
        }
    }
}

/* -------------------------------------------------------------------------- */
/* MAIN                                                                       */
/* -------------------------------------------------------------------------- */
int main(void)
{
    clock_setup();
    gpio_setup();

    usb_set_unique_serial();
    usb_setup();

    while (1) {
        usbd_poll(usbdev);
    }

    return 0;
}
