#ifndef USB_H
#define USB_H


#include <libopencm3/usb/usbd.h>
void usb_setup(void);
void usb_poll(void);
#endif // USB_H
