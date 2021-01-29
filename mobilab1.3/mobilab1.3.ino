//Using libraries by David Prentice
//Special thanks to him

// *** sensor libraries and variables:
//SD
#include <EEPROM.h> 
#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>                           
SoftwareSerial myserial(0, 1);               

int fileNum; 
String fileString = "_";
String fileName = "data"; 

int SD_CS = 39; //SD_CS pin
int SD_5v = 38; //SD 5v

//temperature probe
#include <OneWire.h>
#include <DallasTemperature.h>

int temp_5v = 44;
int temp_in = 45;

OneWire oneWire(temp_in);
DallasTemperature sensors(&oneWire);

//pH Meter
#include "DFRobot_PH.h"

#define ph_in A8
int ph_5v = 42;
float ph_v,phValue,ph_temp = 25;
DFRobot_PH DFRph;


//HC-SR04 sensor:
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
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#include <TouchScreen.h>   
#include <Wire.h>  

#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <FreeDefaultFonts.h>

//#define USE_MEGA_8BIT_PROTOSHIELD
//#define __AVR_ATmega2560__
const int XP=28,XM=A2,YP=A1,YM=29; //240x400 ID=0x9327
const int TS_LEFT=920,TS_RT=178,TS_TOP=958,TS_BOT=182;


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

Adafruit_GFX_Button dist_btn_on, dist_btn_off, temp_btn_on, temp_btn_off, ph_btn_on, ph_btn_off, sd_btn_on, sd_btn_off, ph_cal;

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

void titletext(int x, int y, int sz, const GFXfont *f, const char *msg) {
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
  myserial.begin(9600);
  Serial.println(ID);
  sensors.begin();
  tft.begin(ID);
  tft.fillScreen(BLACK);
  tft.setRotation(0); //Portrait
  dispx = tft.width();
  dispy = tft.height();
  titletext(5, 200, 1, &FreeSans12pt7b, "mobilab v1");
  text_y_center = (dispy / 2) - 6;

  pinMode(dist_5v, OUTPUT);
  pinMode(temp_5v, OUTPUT);
  pinMode(ph_5v, OUTPUT);
  pinMode(SD_5v, OUTPUT);
  digitalWrite(SD_5v, HIGH);
  DFRph.begin();
  delay(750);
  if (EEPROM.read(4) == 255 && EEPROM.read(5) == 255) {
    EEPROM.write(4, 0);
    EEPROM.write(5, 0);
  }

  home();
  page = 0;


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
  text(10, 154, 1, &FreeSans9pt7b, "temperature (*C)", WHITE);

  // --- PH
  text(10, 268, 1, &FreeSans9pt7b, "pH", WHITE);

  // --- /PH
  
  
}


void loop() {
  bool pressed = readTouch();
  
  if (page == 0) {

  // --- text
  String dataString = ""; //String to be written to the SD card. 
  int startTime;
  // --- /text

    float batt = analogRead(A9);
    //Serial.println(batt);
    // --- button for dist
    dist_btn_on.initButton(&tft, 210, 77, 45, 45, BLACK, MBRED, WHITE, "", 1);
    dist_btn_off.initButton(&tft, 210, 77, 45, 45, BLACK, MBGREEN, WHITE, "", 1);
    dist_btn_on.press(pressed && dist_btn_on.contains(pixel_x, pixel_y));
    dist_btn_off.press(pressed && dist_btn_off.contains(pixel_x, pixel_y));
    
    if (dist_btn_on.justPressed()) {
      dist_pressed++; //if int is odd => sensor was turned on. if odd, sensor is off
      //Serial.println("dist_btn was pressed");
      if (dist_pressed % 2 != 0) {
        digitalWrite(dist_5v, HIGH);
      } else {
          digitalWrite(dist_5v, LOW);
      }
    } 
    
    if ((dist_pressed % 2) == 0) {
      dist_btn_on.drawButton();
      dist_on = false;
      //text(20, 80, 1, &FreeSans12pt7b, "off", WHITE);
      //Serial.println("dist: off");
    } else {
      dist_btn_off.drawButton();
      dist_on = true;
      get_dist();
      //printVal(20, 80, 1, &FreeSans12pt7b, dist, WHITE);
      //delay here maybe?
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
      if (temp_pressed % 2 != 0) {
        digitalWrite(temp_5v, HIGH);
      } else {
          digitalWrite(temp_5v, LOW);
      }
    } 
    
    if ((temp_pressed % 2) == 0) {
      temp_btn_on.drawButton();
      temp_on = false;
      //text(20, 194, 1, &FreeSans12pt7b, "off", WHITE);
    } else {
      temp_btn_off.drawButton(); //button to turn off. 
      temp_on = true;
      get_temp();
     // printVal(20, 194, 1, &FreeSans12pt7b, temp, WHITE);
    }
    //Serial.println(dist_pressed);
    // --- /button for temp


    // --- button for pH
    ph_btn_on.initButton(&tft, 210, 300, 45, 45, BLACK, MBRED, WHITE, "", 1); //red to signal off
    ph_btn_off.initButton(&tft, 210, 300, 45, 45, BLACK, MBGREEN, WHITE, "", 1); //green to signal on
    ph_btn_on.press(pressed && ph_btn_on.contains(pixel_x, pixel_y));
    ph_btn_off.press(pressed && ph_btn_off.contains(pixel_x, pixel_y));
    // --- button for pH callibration
    ph_cal.initButton(&tft, 33, 311, 50, 40, MBBLUE, BLACK, WHITE, "", 1);
    ph_cal.press(pressed && ph_cal.contains(pixel_x, pixel_y));

    if (ph_btn_on.isPressed()) {
      ph_pressed++; //if int is odd => sensor was turned on. if odd, sensor is off
      //Serial.println("ph_btn was pressed");
      if (ph_pressed % 2 != 0) {
        digitalWrite(ph_5v, HIGH);
      } else {
          digitalWrite(ph_5v, LOW);
      }
    } 
    
    if ((ph_pressed % 2) == 0) {
      ph_btn_on.drawButton(); //try adding false as a parameter
      ph_on = false;
      //text(20, 308, 1, &FreeSans12pt7b, "off", WHITE);
      //go to pH callibration
      ph_cal.drawButton();
      if (ph_cal.isPressed()) {
        digitalWrite(ph_5v, HIGH);
        calph(); 
      }

    } else {
      ph_btn_off.drawButton();
      ph_on = true;
      get_ph();
      //printVal(20, 308, 1, &FreeSans12pt7b, ph, WHITE);
    }

    // --- /button for pH

   
    // --- button for SD
    sd_btn_on.initButton(&tft, 120, 381, tft.width() + 20, 38, BLACK, MBBLUE, WHITE, "", 1); //BLUE to signal off
    sd_btn_off.initButton(&tft, 120, 381, tft.width() + 20, 38, BLACK, MBYELLOW, WHITE, "", 1); //YELLOW to signal on
    sd_btn_on.press(pressed && sd_btn_on.contains(pixel_x, pixel_y));
    sd_btn_off.press(pressed && sd_btn_off.contains(pixel_x, pixel_y));

    if (sd_btn_on.isPressed()) {
      //get num files here
      sd_pressed++; //if int is odd => button was turned on. if odd, button is off
      Serial.println("sd_btn was pressed");
      if (sd_pressed % 2 != 0) {
        fileName = "data";
        fileNum = EEPROM.read(4);
        fileNum++;
        fileName += fileNum;
        fileName += ".txt";
        // FILE NAME CODE HERE *************************************************** DONT FORGET TO ADD ".txt"
        EEPROM.write(4, fileNum);
        startTime = millis();
        dataString = "time(ms)";
        if (dist_on) dataString += ",distance(cm)";
        if (temp_on) dataString += ",temperature(*C)";
        if (ph_on) dataString += ",pH";
        tft.fillRect(0, 362, tft.width() + 20, 38, MBBLUE);
        text(45, 381, 1, &FreeSans9pt7b, "Initializing SD Card", WHITE);
        if (!SD.begin(SD_CS)) {
           sd_pressed--;
           Serial.println("Card failed, or not present");
           tft.fillRect(0, 362, tft.width() + 20, 38, MBRED);
           text(45, 381, 1, &FreeSans9pt7b, "No SD Card", WHITE);
           delay(1000);
        } //just writing headers
        File dataFile = SD.open(fileName, FILE_WRITE);
        if (dataFile) {
          dataFile.println(dataString);
          dataFile.close();
          // print to the serial port too:
          Serial.println(dataString);
        }
         
      }
     }
    
    if ((sd_pressed % 2) == 0) {
      sd_btn_on.drawButton(); //try adding false as a parameter
      text(45, 381, 1, &FreeSans9pt7b, "start recording", WHITE);
      sd_on = false;
    } else {
      sd_btn_off.drawButton();
      text(40, 381, 1, &FreeSans9pt7b, "stop recording", WHITE);
      sd_on = true;
      File dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        int runtime = millis() - startTime;
        dataString += runtime; 
        dataString += ",";
        if (dist_on) {
          dataString += dist; 
          dataString += ",";
        }
        if (temp_on) {
          dataString += temp;
          dataString += ",";
        }
        if (ph_on) {
          dataString += ph;
          dataString += ",";
        }
        dataString.remove(dataString.length() - 1, 1);
        dataFile.println(dataString);
        dataFile.close();
        // print to the serial port too:
        Serial.println(dataString);
      }
      
    }
    // --- /button for SD
    //delay(250);
    //reseting values, basically just erasing
    tft.fillRect(10, 45, 110, 60, BLACK);
    tft.fillRect(10, 159, 110, 60, BLACK);
    tft.fillRect(3, 273, 110, 60, BLACK);
    //printing values
    if (dist_on) {
      printVal(20, 80, 1, &FreeSans12pt7b, dist, WHITE);
    } else {
       text(20, 80, 1, &FreeSans12pt7b, "off", WHITE);
    }
    if (temp_on) {
      printVal(20, 194, 1, &FreeSans12pt7b, temp, WHITE);
    } else {
      text(20, 194, 1, &FreeSans12pt7b, "off", WHITE);
    }
    if (ph_on) {
      printVal(20, 308, 1, &FreeSans12pt7b, ph, WHITE);
    } else {
      text(20, 308, 1, &FreeSans9pt7b, "cal", WHITE);
    }
    
  }

  //button will serialprint "CALPH" then serialread the string. then go to the ph callibration page. 

  //pH CALIBRATION PAGE
  //if (page == 1) {
    //two buttons to calibrate with pH 4, 7. 
    //also turn on temp
    //use print then read
    
  //}
  
}
void get_dist() {
  //should I place a delay here?
  dist = sonar.ping_cm();
}

void get_temp() {
  sensors.requestTemperatures();
  temp = sensors.getTempCByIndex(0);
}

void get_ph() {
  ph_v = analogRead(ph_in)/1024.0*5000; //voltage reading of sensor
  if (temp_on) {
      ph_temp = temp;
    } else {
      ph_temp = 25.0;
    }
  ph = DFRph.readPH(ph_v, ph_temp);
  //DFRph.calibration(ph_v,ph_temp);  // calibration process by Serail CMD

  
}

void calph() {
  tft.fillRect(5, 273, 180, 80, BLACK);
  tft.drawRect(20, 280, 160, 30, WHITE);
  static unsigned long cal_start = millis();
  //DFRph.calibration(ph_v,ph_temp);  // calibration process by Serail CMD 
  //myserial.print("ENTERPH");
  //myserial.print('\r');
  DFRph.calibration(ph_v, ph_temp, "ENTERPH");
  delay(500);
 
  while (5000 > (millis()-cal_start) && (ph != 7.00 || ph != 4.00)) {
    float ph_prog = (millis()-cal_start)/5000;
    if (temp_on) {
      ph_temp = temp;
    } else {
      ph_temp = 25.0;
    }
    get_ph();
    tft.fillRect(21, 281, ph_prog * 158, 28, MBBLUE);
  }

  DFRph.calibration(ph_v, ph_temp, "CALPH");
  //myserial.print("CALPH");
  //myserial.print('\r');
  delay(500);
  DFRph.calibration(ph_v, ph_temp, "EXITPH");
  //myserial.print("EXITPH");
  //myserial.print('\r');

  digitalWrite(ph_5v, LOW);
  tft.fillRect(5, 273, 180, 80, BLACK);

  
}
