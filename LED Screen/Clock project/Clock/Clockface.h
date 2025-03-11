#ifndef MarioClockface_h
#define MarioClockface_h

#include "Super_Mario_Bros__24pt7b.h"

#include <Adafruit_GFX.h>
#include "Tile.h"
#include "Locator.h"
#include "Game.h"
#include "Object.h"
// Commons
#include "IClockface.h"
#include "CWDateTime.h"

#include "assets.h"
#include "mario.h"
#include "block.h"
#include "PKMN_RBYGSC4pt7b.h"

#include "pacman.h"
#include "hour_font.h"
#include "picopixel.h"

class Clockface: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    void updateTime();

  public:
    Clockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
    void externalEvent(int type);
};
//**********************************************************************************************
//**********************************************************************************************
//Clockface3************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
class Clockface3: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    void updateTime3();

  public:
    Clockface3(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
    void externalEvent3(int type);
};
//**********************************************************************************************
//**********************************************************************************************
//Clockface1************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
class Clockface1: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    //char minutes[3] = {0};

  public:
    Clockface1(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
    void refreshDate(uint8_t weekday, uint16_t color);
    void refreshTime();
    void updatePokemon();
    void updateLoadingBar(uint8_t seconds);
};
//**********************************************************************************************
//**********************************************************************************************
//Clockface2************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
class Clockface2: public IClockface {
  private:
    const int MAP_SIZE = 12;
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    bool pacmanState = true;
    bool show_seconds = true;

    const char* _weekDayWords = "SU\0MO\0TU\0WE\0TH\0FR\0SA\0";
    const char* _monthWords = "JAN\0FEB\0MAR\0APR\0MAY\0JUN\0JUL\0AUG\0SEP\0OCT\0NOV\0DEC\0";
    char weekDayTemp[4]= "\0";
    char monthTemp[4]= "\0";



    enum MapBlock {
      EMPTY = 0,
      FOOD = 1,
      WALL = 2,
      GATE = 3,
      SUPER_FOOD = 4,
      CLOCK = 5,
      GHOST = 6,
      PACMAN = 7,
      OUT_OF_MAP = 99
    };


    const byte _MAP_CONST[12][12] = {
      {4,1,1,1,1,1,7,1,1,1,1,4},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {3,1,5,5,5,5,5,5,5,5,1,3},
      {3,1,5,5,5,5,5,5,5,5,1,3},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {4,1,1,1,1,1,1,1,1,1,1,4}
    };

    
    byte _MAP[12][12] = {
      {4,1,1,1,1,1,7,1,1,1,1,4},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {3,1,5,5,5,5,5,5,5,5,1,3},
      {3,1,5,5,5,5,5,5,5,5,1,3},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {4,1,1,1,1,1,1,1,1,1,1,4}
    };


    const byte MAP_BORDER_SIZE = 2;
    const byte MAP_MIN_POS = 0 + MAP_BORDER_SIZE;
    const byte MAP_MAX_POS = 64 - MAP_BORDER_SIZE;

    // first elem is the size
    const int PACMAN_MOVING_BLOCKS[4] = {3, MapBlock::EMPTY, MapBlock::FOOD, MapBlock::GATE};
    const int PACMAN_BLOCKING_BLOCKS[4] = {3, MapBlock::OUT_OF_MAP, MapBlock::WALL, MapBlock::CLOCK};

    void drawMap();
    Clockface2::MapBlock nextBlock(Direction dir);
    Clockface2::MapBlock nextBlock();
    void turnRandom();
    int countBlocks(Clockface2::MapBlock elem);
    bool contains(int v, const int* values);
    void resetMap();
    void directionDecision(MapBlock nextBlk, bool moving_axis_x);
    void updateClock();
    const char* weekDayName(int weekday);
    const char* monthName(int month);
    
    
  public:
    Clockface2(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime);
    void update();
};
#endif
