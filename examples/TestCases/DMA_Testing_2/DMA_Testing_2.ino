#include "RA8889_Config_8080.h"
#include <RA8889_t41_p.h>

// RA8889_8080_DC, RA8889_8080_CS and RA8889_8080_RESET are defined in
// src/RA8889_Config_8080.h.
RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

uint32_t start = 0;
uint32_t end = 0;

#if defined(ARDUINO_TEENSY_DEVBRD5)
uint16_t *sdram_image;
#endif

void setup() {
    while (!Serial && millis() < 3000) {}  //wait for Serial Monitor
    Serial.printf("%c DB5 Board and RA8889 parallel 8080 mode testing (8Bit/16Bit/DMA) with SDRAM buffer\n\n", 12);
    //  Serial.print(CrashReport);

#if !defined(ARDUINO_TEENSY_DEVBRD5)
    Serial.println("** THIS SKETCH ONLY WORKS WITH THE DEV BOARD 5 THAT HAS SDRAM. STOPPING HERE... **");
    while(1);
#endif
	// Set 8/16bit bus mode. Default is 8bit bus mode.
	tft.setBusWidth(RA8889_8080_BUS_WIDTH); // RA8889_8080_BUS_WIDTH is defined in
                                          // src/RA8889_Config_8080.h. 
	tft.begin(BUS_SPEED); // RA8889_8080_BUS_WIDTH is defined in
                        // src/RA8889_Config_8080.h. Default is 20MHz. 

    Serial.print("Bus speed: ");
    Serial.print(BUS_SPEED, DEC);
    Serial.println(" MHZ");
    Serial.print("Bus Width: ");
    Serial.print(tft.getBusWidth(), DEC);
    Serial.println("-bits");

    tft.graphicMode(true);
    tft.setRotation(0);

#if defined(ARDUINO_TEENSY_DEVBRD5)
    sdram_image = (uint16_t*)sdram_malloc(1024*600*2);
    // lets fill it with RED, have a blue ring around edges. and Green one pixel around the edge.
    uint32_t x, y;
    uint16_t *pb = sdram_image;
    for (x = 0; x < (1024 * 600); x++) pb[x] = RED;

    for (x = 0; x < 1024; x++) pb[x] = GREEN;
   
    for (y = 1; y < 11; y++) {
        pb = &sdram_image[y * 1024];
        for (x = 1; x < 1023; x++) pb[x] = BLUE;
        pb[0] = GREEN;
        pb[1023] = GREEN;
    }
    for (; y < 589; y++) {
        pb = &sdram_image[y * 1024];
        pb[0] = GREEN;
        pb[1023] = GREEN;

        for (x = 1; x < 11; x++) pb[x] = BLUE;
        for (x = 1003; x < 1023; x++) pb[x] = BLUE;
    }
    for (; y < 599; y++) {
        pb = &sdram_image[y * 1024];
        for (x = 1; x < 1023; x++) pb[x] = BLUE;
        pb[0] = GREEN;
        pb[1023] = GREEN;
    }
    pb += 1024; // last row.
    for (x = 0; x < 1024; x++) pb[x] = GREEN;
#endif
}

void loop() {
#if defined(ARDUINO_TEENSY_DEVBRD5)
    tft.fillScreen(YELLOW);
    waitforInput();
    tft.writeRect(0, 0, 1024, 600, sdram_image);
    waitforInput();
    tft.fillScreen(MAGENTA);
    waitforInput();
    tft.pushPixels16bitDMA(sdram_image, 0, 0, 1024, 600);  // FLASHMEM buffer
#endif
    waitforInput();
}

void waitforInput() {
    Serial.println("Press anykey to continue");
    while (Serial.read() == -1)
        ;
    while (Serial.read() != -1)
        ;
}
