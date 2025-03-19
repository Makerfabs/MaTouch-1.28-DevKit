/*
Author: copper
Date:2025.2.21 
Code version: V1.0.3

Library version:
Arduino IDE 2.3.4
esp32 V3.1.1
GFX Library for Arduino v1.5.3
lvgl v8.3.11
RTClib v2.1.4
Adafruit BusIO v1.16.2
ESP32Time v2.0.6

Tools:
USB CDC On Boot: Enabled
Flash size: 16MB(128Mb)
Partition Schrme: 16M Flash(3MB APP/9.9MB FATFS)
PSRAM: OPI PSRAM
*/

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include <RTClib.h>
#include <ESP32Time.h>

#include "touch.h"
#include "pin_config.h"
#include "local_store.h"

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 2 /* rotation */, true /* IPS */);

RTC_PCF8563 rtc_pcf;
ESP32Time rtc_esp;

int counter = 0;
int State;
int old_State;
int move_flag = 0;

// Global
int time_shift_index = 0; // hour,min,sec              sequential
int rtc_set_flag = 0;
int page_index = 0;

int alarm_set_flag = 0;
int alarm_reset_flag = 0;
int alarm_load_flag = 0;
int alarm_store_flag = 0;

int relay_flag = 0;     // Switch Single Control       flag
int relay_state = 0;    // Switching state             transmit a value

// Local
int relay_inedx = 0;       // Relay Number
int relay_status[8] = {0}; // Relay status
int alarm_index = 0;       // Alarm Clock Number
int alarm_status = 0;

typedef struct My_time
{
    int hou;
    int min;
    int sec;
};

My_time t_set = {0, 0, 0};
My_time t_clock = {0, 0, 0};
My_time t_alarm = {0, 0, 0};

//8 relays, 10 sets of clocks, 6 hours, minutes, seconds + 1 end symbol '/0'
char alarm_data[8][10][7];

int relay_pin[8]{
    RELAY1_PIN,
    RELAY2_PIN,
    RELAY3_PIN,
    RELAY4_PIN,
    RELAY5_PIN,
    RELAY6_PIN,
    RELAY7_PIN,
    RELAY8_PIN};

void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */
    delay(1000);

    pin_init();
    for (int i = 0; i < 8; i++)
        set_relay(i, 0); //Set the relay switches to set all 8 to 0;

    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    rtc_pcf_init();

    alarm_data_init(); // Alarm clock initialisation, read flash data, flash with data will be assigned value, no data will be set to ---.
    alarm_data_print();

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

    Serial.println("Setup done");

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 10240, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(Task_main, "Task_main", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(Task_time, "Task_time", 4096, NULL, 3, NULL, 1);
}

long runtime = 0;

void loop()
{
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}

void Task_TFT(void *pvParameters) // screen refresh
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(50);
    }
}

void Task_main(void *pvParameters) // Relay and alarm settings
{
    while (1)
    {
        // screen 1 Sets the time of the RTC.
        if (rtc_set_flag == 1)
        {
            rtc_reset();

            rtc_set_flag = 0;
            time_shift_index = 0;
        }

        relay_event(); // Relay control
        alarm_event(); // Alarm reset, load, save

        // Encoder function
        encoder_func();

        // UI State Refresh
        obj_update();

        vTaskDelay(100);
    }
}

 // Determine whether the alarm clock and the current time match, match the corresponding switch relays
void Task_time(void *pvParameters)
{
    while (1)
    {

        DateTime now = rtc_pcf.now();

        // Update real time
        t_clock.hou = now.hour();
        t_clock.min = now.minute();
        t_clock.sec = now.second();

        char now_time[7];
        sprintf(now_time, "%02d%02d%02d", t_clock.hou, t_clock.min, t_clock.sec);

        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 10; j++)
            {
                if (alarm_check(i, j, now_time)) //Determine whether the alarm clock and the current time match
                {
                    Serial.println("Alarm detect");
                    if (j % 2 == 0)
                        set_relay(i, 1);
                    else
                        set_relay(i, 0);
                }
            }
        }

        vTaskDelay(500);
    }
}

void relay_event()
{
    if (relay_flag == 1)
    {
        set_relay(relay_inedx, relay_state);
        relay_flag = 0;
    }
}

void alarm_event()
{
    if (alarm_reset_flag == 1)
    {
        alarm_data_reset(relay_inedx);
        alarm_index = 0;
        alarm_load_flag = 1;

        alarm_reset_flag = 0;
    }

    if (alarm_load_flag == 1)
    {
        if (alarm_data[relay_inedx][alarm_index][0] == '-')
        {
            alarm_status = 0;

            t_alarm.hou = 0;
            t_alarm.min = 0;
            t_alarm.sec = 0;
        }
        else
        {
            alarm_status = 1;

            char *temp = alarm_data[relay_inedx][alarm_index];

            t_alarm.hou = (temp[0] - '0') * 10 + (temp[1] - '0');
            t_alarm.min = (temp[2] - '0') * 10 + (temp[3] - '0');
            t_alarm.sec = (temp[4] - '0') * 10 + (temp[5] - '0');
        }
        alarm_load_flag = 0;
    }

    if (alarm_set_flag == 1)
    {
        if (alarm_status == 1)
        {
            char temp[7];
            sprintf(temp, "%02d%02d%02d", t_alarm.hou, t_alarm.min, t_alarm.sec);
            alarm_data_update(relay_inedx, alarm_index, temp);
        }

        alarm_index++;
        if (alarm_index > 9)
            alarm_index = 0;

        alarm_load_flag = 1;
        time_shift_index = 0;

        alarm_set_flag = 0;
    }

    if (alarm_store_flag == 1)
    {
        alarm_data_print();
        alarm_data_restore(relay_inedx);

        alarm_index = 0;

        alarm_store_flag = 0;
    }
}

//---------------------------------------------

void pin_init()
{
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    old_State = digitalRead(ENCODER_CLK);

    attachInterrupt(ENCODER_CLK, encoder_irq, CHANGE);

    for (int i = 0; i < 8; i++)
    {
        pinMode(relay_pin[i], OUTPUT);
        digitalWrite(relay_pin[i], 0);
    }
        
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

        data->point.x = (uint16_t)(240 - touchX);
        data->point.y = (uint16_t)(240 - touchY);
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

// UI

void obj_update()
{
    char temp[40];

    if (page_index == 0)
    {
        sprintf(temp, "%02d", t_set.hou);
        lv_label_set_text(ui_Label1, temp);
        sprintf(temp, "%02d", t_set.min);
        lv_label_set_text(ui_Label2, temp);
        sprintf(temp, "%02d", t_set.sec);
        lv_label_set_text(ui_Label3, temp);

        if (time_shift_index == 0)
            lv_obj_set_style_text_color(ui_Label1, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label1, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (time_shift_index == 1)
            lv_obj_set_style_text_color(ui_Label2, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label2, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (time_shift_index == 2)
            lv_obj_set_style_text_color(ui_Label3, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label3, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    else if (page_index == 1)
    {
        sprintf(temp, "%02d:%02d:%02d", t_clock.hou, t_clock.min, t_clock.sec);
        lv_label_set_text(ui_Label25, temp);

        if (relay_status[relay_inedx] == 0)
            lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);
        else
            lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);

        if (relay_status[0] == 0)
            lv_obj_set_style_text_color(ui_Label7, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label7, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (relay_status[1] == 0)
            lv_obj_set_style_text_color(ui_Label8, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label8, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (relay_status[2] == 0)
            lv_obj_set_style_text_color(ui_Label9, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label9, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (relay_status[3] == 0)
            lv_obj_set_style_text_color(ui_Label10, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label10, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (relay_status[4] == 0)
            lv_obj_set_style_text_color(ui_Label11, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label11, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (relay_status[5] == 0)
            lv_obj_set_style_text_color(ui_Label12, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label12, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (relay_status[6] == 0)
            lv_obj_set_style_text_color(ui_Label13, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label13, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        if (relay_status[7] == 0)
            lv_obj_set_style_text_color(ui_Label14, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label14, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    else if (page_index == 2)
    {
        if (alarm_index % 2 == 0)
        {
            sprintf(temp, "Relay%d Open #%d", relay_inedx + 1, alarm_index / 2 + 1);
        }
        else
        {
            sprintf(temp, "Relay%d Close #%d", relay_inedx + 1, alarm_index / 2 + 1);
        }

        lv_label_set_text(ui_Label22, temp);

        if (alarm_status == 0)
        {
            lv_label_set_text(ui_Label19, "--");
            lv_label_set_text(ui_Label20, "--");
            lv_label_set_text(ui_Label21, "--");
        }
        else
        {
            sprintf(temp, "%02d", t_alarm.hou);
            lv_label_set_text(ui_Label19, temp);
            sprintf(temp, "%02d", t_alarm.min);
            lv_label_set_text(ui_Label20, temp);
            sprintf(temp, "%02d", t_alarm.sec);
            lv_label_set_text(ui_Label21, temp);
        }

        if (time_shift_index == 0)
            lv_obj_set_style_text_color(ui_Label19, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label19, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (time_shift_index == 1)
            lv_obj_set_style_text_color(ui_Label20, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label20, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);

        if (time_shift_index == 2)
            lv_obj_set_style_text_color(ui_Label21, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        else
            lv_obj_set_style_text_color(ui_Label21, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void encoder_func()
{
    if (page_index == 0)
    {
        encoder_set_time(&t_set);
    }
    else if (page_index == 1)
    {
        encoder_set_arc(ui_Arc1, &relay_inedx);
    }
    else if (page_index == 2)
    {
        if (counter != 0)
            alarm_status = 1;
        encoder_set_time(&t_alarm);
    }
}

void rtc_pcf_init()
{
    if (!rtc_pcf.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1)
            delay(10);
    }

    if (rtc_pcf.lostPower())
    {
        Serial.println("PCF8563 is NOT initialized, let's set the time!");
        rtc_pcf.adjust(DateTime(2024, 1, 1, 0, 0, 0));
    }
    rtc_pcf.start();

    DateTime now = rtc_pcf.now();

    t_set.hou = now.hour();
    t_set.min = now.minute();
    t_set.sec = now.second();

    char temp[40];
    sprintf(temp, "PCF time %02d:%02d:%02d", t_set.hou, t_set.min, t_set.sec);
    Serial.println(temp);
}

void rtc_reset() 
{
    rtc_pcf.adjust(DateTime(2024, 1, 1, t_set.hou, t_set.min, t_set.sec));

    DateTime now = rtc_pcf.now();
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    rtc_esp.setTime(now.second(), now.minute(), now.hour(), 1, 1, 2024); // 17th Jan 2021 1:2:3
    // Serial.println(rtc_esp.getTime("%A, %B %d %Y %H:%M:%S"));
    Serial.println(rtc_esp.getTime()); //  (String) 15:24:38
}

// encouder
void encoder_set_time(My_time *my_t)
{
    if (time_shift_index == 0) //hour
    {
        my_t->hou += counter;
        if (my_t->hou > 23)
            my_t->hou = 23;
        if (my_t->hou < 0)
            my_t->hou = 0;
    }
    else if (time_shift_index == 1) //min
    {
        my_t->min += counter;
        if (my_t->min > 59)
            my_t->min = 59;
        if (my_t->min < 0)
            my_t->min = 0;
    }
    else if (time_shift_index == 2) //sec
    {
        my_t->sec += counter;
        if (my_t->sec > 59)
            my_t->sec = 59;
        if (my_t->sec < 0)
            my_t->sec = 0;
    }

    counter = 0;
}

void encoder_set_arc(lv_obj_t *arc, int *value)
{
    int min = lv_arc_get_min_value(arc);
    int max = lv_arc_get_max_value(arc);
    *value += counter;
    counter = 0;

    if (*value > max)
        *value = max;
    if (*value < min)
        *value = min;

    lv_arc_set_value(arc, *value);
    lv_obj_invalidate(arc);
}

void set_relay(int num, int status)
{
    char temp[30];
    sprintf(temp, "Num %d, set status %d", num, status);
    Serial.println(temp);

    relay_status[num] = status;

    digitalWrite(relay_pin[num], status);
}

// Alarm data
// char alarm_data[8][10][7];
// Alarm clock initialisation, read flash data, flash with data will be assigned value, no data will be set to ---.
void alarm_data_init()
{
    const char *init_string = "------";

    for (int i = 0; i < 8; i++) //8 channels
    {
        char key_name[10];
        char nvs_data[80];
        sprintf(key_name, "DATA_%d", i);

        if (read_nvs(key_name, nvs_data) == SUCCESS)
        {
            Serial.println("Read NVS Success");
            Serial.println(nvs_data);

            for (int j = 0; j < 10; j++) //10 alarm clocks
            {
                for (int k = 0; k < 6; k++) //6 characters (hour, minute and second data)
                {
                    alarm_data[i][j][k] = nvs_data[j * 6 + k]; //Assign the read characters one by one to the alarm clock data
                }
                alarm_data[i][j][6] = '\0';//string terminator
            }
        }
        else
        {
            Serial.println("Read NVS ERROR");
            alarm_data_reset(i);//No data read. Reset to ‘------’.
            alarm_data_restore(i);
        }
    }

    // 应该是从NVS读的
    // const char *init_string = "------";
    // for (int i = 0; i < 8; i++)
    // {
    //     for (int j = 0; j < 10; j++)
    //     {
    //         strcpy(alarm_data[i][j], init_string);
    //     }
    // }
}

void alarm_data_update(int relay_num, int index, char *time)
{
    strcpy(alarm_data[relay_num][index], time);
}

void alarm_data_reset(int relay_num)
{
    const char *init_string = "------";
    for (int j = 0; j < 10; j++)
    {
        strcpy(alarm_data[relay_num][j], init_string);
    }
}

int alarm_check(int relay_num, int index, char *time)
{   
    if (alarm_data[relay_num][index][0] == '-')
        return 0;

    if (strcmp(alarm_data[relay_num][index], time) == 0)
        return 1;
    else
        return 0;
}

void alarm_data_print()
{
    for (int i = 0; i < 8; i++)
    {
        Serial.printf("Relay %d\n", i);

        for (int j = 0; j < 10; j++)
        {
            Serial.printf(alarm_data[i][j]);
            Serial.printf(",");
        }
        Serial.println();
    }
}

void alarm_data_restore(int relay_num)
{
    char nvs_data[80] = "";

    for (int j = 0; j < 10; j++)
    {
        // strcat(nvs_data, alarm_data[relay_num][j]);
        for (int k = 0; k < 6; k++)
        {
            nvs_data[j * 6 + k] = alarm_data[relay_num][j][k];
        }
    }
    nvs_data[60] = '\0';

    Serial.println(nvs_data);
    Serial.println(sizeof(nvs_data));

    char key_name[10];
    sprintf(key_name, "DATA_%d", relay_num);

    write_nvs(key_name, nvs_data);
}