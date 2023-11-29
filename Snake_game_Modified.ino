//MADE BY HOX DIPAN NEW
// VISIT YouTube.com/HoXDipannew

// Libraries

#include <LiquidCrystal.h> //Install LiquidCrystal_I2C.h
#include <stdlib.h> //Install stdlib.h
#include <limits.h> //Install limits.h

// Macros
#define RS A5
#define EN 13
#define D4 12
#define D5 11
#define D6 10
#define D7 9

#define F_CPU 16000000UL
#define BUTTON_LEFT  2 //LEFT BUTTON
#define BUTTON_RIGHT 0 // RIGHT BUTTON
#define GameSwitch   7 // Swtich Game Button

#define DEBOUNCE_DURATION 20

#define GRAPHIC_WIDTH 16
#define GRAPHIC_HEIGHT 4

#define PIN_BUTTON BUTTON_RIGHT
#define PIN_AUTOPLAY BUTTON_LEFT


#define SPRITE_RUN1 1
#define SPRITE_RUN2 2
#define SPRITE_JUMP 3
#define SPRITE_JUMP_UPPER '.'         // Use the '.' character for the head
#define SPRITE_JUMP_LOWER 4
#define SPRITE_TERRAIN_EMPTY ' '      // User the ' ' character
#define SPRITE_TERRAIN_SOLID 5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT 7

#define HERO_HORIZONTAL_POSITION 1    // Horizontal position of hero on screen

#define TERRAIN_WIDTH 16
#define TERRAIN_EMPTY 0
#define TERRAIN_LOWER_BLOCK 1
#define TERRAIN_UPPER_BLOCK 2

#define HERO_POSITION_OFF 0          // Hero is invisible
#define HERO_POSITION_RUN_LOWER_1 1  // Hero is running on lower row (pose 1)
#define HERO_POSITION_RUN_LOWER_2 2  //                              (pose 2)

#define HERO_POSITION_JUMP_1 3       // Starting a jump
#define HERO_POSITION_JUMP_2 4       // Half-way up
#define HERO_POSITION_JUMP_3 5       // Jump is on upper row
#define HERO_POSITION_JUMP_4 6       // Jump is on upper row
#define HERO_POSITION_JUMP_5 7       // Jump is on upper row
#define HERO_POSITION_JUMP_6 8       // Jump is on upper row
#define HERO_POSITION_JUMP_7 9       // Half-way down
#define HERO_POSITION_JUMP_8 10      // About to land

#define HERO_POSITION_RUN_UPPER_1 11 // Hero is running on upper row (pose 1)
#define HERO_POSITION_RUN_UPPER_2 12 //                              (pose 2)

// Userdefined types 

enum DisplayItem {GRAPHIC_ITEM_NONE, GRAPHIC_ITEM_A, GRAPHIC_ITEM_B,
GRAPHIC_ITEM_NUM};
struct Pos 
{
  uint8_t x=0, y=0;
};
enum {SNAKE_LEFT,SNAKE_UP,SNAKE_RIGHT,SNAKE_DOWN} snakeDirection;
enum {GAME_MENU, GAME_PLAY, GAME_LOSE, GAME_WIN} gameState;

// Global Variables
byte mode = 0;
byte modeOld = 0;
LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);
static char terrainUpper[TERRAIN_WIDTH + 1];
static char terrainLower[TERRAIN_WIDTH + 1];
static bool buttonPushed = false;
unsigned long debounceCounterButtonLeft = 0;
unsigned long debounceCounterButtonRight = 0;
struct Pos snakePosHistory[GRAPHIC_HEIGHT*GRAPHIC_WIDTH];
size_t snakeLength = 0;

byte block[3] = {
  B01110,
  B01110,
  B01110,
};
byte apple[3] = {
  B00100,
  B01010,
  B00100,
};
struct Pos applePos;
unsigned long lastGameUpdateTick = 0;
unsigned long gameUpdateInterval = 1000;
bool thisFrameControlUpdated = false;
uint8_t graphicRam[GRAPHIC_WIDTH*2/8][GRAPHIC_HEIGHT];

// Sub program

//Return true if the actual output value is true
bool debounce_activate(unsigned long* debounceStart)
{
  if(*debounceStart == 0)
  *debounceStart = millis();
  else if(millis()-*debounceStart>DEBOUNCE_DURATION)
  return true;
  return false;
}
//Return true if it's rising edge/falling edge
bool debounce_activate_edge(unsigned long* debounceStart)
{
  if(*debounceStart == ULONG_MAX)
  {
    return false;
  }
  else if(*debounceStart == 0)
  {
    *debounceStart = millis();
    }else if(millis()-*debounceStart>DEBOUNCE_DURATION){
    *debounceStart = ULONG_MAX;
    return true;
  }
  return false;
}
void debounce_deactivate(unsigned long* debounceStart){
  *debounceStart = 0;
}

void graphic_generate_characters()
{
  /*
  space: 0 0
  0: 0 A
  1: 0 B
  2: A 0
  3: A A
  4: A B
  5: B 0
  6: B A
  7: B B
  */
  for (size_t i=0; i<8; i++) {
    byte glyph[8];
    int upperIcon = (i+1)/3;
    int lowerIcon = (i+1)%3;
    memset(glyph, 0, sizeof(glyph));
    if(upperIcon==1)
     memcpy(&glyph[0], &block[0], 3);
    else if(upperIcon==2)
     memcpy(&glyph[0], &apple[0], 3);
    if(lowerIcon==1)
     memcpy(&glyph[4], &block[0], 3);
    else if(lowerIcon==2)
      memcpy(&glyph[4], &apple[0], 3);
    lcd.createChar(i, glyph);
  }
  delay(1); //Wait for the CGRAM to be written
}
void graphic_clear(){
  memset(graphicRam, 0, sizeof(graphicRam));
}
void graphic_add_item(uint8_t x, uint8_t y, enum DisplayItem item) {
  graphicRam[x/(8/2)][y] |= (uint8_t)item*(1<<((x%(8/2))*2));
}
void graphic_flush(){
lcd.clear();
  for(size_t x=0; x<16; x++){
    for(size_t y=0; y<2; y++){
    enum DisplayItem upperItem =
    (DisplayItem)((graphicRam[x/(8/2)][y*2]>>((x%(8/2))*2))&0x3);
    enum DisplayItem lowerItem =
    (DisplayItem)((graphicRam[x/(8/2)][y*2+1]>>((x%(8/2))*2))&0x3);
    if(upperItem>=GRAPHIC_ITEM_NUM)
    upperItem = GRAPHIC_ITEM_B;
    if(lowerItem>=GRAPHIC_ITEM_NUM)
    lowerItem = GRAPHIC_ITEM_B;
    lcd.setCursor(x, y);
    if(upperItem==0 && lowerItem==0)
      lcd.write(' ');
    else
      lcd.write(byte((uint8_t)upperItem*3+(uint8_t)lowerItem-1));
    }
  }
}
void game_new_apple_pos()
{
  bool validApple = true;
  do{
    applePos.x = rand()%GRAPHIC_WIDTH;
    applePos.y = rand()%GRAPHIC_HEIGHT;
    validApple = true;
    for(size_t i=0; i<snakeLength; i++)
    {
      if(applePos.x==snakePosHistory[i].x && applePos.y==snakePosHistory[i].y){
        validApple = false;
      break;
    }
    }
  } while(!validApple);
}
void game_init(){
  srand(micros());
  gameUpdateInterval = 1000;
  gameState = GAME_PLAY;
  snakePosHistory[0].x=3; snakePosHistory[0].y=1;
  snakePosHistory[1].x=2; snakePosHistory[1].y=1;
  snakePosHistory[2].x=1; snakePosHistory[2].y=1;
  snakePosHistory[3].x=0; snakePosHistory[3].y=1;
  snakeLength = 4;
  snakeDirection = SNAKE_RIGHT;
  game_new_apple_pos();
  thisFrameControlUpdated = false;
}

void game_calculate_logic() {
  if(gameState!=GAME_PLAY) //Game over. Don't bother calculating game logic.
    return;
  //Calculate the movement of the tail
  for(size_t i=snakeLength; i>=1; i--){
    snakePosHistory[i] = snakePosHistory[i-1];
  }
  //Calculate the head movement
  snakePosHistory[0]=snakePosHistory[1];
  switch(snakeDirection){
    case SNAKE_LEFT: snakePosHistory[0].x--; break;
    case SNAKE_UP: snakePosHistory[0].y--; break;
    case SNAKE_RIGHT: snakePosHistory[0].x++; break;
    case SNAKE_DOWN: snakePosHistory[0].y++; break;
  }
  //Look for wall collision
  if(snakePosHistory[0].x<0||snakePosHistory[0].x>=GRAPHIC_WIDTH||snakePosHistory[0].y
  <0||snakePosHistory[0].y>=GRAPHIC_HEIGHT){
   gameState = GAME_LOSE;
  return;
  }
  //Look for self collision
  for(size_t i=1; i<snakeLength; i++){
    if(snakePosHistory[0].x==snakePosHistory[i].x &&
      snakePosHistory[0].y==snakePosHistory[i].y){
    gameState = GAME_LOSE;
    return;
  }
  }
  if(snakePosHistory[0].x==applePos.x && snakePosHistory[0].y==applePos.y){
    snakeLength++;
    gameUpdateInterval = gameUpdateInterval*9/10;
    if(snakeLength>=sizeof(snakePosHistory)/sizeof(*snakePosHistory))
     gameState = GAME_WIN;
    else
      game_new_apple_pos();
  }
}
void game_calculate_display() {
  graphic_clear();
  switch(gameState){
    case GAME_LOSE:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" You lose!");
    lcd.setCursor(0, 1);
    lcd.print("Length: ");
    lcd.setCursor(8, 1);
    lcd.print(snakeLength);
    buttonPushed = false; 
    gameState = GAME_PLAY;
    delay(500);
    break;
    case GAME_WIN:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You won. Congratz!");
    lcd.setCursor(0, 1);
    lcd.print("Length: ");
    lcd.setCursor(8, 1);
    lcd.print(snakeLength);
    gameState = GAME_PLAY;
    break;
    case GAME_PLAY:
    for(size_t i=0; i<snakeLength; i++)
    graphic_add_item(snakePosHistory[i].x, snakePosHistory[i].y, GRAPHIC_ITEM_A);
    graphic_add_item(applePos.x, applePos.y, GRAPHIC_ITEM_B);
    graphic_flush();
    break;
    case GAME_MENU:
    buttonPushed = true; 
    break;
  }
}

void initializeGraphics(){
  static byte graphics[] = {
    // Run position 1
    B01100,
    B01100,
    B00000,
    B01110,
    B11100,
    B01100,
    B11010,
    B10011,
    // Run position 2
    B01100,
    B01100,
    B00000,
    B01100,
    B01100,
    B01100,
    B01100,
    B01110,
    // Jump
    B01100,
    B01100,
    B00000,
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    // Jump lower
    B11110,
    B01101,
    B11111,
    B10000,
    B00000,
    B00000,
    B00000,
    B00000,
    // Ground
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    // Ground right
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    B00011,
    // Ground left
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
    B11000,
  };
  int i;
  // Skip using character 0, this allows lcd.print() to be used to
  // quickly draw multiple characters
  for (i = 0; i < 7; ++i) {
   lcd.createChar(i + 1 , &graphics[i * 8]);
  }
  for (i = 0; i < TERRAIN_WIDTH; ++i) {
    terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
    terrainLower[i] = SPRITE_TERRAIN_EMPTY;
  }
}

// Slide the terrain to the left in half-character increments
//
void advanceTerrain(char* terrain, byte newTerrain){
  for (int i = 0; i < TERRAIN_WIDTH; ++i) {
    char current = terrain[i];
    char next = (i == TERRAIN_WIDTH-1) ? newTerrain : terrain[i+1];
    switch (current){
      case SPRITE_TERRAIN_EMPTY:
        terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
        break;
      case SPRITE_TERRAIN_SOLID:
        terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_RIGHT:
        terrain[i] = SPRITE_TERRAIN_SOLID;
        break;
      case SPRITE_TERRAIN_SOLID_LEFT:
        terrain[i] = SPRITE_TERRAIN_EMPTY;
        break;
    }
  }
}

bool drawHero(byte position, char* terrainUpper, char* terrainLower, unsigned int score) {
  bool collide = false;
  char upperSave = terrainUpper[HERO_HORIZONTAL_POSITION];
  char lowerSave = terrainLower[HERO_HORIZONTAL_POSITION];
  byte upper = 0, lower = 0;
  switch (position) {
    case HERO_POSITION_OFF:
      upper = lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_LOWER_1:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN1;
      break;
    case HERO_POSITION_RUN_LOWER_2:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_RUN2;
      break;
    case HERO_POSITION_JUMP_1:
    case HERO_POSITION_JUMP_8:
      upper = SPRITE_TERRAIN_EMPTY;
      lower = SPRITE_JUMP;
      break;
    case HERO_POSITION_JUMP_2:
    case HERO_POSITION_JUMP_7:
      upper = SPRITE_JUMP_UPPER;
      lower = SPRITE_JUMP_LOWER;
      break;
    case HERO_POSITION_JUMP_3:
    case HERO_POSITION_JUMP_4:
    case HERO_POSITION_JUMP_5:
    case HERO_POSITION_JUMP_6:
      upper = SPRITE_JUMP;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_1:
      upper = SPRITE_RUN1;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
    case HERO_POSITION_RUN_UPPER_2:
      upper = SPRITE_RUN2;
      lower = SPRITE_TERRAIN_EMPTY;
      break;
  }
  if (upper != ' ') {
    terrainUpper[HERO_HORIZONTAL_POSITION] = upper;
    collide = (upperSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }
  if (lower != ' ') {
    terrainLower[HERO_HORIZONTAL_POSITION] = lower;
    collide |= (lowerSave == SPRITE_TERRAIN_EMPTY) ? false : true;
  }
  
  byte digits = (score > 9999) ? 5 : (score > 999) ? 4 : (score > 99) ? 3 : (score > 9) ? 2 : 1;
  
  // Draw the scene
  terrainUpper[TERRAIN_WIDTH] = '\0';
  terrainLower[TERRAIN_WIDTH] = '\0';
  char temp = terrainUpper[16-digits];
  terrainUpper[16-digits] = '\0';
  lcd.setCursor(0,0);
  lcd.print(terrainUpper);
  terrainUpper[16-digits] = temp;  
  lcd.setCursor(0,1);
  lcd.print(terrainLower);
  
  lcd.setCursor(16 - digits,0);
  lcd.print(score);

  terrainUpper[HERO_HORIZONTAL_POSITION] = upperSave;
  terrainLower[HERO_HORIZONTAL_POSITION] = lowerSave;
  return collide;
}

// Handle the button push as an interrupt
void buttonPush() {
  buttonPushed = true;
  digitalWrite(4,HIGH);
  delay(250);
  digitalWrite(4,LOW);
}


void setup() {
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_AUTOPLAY, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(PIN_AUTOPLAY, HIGH);
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(GameSwitch, INPUT_PULLUP);
  
  // Digital pin 2 maps to interrupt 0
  attachInterrupt((0), buttonPush, CHANGE);
  initializeGraphics();
  lcd.begin(16,2);
  lcd.print(" Switch GameJoy");
  lcd.setCursor(0, 1);
  graphic_generate_characters();
  gameState = GAME_MENU;
  delay(1000);
  lcd.clear();
}
void loop() {
  static byte heroPos = HERO_POSITION_RUN_LOWER_1;
  static byte newTerrainType = TERRAIN_EMPTY;
  static byte newTerrainDuration = 1;
  static bool playing = false;
  static bool blink = false;
  static unsigned int distance = 0;
  Mode:
  if(mode %2 == 0){
    if (!playing) {
    if (blink) {
      lcd.setCursor(0,0);
      lcd.clear();
      lcd.print("Snake Game");
      lcd.setCursor(0,1);
      lcd.print("Press Start");
      if(digitalRead(GameSwitch) == LOW)
      {
       modeOld = mode++;
        goto Mode;
      }
    }
    delay(50);
    blink = !blink;
    if (buttonPushed) 
    {
      graphic_generate_characters();
      game_init();
      playing = true;
      buttonPushed = false;
    }
    return;
  }
  lcd.setCursor(0, 0);
  if(digitalRead(BUTTON_LEFT)==LOW){
  if(debounce_activate_edge(&debounceCounterButtonLeft)&&!thisFrameControlUpdated){
  switch(gameState){
  case GAME_PLAY:
  switch(snakeDirection){
  case SNAKE_LEFT: snakeDirection=SNAKE_DOWN; break;
  case SNAKE_UP: snakeDirection=SNAKE_LEFT; break;
  case SNAKE_RIGHT: snakeDirection=SNAKE_UP; break;
  case SNAKE_DOWN: snakeDirection=SNAKE_RIGHT; break;
  }
  thisFrameControlUpdated = true;
  break;
  case GAME_MENU:
  break;
  default:
  break;
  }
  }
  }else{
  debounce_deactivate(&debounceCounterButtonLeft);
  }
  lcd.setCursor(8, 0);
  if(digitalRead(BUTTON_RIGHT)==LOW){
  if(debounce_activate_edge(&debounceCounterButtonRight)&&!thisFrameControlUpdated){
  switch(gameState){
  case GAME_PLAY:
  switch(snakeDirection){
  case SNAKE_LEFT: snakeDirection=SNAKE_UP; break;
  case SNAKE_UP: snakeDirection=SNAKE_RIGHT; break;
  case SNAKE_RIGHT: snakeDirection=SNAKE_DOWN; break;
  case SNAKE_DOWN: snakeDirection=SNAKE_LEFT; break;
  }
  thisFrameControlUpdated = true;
  break;
  case GAME_MENU:
  break;
  default:
  break;
  }
  }
  }else{
  debounce_deactivate(&debounceCounterButtonRight);
  }
  if(millis()-lastGameUpdateTick>gameUpdateInterval){
  game_calculate_logic();
  game_calculate_display();
  lastGameUpdateTick = millis();
  thisFrameControlUpdated = false;
  }
    }
  else // Catch Game
  {
    if (!playing) {
    drawHero((blink) ? HERO_POSITION_OFF : heroPos, terrainUpper, terrainLower, distance >> 3);
    if (blink) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Jumping Game");
      lcd.setCursor(0,1);
      lcd.print("Press Start");
      if(digitalRead(GameSwitch) == LOW)
      {
        mode++;
        goto Mode;
      }
    }
    delay(50);
    blink = !blink;
    if (buttonPushed) 
    {
      initializeGraphics();
      heroPos = HERO_POSITION_RUN_LOWER_1;
      playing = true;
      buttonPushed = false;
      distance = 0;
    }
    return;
  }

  // Shift the terrain to the left
  advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
  advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
  
  // Make new terrain to enter on the right
  if (--newTerrainDuration == 0) {
    if (newTerrainType == TERRAIN_EMPTY) {
      newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
      newTerrainDuration = 2 + random(10);
    } else {
      newTerrainType = TERRAIN_EMPTY;
      newTerrainDuration = 10 + random(10);
    }
  }
    
  if (buttonPushed) {
    if (heroPos <= HERO_POSITION_RUN_LOWER_2) heroPos = HERO_POSITION_JUMP_1;
    buttonPushed = false;
  }  

  if (drawHero(heroPos, terrainUpper, terrainLower, distance >> 3)) {
    playing = false; // The hero collided with something. Too bad.
  } else {
    if (heroPos == HERO_POSITION_RUN_LOWER_2 || heroPos == HERO_POSITION_JUMP_8) {
      heroPos = HERO_POSITION_RUN_LOWER_1;
    } else if ((heroPos >= HERO_POSITION_JUMP_3 && heroPos <= HERO_POSITION_JUMP_5) && terrainLower[HERO_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_RUN_UPPER_1;
    } else if (heroPos >= HERO_POSITION_RUN_UPPER_1 && terrainLower[HERO_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY) {
      heroPos = HERO_POSITION_JUMP_5;
    } else if (heroPos == HERO_POSITION_RUN_UPPER_2) {
      heroPos = HERO_POSITION_RUN_UPPER_1;
    } else {
      ++heroPos;
    }
    ++distance;
    
    digitalWrite(PIN_AUTOPLAY, terrainLower[HERO_HORIZONTAL_POSITION + 2] == SPRITE_TERRAIN_EMPTY ? HIGH : LOW);
  }
  delay(100);
  }
}


 
