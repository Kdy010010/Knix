#ifndef USB_H
#define USB_H

#include "dc.h"

typedef struct {
    uint8 address;
    uint8 device_class;
    uint8 subclass;
    uint8 protocol;
} USB_Device;

extern USB_Device usb_devices[MAX_USB_DEVICES];
extern uint32 usb_device_count;

void usb_scan();
void usb_keyboard_handler();
void usb_mouse_handler();
void usb_poll();


#endif //USB_H
