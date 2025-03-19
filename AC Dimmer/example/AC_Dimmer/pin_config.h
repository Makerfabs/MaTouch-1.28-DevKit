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

#define TXT_BLACK lv_color_hex(0x000000)
#define TXT_RED lv_color_hex(0xFF0000)

#define SCR_Pin 6
#define ZCD_PIN 4
#define RELAY_PIN 5

#define AC_CTRL_OFF digitalWrite(SCR_Pin, LOW)
#define AC_CTRL_ON digitalWrite(SCR_Pin, HIGH)

#define RELAY_OFF digitalWrite(RELAY_PIN, LOW)
#define RELAY_ON digitalWrite(RELAY_PIN, HIGH)

#endif