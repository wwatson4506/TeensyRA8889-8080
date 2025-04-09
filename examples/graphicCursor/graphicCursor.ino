// graphicsCursor.ino

/*
* This sketch uses a USB mouse and USBhost_t36 to control the RA8889 graphics
* cursor. It initializes the graphis cursor memory with four different
* cursor shapes. These are defined in RA8889_Lite and can be changed. 
* tft.Select_Graphic_Cursor_1();  Selects Pen Cursor.
* tft.Select_Graphic_Cursor_2();  Selects Arrow Cursor.
* tft.Select_Graphic_Cursor_3();  Selects Hour Glass (Busy) Cursor.
* tft.Select_Graphic_Cursor_4();  Selects Error (Stop) Cursor.
* These graphic cursors have two color setting, Foreground color and
* Outline color.
* I have included a few experimental routines of my own for detecting
* single and double clicks of the left mouse button.
*
* For double and single click testing mouse.cpp in USBHost_t36 library
* has to have "//	buttons = 0;" commented out.
* 
void MouseController::mouseDataClear() {
	mouseEvent = false;
//	buttons = 0;
	mouseX  = 0;
	mouseY  = 0;
	wheel   = 0;
	wheelH  = 0;
}

The button presses automatically cleared on release.
* I don't think they need to be part of "mouseDataClear();". Also
* wheel and wheelH are cleared when there is a button press or there is
* movement of the mouse.
*/

#include "USBHost_t36.h"
#include "RA8889_Config_8080.h"
#include <RA8889_t41_p.h>

// RA8889_8080_DC, RA8889_8080_CS and RA8889_8080_RESET are defined in
// src/RA8889_Config_8080.h.
RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

#define LEFT_MOUSE_BUTTON 1
#define RIGHT_MOUSE_BUTTON 2
#define MIDDLE_MOUSE_BUTTON 4

#define BUTTON_UP 0
#define BUTTON_DN 1
#define BUTTON_DRAG 2
#define BUTTON_SNGL_CLICK 3
#define BUTTON_DBL_CLICK 4 
 
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);

//************************************************************
// Even though the keyboard is not used we still need
// to create a keyboardController and USBHIDParser instance
// just in case a wireless keyboard/mouse is used. Otherwise
// the mouse will not be claimed.
//************************************************************* 
KeyboardController keyboard1(myusb); // Not used.
KeyboardController keyboard2(myusb);
MouseController mouse1(myusb);
USBHIDParser hid1(myusb); // Needed for USB mouse.
USBHIDParser hid2(myusb); // Needed for use with wireless keyboard/mouse combo.

// A structure to hold results of mouse operations.
// Some are not used in this sketch.
struct usbMouseMsg_struct {
  uint8_t buttons;
  uint8_t buttonState;
  uint8_t buttonStateLast;
  uint8_t buttonsLast;
  uint8_t snglClickCnt;
  uint8_t dblClickCnt;
  uint8_t clickCount;
  int8_t mousex;
  int8_t mousey;
  int16_t scaledX;
  int16_t scaledY;
  int8_t wheel;
  int8_t wheelH;
  int16_t mouseXpos;
  int16_t mouseYpos;
  uint32_t msLast;
  uint32_t snglClickTimeout;
  uint32_t dblClickTimeout;
  bool mouseEvent;
  bool isFirstClick = true;
  bool isDoubleClick = false;
};


enum { None, SingleClick, DoubleClick, clickDrag };

usbMouseMsg_struct mouse_msg;

// global Variables for scaleMouseXY.
int16_t fine_dx = 0;
int16_t fine_dy = 0;
int16_t event_dx = 0;
int16_t event_dy = 0;
int16_t nevent_dx = 0;
int16_t nevent_dy = 0;
// Adjustable parameters for mouse cursor movement.
int16_t delta = 127;
int16_t accel = 5;
int16_t scaleX = 2;
int16_t scaleY = 2;
uint8_t scCount = 0;
uint8_t dcCount = 0;

// Scale mouse XY to fit our screen (1023x599).
void scaleMouseXY(void) {
  if(!mouse1.available()) return;
  nevent_dx = (int16_t)mouse1.getMouseX();
  nevent_dy = (int16_t)mouse1.getMouseY();
  if(abs(nevent_dx) + abs(nevent_dy) > delta) {
    nevent_dx *= accel;
    nevent_dy *= accel;
  }
  event_dx += nevent_dx;
  event_dy += nevent_dy;
  fine_dx += event_dx; 
  fine_dy += event_dy; 
  event_dx = fine_dx / scaleX;
  event_dy = fine_dy / scaleY;
  fine_dx %= scaleX;
  fine_dy %= scaleY;
  mouse_msg.scaledX += event_dx;
  mouse_msg.scaledY += event_dy;
  if(mouse_msg.scaledX < 0)
    mouse_msg.scaledX = 0;
  if(mouse_msg.scaledX > (uint16_t)1023)
    mouse_msg.scaledX = (uint16_t)1023;
  if(mouse_msg.scaledY < 0)
    mouse_msg.scaledY = 0;
  if(mouse_msg.scaledY > (uint16_t)599)
    mouse_msg.scaledY = (uint16_t)599;
}

// Check for a mouse Event
bool mouseEvent(void) {
  if(!mouse1.available())
    return false;
  mouse_msg.wheel = (int8_t)mouse1.getWheel(); // Check for wheel movement
  mouse_msg.wheelH = (int8_t)mouse1.getWheelH();
  scaleMouseXY();
  return true;
}

// Check for mouse button presses
uint8_t getMouseButtons(void) {
  mouse_msg.buttons = (uint8_t)mouse1.getButtons();
  return mouse_msg.buttons;
}

/*
  void countMouseClicks(uint8_t button) {
	uint8_t clickCount = 0;
	uint32_t timerStart = 0;
	uint32_t mouse_msg.snglClickTimeout = 305;
	uint32_t mouse_msg.dblClickTimeout = 100;
	bool getOut = false;
    bool isFirstClick = true;
    bool isDoubleClick = false;
    
    if(!mouseEvent()) return;
    button &= 0x07; // Mask off three buttons
	if((mouse1.getButtons() & button)) {
		timerStart = millis();
		mouse_msg.buttonState = BUTTON_DN;
	}
}
*/
void countMouseClicks(uint8_t button) {
	uint8_t clickCount = 0;
	uint32_t timerStart = 0;
	uint32_t timeOut = 305;
	bool getOut = false;
    bool isFirstClick = true;
    bool isDoubleClick = false;
    
    if(!mouseEvent()) return;
    button &= 0x07; // Mask off three buttons
	while(!getOut) {
		// Check for mouse button double click.
		if((mouse1.getButtons() & button)) {
			if(clickCount == 0) {
				mouse_msg.snglClickCnt++;
				timerStart = millis(); 
			}
			clickCount++;
			if(clickCount >= 2) {
				clickCount = 0;
				mouse_msg.dblClickCnt++;
				getOut = true;
			}
 			// This is blocking!!!!
			while((mouse1.getButtons() & button)) {
				delay(0);
			}
		}
		if(((millis()-timerStart) > timeOut) || (getOut == true))
			break;
	}
}

// Check for a double left mouse button click
uint8_t getDblClick(void) {
  uint8_t dClick = mouse_msg.dblClickCnt;
  if(dClick > 0)
    mouse_msg.dblClickCnt = 0;
    return dClick;
}

// Check for a single left mouse button click
uint8_t getSnglClick(void) {
  uint8_t sClick = mouse_msg.snglClickCnt;
  if(sClick > 0)
    mouse_msg.snglClickCnt = 0;
    return sClick;
}

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
                        // src/RA8889_Config_8080.h. Default is 20MHz. 

  myusb.begin();
  mouse_msg.scaledX = 512;
  mouse_msg.scaledY = 300;
  mouse_msg.buttons = 0;
  mouse_msg.buttonsLast = 0;
  
  Serial.printf("%cUSB Mouse and Graphic Cursor Testing\n",12);

  tft.fillScreen(DARKBLUE);
  tft.setFontSize(1, false);
  tft.setCursor(0,0);
  tft.setTextColor(YELLOW, DARKBLUE);
  tft.print("USB Mouse and Graphic Cursor Testing");

  tft.Graphic_cursor_initial(); // Initialize all 4 cursors.
  tft.Select_Graphic_Cursor_2(); // Select Arrow Cursor.
  tft.Enable_Graphic_Cursor(); // Turn on selected graphic cursor.
  // You can play with the cursor colors in the following two lines.
  tft.Set_Graphic_Cursor_Color_1(0xff); // White forground Color. (0 - 255)
  tft.Set_Graphic_Cursor_Color_2(0x00); // Black outline Color. (0 - 255)
  tft.Graphic_Cursor_XY(mouse_msg.scaledX, mouse_msg.scaledY); // Center cursor on screen.
  tft.drawSquareFill(300,100,800,500,WHITE);
  tft.setTextColor(GREEN, DARKBLUE);

}
 
void loop() {
  uint8_t type = None;

  if(mouseEvent()) { // Wait for a mouse event.
    tft.Graphic_Cursor_XY(mouse_msg.scaledX, mouse_msg.scaledY); // Center cursor on screen
    tft.textxy(0,5);
    tft.printf("Mouse X: %4.4d\n", mouse_msg.scaledX);
    tft.printf("Mouse Y: %3.3d\n", mouse_msg.scaledY);
    tft.printf("Buttons: %d\n", getMouseButtons());
    tft.printf("Wheel: %2d\n", mouse_msg.wheel);
    tft.printf("WheelH: %2d\n", mouse_msg.wheelH);

	countMouseClicks(LEFT_MOUSE_BUTTON); // ***** This method is blocking *****

    scCount += getSnglClick(); // Add to Single Click Count.
    dcCount += getDblClick(); // Add to Double Click Count.
    tft.printf("Single Clicks: %d\n", scCount);
    tft.printf("Double Clicks: %d\n", dcCount);
/*
  type = chkButtons();

  switch(type)  {
  case SingleClick:
      mouse_msg.snglClickCnt++;
      break;

  case DoubleClick:
      mouse_msg.dblClickCnt++;
      break;
  }
*/
    mouse1.mouseDataClear();
  }
}
