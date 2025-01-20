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

#define PIN_SD 7
#define PIN_PWM1 4
#define PIN_CTR1_A 5
#define PIN_CTR1_B 6
#define PIN_PWM2 14
#define PIN_CTR2_A 15
#define PIN_CTR2_B 16

#define PIN_HALL1 10
#define PIN_HALL2 12
#define HALL_UNIT 11.0

#define I2C_2_SCL 9
#define I2C_2_SDA 8
#endif