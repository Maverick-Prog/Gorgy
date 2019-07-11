// Controlling a common anode 4 digit 7seg display with the Max7219
// David Barton, 2016
// Other bits added by John Nicot and Maverick 2019

#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <everytime.h>
#include <avr/sleep.h>
#include <RH_RF95.h>
     

// The Max7219 Registers :
// control modes
#define MAXREG_DECODE_MODE    0x09                       
#define MAXREG_INTENSITY      0x0A                        
#define MAXREG_SCAN_LIMIT     0x0B                        
#define MAXREG_SHUTDOWN       0x0C                        
#define MAXREG_DISPLAY_TEST   0x0F
// digit registers
#define MAXREG_NOOP           0x00
#define MAXREG_DIG0           0x01
#define MAXREG_DIG1           0x02
#define MAXREG_DIG2           0x03
#define MAXREG_DIG3           0x04
#define MAXREG_DIG4           0x05
#define MAXREG_DIG5           0x06
#define MAXREG_DIG6           0x07
#define MAXREG_DIG7           0x08

// Lora Pin Setup
#define RFM95_CS 2
#define RFM95_RST 5
#define RFM95_INT 4

// Pin setup for separators
#define SEPARATOR_1           3
#define SEPARATOR_2           6
#define SEPARATOR_3           A4     

// pin setup for LEDS
#define LED_PIN    9
#define LED_COUNT 72

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
uint32_t red=strip.Color(255,0,0);
uint32_t cyan=strip.Color(0,255,255);

#define CS_MAX7221            10    // spi chip select pin for max7221
#define CS_LORA               2     // spi chip select pin for LoRa

// GLOBAL VARIABLES
byte display_num[8];                // array to hold our 6 digits
bool cd=false;                      // Countdown mode
bool ck=true;                       // Clock Mode
bool separatorBlink;
// test
int32_t normaltime=0;
int32_t countdown = 10;

// standard 7 seg display numbers
byte number_seg[]=
{
  0b00111111,  // 0 
  0b00000110,  // 1
  0b01011011,  // 2
  0b01001111,  // 3
  0b01100110,  // 4
  0b01101101,  // 5  
  0b01111101,  // 6
  0b00000111,  // 7
  0b01111111,  // 8
  0b01101111,  // 9
  0b01000000,  // -
  0b00000000   // blank
};


void MAX7221Send(uint8_t address, uint8_t value) {
  uint8_t oldaddr = 0;
  uint8_t oldval = 0;
  // Ensure LOAD/CS is LOW
  digitalWrite(CS_MAX7221, LOW);

  // Send the register address
  oldaddr = SPI.transfer(address);
  /*Serial.print("ADDR : ");
  Serial.print(address);
  Serial.print("\n");*/

  // Send the value
  oldval = SPI.transfer(value);
  /*Serial.print("VAL  : ");
  Serial.print(value);
  Serial.print("\n");*/

  // old values from miso
  /*Serial.print("OADDR: ");
  Serial.print(oldaddr);
  Serial.print("\n");
  Serial.print("OVAL : ");
  Serial.print(oldval);
  Serial.print("\n");*/
  
  // Tell chip to load in data
  digitalWrite(CS_MAX7221, HIGH);

  return;
}

void MAX7221Setup() {
  // set up max7219
  // Disable BCD mode
  MAX7221Send(MAXREG_DECODE_MODE, 0x00);
  // brightness
  MAX7221Send(MAXREG_INTENSITY, 0x09);
  // Scan limit (7 segments as we are using it in reverse)
  MAX7221Send(MAXREG_SCAN_LIMIT, 0x07);
  // Turn on chip/disable shutdown mode
  MAX7221Send(MAXREG_SHUTDOWN, 0x01);
  // display test is off
  MAX7221Send(MAXREG_DISPLAY_TEST, 0x00);
  
  return;
}

void MAX7221DisplayDigit(uint8_t digit, uint8_t value) {
  // since we are wired "in reverse", we don't update digits, but segments with the registers
  // all at once, so therefore a "digit" update on the MAX7221 is going to be actually a "segment"
  // update for all the different digits we want to update
  
  byte output;        // output to be sent to digit registers.
  byte each_segment;  // will be used as our "segments" counter.
  byte digits;        // will be used as our "digits" counter.
  byte digit_corrected; // digit remapping

  // remap digits as on hardware they are not in consecutive order
  switch(digit) {
    case 0:
      digit_corrected = 6;
      break;
    case 1:
      digit_corrected = 1;
      break;
    case 2:
      digit_corrected = 5;
      break;
    case 3:
      digit_corrected = 4;
      break;
    case 4:
      digit_corrected = 2;
      break;
    case 5:
      digit_corrected = 3;
      break;
  }
  // update storage table
  display_num[digit_corrected] = value;
  
  for (each_segment=1;each_segment<=8;each_segment++) {
    // build the byte to send, first we declare the value as empty.
    output = 0b00000000;

    // now for each of our 6 digits
    for (digits = 0; digits < 8; digits++) {
      byte number_to_display = display_num[digits];  
  
      // for out chosen number to display get it's segment bit at the position current position of 'each_segment'
      bool seg = number_seg[number_to_display] & (0b00000001 << (each_segment-1) );
 
      //set this bit in our output byte   
      output = output | (seg << digits);
      
     }
     MAX7221Send(each_segment, output);
  }
  return;
}

int8_t getSeconds(int32_t seconds) {
  return seconds%60;
}

int8_t getMinutes(int32_t seconds) {
  return (seconds/60)%60;
}

int8_t getHours(int32_t seconds) {
  return (seconds/3600)%24;
}

void updateTwoDigits(uint8_t digit1, uint8_t digit2, uint8_t value1, uint8_t value2) {
  MAX7221DisplayDigit(digit1,value1);
  MAX7221DisplayDigit(digit2,value2);
  return;
}

void updateTwoGroupedDigits(uint8_t digitStart, uint8_t value) {
  updateTwoDigits(digitStart,digitStart+1,value%10,value/10);

  return;
}

void ShowClock(int32_t seconds) {
  updateTwoGroupedDigits(0,getSeconds(seconds));
  updateTwoGroupedDigits(2,getMinutes(seconds));
  updateTwoGroupedDigits(4,getHours(seconds));
  return;
}

void ShowCountdown(int32_t countdown) {
  updateTwoGroupedDigits(1,getSeconds(abs(countdown)));
  updateTwoGroupedDigits(3,getMinutes(abs(countdown)));
  if(countdown<0) {
    MAX7221DisplayDigit(5,11);
  } else {
    MAX7221DisplayDigit(5, 10);
  }
  return;
}

void LEDStripDisplay(int32_t seconds) {
  strip.clear();
  if (cd&&seconds>0){
    strip.fill(red,60,12);
    strip.fill(red,0,seconds%60+1); 
  }
  else if (cd&&seconds==0){
    strip.fill(red,60,12);
    strip.fill(red,0,1);
  }
  else if (cd&&seconds<0){
      strip.fill(cyan,60,12);
      strip.fill(cyan,0,abs(seconds)%60+1);
  }
  if (ck){
    strip.fill(red,60,12);
    strip.fill(red,0,abs(seconds)%60+1);
  }
  strip.show();
  return;
}

void setup() {
  byte ctr = 0; // loop counter

  
  // Separator setup
  pinMode (SEPARATOR_1, OUTPUT);
  pinMode (SEPARATOR_2, OUTPUT);
  pinMode (SEPARATOR_3, OUTPUT);

  // MAX7221 setup
  pinMode(CS_MAX7221, OUTPUT); // chip select

  // Lora Module Setup
  pinMode(CS_LORA, OUTPUT);

  // Setup SPI
  SPI.begin();

  // Setup Serial (for debugging is necessary)
  Serial.begin(115200);

  MAX7221Setup();

  // fill with 6 numbers
  for(ctr = 0; ctr < 6; ctr++) {
    MAX7221DisplayDigit(ctr,11);
  }

  // Setup LEDs
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'  
  
  return;
}

void loop() {
  every(500) {
    separatorBlink = !separatorBlink;
    if(separatorBlink) {
      normaltime++;
      if(ck) {
       digitalWrite(SEPARATOR_1, HIGH);
       digitalWrite(SEPARATOR_3, HIGH);
       ShowClock(normaltime);
       LEDStripDisplay(normaltime);
      }
      if(cd) {
        digitalWrite(SEPARATOR_2, HIGH);
        countdown--;
        ShowCountdown(countdown);
        LEDStripDisplay(countdown);
      }
    } else {
      digitalWrite(SEPARATOR_1, LOW);
      digitalWrite(SEPARATOR_2, LOW);
      digitalWrite(SEPARATOR_3, LOW);
    }
  }
  return;
}
