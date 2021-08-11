#include <NixieDriver_ESP.h>

const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;

const int data = 8;
const int clock = 9;
const int oe = 10;

nixie nixie(data, clock, oe);
backlight rgb(redPin, greenPin, bluePin);

int h, m, s;

void setup() {
  //Serial.begin(9600);
    // Start off with the LED off.
  setColourRgb(0,0,0);
}

void loop() {
  /*h = 12;
  m = 33;
  s = 48;
  nixie.setClockMode(1);
  nixie.setTime(h,m,s);
  while(1) {
    delay(1000);
    nixie.updateTime();
    if(m == 59 && s == 59) h = (h == 23) ? 0 : h+1;
    if (s == 59) m = (m == 59) ? 0 : m+1;
    s = (s == 59) ? 0 : s+1;
    nixie.setTime(h,m,s);
  }*/
  for(long i = 999900; i < 1000100; i++)
  {
    nixie.display(i);
    delay(100);
  }
  while(1)
  {
    int rgbColour[3];

    // Start off with red.
    rgbColour[0] = 255;
    rgbColour[1] = 0;
    rgbColour[2] = 0; 
  
    // Choose the colours to increment and decrement.
    for (int decColour = 0; decColour < 3; decColour += 1) {
      int incColour = decColour == 2 ? 0 : decColour + 1;
  
      // cross-fade the two colours.
      for(int i = 0; i < 255; i += 1) {
        rgbColour[decColour] -= 1;
        rgbColour[incColour] += 1;
        
        rgb.setColour(rgbColour);
        delay(5);
      }
    }
  }
}

void setColourRgb(unsigned int red, unsigned int green, unsigned int blue) {
  analogWrite(redPin, 255 - red);
  analogWrite(greenPin, 255 - green);
  analogWrite(bluePin, 255 - blue);
 }


