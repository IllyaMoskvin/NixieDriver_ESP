#include <NixieDriver_ESP.h>

#ifdef  ESP8266
//Pin assignments for backlight pins
const int redPin   = D4;
const int greenPin = D3; 
const int bluePin  = D2; 

//Pin assignments for nixies
const int data  = D5;
const int clock = D6;
const int oe    = D7;
const int srb   = D8;
#else   // ESP8266
const int redPin   =  3;
const int greenPin = 5;
const int bluePin  = 6;

//Pin assignments for nixies
const int data  = 8;
const int clock = 9;
const int oe    = 10;
const int srb   = 11;
#endif  // ESP8266



nixie_esp nixie(data, clock, oe, srb);
backlight_esp rgb(redPin, greenPin, bluePin);

int cycle1[CYCLETYPE] = {
    { RED,     0512 },    //Stay red for 2s (red -> red)
    { RED,     2000 },    //Fade from red to yellow in 2s
    { YELLOW,  0512 },    //Fade from yellow to green in 4s
    { YELLOW,  4000 },    //Fade from yellow to green in 4s
    { GREEN,   0512 },    //Fade from green to blue in 5s
    { GREEN,   5000 },    //Fade from green to blue in 5s
    { BLUE,    0512 },    //Fade from blue to pink in 8s
    { BLUE,    8000 },    //Fade from blue to pink in 8s
    { MAGENTA, 0512 },    //Fade from pink to red in 1s
    { MAGENTA, 1000 },    //Fade from pink to red in 1s
    { ENDCYCLE      }
};

int h = 20;
int m = 45;
int s = 01;

void setup() {
  Serial.begin(115200);
  // Start off with the LED off.
  rgb.setColour(rgb.white);
  delay(1000);
  nixie.setClockMode(1);
  nixie.setTime(h, m, s);
  rgb.setFade(cycle1);
}


void loop() {
  nixie.updateTime();
  if (m == 59 && s == 59) h = (h == 23) ? 0 : h + 1;
  if (s == 59) m = (m == 59) ? 0 : m + 1;
  s = (s == 59) ? 0 : s + 1;
  nixie.setTime(h, m, s);
  delay(990);
}
