/*
Library version:
Arduino IDE 2.3.2
esp32 V3.0.7
GFX Library for Arduino v1.4.9
lvgl v8.3.11
*/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include "touch.h"
#include "pin_config.h"

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

int dimtime = 0;
int dim_percent = 0;

hw_timer_t *dim_timer = NULL;
hw_timer_t *zero_timer = NULL;

void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */

    pin_init();
    dimmer_init();
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

    Serial.println("Setup done");

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 10240, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(Task_Main, "Task_Main", 4096, NULL, 3, NULL, 1);
}

long runtime = 0;

void loop()
{
}

void Task_TFT(void *pvParameters)
{
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(50);
    }
}

void Task_Main(void *pvParameters)
{
    while (1)
    {
        encoder_func();
        obj_update();
        set_power_percent(dim_percent);
        vTaskDelay(500);
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

        data->point.x = (uint16_t)touchX;
        data->point.y = (uint16_t)touchY;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void encoder_func()
{

    dim_percent += counter;
    counter = 0;

    if (dim_percent < 0)
        dim_percent = 0;
    if (dim_percent > 100)
        dim_percent = 100;
}

void obj_update()
{
    char temp[80] = "";

    sprintf(temp, "%d%%", dim_percent);
    lv_label_set_text(ui_Label1, temp);

    lv_arc_set_value(ui_Arc1, dim_percent);
}

// Dimmer Function

void dimmer_init()
{
    pinMode(RELAY_PIN, OUTPUT); // initialize the RELAYPin as an output:
    pinMode(SCR_Pin, OUTPUT);   // initialize the RELAYPin as an output:

    RELAY_OFF;
    AC_CTRL_OFF;
    delay(100);

    dim_timer = timerBegin(1000000);
    timerAttachInterrupt(dim_timer, &open_ac);
    zero_timer = timerBegin(1000000);
    timerAttachInterrupt(zero_timer, &enable_zero); // attach callback

    pinMode(ZCD_PIN, INPUT);
    attachInterrupt(ZCD_PIN, zero_cross_int, RISING);
}

void ARDUINO_ISR_ATTR zero_cross_int() // function to be fired at the zero crossing to dim the light
{
    AC_CTRL_OFF;
    timerRestart(dim_timer);
    timerAlarm(dim_timer, dimtime, false, 0);

    timerRestart(zero_timer);
    timerAlarm(zero_timer, 5 * 1000, false, 0);

    detachInterrupt(ZCD_PIN);
}

void ARDUINO_ISR_ATTR open_ac()
{
    AC_CTRL_ON;
}

void ARDUINO_ISR_ATTR enable_zero()
{
    attachInterrupt(ZCD_PIN, zero_cross_int, RISING);
}

void set_power(int level)
{
    int dim_list[10] = {95, 90, 85, 80, 75, 70, 50, 30, 20, 5};
    dimtime = 100 * dim_list[level];

    if (level == 0)
    {
        RELAY_OFF;
        return;
    }
    else
    {
        RELAY_ON;
    }
}

void set_power_percent(int percent)
{
    dimtime = 100 * map(percent, 0, 100, 95, 5);

    if (percent < 5)
    {
        RELAY_OFF;
        return;
    }
    else
    {
        RELAY_ON;
    }
}
