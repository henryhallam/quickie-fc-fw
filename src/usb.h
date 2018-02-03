#ifndef USB_H
#define USB_H


#include <libopencm3/usb/usbd.h>
#include <stdarg.h>
extern int usb_on;

void usb_setup(void);
void usb_poll(void);
void usb_putc(int i);
int usb_printf(const char *fmt, ...);
#endif // USB_H
