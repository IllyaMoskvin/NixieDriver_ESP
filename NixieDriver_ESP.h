/*
	NixieDriver.h
	Written by Thomas Cousins for http://doayee.co.uk
	
	Library to accompany the Nixie Tube Driver board from Doayee.
	
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#ifndef	ESP8266
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#endif

#ifndef NixieDriver_h
#define NixieDriver_h

#define IN15A 1
#define IN15B 2
#define NUMBER 0

#define MU 0
#define MICRO 0
#define NANO 1
#define PERCENT 2
#define CAP_PI 3
#define KILO 4
#define KELVIN 4
#define MEGA 5
#define METERS 5
#define MINUTES 5
#define MILLI 6
#define PLUS 7
#define MINUS 8
#define PICO 9
#define WATTS 10
#define AMPS 11
#define OHMS 12
#define SIEMENS 14
#define SECONDS 14
#define VOLTS 15
#define HENRYS 16
#define HERTZ 17
#define FARADS 19
#define BLANK 99

#ifndef	ESP8266
#define RESOLUTION 65536    // Timer1 is 16 bit
#endif

#define BLACK 0,0,0 
#define WHITE 255,255,255
#define RED 255,0,0
#define GREEN 0,255,0
#define BLUE 0,0,255
#define YELLOW 255,175,0
#define DIMWHITE 100,100,100
#define AQUA 0,255,255
#define MAGENTA 255,0,100
#define PURPLE 190,0,255
#define ENDCYCLE 0,0,0,0
#define CYCLETYPE 17][4



class nixie_esp
{
	private:
	
		//static nixie_esp *activate_object;
		void transmit(bool data);
		void shift(short data[]);
		void startupDelay(void);
		void setDataPin(int dataPin);
		void setClk(int clk);
		void setOE(int oe);
		void setSrb(int srb);
		void disp(long num);
		void startupTransmission(void);
		
		
	public:
	
		
		volatile long hours;
		volatile long minutes;
		volatile long seconds;
		
		nixie_esp(int dataPin, int clk, int oe, int srb);
		nixie_esp(int dataPin, int clk, int oe);
		nixie_esp(int dataPin, int clk);
		~nixie_esp();
		
		void displayDigits(int a, int b, int c, int d, int e, int f);
		void display(float num);
		void display(long num);
		void display(int num);
		void setDecimalPoint(int segment, bool state);
		void blank(bool state);
		void setClockMode(bool state);
		void setTime(int h, int m, int s);
		void setHours(int h);
		void setMinutes(int m);
		void setSeconds(int s);
		bool updateTime(void);
		void setSegment(int segment, int symbolType);
		void setSymbol(int segment, int symbol);
		
};


class backlight_esp
{
	private:
	
		void tmrFade();
		static void isr(void);
		void updateAnalogPin(volatile int pin, int val);
		//void updateFadeTimer(void);

	public:
	    
		static int black[3];
		static int white[3];
		static int red[3];
		static int green[3];
		static int blue[3];
		static int yellow[3];
		static int dimWhite[3];
		static int magenta[3];
		static int aqua[3];
		static int purple[3];
		
		int currentColour[3];
		
		backlight_esp(uint8_t redPin, uint8_t bluePin, uint8_t greenPin);
		
		void setColour(int colour[]);
		void crossFade(int startColour[], int endColour[], int duration);
		void fadeIn(int colour[], int duration);
		void fadeOut(int duration);
		
		void setFade(int setup[][4]);
		void stopFade(int duration);
};

#ifndef	ESP8266
class TimerOne
{
	public:
  
		// properties
		unsigned int pwmPeriod;
		int clockSelectBits;
		char oldSREG;					// To hold Status Register while ints disabled

		// methods
		void initialize(long microseconds=1000000);
		void start();
		void stop();
		void resume();
		void attachInterrupt(void (*isr)(), long microseconds=-1);
		void detachInterrupt();
		void setPeriod(long microseconds);
		void (*isrCallback)();
};

extern TimerOne Timer1;
#else
#include <Ticker.h>
extern Ticker ticker;
#endif

// Arduino 0012 workaround
#undef int
#undef char
#undef long
#undef byte
#undef float
#undef abs
#undef round

#endif	
