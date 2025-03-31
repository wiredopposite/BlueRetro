#ifndef BOARD_H_
#define BOARD_H_

#ifdef __cplusplus
 extern "C" {
#endif

// #define NEOPIXEL_PIN          8

#define BUTTON_PIN            9
#define BUTTON_STATE_ACTIVE   0

// SPI for USB host shield
#define MAX3421_SPI_HOST SPI2_HOST
#define MAX3421_SCK_PIN  4
#define MAX3421_MOSI_PIN 6
#define MAX3421_MISO_PIN 5
#define MAX3421_CS_PIN   10
#define MAX3421_INTR_PIN 7

// SPI for USB peripheral
#define MAX3421_DEVICE_SPI      SPI1_HOST
#define MAX3421_DEVICE_SCK_PIN  23
#define MAX3421_DEVICE_MOSI_PIN 19
#define MAX3421_DEVICE_MISO_PIN 18
#define MAX3421_DEVICE_CS_PIN   5
#define MAX3421_DEVICE_INTR_PIN 8

#ifdef __cplusplus
 }
#endif

#endif /* BOARD_H_ */