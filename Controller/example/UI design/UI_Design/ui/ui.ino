/*
Library version:
Arduino IDE v2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0
lvgl v8.3.11
DHT sensor library v1.4.6
Adafruit Unified Sensor v1.1.14

Tools:
USB CDC On Boot: Enabled
Flash size:16MB(128Mb)
Partition Schrme:16M Flash(3MB APP/9.9MB FATFS)
PSRAM: OPI PSRAM
*/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include "touch.h"
#include "pin_config.h"
#include <DHT.h>

#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float h,t;
/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true); // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

int counter = 0;
int State;
int old_State;
int move_flag = 0;

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

    // encoder
    // static lv_indev_drv_t indev_drv2;
    // lv_indev_drv_init(&indev_drv2);
    // indev_drv2.type = LV_INDEV_TYPE_ENCODER;
    // indev_drv2.read_cb = my_encoder_read;
    // lv_indev_t *encoder_indev = lv_indev_drv_register(&indev_drv2);

    ui_init();
    dht.begin();
    Serial.println("Setup done");
}

long runtime = 0;

void loop()
{
    display_t_h_data();
    lv_timer_handler(); /* let the GUI do its work */
    delay(5);
}
//---------------------------------------------

void display_t_h_data()
{
    h = dht.readHumidity();
    t = dht.readTemperature();

    String tempStr = String(t, 0) + "°C";
    String humiStr = String(h, 0) + "%";

    lv_label_set_text(ui_Label1,tempStr.c_str());
    lv_label_set_text(ui_Label3,humiStr.c_str());

    lv_arc_set_value(ui_Arc1,t);
    lv_arc_set_value(ui_Arc2,h);
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

void my_encoder_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    if (counter > 0)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_LEFT;
        counter--;
        move_flag = 1;
    }

    else if (counter < 0)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key = LV_KEY_RIGHT;
        counter++;
        move_flag = 1;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}