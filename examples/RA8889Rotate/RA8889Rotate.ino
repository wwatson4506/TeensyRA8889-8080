#include "RA8889_Config_8080.h"
#include <RA8889_t41_p.h>
#include "font_Arial.h"

// RA8889_8080_DC, RA8889_8080_CS and RA8889_8080_RESET are defined in
// src/RA8889_Config_8080.h.
RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

#define PINK        0xFC18
#define REG_DUMP_CNT  0 //0x70
#define USE_STATUS_LINE

uint8_t reg_values[REG_DUMP_CNT];

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

  Serial.println("Setup");

  tft.graphicMode(true);
  Serial.printf("Before W: %d H: %d\n", tft.width(), tft.height());
  tft.fillScreen(LIGHTYELLOW);
  drawTestScreen();
}

void drawTestScreen() {
#ifdef USE_STATUS_LINE
  int image_height = tft.height() - STATUS_LINE_HEIGHT; 
#else
  int image_height = tft.height()
#endif

  int centerx = tft.width() / 2;
  int centery = image_height / 2;
  tft.fillRect(tft.width() - 50, 0, 50, 50, GREEN);
  tft.fillRect(tft.width() - 50, image_height - 50 , 50, 50, BLUE);
  tft.fillRect(0, image_height - 50 , 50, 50, ORCHID);
  tft.drawLine(0, 0, tft.width(), image_height, WHITE);
  tft.drawLine(0, image_height, tft.width(), 0, BLACK);
  tft.drawRect(centerx - 50, centery / 2, 100, 50, BLUE);
  tft.fillRect(centerx - 40, centery / 2 + 10, 80, 30, GREEN);
  tft.drawLine(centerx - 50, centery / 2, centerx, 0, RED);
  tft.drawLine(centerx + 50, centery / 2, centerx, 0, CRIMSON);
  tft.drawLine(centerx, centery / 2, centerx, 0, GREEN);
  tft.drawTriangleFill(centerx, centery, centerx - 100, centery + 100, centerx + 100, centery + 100, CRIMSON);
  tft.fillRect(0, 0, 50, 50, RED);
  tft.fillCircle(centerx, centery, 50, ORCHID);
  tft.drawCircle(centerx, centery, 25, BLACK);
  tft.drawTriangle(50, 50, 50, image_height - 50, centerx - 50, centery, BLUE);
  tft.fillRoundRect(100, centery - 50, 150, 100, 25, 25, GREEN);
  tft.drawRoundRect(110, centery - 40, 130, 80, 25, 25, WHITE);
  tft.drawEllipse(tft.width() - 100, centery, 50, 35, BLUE);
  tft.drawEllipseFill(tft.width() - 100, centery, 40, 25, RED);

  tft.setFont(Arial_14);
  tft.setTextColor(BLACK);
  tft.setCursor(25, centery);
  tft.printf("Rotation: %d", tft.getRotation());
  tft.setCursor(50, 50);
  tft.write('0');
  tft.setCursor(tft.width() - 50, 50);
  tft.write('1');
  tft.setCursor(50, image_height - 50);
  tft.write('2');
  tft.setCursor(tft.width() - 50, image_height - 50);
  tft.write('3');

  tft.printStatusLine(0, RED, GREEN, "Status Text");

#if REG_DUMP_CNT > 0
  Serial.print("LCD Register dump:");
  for (uint8_t reg = 0; reg < REG_DUMP_CNT; reg++) {
    uint8_t r_value = tft.lcdRegDataRead(reg);
    if ((reg & 0xf) == 0) Serial.printf("\n%02x -", reg);
    Serial.printf(" %c%02x",
                  (r_value == reg_values[reg]) ? ' ' : '*',
                  r_value);
    reg_values[reg] = r_value;
  }
  Serial.print("\n");
#endif
}

void loop() {
  // put your main code here, to run repeatedly:
  wait_for_keyboard();  // see if all yellow
  uint8_t rotation = (tft.getRotation() + 1) & 0x3;
  tft.setRotation(rotation);

  Serial.printf("Rotation: %d After W: %d H: %d\n", tft.getRotation(), tft.width(), tft.height());
  tft.fillScreen(YELLOW);
  //wait_for_keyboard();  // see if all yellow
  drawTestScreen();
  //tft.textRotate(false);
  Serial.printf("MACR and End: %x\n", tft.lcdRegDataRead(RA8889_MACR));

}
void wait_for_keyboard() {
  Serial.println("Wait for any key to continue");
  while (Serial.read() == -1);
  while (Serial.read() != -1);
}
