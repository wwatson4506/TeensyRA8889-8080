/*************************************************************** 
 * graphics.ino 
 * 
 * Basic graphics test for RA8889 8080 parallel based display
 ***************************************************************/
#include "Arduino.h"
#include "RA8889_Config_8080.h"
#include <RA8889_t41_p.h>
#include <math.h>
#include "font_Arial.h"

// RA8889_8080_DC, RA8889_8080_CS and RA8889_8080_RESET are defined in
// src/RA8889_Config_8080.h.
RA8889_t41_p tft = RA8889_t41_p(RA8889_8080_DC,RA8889_8080_CS,RA8889_8080_RESET);

// Array of Simple RA8889 Basic Colors
PROGMEM uint16_t myColors[] = {
	0x0000,	0xffff,	0xf800,	0xfc10,	0x8000,	0x07e0,	0x87f0,	0x0400,
	0x001f,	0x051f,	0x841f,	0x0010,	0xffe0,	0xfff0,	0x8400,	0x07ff,
	0x87ff,	0x0410,	0xf81f,	0xfc1f,	0x8010,	0xA145
};
//**************************************************************************
// Note: There is an issue with filled rectangles and filled rrectangles!!!!
//       They are randomly going out of bounds into the status line.
//**************************************************************************

int interations = 0;
int w, h;
int i = 0;
int d = 100;   // Delay between repeat function calls.
int dd = 100; // Delay between functions.

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
  tft.setTextColor(BLACK);
  tft.setRotation(0);
  w = tft.width()-1; h = tft.height()-STATUS_LINE_HEIGHT-1;
  tft.fillScreen(myColors[11]);
  graphicsTest();
  delay(dd);
  tft.setRotation(1);
  w = tft.width()-1; h = tft.height()-STATUS_LINE_HEIGHT-1;
  tft.fillScreen(myColors[11]);
  graphicsTest();
  delay(dd);
  tft.setRotation(2);
  w = tft.width()-1; h = tft.height()-STATUS_LINE_HEIGHT-1;
  tft.fillScreen(myColors[11]);
  graphicsTest();
  delay(dd);
  tft.setRotation(3);
  w = tft.width()-1; h = tft.height()-STATUS_LINE_HEIGHT-1;
  tft.fillScreen(myColors[11]);
  graphicsTest();
  delay(dd);
  tft.printStatusLine(0,myColors[1],myColors[11],"FINISHED!");

}

void loop() {
}

void graphicsTest()
{

  tft.printStatusLine(0,myColors[1],myColors[11],"Rectangles");
  interations = 20000;
  rectangles(1);
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Rectangles 10 pixel line thickness");
  interations = 4000;
  rectangles(10);
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Filled Rectangles");
  interations = 4000;
  filledRectangles();
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Round Rectangles");
  interations = 20000;
  rRectangles(1);
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Round Rectangles 10 pixel line thickness");
  interations = 4000;
  rRectangles(10);
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Filled Round Rectangles");
  interations = 4000;
  filledRRectangles();
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Circles");
  interations = 20000;
  drawcircles(1);
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Circles 10 pixel circle thickness");
  interations = 4000;
  drawcircles(10);
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Filled Circles");
  interations = 4000;
  fillcircles();
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Triangles");
  interations = 20000;
  drawTriangles();
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Filled Triangles");
  interations = 4000;
  fillTriangles();
  tft.fillScreen(myColors[11]);

  tft.printStatusLine(0,myColors[1],myColors[11],"Ellipses");
  interations = 20000;
  drawEllipses();
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Filled Ellipses");
  interations = 4000;
  fillEllipses();
  tft.fillScreen(myColors[11]);
  delay(dd);

  tft.printStatusLine(0,myColors[1],myColors[11],"Lines");
  interations = 20000;
  drawlines();
  tft.fillScreen(myColors[11]);
  delay(dd);
}

// Draw random unfilled rectangle boxes
void rectangles(uint16_t thickness) {
  uint16_t x0, y0, x1, y1, c;
  uint16_t j;

  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(1,512);
    y0 = (uint16_t)random(1,288);
    x1 = (uint16_t)random(512,w);
    y1 = (uint16_t)random(288,h);
    c = (uint16_t)random(21);
    if(x0 > tft.width()) x0 = tft.width();
    if(y0 > tft.height()) y0 = tft.height();
    if(x1 > tft.width()) x1 = tft.width();
    if(y1 > tft.height()) y1 = tft.height();
    if(thickness > 0) {
      for(j = 1; j <= thickness; j++) {
		tft.check2dBusy();
        tft.drawRect(x0,y0,x1-x0,y1-y0,myColors[c]);
        if(x0 <= tft.width()) x0++;
        if(y0 <= tft.height()) y0++;
        if(x1 > 0) x1--;
        if(y1 > 0) y1--;
		delayMicroseconds(d);
      }
    } else {
      tft.check2dBusy();
      tft.drawRect(x0,y0,x1-y0,y1-y0,myColors[c]);
	  delayMicroseconds(d);
    }
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random filled rectangle boxes
void filledRectangles(void) {
  uint16_t x0, y0, x1, y1, c;
  for(int i=0; i< interations; i++) {
    x0 = (uint16_t)random(1,w-1);
    y0 = (uint16_t)random(1,h-1);
    x1 = (uint16_t)random(1,w-1);
    y1 = (uint16_t)random(1,h-1);
    c = (uint16_t)random(1,21);
    tft.check2dBusy();
    tft.fillRect(x0,y0,x1-x0,y1-y0,myColors[c+1]);
    delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random round rectangle boxes
void rRectangles(uint16_t thickness) {
  uint16_t x0, y0, x1, y1, xr, yr, c;
  uint16_t j;

  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(1,512);
    y0 = (uint16_t)random(1,288);
    x1 = (uint16_t)random(512,w);
    y1 = (uint16_t)random(288,h);
    xr = 20; // Major Radius - Line Thickness must be less than
    yr = 20; // Minor Radius - Major and Minor radiuses by at
//                           - least half of xr and yr.
    c = (uint16_t)random(21);
    if(x0 > tft.width()) x0 = tft.width();
    if(y0 > tft.height()) y0 = tft.height();
    if(x1 > tft.width()) x1 = tft.width();
    if(y1 > tft.height()) y1 = tft.height();
    // Make sure major radius (xr) is less than x1 - x0
    // Must be xr * 2 + 1 less than x1 - x0
    // RA8889.pdf section 12.6 page 62
    if((xr * 2 + 1) >= (x1 - x0)) xr = (x1 - x0) / 2 - 1;
    // Same for minor radius (yr)
    if((yr * 2 + 1) >= (y1 - y0)) yr = (y1 - y0) / 2 - 1;
    if(thickness > 0) {
      for(j = 1; j <= thickness; j++) {
		tft.check2dBusy();
        tft.drawRoundRect(x0,y0,x1-x0,y1-y0,xr,yr,myColors[c]);
        if(x0 <= tft.width()) x0++;
        if(y0 <= tft.height()) y0++;
        if(x1 > 0) x1--;
        if(y1 > 0 ) y1--;
        if(xr > 0) xr--;
        if(yr > 0) yr--;
		delayMicroseconds(d);
      }
    } else {
      tft.check2dBusy();
      tft.drawRoundRect(x0,y0,x1-x0,y1-y0,xr,yr,myColors[c]);
	  delayMicroseconds(d);
    }
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random filled round rectangle boxes
void filledRRectangles(void) {
  uint16_t x0, y0, x1, y1, xr, yr, c;

  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(1,512);
    y0 = (uint16_t)random(1,288);
    x1 = (uint16_t)random(512,w);
    y1 = (uint16_t)random(288,h);
    xr = 20; // Major Radius
    yr = 20; // Minor Radius
    c = (uint16_t)random(21);
    // Keep x,y within 1024x576 boundries
    if(x0 > tft.width()) x0 = tft.width();
    if(y0 > tft.height()) y0 = tft.height();
    if(x1 > tft.width()) x1 = tft.width();
    if(y1 > tft.height()) y1 = tft.height();
    // Make sure major radius (xr) is less than x1 - x0
    // Must be xr * 2 + 1 less than x1 - x0
    // RA8889.pdf section 12.6 page 62
    if((xr * 2 + 1) >= (x1 - x0)) xr = (x1 - x0) / 2 - 1;
    // Same for minor radius (yr)
    if((yr * 2 + 1) >= (y1 - y0)) yr = (y1 - y0) / 2 - 1;
	tft.check2dBusy();
    tft.fillRoundRect(x0, y0, x1-x0, y1-y0, xr, yr, myColors[c]);
	delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random circles
void drawcircles(uint16_t thickness) {
  uint16_t x0, y0, r, c;
  int j;
  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(w);
    y0 = (uint16_t)random(h);
    r = (uint16_t)random(239);
    c = (uint16_t)random(21);
    if(x0-r <= 0) x0 += (uint16_t)r;
    if(y0-r <= 0) y0 += (uint16_t)r;
    if(x0+r >=  tft.width()) x0 = (uint16_t)(w - r);
    if(y0+r >= h) y0 = (uint16_t)(h - r);
    if(thickness > 0) {
      for(j = 1; j <= thickness; j++) {
		tft.check2dBusy();
        tft.drawCircle(x0, y0, r, myColors[c]);
        if(r > 0) r--;
		delayMicroseconds(d);
      }
    } else {
      tft.check2dBusy();
      tft.drawCircle(x0, y0, r, myColors[c]);
	  delayMicroseconds(d);
    }
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random filled circles
void fillcircles(void) {
  uint16_t x0, y0, r, c;
  for(int i=0; i< interations; i++) {
    x0 = (uint16_t)random(w);
    y0 = (uint16_t)random(h);
    r = (uint16_t)random(239);
    c = (uint16_t)random(21);
    if(x0-r <= 0) x0 += (uint16_t)r;
    if(y0-r <= 0) y0 += (uint16_t)r;
    if(x0+r >=  w) x0 = (uint16_t)(w - r);
    if(y0+r >= h) y0 = (uint16_t)(h - r);
	tft.check2dBusy();
    tft.fillCircle(x0, y0, r, myColors[c]);
	delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random unfilled tritangles
void drawTriangles(void) {
  uint16_t x0, y0, x1, y1, x2, y2, c;
  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(w);
    y0 = (uint16_t)random(h);
    x1 = (uint16_t)random(w);
    y1 = (uint16_t)random(h);
    x2 = (uint16_t)random(w);
    y2 = (uint16_t)random(h);
    c = (uint16_t)random(21);
	tft.check2dBusy();
    tft.drawTriangle(x0,y0,x1,y1,x2,y2,myColors[c+1]);
	delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random filled triangles
void fillTriangles(void) {
  uint16_t x0, y0, x1, y1, x2, y2, c;
  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(w);
    y0 = (uint16_t)random(h);
    x1 = (uint16_t)random(w);
    y1 = (uint16_t)random(h);
    x2 = (uint16_t)random(w);
    y2 = (uint16_t)random(h);
    c = (uint16_t)random(21);
	tft.check2dBusy();
    tft.fillTriangle(x0,y0,x1,y1,x2,y2,myColors[c+1]);
	delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random unfilled ellipses
void drawEllipses(void) {
  int16_t  x0, y0, xr, yr;
  uint16_t color;
  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(w);
    y0 = (uint16_t)random(h);
    xr = (uint16_t)random(239);
    yr = (uint16_t)random(239);
    color = (uint16_t)random(21);
    if(x0-xr <= 0) x0 += (uint16_t)xr;
    if(y0-yr <= 0) y0 += (uint16_t)yr;
    if(x0+xr >= w) x0 = (uint16_t)(w - xr);
    if(y0+yr >= h) y0 = (uint16_t)(h - yr);
	tft.check2dBusy();
    tft.drawEllipse(x0, y0, xr, yr, myColors[color]);
	delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random filled ellipses
void fillEllipses(void) {
  int16_t  x0, y0, xr, yr;
  uint16_t color;
  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(w);
    y0 = (uint16_t)random(h);
    xr = (uint16_t)random(239);
    yr = (uint16_t)random(239);
    color = (uint16_t)random(21);
    if(x0-xr <= 0) x0 += (uint16_t)xr;
    if(y0-yr <= 0) y0 += (uint16_t)yr;
    if(x0+xr >= w) x0 = (uint16_t)(w - xr);
    if(y0+yr >= h) y0 = (uint16_t)(h- yr);
	tft.check2dBusy();
    tft.fillEllipse(x0, y0, xr, yr, myColors[color]);
	delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}

// Draw random Lines
void drawlines(void) {
  uint16_t x0, y0, x1, y1, c;
  for(int i=0; i < interations; i++) {
    x0 = (uint16_t)random(1,w);
    y0 = (uint16_t)random(1,h);
    x1 = (uint16_t)random(1,w);
    y1 = (uint16_t)random(1,h);
     c = (uint16_t)random(21);
    if(x0 > tft.width()) x0 = tft.width();
    if(y0 > tft.height()) y0 = tft.height();
    if(x1 > tft.width()) x1 = tft.width();
    if(y1 > tft.height()) y1 = tft.height();
	tft.check2dBusy();
    tft.drawLine(x0,y0,x1,y1,myColors[c]);
	delayMicroseconds(d);
  }
  tft.fillStatusLine(myColors[11]);
}
