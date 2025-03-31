#include "tusb.h"
#include "device/dcd.h"
#include "device/usbd_pvt.h"

// You must implement these functions for your MAX3421 peripheral mode
extern void max3421_reset(void);
extern void max3421_set_mode_device(void);
extern void max3421_enable_device_interrupts(void);
extern void max3421_disable_device_interrupts(void);
extern void max3421_configure_ep0(void);
extern void max3421_enable_pullup(void);
extern void max3421_disable_pullup(void);
extern bool max3421_configure_endpoint(tusb_desc_endpoint_t const* desc_edpt);
extern bool max3421_transfer(uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes);
extern void max3421_set_device_address(uint8_t dev_addr);
extern void max3421_set_stall(uint8_t ep_addr);
extern void max3421_clear_stall(uint8_t ep_addr);
extern uint8_t max3421_get_int_status(void);
extern void max3421_clear_int_status(uint8_t status);
extern void max3421_read_setup_packet(uint8_t setup_pkt[8]);
extern uint8_t max3421_get_completed_ep(void);
extern uint16_t max3421_get_xferred_bytes(uint8_t ep_addr);

//--------------------------------------------------------------------+
// DCD Callbacks for MAX3421 in Device Mode
//--------------------------------------------------------------------+

// Initialize the MAX3421 in device mode.
bool dcd_init(uint8_t rhport, const tusb_rhport_init_t* rh_init) {
    (void) rh_init;
    (void) rhport;
    // Reset the MAX3421 and force device mode.
    max3421_reset();
    max3421_set_mode_device();
    
    // Configure control endpoint 0.
    max3421_configure_ep0();
    
    // Enable device interrupts.
    max3421_enable_device_interrupts();

    return true;
}

bool dcd_deinit(uint8_t rhport) {
    (void) rhport;
    max3421_disable_device_interrupts();
    // Optionally, power down or revert to host mode.
    return true;
}

// Set the USB device address.
void dcd_set_address(uint8_t rhport, uint8_t dev_addr) {
    (void) rhport;
    max3421_set_device_address(dev_addr);
    // In device mode, the address change must be acknowledged on EP0 IN.
    dcd_edpt_xfer(rhport, tu_edpt_addr(0, TUSB_DIR_IN), NULL, 0);
}

// Connect the device (enable pull-up so that host sees a connection).
void dcd_connect(uint8_t rhport) {
    (void) rhport;
    max3421_enable_pullup();
}

// Disconnect the device.
void dcd_disconnect(uint8_t rhport) {
    (void) rhport;
    max3421_disable_pullup();
}

// Open an endpoint based on its descriptor.
bool dcd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const* desc_edpt) {
    (void) rhport;
    return max3421_configure_endpoint(desc_edpt);
}

// Submit a transfer on an endpoint.
bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t* buffer, uint16_t total_bytes) {
    (void) rhport;
    return max3421_transfer(ep_addr, buffer, total_bytes);
}

// Stall an endpoint.
void dcd_edpt_stall(uint8_t rhport, uint8_t ep_addr) {
    (void) rhport;
    max3421_set_stall(ep_addr);
}

// Clear stall on an endpoint.
void dcd_edpt_clear_stall(uint8_t rhport, uint8_t ep_addr) {
    (void) rhport;
    max3421_clear_stall(ep_addr);
}

//--------------------------------------------------------------------+
// Interrupt Handler (to be called from your ISR)
//--------------------------------------------------------------------+
void dcd_int_handler(uint8_t rhport, bool in_isr)
{
    (void) rhport;
    uint8_t status = max3421_get_int_status();
    if (!status) return;
    
    // Clear the interrupt status as needed.
    max3421_clear_int_status(status);

    // Check if a SETUP packet was received.
    if (status & /* your flag for setup received */ 0x01) {
        uint8_t setup_pkt[8];
        max3421_read_setup_packet(setup_pkt);
        dcd_event_setup_received(rhport, setup_pkt, in_isr);
    }
    
    // Check for transfer complete events.
    if (status & /* your flag for xfer complete */ 0x02) {
        uint8_t ep_addr = max3421_get_completed_ep();
        uint16_t xferred = max3421_get_xferred_bytes(ep_addr);
        dcd_event_xfer_complete(rhport, ep_addr, xferred, XFER_RESULT_SUCCESS, in_isr);
    }
    
    // Handle other events such as bus reset, suspend, resume as needed.
    if (status & /* bus reset flag */ 0x04) {
        dcd_event_bus_reset(rhport, TUSB_SPEED_FULL, in_isr);
    }
}
