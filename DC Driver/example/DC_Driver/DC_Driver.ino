/*
Library version:
Arduino IDE 2.3.2
esp32 V3.0.7
GFX Library for Arduino v1.4.9
lvgl v8.3.11
Adafruit INA219 v1.2.3
Adafruit BusIO v1.16.1
*/

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include <Adafruit_INA219.h>
#include "touch.h"
#include "pin_config.h"

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 3 /* rotation */, true /* IPS */);
Adafruit_INA219 ina_0(0x40);
Adafruit_INA219 ina_1(0x44);

int counter = 0;
int State;
int old_State;
int move_flag = 0;

// DC motor
int pwm_0 = 0;
int pwm_1 = 0;
int dir_0 = 0;
int dir_1 = 0;

int now_pwm_0 = 0;
int now_pwm_1 = 0;
int now_dir_0 = 0;
int now_dir_1 = 0;

long count_0 = 0;
long count_1 = 0;
double rpm_0 = 0.0;
double rpm_1 = 0.0;
double current_0 = 0.0;
double current_1 = 0.0;

// LVGL Global Value
int page_index = 0;
int all_close_flag = 0;
int dir_change_flag = 0;

void setup()
{
    Serial.begin(115200); /* prepare for possible serial debug */

    pin_init();
    dc_init();
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    Wire1.begin(I2C_2_SDA, I2C_2_SCL);

    if (!ina_0.begin(&Wire1))
    {
        // while (1)
        {
            Serial.println("Failed to find INA219 chip");
            delay(1000);
        }
    }
    if (!ina_1.begin(&Wire1))
    {
        // while (1)
        {
            Serial.println("Failed to find INA219 chip");
            delay(1000);
        }
    }

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
    xTaskCreatePinnedToCore(Task_Main, "Task_Main", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(Task_Hall, "Task_Hall", 4096, NULL, 1, NULL, 1);
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
        if (all_close_flag == 1)
        {
            pwm_0 = 0;
            pwm_1 = 0;

            all_close_flag = 0;
        }

        if (dir_change_flag == 1)
        {
            if (page_index == 1)
                dir_0 = !dir_0;
            else if (page_index == 2)
                dir_1 = !dir_1;

            dir_change_flag = 0;
        }

        encoder_func();
        obj_update();
        set_motor_slow(0, pwm_0, dir_0);
        set_motor_slow(1, pwm_1, dir_1);
        // set_motor(0, pwm_0, dir_0);
        // set_motor(1, pwm_1, dir_1);
        status_print();
        vTaskDelay(500);
    }
}

void Task_Hall(void *pvParameters)
{
    long last_time = 0;
    long currentTime = 0;
    float rpm = 0.0;
    long detect_time = 1000;

    while (1)
    {
        currentTime = millis();

        if (currentTime - last_time >= detect_time)
        {

            rpm_0 = count_0 * 60.0 / HALL_UNIT * 1000 / (currentTime - last_time);
            rpm_1 = count_1 * 60.0 / HALL_UNIT * 1000 / (currentTime - last_time);

            if (now_pwm_0 == 0)
                rpm_0 = 0;
            if (now_pwm_1 == 0)
                rpm_1 = 0;

            // 重置脉冲计数器和时间
            count_0 = 0;
            count_1 = 0;
            last_time = millis();
        }

        current_0 = ina_0.getCurrent_mA() * 2;

        // Serial.print("Motor 1 Current:       ");
        // Serial.print(current_mA);
        // Serial.println(" mA");

        current_1 = ina_1.getCurrent_mA() * 2;

        // Serial.print("Motor 2 Current:       ");
        // Serial.print(current_mA);
        // Serial.println(" mA");

        vTaskDelay(1000);
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

        data->point.x = (uint16_t)(240 - touchY);
        data->point.y = (uint16_t)touchX;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

// UI

void encoder_func()
{
    if (page_index == 0)
    {
        counter = 0;
    }
    else if (page_index == 1)
    {
        pwm_0 += counter;
        counter = 0;

        if (pwm_0 < 0)
            pwm_0 = 0;
        if (pwm_0 > 100)
            pwm_0 = 100;
    }
    else if (page_index == 2)
    {
        pwm_1 += counter;
        counter = 0;

        if (pwm_1 < 0)
            pwm_1 = 0;
        if (pwm_1 > 100)
            pwm_1 = 100;
    }
}

void obj_update()
{
    char temp[80] = "";
    if (page_index == 0)
    {
        sprintf(temp, "%d%%", pwm_0);
        lv_label_set_text(ui_Label10, temp);//电机1pwm百分比

        sprintf(temp, "%d%%", pwm_1);
        lv_label_set_text(ui_Label11, temp);//电机2pwm百分比

        sprintf(temp, "%.0fRPM %.0fmA", fabs(rpm_0), fabs(current_0));
        lv_label_set_text(ui_Label1, temp);//电机1当前转速和电流大小

        sprintf(temp, "%.0fRPM %.0fmA", fabs(rpm_1), fabs(current_1));
        lv_label_set_text(ui_Label5, temp);//电机2当前转速和电流大小

        lv_arc_set_value(ui_Arc2, pwm_0);//电机1pwm百分比
        lv_arc_set_value(ui_Arc4, pwm_1);//电机1pwm百分比
    }

    if (page_index == 1)//电机1调速
    {
        lv_arc_set_value(ui_Arc1, pwm_0);

        sprintf(temp, "%d%%", pwm_0);
        lv_label_set_text(ui_Label8, temp);

        sprintf(temp, "%.1fRPM %.1fmA", fabs(rpm_0), fabs(current_0));
        lv_label_set_text(ui_Label9, temp);

        if (dir_0 == 0)
            lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);
        else
            lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);
    }

    if (page_index == 2)//电机2调速
    {
        lv_arc_set_value(ui_Arc1, pwm_1);

        sprintf(temp, "%d%%", pwm_1);
        lv_label_set_text(ui_Label8, temp);
        
        sprintf(temp, "%.1fRPM %.1fmA", fabs(rpm_1), fabs(current_1));
        lv_label_set_text(ui_Label9, temp);

        if (dir_1 == 0)
            lv_obj_clear_state(ui_Switch1, LV_STATE_CHECKED);
        else
            lv_obj_add_state(ui_Switch1, LV_STATE_CHECKED);
    }
}

// Hardware func

void status_print()
{
    Serial.println("Motor 0:");
    Serial.println(dir_0);
    Serial.println(pwm_0);
    Serial.print(rpm_0);
    Serial.println(" rpm");
    Serial.print(current_0);
    Serial.println(" mA");

    Serial.println("Motor 1:");
    Serial.println(dir_1);
    Serial.println(pwm_1);
    Serial.println(rpm_1);
    Serial.println(" rpm");
    Serial.print(current_1);
    Serial.println(" mA");
}

void dc_init()
{
    pinMode(PIN_CTR1_A, OUTPUT);
    pinMode(PIN_CTR1_B, OUTPUT);
    pinMode(PIN_PWM1, OUTPUT); // Speed control

    // Motor_2 controll pin initiate;
    pinMode(PIN_CTR2_A, OUTPUT);
    pinMode(PIN_CTR2_B, OUTPUT);
    pinMode(PIN_PWM2, OUTPUT); // Speed control

    digitalWrite(PIN_CTR1_A, LOW);
    digitalWrite(PIN_CTR1_B, LOW); // Set the rotation of motor_1
    digitalWrite(PIN_CTR2_A, LOW);
    digitalWrite(PIN_CTR2_B, LOW); // Set the rotation of motor_2

    // Enable the Motor Shield output;
    pinMode(PIN_SD, OUTPUT);
    digitalWrite(PIN_SD, HIGH);

    pinMode(PIN_HALL1, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_HALL1), countPulse_1, FALLING);
    pinMode(PIN_HALL2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_HALL2), countPulse_2, FALLING);
}

void countPulse_1()
{
    count_0++;
}

void countPulse_2()
{
    count_1++;
}

// PWM 0~100
void set_motor(int index, int pwm, int dir)
{
    // Motor 0
    if (index == 0)
    {
        if (dir == 0)
        {
            digitalWrite(PIN_CTR1_A, HIGH);
            digitalWrite(PIN_CTR1_B, LOW);
        }
        else
        {
            digitalWrite(PIN_CTR1_A, LOW);
            digitalWrite(PIN_CTR1_B, HIGH);
        }

        analogWrite(PIN_PWM1, map(pwm, 0, 100, 0, 250));
    }
    // Motor 1
    if (index == 1)
    {
        if (dir == 0)
        {
            digitalWrite(PIN_CTR2_A, HIGH);
            digitalWrite(PIN_CTR2_B, LOW);
        }
        else
        {
            digitalWrite(PIN_CTR2_A, LOW);
            digitalWrite(PIN_CTR2_B, HIGH);
        }

        analogWrite(PIN_PWM2, map(pwm, 0, 100, 0, 250));
    }
}

void set_motor_slow(int index, int pwm, int dir)
{
    int step_unit = 10;
    // Motor 0
    if (index == 0)
    {
        if (now_dir_0 != dir)
        {
            if (now_pwm_0 > 0)
                now_pwm_0 -= step_unit;
            else
                now_dir_0 = dir;
        }
        else
        {
            if ((pwm - now_pwm_0) > step_unit)
            {
                now_pwm_0 += step_unit;
            }
            else if ((now_pwm_0 - pwm) > step_unit)
            {
                now_pwm_0 -= step_unit;
            }
            else
            {
                now_pwm_0 = pwm;
            }
        }

        set_motor(0, now_pwm_0, now_dir_0);
    }
    // Motor 1
    if (index == 1)
    {
        if (now_dir_1 != dir)
        {
            if (now_pwm_1 > 0)
                now_pwm_1 -= step_unit;
            else
                now_dir_1 = dir;
        }
        else
        {
            if ((pwm - now_pwm_1) > step_unit)
            {
                now_pwm_1 += step_unit;
            }
            else if ((now_pwm_1 - pwm) > step_unit)
            {
                now_pwm_1 -= step_unit;
            }
            else
            {
                now_pwm_1 = pwm;
            }
        }

        set_motor(1, now_pwm_1, now_dir_1);
    }
}