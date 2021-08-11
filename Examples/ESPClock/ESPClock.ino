
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NixieDriver_ESP.h>


// ----- Network related constants and variables
static const char     ssid[] = "FlatmanN";  // your network SSID (name)
static const char     pass[] = "feedface";  // your network password
static const char     *ntpServerName = "time.nist.gov";
static const uint16_t localPort = 2390;     // local port to listen for UDP packets
static const int  NTP_PACKET_SIZE = 48;     // NTP time stamp is in the first 48 bytes

byte        packetBuffer[ NTP_PACKET_SIZE]; // buffer to hold UDP packets
WiFiUDP     udp;          // A UDP instance to let us send and receive packets over UDP
IPAddress   timeServerIP; // time.nist.gov NTP server address

// ----- Time related constants and variables
static const int TZOffset = -8; // Adjust for local time zone
int hour = -1;
int minute = -1;
int second = -1;
long millisOfLastSecond = 0;

// ----- Nixie Board related constants and variables
//Pin assignments for backlight pins
static const int redPin   = D4;
static const int greenPin = D3;
static const int bluePin  = D2;

//Pin assignments for nixies
static const int data  = D5;
static const int clock = D6;
static const int oe    = D7;
static const int srb   = D8;

// The primary display objects
nixie_esp nixie(data, clock, oe, srb);
backlight_esp rgb(redPin, greenPin, bluePin);

int cycle[CYCLETYPE] = {
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


// ----- setup() and loop() functions

void setup() {
  Serial.begin(115200);
  connectToWiFi();

  rgb.setColour(rgb.white); delay(100);
  nixie.setClockMode(1);
  nixie.setTime(hour, minute, second);
  rgb.setFade(cycle);
}

void loop() {
  if (hour < 0) {
    getTheTime();
    return;
  }
  
  int cur = millis();
  if (cur - millisOfLastSecond >= 1000) {
    millisOfLastSecond = cur;
    second++;
    if (second == 60) {
      second = 0;
      minute++;
      if (minute == 60) {
        minute = 0;
        hour++;
        if (hour == 24) hour = 0;
      }
    }
    nixie.setTime(hour, minute, second);
    nixie.updateTime();
  }
}

// void printTime() {
//   Serial.print(hour); Serial.print(":");
//   if (minute < 10) Serial.print('0');
//   Serial.print(minute); Serial.print(":");
//   if (second < 10) Serial.print('0');
//   Serial.println(second);
// }


/*
 * Code related to WiFi, UDP, and NTP
 * Based on sample code in the ESP8266 core
 */

void connectToWiFi() {
  Serial.print("Connecting to "); Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  udp.begin(localPort);
  WiFi.hostByName(ntpServerName, timeServerIP); }



// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void getTheTime() {
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
    return;
  }

  millisOfLastSecond = millis();
  // We've received a packet, read the data from it
  udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

  // The timestamp starts at byte 40 of the received packet and is four bytes,
  // or two words, long. First, esxtract the two words:
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  Serial.print("Seconds since Jan 1 1900 = " );
  Serial.println(secsSince1900);

  // now convert NTP time into everyday time:
  Serial.print("Unix time = ");
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSince1900 - seventyYears;
  // print Unix time:
  Serial.println(epoch);

  // Adjust for the TZ
  epoch += ((TZOffset * 60) * 60);

  hour = (epoch  % 86400L) / 3600;
  minute = (epoch  % 3600) / 60;
  second = (epoch % 60);
}
