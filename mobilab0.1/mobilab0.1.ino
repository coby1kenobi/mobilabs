//Using libraries from David Prentice
//Special thanks to him
#include <EEPROM.h>
int fileNum; 
String fileString = "_";
String toFileName; 

// *** sensor libraries and variables:
//HC-SR04 sensor:
#include <Wire.h>
#include <NewPing.h>
int trig = 33; //dpin __
int echo = 34; //dpin __
int dist_5v = 32; //don't forget to pinmode, digital write high then low
//grnd 
NewPing sonar(trig, echo, 350); //350cm range but can go up to 400. 

// --- display libraries:
//Using libraries from David Prentice
//Special thanks to him
#include <Adafruit_GFX.h>
#include <OPENSMART_kbv.h>
OPENSMART_kbv tft;
#include <TouchScreen.h>     

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <FreeDefaultFonts.h>

const int XP=6,XM=A2,YP=A1,YM=7; //240x400 ID=0x9327
const int TS_LEFT=923,TS_RT=144,TS_TOP=964,TS_BOT=150;

int page; //page number || 0 for home, 

TouchScreen ts(XP, YP, XM, YM, 300);   //re-initialised after diagnose
TSPoint tp;              

#define TFT_BEGIN()  tft.begin(ID)

#define WHITE 0xFFFF
#define RED   0xF800
#define BLUE  0x001F
#define GREEN 0x07E0
#define BLACK 0x0000
#define MBYELLOW tft.color565(240, 221, 48)
#define MBBLUE tft.color565(43, 137, 240)
#define MBGREEN tft.color565(4, 153, 56)
#define MBRED tft.color565(153, 5, 0)

//#define GRAY  0x2408        //un-highlighted cross-hair
#define GRAY      BLUE     //idle cross-hair colour
#define GRAY_DONE RED      //finished cross-hair

#define MINPRESSURE 200
#define MAXPRESSURE 1000

int dist_pressed = 0; 
int temp_pressed = 0;
int ph_pressed = 0;
int sd_pressed = 0;

bool dist_on;
float dist;

bool temp_on;
float temp;

bool ph_on;
float ph;

bool sd_on;

Adafruit_GFX_Button dist_btn_on, dist_btn_off, temp_btn_on, temp_btn_off, ph_btn_on, ph_btn_off, sd_btn_on, sd_btn_off;

int pixel_x, pixel_y;     //Touch_getXY() updates global vars
int count = 0;


bool readTouch(void) {
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        pixel_x = map(p.x, TS_LEFT, TS_RT, 0, tft.width()); //.kbv makes sense to me
        pixel_y = map(p.y, TS_TOP, TS_BOT, 0, tft.height());
        //count++;
        //delay(5);
        Serial.print(count); Serial.print(" || ");
    }
    //else count = 0;
    //if (count == 1) return pressed;
    //else return false; 
    return pressed;
}




uint16_t readID(void) {
    uint16_t ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9486;
    return ID;
}


uint32_t cx, cy, cz;
uint32_t rx[8], ry[8];
int32_t clx, crx, cty, cby;
float px, py;
int dispx, dispy, text_y_center, swapxy;
uint32_t calx, caly, cals;

void titletext(int x, int y, int sz, const GFXfont *f, const char *msg)
{
    int16_t x1, y1;
    uint16_t wid, ht;
    tft.drawFastHLine(0, y+5, tft.width(), MBYELLOW);
    tft.setFont(f);
    tft.setCursor(x, y);
    tft.setTextColor(WHITE);
    tft.setTextSize(sz);
    tft.print(msg);
}

void text(int x, int y, int sz, const GFXfont *f, const char *msg, const char clr) {
  int16_t x1, y1;
  uint16_t wid, ht;
  tft.setFont(f);
  tft.setCursor(x, y);
  tft.setTextColor(clr);
  tft.setTextSize(sz);
  tft.print(msg);
}

void printVal(int x, int y, int sz, const GFXfont *f, float val, const char clr) {
  int16_t x1, y1;
  uint16_t wid, ht;
  tft.setFont(f);
  tft.setCursor(x, y);
  tft.setTextColor(clr);
  tft.setTextSize(sz);
  tft.print(val);
}


//****** SETUP HERE
void setup() {
  uint16_t ID = readID();
  Serial.begin(9600);

  if (EEPROM.read(0) == 255) {
    fileNum = 0; 
  } else {
    fileNum = EEPROM.read(0);
  }
  
  tft.begin(ID);
  tft.fillScreen(BLACK);
  tft.setRotation(0); //Portrait
  dispx = tft.width();
  dispy = tft.height();
  titletext(5, 200, 1, &FreeSans12pt7b, "mobilab v1");
  text_y_center = (dispy / 2) - 6;

  pinMode(dist_5v, OUTPUT);
  delay(750);

  home();
  page = 0;
  Serial.print("EEPROM:  ");
  Serial.println(fileNum);


}

void home() {
  tft.fillScreen(BLACK);

  //header
  tft.drawFastHLine(0, 20, tft.width(), MBYELLOW);
  tft.fillRect(0, 0, tft.width(), 20, MBBLUE);
  text(10, 15, 1, &FreeSans9pt7b, "mobilab v1", WHITE);

  // --- DISTANCE
  tft.drawFastHLine(0, 134, tft.width(), MBYELLOW);
  text(10, 40, 1, &FreeSans9pt7b, "distance (cm)", WHITE);
  
  // --- /DISTANCE

  // --- TEMPERATURE
  tft.drawFastHLine(0, 248, tft.width(), MBYELLOW);
  // --- /TEMPERATURE
  text(10, 154, 1, &FreeSans9pt7b, "temperature (*C)", WHITE);

  // --- PH
  text(10, 268, 1, &FreeSans9pt7b, "pH", WHITE);

  // --- /PH
  
  //RECORD
  tft.drawFastHLine(0, 362, tft.width(), MBYELLOW);
  text(10, 395, 1, &FreeSans12pt7b, "record", MBYELLOW);
}



void loop() {
  bool pressed = readTouch();
  
  if (page == 0) {

  // --- text
  
  // --- /text

  
    // --- button for dist
    dist_btn_on.initButton(&tft, 210, 77, 45, 45, BLACK, MBRED, WHITE, "", 1);
    dist_btn_off.initButton(&tft, 210, 77, 45, 45, BLACK, MBGREEN, WHITE, "", 1);
    dist_btn_on.press(pressed && dist_btn_on.contains(pixel_x, pixel_y));
    dist_btn_off.press(pressed && dist_btn_off.contains(pixel_x, pixel_y));
    
    if (dist_btn_on.justPressed()) {
      dist_pressed++; //if int is odd => sensor was turned on. if odd, sensor is off
      //Serial.println("dist_btn was pressed");
    } 
    
    if ((dist_pressed % 2) == 0) {
      dist_btn_on.drawButton();
      dist_on = false;
      text(20, 80, 1, &FreeSans12pt7b, "off", WHITE);
      delay(100);
      Serial.println("dist: off");
    } else {
      dist_btn_off.drawButton();
      dist_on = true;
      get_dist();
      printVal(20, 70, 1, &FreeSans12pt7b, dist, WHITE);
      delay(100);
      Serial.println("dist: on");
    }
    //Serial.println(dist_pressed);
    // --- /button for dist


    // --- button for temp
    temp_btn_on.initButton(&tft, 210, 191, 45, 45, BLACK, MBRED, WHITE, "", 1);
    temp_btn_off.initButton(&tft, 210, 191, 45, 45, BLACK, MBGREEN, WHITE, "", 1);
    temp_btn_on.press(pressed && temp_btn_on.contains(pixel_x, pixel_y));
    temp_btn_off.press(pressed && temp_btn_off.contains(pixel_x, pixel_y));

    if (temp_btn_on.justPressed()) {
      temp_pressed++; //if int is odd => sensor was turned on. if odd, sensor is off
      Serial.println("temp_btn was pressed");
    } 
    
    if ((temp_pressed % 2) == 0) {
      temp_btn_on.drawButton();
      temp_on = false;
      //Serial.println("dist: off");
    } else {
      temp_btn_off.drawButton(); //button to turn off. 
      temp_on = true;
      //Serial.println("dist: on");
    }
    //Serial.println(dist_pressed);
    // --- /button for temp


    // --- button for pH
    ph_btn_on.initButton(&tft, 210, 305, 45, 45, BLACK, MBRED, WHITE, "", 1); //red to signal off
    ph_btn_off.initButton(&tft, 210, 305, 45, 45, BLACK, MBGREEN, WHITE, "", 1); //green to signal on
    ph_btn_on.press(pressed && ph_btn_on.contains(pixel_x, pixel_y));
    ph_btn_off.press(pressed && ph_btn_off.contains(pixel_x, pixel_y));

    if (ph_btn_on.isPressed()) {
      ph_pressed++; //if int is odd => sensor was turned on. if odd, sensor is off
      //Serial.println("ph_btn was pressed");
    } 
    
    if ((ph_pressed % 2) == 0) {
      ph_btn_on.drawButton(); //try adding false as a parameter
      ph_on = false;
    } else {
      ph_btn_off.drawButton();
      ph_on = true;
    }
    // --- /button for pH

    // --- button for SD
    sd_btn_on.initButton(&tft, 120, 381, tft.width() + 20, 38, BLACK, MBBLUE, WHITE, "", 1); //BLUE to signal off
    sd_btn_off.initButton(&tft, 120, 381, tft.width() + 20, 38, BLACK, MBYELLOW, WHITE, "", 1); //YELLOW to signal on
    sd_btn_on.press(pressed && sd_btn_on.contains(pixel_x, pixel_y));
    sd_btn_off.press(pressed && sd_btn_off.contains(pixel_x, pixel_y));

    if (sd_btn_on.isPressed()) {
      sd_pressed++; //if int is odd => sensor was turned on. if odd, sensor is off
      if (sd_pressed % 2 != 0) {
        fileNum++;
        EEPROM.write(0, fileNum);
        Serial.println(fileNum);
      }
      Serial.println("sd_btn was pressed");
    } 
    
    if ((sd_pressed % 2) == 0) {
      sd_btn_on.drawButton(); //try adding false as a parameter
      text(45, 381, 1, &FreeSans9pt7b, "start recording", WHITE);
      sd_on = false;
    } else {
      sd_btn_off.drawButton();
      text(40, 381, 1, &FreeSans9pt7b, "stop recording", WHITE);
      sd_on = true;
    }
    // --- /button for SD

    //reseting values:
    tft.fillRect(10, 45, 60, 40, BLACK);


  }
}

void get_dist() {
  delay(100);
  digitalWrite(dist_5v, HIGH);
  dist = sonar.ping_cm();
  delay(100); //just for safety but please message or email me if this can be deleted or should be increased.
  digitalWrite(dist_5v, LOW);
}
