#include "flexio_teensy_mm.c"
#include "teensy41.c"
#include "RA8889_Config_8080.h"
#include <RA8889_t41_p.h>

// RA8889_8080_DC, RA8889_8080_CS and RA8889_8080_RESET are defined in
// src/RA8889_Config_8080.h.
RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

uint32_t start = 0;
uint32_t end =  0;

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

  Serial.printf("%c DB5 board and RA8889 parallel 8080 mode testing (8-Bit/16-bit,DMA/ASYNC)\n\n",12);
//  Serial.print(CrashReport);

  Serial.print("Bus speed: ");
  Serial.print(BUS_SPEED,DEC);
  Serial.println(" MHZ");
  Serial.print("Bus Width: ");
  Serial.print(tft.getBusWidth(),DEC);
  Serial.println("-bits");

  tft.graphicMode(true);
  tft.fillScreen(0x0000);
  tft.setRotation(0);
}

void loop() {
  start = micros();
  tft.pushPixels16bitAsync(flexio_teensy_mm,530,260,480,320); // 480x320

#if defined(ARDUINO_TEENSY_DEVBRD5) || defined(ARDUINO_TEENSY_MICROMOD)

// *********************** DMA not working properly in 16bit bus mode with bus speed greater than 12MHz ************************
  if(RA8889_8080_BUS_WIDTH == 16) 
    Serial.printf("**************** DMA not working properly in 16bit bus mode with bus speed greater than 12MHz *******************\n");
// *****************************************************************************************************************************

  tft.pushPixels16bitDMA(teensy41,1,1,480,320);  // FLASHMEM buffer. 16-Bit bus width fails with bus speed
                                                 // above 12 MHZ. Causes distorted image. SDRAM buffer is ok.
#endif //DMA is not available on the T4.x in 8080 parallel mode.

  end = micros() - start;
  Serial.printf("Wrote %d bytes in %dus\n\n",(480*320)*2, end);
  waitforInput();
}

void waitforInput()
{
  Serial.println("Press anykey to continue");
  while (Serial.read() == -1) ;
  while (Serial.read() != -1) ;
}
