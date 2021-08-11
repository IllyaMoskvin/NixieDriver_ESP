/*
	NixieDriver.cpp`
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

#ifndef	ESP8266
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#endif
#include <Arduino.h>
#include <NixieDriver_ESP.h>
#ifndef	ESP8266
#include <util/delay.h>
#endif

//#define DEBUG_TIMER
//#define DEBUG_BACKFADE

#define del 1
#define STARTDELAY 1000
#define calculateFadeColour(fadeIndex, i, fraction) (fadeSetup[(fadeIndex)][(i)] + ( (fraction) * ((fadeSetup[(fadeIndex)+1][3] != 0 ? fadeSetup[(fadeIndex)+1][(i)] : fadeSetup[0][(i)]) - fadeSetup[(fadeIndex)][(i)])))
#ifdef	ESP8266
#define TicksPerMilli		1		// 1 tick every millisecond
#else 	// ESP8266
#define TicksPerMilli		2.56	// 2560 ticks per second
#endif 	// ESP8266

const int  StepsPerFade	    = 256;
const int  TicksPerStep 	= (StepsPerFade / TicksPerMilli);
const long MaxTicksPerCycle	= (60 * 1000) * TicksPerMilli;

#define FADELIMIT 16 //maximum of 16 fade colours
#define CYCLETYPE 17][4
#define ENDCYCLE 0,0,0,0

//nixie_esp variables
int _dataPin;
int _clockPin;
int _outputEnablePin;
int _strobePin;
short _dpMask = 0x0; //mask for the decimal points
bool _clockModeEnable = 0;
int _symbolMask[6] = {0,0,0,0,0,0}; //for each nixie_esp (0 = NONE, 1 = IN15A, 2 = IN15B)
int _symbols[6] = {BLANK,BLANK,BLANK,BLANK,BLANK,BLANK};
int _symbolDivisor = 1;

//clock mode variables
long hours = 0;
long minutes = 0;
long seconds = 0;

//backlight variables
volatile uint8_t _pins[3]; //holds the pin declarations
volatile int currentColour[3] = {0,0,0}; //accesable from outside the library
int currentFadeColour[3] = {0,0,0}; //not accesable from outside the library
volatile unsigned long timerCount; //incremented on each timer call
volatile int fadeIndex = 0; 
volatile int fadeSetup[FADELIMIT + 1][4]; //holds the colour fade data

#ifndef	ESP8266
const unsigned long TMR1Speed = 390; 	//2560 cals per second
#else	// ESP8266
Ticker ticker;
#endif 	// ESP8266

void* pt2object = nullptr;

//colours

int backlight_esp::black[3]  = 		{	BLACK	  };
int backlight_esp::white[3]  = 		{ 	WHITE     };
int backlight_esp::red[3]    = 		{ 	RED  	  };
int backlight_esp::green[3]  = 		{ 	GREEN	  };
int backlight_esp::blue[3]   = 		{	BLUE      };
int backlight_esp::yellow[3] = 		{ 	YELLOW 	  };
int backlight_esp::dimWhite[3] = 	{ 	DIMWHITE  };
int backlight_esp::aqua[3] = 		{ 	AQUA      };
int backlight_esp::magenta[3] = 	{	MAGENTA   };
int backlight_esp::purple[3]= 		{	PURPLE    };

// Preserve SRAM by putting the sineTable in PROGMEM
// For faster access on devices with more SRAM, just
// pull out the PROGMEM below and redefine the sin()
// macro accordingly (see below)
const float sineTable[] PROGMEM = {
	0.000,0.000,0.000,0.001,0.001,0.001,0.002,0.,
	0.003,0.004,0.004,0.005,0.006,0.007,0.008,0.009,
	0.011,0.012,0.013,0.015,0.016,0.018,0.019,0.021,
	0.023,0.025,0.027,0.029,0.031,0.033,0.035,0.038,
	0.040,0.042,0.045,0.048,0.050,0.053,0.056,0.059,
	0.061,0.064,0.068,0.071,0.074,0.077,0.080,0.084,
	0.087,0.091,0.094,0.098,0.102,0.105,0.109,0.113,
	0.117,0.121,0.125,0.129,0.133,0.137,0.142,0.146,
	0.150,0.155,0.159,0.164,0.168,0.173,0.178,0.182,
	0.187,0.192,0.197,0.202,0.207,0.212,0.217,0.222,
	0.227,0.232,0.237,0.242,0.248,0.253,0.258,0.264,
	0.269,0.275,0.280,0.286,0.291,0.297,0.303,0.308,
	0.314,0.320,0.325,0.331,0.337,0.343,0.349,0.355,
	0.360,0.366,0.372,0.378,0.384,0.390,0.396,0.402,
	0.408,0.414,0.420,0.426,0.433,0.439,0.445,0.451,
	0.457,0.463,0.469,0.475,0.482,0.488,0.494,0.500,
	0.506,0.512,0.518,0.525,0.531,0.537,0.543,
	0.549,0.555,0.561,0.567,0.574,0.580,0.586,0.592,
	0.598,0.604,0.610,0.616,0.622,0.628,0.634,0.640,
	0.645,0.651,0.657,0.663,0.669,0.675,0.680,0.686,
	0.692,0.697,0.703,0.709,0.714,0.720,0.725,0.731,
	0.736,0.742,0.747,0.752,0.758,0.763,0.768,0.773,
	0.778,0.783,0.788,0.793,0.798,0.803,0.808,0.813,
	0.818,0.822,0.827,0.832,0.836,0.841,0.845,0.850,
	0.854,0.858,0.863,0.867,0.871,0.875,0.879,0.883,
	0.887,0.891,0.895,0.898,0.902,0.906,0.909,0.913,
	0.916,0.920,0.923,0.926,0.929,0.932,0.936,0.939,
	0.941,0.944,0.947,0.950,0.952,0.955,0.958,0.960,
	0.962,0.965,0.967,0.969,0.971,0.973,0.975,0.977,
	0.979,0.981,0.982,0.984,0.985,0.987,0.988,0.989,
	0.991,0.992,0.993,0.994,0.995,0.996,0.996,0.997,
	0.998,0.998,0.999,0.999,0.999,1.000,1.000,1.000200,
	1.000,
};

#define sin(i) (pgm_read_float(&sineTable[i]))
//#define sin(i) (sineTable[i])

/*-------------------NIXIE----------------------*/

nixie_esp::nixie_esp(int dataPin, int clk, int oe, int srb) //initialise the library
{
	setDataPin(dataPin); //set up pins
	setClk(clk);
	setOE(oe);
	setSrb(srb);
	//startupDelay();
	startupTransmission();
}

nixie_esp::nixie_esp(int dataPin, int clk, int oe) //initialise library
{
	setDataPin(dataPin); //set up pins
	setClk(clk);
	setOE(oe);
	//startupDelay();
	startupTransmission();
}

nixie_esp::nixie_esp(int dataPin, int clk) //initialise library
{
	setDataPin(dataPin); //set up pins
	setClk(clk);
	//startupDelay();
	startupTransmission();
}

nixie_esp::~nixie_esp() //close instance of library
{
}

void nixie_esp::startupTransmission(void)
{
	short data[6] = {0x0,0x0,0x0,0x0,0x0,0x0};
	shift(data);
	shift(data);
	for(int i = 0; i < 6; i++)
		setDecimalPoint(i, 0);
}

void nixie_esp::startupDelay() //delay to stop shit going wrong
{
	delay(STARTDELAY);
}

void nixie_esp::setDataPin(int dataPin) //set the data pin
{
	pinMode(dataPin, 1);
	_dataPin = dataPin;
}

void nixie_esp::setClk(int clk) //set the clock pin
{
	pinMode(clk, 1);
	_clockPin = clk;
}

void nixie_esp::setOE(int oe) //set the OE pin, and write it high
{
	pinMode(oe, 1);
	digitalWrite(oe, 1);
	_outputEnablePin = oe;
}

void nixie_esp::setSrb(int srb) //set the strobe pin, and write it high
{
	pinMode(srb, 1);
	digitalWrite(srb, 1);
	_strobePin = srb;
}

void nixie_esp::transmit(bool data)
{
  digitalWrite(_clockPin, 1);
  digitalWrite(_dataPin, data);
  digitalWrite(_clockPin, 0);
}

void nixie_esp::shift(short data[]) //shift out data to the HV5122
{
  blank(1);
  for(int i = 0; i < 8; i++)  //for the decimal points
	  transmit(_dpMask & (1 << i));
  for (int i = 5; i >= 0; i--) //for all 6 tubes
  {
    for (int j = 9; j >= 0; j--) //for all 10 segments
    {
      bool toShift = data[i] & (1 << j); //transfer the data into bool
      transmit(toShift); //send the data
    }
  }
  //transmit(0);
  blank(0);
}

void nixie_esp::displayDigits(int a, int b, int c, int d, int e, int f) //display a number
{
  int numbers[6] = {a, b, c, d, e, f}; 
  short data[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
  for (int i = 0; i < 6; i++) //for each tube
  {
    if(numbers[i] >= 10 || numbers[i] < 0)
	{
		data[i] = 0x0;
	}
    else if (numbers[i] != 0) 
      data[i] = (1 << ((numbers[i]) - 1)) ; //flip the correct bit
    else
      data[i] = (1 << 9); 
  }
  shift(data);
}

/*display a number with an integer input*/
void nixie_esp::disp(long num)
{
	int seg[6];
	seg[0] = num / 100000;        	/*seperate the number into it's component digits*/
	seg[1] = (num / 10000) % 10;
	seg[2] = (num / 1000) % 10;
	seg[3] = (num / 100) % 10;
	seg[4] = (num / 10) % 10;
	seg[5] = (num % 10);

	for(int i = 0; i < 5; i++)
	{
		if(_symbolMask[i])
		{
			for(int j = 5; j > i; j--) //move the set down one 
				seg[j] = seg[j-1];
		}
	}
	
	for(int i = 0; i < 6; i++)
		if(_symbolMask[i])
		{
			if(_symbolMask[i] == 1 && (_symbols[i] < 10 && _symbols[i] >= 0))
				seg[i] = _symbols[i];
			else if(_symbolMask[i] == 2 && (_symbols[i] >= 10 && _symbols[i] < 20 ))
				seg[i] = _symbols[i] - 10;
			else seg[i] = BLANK;
		}

	displayDigits(seg[0], seg[1], seg[2], seg[3], seg[4], seg[5]); /*display the number*/
}

void nixie_esp::display(long num)
{
	if(num < 0) num *= -1;
	if(!_clockModeEnable) _dpMask = 0x0;
	num *= _symbolDivisor;
	disp(num);
}

void nixie_esp::display(int num)
{
	if(num < 0) num *= -1;
	long longNum = num;
	if(!_clockModeEnable) _dpMask = 0x0;
	longNum *= _symbolDivisor;
	disp(longNum);
}

void nixie_esp::display(float num)
{
	if(num < 0) num *= -1;
	if(long(num*1000000) % 10 == 0) num += 0.000001;	/*fixes an issue which occurs when you input 2.1*/
	long mul = 1;						/*multiplier to turn float to int*/		
	long i = 1;	
	int j = 5;							/*order of bit that must be flipped for DP*/
	while(num * i < 1)
		i*=10;
	while(num * mul < (100000/i)) {
		mul *= 10;
		j--;
	}
	if(_clockModeEnable)
	{
		_clockModeEnable = 0;
		_dpMask = 0x0;
	}
	long dispNum = long(num * mul);
	int k = 0;
	while(_symbolMask[k] && k++ != 5) j++;
	for(int i = 0; i < 6; i++)
	{
		setDecimalPoint(i, (i == j ? 1 : 0));
	}
	
	disp(dispNum);
}

void nixie_esp::setDecimalPoint(int segment, bool state) /*sets the state of a given decimal point*/
{
	if(segment > 5) return;
	if(state) _dpMask |= (1 << (7 - segment));
	else _dpMask &= (0xFF ^ (1 << (7 - segment)));
}

void nixie_esp::blank(bool state) //blank function
{
	digitalWrite(_outputEnablePin, !state);
}

void nixie_esp::setClockMode(bool state)
{
	_clockModeEnable = state;	//sets the clock mode on
	for(int i = 0; i < 6; i++)
		_symbolMask[i] = 0;
	_dpMask = state ? 0x50 : 0x0; //sets the Decimal point mask to 001010000 - i.e. hh.mm.ss
}

void nixie_esp::setSegment(int segment, int symbolType)
{
	if(segment > 5 || segment < 0) return;
	if(symbolType > 2 || symbolType < 0) return;
	if(symbolType && !_symbolMask[segment])
		_symbolDivisor *= 10;
	else if (!symbolType && _symbolMask[segment])
		_symbolDivisor /= 10;
	_symbolMask[segment] = symbolType;
}

void nixie_esp::setSymbol(int segment, int symbol)
{
	if(segment > 5) return;
	if(!_symbolMask[segment]) return;
	
	_symbols[segment] = symbol;
}

void nixie_esp::setTime(int h, int m, int s) //sets the time variables
{
	hours = h;
	minutes = m;
	seconds = s;
}

void nixie_esp::setHours(int h) //sets the hours
{
	hours = h;
}

void nixie_esp::setMinutes(int m) //sets the minutes
{
	minutes = m;
}

void nixie_esp::setSeconds(int s) //sets the seconds
{
	seconds = s;
}

bool nixie_esp::updateTime(void)
{
	if (!_clockModeEnable) return 0;
	long dispNum = (10000 * hours) + (100 * minutes) + seconds;
	disp(dispNum);
	return 1;
}

/*-------------------------------------BACKLIGHT-----------------------------------*/

backlight_esp::backlight_esp(uint8_t redPin, uint8_t greenPin, uint8_t bluePin) //set up the pins and turn the leds off
{
	_pins[0] = redPin;
	_pins[1] = greenPin;
	_pins[2] = bluePin;
#ifdef 	ESP8266
	analogWriteRange(255);
#endif 	// ESP8266
	for(int i = 0; i < 3; i++)
		pinMode(_pins[i], 1);
	setColour(black);
}

void backlight_esp::setColour(int colour[]) //write a colour to the leds
{
#ifdef DEBUG_TIMER
	Serial.print("Set colour to: ");
	for(int z = 0; z < 3; z++)
	{
		Serial.print(colour[z]);
		Serial.print(" ");
	}
	Serial.println("");
#endif
	for(int i = 0; i < 3; i++) {
		analogWrite(_pins[i], 255 - colour[i]);
		currentColour[i] = colour[i];
	}
}

void backlight_esp::crossFade(int startColour[], int endColour[], int duration) //crossfades between colours
{
	int displayColour[3];
	setColour(startColour); //start at the start colour
	for (int i = 0; i < 256; i++) { //for all the entries in the sine table
		for(int j = 0; j < 3; j++) { //for r, g, and b
			displayColour[j] = startColour[j] + int(sin(i) * (endColour[j] - startColour[j]));
			//fade along a sine wave (between -90 and +90) between the two colours
		}
		setColour(displayColour); 
		delay(duration/256); //delay by the correct amount
	}
	setColour(endColour); //end colour to finish
}

void backlight_esp::fadeIn(int colour[], int duration) 
{
	int displayColour[3];
	setColour(black); //start at black
	for(int i = 4; i < 127; i++) //for the first half of the fade, increase linearly
	{
		for(int j = 0; j < 3; j++)
			displayColour[j] = int(float(i*0.5/127) * colour[j]); 
		setColour(displayColour);
		delay(duration/252);
	}
	for(int i = 127; i < 256; i++) //for the second half of the fade, increase according to the sine
	{
		for(int j = 0; j < 3; j++)
			displayColour[j] = int(sin(i) * colour[j]);
		setColour(displayColour);
		delay(duration/252);
	}
	setColour(colour);
}

void backlight_esp::fadeOut(int duration)
{
	int colour[3];
	for(int i = 0; i < 3; i++)
		colour[i] = currentColour[i];
	int displayColour[3];
	for(int i = 255; i > 127; i--) //for the first half of the fade, decrease according to the sine
	{
		for(int j = 0; j < 3; j++)
			displayColour[j] = int(sin(i) * colour[j]);
		setColour(displayColour);
		delay(duration/254);
	}
	for(int i = 127; i > 0; i--) //for the second half of the fade, decrease linearly
	{
		for(int j = 0; j < 3; j++)
			displayColour[j] = int(float(i*0.5/127) * colour[j]); 
		setColour(displayColour);
		delay(duration/254);
	}
	setColour(black);
}


void backlight_esp::setFade(int setup[][4])
{
	pt2object = this;
#ifndef	ESP8266
	Timer1.initialize(TMR1Speed); //start the timer with 2560 steps per second
#endif 	// ESP8266
	
	int i = 0;
	while(setup[i][3] != 0 && i <= FADELIMIT - 1) //copy setup into fadeSetup
	{
		for(int j = 0; j < 4; j++)
			fadeSetup[i][j] = setup[i][j];
		i++;
	}
	
	for(int i = 0; i < 3; i++)
	{
		analogWrite(_pins[i], 254); //set timers running
	}
	
	timerCount = 0;
#ifdef	ESP8266
	ticker.attach_ms((1/TicksPerMilli), isr);	// (1, isr_debug);
#else  	// ESP8266
	Timer1.attachInterrupt(isr);
#endif	// ESP8266
}


void backlight_esp::tmrFade()
{
#ifndef	ESP8266
	Timer1.detachInterrupt();
#endif 	// ESP8266

	int displayColour[3]; //holds the current colour
	
	if(fadeSetup[0][3] == 0) stopFade(10); //if no setup had been done

	int callColour = fadeSetup[fadeIndex][3] / TicksPerStep;  //works out the minimum step

	if(timerCount % callColour == 0) //if a colour change needs to be done
	{
		int colourTimer = timerCount / callColour; //work out the place in the sine table
		float fraction = sin(colourTimer);
		for(int i = 0; i < 3; i++)
		{
			displayColour[i] = calculateFadeColour(fadeIndex, i, fraction); //calculate next display colour
			updateAnalogPin(_pins[i], 255-displayColour[i]); //update the colour	
			currentFadeColour[i] = displayColour[i]; //update the current colour
		}
	}

	callColour = (fadeSetup[fadeIndex][3] / TicksPerStep) * 256; //for when the colour is finished fading

	timerCount++;
	if(timerCount % callColour == 0 || timerCount == MaxTicksPerCycle){
		fadeIndex++;
		if (fadeIndex == FADELIMIT || fadeSetup[fadeIndex][3] == 0) //wrap index for 0 duration elements or for general wrap
			fadeIndex = 0;
		timerCount = 0;
		//updateFadeTimer();
	}

#ifndef	ESP8266
	Timer1.attachInterrupt(isr);
#endif 	// ESP8266
}

void backlight_esp::stopFade(int duration)
{
#ifndef	ESP8266
	Timer1.detachInterrupt();
	Timer1.stop();
#else 	// ESP8266
	ticker.detach();
#endif 	// ESP8266
	setColour(currentFadeColour);
	fadeOut(duration);
}

void backlight_esp::isr(void)
{
	backlight_esp* mySelf = (backlight_esp*) pt2object;
	if (mySelf != nullptr) mySelf->tmrFade();
}

void backlight_esp::updateAnalogPin(volatile int pin, int val)
{
#ifndef	ESP8266
	// We need to make sure the PWM output is enabled for those pins
	// that support it, as we turn it off when digitally reading or
	// writing with them.  Also, make sure the pin is in output mode
	// for consistenty with Wiring, which doesn't require a pinMode
	// call for the analog output pins.
	
	switch(digitalPinToTimer(pin))
	{
		// XXX fix needed for atmega8
		#if defined(TCCR0) && defined(COM00) && !defined(__AVR_ATmega8__)
		case TIMER0A:
			OCR0 = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR0A) && defined(COM0A1)
		case TIMER0A:
			OCR0A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR0A) && defined(COM0B1)
		case TIMER0B:
			OCR0B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR1A) && defined(COM1A1)
		case TIMER1A:
			break;
		#endif

		#if defined(TCCR1A) && defined(COM1B1)
		case TIMER1B:
			break;
		#endif

		#if defined(TCCR1A) && defined(COM1C1)
		case TIMER1C:
			break;
		#endif

		#if defined(TCCR2) && defined(COM21)
		case TIMER2:
			OCR2 = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR2A) && defined(COM2A1)
		case TIMER2A:
			OCR2A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR2A) && defined(COM2B1)
		case TIMER2B:
			OCR2B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR3A) && defined(COM3A1)
		case TIMER3A:
			OCR3A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR3A) && defined(COM3B1)
		case TIMER3B:
			OCR3B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR3A) && defined(COM3C1)
		case TIMER3C:
			OCR3C = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR4A)
		case TIMER4A:
			OCR4A = val;	// set pwm duty
			break;
		#endif
		
		#if defined(TCCR4A) && defined(COM4B1)
		case TIMER4B:
			OCR4B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR4A) && defined(COM4C1)
		case TIMER4C:
			OCR4C = val; // set pwm duty
			break;
		#endif
			
		#if defined(TCCR4C) && defined(COM4D1)
		case TIMER4D:				
			OCR4D = val;	// set pwm duty
			break;
		#endif

						
		#if defined(TCCR5A) && defined(COM5A1)
		case TIMER5A:
			OCR5A = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR5A) && defined(COM5B1)
		case TIMER5B:
			OCR5B = val; // set pwm duty
			break;
		#endif

		#if defined(TCCR5A) && defined(COM5C1)
		case TIMER5C:
			OCR5C = val; // set pwm duty
			break;
		#endif

		case NOT_ON_TIMER:
			break;
		default:
			break;
	}
#else 	// ESP8266
	analogWrite(pin, val);
#endif	// ESP8266
}



/*--------------- TIMER 1 CODE-------------------------*/
#ifndef	ESP8266

TimerOne Timer1;              // preinstatiate

ISR(TIMER1_OVF_vect)          // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  Timer1.isrCallback();
}

void TimerOne::initialize(long microseconds)
{
  TCCR1A = 0;                 // clear control register A 
  TCCR1B = _BV(WGM13);        // set mode 8: phase and frequency correct pwm, stop the timer
  setPeriod(microseconds);
}


void TimerOne::setPeriod(long microseconds)		// AR modified for atomic access
{
  
  long cycles = (F_CPU / 2000000) * microseconds;                                // the counter runs backwards after TOP, interrupt is at BOTTOM so divide microseconds by 2
  if(cycles < RESOLUTION)              clockSelectBits = _BV(CS10);              // no prescale, full xtal
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11);              // prescale by /8
  else if((cycles >>= 3) < RESOLUTION) clockSelectBits = _BV(CS11) | _BV(CS10);  // prescale by /64
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12);              // prescale by /256
  else if((cycles >>= 2) < RESOLUTION) clockSelectBits = _BV(CS12) | _BV(CS10);  // prescale by /1024
  else        cycles = RESOLUTION - 1, clockSelectBits = _BV(CS12) | _BV(CS10);  // request was out of bounds, set as maximum
  
  oldSREG = SREG;				
  cli();							// Disable interrupts for 16 bit register access
  ICR1 = pwmPeriod = cycles;                                          // ICR1 is TOP in p & f correct pwm mode
  SREG = oldSREG;
  
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));
  TCCR1B |= clockSelectBits;                                          // reset clock select register, and starts the clock
}


void TimerOne::attachInterrupt(void (*isr)(), long microseconds)
{
  if(microseconds > 0) setPeriod(microseconds);
  isrCallback = isr;                                       // register the user's callback with the real ISR
  TIMSK1 = _BV(TOIE1);                                     // sets the timer overflow interrupt enable bit
	// might be running with interrupts disabled (eg inside an ISR), so don't touch the global state
//  sei();
  resume();												
}

void TimerOne::detachInterrupt()
{
  TIMSK1 &= ~_BV(TOIE1);                                   // clears the timer overflow interrupt enable bit 
															// timer continues to count without calling the isr
}

void TimerOne::resume()				// AR suggested
{ 
  TCCR1B |= clockSelectBits;
}

void TimerOne::start()	// AR addition, renamed by Lex to reflect it's actual role
{
  unsigned int tcnt1;
  
  TIMSK1 &= ~_BV(TOIE1);        // AR added 
  GTCCR |= _BV(PSRSYNC);   		// AR added - reset prescaler (NB: shared with all 16 bit timers);

  oldSREG = SREG;				// AR - save status register
  cli();						// AR - Disable interrupts
  TCNT1 = 0;                	
  SREG = oldSREG;          		// AR - Restore status register
	resume();
  do {	// Nothing -- wait until timer moved on from zero - otherwise get a phantom interrupt
	oldSREG = SREG;
	cli();
	tcnt1 = TCNT1;
	SREG = oldSREG;
  } while (tcnt1==0); 
 
//  TIFR1 = 0xff;              		// AR - Clear interrupt flags
//  TIMSK1 = _BV(TOIE1);              // sets the timer overflow interrupt enable bit
}

void TimerOne::stop()
{
  TCCR1B &= ~(_BV(CS10) | _BV(CS11) | _BV(CS12));          // clears all clock selects bits
}
#endif	// ESP8266
