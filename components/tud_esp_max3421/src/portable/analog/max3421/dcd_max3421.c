#include "tusb_option.h"

#include <stdatomic.h>
#include "tusb.h"
#include "device/dcd.h"

//Peripheral Mode Registers
enum {
    EP0FIFO_ADDR    = (0 << 3),
    EP1OUTFIFO_ADDR = (1 << 3),
    EP2INFIFO_ADDR  = (2 << 3),
    EP3INFIFO_ADDR  = (3 << 3),
    SUDFIFO_ADDR    = (4 << 3),
    EP0BC_ADDR      = (5 << 3),
    EP1OUTBC_ADDR   = (6 << 3),
    EP2INBC_ADDR    = (7 << 3),
    EP3INBC_ADDR    = (8 << 3),
    EPSTALLS_ADDR   = (9 << 3),
    CLRTOGS_ADDR    = (10 << 3),
    EPIRQ_ADDR      = (11 << 3),
    EPIEN_ADDR      = (12 << 3),
    USBIRQ_ADDR     = (13 << 3),
    USBIEN_ADDR     = (14 << 3),
    USBCTL_ADDR     = (15 << 3),
    CPUCTL_ADDR     = (16 << 3),
    PINCTL_ADDR     = (17 << 3),
    REVISION_ADDR   = (18 << 3),
    FNADDR_ADDR     = (19 << 3),
    MODE_ADDR       = (27 << 3),
};

enum {
    EPSTALLS_ACKSTAT    = 0x40,
    EPSTALLS_STLSTAT    = 0x20,
    EPSTALLS_STLEP3IN   = 0x10,
    EPSTALLS_STLEP2IN   = 0x08,
    EPSTALLS_STLEP1OUT  = 0x04,
    EPSTALLS_STLEP0OUT  = 0x02,
    EPSTALLS_STLEP0IN   = 0x01,
};

enum {
    CLRTOGS_EP3DISAB   = 0x80,
    CLRTOGS_EP2DISAB   = 0x40,
    CLRTOGS_EP1DISAB   = 0x20,
    CLRTOGS_CTGEP3IN   = 0x10,
    CLRTOGS_CTGEP2IN   = 0x08,
    CLRTOGS_CTGEP1OUT  = 0x04,
};

enum {
    EPIRQ_SUDAVIRQ      = 0x20,
    EPIRQ_IN3BAVIRQ     = 0x10,
    EPIRQ_IN2BAVIRQ     = 0x08,
    EPIRQ_OUT1DAVIRQ    = 0x04,
    EPIRQ_OUT0DAVIRQ    = 0x02,
    EPIRQ_IN0BAVIRQ     = 0x01,
};

enum {
    EPIEN_SUDAVIE       = 0x20,
    EPIEN_IN3BAVIE      = 0x10,
    EPIEN_IN2BAVIE      = 0x08,
    EPIEN_OUT1DAVIE     = 0x04,
    EPIEN_OUT0DAVIE     = 0x02,
    EPIEN_IN0BAVIE      = 0x01,
};

enum {
    USBIRQ_URESDNIRQ    = 0x80,
    USBIRQ_VBUSIRQ      = 0x40,
    USBIRQ_NOVBUSIRQ    = 0x20,
    USBIRQ_SUSPIRQ      = 0x10,
    USBIRQ_URESIRQ      = 0x08,
    USBIRQ_BUSACTIRQ    = 0x04,
    USBIRQ_RWUDNIRQ     = 0x02,
    USBIRQ_OSCOKIRQ     = 0x01,
};

enum {
    USBIEN_URESDNIE     = 0x80,
    USBIEN_VBUSIE       = 0x40,
    USBIEN_NOVBUSIE     = 0x20,
    USBIEN_SUSPIE       = 0x10,
    USBIEN_URESIE       = 0x08,
    USBIEN_BUSACTIE     = 0x04,
    USBIEN_RWUDNIE      = 0x02,
    USBIEN_OSCOKIE      = 0x01,
};

enum {
    USBCTL_HOSCSTEN     = 0x80,
    USBCTL_VBGATE       = 0x40,
    USBCTL_CHIPRES      = 0x20,
    USBCTL_PWRDOWN      = 0x10,
    USBCTL_CONNECT      = 0x08,
    USBCTL_SIGRWU       = 0x04,
};

enum {
    CPUCTL_IE           = 0x01,
    CPUCTL_PULSEWID1    = 0x80,
    CPUCTL_PULSEWID0    = 0x40,
};

enum {
    PINCTL_EP3INAK      = 0x80,
    PINCTL_EP2INAK      = 0x40,
    PINCTL_EP1INAK      = 0x20,
    PINCTL_FDUPSPI      = 0x10,
    PINCTL_INTLEVEL     = 0x08,
    PINCTL_POSINT       = 0x04,
    PINCTL_GPXB         = 0x02,
    PINCTL_GPXA         = 0x01,
};

enum {
    GPX_OPERATE         = 0x00,
    GPX_VBDET           = 0x01,
    GPX_BUSACT          = 0x02,
    GPX_SOF             = 0x03,
};

enum {
    MODE_SEPIRQ        = 0x10,
    MODE_DMPULLDN      = 0x40,
    MODE_DPPULLDN      = 0x80,
};

enum {
    CMDBYTE_WRITE = 0x02,
};

// MAX3421E in peripheral mode supports
// - 1 control endpoint
// - 2 in endpoints
// - 1 out endpoint
// We could do some kind of software multiplexing 
// to support more, but for now we'll do the 4
// #define CFG_TUD_MAX3421_ENDPOINT_TOTAL 4

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

typedef struct TU_ATTR_PACKED {
    uint8_t ep_num   : 4;
    uint8_t is_setup : 1;
    uint8_t is_out   : 1;
    uint8_t is_iso   : 1;
} dxfr_bm_t;

typedef struct {
    uint8_t alloc;

    uint8_t ep_bc_addr;
    uint8_t ep_fifo_addr;
    uint8_t ep_irq;
    uint8_t ep_stall;
    uint16_t ep_size;

    atomic_flag busy;
    dxfr_bm_t dxfr;

    uint16_t total_len;
    uint16_t xferred_len;
    uint8_t* buf;
} max3421_ep_t;

typedef struct {
    uint8_t daddr;

#if OSAL_MUTEX_REQUIRED
    OSAL_MUTEX_DEF(spi_mutexdef);
    osal_mutex_t spi_mutex;
#endif

    max3421_ep_t ep[CFG_TUD_MAX3421_ENDPOINT_TOTAL]; // [0] is reserved for addr0
} max3421_data_t;

static max3421_data_t _dcd_data;

extern void tud_max3421_int_api(uint8_t rhport, bool enabled);
extern void tud_max3421_spi_cs_api(uint8_t rhport, bool active);
extern bool tud_max3421_spi_xfer_api(uint8_t rhport, uint8_t const* tx_buf, uint8_t* rx_buf, size_t xfer_bytes);

//============================================================================
// EP Helpers
//============================================================================

static inline void init_endpoints() {
    tu_memclr(&_dcd_data.ep, sizeof(_dcd_data.ep));
    _dcd_data.ep[0].ep_fifo_addr = EP0FIFO_ADDR;
    _dcd_data.ep[0].ep_bc_addr = EP0BC_ADDR;
    _dcd_data.ep[0].ep_irq = (EPIRQ_OUT0DAVIRQ | EPIRQ_IN0BAVIRQ | EPIRQ_SUDAVIRQ);

    _dcd_data.ep[1].ep_fifo_addr = EP1OUTFIFO_ADDR;
    _dcd_data.ep[1].ep_bc_addr = EP1OUTBC_ADDR;
    _dcd_data.ep[1].ep_stall = EPSTALLS_STLEP1OUT;
    _dcd_data.ep[1].ep_irq = EPIRQ_OUT1DAVIRQ;
    _dcd_data.ep[1].dxfr.is_out = 1;

    _dcd_data.ep[2].ep_fifo_addr = EP2INFIFO_ADDR;
    _dcd_data.ep[2].ep_bc_addr = EP2INBC_ADDR;
    _dcd_data.ep[2].ep_stall = EPSTALLS_STLEP2IN;
    _dcd_data.ep[2].ep_irq = EPIRQ_IN2BAVIRQ;

    _dcd_data.ep[3].ep_fifo_addr = EP3INFIFO_ADDR;
    _dcd_data.ep[3].ep_bc_addr = EP3INBC_ADDR;
    _dcd_data.ep[3].ep_stall = EPSTALLS_STLEP3IN;   
    _dcd_data.ep[3].ep_irq = EPIRQ_IN3BAVIRQ;
}

static inline max3421_ep_t* allocate_ep(uint8_t daddr, tusb_desc_endpoint_t const * desc_ep) {
    const uint8_t is_out = (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT) ? 1 : 0;
    for (uint8_t i = 1; i < ARRAY_SIZE(_dcd_data.ep); i++) {
        max3421_ep_t* ep = &_dcd_data.ep[i];
        if (!ep->alloc && (ep->dxfr.is_out == is_out)) {
            ep->alloc = 1;
            ep->dxfr.ep_num = tu_edpt_number(desc_ep->bEndpointAddress);
            ep->dxfr.is_iso = (desc_ep->bmAttributes.xfer == TUSB_XFER_ISOCHRONOUS) ? 1 : 0;
            ep->ep_size = desc_ep->wMaxPacketSize;
            return ep;
        }
    }
    return NULL;
}

static inline max3421_ep_t* get_ep_by_epaddr(uint8_t epaddr) {
    const uint8_t ep_num = tu_edpt_number(epaddr);
    if (ep_num == 0) {
        return &_dcd_data.ep[0];
    }
    const uint8_t is_out = (tu_edpt_dir(epaddr) == TUSB_DIR_OUT) ? 1 : 0;
    for (uint8_t i = 1; i < ARRAY_SIZE(_dcd_data.ep); i++) {
        max3421_ep_t* ep = &_dcd_data.ep[i];
        if (ep->alloc && (ep->dxfr.ep_num == ep_num) && (ep->dxfr.is_out == is_out)) {
            return ep;
        }
    }
    return NULL;
}

static inline max3421_ep_t* get_ep_by_irq(uint8_t irq) {
    for (uint8_t i = 0; i < ARRAY_SIZE(_dcd_data.ep); i++) {
        max3421_ep_t* ep = &_dcd_data.ep[i];
        if (ep->alloc && (ep->ep_irq & irq)) {
            return ep;
        }
    }
    return NULL;
}

//============================================================================
// Low-level hardware access functions
//============================================================================

static void max3421_spi_lock(uint8_t rhport, bool in_isr) {
    // disable interrupt and mutex lock (for pre-emptive RTOS) if not in_isr
    if (!in_isr) {
        (void) osal_mutex_lock(_dcd_data.spi_mutex, OSAL_TIMEOUT_WAIT_FOREVER);
        tud_max3421_int_api(rhport, false);
    }
    // assert CS
    tud_max3421_spi_cs_api(rhport, true);
}

static void max3421_spi_unlock(uint8_t rhport, bool in_isr) {
    // de-assert CS
    tud_max3421_spi_cs_api(rhport, false);

    // mutex unlock and re-enable interrupt
    if (!in_isr) {
        tud_max3421_int_api(rhport, true);
        (void) osal_mutex_unlock(_dcd_data.spi_mutex);
    }
}

uint8_t tud_max3421_reg_read(uint8_t rhport, uint8_t reg, bool in_isr) {
    uint8_t tx_buf[2] = {reg, 0};
    uint8_t rx_buf[2] = {0, 0};

    max3421_spi_lock(rhport, in_isr);
    bool ret = tud_max3421_spi_xfer_api(rhport, tx_buf, rx_buf, 2);
    max3421_spi_unlock(rhport, in_isr);

    // _dcd_data.usbirq = rx_buf[0];
    return ret ? rx_buf[1] : 0;
}

bool tud_max3421_reg_write(uint8_t rhport, uint8_t reg, uint8_t data, bool in_isr) {
    uint8_t tx_buf[2] = {reg | CMDBYTE_WRITE, data};
    uint8_t rx_buf[2] = {0, 0};

    max3421_spi_lock(rhport, in_isr);
    bool ret = tud_max3421_spi_xfer_api(rhport, tx_buf, rx_buf, 2);
    max3421_spi_unlock(rhport, in_isr);

    // HIRQ register since we are in full-duplex mode
    // _dcd_data.usbirq = rx_buf[0];

    return ret;
}

static void hwfifo_write(uint8_t rhport, uint8_t reg, const uint8_t* buffer, uint8_t len, bool in_isr) {
    uint8_t usbirq;
    reg |= CMDBYTE_WRITE;

    max3421_spi_lock(rhport, in_isr);
    tud_max3421_spi_xfer_api(rhport, &reg, &usbirq, 1);
    // _dcd_data.usbirq = usbirq;
    tud_max3421_spi_xfer_api(rhport, buffer, NULL, len);
    max3421_spi_unlock(rhport, in_isr);
}

static void hwfifo_read(uint8_t rhport, uint8_t reg, uint8_t* buffer, uint8_t len, bool in_isr) {
    uint8_t usbirq;
    max3421_spi_lock(rhport, in_isr);
    tud_max3421_spi_xfer_api(rhport, &reg, &usbirq, 1);
    // _dcd_data.usbirq = usbirq;
    tud_max3421_spi_xfer_api(rhport, NULL, buffer, len);
    max3421_spi_unlock(rhport, in_isr);
}

//============================================================================
// DCD API Implementation for MAX3421 Device Mode
//============================================================================

static void ep_int_handler(uint8_t rhport, uint8_t epirq);

bool dcd_init(uint8_t rhport, const tusb_rhport_init_t* rh_init) {
    (void) rh_init;
    
    tud_max3421_int_api(rhport, false);

    tu_memclr(&_dcd_data, sizeof(_dcd_data));
    init_endpoints();

#if OSAL_MUTEX_REQUIRED
    _dcd_data.spi_mutex = osal_mutex_create(&_dcd_data.spi_mutexdef);
#endif

    // full duplex, interrupt negative edge
    tud_max3421_reg_write(rhport, PINCTL_ADDR, PINCTL_FDUPSPI, false);

    // v1 is 0x01, v2 is 0x12, v3 is 0x13
    // Note: v1 and v2 has host OUT errata whose workaround is not implemented in this driver
    const uint8_t revision = tud_max3421_reg_read(rhport, REVISION_ADDR, false);
    TU_LOG2_HEX(revision);
    TU_ASSERT(revision == 0x01 || revision == 0x12 || revision == 0x13, false);

    // reset
    tud_max3421_reg_write(rhport, USBCTL_ADDR, USBCTL_CHIPRES, false);
    tud_max3421_reg_write(rhport, USBCTL_ADDR, 0, false);

    while (!(tud_max3421_reg_read(rhport, USBIRQ_ADDR, false) & USBIRQ_OSCOKIRQ)) {
        // wait for oscillator to stabilize
    }

    // Force the chip into peripheral (device) mode.
    // For example, clear the chip reset and then set the CONNECT bit to enable the pull-up.
    tud_max3421_reg_write(rhport, MODE_ADDR, 0, false);

    // Enable device-level interrupts
    uint8_t usbien_mask = USBIEN_URESDNIE | USBIEN_BUSACTIE | USBIEN_SUSPIE | USBIEN_URESIE;
    tud_max3421_reg_write(rhport, USBIEN_ADDR, usbien_mask, false);
    // For EP0, always enable the SETUP/data available interrupt
    tud_max3421_reg_write(rhport, EPIEN_ADDR, EPIEN_SUDAVIE, false);

    tud_max3421_int_api(rhport, true);

    // Clear function address (rFNADDR) to 0.
    tud_max3421_reg_write(rhport, CPUCTL_ADDR, CPUCTL_IE, false);

    return true;
}

// Deinitialize controller, unset device mode.
bool dcd_deinit(uint8_t rhport) {
    (void) rhport;
    // disable interrupt
    tud_max3421_int_api(rhport, false);

    // reset max3421 and power down
    tud_max3421_reg_write(rhport, USBCTL_ADDR, USBCTL_CHIPRES, false);
    tud_max3421_reg_write(rhport, USBCTL_ADDR, USBCTL_PWRDOWN, false);

#if OSAL_MUTEX_REQUIRED
    osal_mutex_delete(_dcd_data.spi_mutex);
    _dcd_data.spi_mutex = NULL;
#endif

    return true;
}

// Interrupt Handler
void dcd_int_handler(uint8_t rhport) {
    uint8_t usbirq = tud_max3421_reg_read(rhport, USBIRQ_ADDR, true);
    if (usbirq) {
        if (usbirq & USBIRQ_URESDNIRQ) {
            dcd_event_bus_signal(rhport, DCD_EVENT_BUS_RESET, true);
        }
        if (usbirq & USBIRQ_SUSPIRQ) {
            dcd_event_bus_signal(rhport, DCD_EVENT_SUSPEND, true);
        }
        if (usbirq & USBIRQ_BUSACTIRQ) {
            dcd_event_bus_signal(rhport, DCD_EVENT_RESUME, true);
        }
        if (usbirq & USBIRQ_URESIRQ) {
            dcd_event_bus_signal(rhport, DCD_EVENT_UNPLUGGED, true);
        }
        tud_max3421_reg_write(rhport, USBIRQ_ADDR, usbirq, true);
    }

    uint8_t epirq = tud_max3421_reg_read(rhport, EPIRQ_ADDR, true);
    if (epirq) {
        ep_int_handler(rhport, epirq);
    }
}

// Enable device interrupt
void dcd_int_enable (uint8_t rhport) {
    tud_max3421_int_api(rhport, true);
}

// Disable device interrupt
void dcd_int_disable(uint8_t rhport) {
    tud_max3421_int_api(rhport, false);
}

// Receive Set Address request, mcu port must also include status IN response
void dcd_set_address(uint8_t rhport, uint8_t dev_addr) {
    tud_max3421_reg_write(rhport, FNADDR_ADDR, dev_addr, false);
    // According to USB spec, a zero-length packet on EP0 IN is sent as status.
    dcd_edpt_xfer(rhport, tu_edpt_addr(0, TUSB_DIR_IN), NULL, 0);
}

// Wake up host
void dcd_remote_wakeup(uint8_t rhport) {
    uint8_t usbctl = tud_max3421_reg_read(rhport, USBCTL_ADDR, false);
    usbctl |= USBCTL_SIGRWU;
    tud_max3421_reg_write(rhport, USBCTL_ADDR, usbctl, false);
}

// Connect by enabling internal pull-up resistor on D+/D-
void dcd_connect(uint8_t rhport) {
    uint8_t usbctl = tud_max3421_reg_read(rhport, USBCTL_ADDR, false);
    usbctl |= USBCTL_CONNECT;
    tud_max3421_reg_write(rhport, USBCTL_ADDR, usbctl, false);
}

// Disconnect by disabling internal pull-up resistor on D+/D-
void dcd_disconnect(uint8_t rhport) {
    tud_max3421_reg_write(rhport, USBCTL_ADDR, 0, false);
}

// Enable/Disable Start-of-frame interrupt. Default is disabled
void dcd_sof_enable(uint8_t rhport, bool en) {
    // MAX3421E does not support SOF interrupt in peripheral mode, emulate?
    (void) rhport;
    (void) en;
}

//--------------------------------------------------------------------+
// Endpoint API
//--------------------------------------------------------------------+

// Invoked when a control transfer's status stage is complete.
// May help DCD to prepare for next control transfer, this API is optional.
// void dcd_edpt0_status_complete(uint8_t rhport, tusb_control_request_t const * request) {

// }

// Configure endpoint's registers according to descriptor
bool dcd_edpt_open(uint8_t rhport, tusb_desc_endpoint_t const * desc_ep) {
    // Find available endpoint
    return (allocate_ep(_dcd_data.daddr, desc_ep) != NULL);
}

// Close all non-control endpoints, cancel all pending transfers if any.
// Invoked when switching from a non-zero Configuration by SET_CONFIGURE therefore
// required for multiple configuration support.
void dcd_edpt_close_all(uint8_t rhport) {
    for (uint8_t i = 1; i < ARRAY_SIZE(_dcd_data.ep); i++) {
        max3421_ep_t* ep = &_dcd_data.ep[i];
        if (ep->alloc) {
            uint8_t epaddr = tu_edpt_addr(ep->dxfr.ep_num, 
                                          ep->dxfr.is_out ? TUSB_DIR_OUT : TUSB_DIR_IN);
            dcd_edpt_close(rhport, epaddr);
        }
    }
}

static inline void flush_out_fifo(uint8_t rhport, max3421_ep_t* ep, uint8_t len) {
    TU_VERIFY(len && ep->dxfr.is_out, );
    uint8_t buf[16] = {0};
    while (len) {
        uint8_t read_len = tu_min8(len, sizeof(buf));
        hwfifo_read(rhport, ep->ep_fifo_addr, buf, read_len, true);
        len -= read_len;
    }
}

static void ep_int_handler(uint8_t rhport, uint8_t epirq) {
    uint8_t available_len = 0;
    uint16_t chunk_len = 0;
    uint8_t new_epien = EPIRQ_SUDAVIRQ;
    max3421_ep_t* ep = NULL;
    uint8_t epirq_check = epirq & (EPIRQ_SUDAVIRQ | EPIRQ_IN0BAVIRQ | EPIRQ_OUT0DAVIRQ);

    if (epirq_check && ((ep = get_ep_by_irq(epirq_check)) != NULL)) { // Control EP0
        if (epirq & EPIRQ_SUDAVIRQ) {
            uint8_t setup[sizeof(tusb_control_request_t)] = {0};
            hwfifo_read(rhport, SUDFIFO_ADDR, setup, sizeof(setup), true);
            atomic_flag_clear(&ep->busy);
            dcd_event_setup_received(rhport, setup, true);
        }
        if (epirq & EPIRQ_IN0BAVIRQ) {
            // EP0 IN
            available_len = tud_max3421_reg_read(rhport, EP0BC_ADDR, true);
            chunk_len = tu_min16(ep->total_len - ep->xferred_len, available_len);
            hwfifo_write(rhport, EP0FIFO_ADDR, &ep->buf[ep->xferred_len], chunk_len, true);
            ep->xferred_len += chunk_len;

            if (ep->xferred_len >= ep->total_len) {
                atomic_flag_clear(&ep->busy);
                dcd_event_xfer_complete(rhport, tu_edpt_addr(0, TUSB_DIR_IN), 
                                        ep->xferred_len, XFER_RESULT_SUCCESS, true);
            } else {
                new_epien |= EPIRQ_IN0BAVIRQ;
            }
        }
        if (epirq & EPIRQ_OUT0DAVIRQ) {
            // EP0 OUT
            available_len = tud_max3421_reg_read(rhport, EP0BC_ADDR, true);
            chunk_len = tu_min16(ep->total_len - ep->xferred_len, available_len);
            hwfifo_read(rhport, EP0FIFO_ADDR, &ep->buf[ep->xferred_len], chunk_len, true);
            ep->xferred_len += chunk_len;

            if (ep->xferred_len >= ep->total_len) {
                if (available_len > chunk_len) {
                    flush_out_fifo(rhport, ep, available_len - chunk_len);
                }
                atomic_flag_clear(&ep->busy);
                dcd_event_xfer_complete(rhport, tu_edpt_addr(0, TUSB_DIR_OUT),
                                        ep->xferred_len, XFER_RESULT_SUCCESS, true);
            } else if (ep->ep_size > available_len) {
                // Short packet
                atomic_flag_clear(&ep->busy);
                dcd_event_xfer_complete(rhport, tu_edpt_addr(0, TUSB_DIR_OUT),
                                        ep->xferred_len, XFER_RESULT_SUCCESS, true);
            } else {
                new_epien |= EPIRQ_OUT0DAVIRQ;
            }
        }
    }

    epirq_check = epirq & ~(EPIRQ_SUDAVIRQ | EPIRQ_IN0BAVIRQ | EPIRQ_OUT0DAVIRQ);
    if (epirq_check) {
        while ((ep = get_ep_by_irq(epirq_check)) != NULL) {
            if (ep->dxfr.is_out) {
                available_len = tud_max3421_reg_read(rhport, ep->ep_bc_addr, true);
                chunk_len = tu_min16(ep->total_len - ep->xferred_len, available_len);
                hwfifo_read(rhport, ep->ep_fifo_addr, &ep->buf[ep->xferred_len], chunk_len, true);
                ep->xferred_len += chunk_len;

                if (ep->xferred_len >= ep->total_len) {
                    if (available_len > chunk_len) {
                        flush_out_fifo(rhport, ep, available_len - chunk_len);
                    }
                    atomic_flag_clear(&ep->busy);
                    dcd_event_xfer_complete(rhport, tu_edpt_addr(ep->dxfr.ep_num, TUSB_DIR_OUT), 
                                            ep->xferred_len, XFER_RESULT_SUCCESS, true);
                } else if (ep->ep_size > available_len) {
                    // Short packet
                    atomic_flag_clear(&ep->busy);
                    dcd_event_xfer_complete(rhport, tu_edpt_addr(ep->dxfr.ep_num, TUSB_DIR_OUT), 
                                            ep->xferred_len, XFER_RESULT_SUCCESS, true);
                } else {
                    new_epien |= ep->ep_irq;
                }
            } else {
                available_len = tud_max3421_reg_read(rhport, ep->ep_bc_addr, true);
                chunk_len = tu_min16(ep->total_len - ep->xferred_len, available_len);
                hwfifo_write(rhport, ep->ep_fifo_addr, &ep->buf[ep->xferred_len], chunk_len, true);
                ep->xferred_len += chunk_len;

                if (ep->xferred_len >= ep->total_len) {
                    atomic_flag_clear(&ep->busy);
                    dcd_event_xfer_complete(rhport, tu_edpt_addr(ep->dxfr.ep_num, TUSB_DIR_IN), 
                                            ep->xferred_len, XFER_RESULT_SUCCESS, true);
                } else {
                    new_epien |= ep->ep_irq;
                }
            }
            epirq_check &= ~ep->ep_irq;
        }
    }
    tud_max3421_reg_write(rhport, EPIEN_ADDR, new_epien, true);
    tud_max3421_reg_write(rhport, EPIRQ_ADDR, epirq, true);
}

// Submit a transfer, When complete dcd_event_xfer_complete() is invoked to notify the stack
bool dcd_edpt_xfer(uint8_t rhport, uint8_t ep_addr, uint8_t * buffer, uint16_t total_bytes) {
    max3421_ep_t* ep = NULL;
    uint8_t epien = tud_max3421_reg_read(rhport, EPIEN_ADDR, false);
    if (tu_edpt_number(ep_addr) == 0) {
        // Control xfer
        ep = &_dcd_data.ep[0];
        TU_VERIFY(!atomic_flag_test_and_set(&ep->busy), false);

        ep->buf = buffer;
        ep->total_len = total_bytes;
        ep->xferred_len = 0;
        // Enable endpoint interrupt
        epien |= (ep->dxfr.is_out ? EPIEN_OUT0DAVIE : EPIEN_IN0BAVIE);

    } else {
        ep = get_ep_by_epaddr(ep_addr);
        TU_ASSERT(ep, false); 
        TU_VERIFY(!atomic_flag_test_and_set(&ep->busy), false);

        ep->total_len = total_bytes;
        ep->xferred_len = 0;
        ep->buf = buffer;
        // Enable endpoint interrupt
        epien |= ep->ep_irq;
    }
    tud_max3421_reg_write(rhport, EPIEN_ADDR, epien, false);
    return true;
}

// Submit an transfer using fifo, When complete dcd_event_xfer_complete() is invoked to notify the stack
// This API is optional, may be useful for register-based for transferring data.
// bool dcd_edpt_xfer_fifo(uint8_t rhport, uint8_t ep_addr, tu_fifo_t * ff, uint16_t total_bytes) {

// }

// Stall endpoint, any queuing transfer should be removed from endpoint
void dcd_edpt_stall(uint8_t rhport, uint8_t ep_addr) {
    max3421_ep_t* ep = get_ep_by_epaddr(ep_addr);
    TU_ASSERT(ep, );
    atomic_flag_clear(&ep->busy);
    uint8_t epstalls = 0;
    if (ep->ep_fifo_addr == EP0FIFO_ADDR) {
        epstalls = (ep->dxfr.is_out) ? EPSTALLS_STLEP0OUT : EPSTALLS_STLEP0IN;
    } else {
        epstalls = ep->ep_stall;
    }
    tud_max3421_reg_write(rhport, EPSTALLS_ADDR, epstalls, false);
}

// clear stall, data toggle is also reset to DATA0
// This API never calls with control endpoints, since it is auto cleared when receiving setup packet
void dcd_edpt_clear_stall(uint8_t rhport, uint8_t ep_addr) {
    max3421_ep_t* ep = get_ep_by_epaddr(ep_addr);
    TU_ASSERT(ep, );
    uint8_t epstalls = tud_max3421_reg_read(rhport, EPSTALLS_ADDR, false);
    if (ep->ep_fifo_addr == EP0FIFO_ADDR) {
        epstalls &= (tu_edpt_dir(ep_addr) == TUSB_DIR_OUT) ? ~EPSTALLS_STLEP0OUT : ~EPSTALLS_STLEP0IN;
    } else {
        epstalls &= ~ep->ep_stall;
    }
    tud_max3421_reg_write(rhport, EPSTALLS_ADDR, epstalls, false);
}

void dcd_edpt_close(uint8_t rhport, uint8_t ep_addr) {
    tud_max3421_int_api(rhport, false);
    max3421_ep_t* ep = get_ep_by_epaddr(ep_addr);
    TU_ASSERT(ep, );
    bool abort = atomic_flag_test_and_set(&ep->busy);
    atomic_flag_clear(&ep->busy);
    ep->total_len = 0;
    ep->xferred_len = 0;
    ep->buf = NULL;
    ep->alloc = 0;
    // Clear endpoint interrupt
    uint8_t epien = tud_max3421_reg_read(rhport, EPIEN_ADDR, false);
    uint8_t epirq = tud_max3421_reg_read(rhport, EPIRQ_ADDR, false);
    epien &= ~ep->ep_irq;
    epirq &= ~ep->ep_irq;
    tud_max3421_reg_write(rhport, EPIRQ_ADDR, epirq, false);
    tud_max3421_reg_write(rhport, EPIEN_ADDR, epien, false);
    tud_max3421_int_api(rhport, true);
    if (abort) {
        dcd_event_xfer_complete(rhport, ep_addr, 0, XFER_RESULT_FAILED, false);
    }
}