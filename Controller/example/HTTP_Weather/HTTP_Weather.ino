/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0
ArduinoJson v7.2.0

Tools:
USB CDC On Boot: Enabled
*/

#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define TFT_BLK 45
#define TFT_RES 21
#define TFT_CS 1
#define TFT_MOSI 2
#define TFT_MISO -1
#define TFT_SCLK 42
#define TFT_DC 46

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

const char* ssid = "Your SSID";
const char* password = "Your PIN";

const String apiKey = "Your OpenWeatherMap API";
const String city = "Beijing";
const String countryCode = "CN";
const String units = "metric";

String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=" + units + "&appid=" + apiKey;

void setup()
{
  Serial.begin(115200);
  
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  gfx->begin();
  gfx->fillScreen(WHITE);
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(15, 100);
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  gfx->println(F("Connecting to WiFi"));

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  
  gfx->fillScreen(WHITE);
  gfx->setCursor(65, 100);
  gfx->print(F("Connected"));

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  getWeatherData();
  delay(60000);
}

void getWeatherData()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    
    http.begin(serverPath.c_str());
    
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      
      String payload = http.getString();
      Serial.println(payload);
      
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, payload);
      
      JsonObject weather = doc["weather"][0];
      JsonObject main = doc["main"];
      JsonObject wind = doc["wind"];

      String weatherMain = weather["main"];
      String weatherDesc = weather["description"];
      float temp = main["temp"];
      float feelsLike = main["feels_like"];
      int humidity = main["humidity"];
      float windSpeed = wind["speed"];

      gfx->fillScreen(WHITE);
      gfx->setCursor(20, 70);
      gfx->print(F("City:"));
      gfx->println(city);
      gfx->setCursor(20, 100);
      gfx->print(F("Temp:"));
      gfx->print(temp);
      gfx->println("C");
      gfx->setCursor(20, 125);
      gfx->print(F("Humi:"));
      gfx->print(humidity);
      gfx->println(F("%"));
      gfx->setCursor(20, 150);
      gfx->print(F("WindSpeed:"));
      gfx->print(windSpeed);
      gfx->println(F("m/s"));
      
      Serial.println("\nCurrent weather information:");
      Serial.println("----------------------------");
      Serial.print("city: ");
      Serial.println(city);
      Serial.print("weatherMain: ");
      Serial.println(weatherMain);
      Serial.print("weatherDesc: ");
      Serial.println(weatherDesc);
      Serial.print("temperature: ");
      Serial.print(temp);
      Serial.println("°C");
      Serial.print("feelsLike: ");
      Serial.print(feelsLike);
      Serial.println("°C");
      Serial.print("humidity: ");
      Serial.print(humidity);
      Serial.println("%");
      Serial.print("windSpeed: ");
      Serial.print(windSpeed);
      Serial.println(" m/s");
      Serial.println("----------------------------");
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      Serial.print("Error: ");
      Serial.println(http.errorToString(httpResponseCode));
      gfx->fillScreen(WHITE);
      gfx->setCursor(50, 100);
      gfx->print(F("Error: "));
      gfx->print(http.errorToString(httpResponseCode));
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
    gfx->setCursor(15, 100);
    gfx->println(F("WiFi Disconnected"));
  }
}
