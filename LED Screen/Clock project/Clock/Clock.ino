/*
Library version:
Arduino IDE 2.3.4
esp32 V3.1.1
GFX Library for Arduino v1.5.5
lvgl v8.3.11
ESP32 HUB75 LED MATRIX PANEL DMA Display v3.0.11
Adafruit GFX Library v1.11.11
Adafruit BusIO v1.16.2
RTClib v2.1.4

Tools:
USB CDC On Boot: Enabled
Flash size: 16MB(128Mb)
Partition Schrme: 16M Flash(3MB APP/9.9MB FATFS)
PSRAM: OPI PSRAM
*/

//*******************************************************************
#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "Clockface.h"
#include "CWDateTime.h"
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include "touch.h"
#include "pin_config.h"
#include "RTClib.h"

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 3 /* rotation */, true /* IPS */);

int counter = 0;
int State;
int old_State;
int move_flag = 0;

int brightness = 100;

// LVGL Global Value
int page_index = 0;
int gif_num = 0;
int fresh_flag = 0;

MatrixPanel_I2S_DMA *dma_display = nullptr;

CWDateTime cwDateTime;
Clockface *clockface;
//Clockface1 *clockface1;
Clockface2 *clockface2;
Clockface3 *clockface3;
RTC_PCF8563 rtc;

// Replace with your network credentials
const char *ssid = "Makerfabs";
const char *password = "20160704";

#define panelResX 64     // Number of pixels wide of each INDIVIDUAL panel module. 
#define panelResY 64     // Number of pixels tall of each INDIVIDUAL panel module.
#define panel_chain 1      // Total number of panels chained one to another

uint16_t myBLACK = dma_display->color565(0, 0, 0);
uint16_t myWHITE = dma_display->color565(255, 255, 255);
uint16_t myBLUE = dma_display->color565(0, 0, 255);

unsigned long screen_lastMillis = 0;
int screen_sleep = 0;
int screen_weak_up = 0;

//**********************************************************************************************
void displaySetup() 
{
  HUB75_I2S_CFG mxconfig(
    panelResX,   // module width
    panelResY,   // module height
    panel_chain  // Chain length
    );

  //******* 64x64 RGB MAtirx Display Setup *******
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  //dma_display->begin();
  // ***** Use this if you want to use your own Pins ****
  dma_display->begin(R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN);   
  dma_display->setBrightness(255); //0-255
  dma_display->clearScreen();
}
//**********************************************************************************************
void setup() 
{
  Serial.begin(115200); /* prepare for possible serial debug */

  pin_init();
  gfx->begin();

  delay(200);

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  ui_init();

  displaySetup();

  clockface = new Clockface(dma_display);
  //clockface1 = new Clockface1(dma_display);
  clockface2 = new Clockface2(dma_display);
  clockface3 = new Clockface3(dma_display);

  cwDateTime.begin();
  
  clockface->setup(&cwDateTime);
  Serial.println("Setup done");

  xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 10240, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(Task_Main, "Task_Main", 4096, NULL, 1, NULL, 0);
  delay(2500);//开机界面结束后才显示灯板
  xTaskCreatePinnedToCore(Task_Time, "Task_Time", 102400, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(Task_screen_sleep, "Task_screen_sleep", 10240, NULL, 1, NULL, 0);
}
//**********************************************************************************************
void loop() 
{
}
//**********************************************************************************************
void Task_TFT(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(50);
    }
}
//**********************************************************************************************
void Task_Main(void *pvParameters)
{
    while (1)
    {
        encoder_func();
        obj_update();
        vTaskDelay(500);
    }
}
//**********************************************************************************************
void Task_Time(void *pvParameters)
{
  while(1)
  {
      update_time(gif_num);
      if(fresh_flag==1){vTaskDelay(500);time_bg(gif_num);}
      //vTaskDelay(10);
  }
}
//**********************************************************************************************
void Task_screen_sleep(void *pvParameters)
{
    while (1)
    {
        if(millis()-screen_lastMillis>30000)//30s
        {
            screen_sleep=1;
            gfx->displayOff();
            dma_display->setBrightness(0);
            if(screen_weak_up==1)
            {
                gfx->displayOn();
                dma_display->setBrightness(map(brightness, 0, 100, 0, 255));
                screen_lastMillis=millis();
                screen_weak_up=0;
                screen_sleep=0;
            }
        }
        vTaskDelay(100);
    }
    
}
//**********************************************************************************************
void time_bg(int bg)
{
  switch(bg)
  {
    case 0:
    clockface->setup(&cwDateTime);
    clockface->update();
    fresh_flag=0;break;
    case 1:
    clockface3->setup(&cwDateTime);
    clockface3->update();
    fresh_flag=0;break;
    case 2:
    clockface2->setup(&cwDateTime);
    clockface2->update();
    fresh_flag=0;break;
    /*case 3:
    clockface1->setup(&cwDateTime);
    clockface1->update();
    fresh_flag=0;break;*/
  }
}

void update_time(int time)
{
  if(time==0)
  {
    clockface->update();
  }
  else if(time==1)
  {
    clockface3->update();
  }
  else if(time==2)
  {
    clockface2->update();
  }
  /*else if(time==3)
  {
    clockface1->update();
  }*/
}

void pin_init()
{
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    old_State = digitalRead(ENCODER_CLK);

    attachInterrupt(ENCODER_CLK, encoder_irq, CHANGE);
}

void encoder_irq()
{
    State = digitalRead(ENCODER_CLK);
    if (State != old_State)
    {
        if (digitalRead(ENCODER_DT) == State)
        {
            counter++;
        }
        else
        {
            counter--;
        }

        if(screen_sleep==1)
        {
            screen_weak_up= 1;
        }
    }
    old_State = State; // the first position was changed
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    int touchX = 0, touchY = 0;

    if (read_touch(&touchX, &touchY) == 1)
    {
        data->state = LV_INDEV_STATE_PR;

        data->point.x = (uint16_t)(240 - touchY);
        data->point.y = (uint16_t)touchX;

        if(screen_sleep==1)
        {
            screen_weak_up= 1;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void encoder_func()
{
    if (page_index == 0)
    {
        if (counter != 0)
        {
            screen_lastMillis=millis();
            gif_num += counter;
            counter = 0;

            if (gif_num < 0)
                gif_num = 2;
            if (gif_num > 2)
                gif_num = 0;

            Serial.print("gif_num=");
            Serial.println(gif_num);
            fresh_flag = 1;
        }
    }
    else if (page_index == 1)
    {
        if (counter != 0)
        {
            screen_lastMillis=millis();
            brightness += counter;
            counter = 0;

            if (brightness < 0)
                brightness = 0;
            if (brightness > 100)
                brightness = 100;

            dma_display->setBrightness(map(brightness, 0, 100, 0, 255)); // map()函数将亮度值从用户熟悉的百分比（0% 到 100%）映射到硬件亮度范围（0 到 255）
        }
    }
}

void obj_update()
{
    char temp[80] = "";
    if (page_index == 0)
    {
        lv_roller_set_selected(ui_Roller1, gif_num, LV_ANIM_ON);
    }

    if (page_index == 1) // 改变亮度
    {
        lv_arc_set_value(ui_Arc1, brightness);

        sprintf(temp, "%d%%", brightness);
        lv_label_set_text(ui_Label1, temp);
    }
}
