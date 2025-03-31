#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

void usb_device_config(uint32_t device_type);
void usb_device_task(void *param);

#endif // _USB_DEVICE_H_