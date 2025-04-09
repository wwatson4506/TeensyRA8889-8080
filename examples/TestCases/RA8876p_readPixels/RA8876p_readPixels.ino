#include "Arduino.h"
#include "RA8889_Config_8080.h"
#include <RA8889_t41_p.h>
#include "font_Arial.h"

// RA8889_8080_DC, RA8889_8080_CS and RA8889_8080_RESET are defined in
// src/RA8889_Config_8080.h.
RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

// Array of Simple RA8889 Basic Colors
PROGMEM uint16_t myColors[] = {
	0x0000, 0xffff,	0xf800,	0xfc10,	0x8000,	0x07e0,	0x87f0,	0x0400,
	0x001f,	0x051f,	0x841f,	0x0010,	0xffe0,	0xfff0,	0x8400,	0x07ff,
	0x87ff,	0x0410,	0xf81f,	0xfc1f,	0x8010,	0xA145
};

int interations = 0;
int w, h;
int i = 0;

#define BAND_WIDTH 16
#define BAND_HEIGHT 40
#define BAND_START_X 200
#define BAND_START_Y 200
uint16_t pixel_data[5500];
uint8_t test_screen_rotation = 0;

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

  tft.graphicMode(true);
  tft.setTextCursor(0,0);
  tft.setFont(Arial_14);
}

void testGetPixel(uint8_t rotation) {
  tft.setRotation(rotation);
  Serial.printf("TFT Width: %d, Height: %d\n", tft.width(), tft.height());
  Serial.printf("ROTATION: %d\n", rotation);
  tft.fillScreen(0xf800);
  w = tft.width()-1; h = tft.height()-STATUS_LINE_HEIGHT-1;
  tft.printStatusLine(0,myColors[1],myColors[11], "Status Text");
  tft.fillRect(0, 0, 400, 200, myColors[8]);
  uint16_t pixel =  tft.getPixel(200, 100);
  Serial.printf("Rect Color: 0x%x, Pixel Color: 0x%x\n", myColors[8], pixel);

  tft.fillCircle(200, 100, 50, GREEN);
  Serial.printf("Circle Color: 0x%x, Pixel Color: 0x%x\n", GREEN, tft.getPixel(200, 100));

  tft.setCursor(195, 95);
  tft.print(rotation);

}

void colorBar(uint8_t rotation){
  tft.setRotation(rotation);
  tft.fillScreen(BLACK);
  tft.drawRect(BAND_START_X-1, BAND_START_Y-1, BAND_WIDTH * 8 + 2, BAND_HEIGHT + 2, WHITE);

  tft.fillRect(BAND_START_X + BAND_WIDTH * 0, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, RED);
  tft.fillRect(BAND_START_X + BAND_WIDTH * 1, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, GREEN);
  tft.fillRect(BAND_START_X + BAND_WIDTH * 2, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, BLUE);
  tft.fillRect(BAND_START_X + BAND_WIDTH * 3, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, BLACK);
  tft.fillRect(BAND_START_X + BAND_WIDTH * 4, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, WHITE);
  tft.fillRect(BAND_START_X + BAND_WIDTH * 5, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, YELLOW);
  tft.fillRect(BAND_START_X + BAND_WIDTH * 6, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, CYAN);
  tft.fillRect(BAND_START_X + BAND_WIDTH * 7, BAND_START_Y, BAND_WIDTH, BAND_HEIGHT, 0xF81F);
  tft.printStatusLine(0,myColors[1],myColors[11], "Status Text");
  memset(pixel_data, 0, sizeof(pixel_data));
  tft.readRect(BAND_START_X, BAND_START_Y, BAND_WIDTH * 8, BAND_HEIGHT, pixel_data);
  tft.writeRect(BAND_START_X, BAND_START_Y + BAND_HEIGHT, BAND_WIDTH * 8, BAND_HEIGHT, pixel_data);
}


//=============================================================================
// Wait for user input
//=============================================================================
void WaitForUserInput() {
    Serial.println("Hit Enter to continue");
    Serial.flush();
    while (Serial.read() == -1)
        ;
    while (Serial.read() != -1)
        ;
}

void test() {
  tft.setTextColor(BLACK);
   
  testGetPixel(0);
  WaitForUserInput();
  testGetPixel(1);
  WaitForUserInput();
  testGetPixel(2);
  WaitForUserInput();
  testGetPixel(3);
  WaitForUserInput();

  colorBar(0);
  WaitForUserInput();
  colorBar(1);
  WaitForUserInput();
  colorBar(2);
  WaitForUserInput();
  colorBar(3);
  WaitForUserInput();

}
void loop() {
  //testScreenRotation();
  test();
}

void testScreenRotation(){
  Serial.printf("\nRotation: %d\n", test_screen_rotation);
  tft.setRotation(test_screen_rotation);
  tft.fillScreen(RED);
  tft.setCursor(tft.width()/2, tft.height()/2, true);
  tft.printf("Rotation: %d", test_screen_rotation);
  test_screen_rotation = (test_screen_rotation + 1) & 0x3;
  tft.setCursor(200, 300);
  Serial.printf("  Set cursor(200, 300), retrieved(%d %d)",
                tft.getCursorX(), tft.getCursorY());
  tft.setCursor(50, 50);
  tft.write('0');
  tft.setCursor(tft.width() - 50, 50);
  tft.write('1');
  tft.setCursor(50, tft.height() - 50);
  tft.write('2');
  tft.setCursor(tft.width() - 50, tft.height() - 50);
  tft.write('3');
  delay(1000);

}
