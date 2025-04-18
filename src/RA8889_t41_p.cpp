//**************************************************************//
// Teensy 4.1 8080 Parallel 8/16 bit with 8 bit ASYNC support.
//**************************************************************//
/*
 * RA8876LiteTeensy.cpp
 * Modified Version of: File Name : RA8889_t41_p.cpp
 *          Author    : RAiO Application Team
 *          Edit Date : 09/13/2017
 *          Version   : v2.0  1.modify bte_DestinationMemoryStartAddr bug
 *                            2.modify RA8889SdramInitial Auto_Refresh
 *                            3.modify RA8889PllInitial
 ****************************************************************
 *                    : New 8080 Parallel version
 *                    : For MicroMod
 *                    : By Warren Watson
 *                    : 06/07/2018 - 05/03/2024
 *                    : Copyright (c) 2017-2024 Warren Watson.
 *****************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
//**************************************************************//

#include "RA8889_t41_p.h"
#include "Arduino.h"
#include "RA8889_t41_p_default_pins.h"

#define READS_USE_DIGITAL_WRITE

inline void RA8889_t41_p::RDHigh() {
#ifdef READS_USE_DIGITAL_WRITE
   digitalWriteFast(_rd_pin, HIGH);
#endif
}

inline void RA8889_t41_p::RDLow() {
#ifdef READS_USE_DIGITAL_WRITE
   digitalWriteFast(_rd_pin, LOW);
#endif
}

//#define DEBUG
//#define DEBUG_VERBOSE
//#define DEBUG_FLEXIO

#ifndef DEBUG
#undef DEBUG_VERBOSE
void inline DBGPrintf(...){};
void inline DBGWrite(uint8_t ch) {};
void inline DBGFlush(){};
#else
#define DBGPrintf Serial.printf
#define DBGFlush Serial.flush
#define DBGWrite Serial.write
#endif

#ifndef DEBUG_VERBOSE
void inline VDBGPrintf(...){};
#else
#define VDBGPrintf Serial.printf
#endif

//**************************************************************//
// RA8889_t41_p()
//**************************************************************//
// Create RA8889 driver instance 8080 IF
//**************************************************************//
#ifndef BUS_WIDTH
#define BUS_WIDTH 8
#endif
RA8889_t41_p::RA8889_t41_p(const uint8_t DCp, const uint8_t CSp, const uint8_t RSTp) 
    : _dc(DCp), _cs(CSp), _rst(RSTp),
      _data_pins{DISPLAY_D0, DISPLAY_D1, DISPLAY_D2, DISPLAY_D3, DISPLAY_D4, DISPLAY_D5, DISPLAY_D6, DISPLAY_D7,
#if defined(DISPLAY_D8)
      DISPLAY_D8, DISPLAY_D9, DISPLAY_D10, DISPLAY_D11, DISPLAY_D12, DISPLAY_D13, DISPLAY_D14, DISPLAY_D15}, 
#else
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
#endif      
      _wr_pin(DISPLAY_WR), _rd_pin(DISPLAY_RD) 
{
    RA8889_GFX(BUS_WIDTH);
}

FLASHMEM boolean RA8889_t41_p::begin(uint8_t baud_div) {
    switch (baud_div) {
    case 2:
        _baud_div = 120;
        break;
    case 4:
        _baud_div = 60;
        break;
    case 8:
        _baud_div = 30;
        break;
    case 12:
        _baud_div = 20;
        break;
    case 20:
        _baud_div = 12;
        break;
    case 24:
        _baud_div = 10;
        break;
    case 30:
        _baud_div = 8;
        break;
    case 40:
        _baud_div = 6;
        break;
    case 60:
        _baud_div = 4;
        break;
    case 120:
        _baud_div = 2;
        break;
    default:
        _baud_div = 20; // 12Mhz
        break;
    }
    pinMode(_cs, OUTPUT);  // CS
    pinMode(_dc, OUTPUT);  // DC
    pinMode(_rst, OUTPUT); // RST
    *(portControlRegister(_cs)) = 0xFF;
    *(portControlRegister(_dc)) = 0xFF;
    *(portControlRegister(_rst)) = 0xFF;

    digitalWriteFast(_cs, HIGH);
    digitalWriteFast(_dc, HIGH);
    digitalWriteFast(_rst, HIGH);

    delay(15);
    digitalWrite(_rst, LOW);
    delay(15);
    digitalWriteFast(_rst, HIGH);
    delay(100);

    FlexIO_Init();

    if (!checkIcReady())
        return false;

    // read ID code must disable pll, 01h bit7 set 0
    if (_bus_width == 16) {
        lcdRegDataWrite(0x01, 0x09);
    } else {
        lcdRegDataWrite(0x01, 0x08);
    }
    delay(1);

	if (readIdCode() != 0x89) {
		Serial.println("RA8889 not found!");
		return false;
	}

    // Turn on data line pullups.
    if(_bus_width == 16) {
		lcdRegDataWrite(RA8889_PUENR, 3, true);
    } else {
		lcdRegDataWrite(RA8889_PUENR, 1, true);
	} 

    // Initialize RA8889 to default settings
    if (!RA8889Initialize()) {
        return false;
    }

    return true;
}

//**************************************************************//
/*  page = 0 or 1                                               */
//**************************************************************//
void RA8889_t41_p::registerPageSwitch(ru8 page)
{
  if(page == 0)
  lcdRegDataWrite(RA8889_CMC,RA8889_REGISTER_PAGE0);//46h
  else if(page == 1)
  lcdRegDataWrite(RA8889_CMC,RA8889_REGISTER_PAGE1);//46h
}

//**************************************************************//	
 //**************************************************************//
 ru8 RA8889_t41_p:: readIdCode(void)
 { ru8 temp;
    //read ID code must disable pll, 01h bit7 set 0
  lcdRegDataWrite(0x01,0x08);
  delay(1);
  registerPageSwitch(1);
  temp = lcdRegDataRead(0xff);
  registerPageSwitch(0);
  return temp;
 }

FASTRUN void RA8889_t41_p::CSLow() {
    digitalWriteFast(_cs, LOW); // Select TFT
}

FASTRUN void RA8889_t41_p::CSHigh() {
    digitalWriteFast(_cs, HIGH); // Deselect TFT
}

FASTRUN void RA8889_t41_p::DCLow() {
    digitalWriteFast(_dc, LOW); // Writing command to TFT
}

FASTRUN void RA8889_t41_p::DCHigh() {
    digitalWriteFast(_dc, HIGH); // Writing data to TFT
}

FASTRUN void RA8889_t41_p::microSecondDelay() {
    for (uint32_t i = 0; i < 99; i++)
        __asm__("nop\n\t");
}

FASTRUN void RA8889_t41_p::gpioWrite() {
    pFlex->setIOPinToFlexMode(_wr_pin);
#ifndef READS_USE_DIGITAL_WRITE
    pinMode(_rd_pin, OUTPUT);
#endif    
    digitalWriteFast(_rd_pin, HIGH);
}

FASTRUN void RA8889_t41_p::gpioRead() {
#ifndef READS_USE_DIGITAL_WRITE
    pFlex->setIOPinToFlexMode(_rd_pin);
#endif    
    pinMode(_wr_pin, OUTPUT);
    digitalWriteFast(_wr_pin, HIGH);
}

// If used this must be called before begin
// Set the FlexIO pins.  The first version you can specify just the wr, and read and optionsl first Data.
// it will use information in the Flexio library to fill in d1-d7
FASTRUN bool RA8889_t41_p::setFlexIOPins(uint8_t write_pin, uint8_t rd_pin, uint8_t tft_d0) {
    DBGPrintf("RA8889_t41_p::setFlexIOPins(%u, %u, %u) %u %u %u\n", write_pin, rd_pin, tft_d0, _data_pins[0], _wr_pin, _rd_pin);
    DBGFlush();
    if (tft_d0 != 0xff) {
#ifdef FLEX_IO_HAS_FULL_PIN_MAPPING
        DBGPrintf("\td0 != 0xff\n\n");

        uint8_t flexio_pin;
        pFlex = FlexIOHandler::mapIOPinToFlexIOHandler(tft_d0, flexio_pin);
        if ((pFlex == nullptr) || (flexio_pin == 0xff))
            return false;

        uint8_t flexio_wr = pFlex->mapIOPinToFlexPin(write_pin);
        if (flexio_wr == 0xff) {
            // WR pin not valid on pFlex, see if maybe 2 versus 3 issue...
            pFlex = FlexIOHandler::mapIOPinToFlexIOHandler(write_pin, flexio_wr);
            if (pFlex == nullptr) {
                Serial.printf("RA8889_t41_p::FlexIO_Init() wr:%u is not valid flexio pin\n", write_pin);
                return false;            
            }
            flexio_pin = pFlex->mapIOPinToFlexPin(tft_d0);
            if (flexio_pin == 0xff) {
                Serial.printf("RA8889_t41_p::FlexIO_Init() D0:%u and WR:%u are not on same flexio object", tft_d0, write_pin);
            }
        }
        _data_pins[0] = tft_d0;


        // lets dos some quick validation of the pins.
        for (uint8_t i = 1; i < _bus_width; i++) {
            flexio_pin++; // lets look up the what pins come next.
            _data_pins[i] = pFlex->mapFlexPinToIOPin(flexio_pin);
            if (_data_pins[i] == 0xff) {
                Serial.printf("Failed to find Teensy IO pin for Flexio pin %u\n", flexio_pin);
                return false;
            }
        }
#else
        return false;
#endif
    }
    // set the write and read pins and see if d0 is not 0xff set it and compute the others.
    if (write_pin != 0xff)
        _wr_pin = write_pin;
    if (rd_pin != 0xff)
        _rd_pin = rd_pin;

    DBGPrintf("FlexIO pins: data: %u %u %u %u %u %u %u %u WR:%u RD:%u\n",
              _data_pins[0], _data_pins[1], _data_pins[2], _data_pins[3], _data_pins[4], _data_pins[5], _data_pins[6], _data_pins[7],
              _wr_pin, _rd_pin);
    return true;
}

// Set the FlexIO pins.  Specify all of the pins for 8 bit mode. Must be called before begin
FLASHMEM bool RA8889_t41_p::setFlexIOPins(uint8_t write_pin, uint8_t rd_pin, uint8_t d0, uint8_t d1, uint8_t d2, uint8_t d3,
                                           uint8_t d4, uint8_t d5, uint8_t d6, uint8_t d7,
                                            uint8_t d8, uint8_t d9, uint8_t d10, uint8_t d11,
                                            uint8_t d12, uint8_t d13, uint8_t d14, uint8_t d15
                                           ) {
    _data_pins[0] = d0;
    _data_pins[1] = d1;
    _data_pins[2] = d2;
    _data_pins[3] = d3;
    _data_pins[4] = d4;
    _data_pins[5] = d5;
    _data_pins[6] = d6;
    _data_pins[7] = d7;
    _wr_pin = write_pin;
    _rd_pin = rd_pin;

    _data_pins[8] = d8;
    _data_pins[9] = d9;
    _data_pins[10] = d10;
    _data_pins[11] = d11;
    _data_pins[12] = d12;
    _data_pins[13] = d13;
    _data_pins[14] = d14;
    _data_pins[15] = d15;
    DBGPrintf("FlexIO pins: data: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u WR:%u RD:%u\n",
              _data_pins[0], _data_pins[1], _data_pins[2], _data_pins[3], _data_pins[4], _data_pins[5], _data_pins[6], _data_pins[7],
              _data_pins[8], _data_pins[9], _data_pins[10], _data_pins[11], _data_pins[12], _data_pins[13], _data_pins[14], _data_pins[15],
              _wr_pin, _rd_pin);
    // Note this does not verify the pins are valid.
    return true;
}



FASTRUN void RA8889_t41_p::FlexIO_Init() {
    /* Get a FlexIO channel */
    // lets assume D0 is the valid one...
    DBGPrintf("FlexIO_Init: D0:%u WR:%u RD:%u\n", _data_pins[0], _wr_pin, _rd_pin);

    pFlex = FlexIOHandler::mapIOPinToFlexIOHandler(_data_pins[0], _flexio_D0);
    if (pFlex == nullptr) {
        Serial.printf("RA8889_t41_p::FlexIO_Init() d0:%u is not valid flexio pin\n", _data_pins[0]);
        return;
    }

    // See if the WR and Read pins map...
    _flexio_WR = pFlex->mapIOPinToFlexPin(_wr_pin);
    if (_flexio_WR == 0xff) {
        // WR pin not valid on pFlex, see if maybe 2 versus 3 issue...
        pFlex = FlexIOHandler::mapIOPinToFlexIOHandler(_wr_pin, _flexio_WR);
        if (pFlex == nullptr) {
            Serial.printf("RA8889_t41_p::FlexIO_Init() wr:%u is not valid flexio pin\n", _wr_pin);
            return;            
        }
        _flexio_D0 = pFlex->mapIOPinToFlexPin(_data_pins[0]);
        if (_flexio_D0 == 0xff) {
            Serial.printf("RA8889_t41_p::FlexIO_Init() D0:%u and RD:%u are not on same flexio object", _data_pins[0], _wr_pin);
        }
    }

    // Looks like we may not use RD pin this way...
    _flexio_RD = pFlex->mapIOPinToFlexPin(_rd_pin);
    uint8_t working_bus_width = _bus_width;
    // lets dos some quick validation of the pins.
    uint8_t previous_flexio_pin = _flexio_D0;
    for (uint8_t i = 1; i < _bus_width; i++) {
        uint8_t flexio_pin = pFlex->mapIOPinToFlexPin(_data_pins[i]);
        if (flexio_pin != (previous_flexio_pin + 1)) {
            if ((i == 4) && (flexio_pin != 0xff)) {
                Serial.printf("\tNibble Mode: D4(%u): %u\n", _data_pins[4], flexio_pin);
                working_bus_width = 10;
            }
            Serial.printf("ILI948x_t4x_p::FlexIO_Ini - Flex IO Data pins pin issue D0(%u), D%u(%u)\n", _flexio_D0, i, flexio_pin);
        }
        previous_flexio_pin = flexio_pin;
    }


    /* Pointer to the port structure in the FlexIO channel */
    p = &pFlex->port();
    /* Pointer to the hardware structure in the FlexIO channel */
    hw = &pFlex->hardware();
    /* Basic pin setup */

    // First set all of the Data pins to OUTPUT
    for (uint8_t i = 0; i < _bus_width; i++) {
        pinMode(_data_pins[i], OUTPUT);
    }

    pinMode(_rd_pin, OUTPUT);
    digitalWriteFast(_rd_pin, HIGH);
    pinMode(_wr_pin, OUTPUT);
    digitalWriteFast(_wr_pin, HIGH);

    /* High speed and drive strength configuration */
    *(portControlRegister(_wr_pin)) = 0xFF;
    //    *(portControlRegister(_rd_pin)) = 0xFF;

    for (uint8_t i = 0; i < _bus_width; i++) {
        *(portControlRegister(_data_pins[i])) = 0xFF;
    }

    /* Set clock */
    pFlex->setClockSettings(3, 1, 0); // (480 MHz source, 1+1, 1+0) >> 480/2/1 >> 240Mhz

    // If we found we were T4 nibble mode update the _bus_width
    _bus_width = working_bus_width;

    /* Set up pin mux */
    pFlex->setIOPinToFlexMode(_wr_pin);
    //    pFlex->setIOPinToFlexMode(RD_PIN);

    for (uint8_t i = 0; i < _bus_width; i++) {
        pFlex->setIOPinToFlexMode(_data_pins[i]);
    }

    hw->clock_gate_register |= hw->clock_gate_mask;
    /* Enable the FlexIO with fast access */
    p->CTRL = FLEXIO_CTRL_FLEXEN;

    gpioWrite();

}

void print_flexio_debug_data(FlexIOHandler *pFlex, const char *szCaller) {
#if defined(DEBUG_FLEXIO)
    IMXRT_FLEXIO_t *p = &pFlex->port();
    Serial.println("\n**********************************");
    Serial.printf("FlexIO(%p) Index: %u - %s\n", pFlex, pFlex->FlexIOIndex(), szCaller);
    Serial.printf("CCM_CDCDR: %x\n", CCM_CDCDR);
    Serial.printf("CCM FlexIO1: %x FlexIO2: %x FlexIO3: %x\n", CCM_CCGR5 & CCM_CCGR5_FLEXIO1(CCM_CCGR_ON),
                  CCM_CCGR3 & CCM_CCGR3_FLEXIO2(CCM_CCGR_ON), CCM_CCGR7 & CCM_CCGR7_FLEXIO3(CCM_CCGR_ON));
    Serial.printf("VERID:%x PARAM:%x CTRL:%x PIN: %x\n", p->VERID, p->PARAM, p->CTRL, p->PIN);
    Serial.printf("SHIFTSTAT:%x SHIFTERR=%x TIMSTAT=%x\n", p->SHIFTSTAT, p->SHIFTERR, p->TIMSTAT);
    Serial.printf("SHIFTSIEN:%x SHIFTEIEN=%x TIMIEN=%x\n", p->SHIFTSIEN, p->SHIFTEIEN, p->TIMIEN);
    Serial.printf("SHIFTSDEN:%x SHIFTSTATE=%x\n", p->SHIFTSDEN, p->SHIFTSTATE);
    Serial.print("SHIFTCTL:");
    for (int i = 0; i < 8; i++) {
        Serial.printf(" %08x", p->SHIFTCTL[i]);
    }
    Serial.print("\nSHIFTCFG:");
    for (int i = 0; i < 8; i++) {
        Serial.printf(" %08x", p->SHIFTCFG[i]);
    }

    Serial.printf("\nTIMCTL:%x %x %x %x\n", p->TIMCTL[0], p->TIMCTL[1], p->TIMCTL[2], p->TIMCTL[3]);
    Serial.printf("TIMCFG:%x %x %x %x\n", p->TIMCFG[0], p->TIMCFG[1], p->TIMCFG[2], p->TIMCFG[3]);
    Serial.printf("TIMCMP:%x %x %x %x\n", p->TIMCMP[0], p->TIMCMP[1], p->TIMCMP[2], p->TIMCMP[3]);
#endif    
}



FASTRUN void RA8889_t41_p::FlexIO_Config_SnglBeat_Read() {

    if (flex_config == CONFIG_SNGLREAD)
        return;
    flex_config = CONFIG_SNGLREAD;
    DBGPrintf("RA8889_t41_p::FlexIO_Config_SnglBeat_Read - Enter\n");

    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    //p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    // Clear out Write mode 
    p->SHIFTCFG[0] = 0;
    p->SHIFTCTL[0] = 0;
    p->SHIFTSTAT = (1 << 0);
    p->SHIFTERR = (1 << 0);

    gpioRead(); // write line high, pin 12(rst) as output

    p->SHIFTCFG[3] =
    /* Configure the shifters */
        FLEXIO_SHIFTCFG_SSTOP(0)                 /* Shifter stop bit disabled */
        | FLEXIO_SHIFTCFG_SSTART(0)              /* Shifter start bit disabled and loading data on enabled */
        | FLEXIO_SHIFTCFG_PWIDTH(_bus_width - 1); /* Bus width */

    p->SHIFTCTL[3] =
        FLEXIO_SHIFTCTL_TIMSEL(0)      /* Shifter's assigned timer index */
        | FLEXIO_SHIFTCTL_TIMPOL * (1) /* Shift on negative edge of shift clock */
        | FLEXIO_SHIFTCTL_PINCFG(1)    /* Shifter's pin configured as output */
        | FLEXIO_SHIFTCTL_PINSEL(_flexio_D0)    /* Shifter's pin start index */
        | FLEXIO_SHIFTCTL_PINPOL * (0) /* Shifter's pin active high */
        | FLEXIO_SHIFTCTL_SMOD(1);     /* Shifter mode as receive */

    /* Configure the timer for shift clock */
#ifndef RA8889_CLOCK_READ
#define RA8889_CLOCK_READ 60
#endif    

    p->TIMCMP[0] =
        (((1 * 2) - 1) << 8) /* TIMCMP[15:8] = number of beats x 2 – 1 */
        | ((RA8889_CLOCK_READ / 2) - 1);    /* TIMCMP[7:0] = baud rate divider / 2 – 1 */
    p->TIMCFG[0] =
        FLEXIO_TIMCFG_TIMOUT(0)       /* Timer output logic one when enabled and not affected by reset */
        | FLEXIO_TIMCFG_TIMDEC(0)     /* Timer decrement on FlexIO clock, shift clock equals timer output */
        | FLEXIO_TIMCFG_TIMRST(0)     /* Timer never reset */
        | FLEXIO_TIMCFG_TIMDIS(2)     /* Timer disabled on timer compare */
        | FLEXIO_TIMCFG_TIMENA(2)     /* Timer enabled on trigger high */
        | FLEXIO_TIMCFG_TSTOP(1)      /* Timer stop bit enabled */
        | FLEXIO_TIMCFG_TSTART * (0); /* Timer start bit disabled */

    p->TIMCTL[0] =
        FLEXIO_TIMCTL_TRGSEL((((3) << 2) | 1)) /* Timer trigger selected as shifter's status flag */
        | FLEXIO_TIMCTL_TRGPOL * (1)           /* Timer trigger polarity as active low */
        | FLEXIO_TIMCTL_TRGSRC * (1)           // 1                                              /* Timer trigger source as internal */
        | FLEXIO_TIMCTL_PINCFG(3)              /* Timer' pin configured as output */
        | FLEXIO_TIMCTL_PINSEL(_flexio_RD)             /* Timer' pin index: RD pin */
        | FLEXIO_TIMCTL_PINPOL * (1)           /* Timer' pin active low */
        | FLEXIO_TIMCTL_TIMOD(1);              /* Timer mode as dual 8-bit counters baud/bit */

    // Clear the shifter status 
    p->SHIFTSTAT = 1 << 3;
    p->SHIFTERR = 1 << 3;

#if defined(DEBUG_FLEXIO)
    static uint8_t DEBUG_COUNT = 2;
    if (DEBUG_COUNT) {
        DEBUG_COUNT--;
        print_flexio_debug_data(pFlex, "FlexIO_Config_SnglBeat_Read");
    }
#endif
    /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;
    DBGPrintf("RA8889_t41_p::FlexIO_Config_SnglBeat_Read - Exit\n");
}

FASTRUN void RA8889_t41_p::FlexIO_Config_SnglBeat() {

    if (flex_config == CONFIG_SNGLBEAT)
        return;
    flex_config = CONFIG_SNGLBEAT;
    DBGPrintf("RA8889_t41_p::FlexIO_Config_SnglBeat - Enter\n");

    // Clear out Read mode 
    p->SHIFTCFG[3] = 0;
    p->SHIFTCTL[3] = 0;
    p->SHIFTSTAT = (1 << 3);
    p->SHIFTERR = (1 << 3);

    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    //p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    pFlex->setIOPinToFlexMode(_wr_pin);
    
    gpioWrite();

    /* Configure the shifters */
    p->SHIFTCFG[0] =
        FLEXIO_SHIFTCFG_INSRC * (1)              /* Shifter input */
        | FLEXIO_SHIFTCFG_SSTOP(0)               /* Shifter stop bit disabled */
        | FLEXIO_SHIFTCFG_SSTART(0)              /* Shifter start bit disabled and loading data on enabled */
        | FLEXIO_SHIFTCFG_PWIDTH(_bus_width - 1); // Was 7 for 8 bit bus         /* Bus width */

    p->SHIFTCTL[0] =
        FLEXIO_SHIFTCTL_TIMSEL(0)      /* Shifter's assigned timer index */
        | FLEXIO_SHIFTCTL_TIMPOL * (0) /* Shift on posedge of shift clock */
        | FLEXIO_SHIFTCTL_PINCFG(3)    /* Shifter's pin configured as output */
        | FLEXIO_SHIFTCTL_PINSEL(_flexio_D0)    /* Shifter's pin start index */
        | FLEXIO_SHIFTCTL_PINPOL * (0) /* Shifter's pin active high */
        | FLEXIO_SHIFTCTL_SMOD(2);     /* Shifter mode as transmit */

    /* Configure the timer for shift clock */
    p->TIMCMP[0] =
        (((1 * 2) - 1) << 8)     /* TIMCMP[15:8] = number of beats x 2 – 1 */
        | ((_baud_div / 2) - 1); /* TIMCMP[7:0] = baud rate divider / 2 – 1 */

    p->TIMCFG[0] =
        FLEXIO_TIMCFG_TIMOUT(0)       /* Timer output logic one when enabled and not affected by reset */
        | FLEXIO_TIMCFG_TIMDEC(0)     /* Timer decrement on FlexIO clock, shift clock equals timer output */
        | FLEXIO_TIMCFG_TIMRST(0)     /* Timer never reset */
        | FLEXIO_TIMCFG_TIMDIS(2)     /* Timer disabled on timer compare */
        | FLEXIO_TIMCFG_TIMENA(2)     /* Timer enabled on trigger high */
        | FLEXIO_TIMCFG_TSTOP(0)      /* Timer stop bit disabled */
        | FLEXIO_TIMCFG_TSTART * (0); /* Timer start bit disabled */

    p->TIMCTL[0] =
        FLEXIO_TIMCTL_TRGSEL((((0) << 2) | 1)) /* Timer trigger selected as shifter's status flag */
        | FLEXIO_TIMCTL_TRGPOL * (1)           /* Timer trigger polarity as active low */
        | FLEXIO_TIMCTL_TRGSRC * (1)           /* Timer trigger source as internal */
        | FLEXIO_TIMCTL_PINCFG(3)              /* Timer' pin configured as output */
        | FLEXIO_TIMCTL_PINSEL(_flexio_WR)             /* Timer' pin index: WR pin */
        | FLEXIO_TIMCTL_PINPOL * (1)           /* Timer' pin active low */
        | FLEXIO_TIMCTL_TIMOD(1);              /* Timer mode as dual 8-bit counters baud/bit */

#if defined(DEBUG_FLEXIO)
    static uint8_t DEBUG_COUNT = 2;
    if (DEBUG_COUNT) {
        DEBUG_COUNT--;
        print_flexio_debug_data(pFlex, "FlexIO_Config_SnglBeat" );
    }
#endif
    /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;
    DBGPrintf("RA8889_t41_p::FlexIO_Config_SnglBeat - Exit\n");
}

FASTRUN void RA8889_t41_p::FlexIO_Clear_Config_SnglBeat() {
    if (flex_config == CONFIG_CLEAR)
        return;
    flex_config = CONFIG_CLEAR;
    DBGPrintf("RA8889_t41_p::FlexIO_Clear_Config_SnglBeat() - Enter\n");

    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    p->SHIFTCFG[0] = 0;
    p->SHIFTCTL[0] = 0;
    p->SHIFTSTAT = (1 << 0);
    p->TIMCMP[0] = 0;
    p->TIMCFG[0] = 0;
    p->TIMSTAT = (1U << 0); /* Timer start bit disabled */
    p->TIMCTL[0] = 0;


    /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;
    DBGPrintf("RA8889_t41_p::FlexIO_Clear_Config_SnglBeat() - Exit\n");
}

FASTRUN void RA8889_t41_p::FlexIO_Config_MultiBeat() {
    if (flex_config == CONFIG_MULTIBEAT)
        return;
    flex_config = CONFIG_MULTIBEAT;
    DBGPrintf("RA8889_t41_p::FlexIO_Config_MultiBeat() - Enter\n");
	// Number of beats = number of shifters * beats per shifter * 8.
	// 32 beats in 8-bit mode and 16 beats in 16-bit mode.
    uint8_t MulBeatWR_BeatQty = SHIFTNUM * sizeof(uint32_t) / _bus_width * 8;

    /* Disable and reset FlexIO */
    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    //p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    pFlex->setIOPinToFlexMode(_wr_pin);
    gpioWrite();

    /* Configure the shifters */
    for (int i = 0; i <= SHIFTNUM - 1; i++) {
        p->SHIFTCFG[i] =
            FLEXIO_SHIFTCFG_INSRC * (1U)             /* Shifter input from next shifter's output */
            | FLEXIO_SHIFTCFG_SSTOP(0U)              /* Shifter stop bit disabled */
            | FLEXIO_SHIFTCFG_SSTART(0U)             /* Shifter start bit disabled and loading data on enabled */
            | FLEXIO_SHIFTCFG_PWIDTH(_bus_width - 1); /* 8 bit shift width */
    }

    p->SHIFTCTL[0] =
        FLEXIO_SHIFTCTL_TIMSEL(0)       /* Shifter's assigned timer index */
        | FLEXIO_SHIFTCTL_TIMPOL * (0U) /* Shift on posedge of shift clock */
        | FLEXIO_SHIFTCTL_PINCFG(3U)    /* Shifter's pin configured as output */
        | FLEXIO_SHIFTCTL_PINSEL(_flexio_D0)     /* Shifter's pin start index */
        | FLEXIO_SHIFTCTL_PINPOL * (0U) /* Shifter's pin active high */
        | FLEXIO_SHIFTCTL_SMOD(2U);     /* shifter mode transmit */

    for (int i = 1; i <= SHIFTNUM - 1; i++) {
        p->SHIFTCTL[i] =
            FLEXIO_SHIFTCTL_TIMSEL(0)       /* Shifter's assigned timer index */
            | FLEXIO_SHIFTCTL_TIMPOL * (0U) /* Shift on posedge of shift clock */
            | FLEXIO_SHIFTCTL_PINCFG(0U)    /* Shifter's pin configured as output disabled */
            | FLEXIO_SHIFTCTL_PINSEL(_flexio_D0)     /* Shifter's pin start index */
            | FLEXIO_SHIFTCTL_PINPOL * (0U) /* Shifter's pin active high */
            | FLEXIO_SHIFTCTL_SMOD(2U);
        p->SHIFTERR = 1 << (i); // clear out any previous state
    }
    /* Configure the timer for shift clock */
    p->TIMCMP[0] =
        ((MulBeatWR_BeatQty * 2U - 1U) << 8)  /* TIMCMP[15:8] = number of beats x 2 – 1 */
        | (_baud_div / 2U - 1U); /* TIMCMP[7:0] = shift clock divide ratio / 2 - 1 */

    p->TIMCFG[0] = FLEXIO_TIMCFG_TIMOUT(0U)       /* Timer output logic one when enabled and not affected by reset */
                   | FLEXIO_TIMCFG_TIMDEC(0U)     /* Timer decrement on FlexIO clock, shift clock equals timer output */
                   | FLEXIO_TIMCFG_TIMRST(0U)     /* Timer never reset */
                   | FLEXIO_TIMCFG_TIMDIS(2U)     /* Timer disabled on timer compare */
                   | FLEXIO_TIMCFG_TIMENA(2U)     /* Timer enabled on trigger high */
                   | FLEXIO_TIMCFG_TSTOP(0U)      /* Timer stop bit disabled */
                   | FLEXIO_TIMCFG_TSTART * (0U); /* Timer start bit disabled */

    p->TIMCTL[0] =
        FLEXIO_TIMCTL_TRGSEL(((SHIFTNUM - 1) << 2) | 1U) /* Timer trigger selected as highest shifter's status flag */
        | FLEXIO_TIMCTL_TRGPOL * (1U)                    /* Timer trigger polarity as active low */
        | FLEXIO_TIMCTL_TRGSRC * (1U)                    /* Timer trigger source as internal */
        | FLEXIO_TIMCTL_PINCFG(3U)                       /* Timer' pin configured as output */
        | FLEXIO_TIMCTL_PINSEL(_flexio_WR)                       /* Timer' pin index: WR pin */
        | FLEXIO_TIMCTL_PINPOL * (1U)                    /* Timer' pin active low */
        | FLEXIO_TIMCTL_TIMOD(1U);                       /* Timer mode 8-bit baud counter */
                                                         /* Enable FlexIO */
    p->CTRL |= FLEXIO_CTRL_FLEXEN;

    // configure interrupts
    attachInterruptVector(hw->flex_irq, ISR);
    NVIC_ENABLE_IRQ(hw->flex_irq);
    NVIC_SET_PRIORITY(hw->flex_irq, FLEXIO_ISR_PRIORITY);

    // disable interrupts until later
    p->SHIFTSIEN &= ~(1 << SHIFTER_IRQ);
    p->TIMIEN &= ~(1 << TIMER_IRQ);
    DBGPrintf("RA8889_t41_p::FlexIO_Config_MultiBeat() - Exit\n");
}

//=============================================================================
// FlexIO IRQ - mainly for FlexIO3 which does not have DMA
FASTRUN void RA8889_t41_p::flexIRQ_Callback() {
    if (p->TIMSTAT & (1 << TIMER_IRQ)) { // interrupt from end of burst
        p->TIMSTAT = (1 << TIMER_IRQ);   // clear timer interrupt signal
        _irq_bursts_to_complete--;
        if ((_irq_bursts_to_complete == 0) || (_irq_bytes_remaining == 0)) {
            p->TIMIEN &= ~(1 << TIMER_IRQ); // disable timer interrupt
            asm("dsb");
            WR_IRQTransferDone = true;
            CSHigh();
            _onCompleteCB();
            return;
        }
    }

    if (p->SHIFTSTAT & (1 << SHIFTER_IRQ)) { // interrupt from empty shifter buffer
        // note, the interrupt signal is cleared automatically when writing data to the shifter buffers
        if (_irq_bytes_remaining == 0) {                     // just started final burst, no data to load
            p->SHIFTSIEN &= ~(1 << SHIFTER_IRQ);        // disable shifter interrupt signal
        } else if (_irq_bytes_remaining < _irq_bytes_per_burst) { // just started second-to-last burst, load data for final burst
            p->TIMCMP[0] = ((_irq_bytes_remaining * 2U - 1) << 8) | (_baud_div / 2U - 1); // takes effect on final burst
            _irq_readPtr = finalBurstBuffer;
            _irq_bytes_remaining = 0;
            for (int i = 0; i < SHIFTNUM; i++) {
                uint32_t data = *_irq_readPtr++;
                p->SHIFTBUFHWS[i] = ((data >> 16) & 0xFFFF) | ((data << 16) & 0xFFFF0000);
                while (0 == (p->SHIFTSTAT & (1U << SHIFTER_IRQ))) {
                }
            }
        } else {
			_irq_bytes_remaining -= _irq_bytes_per_burst;
            for (int i = 0; i < SHIFTNUM; i++) {
                uint32_t data = *_irq_readPtr++;
                p->SHIFTBUFHWS[i] = ((data >> 16) & 0xFFFF) | ((data << 16) & 0xFFFF0000);
                while (0 == (p->SHIFTSTAT & (1U << SHIFTER_IRQ))) {
                }
            }
        }
		p->TIMSTAT |= (1U << 0);
		/*Wait for transfer to be completed */
		while(0 == (p->TIMSTAT |= (1U << 0))) {}
    }
    asm("dsb");
}

FASTRUN void RA8889_t41_p::ISR() {
    asm("dsb");
    IRQcallback->flexIRQ_Callback();
}

RA8889_t41_p *RA8889_t41_p::IRQcallback = nullptr;

FASTRUN void RA8889_t41_p::MulBeatWR_nPrm_IRQ(const void *value, uint32_t const length) {

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

    FlexIO_Config_MultiBeat();
    WR_IRQTransferDone = false;

    uint32_t bytes = length * 2U;
    _irq_bytes_per_shifter = (_bus_width <= 8) ? 4 : 2;
	// Need to multiply _irq_bytes_per_burst by 2 in 16-bit mode to get 32 bytes per burst.
    _irq_bytes_per_burst = (_bus_width <= 8) ? _irq_bytes_per_shifter * SHIFTNUM :
											_irq_bytes_per_shifter * SHIFTNUM * 2;
    CSLow();
    DCHigh();

    _irq_bursts_to_complete = bytes / _irq_bytes_per_burst;

    int remainder = bytes % _irq_bytes_per_burst;
    if (remainder != 0) {
        memset(finalBurstBuffer, 0, sizeof(finalBurstBuffer));
        memcpy(finalBurstBuffer, (uint8_t *)value + bytes - remainder, remainder);
        _irq_bursts_to_complete++;
    }

    _irq_bytes_remaining = bytes; // +32; // Need to add 32 to get remaining burst?
    _irq_readPtr = (uint32_t *)value;
        //Serial.printf ("arg addr: %x, _irq_readPtr addr: %x, contents: %x\n", value, _irq_readPtr, *_irq_readPtr);
        //Serial.printf("START::_irq_bursts_to_complete: %d _irq_bytes_remaining: %d \n", _irq_bursts_to_complete, _irq_bytes_remaining);

    uint8_t beats = SHIFTNUM * _irq_bytes_per_shifter;
    p->TIMCMP[0] = ((beats * 2U - 1) << 8) | (_baud_div / 2U - 1U);
    p->TIMSTAT = (1 << TIMER_IRQ); // clear timer interrupt signal

    asm("dsb");

    IRQcallback = this;
    // enable interrupts to trigger bursts
    p->TIMIEN |= (1 << TIMER_IRQ);
    p->SHIFTSIEN |= (1 << SHIFTER_IRQ);
}

FASTRUN void RA8889_t41_p::_onDMACompleteCB() {
    if (_DMAcallback) {
        _DMAcallback();
    }
    return;
}


FASTRUN void RA8889_t41_p::pushPixels16bitAsync(const uint16_t *pcolors, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any DMA transfers to complete
    uint32_t area = (x2) * (y2);
    graphicMode(true);
    activeWindowXY(x1, y1);
    activeWindowWH(x2, y2);
    setPixelCursor(x1, y1);
    ramAccessPrepare();
    MulBeatWR_nPrm_IRQ(pcolors, area);
}

//----------------------------------------------------------------------
// 8/16 BIT DMA STUFF starts here
//----------------------------------------------------------------------
FASTRUN void RA8889_t41_p::FlexIO_Config_DMA_MultiBeat()
{
    if (flex_config == CONFIG_DMA_MULTIBEAT)
        return;
    flex_config = CONFIG_DMA_MULTIBEAT;
    DBGPrintf("RA8889_t41_p::FlexIO_Config_DMA_MultiBeat() - Enter\n");

    uint32_t i;
    uint8_t MulBeatWR_BeatQty = SHIFTNUM * sizeof(uint32_t) / _bus_width * 8;   //Number of beats = number of shifters * beats per shifter

    /* Disable and reset FlexIO */
    p->CTRL &= ~FLEXIO_CTRL_FLEXEN;
    //p->CTRL |= FLEXIO_CTRL_SWRST;
    p->CTRL &= ~FLEXIO_CTRL_SWRST;

    pFlex->setIOPinToFlexMode(_wr_pin);
    gpioWrite();

    for(i=0; i<=SHIFTNUM-1; i++)
    {
        p->SHIFTCFG[i] = 
        FLEXIO_SHIFTCFG_INSRC*(1U)                                                /* Shifter input from next shifter's output */
      | FLEXIO_SHIFTCFG_SSTOP(0U)                                                 /* Shifter stop bit disabled */
      | FLEXIO_SHIFTCFG_SSTART(0U)                                                /* Shifter start bit disabled and loading data on enabled */
      | FLEXIO_SHIFTCFG_PWIDTH(_bus_width - 1);                                    /* 8 bit shift width */
    }

    p->SHIFTCTL[0] = 
    FLEXIO_SHIFTCTL_TIMSEL(0)                                                     /* Shifter's assigned timer index */
      | FLEXIO_SHIFTCTL_TIMPOL*(0U)                                               /* Shift on posedge of shift clock */
      | FLEXIO_SHIFTCTL_PINCFG(3U)                                                /* Shifter's pin configured as output */
      | FLEXIO_SHIFTCTL_PINSEL(_flexio_D0)  //4                                               /* Shifter's pin start index */
      | FLEXIO_SHIFTCTL_PINPOL*(0U)                                               /* Shifter's pin active high */
      | FLEXIO_SHIFTCTL_SMOD(2U);                                                 /* shifter mode transmit */

    for(i=1; i<=SHIFTNUM-1; i++)
    {
        p->SHIFTCTL[i] = 
        FLEXIO_SHIFTCTL_TIMSEL(0)                                                 /* Shifter's assigned timer index */
      | FLEXIO_SHIFTCTL_TIMPOL*(0U)                                               /* Shift on posedge of shift clock */
      | FLEXIO_SHIFTCTL_PINCFG(0U)                                                /* Shifter's pin configured as output disabled */
      | FLEXIO_SHIFTCTL_PINSEL(_flexio_D0)                                                 /* Shifter's pin start index */
      | FLEXIO_SHIFTCTL_PINPOL*(0U)                                               /* Shifter's pin active high */
      | FLEXIO_SHIFTCTL_SMOD(2U);                                                 /* shifter mode transmit */          
    }

    /* Configure the timer for shift clock */
    p->TIMCMP[0] = ((MulBeatWR_BeatQty * 2U - 1U) << 8)                            /* TIMCMP[15:8] = number of beats x 2 – 1 */
      | (_baud_div / 2U - 1U);                                                      /* TIMCMP[7:0] = shift clock divide ratio / 2 - 1 */
      
    p->TIMCFG[0] =   FLEXIO_TIMCFG_TIMOUT(0U)                                     /* Timer output logic one when enabled and not affected by reset */
      | FLEXIO_TIMCFG_TIMDEC(0U)                                                  /* Timer decrement on FlexIO clock, shift clock equals timer output */
      | FLEXIO_TIMCFG_TIMRST(0U)                                                  /* Timer never reset */
      | FLEXIO_TIMCFG_TIMDIS(2U)                                                  /* Timer disabled on timer compare */
      | FLEXIO_TIMCFG_TIMENA(2U)                                                  /* Timer enabled on trigger high */
      | FLEXIO_TIMCFG_TSTOP(0U)                                                   /* Timer stop bit disabled */
      | FLEXIO_TIMCFG_TSTART*(0U);                                                /* Timer start bit disabled */

    p->TIMCTL[0] = 
        FLEXIO_TIMCTL_TRGSEL((0 << 2) | 1U)                                       /* Timer trigger selected as highest shifter's status flag */
      | FLEXIO_TIMCTL_TRGPOL*(1U)                                                 /* Timer trigger polarity as active low */
      | FLEXIO_TIMCTL_TRGSRC*(1U)                                                 /* Timer trigger source as internal */
      | FLEXIO_TIMCTL_PINCFG(3U)                                                  /* Timer' pin configured as output */
      | FLEXIO_TIMCTL_PINSEL(_flexio_WR)                                                   /* Timer' pin index: WR pin */
      | FLEXIO_TIMCTL_PINPOL*(1U)                                                 /* Timer' pin active low */
      | FLEXIO_TIMCTL_TIMOD(1U);                                                  /* Timer mode 8-bit baud counter */

    /* Enable FlexIO */
   p->CTRL |= FLEXIO_CTRL_FLEXEN;
   p->SHIFTSDEN |= 1U << (SHIFTER_DMA_REQUEST); // enable DMA trigger when shifter status flag is set on shifter SHIFTER_DMA_REQUEST
    DBGPrintf("RA8889_t41_p::FlexIO_Config_MultiBeat() - Exit\n");
}

RA8889_t41_p * RA8889_t41_p::dmaCallback = nullptr;
DMAChannel RA8889_t41_p::flexDma;
DMASetting RA8889_t41_p::_dmaSettings[2];

#ifdef DEBUG
static void dumpDMA_TCD(DMABaseClass *dmabc, const char *psz_title) {
    if (psz_title)
        Serial.print(psz_title);
    Serial.printf("%x %x: ", (uint32_t)dmabc, (uint32_t)dmabc->TCD);

    Serial.printf(
        "SA:%x SO:%d AT:%x (SM:%x SS:%x DM:%x DS:%x) NB:%x SL:%d DA:%x "
        "DO: %d CI:%x DL:%x CS:%x BI:%x\n",
        (uint32_t)dmabc->TCD->SADDR, dmabc->TCD->SOFF, dmabc->TCD->ATTR,
        (dmabc->TCD->ATTR >> 11) & 0x1f, (dmabc->TCD->ATTR >> 8) & 0x7,
        (dmabc->TCD->ATTR >> 3) & 0x1f, (dmabc->TCD->ATTR >> 0) & 0x7,
        dmabc->TCD->NBYTES, dmabc->TCD->SLAST, (uint32_t)dmabc->TCD->DADDR,
        dmabc->TCD->DOFF, dmabc->TCD->CITER, dmabc->TCD->DLASTSGA,
        dmabc->TCD->CSR, dmabc->TCD->BITER);
}
#endif


FASTRUN void RA8889_t41_p::MulBeatWR_nPrm_DMA(const void *value, uint32_t const length) 
{

  while(WR_DMATransferDone == false) {}  //Wait for any DMA transfers to complete

  uint8_t BeatsPerMinLoop = SHIFTNUM * sizeof(uint32_t) / _bus_width /** 8 */; // Number of shifters * number of 8 bit values per shifter

  uint32_t majorLoopCount, minorLoopBytes;
  uint32_t destinationModulo = 31-(__builtin_clz(SHIFTNUM*sizeof(uint32_t))); // defines address range for circular DMA destination buffer 

  CSLow();
  DCHigh();

  if (length < 8){
    const uint16_t * newValue = (uint16_t*)value;
    uint16_t buf;
    for(uint32_t i=0; i<length; i++) {
        buf = *newValue++;
          while(0 == (p->SHIFTSTAT & (1U << 0))) {}
          p->SHIFTBUF[0] = generate_output_word(buf >> 8);
          while(0 == (p->SHIFTSTAT & (1U << 0))) {}
          p->SHIFTBUF[0] = generate_output_word(buf & 0xFF);
    }        
    //Wait for transfer to be completed 
    while(0 == (p->TIMSTAT & (1U << 0))) {}
    CSHigh();

  } else {
    
  FlexIO_Config_DMA_MultiBeat();
    
  MulBeatCountRemain = length % BeatsPerMinLoop;
  MulBeatDataRemain = (uint16_t*)value + ((length - MulBeatCountRemain)); // pointer to the next unused byte (overflow if MulBeatCountRemain = 0)
  TotalSize = (length - MulBeatCountRemain)*2;               /* in bytes */
  minorLoopBytes = SHIFTNUM * sizeof(uint32_t);
  majorLoopCount = TotalSize/minorLoopBytes;
  /* Configure FlexIO with multi-beat write configuration */
  flexDma.begin();

  int destinationAddressOffset, destinationAddressLastOffset, sourceAddressOffset, sourceAddressLastOffset, minorLoopOffset;
  volatile void *destinationAddress, *sourceAddress;

  DMA_CR |= DMA_CR_EMLM; // enable minor loop mapping
  arm_dcache_flush_delete((void *)value, length * 2);  // important to flush cache before DMA. Otherwise, DMA will read from cache instead of memory and screen shows "snow" effects.

  /* From now on, the SHIFTERS in MultiBeat mode are working correctly. Begin DMA transfer */
  sourceAddress = (uint16_t*)value + minorLoopBytes/sizeof(uint16_t) - 1; // last 16bit address within current minor loop
  sourceAddressOffset = -sizeof(uint16_t); // read values in reverse order
  minorLoopOffset = 2*minorLoopBytes; // source address offset at end of minor loop to advance to next minor loop
  sourceAddressLastOffset = minorLoopOffset - TotalSize; // source address offset at completion to reset to beginning
  destinationAddress = (void *)&p->SHIFTBUFHWS[SHIFTNUM - 1]; // last 32bit shifter address (with reverse byte order)
  destinationAddressOffset = -sizeof(uint32_t); // write words in reverse order
  destinationAddressLastOffset = 0;

  if (majorLoopCount <= 32767) {
     
      flexDma.TCD->SADDR = sourceAddress;
      flexDma.TCD->SOFF = sourceAddressOffset;
      flexDma.TCD->SLAST = sourceAddressLastOffset;
      flexDma.TCD->DADDR = destinationAddress;
      flexDma.TCD->DOFF = destinationAddressOffset;
      flexDma.TCD->DLASTSGA = destinationAddressLastOffset;
      flexDma.TCD->ATTR =
        DMA_TCD_ATTR_SMOD(0U)
        | DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_16BIT) // 16bit reads
        | DMA_TCD_ATTR_DMOD(destinationModulo)
        | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT); // 32bit writes
      flexDma.TCD->NBYTES_MLOFFYES = 
        DMA_TCD_NBYTES_SMLOE
        | DMA_TCD_NBYTES_MLOFFYES_MLOFF(minorLoopOffset)
        | DMA_TCD_NBYTES_MLOFFYES_NBYTES(minorLoopBytes);
        flexDma.TCD->CITER = majorLoopCount; // Current major iteration count
        flexDma.TCD->BITER = majorLoopCount; // Starting major iteration count
        flexDma.triggerAtHardwareEvent(hw->shifters_dma_channel[SHIFTER_DMA_REQUEST]);
        flexDma.disableOnCompletion();
        flexDma.interruptAtCompletion();
        flexDma.clearComplete();
        #ifdef DEBUG
        dumpDMA_TCD(&flexDma, "\n*** MulBeatWR_nPrm_DMA ***\n");
        #endif
    } else {
        // Too big to fit into one...  
        uint32_t half_major_loop_count = majorLoopCount / 2;      
        _dmaSettings[0].TCD->SADDR = sourceAddress;
        _dmaSettings[0].TCD->SOFF = sourceAddressOffset;
        _dmaSettings[0].TCD->SLAST = sourceAddressLastOffset;
        _dmaSettings[0].TCD->DADDR = destinationAddress;
        _dmaSettings[0].TCD->DOFF = destinationAddressOffset;
        _dmaSettings[0].TCD->DLASTSGA = destinationAddressLastOffset;
        _dmaSettings[0].TCD->ATTR =
            DMA_TCD_ATTR_SMOD(0U)
            | DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_16BIT) // 16bit reads
            | DMA_TCD_ATTR_DMOD(destinationModulo)
            | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT); // 32bit writes
        _dmaSettings[0].TCD->NBYTES_MLOFFYES = 
            DMA_TCD_NBYTES_SMLOE
            | DMA_TCD_NBYTES_MLOFFYES_MLOFF(minorLoopOffset)
            | DMA_TCD_NBYTES_MLOFFYES_NBYTES(minorLoopBytes);
        _dmaSettings[0].TCD->CITER = half_major_loop_count; // Current major iteration count
        _dmaSettings[0].TCD->BITER = half_major_loop_count; // Starting major iteration count
        _dmaSettings[0].replaceSettingsOnCompletion(_dmaSettings[1]);

        _dmaSettings[1].TCD->SADDR = (uint8_t *)sourceAddress + (half_major_loop_count * minorLoopBytes);
        _dmaSettings[1].TCD->SOFF = sourceAddressOffset;
        _dmaSettings[1].TCD->SLAST = sourceAddressLastOffset;
        _dmaSettings[1].TCD->DADDR = destinationAddress;
        _dmaSettings[1].TCD->DOFF = destinationAddressOffset;
        _dmaSettings[1].TCD->DLASTSGA = destinationAddressLastOffset;
        _dmaSettings[1].TCD->ATTR =
            DMA_TCD_ATTR_SMOD(0U)
            | DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_16BIT) // 16bit reads
            | DMA_TCD_ATTR_DMOD(destinationModulo)
            | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT); // 32bit writes
        _dmaSettings[1].TCD->NBYTES_MLOFFYES = 
            DMA_TCD_NBYTES_SMLOE
            | DMA_TCD_NBYTES_MLOFFYES_MLOFF(minorLoopOffset)
            | DMA_TCD_NBYTES_MLOFFYES_NBYTES(minorLoopBytes);
        _dmaSettings[1].TCD->CITER = majorLoopCount - half_major_loop_count; // Current major iteration count
        _dmaSettings[1].TCD->BITER = majorLoopCount - half_major_loop_count; // Starting major iteration count
        _dmaSettings[1].disableOnCompletion();
        _dmaSettings[1].interruptAtCompletion();
        _dmaSettings[1].replaceSettingsOnCompletion(_dmaSettings[0]);

        flexDma = _dmaSettings[0];
        flexDma.triggerAtHardwareEvent(hw->shifters_dma_channel[SHIFTER_DMA_REQUEST]);
        flexDma.clearComplete();
        #ifdef DEBUG
        dumpDMA_TCD(&flexDma, "\n*** MulBeatWR_nPrm_DMA ***\n");
        dumpDMA_TCD(&_dmaSettings[0], "DmaSettings[0]\n");
        dumpDMA_TCD(&_dmaSettings[1], "DmaSettings[1]\n");
        #endif
}

    /* Start data transfer by using DMA */
    WR_DMATransferDone = false;
    flexDma.attachInterrupt(dmaISR);
    flexDma.enable();
    dmaCallback = this;

    DBGPrintf("Major Loop count: %u\n", majorLoopCount);
  }
}

FASTRUN void RA8889_t41_p::dmaISR() {
  flexDma.clearInterrupt();
  asm volatile ("dsb"); // prevent interrupt from re-entering
  dmaCallback->flexDma_Callback();
}

FASTRUN void RA8889_t41_p::flexDma_Callback() {
  /* the interrupt is called when the final DMA transfer completes writing to the shifter buffers, which would generally happen while
  data is still in the process of being shifted out from the second-to-last major iteration. In this state, all the status flags are cleared.
  when the second-to-last major iteration is fully shifted out, the final data is transfered from the buffers into the shifters which sets all the status flags.
  if you have only one major iteration, the status flags will be immediately set before the interrupt is called, so the while loop will be skipped. */
  while(0 == (p->SHIFTSTAT & (1U << (SHIFTNUM-1)))) {}
  
  /* Wait the last multi-beat transfer to be completed. Clear the timer flag
  before the completing of the last beat. The last beat may has been completed
  at this point, then code would be dead in the while() below. So mask the
  while() statement and use the software delay .*/
  p->TIMSTAT |= (1U << 0U);

  /* Wait timer flag to be set to ensure the completing of the last beat. */
  while(0 == (p->TIMSTAT & (1U << 0U))) {}

  if(MulBeatCountRemain) {
    uint16_t value;
    /* Configure FlexIO with 1-beat write configuration */
    FlexIO_Config_SnglBeat();

    /* Use polling method for data transfer */
    for(uint32_t i=0; i<(MulBeatCountRemain); i++) {
      value = *MulBeatDataRemain++;
      while(0 == (p->SHIFTSTAT & (1U << 0))) {}
      p->SHIFTBUF[0] = generate_output_word(value >> 8);
      while(0 == (p->SHIFTSTAT & (1U << 0))) {}
      p->SHIFTBUF[0] = generate_output_word(value & 0xFF);
    }
    p->TIMSTAT |= (1U << 0);
    /*Wait for transfer to be completed */
    while(0 == (p->TIMSTAT |= (1U << 0))) {}
  }

  CSHigh();

  /* the for loop is probably not sufficient to complete the transfer. Shifting out all 32 bytes takes (32 beats)/(6 MHz) = 5.333 microseconds which is over 3000 CPU cycles.
  If you really need to wait in this callback until all the data has been shifted out, the while loop is probably the correct solution and I don't think it risks an infinite loop.
  however, it seems like a waste of time to wait here, since the process otherwise completes in the background and the shifter buffers are ready to receive new data while the transfer completes.
  I think in most applications you could continue without waiting. You can start a new DMA transfer as soon as the first one completes (no need to wait for FlexIO to finish shifting). */
  WR_DMATransferDone = true;
  if(isDMACB) {
    _onDMACompleteCB();
  }
}

void RA8889_t41_p::DMAerror() {
  if(flexDma.error()) {
    Serial.print("DMA error: ");
    Serial.println(DMA_ES, HEX);
  } 
}

FASTRUN void RA8889_t41_p::pushPixels16bitDMA(const uint16_t * pcolors, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint32_t area = (x2)*(y2);
  while(WR_DMATransferDone == false) {}    //Wait for any DMA transfers to complete
  graphicMode(true);
  activeWindowXY(x1,y1);
  activeWindowWH(x2,y2);
  setPixelCursor(x1,y1);
  ramAccessPrepare();
  MulBeatWR_nPrm_DMA(pcolors, area);
}

FASTRUN void RA8889_t41_p::_onCompleteCB() {
  if(_callback) {
    _callback();
  }
  return;
}

FLASHMEM void RA8889_t41_p::onCompleteCB(CBF callback) {
    _callback = callback;
    isCB = true;
}

FLASHMEM void RA8889_t41_p::onDMACompleteCB(CBF callback) {
    _DMAcallback = callback;
    isDMACB = true;
}

//**********************************************************************
// This section defines the low level parallel 8080 access routines.
// It uses "_bus_width" (8 or 16) to decide which drivers to use.
//**********************************************************************
void RA8889_t41_p::LCD_CmdWrite(unsigned char cmd) {
    //  startSend();
    //  _pspi->transfer16(0x00);
    //  _pspi->transfer(cmd);
    //  endSend(true);
}

//****************************************************************
// Write to a RA8889 register (8 bit only) in 8/16 bbit buss mode.
//****************************************************************
void RA8889_t41_p::lcdRegWrite(ru8 reg, bool finalize) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCLow();
    /* Write command index */
    p->SHIFTBUF[0] = generate_output_word(reg);
    /*Wait for transfer to be completed */
    while (0 == (p->SHIFTSTAT & (1 << 0))) {
    }
    while (0 == (p->TIMSTAT & (1 << 0))) {
    }
    /* De-assert RS pin */
    DCHigh();
    CSHigh();
}

//*****************************************************************
// Write RA8889 Data 8 bit data reg or memory in 8/16 bit bus mode.
//*****************************************************************
void RA8889_t41_p::lcdDataWrite(ru8 data, bool finalize) {
    while (WR_IRQTransferDone == false) {
    }
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8889.

    FlexIO_Config_SnglBeat();

    CSLow();
    DCHigh();

    p->SHIFTBUF[0] = generate_output_word(data);
    /*Wait for transfer to be completed */
    while (0 == (p->SHIFTSTAT & (1 << 0))) {
    }
    while (0 == (p->TIMSTAT & (1 << 0))) {
    }
    CSHigh();
}

//**************************************************************//
// Read RA8889 parallel Data (8-bit read) 8/16 bit bus mode.
//**************************************************************//
ru8 RA8889_t41_p::lcdDataRead(bool finalize) {
    uint16_t dummy __attribute__((unused)) = 0;
    uint16_t data = 0;

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

//    FlexIO_Clear_Config_SnglBeat();
    FlexIO_Config_SnglBeat_Read();

    CSLow();  // Must to go low after config above.
    DCHigh(); // Set HIGH for data read

    RDLow(); // Set RD pin low manually

    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    dummy = read_shiftbuf_byte();
    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    if (_bus_width != 16)
        data = read_shiftbuf_byte();
    else
        data = (p->SHIFTBUFBYS[3] >> 8) & 0xff;

    RDHigh(); // Set RD pin high manually

    CSHigh();

    // Serial.printf("lcdDataread(): Dummy 0x%4.4x, data 0x%4.4x\n", dummy, data);

    // Set FlexIO back to Write mode
    FlexIO_Config_SnglBeat(); // Not sure if this is needed.
    return data;
}

ru16 RA8889_t41_p::lcdDataRead16(bool finalize) {
    uint16_t dummy __attribute__((unused)) = 0;
    uint16_t data = 0;

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat_Read();

    CSLow();  // Must to go low after config above.
    DCHigh(); // Set HIGH for data read
    RDLow(); // Set RD pin low manually

    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    dummy = p->SHIFTBUFBYS[3];
    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    data = p->SHIFTBUFBYS[3];

    RDHigh(); // Set RD pin high manually
    CSHigh();

    //Serial.printf("lcdDataread(): Dummy 0x%4.4x, data 0x%4.4x\n", dummy, data);

    // Set FlexIO back to Write mode
    FlexIO_Config_SnglBeat(); // Not sure if this is needed.
    return ((data >> 8) & 0x00FF) | ((data << 8) & 0xFF00);
}

//**************************************************************//
// Read RA8889 status register 8bit data R/W only. 8/16 bit bus.
// Special case for status register access:
// /CS = 0, /DC = 0, /RD = 0, /WR = 1.
//**************************************************************//
ru8 RA8889_t41_p::lcdStatusRead(bool finalize) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

    //FlexIO_Clear_Config_SnglBeat();
    FlexIO_Config_SnglBeat_Read();

    CSLow();
    DCLow();

    RDLow(); // Set RD pin low manually

    uint16_t data = 0;
    while (0 == (p->SHIFTSTAT & (1 << 3))) {
    }
    if (_bus_width != 16)
        data = read_shiftbuf_byte();
    else
        data = (p->SHIFTBUFBYS[3] >> 8) & 0xff;

    DCHigh();
    CSHigh();

    RDHigh(); // Set RD pin high manually

    // Set FlexIO back to Write mode
    FlexIO_Config_SnglBeat();

    return data;
}

//**************************************************************//
// Write Data to a RA8889 register
//**************************************************************//
void RA8889_t41_p::lcdRegDataWrite(ru8 reg, ru8 data, bool finalize) {
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8889.
//    Serial.printf("%x:%x\n", reg, data);
    lcdRegWrite(reg);
    lcdDataWrite(data);
}

//**************************************************************//
// Read a RA8889 register Data
//**************************************************************//
ru8 RA8889_t41_p::lcdRegDataRead(ru8 reg, bool finalize) {
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8889.
    lcdRegWrite(reg, finalize);
    return lcdDataRead();
}

//******************************************************************
// support 8080 bus interface to write 16bpp data after Regwrite 04h
//******************************************************************
void RA8889_t41_p::lcdDataWrite16bbp(ru16 data, bool finalize) {
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8889.
    if (_bus_width != 16) {
        lcdDataWrite(data & 0xff);
        lcdDataWrite(data >> 8);
    } else {
        lcdDataWrite16(data, false);
    }
}

//*********************************************************
// Write RA8889 data to display memory, 8/16 bit buss mode.
//*********************************************************
void RA8889_t41_p::lcdDataWrite16(uint16_t data, bool finalize) {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8889.

    FlexIO_Config_SnglBeat();

    CSLow();
    DCHigh();

    if (_bus_width == 16) {
        p->SHIFTBUF[0] = data;
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        while (0 == (p->TIMSTAT & (1 << 0))) {
        }
    } else {
        p->SHIFTBUF[0] = generate_output_word(data >> 8);
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = generate_output_word(data & 0xff);
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        while (0 == (p->TIMSTAT & (1 << 0))) {
        }
    }
    /* De-assert /CS pin */
    CSHigh();
    DCLow();
}

//**************************************************************//
/* Write 16bpp(RGB565) picture data for user operation          */
/* Not recommended for future use - use BTE instead             */
//**************************************************************//
void RA8889_t41_p::putPicture_16bpp(ru16 x, ru16 y, ru16 width, ru16 height) {
    graphicMode(true);
    activeWindowXY(x, y);
    activeWindowWH(width, height);
    setPixelCursor(x, y);
    ramAccessPrepare();
    // Now your program has to send the image data pixels
}

//*******************************************************************//
/* write 16bpp(RGB565) picture data in byte format from data pointer */
/* Not recommended for future use - use BTE instead                  */
//*******************************************************************//
void RA8889_t41_p::putPicture_16bppData8(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned char *data) {
    ru16 i, j;

    putPicture_16bpp(x, y, width, height);
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(10); // Initially setup for the dev board v4.0
            p->SHIFTBUF[0] = generate_output_word(*data++);
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
            p->SHIFTBUF[0] = generate_output_word(*data++);
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
    checkWriteFifoEmpty(); // if high speed mcu and without Xnwait check
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

//****************************************************************//
/* Write 16bpp(RGB565) picture data word format from data pointer */
/* Not recommended for future use - use BTE instead               */
//****************************************************************//
void RA8889_t41_p::putPicture_16bppData16(ru16 x, ru16 y, ru16 width, ru16 height, const unsigned short *data) {
    ru16 i, j;

    putPicture_16bpp(x, y, width, height);
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(25); // Initially setup for the dev board v4.0
            p->SHIFTBUF[0] = generate_output_word(*data++);
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
    checkWriteFifoEmpty(); // if high speed mcu and without Xnwait check
    activeWindowXY(0, 0);
    activeWindowWH(_width, _height);
}

//**************************************************************//
// Send data from the microcontroller to the RA8889
// Does a Raster OPeration to combine with an image already in memory
// For a simple overwrite operation, use ROP 12
//**************************************************************//
void RA8889_t41_p::bteMpuWriteWithROPData8(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                           ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned char *data) {
    bteMpuWriteWithROP(s1_addr, s1_image_width, s1_x, s1_y, des_addr, des_image_width, des_x, des_y, width, height, rop_code);
    // MulBeatWR_nPrm_DMA(data,(width)*(height));
    ru16 i, j;
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(10); // Initially setup for the T4.1 board
            if (_rotation & 1)
                delayNanoseconds(20);
            p->SHIFTBUF[0] = generate_output_word(*data++);
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
            p->SHIFTBUF[0] = generate_output_word(*data++);
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    // De-assert /CS pin
    CSHigh();
}

//**************************************************************//
// For 16-bit byte-reversed data.
// Note this is 4-5 milliseconds slower than the 8-bit version above
// as the bulk byte-reversing SPI transfer operation is not available
// on all Teensys.
//**************************************************************//
void RA8889_t41_p::bteMpuWriteWithROPData16(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                            ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code, const unsigned short *data) {
    ru16 i, j;
    DBGPrintf("bteMpuWriteWithROPData16(%u %u %u %u %u - %x)\n", des_x, des_y, width, height, rop_code, data);
    bteMpuWriteWithROP(s1_addr, s1_image_width, s1_x, s1_y, des_addr, des_image_width, des_x, des_y, width, height, rop_code);

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete

    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(10); // Initially setup for the T4.1 board
            if (_rotation & 1)
                delayNanoseconds(70);
            p->SHIFTBUF[0] = *data++;
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
}

//**************************************************************//
// write data after setting, using lcdDataWrite() or lcdDataWrite16bbp()
//**************************************************************//
void RA8889_t41_p::bteMpuWriteWithROP(ru32 s1_addr, ru16 s1_image_width, ru16 s1_x, ru16 s1_y, ru32 des_addr, ru16 des_image_width,
                                      ru16 des_x, ru16 des_y, ru16 width, ru16 height, ru8 rop_code) {
    check2dBusy();
    graphicMode(true);
    bte_Source1_MemoryStartAddr(s1_addr);
    bte_Source1_ImageWidth(s1_image_width);
    bte_Source1_WindowStartXY(s1_x, s1_y);
    bte_DestinationMemoryStartAddr(des_addr);
    bte_DestinationImageWidth(des_image_width);
    bte_DestinationWindowStartXY(des_x, des_y);
    bte_WindowSize(width, height);
    lcdRegDataWrite(RA8889_BTE_CTRL1, rop_code << 4 | RA8889_BTE_MPU_WRITE_WITH_ROP);                                                             // 91h
    lcdRegDataWrite(RA8889_BTE_COLR, RA8889_S0_COLOR_DEPTH_16BPP << 5 | RA8889_S1_COLOR_DEPTH_16BPP << 2 | RA8889_DESTINATION_COLOR_DEPTH_16BPP); // 92h
    lcdRegDataWrite(RA8889_BTE_CTRL0, RA8889_BTE_ENABLE << 4);                                                                                    // 90h
    ramAccessPrepare();
}

//**************************************************************//
// Send data from the microcontroller to the RA8889
// Does a chromakey (transparent color) to combine with the image already in memory
//**************************************************************//
void RA8889_t41_p::bteMpuWriteWithChromaKeyData8(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y, ru16 width,
                                                 ru16 height, ru16 chromakey_color, const unsigned char *data) {
    bteMpuWriteWithChromaKey(des_addr, des_image_width, des_x, des_y, width, height, chromakey_color);
    // MulBeatWR_nPrm_DMA(data,(width)*(height));
    ru16 i, j;
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    //  while(digitalReadFast(WINT) == 0);  // If monitoring XnWAIT signal from RA8889.

    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            // delayNanoseconds(70);  // Initially setup for the dev board v4.0
            if (_rotation & 1)
                delayNanoseconds(20);
            p->SHIFTBUF[0] = generate_output_word(*data++);
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
            p->SHIFTBUF[0] = generate_output_word(*data++);
            // Wait for transfer to be completed
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    // De-assert /CS pin
    CSHigh();
}
//**************************************************************//
// Chromakey for 16-bit byte-reversed data. (Slower than 8-bit.)
//**************************************************************//
void RA8889_t41_p::bteMpuWriteWithChromaKeyData16(ru32 des_addr, ru16 des_image_width, ru16 des_x, ru16 des_y,
                                                  ru16 width, ru16 height, ru16 chromakey_color, const unsigned short *data) {
    ru16 i, j;
    bteMpuWriteWithChromaKey(des_addr, des_image_width, des_x, des_y, width, height, chromakey_color);

    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    for (j = 0; j < height; j++) {
        for (i = 0; i < width; i++) {
            delayNanoseconds(20); // Initially setup for the dev board v4.0
            if (_rotation & 1)
                delayNanoseconds(70);
            p->SHIFTBUF[0] = *data++;
            /*Wait for transfer to be completed */
            while (0 == (p->SHIFTSTAT & (1 << 0))) {
            }
            while (0 == (p->TIMSTAT & (1 << 0))) {
            }
        }
    }
    /* De-assert /CS pin */
    CSHigh();
}

// Helper functions.
void RA8889_t41_p::beginWrite16BitColors() {
    while (WR_IRQTransferDone == false) {
    } // Wait for any IRQ transfers to complete
    FlexIO_Config_SnglBeat();
    CSLow();
    DCHigh();
    delayNanoseconds(10); // Initially setup for the T4.1 board
    if (_rotation & 1)
        delayNanoseconds(20);
}
void RA8889_t41_p::write16BitColor(uint16_t color) {
    delayNanoseconds(10); // Initially setup for the T4.1 board
    if (_rotation & 1)
        delayNanoseconds(20);

    if (_bus_width != 16) {
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = generate_output_word(color & 0xff);

        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = generate_output_word(color >> 8);
    } else {
        while (0 == (p->SHIFTSTAT & (1 << 0))) {
        }
        p->SHIFTBUF[0] = color;
        DBGPrintf("$$16$$write16BitColor(%x)\n", color);
    }
}
void RA8889_t41_p::endWrite16BitColors() {
    // De-assert /CS pin
    while (0 == (p->TIMSTAT & (1 << 0))) {
    }
    delayNanoseconds(10); // Initially setup for the T4.1 board
    CSHigh();
}
