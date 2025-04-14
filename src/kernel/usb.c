#include "usb.h"
#include "kprint.h"

/*=========================*/
/* 13. USB Driver (Stub) */
/*=========================*/
USB_Device usb_devices[MAX_USB_DEVICES];
uint32 usb_device_count = 0;

void usb_scan() {
    usb_device_count = 2;
    usb_devices[0].address = 1;
    usb_devices[0].device_class = USB_CLASS_HID;
    usb_devices[0].subclass = USB_SUBCLASS_BOOT;
    usb_devices[0].protocol = USB_PROTOCOL_KEYBOARD;

    usb_devices[1].address = 2;
    usb_devices[1].device_class = USB_CLASS_HID;
    usb_devices[1].subclass = USB_SUBCLASS_BOOT;
    usb_devices[1].protocol = USB_PROTOCOL_MOUSE;

    kprint("USB Device Scan Completed. Number: ");
    kprint_hex(usb_device_count);
    kprint("\n");
}

void usb_keyboard_handler() {
    kprint("USB Keyboard Event Occurred.\n");
}

void usb_mouse_handler() {
    kprint("USB Mouse Event Occurred.\n");
}

void usb_poll() {
    uint32 i;
    for (i = 0; i < usb_device_count; i++) {
        if (usb_devices[i].device_class == USB_CLASS_HID) {
            if (usb_devices[i].protocol == USB_PROTOCOL_KEYBOARD)
                usb_keyboard_handler();
            else if (usb_devices[i].protocol == USB_PROTOCOL_MOUSE)
                usb_mouse_handler();
        }
    }
}
