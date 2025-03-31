#include <stdint.h>
#include <stddef.h>
#include "tusb.h"
#include "class/hid/hid_device.h"
#include "device/usbd_pvt.h"

#include "usb/usb.h"
#include "usb/descriptors/xinput.h"
#include "usb/descriptors/dinput.h"
#include "usb/descriptors/switch.h"
#include "usb/drivers/xinput.h"

#define DRIVER_NAME(name) (CFG_TUSB_DEBUG > 1) ? name : NULL

typedef uint16_t (*usbd_hid_get_report_cb_t)(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen);
typedef void (*usbd_hid_set_report_cb_t)(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize);

enum {
    USBD_XINPUT = 0,
    USBD_XBOXOG,
    USBD_DINPUT,
    USBD_SWITCH,
    USBD_MAX,
};

static const uint8_t* usbd_desc_device[USBD_MAX] = {
    xinput_desc_device, // USBD_XINPUT
    NULL,               // USBD_XBOXOG
    dinput_desc_device, // USBD_DINPUT
    switch_desc_device, // USBD_SWITCH
};

static const uint8_t** usbd_desc_string[USBD_MAX] = {
    xinput_desc_string, // USBD_XINPUT
    NULL,               // USBD_XBOXOG
    dinput_desc_string, // USBD_DINPUT
    switch_desc_string, // USBD_SWITCH
};

static const uint8_t* usbd_desc_config[USBD_MAX] = {
    xinput_desc_config, // USBD_XINPUT
    NULL,               // USBD_XBOXOG
    dinput_desc_config, // USBD_DINPUT
    switch_desc_config, // USBD_SWITCH
};

static const uint8_t* usbd_desc_report[USBD_MAX] = {
    NULL,               // USBD_XINPUT
    NULL,               // USBD_XBOXOG
    dinput_desc_report, // USBD_DINPUT
    switch_desc_report, // USBD_SWITCH
};

static const usbd_class_driver_t usbd_driver[USBD_MAX] = {
    // USBD_XINPUT
    {
        .name               = DRIVER_NAME("XInput"),
        .init               = xinput_init,
        .deinit             = xinput_deinit,
        .reset              = xinput_reset,
        .open               = xinput_open,
        .control_xfer_cb    = xinput_control_xfer_cb,
        .xfer_cb            = xinput_xfer_cb,
        .sof                = NULL,
    }, 
    // USBD_XBOXOG
    {
        .name               = DRIVER_NAME("XboxOG"),
        .init               = xinput_init,
        .deinit             = xinput_deinit,
        .reset              = xinput_reset,
        .open               = xinput_open,
        .control_xfer_cb    = xinput_control_xfer_cb,
        .xfer_cb            = xinput_xfer_cb,
        .sof                = NULL,
    },
    // USBD_DINPUT
    {
        .name               = DRIVER_NAME("DInput"),
        .init               = hidd_init,
        .deinit             = hidd_deinit,
        .reset              = hidd_reset,
        .open               = hidd_open,
        .control_xfer_cb    = hidd_control_xfer_cb,
        .xfer_cb            = hidd_xfer_cb,
        .sof                = NULL,
    },
    // USBD_SWITCH
    {
        .name               = DRIVER_NAME("Switch"),
        .init               = hidd_init,
        .deinit             = hidd_deinit,
        .reset              = hidd_reset,
        .open               = hidd_open,
        .control_xfer_cb    = hidd_control_xfer_cb,
        .xfer_cb            = hidd_xfer_cb,
        .sof                = NULL,
    },
};

static const usbd_hid_get_report_cb_t hid_get_report_cb[USBD_MAX] = {
    NULL, // USBD_XINPUT
    NULL, // USBD_XBOXOG
    NULL, // USBD_DINPUT
    NULL, // USBD_SWITCH
};

static const usbd_hid_set_report_cb_t hid_set_report_cb[USBD_MAX] = {
    NULL, // USBD_XINPUT
    NULL, // USBD_XBOXOG
    NULL, // USBD_DINPUT
    NULL, // USBD_SWITCH
};

static uint32_t usbd_device_type = USBD_XINPUT;

void usb_device_config(uint32_t device_type) {
    usbd_device_type = device_type;
}

void usb_device_task(void *param) {
    (void) param;
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    // if (board_init_after_tusb) {
    //     board_init_after_tusb();
    // }

    while (1) {
        tud_task();
    }
}

// TinyUSB Callbacks

// Driver

const usbd_class_driver_t *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    return &usbd_driver[usbd_device_type];
}

// Desc

uint8_t const *tud_descriptor_device_cb() {
    return usbd_desc_device[usbd_device_type];
}

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    return usbd_desc_config[usbd_device_type];
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) {
    return usbd_desc_report[usbd_device_type];
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    if (!usbd_desc_string[usbd_device_type] || !usbd_desc_string[usbd_device_type][index]) {
        return NULL;
    }
    static uint16_t string_desc_buffer[32];

    const char *value = (const char*)usbd_desc_string[usbd_device_type][index];
    size_t char_count;

    if (index == 0) {
        char_count = 1;
    } else {
        char_count = strlen(value);
        if (char_count > 31) {
            char_count = 31;
        }
    }
    for (uint8_t i = 0; i < char_count; i++) {
        string_desc_buffer[i + 1] = value[i];
    }
    string_desc_buffer[0] = (uint16_t)((0x03 << 8) | (2 * (uint8_t)char_count + 2));
    return string_desc_buffer;
}

// uint8_t const* tud_descriptor_device_qualifier_cb() {
//     return NULL;
// }

// Xfer

uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
	if (hid_get_report_cb[usbd_device_type]) {
        return hid_get_report_cb[usbd_device_type](itf, report_id, report_type, buffer, reqlen);
    }
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    if (hid_set_report_cb[usbd_device_type]) {
        hid_set_report_cb[usbd_device_type](itf, report_id, report_type, buffer, bufsize);
        tud_hid_report(report_id, buffer, bufsize);
    }
}