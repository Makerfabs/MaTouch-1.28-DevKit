/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0
ArduinoMqttClient v0.1.8

Tools:
USB CDC On Boot: Enabled
*/

#include <Arduino_GFX_Library.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include "touch.h"

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

char ssid[] = "YOUR SSID";
char pass[] = "YOUR PIN";

int count =0;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "broker.emqx.io"; //"test.mosquitto.org";
int port = 1883;

bool retained = false;  //When a new client subscribes to this topic, it will immediately receive the retained message
int qos = 1;


const char outTopic[]  = "makerfabs/in";

int x = 0, y = 0;

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

void setup()
{
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  gfx->begin();
  gfx->fillScreen(WHITE);
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(40, 100);
  gfx->println(F("Connecting to WiFi"));

  WiFi.begin(ssid, pass);

  while (WiFi.status()!= WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }

  if (!mqttClient.connect(broker, port))
  {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  gfx->fillScreen(WHITE);
  gfx->setCursor(40, 50);
  gfx->println(F("I'm Publisher!"));

  Serial.println("Setup down!");

}

void loop()
{
  mqttClient.poll();

  String payload;

  payload += "hello world!";
  payload += " ";
  payload += count;

  bool dup = false;//duplicate transmission of the same message
  Serial.print("Sending message to topic: ");
  Serial.println(outTopic);
  Serial.print("Sending message: ");
  Serial.println(payload);
  mqttClient.beginMessage(outTopic, payload.length(), retained, qos, dup);
  mqttClient.write((const uint8_t *)payload.c_str(), payload.length());
  mqttClient.endMessage();

  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->fillRect(0, 95, 240, 170, WHITE);
  gfx->setCursor(30, 100);
  gfx->println(F("Sending message: "));
  gfx->setTextSize(2);
  gfx->setTextColor(RED);
  gfx->setCursor(40, 150);
  gfx->println(payload);
  Serial.println();

  count++;
  vTaskDelay(2000);
}

