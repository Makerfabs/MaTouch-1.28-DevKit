/*
Library version:
Arduino IDE v2.3.6
esp32 v3.2.0
GFX Library for Arduino v1.6.0
RTClib v2.1.4
Adafruit BusIO v1.16.2
*/

#include <Arduino_GFX_Library.h>
#include <RTClib.h>

#define TFT_BLK 45
#define TFT_RES 21

#define TFT_CS 1
#define TFT_MOSI 2
#define TFT_MISO -1
#define TFT_SCLK 42
#define TFT_DC 46

#define MOTOR_PIN 41

#define RTC_SCL 39
#define RTC_SDA 38
#define RTC_INT 3

RTC_PCF8563 rtc_pcf;

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

int alarm_flag=0;
int alarm_hour=0, alarm_min=0, alarm_sec=0;

unsigned long motorStartTime = 0;
bool motorRunning = false;

String inputString = "";
bool stringComplete = false;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  Wire.begin(RTC_SDA, RTC_SCL);
  rtc_pcf_init();

  gfx->begin();
  gfx->fillScreen(WHITE);
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(50, 50);
  gfx->println(F("Current Time"));

  Serial.println("Please enter the format: 1.SET TIME HH MM SS OR 2.SET ALARM HH MM SS");
}

void loop()
{
  // put your main code here, to run repeatedly:
  char timeStr[9]; 
  DateTime now = rtc_pcf.now();
  sprintf(timeStr, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  gfx->fillRect(50, 70, 155, 30, YELLOW);
  gfx->setTextSize(3);
  gfx->setTextColor(BLACK);
  gfx->setCursor(50, 75);
  gfx->print(timeStr);

  serialEvent();
  //print_time();
  if (stringComplete)
  {
    inputString.trim();
    int h, m, s;
    if (sscanf(inputString.c_str(), "SET TIME %d %d %d", &h, &m, &s) == 3)
    {
      set_rtc_time(h, m, s);
    }
    else if (sscanf(inputString.c_str(), "SET ALARM %d %d %d", &h, &m, &s) == 3)
    {
      alarm_time_set(h, m, s);
      alarm_flag = 1;
    }
    else
    {
      Serial.println("Invalid command.");
    }

    inputString = "";
    stringComplete = false;
  }

  if(alarm_flag == 1)
  {
    if(now.hour()==alarm_hour && now.minute()==alarm_min && now.second()==alarm_sec)
    {
      analogWrite(MOTOR_PIN, 127);
      motorStartTime = millis();
      motorRunning = true;
    }
  }

  if (motorRunning && millis() - motorStartTime >= 2000) 
  {
    analogWrite(MOTOR_PIN, 0);
    motorRunning = false;
  }

   delay(1000); // 每秒更新一次
}

void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read(); // 读取一个字节
    inputString += inChar;             // 加入命令缓存
    if (inChar == '\n')
    {
      stringComplete = true;           // 标记接收完毕
    }
  }
}

void set_rtc_time(int h, int m, int s)
{
  DateTime now = rtc_pcf.now();
  rtc_pcf.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
  Serial.println("Time updated.");

  // 清空串口缓冲区
  while (Serial.available() > 0)
  {
    Serial.read();
  }
}

void alarm_time_set(int h, int m, int s)
{
  char timeStr[9];
  alarm_hour = h;
  alarm_min = m;
  alarm_sec = s;

  sprintf(timeStr, "%02d:%02d:%02d", alarm_hour, alarm_min, alarm_sec);

  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(60, 135);
  gfx->println(F("Alarm Time"));
  gfx->fillRect(50, 155, 155, 30, WHITE);
  gfx->setTextSize(3);
  gfx->setCursor(50, 160);
  gfx->print(timeStr);

  Serial.println("alarm time set successfully!");

  // 清空串口缓冲区
  while (Serial.available() > 0)
  {
    Serial.read();
  }
}

void rtc_pcf_init()
{
    if (!rtc_pcf.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
    }
    //rtc_pcf.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void print_time()
{
  DateTime now = rtc_pcf.now();
  Serial.print("Current RTC Time: ");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}
