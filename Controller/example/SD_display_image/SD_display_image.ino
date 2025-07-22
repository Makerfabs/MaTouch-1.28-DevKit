/*
Library version:
Arduino IDE v2.3.6
esp32 v3.2.0
GFX Library for Arduino v1.6.0
JPEGDecoder v2.0.0

Tools:
USB CDC On Boot: Enabled
*/
#include <Arduino_GFX_Library.h>
#include "touch.h"
#include <SD.h>
#include <JPEGDecoder.h>
#include <FS.h>

#define TFT_BLK 45
#define TFT_RES 21

#define SD_CS 43
#define TFT_CS 1
#define TFT_DC 46
#define MOSI 2
#define MISO 18
#define SCLK 42

#define TOUCH_INT 40
#define TOUCH_SDA 38
#define TOUCH_SCL 39

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, SCLK, MOSI, MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

int x = 0, y = 0;

uint8_t* jpgArray = nullptr;
size_t jpgArraySize = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(SD_CS, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH); //Disable display
  digitalWrite(SD_CS, LOW);   // Enable SD
  
  SPI.begin(SCLK, MISO, MOSI);
  
  if (!SD.begin(SD_CS))
  {
    Serial.println(F("ERROR: File System Mount Failed!"));
  }

  listDir(SD, "/", 0); // Read SD card files
  writeFile(SD, "/hello.txt", "Hello World!\n"); //Write a file
  readFile(SD, "/hello.txt"); //Read a file

  processJPG("/logo_240240.jpg");

  digitalWrite(SD_CS, HIGH);
  digitalWrite(TFT_CS, LOW);

  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  gfx->begin();
  gfx->fillScreen(WHITE);
  displayJPGFromArray(jpgArray, jpgArraySize);
}

void loop()
{
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\n", dirname);
    Serial.flush();
    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    Serial.flush();
}

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

bool isJPGByHeader(fs::FS &fs, const char* filePath) 
{
  File file = fs.open(filePath, FILE_READ);
  if (!file) 
  {
    Serial.println("Failed to open file!");
    return false;
  }

  uint8_t header[3];
  file.read(header, 3);
  file.close();

  return (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF);
}

bool saveJPGToArray(fs::FS &fs, const char* jpgPath) 
{
  if (jpgArray != nullptr) 
  {
    free(jpgArray);
    jpgArray = nullptr;
    jpgArraySize = 0;
  }

  File jpgFile = fs.open(jpgPath, FILE_READ);
  if (!jpgFile) 
  {
    Serial.println("Failed to open JPG file!");
    return false;
  }

  jpgArraySize = jpgFile.size();
  Serial.printf("JPG file size: %d bytes\n", jpgArraySize);

  jpgArray = (uint8_t*)malloc(jpgArraySize);
  if (jpgArray == nullptr) 
  {
    Serial.println("Memory allocation failed!");
    jpgFile.close();
    return false;
  }

  jpgFile.read(jpgArray, jpgArraySize);
  jpgFile.close();

  Serial.println("JPG successfully saved to global array.");
  return true;
}

void printJPGArray()
{
  if (jpgArray == nullptr || jpgArraySize == 0) 
  {
    Serial.println("JPG array is empty!");
    return;
  }

  Serial.println("const uint8_t jpgArray[] PROGMEM = {");
  for (size_t i = 0; i < jpgArraySize; i++) 
  {
    Serial.printf("0x%02X", jpgArray[i]);
    if (i < jpgArraySize - 1) 
      Serial.print(", ");
    if ((i + 1) % 16 == 0) 
      Serial.println();
  }
  Serial.println("};");
  Serial.printf("JPG array size: %d bytes\n", jpgArraySize);
}

void displayJPGFromArray(const uint8_t* jpgData, size_t jpgSize) 
{
  JpegDec.decodeArray(jpgData, jpgSize);

  uint16_t w = JpegDec.width;
  uint16_t h = JpegDec.height;
  Serial.printf("Image decoded: %dx%d\n", w, h);

  int xOffset = (gfx->width() - w) / 2;
  int yOffset = (gfx->height() - h) / 2;

  while (JpegDec.read()) 
  {
    for (uint16_t y = 0; y < JpegDec.MCUHeight; y++) 
    {
      for (uint16_t x = 0; x < JpegDec.MCUWidth; x++) 
      {
        int16_t drawX = JpegDec.MCUx * JpegDec.MCUWidth + x;
        int16_t drawY = JpegDec.MCUy * JpegDec.MCUHeight + y;

        if (drawX < w && drawY < h) 
        {
          uint16_t color = JpegDec.pImage[y * JpegDec.MCUWidth + x];
          gfx->drawPixel(drawX + xOffset, drawY + yOffset, color);
        }
      }
    }
  }
  Serial.println("JPG image rendered to screen.");
}

void processJPG(const char* jpgPath) 
{
  if (!isJPGByHeader(SD, jpgPath)) 
  {
    Serial.printf("File %s is not a valid JPG file.\n", jpgPath);
    return;
  }

  if (!saveJPGToArray(SD, jpgPath)) 
  {
    Serial.println("Error saving JPG to array!");
    return;
  }

  Serial.printf("JPG %s loaded successfully.\n", jpgPath);
}
