#include <RDA5807FP.h>
#include <RDA5807M.h>
#include <RDSParser.h>
#include <SI4703.h>
#include <SI4705.h>
#include <SI47xx.h>
#include <TEA5767.h>
#include <radio.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Adafruit_ILI9341.h>
#include "TouchScreen.h"
#include "SparkFunSi4703.h"
#include <Wire.h>
#include <IRremote.h>
#include "Rtc_Pcf8563.h"

#define IR_RECEIVE_PIN 7 // Signal Pin of IR receiver
IRrecv receiver(IR_RECEIVE_PIN);


Rtc_Pcf8563 rtc;
bool rtcFound = false;
bool infrequency = false;
bool invol = false;

// wiring setup
int resetPin = 2;
int SDIO = A4;
int SCLK = A5;
#define YP A1  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM 8   // can be any digital pin
#define XP 9   // can be any digital pin
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940
#define MINPRESSURE 10
#define MAXPRESSURE 1000
#define TFT_CS 10
#define TFT_DC 9
#define BLACK   0x0000
#define RED     0xF800
#define BLUE    0x001F
#define CYAN    0x07FF
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define MAGENTA 0xF81F
#define WHITE   0xFFFF
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 330);

Si4703_Breakout radio(resetPin, SDIO, SCLK);
int channel;
int volume;
char rdsBuffer[10];

void setup()
{
  Serial.begin(9600);
  receiver.enableIRIn(); // Start the receiver
  IrReceiver.begin(IR_RECEIVE_PIN);
  Wire.begin();
  Wire.beginTransmission(RTCC_R>>1);
  if (Wire.endTransmission()==0)
    {
    rtcFound = true;
    Serial.println("RTC module found");
    rtc.setDate(02, 2, 5, 0, 23); // Friday 31/12/2021
    rtc.setTime(15, 40, 10); // 8:30pm
  }
  else
  {
    Serial.println("RTC module not found");
  }

  Serial.println("Start RTC testing");
  Serial.println(F("FM Radio"));
  Serial.println("\n\nSi4703_Breakout Test Sketch");
  Serial.println("===========================");  
  Serial.println("a b     Favourite stations");
  Serial.println("+ -     Volume (max 15)");
  Serial.println("u d     Seek up / down");
  Serial.println("r       Listen for RDS Data (15 sec timeout)");
  Serial.println("Send me a command letter.");
  tft.begin();
  MainP();
  radio.powerOn();
  radio.setVolume(0);
  
}

void loop()
{ 
  if (receiver.decode()) {
    translateIR();
    receiver.resume();
  }
  if (rtcFound)
  {
    tft.setCursor(3,220);
    tft.print("");
    tft.print(rtc.formatTime(RTCC_TIME_HMS));
  
    tft.print("        ");
    tft.println(rtc.formatDate(RTCC_DATE_WORLD));
    tft.fillRect(0,192,320,48,WHITE);
    delay(1);

    tft.setCursor(3,220);
  }

  if (Serial.available())
  {
    char ch = Serial.read();
    if (ch == 'u') 
    {
      channel = radio.seekUp();
      displayInfo();
    } 
    else if (ch == 'd') 
    {
      channel = radio.seekDown();
      displayInfo();
    } 
    else if (ch == '+') 
    {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == '-') 
    {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == 'a')
    {
      channel = 930; // Rock FM
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'b')
    {
      channel = 974; // BBC R4
      radio.setChannel(channel);
      
      displayInfo();
    }
    else if (ch == 'r')
    {
      Serial.println("RDS listening");
      radio.readRDS(rdsBuffer, 15000);
      Serial.print("RDS heard:");
      Serial.println(rdsBuffer);      
    }
  }
    TSPoint p = ts.getPoint();
    
  Serial.print("X = "); Serial.print(p.x);
  Serial.print("\tY = "); Serial.print(p.y);
  Serial.print("\tPressure = "); Serial.println(p.z); 
  if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
     return;
  }
  
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());
  Serial.print("("); Serial.print(p.x);
  Serial.print(", "); Serial.print(p.y);
  Serial.println(")");

  if (p.z > 10)
  {
    Serial.println(p.x);
    Serial.println(p.y);
    //VOLUME
    if ( (p.x > 96 ) && ( p.x <= 144 ))
    {
    tft.setCursor(115,120);
    tft.setTextColor(RED);
    tft.print("Volume =   ");
     tft.setCursor(100,72);
    tft.setTextColor(BLACK);
    tft.print("Frequency =   ");
    invol = true;
    infrequency = false;
    }
    if ((p.x > 240) && (p.y < 48 ) && (invol == true) ) 
      {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
      }
      if ( (p.x > 240 ) && (p.y < 280 ) && (p.y > 180) && (invol == true) ) 
      {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
      }
     if ( (p.x > 144 ) && ( p.x <= 192 ))  
    {
    tft.setCursor(100,72);
    tft.setTextColor(BLUE);
    tft.print("Frequency =   ");
    tft.setCursor(115,120);
    tft.setTextColor(BLACK);
    tft.print("Volume =   "); //FREQUENCY TOUCH
    infrequency= true; 
    invol = false;
    }
      if ((p.x > 240) && (p.y < 48 ) && (infrequency == true) ) 
      {
      channel = radio.seekUp();
      displayInfo();
      } 
      if ( (p.x > 240 ) && (p.y < 280 ) && (p.y > 180) && (infrequency == true) ) 
      {
      channel = radio.seekDown();
      displayInfo();
      } 
   
  }



   
}

void translateIR() //IR CONTROLS
{
  // Takes command based on IR code received
  switch (receiver.decodedIRData.command) {
  case 69:
  Serial.println(receiver.decodedIRData.command); 
  break;
  case 70:
  Serial.println("Num 2");
  break;
  case 71:
   Serial.println("3");
  break;
  case 68:
  Serial.println("4");
  break;
  case 64:
  Serial.println("5");
  break;
  case 67:
  Serial.println("6");
  break;
  case 7:
  Serial.println("7");
  break;
  case 21:
  Serial.println("8");
  break;
  case 9:
  Serial.println("9");
  break;
  case 22:
  Serial.println("*");
  break;
  case 25:
  Serial.println("0");
  break;
  case 13:
  Serial.println("#");
  break;
  case 24:
  Serial.println("^");
       volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
  break;
  case 8:
  Serial.println("<--");
      channel = radio.seekDown();
      displayInfo();
  break;
  case 90:
  Serial.println("-->");
   channel = radio.seekUp();
    displayInfo();
  break;
  case 82:
  Serial.println("v");
        volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
  break;
  case 28:
  Serial.println("ok");
  break;
  default:
  ;}
}
void displayInfo()
{
   Serial.print("Channel:"); Serial.print(channel);
   tft.fillRect(240,48,240,96,WHITE);
   tft.setCursor(250,72);
   tft.print(channel);
   tft.setCursor(250,120);
   tft.print(volume);
   
   Serial.print(" Volume:"); Serial.println(volume); 
}

int MainP()
{
  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.fillScreen(ILI9341_WHITE);
  tft.setRotation(1);
  tft.fillRect(0,0,80,48,GREEN);
  tft.fillRect(240,0,240,48,RED);
  tft.setCursor(3,220);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(25,14);
  tft.print("Up");
  tft.setCursor(255,14);
  tft.print("Down");
  tft.setCursor(110,14);
  tft.print("FM RADIO");
  tft.setCursor(100,72);
  tft.print("Frequency =   ");
  tft.setCursor(115,120);
  tft.print("Volume = ");
  tft.setCursor(70,168); 
  tft.print("Created by group 6");





}

void Info()
{

  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.fillScreen(ILI9341_WHITE);
  tft.setRotation(1);
  tft.fillRect(0,0,80,48,GREEN);
  tft.fillRect(240,0,240,48,RED);
  tft.setCursor(3,220);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(25,14);
  tft.print("Up");
  tft.setCursor(255,14);
  tft.print("Down");
  tft.setCursor(110,14);
  tft.print("BATTERY");
  tft.setCursor(130,72);
  tft.print("FREQ");
  tft.setCursor(115,120);
  tft.print("VOLUME");

}

void Display()
{

  tft.setTextColor(BLACK);
  tft.setTextSize(3);
  tft.fillScreen(ILI9341_WHITE);
  tft.setRotation(1);
  tft.fillRect(0,0,80,48,GREEN);
  tft.fillRect(240,0,240,48,RED);
  tft.setCursor(3,220);
  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(25,14);
  tft.print("Up");
  tft.setCursor(255,14);
  tft.print("Down");
  tft.setCursor(110,14);
  tft.print("Radio");
  tft.setCursor(130,72);
  tft.print("FM RADIO");
  tft.setCursor(115,120);
  tft.print("FREQUENCY");
  tft.setCursor(100,168); 
  tft.print("VOLUME");


}
