#include <NixieDriver.h>

//Pin assignments for backlight pins
const int redPin = 3;
const int greenPin = 5;
const int bluePin = 6;

//Pin assignments for nixies
const int data = 8;
const int clock = 9;
const int oe = 10;

nixie nixie(data, clock, oe);
backlight rgb(redPin, greenPin, bluePin);

void setup() {
  //Serial.begin(9600);
  // Start off with the LED off.
  rgb.setColour(rgb.white);
  delay(1000);
}

void loop() {
  int rgbColour[3];
  int decColour = -1;
  int incColour;
  rgbColour[0] = 255;
  rgbColour[1] = 0;
  rgbColour[2] = 0;


  int h = 20;
  int m = 45;
  int s = 01;
  nixie.setClockMode(1);
  nixie.setTime(h, m, s);

  while (1) {

    for (int i = 0; i < 255; i += 1) {
      rgbColour[decColour] -= 1;
      rgbColour[incColour] += 1;

      rgb.setColour(rgbColour);
      delay(4);
    }

    decColour = decColour == 2 ? 0 : decColour + 1;
    incColour = decColour == 2 ? 0 : decColour + 1;

    nixie.updateTime();
    if (m == 59 && s == 59) h = (h == 23) ? 0 : h + 1;
    if (s == 59) m = (m == 59) ? 0 : m + 1;
    s = (s == 59) ? 0 : s + 1;
    nixie.setTime(h, m, s);
  }
}
