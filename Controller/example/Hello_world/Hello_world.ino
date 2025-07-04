/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0

Tools:
USB CDC On Boot: Enabled
*/

#include <Arduino_GFX_Library.h>

#define TFT_BLK 45
#define TFT_RES 21
#define TFT_CS 1
#define TFT_MOSI 2
#define TFT_MISO -1
#define TFT_SCLK 42
#define TFT_DC 46

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

void setup()
{
  // put your setup code here, to run once:
    Serial.begin(115200);

    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    delay(1000);
    Serial.println("Start to color test");

    gfx->begin();
    
    gfx->fillScreen(RED);
    delay(1000);
    gfx->fillScreen(GREEN);
    delay(1000);
    gfx->fillScreen(BLUE);
    delay(1000);
    gfx->fillScreen(BLACK);
    delay(1000);
    gfx->fillScreen(WHITE);
    delay(1000);

    gfx->setTextSize(3);
    gfx->setTextColor(BLACK);
    gfx->setCursor(20, 100);
    gfx->println(F("Hello world"));
}

void loop()
{
}
