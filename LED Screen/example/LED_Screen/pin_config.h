#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

#define TFT_BLK 45
#define TFT_RES 21

#define TFT_CS 1
#define TFT_MOSI 2
#define TFT_MISO -1
#define TFT_SCLK 42
#define TFT_DC 46

#define TOUCH_INT 40
#define TOUCH_SDA 38
#define TOUCH_SCL 39
#define TOUCH_RST 18

#define BUTTON_PIN 17
#define ENCODER_CLK 48 // CLK
#define ENCODER_DT 47  // DT

#define MOTOR_PIN 41

#define SD_SCK 42
#define SD_MISO 18
#define SD_MOSI 2
#define SD_CS 43

#define SPI_ON_TFT  digitalWrite(TFT_CS, LOW)
#define SPI_OFF_TFT digitalWrite(TFT_CS, HIGH)
#define SPI_ON_SD   digitalWrite(SD_CS, LOW)
#define SPI_OFF_SD  digitalWrite(SD_CS, HIGH)

#define TXT_BLACK lv_color_hex(0x000000)
#define TXT_RED lv_color_hex(0xFF0000)


#endif