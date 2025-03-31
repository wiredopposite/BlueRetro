#include "tusb_option.h"
#if (CFG_TUD_ENABLED && CFG_TUD_XINPUT)

#include <stddef.h>
#include <string.h>
#include "device/usbd_pvt.h"
#include "usb/drivers/xinput.h"

#define ENDPOINT_SIZE 32

typedef struct xinput_itf_t {
    uint8_t  ep_in;
    uint8_t  ep_out;
    uint16_t ep_in_size;
    uint16_t ep_out_size;
    TUD_EPBUF_DEF(ep_in_buf, ENDPOINT_SIZE);
    TUD_EPBUF_DEF(ep_out_buf, ENDPOINT_SIZE);
};

// XInput only uses one interface for gamepad data
// This would change if supporting headsets or some other gamepad peripherals
static struct xinput_itf_t xinput_itf;

// Class Driver

void xinput_init(void) {
    tu_memclr(&xinput_itf, sizeof(xinput_itf));
    xinput_itf.ep_in = 0xFF;
    xinput_itf.ep_out = 0xFF;
}

bool xinput_deinit(void) {
    xinput_init();
    return true;
}

void xinput_reset(uint8_t rhport) {
    (void) rhport;
    xinput_init();
}

uint16_t xinput_open(uint8_t rhport, tusb_desc_interface_t const *itf_descriptor, uint16_t max_length) {
	uint16_t driver_length = 
        sizeof(tusb_desc_interface_t) + (itf_descriptor->bNumEndpoints * sizeof(tusb_desc_endpoint_t)) + 16;

	TU_VERIFY(max_length >= driver_length, 0);

	uint8_t const *current_descriptor = tu_desc_next(itf_descriptor);
	uint8_t found_endpoints = 0;

	while ((found_endpoints < itf_descriptor->bNumEndpoints) && (driver_length <= max_length)) {
		const tusb_desc_endpoint_t *endpoint_descriptor = (const tusb_desc_endpoint_t*)current_descriptor;

		if (TUSB_DESC_ENDPOINT == tu_desc_type(endpoint_descriptor)) {
			TU_ASSERT(usbd_edpt_open(rhport, endpoint_descriptor));

			if (tu_edpt_dir(endpoint_descriptor->bEndpointAddress) == TUSB_DIR_IN) {
				xinput_itf.ep_in = endpoint_descriptor->bEndpointAddress;
                xinput_itf.ep_in_size = tu_edpt_packet_size(endpoint_descriptor);
            } else {
				xinput_itf.ep_out = endpoint_descriptor->bEndpointAddress;
                xinput_itf.ep_out_size = tu_edpt_packet_size(endpoint_descriptor);
            }
			++found_endpoints;
		}
		current_descriptor = tu_desc_next(current_descriptor);
	}
	return driver_length;
}

bool xinput_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
	return true;
}

bool xinput_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
	if (ep_addr == xinput_itf.ep_out) {
        usbd_edpt_xfer(BOARD_TUD_RHPORT, xinput_itf.ep_out, xinput_itf.ep_out_buf, xinput_itf.ep_out_size);
    }
	return true;
}

// Xfer API

bool xinput_send_report_ready() {
    if (tud_ready() && (xinput_itf.ep_in != 0xFF) && (!usbd_edpt_busy(BOARD_TUD_RHPORT, xinput_itf.ep_in))) {
        return true;
    }
    return false;
}

bool xinput_receive_report_ready() {
    if (tud_ready() && (xinput_itf.ep_out != 0xFF) && (!usbd_edpt_busy(BOARD_TUD_RHPORT, xinput_itf.ep_out))) {
        return true;
    }
    return false;
}

bool xinput_send_report(const uint8_t *report, uint16_t len) {
    if (xinput_send_report_ready()) {
        uint16_t xfer_len = tu_min16(len, xinput_itf.ep_in_size);
        memcpy(xinput_itf.ep_in_buf, report, xfer_len);

        if (usbd_edpt_claim(BOARD_TUD_RHPORT, xinput_itf.ep_in)) {
            usbd_edpt_xfer(BOARD_TUD_RHPORT, xinput_itf.ep_in, xinput_itf.ep_in_buf, xfer_len);
            usbd_edpt_release(BOARD_TUD_RHPORT, xinput_itf.ep_in);
            return true;
        }
    }
    return false;
}

bool xinput_receive_report(uint8_t *report, uint16_t len) {
    uint16_t xfer_len = tu_min16(len, xinput_itf.ep_out_size);
    if (xinput_receive_report_ready() && usbd_edpt_claim(BOARD_TUD_RHPORT, xinput_itf.ep_out)) {
        usbd_edpt_xfer(BOARD_TUD_RHPORT, xinput_itf.ep_out, xinput_itf.ep_out_buf, xfer_len);
        usbd_edpt_release(BOARD_TUD_RHPORT, xinput_itf.ep_out);
    }
    memcpy(report, xinput_itf.ep_out_buf, xfer_len);
    return true;
}

#endif // CFG_TUD_ENABLED && CFG_TUD_XINPUT