#include <Adafruit_GFX.h>
#include <_font_ComicSansMS.h>
#include "Fonts/FreeSansOblique12pt7b.h"
#include "RA8889_Config_8080.h"
#include "Arduino.h"
#include <RA8889_t41_p.h>

// RA8889_8080_DC, RA8889_8080_CS and RA8889_8080_RESET are defined in
// src/RA8889_Config_8080.h.
RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

void setup() {
  //I'm guessing most copies of this display are using external PWM
  //backlight control instead of the internal RA8889 PWM.
  //Connect a Teensy pin to pin 14 on the display.
  //Can use analogWrite() but I suggest you increase the PWM frequency first so it doesn't sing.
#if defined(BACKLITE)
  pinMode(BACKLITE, OUTPUT);
  digitalWrite(BACKLITE, HIGH);
#endif

  Serial.begin(115200);
  while (!Serial && millis() < 1000) {} //wait for Serial Monitor

  // Set 8/16bit bus mode. Default is 8bit bus mode.
  tft.setBusWidth(RA8889_8080_BUS_WIDTH); // RA8889_8080_BUS_WIDTH is defined in
                                          // src/RA8889_Config_8080.h. 
  tft.begin(BUS_SPEED); // RA8889_8080_BUS_WIDTH is defined in
                        // src/RA8889_Config_8080.h. Default is 20MHz. 

  //tft.setRotation(1);
  tft.fillScreen(BLACK);
  tft.setFontSize(1,true); // (1) = 12x24 only available internal font size on RA8889.
  tft.setTextColor(WHITE);
  tft.println("Test of the default font. (RA8889 only has 12x24 internal font size available!)");
  tft.println();
//  wait_for_keyboard();

  tft.setTextColor(WHITE, BLUE);
  tft.setFont(ComicSansMS_18);
  tft.println("Opaque ILI font ComicSansMS_18");
  tft.println();

//  wait_for_keyboard();
  int cursor_x = tft.getCursorX();
  int cursor_y = tft.getCursorY();
  tft.fillRect(cursor_x, cursor_y, tft.width(), 20, RED);
//  wait_for_keyboard();
  tft.setTextColor(GREEN);
  tft.println("Transparent ILI ComicSansMS_18");
  tft.println();
//  wait_for_keyboard();

  tft.setTextColor(WHITE, RED);
  tft.setFont(&FreeSansOblique12pt7b);
  tft.println("Opaque GFX FreeSansOblique12pt");
//  wait_for_keyboard();

  cursor_x = tft.getCursorX();
  cursor_y = tft.getCursorY();
  tft.fillRect(cursor_x, cursor_y, tft.width(), 20, YELLOW);
//  wait_for_keyboard();
  tft.setTextColor(BLUE);
  tft.println("Transparent GFX");

//  wait_for_keyboard();
  tft.setFontDef();
  tft.setTextColor(GREEN);
  tft.setFontSize(1, true);
  tft.println("This is default font:");
  //tft.setFontSpacing(1);//now give 5 pix extra spacing between chars
  tft.println("ABCDEF 1 2 3 4 5 6 7");

}

void wait_for_keyboard() {
  Serial.println("Wait for any key to continue");
  while (Serial.read() == -1);
  while (Serial.read() != -1);
}

void loop()
{  }
