/*
 * Copyright (c) 2025, Jack Grogan
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _DESC_XINPUT_H_
#define _DESC_XINPUT_H_

#include <stdint.h>
#include <esp_attr.h>

// #ifndef __packed
// #define __packed __attribute__((__packed__))
// #endif

enum {
    XINPUT_REPORT_ID_
};

struct xinput_report_in {
    uint8_t report_id;
    uint8_t len;
    uint8_t buttons[2];
    uint8_t trigger_l;
    uint8_t trigger_r;
    int16_t joystick_lx;
    int16_t joystick_ly;
    int16_t joystick_rx;
    int16_t joystick_ry;
    uint8_t reserved[6];
};

struct xinput_report_out {
    uint8_t report_id;
    uint8_t len;
    uint8_t led;
    uint8_t rumble_l;
    uint8_t rumble_r;
    uint8_t reserved[3];
};

static const uint8_t xinput_desc_string_language[]     = { 0x09, 0x04 };
static const uint8_t xinput_desc_string_manufacturer[] = "Microsoft";
static const uint8_t xinput_desc_string_product[]      = "XInput STANDARD GAMEPAD";
static const uint8_t xinput_desc_string_version[]      = "1.0";

static const uint8_t* xinput_desc_string[] = {
    xinput_desc_string_language,
    xinput_desc_string_manufacturer,
    xinput_desc_string_product,
    xinput_desc_string_version
};

static const uint8_t xinput_desc_device[] = {
    0x12,       // bLength
    0x01,       // bDescriptorType (Device)
    0x00, 0x02, // bcdUSB 2.00
    0xFF,	    // bDeviceClass
    0xFF,	    // bDeviceSubClass
    0xFF,	    // bDeviceProtocol
    0x40,	    // bMaxPacketSize0 64
    0x5E, 0x04, // idVendor 0x045E
    0x8E, 0x02, // idProduct 0x028E
    0x14, 0x01, // bcdDevice 2.14
    0x01,       // iManufacturer (String Index)
    0x02,       // iProduct (String Index)
    0x03,       // iSerialNumber (String Index)
    0x01,       // bNumConfigurations 1
};

static const uint8_t xinput_desc_config[] = {
    0x09,        // bLength
    0x02,        // bDescriptorType (Configuration)
    0x30, 0x00,  // wTotalLength 48
    0x01,        // bNumInterfaces 1
    0x01,        // bConfigurationValue
    0x00,        // iConfiguration (String Index)
    0x80,        // bmAttributes
    0xFA,        // bMaxPower 500mA

    0x09,        // bLength
    0x04,        // bDescriptorType (Interface)
    0x00,        // bInterfaceNumber 0
    0x00,        // bAlternateSetting
    0x02,        // bNumEndpoints 2
    0xFF,        // bInterfaceClass
    0x5D,        // bInterfaceSubClass
    0x01,        // bInterfaceProtocol
    0x00,        // iInterface (String Index)

    0x10,        // bLength
    0x21,        // bDescriptorType (HID)
    // 0x10, 0x01,  // bcdHID 1.10
    0x00, 0x01,  // bcdHID 1.00
    0x01,        // bCountryCode
    0x24,        // bNumDescriptors
    0x81,        // bDescriptorType[0] (Unknown 0x81)
    0x14, 0x03,  // wDescriptorLength[0] 788
    0x00,        // bDescriptorType[1] (Unknown 0x00)
    0x03, 0x13,  // wDescriptorLength[1] 4867
    0x01,        // bDescriptorType[2] (Unknown 0x02)
    0x00, 0x03,  // wDescriptorLength[2] 768
    0x00,        // bDescriptorType[3] (Unknown 0x00)

    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x81,        // bEndpointAddress (IN/D2H)
    0x03,        // bmAttributes (Interrupt)
    0x20, 0x00,  // wMaxPacketSize 32
    0x01,        // bInterval 1 (unit depends on device speed)

    0x07,        // bLength
    0x05,        // bDescriptorType (Endpoint)
    0x01,        // bEndpointAddress (OUT/H2D)
    0x03,        // bmAttributes (Interrupt)
    0x20, 0x00,  // wMaxPacketSize 32
    0x08,        // bInterval 8 (unit depends on device speed)
};

#endif // _DESC_XINPUT_H_