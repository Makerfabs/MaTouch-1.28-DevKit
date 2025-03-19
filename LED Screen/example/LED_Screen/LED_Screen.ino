/*
Author: Copper
Date:2025.2.20
Code version: V1.0.3

Library version:
Arduino IDE 2.3.4
esp32 V3.1.1
GFX Library for Arduino v1.5.3
lvgl v8.3.11
ESP32 HUB75 LED MATRIX PANEL DMA Display v3.0.11
Adafruit GFX Library v1.11.11
Adafruit BusIO v1.16.2
AnimatedGIF v2.1.1

Tools:
USB CDC On Boot: Enabled
Flash size: 16MB(128Mb)
Partition Schrme: 16M Flash(3MB APP/9.9MB FATFS)
PSRAM: OPI PSRAM
*/

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <ui.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>
#include "touch.h"
#include "pin_config.h"
#include <SD.h>
#include "SPI.h"


#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>



EventGroupHandle_t eventGroup;
#define BIT0  1 << 0
#define BIT1  1 << 1

EventBits_t bits = BIT1;

#define MAX_FILES 50
#define MAX_FILENAME_LENGTH 32

char fileNames[MAX_FILES][MAX_FILENAME_LENGTH];
char fileNameBuffer[1024];
int now_file = 0;
int file_sum = 0;

uint8_t* gifArray = nullptr;
size_t gifArraySize = 0;

int sd_update_flag = 0;
int txt_flag = 0;
String txt_content = "";
/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

const int panelResX = 256;  // Number of pixels wide of each INDIVIDUAL panel module.
const int panelResY = 64;   // Number of pixels tall of each INDIVIDUAL panel module.
const int panel_chain = 1;  // Total number of panels chained one to another

MatrixPanel_I2S_DMA *dma_display = nullptr;
AnimatedGIF gif;
File f;

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);  // Constructor
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES,  2 /* rotation */, true /* IPS */);

int counter = 0;
int State;
int old_State;
int move_flag = 0;

int brightness = 100;

// LVGL Global Value
int page_index = 0;

void setup() 
{
  Serial.begin(115200); /* prepare for possible serial debug */

  pin_init();
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  Wire.begin(TOUCH_SDA, TOUCH_SCL);

  SPI_ON_SD;
  if (!SD.begin(SD_CS, SPI, 80000000))
    Serial.println(F("ERROR: File System Mount Failed!"));

  listDir(SD, "/", 0); // Read SD card files
  processGIF(fileNames[now_file]); // Read the first file

  SPI_OFF_SD;
  Serial.println("SD init finish.");

  SPI_ON_TFT;
  gfx->begin();

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
  dma_display->fillScreen(dma_display->color565(0, 0, 0));
  dma_display->setBrightness(map(brightness, 0, 100, 0, 255));
  dma_display->setRotation(2);

  gif.begin(GIF_PALETTE_RGB565_LE);

  Serial.println("TFT init over");
  Serial.println("Setup done");

  eventGroup = xEventGroupCreate();  // 创建事件组

  xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 4096, NULL, 3, NULL, 1);
  xTaskCreatePinnedToCore(Task_Main, "Task_Main", 10240, NULL, 2, NULL, 1);
  //display_text("test");
  delay(2500);
  xTaskCreatePinnedToCore(Task_Gif_and_text, "Task_Gif_and_text", 10240, NULL, 1, NULL, 1);
  EventBits_t currentBits = xEventGroupClearBits(eventGroup, BIT0 | BIT1);
  xEventGroupSetBits(eventGroup, BIT1);  // 设置事件位 BIT0

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
    // encoder_func();
    if(sd_update_flag == 0)
    {
      obj_update();
      Serial.print("obj_update:");
      Serial.println(fileNames[now_file]);
      vTaskDelay(400);
    }
    else if(sd_update_flag == 1)
    {
      sd_updata();
      Serial.print("sd_updata:");
      Serial.println(fileNames[now_file]);
      vTaskDelay(10);
    }
  }
}

void Task_Gif_and_text(void *pvParameters) // Light panel display
{
  while(1)
  {

    bits = xEventGroupWaitBits(eventGroup, BIT0 | BIT1, pdFALSE, pdFALSE, 0);  

    dma_display->fillScreen(dma_display->color565(0, 0, 0)); // Fill the display with black

    if(bits & BIT0) // txt file display
    {
    //   while(1)
    //   {
        display_text(txt_content);
        // if(sd_update_flag == 1)
        // {
        //     break;
        // }
    //   }
    }
    else if(bits & BIT1) // gif file display
    {
        for (int j = 0; j < 11; j++)
        {
            if (gif.open((uint8_t *)gifArray, gifArraySize, GIFDraw))
            {
                while (gif.playFrame(true, NULL)) // Play all frames of the GIF until playback is complete
                {
                    if(sd_update_flag == 1)
                    {
                        break;
                    }
                }
                gif.close();            
                
            } 
            else 
            {
                Serial.printf("Error opening file = %d, file name: %s\n", gif.getLastError(), fileNames[now_file]);
                break;
            }
        }
    }
    vTaskDelay(500);

  }
}

//---------------------------------------------
void sd_updata() // Read sd card
{

  switch_to_SD();
  if(isTXTByExtension(fileNames[now_file]))
  {
    // txt_flag=1;
    EventBits_t currentBits = xEventGroupClearBits(eventGroup, BIT0 | BIT1);
    xEventGroupSetBits(eventGroup, BIT0);  // 设置事件位 BIT0
    readTextFromSD(fileNames[now_file]);
    switch_to_TFT();
  }
  else
  {
    // txt_flag=0;
    EventBits_t currentBits = xEventGroupClearBits(eventGroup, BIT0 | BIT1);
    xEventGroupSetBits(eventGroup, BIT1);  // 设置事件位 BIT0
    processGIF(fileNames[now_file]);
    switch_to_TFT();
  }
  sd_update_flag = 0;    
}

void encoder_irq() // knob interruption
{
  State = digitalRead(ENCODER_CLK);
  if (State != old_State)
  {
    if (digitalRead(ENCODER_DT) == State)
      counter++;
    else 
      counter--;

    encoder_func();
    if(page_index == 0)
      sd_update_flag=1; //Reread sd card    

    Serial.println("Encoder_irq Detection");
  }
  old_State = State;  // the first position was changed
}

void encoder_func() 
{
  if (page_index == 0) 
  {
    if (counter != 0) 
    {
      now_file += counter;
      counter = 0;
      if (now_file > file_sum - 1)
        now_file = 0;
      if (now_file < 0)
        now_file = file_sum - 1;
    }
  } 
  else if (page_index == 1) 
  {
    if (counter != 0) 
    {
      brightness += counter*5;
      counter = 0;

      if (brightness < 0)
        brightness = 0;
      if (brightness > 100)
        brightness = 100;

      dma_display->setBrightness(map(brightness, 0, 100, 0, 255));  // The map() function maps luminance values from a percentage (0% to 100%) to the hardware luminance range (0 to 255)
    }
  }
}

void obj_update() // ui variable update
{
  char temp[80] = "";
  if (page_index == 0) // Update the screen display
  {
    lv_roller_set_selected(ui_Roller1, now_file, LV_ANIM_ON);
  }

  if (page_index == 1) // Update brightness
  {
    lv_arc_set_value(ui_Arc1, brightness);

    sprintf(temp, "%d%%", brightness);
    lv_label_set_text(ui_Label1, temp);
  }
}

void pin_init() 
{
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  old_State = digitalRead(ENCODER_CLK);

  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  SPI_OFF_SD;
  SPI_OFF_TFT;

  attachInterrupt(ENCODER_CLK, encoder_irq, CHANGE);
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
    //Serial.printf("Touch detected: X = %d, Y = %d\n", data->point.x, data->point.y);
  } else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

// Light panel display initialisation
void displaySetup() 
{
  HUB75_I2S_CFG mxconfig(
    panelResX,  // module width
    panelResY,  // module height
    panel_chain);

  mxconfig.gpio.r1 = 10;
  mxconfig.gpio.g1 = 11;
  mxconfig.gpio.b1 = 12;
  mxconfig.gpio.r2 = 13;
  mxconfig.gpio.g2 = 14;
  mxconfig.gpio.b2 = 15;
  mxconfig.gpio.a = 4;
  mxconfig.gpio.b = 5;
  mxconfig.gpio.c = 6;
  mxconfig.gpio.d = 7;
  mxconfig.gpio.e = 16;
  mxconfig.gpio.lat = 9;
  mxconfig.gpio.oe = 44;
  mxconfig.gpio.clk = 8;

  mxconfig.clkphase = false;
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
}

uint16_t rbg2rgb(uint16_t input) 
{
  return input;

  // uint16_t r = (input >> 11) & 0x1F; // 0001 1111
  // uint16_t b = (input >> 6) & 0x1F;
  // uint16_t g = input & 0x1F;

  // return ((r << 11) | (g << 6) | b);
}

void *GIFOpenFile(const char *fname, int32_t *pSize)
{
  f = SD.open(fname);
  if (f) 
  {
    *pSize = f.size();
    return (void *)&f;
  }
  return NULL;
}

void GIFCloseFile(void *pHandle)
{
  File *f = static_cast<File *>(pHandle);
  if (f != NULL)
    f->close();
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  int32_t iBytesRead;
  iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);

  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos - 1;
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition)
{
  int i = micros();
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  i = micros() - i;
  return pFile->iPos;
}

void GIFDraw(GIFDRAW *pDraw) 
{
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;  // current line

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2)  // restore to background color
  {
    for (x = 0; x < iWidth; x++) 
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }
  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency)  // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    int x, iCount;
    pEnd = s + pDraw->iWidth;
    x = 0;
    iCount = 0;  // count non-transparent pixels
    while (x < pDraw->iWidth)
    {
      c = ucTransparent - 1;
      d = usTemp;
      while (c != ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)  // done, stop
        {
          s--;  // back up to treat it like transparent
        } else  // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      }            // while looking for opaque pixels
      if (iCount)  // any opaque pixels?
      {
        for (int xOffset = 0; xOffset < iCount; xOffset++)
        {
          dma_display->drawPixel(x + xOffset, y, rbg2rgb(usTemp[xOffset]));
        }
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          iCount++;
        else
          s--;
      }
      if (iCount)
      {
        x += iCount;  // skip these
        iCount = 0;
      }
    }
  } else 
  {
    s = pDraw->pPixels;
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    for (x = 0; x < pDraw->iWidth; x++) 
    {
      dma_display->drawPixel(x, y, rbg2rgb(usPalette[*s++]));
    }
  }
}


void listDir(fs::FS &fs, const char *dirname, uint8_t levels) 
{
  //Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) 
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) 
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  bool isFirstFile = true; // Used to determine if it is the first file

  while (file) 
  {
    if (file.isDirectory()) //check if the opened file is a directory
    {
      //Serial.print("  DIR : ");
      //Serial.println(file.name());
      if (levels)
        listDir(fs, file.path(), levels - 1);
    }
    else
    {
      snprintf(fileNames[file_sum], MAX_FILENAME_LENGTH, "/%s", file.name()); // Save filenames to an array
      fileNames[file_sum][MAX_FILENAME_LENGTH - 1] = '\0';  // Make sure the string ends in '0'

      if (!isFirstFile) // If it's not the first file, add a newline before it
        strcat(fileNameBuffer, "\n");
      else
        isFirstFile = false;

      strcat(fileNameBuffer, file.name()); // Construct the string that defines the Roller options.
      file_sum++;
    }
    file = root.openNextFile();
  }
}


void my_check() // Verify sd card read
{
  Serial.println("Stored file names:"); // Print the name of the stored file
  for (int i = 0; i < file_sum; i++) 
  {
    Serial.println(fileNames[i]);
  }

  Serial.println("Files on SD card:"); // Print the entire string
  Serial.println(fileNameBuffer);
}

void switch_to_SD() 
{
  SPI_OFF_TFT;
  SPI.end();
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
  Serial.println("Switching to SD...");
  SPI_ON_SD;

  if (!SD.begin(SD_CS))
    Serial.println(F("ERROR: File System Mount Failed!"));

  vTaskDelay(10);
}

void switch_to_TFT() 
{
  SPI_OFF_SD;
  Serial.println("Switching to TFT...");
  SPI_ON_TFT;
  vTaskDelay(10);
  gfx->begin();
}

bool isGIFByHeader(fs::FS &fs, const char* filePath) 
{
  File file = fs.open(filePath, FILE_READ);
  if (!file) 
  {
    Serial.println("Failed to open file!");
    return false;
  }

  char header[6];
  file.readBytes(header, 6);
  file.close();

  return (strncmp(header, "GIF87a", 6) == 0 || strncmp(header, "GIF89a", 6) == 0);
}

// Read the contents of a GIF file into a byte array and save it.
bool saveGIFToArray(fs::FS &fs, const char* gifPath) 
{
  if (gifArray != nullptr) 
  {
    free(gifArray);
    gifArray = nullptr;
    gifArraySize = 0;
  }

  File gifFile = fs.open(gifPath, FILE_READ);
  if (!gifFile) 
  {
    Serial.println("Failed to open GIF file!");
    return false;
  }

  gifArraySize = gifFile.size();
  Serial.printf("GIF file size: %d bytes\n", gifArraySize);

  gifArray = (uint8_t*)malloc(gifArraySize);
  if (gifArray == nullptr) 
  {
    Serial.println("Memory allocation failed!");
    gifFile.close();
    return false;
  }

  gifFile.read(gifArray, gifArraySize);
  gifFile.close();

  Serial.println("GIF successfully saved to global array.");
  return true;
}

// Print the contents of the GIF array
void printGIFArray()
{
  if (gifArray == nullptr || gifArraySize == 0) 
  {
    Serial.println("GIF array is empty!");
    return;
  }

  Serial.println("const uint8_t gifArray[] PROGMEM = {");
  for (size_t i = 0; i < gifArraySize; i++) 
  {
    Serial.printf("0x%02X", gifArray[i]);
    if (i < gifArraySize - 1) 
      Serial.print(", ");
    if ((i + 1) % 16 == 0) 
      Serial.println();  // 16 bytes per line
  }
  Serial.println("};");
  Serial.printf("GIF array size: %d bytes\n", gifArraySize);
}

// Check the file and save it to an array
void processGIF(const char* gifPath) 
{
  if (!isGIFByHeader(SD, gifPath)) 
  {
    Serial.printf("File %s is not a valid GIF file.\n", gifPath);
    return;
  }

  if (!saveGIFToArray(SD, gifPath)) 
  {
    Serial.println("Error saving GIF to array!");
    return;
  }

  Serial.printf("GIF %s saved to global array successfully.\n", gifPath);
}

void display_text(String txt)
{
  dma_display->setTextSize(1);     // size 1 == 8 pixels high
  dma_display->setCursor(5, 30);    // start at (5,30)
  dma_display->setTextColor(dma_display->color444(15,15,15));
  dma_display->println(txt);
}


bool isTXTByExtension(const char* filePath)
{
    String fname = String(filePath);
    fname.toLowerCase();

    if (fname.endsWith(".txt")) {
        Serial.println(String(filePath) + " is a TXT file.");
        return true;
    } else {
        Serial.println(String(filePath) + " is NOT a TXT file.");
        return false;
    }
}

void readTextFromSD(const char *filename)
{
    txt_content = "";

    File file = SD.open(filename);
    if (!file) {
        Serial.println("Failed to open file!");
    }

    while (file.available()) {
        txt_content += (char)file.read();
    }
    file.close();

    Serial.println("Text:" + txt_content);
}
