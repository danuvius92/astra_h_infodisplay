#include <SPI.h>
#include <TFT_eSPI.h>
#include "ELMduino.h"
#include "BluetoothSerial.h"
#include "opel.h"
#include "battery.h"
#include <LittleFS.h>

//////////////////////////////////////////////////////////////////////////
// TFT_eSPI
//////////////////////////////////////////////////////////////////////////

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite boosttxt = TFT_eSprite(&tft);
TFT_eSprite coolanttxt = TFT_eSprite(&tft);
TFT_eSprite batteryicon = TFT_eSprite(&tft);
TFT_eSprite volt = TFT_eSprite(&tft);
TFT_eSprite buttonspray = TFT_eSprite(&tft);
TFT_eSprite opellogo = TFT_eSprite(&tft);
TFT_eSprite ui = TFT_eSprite(&tft);
TFT_eSprite errorspray = TFT_eSprite(&tft);

//////////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////////

#define DARKER_GREY 0x18E3
#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_MEDIUM "Latin-Hiragana-24"
#define AA_FONT_LARGE "NotoSansBold36"
#define FlashFS LittleFS

BluetoothSerial SerialBT;
#define ELM_PORT   SerialBT
#define DEBUG_PORT Serial

//String MACadd = "7A:B4:84:AE:81:8D";                         //enter the ELM327 MAC address
uint8_t address[6]  = {0x7A, 0xB4, 0x84, 0xAE, 0x81, 0x8D};  //enter the ELM327 MAC address after the 0x

//////////////////////////////////////////////////////////////////////////
// Variables
//////////////////////////////////////////////////////////////////////////

int reading = 0; // Value to be displayed

static uint16_t last_angle_boost = 30;
static uint16_t last_angle_coolant = 30;

typedef enum
{
  COOLANT,
  LOAD,
  VOLTAGE
} obd_pid_states;
obd_pid_states obd_state = VOLTAGE;

float coolant = 0;
float voltage = 0;
float load = 0;

String message="";

//////////////////////////////////////////////////////////////////////////
// Functions
//////////////////////////////////////////////////////////////////////////

void(* resetFunc) (void) = 0;

void drawVolt(float reading)
{

  volt.fillSprite(TFT_BLACK);
  volt.loadFont(AA_FONT_LARGE, LittleFS);
  volt.drawFloat(reading, 1, 0, 0);
  volt.pushSprite(130, 190);
  volt.unloadFont();
}

void ringMeterBoost(int x, int y, int r, float val)
{

  r -= 3;

  // Range here is 0-100 so value is scaled to an angle 30-330
  float val_angle = map(val, 0, 100, 30, 330);

  if (last_angle_boost != val_angle)
  {

    boosttxt.fillSprite(DARKER_GREY);
    boosttxt.loadFont(AA_FONT_LARGE, LittleFS);
    boosttxt.setTextDatum(CC_DATUM);
    boosttxt.drawFloat(val, 0, 35, 20);
    boosttxt.pushSprite(35, 55);
    boosttxt.unloadFont();

    // Allocate a value to the arc thickness dependant of radius
    uint8_t thickness = r / 5;
    if (r < 25)
      thickness = r / 3;

    // Update the arc, only the zone between last_angle and new val_angle is updated
    if (val_angle > last_angle_boost)
    {
      tft.drawArc(x, y, r, r - thickness, last_angle_boost, val_angle, TFT_GOLD, TFT_BLACK); // TFT_SKYBLUE random(0x10000)
    }
    else
    {
      tft.drawArc(x, y, r, r - thickness, val_angle, last_angle_boost, TFT_BLACK, DARKER_GREY);
    }
    last_angle_boost = val_angle; // Store meter arc position for next redraw
  }
}

void ringMeterCoolant(int x, int y, int r, int val)
{

  r -= 3;

  // Range here is 0-100 so value is scaled to an angle 30-330
  int val_angle = map(val, 0, 125, 30, 330);

  if (last_angle_coolant != val_angle)
  {

    coolanttxt.fillSprite(DARKER_GREY);
    coolanttxt.loadFont(AA_FONT_LARGE, LittleFS);
    coolanttxt.setTextDatum(CC_DATUM);
    coolanttxt.drawFloat(val, 0, 35, 20);
    coolanttxt.pushSprite(175, 55);
    coolanttxt.unloadFont();

    // Allocate a value to the arc thickness dependant of radius
    uint8_t thickness = r / 5;
    if (r < 25)
      thickness = r / 3;

    // Update the arc, only the zone between last_angle and new val_angle is updated
    if (val_angle > last_angle_coolant)
    {

      tft.drawArc(x, y, r, r - thickness, last_angle_coolant, val_angle, TFT_SKYBLUE, TFT_BLACK, true); // TFT_SKYBLUE random(0x10000)
    }
    else
    {
      tft.drawArc(x, y, r, r - thickness, val_angle, last_angle_coolant, TFT_BLACK, DARKER_GREY, true);
    }
    last_angle_coolant = val_angle; // Store meter arc position for next redraw
  }
}

void drawBoost(float load)
{
  static uint8_t radius = 60;

  ringMeterBoost(70, 70, radius, load); // Draw analogue meter
}

void drawCoolant(float reading)
{
  static uint8_t radius = 60;

  ringMeterCoolant(210, 70, radius, reading); // Draw analogue meter
}

void errorHandling(String message)
{

  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);

  tft.begin(); // initialize a ST7789 chip
  tft.setRotation(1);
  tft.setSwapBytes(true); // Swap the byte order for pushImage() - corrects endianness
  tft.fillScreen(TFT_BLACK);

  digitalWrite(12, HIGH);


  errorspray.setColorDepth(8);
  errorspray.createSprite(280,240);
  errorspray.fillSprite(TFT_BLACK);
  errorspray.setSwapBytes(true);
  errorspray.setTextWrap(true, false);
  errorspray.setTextColor(TFT_WHITE, TFT_BLACK);
  errorspray.setTextSize(2);
  errorspray.drawString(message,5,110);  
  errorspray.pushSprite(0,0,TFT_BLACK);


  delay(10000);
  ESP.restart();
}

//////////////////////////////////////////////////////////////////////////
// Build Constructor
//////////////////////////////////////////////////////////////////////////

ELM327 myELM327;

//////////////////////////////////////////////////////////////////////////
// Setup
//////////////////////////////////////////////////////////////////////////

void setup(void)
{
pinMode(12, OUTPUT);
digitalWrite(12, LOW);

#if LED_BUILTIN
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif

  DEBUG_PORT.begin(115200);
//  SerialBT.setPin("1234");
  ELM_PORT.begin("ArduHUD", true);
  
  if (!ELM_PORT.connect(address))
  //if (!ELM_PORT.connect("ELMULATOR"))
  {
    DEBUG_PORT.println(F("Couldn't connect to OBD scanner - Bluetooth Connection Problem"));
    message="Bluetooth Connection Problem";
    errorHandling(message);    
  }

  if (!myELM327.begin(ELM_PORT, false, 2000))
  {
    Serial.println(F("Couldn't connect to OBD scanner - ELMduino Problem"));
    message="ELMduino Problem";
    errorHandling(message);
  }

  Serial.println(F("Connected to ELM327"));

  if (!LittleFS.begin()) {
    Serial.println(F("Flash FS initialisation failed!"));
    message="Flash FS initialisation failed!";
    errorHandling(message);
  }
  Serial.println("\n\Flash FS available!");

  bool font_missing = false;
  if (LittleFS.exists("/NotoSansBold15.vlw")    == false) font_missing = true;
  if (LittleFS.exists("/NotoSansBold36.vlw")    == false) font_missing = true;
  if (LittleFS.exists("/Latin-Hiragana-24.vlw")    == false) font_missing = true;

  if (font_missing)
  {
    Serial.println("\nFont missing in Flash FS, did you upload it?");
    message="Font missing in Flash FS!";
    errorHandling(message);
  }
  else Serial.println("\nFonts found OK.");

  //pinMode(4, INPUT);

  tft.begin(); // initialize a ST7789 chip
  tft.setRotation(1);
  tft.setSwapBytes(true); // Swap the byte order for pushImage() - corrects endianness
  tft.fillScreen(TFT_BLACK);
  
  opellogo.setColorDepth(8);
  opellogo.createSprite(280, 240);
  opellogo.setSwapBytes(true);
  opellogo.fillSprite(TFT_BLACK);
  opellogo.pushImage(0, 0, 280, 240, opel);
  opellogo.pushSprite(0, 0);

  digitalWrite(12, HIGH);

  delay(3000);
  tft.fillScreen(TFT_BLACK);
  opellogo.deleteSprite();

  // ui
  ui.setColorDepth(8);
  ui.createSprite(280,240);
  ui.fillSprite(TFT_BLACK);
  ui.setSwapBytes(true);
  ui.setTextColor(TFT_WHITE, TFT_BLACK);
  ui.pushImage(34,175,72,54,battery);
  ui.loadFont(AA_FONT_LARGE, LittleFS);
  ui.drawString("V",220,190);
  ui.unloadFont();
  ui.loadFont(AA_FONT_MEDIUM, LittleFS);
  ui.drawString("Last",48,138);
  ui.drawString("Kuehlwa.",160,138);
  ui.unloadFont();
  tft.fillCircle(70,70,60,DARKER_GREY);
  tft.drawSmoothCircle(70,70,60,TFT_SILVER,DARKER_GREY);
  tft.drawArc(70,70,57,57-57/5,30,330,TFT_BLACK,DARKER_GREY);
  ui.loadFont(AA_FONT_MEDIUM, LittleFS);
  ui.drawString("%",61,105);
  tft.fillCircle(210,70,60,DARKER_GREY);
  tft.drawSmoothCircle(210,70,60,TFT_SILVER,DARKER_GREY);
  tft.drawArc(210,70,57,57-57/5,30,330,TFT_BLACK,DARKER_GREY); 
  ui.drawSmoothCircle(200,108,3,TFT_WHITE,DARKER_GREY);
  ui.drawString("C",206,105); 
  ui.pushSprite(0,0,TFT_BLACK);
  ui.unloadFont();

  // Voltage Reading
  volt.createSprite(80, 30);
  volt.fillSprite(TFT_BLACK);
  volt.setTextColor(TFT_WHITE, TFT_BLACK);

  buttonspray.createSprite(30, 30);

  // Boost Reading
  boosttxt.createSprite(75, 30);
  boosttxt.fillSprite(DARKER_GREY);
  boosttxt.setTextColor(TFT_WHITE, DARKER_GREY);

  // Coolant Reading
  coolanttxt.createSprite(75, 30);
  coolanttxt.fillSprite(DARKER_GREY);
  coolanttxt.setTextColor(TFT_WHITE, DARKER_GREY);

  drawBoost(0.00);
  drawCoolant(00);

}

//////////////////////////////////////////////////////////////////////////
// Loop
//////////////////////////////////////////////////////////////////////////

void loop()
{

  switch (obd_state)
  {

  case LOAD:
  {
    load = myELM327.engineLoad();

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
      Serial.print("LOAD: ");
      Serial.print(load);
      Serial.println(" %");
      obd_state = COOLANT;
      drawBoost(load);
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
      myELM327.printError();
      obd_state = COOLANT;
    }

    break;
  }

  case COOLANT:
  {
    float coolant = myELM327.engineCoolantTemp();

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
      Serial.print("Coolant: ");
      Serial.print(coolant);
      Serial.println(" °C");
      obd_state = VOLTAGE;
      drawCoolant(coolant);
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
      myELM327.printError();
      obd_state = VOLTAGE;
    }

    break;
  }

  case VOLTAGE:
  {
    float battery = myELM327.batteryVoltage();

    if (myELM327.nb_rx_state == ELM_SUCCESS)
    {
      Serial.print("Battery: ");
      Serial.print(battery);
      Serial.println(" V");
      obd_state = LOAD;
      drawVolt(battery);
    }
    else if (myELM327.nb_rx_state != ELM_GETTING_MSG)
    {
      myELM327.printError();
      obd_state = LOAD;
    }

    break;
  }
  }
}
