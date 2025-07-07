/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0

Tools:
USB CDC On Boot: Enabled
*/
#include <Arduino_GFX_Library.h>
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

#define ENCODER_CLK 48 // CLK
#define ENCODER_DT 47  // DT

int counter = 0;
int State;
int old_State;
int move_flag = 0;
int flesh_flag = 1;

int x = 0, y = 0;

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

void setup(void)
{
    Serial.begin(115200);

    pin_init();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    delay(1000);
    Serial.println("start");

    gfx->begin();
    gfx->fillScreen(WHITE);
}

void loop()
{

    if (read_touch(&x, &y) == 1)
    {
        flesh_flag = 1;
    }

    if (move_flag == 1)
    {
        Serial.print("Position: ");
        Serial.println(counter);
        move_flag = 0;
        flesh_flag = 1;
    }
    if (flesh_flag == 1)
        page_1();
}

void pin_init()
{
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    old_State = digitalRead(ENCODER_CLK); // Store initial CLK state

     // Attach interrupt to CLK pin, trigger on both rising and falling edges
    attachInterrupt(ENCODER_CLK, encoder_irq, CHANGE);
}

void encoder_irq()
{
    State = digitalRead(ENCODER_CLK);
    if (State != old_State)
    {
        if (digitalRead(ENCODER_DT) == State)
        {
            counter++; // Clockwise rotation
        }
        else
        {
            counter--; // Counterclockwise rotation
        }
    }
    old_State = State;
    move_flag = 1;
}

void page_1()
{
    gfx->fillRect(135, 95, 50, 65, YELLOW);
    gfx->fillCircle(x, y, 5, RED);

    gfx->setTextSize(2);
    gfx->setTextColor(BLACK);
    gfx->setCursor(60, 50);
    gfx->println(F("Makerfabs"));

    char temp[30];

    gfx->setTextSize(2);
    gfx->setCursor(30, 100);
    sprintf(temp, "Encoder: %4d", counter);
    gfx->println(temp);

    gfx->setTextSize(2);
    gfx->setCursor(30, 120);
    sprintf(temp, "Touch X: %4d", x);
    gfx->println(temp);

    gfx->setTextSize(2);
    gfx->setCursor(30, 140);
    sprintf(temp, "Touch Y: %4d", y);
    gfx->println(temp);

    flesh_flag = 0;
}
