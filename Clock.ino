#include <Adafruit_NeoPixel.h>
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    9

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 72

int seconds =7;
bool cd=false;
bool ck=true;


// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);
uint32_t red=strip.Color(255,0,0);
uint32_t cyan=strip.Color(0,255,255);

// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

void setup() {
  
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
//  strip.fill(red,0,LED_COUNT);
//  strip.show();
}

void loop() {

  if (cd&&seconds>0){
      strip.clear();
      strip.fill(red,60,12);
      strip.fill(red,0,seconds%60+1);
      strip.show();
      delay(1000);
      seconds=seconds-1;
    }
    else if (cd&&seconds==0){
      strip.clear();
      strip.fill(red,60,12);
      strip.fill(red,0,1);
      strip.show();
      delay(1000);
      seconds=seconds-1;
    }
    else if (cd&&seconds<0){
      strip.clear();
      strip.fill(cyan,60,12);
      strip.fill(cyan,0,abs(seconds)%60+1);
      strip.show();
      delay(1000);
      seconds=seconds-1;
    }

  if (ck){
    strip.clear();
    strip.fill(red,60,12);
    strip.fill(red,0,abs(seconds)%60+1);
    strip.show();
    delay(1000);
    seconds=seconds+1; 
  }


}



