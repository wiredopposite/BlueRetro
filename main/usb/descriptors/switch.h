#ifndef _SWITCH_DESCRIPTORS_H_
#define _SWITCH_DESCRIPTORS_H_

#include <stdint.h>
#include "tusb.h"

#define	SWITCH_JOYSTICK_MID    0x80

#define SWITCH_DPAD_UP         0x00
#define SWITCH_DPAD_UP_RIGHT   0x01
#define SWITCH_DPAD_RIGHT      0x02
#define SWITCH_DPAD_DOWN_RIGHT 0x03
#define SWITCH_DPAD_DOWN       0x04
#define SWITCH_DPAD_DOWN_LEFT  0x05
#define SWITCH_DPAD_LEFT       0x06
#define SWITCH_DPAD_UP_LEFT    0x07
#define SWITCH_DPAD_CENTER     0x08

#define SWITCH_BUTTON_Y       (1U <<  0)
#define SWITCH_BUTTON_B       (1U <<  1)
#define SWITCH_BUTTON_A       (1U <<  2)
#define SWITCH_BUTTON_X       (1U <<  3)
#define SWITCH_BUTTON_L       (1U <<  4)
#define SWITCH_BUTTON_R       (1U <<  5)
#define SWITCH_BUTTON_ZL      (1U <<  6)
#define SWITCH_BUTTON_ZR      (1U <<  7)
#define SWITCH_BUTTON_MINUS   (1U <<  8)
#define SWITCH_BUTTON_PLUS    (1U <<  9)
#define SWITCH_BUTTON_L3      (1U << 10)
#define SWITCH_BUTTON_R3      (1U << 11)
#define SWITCH_BUTTON_HOME    (1U << 12)
#define SWITCH_BUTTON_CAPTURE (1U << 13)

struct switch_report_in_t {
    uint16_t buttons;
    uint8_t  dpad;
    uint8_t  joystick_lx;
    uint8_t  joystick_ly;
    uint8_t  joystick_rx;
    uint8_t  joystick_ry;
    uint8_t  vendor;
};

static const uint8_t switch_desc_string_language[]     = { 0x09, 0x04 };
static const uint8_t switch_desc_string_manufacturer[] = "HORI CO.,LTD.";
static const uint8_t switch_desc_string_product[]      = "POKKEN CONTROLLER";
static const uint8_t switch_desc_string_version[]      = "1.0";

static const uint8_t *switch_desc_string[] = {
    switch_desc_string_language,
    switch_desc_string_manufacturer,
    switch_desc_string_product,
    switch_desc_string_version
};

static const uint8_t switch_desc_device[] = {
    0x12,        // bLength
    0x01,        // bDescriptorType (Device)
    0x00, 0x02,  // bcdUSB 2.00
    0x00,        // bDeviceClass (Use class information in the Interface Descriptors)
    0x00,        // bDeviceSubClass
    0x00,        // bDeviceProtocol
    0x40,        // bMaxPacketSize0 64
    0x0D, 0x0F,  // idVendor 0x0F0D
    0x92, 0x00,  // idProduct 0x92
    0x00, 0x01,  // bcdDevice 2.00
    0x01,        // iManufacturer (String Index)
    0x02,        // iProduct (String Index)
    0x00,        // iSerialNumber (String Index)
    0x01,        // bNumConfigurations 1
};

static const uint8_t switch_desc_report[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x35, 0x00,        //   Physical Minimum (0)
    0x45, 0x01,        //   Physical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x05, 0x09,        //   Usage Page (Button)
    0x19, 0x01,        //   Usage Minimum (0x01)
    0x29, 0x10,        //   Usage Maximum (0x10)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x25, 0x07,        //   Logical Maximum (7)
    0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    0x75, 0x04,        //   Report Size (4)
    0x95, 0x01,        //   Report Count (1)
    0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    0x09, 0x39,        //   Usage (Hat switch)
    0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    0x65, 0x00,        //   Unit (None)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x46, 0xFF, 0x00,  //   Physical Maximum (255)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    0x09, 0x20,        //   Usage (0x20)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x21, 0x26,  //   Usage (0x2621)
    0x95, 0x08,        //   Report Count (8)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

enum { 
    SWITCH_ITF_NUM_HID1, 
    SWITCH_ITF_NUM_TOTAL 
};

#define SWITCH_CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + (TUD_HID_DESC_LEN * SWITCH_ITF_NUM_TOTAL))

// More interfaces can be added, Switch does support composite HID devices
static const uint8_t switch_desc_config[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(  1,
                            SWITCH_ITF_NUM_TOTAL,
                            0,
                            SWITCH_CONFIG_TOTAL_LEN,
                            TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP,
                            500),

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR( SWITCH_ITF_NUM_HID1,
                        0,
                        HID_ITF_PROTOCOL_NONE,
                        sizeof(switch_desc_report),
                        (0x80 | (SWITCH_ITF_NUM_HID1 + 1)),
                        CFG_TUD_HID_EP_BUFSIZE,
                        1),
};

#endif // _SWITCH_DESCRIPTORS_H_	