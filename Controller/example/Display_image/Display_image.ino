/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0

Tools:
USB CDC On Boot: Enabled
*/

#include <Arduino_GFX_Library.h>
#include "Image.h"

#define TFT_BLK 45
#define TFT_RES 21

#define TFT_CS 1
#define TFT_MOSI 2
#define TFT_MISO -1
#define TFT_SCLK 42
#define TFT_DC 46

const int freq = 2000;
const int resolution = 8;

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

void setup()
{
  // put your setup code here, to run once:
    Serial.begin(115200);

    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    delay(1000);
    Serial.println("start");

    gfx->begin();
    gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)Image, 240, 240);

    ledcAttach(TFT_BLK, freq, resolution);
}

void loop()
{
  // put your main code here, to run repeatedly:
    // Setup and start fade on led (duty from 0 to 255)
    ledcFade(TFT_BLK, 0, 255, 2000);
    // Wait for fade to end
    delay(2000);
    ledcFade(TFT_BLK, 255, 0, 2000);
    delay(2000);
}
