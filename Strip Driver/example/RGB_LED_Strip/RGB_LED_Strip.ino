/*
Author: copper
Date:2025.2.27
Code version: V1.0.4

Library version:
Arduino IDE 2.3.4
esp32 V3.1.1
GFX Library for Arduino v1.5.4
lvgl v8.3.11
Adafruit NeoPixel v1.12.4

Tools:
USB CDC On Boot: Enabled
Flash size: 16MB(128Mb)
Partition Schrme: 16M Flash(3MB APP/9.9MB FATFS)
PSRAM: OPI PSRAM
*/

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include <Adafruit_NeoPixel.h>
#include "touch.h"
#include "pin_config.h"
#include "power.h"

#define VOLTAGE_PAGE 1
#define COUNT_PAGE 2
#define BRIGHT_PAGE 3
#define COLOR_PAGE 4
#define MODE_PAGE 5
#define FREQ_PAGE 6

#define NORMAL_MODE 0
#define BLINK_MODE 1
#define FLOW_1_MODE 2
#define FLOW_2_MODE 3

#define RGB_MODE 0
#define RANDOM_MODE 1

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 3 /* rotation */, true /* IPS */);
Adafruit_NeoPixel strip_1(300, WS2812_PIN1, NEO_RGB + NEO_KHZ800);
Adafruit_NeoPixel strip_2(300, WS2812_PIN2, NEO_RGB + NEO_KHZ800);

// Encoder
int counter = 0;
int State;
int old_State;
int move_flag = 0;

// Led config
typedef struct
{
    int count;
    int max_bright;
    int bright;
    int freq;
    int mode;
    int random;
    int color[3];
} Led;

Led Led_1 =
    {
        30,
        100,
        100,
        0,
        NORMAL_MODE,
        RGB_MODE,
        {255, 255, 255}};

Led Led_2 =
    {
        30,
        100,
        100,
        0,
        NORMAL_MODE,
        RGB_MODE,
        {255, 255, 255}};

int flow_list[6] = {0};

// UI status
int page_index = 0;
int rgb_index = 0;
int led_channel = 0;
int on_off_flag = 0;
int temp_bright1 = 0;
int temp_bright2 = 0;
int mode_index = 0;
int mode_flag = 0;
int rd_color_index = 0;
int rd_color_flag = 0;

void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */

    pin_init();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
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

    strip_1.begin(); // INITIALIZE NeoPixel strip_1 object (REQUIRED)
    strip_1.clear();
    strip_1.show();           // Turn OFF all strip_1 ASAP
    strip_1.setBrightness(0); // Set BRIGHTNESS to about 1/5 (max = 255)

    strip_2.begin(); // INITIALIZE NeoPixel strip_1 object (REQUIRED)
    strip_2.clear();
    strip_2.show();             // Turn OFF all strip_1 ASAP
    strip_2.setBrightness(100); // Set BRIGHTNESS to about 1/5 (max = 255)

    Serial.println("Setup done");
    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 10240, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(Task_main, "Task_main", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(Task_led1, "Task_led1", 10240, NULL, 3, NULL, 1);
    xTaskCreatePinnedToCore(Task_led2, "Task_led2", 10240, NULL, 3, NULL, 1);
}

void loop()
{
}

void Task_TFT(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(10);
    }
}

long runtime = 0;
void Task_main(void *pvParameters)
{
    while (1)
    {
        if (voltage_flag == 1)
        {
            set_voltage_extern();
            voltage_flag = 0;
        }

        if (mode_flag == 1)
        {
            if (led_channel == 0)
            {
                Led_1.freq = 1;
                Led_1.mode = mode_index;
            }
            else if (led_channel == 1)
            {
                Led_2.freq = 1;
                Led_2.mode = mode_index;
            }
            mode_flag = 0;
        }

        if (rd_color_flag == 1)
        {
            if (led_channel == 0)
            {
                Led_1.random = rd_color_index;
            }
            else if (led_channel == 1)
            {
                Led_2.random = rd_color_index;
            }
            rd_color_flag = 0;
        }

        if (on_off_flag == 1)
        {   
            on_off_flag = 0;
            if(Led_1.max_bright!=0)//全关
            {
                temp_bright1 = Led_1.max_bright;
                temp_bright2 = Led_2.max_bright;
                Led_1.max_bright = 0;
                Led_2.max_bright = 0;
                Led_1.bright = 0;
                Led_2.bright = 0;
            }
            else//恢复
            {
                Led_1.max_bright = temp_bright1;
                Led_2.max_bright = temp_bright2;
                Led_1.bright = temp_bright1;
                Led_2.bright = temp_bright2;
            }
        }

        if (millis() - runtime > 3000)
        {
            if (Led_1.random == RANDOM_MODE)
                set_random_color(&Led_1);
            if (Led_2.random == RANDOM_MODE)
                set_random_color(&Led_2);

            runtime = millis();
        }

        if (led_channel == 0)
        {
            load_led_config(&Led_1);
            // global_value_get();
            encoder_func(&Led_1);
            obj_set(&Led_1);
        }
        else if (led_channel == 1)
        {
            load_led_config(&Led_2);
            encoder_func(&Led_2);
            obj_set(&Led_2);
        }

        vTaskDelay(300);
    }
}

void Task_led1(void *pvParameters)
{
    int dir = 1;
    int index = 0;
    while (1)
    {
        if (Led_1.mode == NORMAL_MODE)
        {
            Led_1.bright = Led_1.max_bright;
            fresh_led(&strip_1, &Led_1);
            vTaskDelay(100);
        }
        else if (Led_1.mode == BLINK_MODE)
        {
            float step = Led_1.max_bright / 20.0;

            if (dir == 1)
            {
                Led_1.bright += (int)(Led_1.freq * step);
                if (Led_1.bright >= Led_1.max_bright)
                {
                    dir = 0;
                    Led_1.bright = Led_1.max_bright;
                }
            }
            else if (dir == 0)
            {
                Led_1.bright -= (int)(Led_1.freq * step);
                if (Led_1.bright <= 0)
                {
                    dir = 1;
                    Led_1.bright = 0;
                }
            }
            fresh_led(&strip_1, &Led_1);
            vTaskDelay(100);
        }
        else if (Led_1.mode == FLOW_1_MODE)
        {
            Led_1.bright = Led_1.max_bright;

            if (index > Led_1.count)
                index = 0;

            flow_led(&strip_1, &Led_1, index++, 0, Led_1.count);
            judge_flow_list(Led_1.freq);
        }
        else if (Led_1.mode == FLOW_2_MODE)
        {
            Led_1.bright = Led_1.max_bright;

            if (index ==0)
                index = Led_1.count;

            flow_led(&strip_1, &Led_1, index--, 1, Led_1.count);
            judge_flow_list(Led_1.freq);
        }

        // fresh_led(&strip_1, &Led_1);
        // vTaskDelay(100);
    }
}

void Task_led2(void *pvParameters)
{
    int dir = 1;
    int index = 0;
    while (1)
    {

        if (Led_2.mode == NORMAL_MODE)
        {
            Led_2.bright = Led_2.max_bright;
            fresh_led(&strip_2, &Led_2);
            vTaskDelay(100);
        }
        else if (Led_2.mode == BLINK_MODE)
        {
            float step = Led_2.max_bright / 20.0;
            if (dir == 1)
            {
                Led_2.bright += (int)(Led_2.freq * step);
                if (Led_2.bright >= Led_2.max_bright)
                {
                    dir = 0;
                    Led_2.bright = Led_2.max_bright;
                }
            }
            else if (dir == 0)
            {
                Led_2.bright -= (int)(Led_2.freq * step);
                if (Led_2.bright <= 0)
                {
                    dir = 1;
                    Led_2.bright = 0;
                }
            }
            fresh_led(&strip_2, &Led_2);
            vTaskDelay(100);
        }
        else if (Led_2.mode == FLOW_1_MODE)
        {
            Led_2.bright = Led_2.max_bright;

            if (index > Led_2.count)
                index = 0;

            flow_led(&strip_2, &Led_2, index++, 0, Led_2.count);
            judge_flow_list(Led_2.freq);
        }
        else if (Led_2.mode == FLOW_2_MODE)
        {
            Led_2.bright = Led_2.max_bright;

            if (index == 0)
                index = Led_2.count;

            flow_led(&strip_2, &Led_2, index--, 1, Led_2.count);
            judge_flow_list(Led_2.freq);
        }

        // fresh_led(&strip_2, &Led_2);
    }
}

// Hardware init
void pin_init()
{
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(ENCODER_CLK, INPUT_PULLUP);
    pinMode(ENCODER_DT, INPUT_PULLUP);
    old_State = digitalRead(ENCODER_CLK);

    attachInterrupt(ENCODER_CLK, encoder_irq, CHANGE);

    pinMode(USB_PD_CFG1, OUTPUT);
    pinMode(USB_PD_CFG2, OUTPUT);
    pinMode(USB_PD_CFG3, OUTPUT);

    USB_PD_POWER_5V();
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

        data->point.x = (uint16_t)(240 - touchY);
        data->point.y = (uint16_t)touchX;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

// LVGL value
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
}

void encoder_set_arc_freq(lv_obj_t *arc, int *value)
{
    int min = lv_arc_get_min_value(arc);
    int max = lv_arc_get_max_value(arc);
    *value += counter;
    counter = 0;

    if (*value > max)
        *value = max;
    if (*value < 1)
        *value = 1;

    lv_arc_set_value(arc, *value);
}

void encoder_set_arc_fast(lv_obj_t *arc, int *value, int rate)
{
    counter *= rate;
    encoder_set_arc(arc, value);
}

void encoder_func(Led *led)
{
    if (page_index == BRIGHT_PAGE)
    {
        encoder_set_arc_fast(ui_Arc2, &led->max_bright, 4);
    }
    if (page_index == COLOR_PAGE)
    {
        if (rgb_index == 0)
            encoder_set_arc_fast(ui_Arc6, &led->color[0], 8);
        if (rgb_index == 1)
            encoder_set_arc_fast(ui_Arc7, &led->color[1], 8);
        if (rgb_index == 2)
            encoder_set_arc_fast(ui_Arc8, &led->color[2], 8);
    }
    if (page_index == COUNT_PAGE)
    {
        encoder_set_arc(ui_Arc3, &led->count);
    }
    if (page_index == FREQ_PAGE)
    {
        encoder_set_arc_freq(ui_Arc4, &led->freq);
    }
}

void load_led_config(Led *led)
{
    lv_arc_set_value(ui_Arc2, led->max_bright);
    lv_arc_set_value(ui_Arc3, led->count);
    lv_arc_set_value(ui_Arc4, led->freq);
    lv_arc_set_value(ui_Arc6, led->color[0]);
    lv_arc_set_value(ui_Arc7, led->color[1]);
    lv_arc_set_value(ui_Arc8, led->color[2]);
}

void obj_set(Led *led)
{
    char temp[40];

    // sprintf(temp, "Brightness\n%d", led->max_bright);
    sprintf(temp, "%d", led->max_bright);
    lv_label_set_text(ui_Label5, temp);
    sprintf(temp, "Color\nR%03dG%03dB%03d", led->color[0], led->color[1], led->color[2]);
    lv_label_set_text(ui_Label6, temp);
    // sprintf(temp, "Count\n%d", led->count);
    sprintf(temp, "%d", led->count);
    lv_label_set_text(ui_Label7, temp);
    // sprintf(temp, "Frequence\n%d", led->freq);
    sprintf(temp, "%d", led->freq);
    lv_label_set_text(ui_Label8, temp);

    lv_dropdown_set_selected(ui_Dropdown2, led->mode);

    if (led->random == RGB_MODE)
        lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);
    else
        lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);

    lv_color_t temp_color;
    temp_color = lv_color_make(led->color[0], led->color[1], led->color[2]);
    lv_obj_set_style_bg_color(ui_Panel1, temp_color, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (led_channel == 0)
    {

        lv_imgbtn_set_state(ui_ImgButton1, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton4, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton5, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton6, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton7, LV_IMGBTN_STATE_PRESSED);

        lv_imgbtn_set_state(ui_ImgButton2, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton8, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton9, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton10, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton11, LV_IMGBTN_STATE_RELEASED);
    }
    else if (led_channel == 1)
    {
        lv_imgbtn_set_state(ui_ImgButton1, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton4, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton5, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton6, LV_IMGBTN_STATE_RELEASED);
        lv_imgbtn_set_state(ui_ImgButton7, LV_IMGBTN_STATE_RELEASED);

        lv_imgbtn_set_state(ui_ImgButton2, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton8, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton9, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton10, LV_IMGBTN_STATE_PRESSED);
        lv_imgbtn_set_state(ui_ImgButton11, LV_IMGBTN_STATE_PRESSED);
    }
}

void set_random_color(Led *led)
{
    led->color[0] = random(256);
    led->color[1] = random(256);
    led->color[2] = random(256);
}

// WS2812 Function
void fresh_led(Adafruit_NeoPixel *strip, Led *led)
{
    strip->setBrightness(led->bright);
    for (int i = 0; i < strip->numPixels(); i++)
    {
        if (i < (led->count))
            strip->setPixelColor(i, strip->Color(led->color[0], led->color[1], led->color[2]));
        else
            strip->setPixelColor(i, 0, 0, 0);
    }
    strip->show();
}

void flow_led(Adafruit_NeoPixel *strip, Led *led, int index, int dir, int counts)
{
    // int flow_list[5] = {0, 60, 30, 20, 10};
    strip->setBrightness(led->bright);

    for (int i = 0; i < strip->numPixels(); i++)
    {
        // int step = flow_list[led->freq];
        int step = counts;
        float rate = 0.0;

        if (dir == 0)
            rate = ((i + index) % step) / (float)step;
        else
            rate = (step - (i + index) % step) / (float)step;

        uint32_t color = strip->Color((int)(led->color[0] * rate), (int)(led->color[1] * rate), (int)(led->color[2] * rate));

        if (i < (led->count))
            strip->setPixelColor(i, color);
        else
            strip->setPixelColor(i, 0, 0, 0);

        strip->show();
    }
}

void judge_flow_list(int freq)
{
    switch(freq)
    {
        case 1:
        vTaskDelay( 200 );
        break;
        case 2:
        vTaskDelay( 100 );
        break;
        case 3:
        vTaskDelay( 50 );
        break;
        case 4:
        vTaskDelay( 20 );
        break;
        case 5:
        vTaskDelay( 5 );
        break;
        default:
        vTaskDelay( 200 );
        break; 
    }
}
           
    
