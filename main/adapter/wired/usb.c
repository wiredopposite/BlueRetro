#include "usb.h"
#include "usb/usb_xinput.h"
#include "adapter/adapter.h"

static const meta_init_t usb_meta_init_func[USB_WIRED_MAX] = {
    usb_xinput_meta_init, /* XINPUT */
    NULL, /* XBOXOG */
    NULL, /* DINPUT */
    NULL, /* SWITCH */
};

static const buffer_init_t usb_init_buffer_func[USB_WIRED_MAX] = {
    usb_xinput_init_buffer, /* XINPUT */
    NULL, /* XBOXOG */
    NULL, /* DINPUT */
    NULL, /* SWITCH */
};

static const from_generic_t usb_from_generic_func[USB_WIRED_MAX] = {
    usb_xinput_from_generic, /* XINPUT */
    NULL, /* XBOXOG */
    NULL, /* DINPUT */
    NULL, /* SWITCH */
};

static const fb_to_generic_t usb_fb_to_generic_func[USB_WIRED_MAX] = {
    usb_xinput_fb_to_generic, /* XINPUT */
    NULL, /* XBOXOG */
    NULL, /* DINPUT */
    NULL, /* SWITCH */
};

void usb_meta_init(struct wired_ctrl *ctrl_data) {
    if (usb_meta_init_func[wired_adapter.usb_system_id]) {
        usb_meta_init_func[wired_adapter.usb_system_id](ctrl_data);
    }
}

void usb_init_buffer(int32_t dev_mode, struct wired_data *wired_data) {
    if (usb_init_buffer_func[wired_adapter.usb_system_id]) {
        usb_init_buffer_func[wired_adapter.usb_system_id](dev_mode, wired_data);
    }
}

void usb_from_generic(int32_t dev_mode, struct wired_ctrl *ctrl_data, struct wired_data *wired_data) {
    if (usb_from_generic_func[wired_adapter.usb_system_id]) {
        usb_from_generic_func[wired_adapter.usb_system_id](dev_mode, ctrl_data, wired_data);
    }
}

void usb_gen_turbo_mask(struct wired_data *wired_data) {
    if (usb_gen_turbo_mask_func[wired_adapter.usb_system_id]) {
        usb_gen_turbo_mask_func[wired_adapter.usb_system_id](wired_data);
    }
}

void usb_fb_to_generic(int32_t dev_mode, struct raw_fb *raw_fb_data, struct generic_fb *fb_data) {
    if (usb_fb_to_generic_func[wired_adapter.usb_system_id]) {
        usb_fb_to_generic_func[wired_adapter.usb_system_id](dev_mode, raw_fb_data, fb_data);
    }
}