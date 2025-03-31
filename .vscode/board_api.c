#include "tusb.h"
#include "board_api.h"

//--------------------------------------------------------------------+
// Board porting API
//--------------------------------------------------------------------+

void board_init(void) {
    tud_max3421_init();
}

size_t board_get_unique_id(uint8_t id[], size_t max_len) {
    // use factory default MAC as serial ID
    esp_efuse_mac_get_default(id);
    return 6;
}

// void board_led_write(bool state) {
//     (void)state;
// }

// // Get the current state of button
// // a '1' means active (pressed), a '0' means inactive.
// uint32_t board_button_read(void) {
//     return gpio_get_level(BUTTON_PIN) == BUTTON_STATE_ACTIVE;
// }

// Get characters from UART
int board_uart_read(uint8_t* buf, int len) {
    for (int i=0; i<len; i++) {
        int c = getchar();
        if (c == EOF) {
            return i;
        }
        buf[i] = (uint8_t) c;
    }
    return len;
}

// Send characters to UART
int board_uart_write(void const* buf, int len) {
    for (int i = 0; i < len; i++) {
        putchar(((char*) buf)[i]);
    }
    return len;
}

int board_getchar(void) {
    return getchar();
}