/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0
DHT sensor library v1.4.6
Adafruit Unified Sensor v1.1.14

Tools:
USB CDC On Boot: Enabled
*/

#include <Arduino_GFX_Library.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>

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

const char* ssid     = "WIFI SSID"; // Change this to your WiFi SSID
const char* password = "WIFI PIN"; // Change this to your WiFi password

const char* host = "api.thingspeak.com"; // This should not be changed
const String writeApiKey = "Your API key"; // Change this to your Write API key

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

void setup()
{
  // put your setup code here, to run once:
    Serial.begin(115200);
    dht.begin();

    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);
    
    gfx->begin();
    gfx->fillScreen(WHITE);
    gfx->setTextSize(2);
    gfx->setTextColor(BLACK);
    gfx->setCursor(60, 50);
    gfx->println(F("HTTP Demo"));
    gfx->setCursor(20, 95);
    gfx->println(F("Temperature: "));
    gfx->setCursor(20, 135);
    gfx->println(F("Humidity: "));

    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    h = dht.readHumidity();
    t = dht.readTemperature();

    String url = "http://" + String(host) + "/update";
           url += "?api_key=" + writeApiKey;
           url += "&field1=" + String(t);
           url += "&field2=" + String(h);

    http.begin(url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String payload = http.getString();
      Serial.println("Response: " + payload);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      Serial.println("Error: " + http.errorToString(httpResponseCode));
    }

    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }

  gfx->fillRect(165, 90, 70, 30, WHITE);
  gfx->setCursor(170, 95);
  gfx->print(t);
  gfx->fillRect(130, 130, 70, 30, WHITE);
  gfx->setCursor(135, 135);
  gfx->print(h);

  delay(60000);
}
