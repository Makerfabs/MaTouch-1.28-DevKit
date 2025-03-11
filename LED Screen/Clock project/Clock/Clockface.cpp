#include "Clockface.h"

//Clockface
EventBus eventBus;

const char* FORMAT_TWO_DIGITS = "%02d";

// Graphical
Tile ground(GROUND, 8, 8); 

Object bush(BUSH, 21, 9);
Object cloud1(CLOUD1, 13, 12);  // was 13, 12
Object cloud2(CLOUD2, 13, 12);   // was 13, 12
Object cloud3(CLOUD3, 17, 6);
Object hill(HILL, 20, 22);

Mario mario(21, 40);       //was 26
Block hourBlock(3, 8);
Block minuteBlock(22, 8);
Block secondBlock(41, 8);

unsigned long lastMillis = 0;
int lastSecond;

//Clockface3
// Graphical elements
Tile ground3(GROUND, 8, 8); 

Object bush3(BUSH, 21, 9);
Object cloud13(CLOUD1, 13, 12);
Object cloud23(CLOUD2, 13, 12);
Object hill3(HILL, 20, 22);


Mario mario3(23, 40);
Block hourBlock3(13, 8);
Block minuteBlock3(32, 8);

unsigned long lastMillis3 = 0;
int lastMinute;

//Clockface1
#define LIGHT_GREEN 0x754d
#define DARK_GREEN 0x0264
#define DARK_BLUE 0x016D
#define LIGHT_BLUE 0x24fe
#define LIGHT_BLACK 0X10c4
unsigned long lastMillis1 = 0;
const uint16_t* const pokemons[] PROGMEM = {pokemon1, pokemon2, pokemon3, pokemon4, pokemon5, pokemon6, pokemon7,};

//Clockface2
unsigned long lastMillisMin2 = 0;
unsigned long lastMillisTime2 = 0;
unsigned long lastMillisSec2 = 0;
Pacman *pacman;

//**********************************************************************************************
Clockface::Clockface(Adafruit_GFX* display) 
{
  _display = display;

  Locator::provide(display);
  Locator::provide(&eventBus);
}

Clockface1::Clockface1(Adafruit_GFX* display) 
{
  _display = display;

  Locator::provide(display);
  Locator::provide(&eventBus);
}

Clockface2::Clockface2(Adafruit_GFX* display) 
{
  _display = display;

  Locator::provide(display);
  Locator::provide(&eventBus);
}

Clockface3::Clockface3(Adafruit_GFX* display) 
{
  _display = display;

  Locator::provide(display);
  Locator::provide(&eventBus);
}
//**********************************************************************************************
//**********************************************************************************************
//Clockface*************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
void Clockface::setup(CWDateTime *dateTime) 
{
  _dateTime = dateTime;

  Locator::getDisplay()->setFont(&Super_Mario_Bros__24pt7b);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, SKY_COLOR);

  ground.fillRow(DISPLAY_HEIGHT - ground._height);

  bush.draw(43, 47);
  hill.draw(0, 34);
  cloud1.draw(0, 25);
  cloud2.draw(51, 30);
  cloud3.draw(30, 0);

  updateTime();

  hourBlock.init();
  minuteBlock.init();
  secondBlock.init();
  mario.init();
}

//**********************************************************************************************
void Clockface::update() 
{
  int updatedSecond;
  
  hourBlock.update();
  minuteBlock.update();
  secondBlock.update();
  mario.update();
   
  updatedSecond = _dateTime->getSecond();
 
  if (lastSecond != updatedSecond) 
    {    
    mario.jump(); 
    updateTime();      
    //lastMillis = millis();
    lastSecond = updatedSecond;
    //Serial.println(_dateTime->getFormattedTime());
    }
}

//**********************************************************************************************
void Clockface::externalEvent(int type) 
{
  if (type == 0) 
    {  //TODO create an enum
    mario.jump();
    updateTime();
    }
}
//**********************************************************************************************
void Clockface::updateTime() 
{
  hourBlock.setText(String(_dateTime->getHour(FORMAT_TWO_DIGITS)));
  minuteBlock.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
  secondBlock.setText(String(_dateTime->getSecond(FORMAT_TWO_DIGITS)));
}
//**********************************************************************************************
//**********************************************************************************************
//Clockface3  none second***********************************************************************
//**********************************************************************************************
//**********************************************************************************************
void Clockface3::setup(CWDateTime *dateTime) {
  _dateTime = dateTime;

  Locator::getDisplay()->setFont(&Super_Mario_Bros__24pt7b);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, SKY_COLOR);

  ground3.fillRow(DISPLAY_HEIGHT - ground3._height);

  bush3.draw(43, 47);
  hill3.draw(0, 34);
  cloud13.draw(0, 21);
  cloud23.draw(51, 7);

  updateTime3();

  hourBlock3.init();
  minuteBlock3.init();
  mario3.init();
}

void Clockface3::update() {

  hourBlock3.update();
  minuteBlock3.update();
  mario3.update();
  if (_dateTime->getSecond() == 0 && millis() - lastMillis3 > 1000) {

    mario3.jump();
    updateTime3();
    minuteBlock3.init();
    lastMillis3 = millis();
    Serial.println("Update Minute");
    //Serial.println(_dateTime->getFormattedTime());
  }
}

void Clockface3::updateTime3() {
  minuteBlock3.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
  hourBlock3.setText(String(_dateTime->getHour(FORMAT_TWO_DIGITS)));
  minuteBlock3.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
}

void Clockface3::externalEvent3(int type) {
  if (type == 0) {  //TODO create an enum
    mario3.jump();
    updateTime3();
  }
}
//**********************************************************************************************
//**********************************************************************************************
//Clockface1************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
void Clockface1::setup(CWDateTime *dateTime) 
{
  _dateTime = dateTime;

  //randomSeed(dateTime->getMilliseconds() + millis());

  // Clear screen
  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);

  // Draw background
  Locator::getDisplay()->drawRGBBitmap(0, 0, POKEDEX_BG, 64, 64);

  Locator::getDisplay()->setFont(&PKMN_RBYGSC4pt7b);

  refreshTime();
  refreshDate(_dateTime->getWeekday(), DARK_BLUE);
  updatePokemon();
}

void Clockface1::update() 
{
  if (millis() - lastMillis1 >= 1000) {

    uint8_t seconds = _dateTime->getSecond();

    if (seconds == 0) {
      refreshTime();
    }
    updatePokemon();
    if (_dateTime->getMinute() == 0 && _dateTime->getSecond() == 0) {
      uint8_t wd = this->_dateTime->getWeekday();
      Serial.println(wd);
      //clean up the previous square
      refreshDate((wd == 0 ? 6 : wd-1), LIGHT_BLUE);
      // update date
      refreshDate(wd, DARK_BLUE);
    }

    updateLoadingBar(seconds);

    // Update blink seconds
    uint16_t color = random(LONG_MAX);
    Locator::getDisplay()->fillRect(5, 4, 2, 4, color);
    Locator::getDisplay()->fillRect(4, 5, 4, 2, color);

    lastMillis1 = millis();
  }
}

void Clockface1::updatePokemon() { 
  Locator::getDisplay()->drawRGBBitmap(8, 21, pokemons[random(sizeof(pokemons)/4)], 16, 16);
  //Locator::getDisplay()->drawRGBBitmap(8, 21, pokemons[1], 16, 16);
}

void Clockface1::refreshTime() { 

  // Clean up the clock area
  Locator::getDisplay()->fillRect(35, 17, 26, 14, LIGHT_BLACK);
  Locator::getDisplay()->setTextColor(0xffff);//white
  //snprintf(minutes, sizeof(minutes), "%02d", _dateTime->getMinute());

  Locator::getDisplay()->setCursor(35, 22);
  Locator::getDisplay()->print(_dateTime->getHour(FORMAT_TWO_DIGITS));

  Locator::getDisplay()->setCursor(46, 30);
  //Locator::getDisplay()->print(minutes);
  Locator::getDisplay()->print(_dateTime->getMinute(FORMAT_TWO_DIGITS));

  /*if (!_dateTime->is24hFormat())
    Locator::getDisplay()->drawBitmap(55, 18, (_dateTime->isAM() ? AM_SIGN : PM_SIGN), 4, 4, 0xffff);*/
}

void Clockface1::refreshDate(uint8_t weekday, uint16_t color) {
  // Update weekday
  uint8_t x = 36 + ((weekday > 3 ? (weekday-4) : weekday) * 6);
  uint8_t y = 35 + (weekday > 3 ? 5 : 0);

  Locator::getDisplay()->fillRect(x, y, 5, 4, color);
}

void Clockface1::updateLoadingBar(uint8_t seconds) {

  if (seconds == 0) {
    Locator::getDisplay()->fillRect(9, 53, 11, 5, LIGHT_GREEN);
  } else {
    Locator::getDisplay()->fillRect(9, 53, ((10*seconds) / 59)+1, 5, DARK_GREEN);
  }
}
//**********************************************************************************************
//**********************************************************************************************
//Clockface2************************************************************************************
//**********************************************************************************************
//**********************************************************************************************
void Clockface2::setup(CWDateTime *dateTime) {
  _dateTime = dateTime;
  Locator::getDisplay()->setFont(&hourFont);
  //randomSeed(dateTime->getMilliseconds() + millis());
  drawMap();
  updateClock();
}

void Clockface2::update()
{
  // Seconds blink  
  if ((millis() - lastMillisSec2) >= 1000) {
    
    uint8_t seconds = _dateTime->getSecond();

    if (seconds == 0) {
      updateClock();
    }

    if (show_seconds) {
      Locator::getDisplay()->fillRect(31, 24, 2, 2, 0xFE40);
      Locator::getDisplay()->fillRect(31, 29, 2, 2, 0xFE40);
    } else  {
      Locator::getDisplay()->fillRect(31, 24, 2, 2, 0);
      Locator::getDisplay()->fillRect(31, 29, 2, 2, 0);
    }

    show_seconds = !show_seconds;
    lastMillisSec2 = millis();
  }

  // Clock
  /*if (millis() - lastMillisTime2 >= 60000) {
    updateClock();
    
    //lastMillisTime2 = millis();
  }*/

  // Pacman
  if (millis() - lastMillis >= 75) {
    
    bool fullBlock = // X axis
                     ((pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT) && (pacman->getX()-2) % 5 == 0) ||
                     // Y axis
                     ((pacman->_direction == Direction::UP || pacman->_direction == Direction::DOWN) && (pacman->getY()-2) % 5 == 0);

    
    if (fullBlock) {
      
      MapBlock nextBlk = nextBlock();

      //change block to empty where pacman passes
      _MAP[(pacman->getY()-2)/5][(pacman->getX()-2)/5] = MapBlock::EMPTY;

      directionDecision(nextBlk, (pacman->_direction == Direction::LEFT || pacman->_direction == Direction::RIGHT));

      if (nextBlk == MapBlock::SUPER_FOOD) {
        pacman->setState(Pacman::State::INVENCIBLE);
      }

      if (countBlocks(MapBlock::FOOD) == 0) {
        resetMap();
      }
    }
    pacman->update();

    lastMillis = millis();
  }
}

void Clockface2::drawMap() 
{
  //Locator::getDisplay()->drawRGBBitmap(0, 0, _PACMAN_MAP, 64, 64);
  //Locator::getDisplay()->getPixel(0, 0);

  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);

  uint16_t food_color = 0xB58C;
  uint16_t wall_color = 0x0016;
  uint16_t spcfood_color = 0xFBE0;

  Locator::getDisplay()->drawRect(0,0,64,64,wall_color);
  Locator::getDisplay()->drawRect(1,1,62,62,wall_color);

  for (int i=0; i<MAP_SIZE; i++) {
    for (int j=0; j<MAP_SIZE; j++) {
      if (_MAP[j][i] == MapBlock::FOOD) {
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+4,3,1,food_color);
      } else if (_MAP[j][i] == MapBlock::WALL) {
        Locator::getDisplay()->fillRect((i*5)+2,(j*5)+2,5,5,wall_color);
      } else if (_MAP[j][i] == MapBlock::CLOCK) {
        Locator::getDisplay()->fillRect((i*5)+2,(j*5)+2,5,5,wall_color);
      } else if (_MAP[j][i] == MapBlock::GATE) {
        //Locator::getDisplay()->fillRect((i*5)+((bool)i*2),(j*5)+2,7,5,0x0000);
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+4,3,1,food_color);
      } else if (_MAP[j][i] == MapBlock::SUPER_FOOD) {
        Locator::getDisplay()->fillRect((i*5)+3,(j*5)+3,3,3,spcfood_color);
      } else if (_MAP[j][i] == MapBlock::PACMAN) {
        pacman = new Pacman((i*5)+2,(j*5)+2);

        // Locator::getDisplay()->drawRGBBitmap((i*5)+2,(j*5)+2, _PACMAN_2, 5, 5);
        // pacmanState = !pacmanState;
      }
    }
  }  
}

void Clockface2::updateClock() {

  Locator::getDisplay()->fillRect(14, 19, 36, 26, 0x0000);

  Locator::getDisplay()->setFont(&Picopixel);
  Locator::getDisplay()->setTextColor(0xAD55);
  Locator::getDisplay()->setCursor(15, 41);
  Locator::getDisplay()->print(monthName(_dateTime->getMonth()));
  Locator::getDisplay()->print(" ");
  Locator::getDisplay()->print(_dateTime->getDay());
  Locator::getDisplay()->print(" ");
  Locator::getDisplay()->print(weekDayName(_dateTime->getWeekday()));
  
  Locator::getDisplay()->setFont(&hourFont);
  
  Locator::getDisplay()->setTextColor(0xFE40);
  Locator::getDisplay()->setCursor(15, 28);
  
  Locator::getDisplay()->print(_dateTime->getHour(FORMAT_TWO_DIGITS));
  Locator::getDisplay()->print(" ");
  Locator::getDisplay()->print(_dateTime->getMinute(FORMAT_TWO_DIGITS));
}

Clockface2::MapBlock Clockface2::nextBlock() {
  return nextBlock(pacman->_direction);
}

Clockface2::MapBlock Clockface2::nextBlock(Direction dir) {

  Clockface2::MapBlock map_block = Clockface2::MapBlock::OUT_OF_MAP;

  if (dir == Direction::RIGHT) {
    if (pacman->getX()+pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)+1]);
    }
    
  } else if (dir == Direction::DOWN) {
    if (pacman->getY()+pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[((pacman->getY()-2)/5)+1][(pacman->getX()-2)/5]);
    }
  } else if (dir == Direction::LEFT) {

    if ((pacman->getX()-2) > 0) {
      map_block = static_cast<MapBlock>(_MAP[(pacman->getY()-2)/5][((pacman->getX()-2)/5)-1]);
    }

  } else if (dir == Direction::UP) {
    if ((pacman->getY()-2) > 0) {
      map_block = static_cast<MapBlock>(_MAP[((pacman->getY()-2)/5)-1][((pacman->getX()-2)/5)]);
    }
  }

  return map_block;

}

const char* Clockface2::weekDayName(int weekday) {
  strncpy(weekDayTemp, _weekDayWords + (weekday*3), 3);
  return weekDayTemp;
}

const char* Clockface2::monthName(int month) {
  strncpy(monthTemp, _monthWords + ((month-1)*4), 4);
  return monthTemp;
}

void Clockface2::resetMap() {

  memcpy( _MAP, _MAP_CONST, sizeof(_MAP_CONST) );
  drawMap();
  updateClock();
}

void Clockface2::directionDecision(MapBlock nextBlk, bool moving_axis_x) {

  // Serial.print("Next Block: ");
  // Serial.println(nextBlk);

  if (contains(nextBlk, PACMAN_BLOCKING_BLOCKS)) {
    turnRandom();
  } else if (moving_axis_x && contains(nextBlock(Direction::DOWN), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    pacman->turn(Direction::DOWN);
  } else if (moving_axis_x && contains(nextBlock(Direction::UP), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    pacman->turn(Direction::UP);
  } else if (!moving_axis_x && contains(nextBlock(Direction::LEFT), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    pacman->turn(Direction::LEFT);
  } else if (!moving_axis_x && contains(nextBlock(Direction::RIGHT), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    pacman->turn(Direction::RIGHT);
  }

}

bool Clockface2::contains(int v, const int* values) {
  
  for (int i = 1; i<values[0]+1; i++) {
    if (v == values[i])
      return true;
  }

  return false;
}

void Clockface2::turnRandom() {
  int dir = random(4);
  //int dir = 3;
  //pacman->_state = Pacman::State::TURNING;

  do {
    pacman->turn(static_cast<Direction>(dir));
    dir = random(4);
    //dir++;

    
  } while (!contains(nextBlock(), PACMAN_MOVING_BLOCKS));

  Serial.print("New direction: ");
  Serial.println(pacman->_direction);
}

int Clockface2::countBlocks(Clockface2::MapBlock elem) {
  int count = 0;
  for (int i = 0; i<MAP_SIZE; i++) {
    for (int j = 0; j<MAP_SIZE; j++) {
      if (_MAP[i][j] == elem)
        count++;
    }
  }

  return count;
}