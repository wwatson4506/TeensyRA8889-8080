/* RA8889_Config_8080.h
 A file to place user defines and configs.
*/

#ifndef RA8889_CONFIG_8080_H
#define RA8889_CONFIG_8080_H

// Uncomment next line to increase SDRAM and CORE clock speeds.
//#define USE_FAST_CLOCK_MODE

// Select 8 or 16 for your bus width.
#define RA8889_8080_BUS_WIDTH 16

// Set the bus speed in megahertz.
#define BUS_SPEED 20 //Available settings 2,4,8,12,20,24,30,40,60,120

// The following are the default defines for the Teensy 4.1 and Dev Board 5 (DB5).
// External backlight control connected to this Arduino pin. Can be controlled with PWM.
// Otherwise 3.3v
// Un-comment this define for pin control of backlite.
//#define BACKLITE 5 or change to your pin choice.

// The following are the default defines for the Teensy 4.1 and Dev Board 5 (DB5).
#if defined(ARDUINO_TEENSY41)
// Hardware defines T4.1
#define RA8889_8080_CS 11
#define RA8889_8080_DC 13
#define RA8889_8080_RESET 12
// Example usage in sketch: RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

#define RA8889_D0 19
#define RA8889_RD 37    // FlexIO3:19: RD
#define RA8889_WR 36    // FlexIO3:18: WR

#elif defined(ARDUINO_TEENSY_DEVBRD5)
// Hardware defines DB5 board and Kurt's shield
#define RA8889_8080_CS 53
#define RA8889_8080_RESET 54
#define RA8889_8080_DC 55
// Usage in sketch: RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

#define RA8889_D0 40
#define BACKLITE 29
#define RA8889_RD 52    // FlexIO3:10: RD
#define RA8889_WR 56    // FlexIO3:11 WR
#endif

// Uncomment to use FT5206 touch. (Not used on the ER-TFTM1010-1)
#define USE_FT5206_TOUCH

#endif // RA8889_CONFIG_H
