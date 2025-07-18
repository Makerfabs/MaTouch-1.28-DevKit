/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0
DHT sensor library v1.4.6
Adafruit Unified Sensor v1.1.14
*/

#include <Arduino_GFX_Library.h>
#include <DHT.h>

#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float h,t;

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
    Serial.println("start");
    dht.begin();
    gfx->begin();
    gfx->fillScreen(WHITE);
    gfx->setTextSize(2);
    gfx->setTextColor(BLACK);
    gfx->setCursor(60, 50);
    gfx->println(F("DHT11 Demo"));
    gfx->setCursor(20, 95);
    gfx->println(F("Temperature: "));
    gfx->setCursor(20, 135);
    gfx->println(F("Humidity: "));
}

void loop()
{
  h = dht.readHumidity();
  t = dht.readTemperature();
  gfx->fillRect(165, 90, 70, 30, WHITE);
  gfx->setCursor(170, 95);
  gfx->print(t);
  gfx->fillRect(130, 130, 70, 30, WHITE);
  gfx->setCursor(135, 135);
  gfx->print(h);
  delay(2000);
}
