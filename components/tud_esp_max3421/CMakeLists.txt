set(srcs 
    "src/tusb.c"
    "src/device/usbd.c"
    # "src/device/dcd.c"
    "src/device/usbd_control.c"
    "src/class/hid/hid_device.c"
    "src/common/tusb_fifo.c"
    # "bsp/board_api.c"
    "bsp/family.c"
    "src/portable/analog/max3421/dcd_max3421.c"
)

idf_component_register(
    SRCS "${srcs}"
    PRIV_INCLUDE_DIRS "src/." "bsp/." "."
    REQUIRES esp_driver_spi esp_driver_gpio esp_driver_uart
)