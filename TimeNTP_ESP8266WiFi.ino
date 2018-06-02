/*
 * Time NTP for WiFiKit8 (ESP8266)
 * Original code is from Time master Sketch
 * Example showing time sync to NTP time source
 *
 * This sketch uses the ESP8266WiFi library
 */

#include <Wire.h>
#include <U8x8lib.h>

#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

const char ssid[] = "aterm-a42c3e-g";  //  your network SSID (name)
const char pass[] = "6c8a8d4deac61";       // your network password

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);   // WiFiKit 8 pin remapping with ESP8266 HW I2C

// Create a U8x8log object for logging
U8X8LOG u8x8log;
// Define the dimension of the U8x8log window
#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8
// Allocate static memory for the U8x8log window
uint8_t u8log_buffer[U8LOG_WIDTH*U8LOG_HEIGHT];

// NTP Servers:
static const char ntpServerName[] = "time.nist.gov";

const int timeZone = 9;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)


WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void setup()
{
// initialize OLED
  u8x8.begin();
  // Set a suitable font. This font will be used for U8x8log
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  // Start u8x8log, connect to U8x8, set the dimension and assign the static memory
  u8x8log.begin(u8x8, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer);
  // Set the U8x8log redraw mode
  u8x8log.setRedrawMode(1);		// 0: Update screen with newline, 1: Update screen for every char
  u8x8.clear();

  u8x8log.print("Connecting to ");
  u8x8log.println(ssid);
  u8x8log.print("\n");
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    u8x8log.print(".");
  }

  u8x8log.print("Local IP:");
  u8x8log.print(WiFi.localIP());
  u8x8log.print("\n");

  Udp.begin(localPort);
  u8x8log.print("Local port: ");
  u8x8log.println(Udp.localPort());
  u8x8log.print("\n");
  u8x8log.println("waiting for sync");
  u8x8log.print("\f");

  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  delay(5000);
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop()
{
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
  delay(4000);
  u8x8log.println(ssid);
  u8x8log.print("\n");
  u8x8log.print("Local IP:");
  u8x8log.print(WiFi.localIP());
  u8x8log.print("\n");
  u8x8log.print("Local port: ");
  u8x8log.println(Udp.localPort());
  delay(2000);
  u8x8log.print("\f");
}

void digitalClockDisplay()
{
  // digital clock display of the time
  u8x8log.print(hour());
  printDigits(minute());
  printDigits(second());
  u8x8log.print(" ");
  u8x8log.print(month());
  u8x8log.print("/");
  u8x8log.print(day());
  u8x8log.print("\r");
//  u8x8log.print(".");
//  u8x8log.print(year());
//  u8x8log.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  u8x8log.print(":");
  if (digits < 10)
    u8x8log.print('0');
  u8x8log.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  //Serial.print(ntpServerName);
  //Serial.print(": ");
  //Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      //Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  u8x8log.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
