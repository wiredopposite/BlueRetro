#ifndef _USB_DRIVER_XINPUT_H_
#define _USB_DRIVER_XINPUT_H_

#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"

// Xfer API

bool xinput_send_report_ready();
bool xinput_receive_report_ready();
bool xinput_send_report(const uint8_t *report, uint16_t len);
bool xinput_receive_report(uint8_t *report, uint16_t len);

// Class Driver

void xinput_init(void);
bool xinput_deinit(void);
void xinput_reset(uint8_t rhport);
uint16_t xinput_open(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length);
bool xinput_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request);
bool xinput_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes);

#endif // _USB_DRIVER_XINPUT_H_