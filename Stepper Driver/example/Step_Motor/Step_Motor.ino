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

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 3 /* rotation */, true /* IPS */);

int counter = 0;
int State;
int old_State;
int move_flag = 0;

long position_x = 0;
long now_position_x = 0;
long position_y = 0;
long now_position_y = 0;
int unit_x = 200;
int unit_y = 200;
int rpm_x = 0;
int rpm_y = 0;
int dir_x = 0;
int dir_y = 0;

int page_index = 1;
int work_mode = 0;
int stepper_index = 0;
int home_reset_flag = 0;
int rpm_flag = 0;
int dir_change_flag = 0;

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

    Serial.println("Setup done");

    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 10240, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(Task_Main, "Task_Main", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(Task_Stepper, "Task_Stepper", 4096, NULL, 3, NULL, 1);
}

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
        if (home_reset_flag == 1) //Press reset to clear all.
        {
            home_reset_flag = 0;
            position_x = 0;
            now_position_x = 0;
            position_y = 0;
            now_position_y = 0;
        }

        if (rpm_flag == 1)
        {
            rpm_flag = 0;

            work_mode = 1;

            //Init pwm
            ledcAttachChannel(STEP_0_PIN, 3200, 8, 0); //frequency 3200, resolution 8, channel 0
            ledcAttachChannel(STEP_1_PIN, 3200, 8, 2);

            ledcWriteChannel(0, 0); //Set motor pwm to 0
            ledcWriteChannel(2, 0);
        }

        if (dir_change_flag == 1) //Forward and reverse button, press to take reverse
        {
            if (stepper_index == 0)
                dir_x = !dir_x;
            else
                dir_y = !dir_y;

            dir_change_flag = 0;
        }

        encoder_func();
        obj_update();
        vTaskDelay(500);
    }
}

void Task_Stepper(void *pvParameters)
{
    while (1)
    {
        // Positioning mode
        if (work_mode == 0)
        {
            pinMode(STEP_0_PIN, OUTPUT);
            pinMode(STEP_1_PIN, OUTPUT);

            if (position_x - now_position_x > 0)
            {
                run_stepper(0, 1, 1);
                now_position_x++;
            }
            else if (position_x - now_position_x < 0)
            {
                run_stepper(0, 0, 1);
                now_position_x--;
            }

            if (position_y - now_position_y > 0)
            {
                run_stepper(1, 1, 1);
                now_position_y++;
            }
            else if (position_y - now_position_y < 0)
            {
                run_stepper(1, 0, 1);
                now_position_y--;
            }

            vTaskDelay(20);
        }
        // RPM mode
        else if (work_mode == 1)
        {
            ledcChangeFrequency(STEP_0_PIN, rpm_to_step(rpm_x), 8);
            ledcChangeFrequency(STEP_1_PIN, rpm_to_step(rpm_y), 8);

            if (dir_x == 0)
                digitalWrite(DIR_0_PIN, LOW);
            else
                digitalWrite(DIR_0_PIN, HIGH);

            if (dir_y == 0)
                digitalWrite(DIR_1_PIN, LOW);
            else
                digitalWrite(DIR_1_PIN, HIGH);

            if (rpm_x == 0)
                ledcWriteChannel(0, 0);
            else
                ledcWriteChannel(0, 127);

            if (rpm_y == 0)
                ledcWriteChannel(2, 0);
            else
                ledcWriteChannel(2, 127);

            vTaskDelay(500);
        }
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

    pinMode(EN_PIN, OUTPUT);
    pinMode(STEP_0_PIN, OUTPUT);
    pinMode(DIR_0_PIN, OUTPUT);
    pinMode(STEP_1_PIN, OUTPUT);
    pinMode(DIR_1_PIN, OUTPUT);
    digitalWrite(EN_PIN, LOW); // Enable driver in hardware
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

void encoder_func()
{
    if (page_index == 1)
    {
        counter = 0;
    }
    else if (page_index == 2)
    {
        if (stepper_index == 0)
        {
            position_x += counter;
            counter = 0;
        }
        else
        {
            position_y += counter;
            counter = 0;
        }
    }
    else if (page_index == 3)
    {
        if (stepper_index == 0)
        {
            unit_x += counter * 10;
            if(unit_x < 0){unit_x = 0;}
            counter = 0;
        }
        else
        {
            unit_y += counter * 10;
            if(unit_y < 0){unit_y = 0;}
            counter = 0;
        }
    }
    else if (page_index == 4)
    {
        if (stepper_index == 0)
        {
            rpm_x += counter * 5;

            if (rpm_x > 100)
                rpm_x = 100;
            if (rpm_x < 0)
                rpm_x = 0;

            counter = 0;
        }
        else
        {
            rpm_y += counter * 5;

            if (rpm_y > 100)
                rpm_y = 100;
            if (rpm_y < 0)
                rpm_y = 0;

            counter = 0;
        }
    }
}

void obj_update()
{
    char temp[80] = "";
    if (page_index == 0)
    {
        return;
    }

    if (page_index == 2)
    {
        if (stepper_index == 0)
        {
            lv_obj_set_style_text_color(ui_Label5, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Label10, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_text_color(ui_Label5, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Label10, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        sprintf(temp, "X: %d mm", position_x);
        lv_label_set_text(ui_Label5, temp);
        sprintf(temp, "Y: %d mm", position_y);
        lv_label_set_text(ui_Label10, temp);

        return;
    }

    if (page_index == 3)
    {
        if (stepper_index == 0)
        {
            lv_obj_set_style_text_color(ui_Label6, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Label11, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        else
        {
            lv_obj_set_style_text_color(ui_Label6, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Label11, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        sprintf(temp, "X: %d step/mm", unit_x);
        lv_label_set_text(ui_Label6, temp);
        sprintf(temp, "Y: %d step/mm", unit_y);
        lv_label_set_text(ui_Label11, temp);

        return;
    }

    if (page_index == 4)
    {
        if (stepper_index == 0)
        {
            lv_obj_set_style_text_color(ui_Label9, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Label12, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_arc_set_value(ui_Arc1, rpm_x);
            lv_obj_invalidate(ui_Arc1);

            if (dir_x == 0)
                lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);
            else
                lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_set_style_text_color(ui_Label9, TXT_BLACK, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Label12, TXT_RED, LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_arc_set_value(ui_Arc1, rpm_y);
            lv_obj_invalidate(ui_Arc1);

            if (dir_y == 0)
                lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);
            else
                lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);
        }

        sprintf(temp, "X: %d rpm", rpm_x);
        lv_label_set_text(ui_Label9, temp);
        sprintf(temp, "Y: %d rpm", rpm_y);
        lv_label_set_text(ui_Label12, temp);

        return;
    }
}

// Stepper
void run_stepper(int index, int dir, int position)
{

    if (index == 0)
    {
        if (dir == 0)
            digitalWrite(DIR_0_PIN, LOW);
        else
            digitalWrite(DIR_0_PIN, HIGH);

        for (uint16_t i = unit_x; i > 0; i--)
        {
            digitalWrite(STEP_0_PIN, HIGH);
            delayMicroseconds(STEPPER_DELAY);
            digitalWrite(STEP_0_PIN, LOW);
            delayMicroseconds(STEPPER_DELAY);
        }
    }
    else
    {
        if (dir == 0)
            digitalWrite(DIR_1_PIN, LOW);
        else
            digitalWrite(DIR_1_PIN, HIGH);

        for (uint16_t i = unit_y; i > 0; i--)
        {
            digitalWrite(STEP_1_PIN, HIGH);
            delayMicroseconds(STEPPER_DELAY);
            digitalWrite(STEP_1_PIN, LOW);
            delayMicroseconds(STEPPER_DELAY);
        }
    }
}

int rpm_to_step(int rpm)
{
    return (int)rpm * 3200 / 60.0;
}