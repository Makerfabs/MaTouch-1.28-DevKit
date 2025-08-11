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

char ssid[] = "Makerfabs";
char pass[] = "20160704";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char broker[] = "broker.emqx.io"; //"test.mosquitto.org";
int port = 1883;

const char inTopic[] = "makerfabs/in";

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

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  Serial.print("Subscribing to topic: ");
  Serial.println(inTopic);
  Serial.println();

  int subscribeQos = 1;

  mqttClient.subscribe(inTopic, subscribeQos);

  Serial.print("Waiting for messages on topic: ");
  Serial.println(inTopic);
  Serial.println();

  gfx->fillScreen(WHITE);
  gfx->setCursor(40, 50);
  gfx->println(F("I'm Subscriber!"));
  gfx->setCursor(40, 100);
  gfx->println(F("Waiting to receive..."));

  Serial.println("Setup down!");

}

void loop()
{
  mqttClient.poll();
  vTaskDelay(50);
}

void onMqttMessage(int messageSize)
{
  String receive;
  // we received a message, print out the topic and contents
  Serial.print("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', duplicate = ");
  Serial.print(mqttClient.messageDup() ? "true" : "false");
  Serial.print(", QoS = ");
  Serial.print(mqttClient.messageQoS());
  Serial.print(", retained = ");
  Serial.print(mqttClient.messageRetain() ? "true" : "false");
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");

  // use the Stream interface to print the contents
  while (mqttClient.available())
  {
    char c = (char)mqttClient.read();
    receive += c;
  }
  Serial.println("Received payload: " + receive);
  Serial.println();
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->fillRect(0, 95, 240, 170, WHITE);
  gfx->setCursor(30, 100);
  gfx->println(F("Received message: "));
  gfx->setTextSize(2);
  gfx->setTextColor(RED);
  gfx->setCursor(40, 150);
  gfx->println(receive.c_str());
  Serial.println();
}
