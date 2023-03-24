/*
File: main.cpp
Tauno Erik
23.03.2023
https://taunoerik.art/

Hardware:
  Raspberry Pi Pico
  LD2410 Radar Sensor
  WS2812B RGB LED strip, 60 LED/m, 0.8 m ringis

Pins:
Pico GPIO0 (TX) -> Radar RX
Pico GPIO1 (RX) -> Radar TX
Pico VBUS (5V)  -> Radar VCC
Pico GND        -> Radar GND
*/

#include <Arduino.h>
#include "LD2410.h"             // https://github.com/Renstec/LD2410/
#include <Adafruit_NeoPixel.h>  // https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest_nodelay/strandtest_nodelay.ino

//#define DEBUG
#include "utils_debug.h"

#define RADAR_BAUD 256000
#define USB_BAUD   115200

#define TO_RADAR_RESET 0  // 0 no, 1 yes

// Ring 49 LEDs, Ruut 59 LEDS
#define RING 49
#define RUUT 59
#define KUJU RUUT

#define FORWARD    0
#define BACKWARD   1
#define RGB_DELAY 70  // smaller = faster

// Pins:
//const int RADAR_RX_PIN = 4;  // Pico default TX pin is GP0
//const int RADAR_TX_PIN = 5;  // Pico default RX pin is GP1
const int RADAR_OUT_PIN = 2;  // GP2
const int RGB_IN_PIN   = 22;  // GP22

const int RANDOM_SEED_ANALOG_PIN = 26;  // GP26

const int NUM_OF_LEDS = KUJU;
const int BRIGHTNESS = 75;  // 0-255

int rgb_R = 0;
int rgb_G = 0;
int rgb_B = 0;

// Kasutaud funksioonides
int      pixel_interval = 50;  // ms
uint16_t pixel_current = 0;    // Pattern Current Pixel Number
int      pixel_cycle = 0;      // Pattern Pixel Cycle

// Init RGB strip
Adafruit_NeoPixel RGB_strip(NUM_OF_LEDS, RGB_IN_PIN, NEO_GRB + NEO_KHZ800);

// Init Radar
LD2410 radar(Serial1);

bool is_radar_eng_mode = false;  // True enables Radar Enginering Mode

bool is_first_loop = true;

// Input: 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t wheel_pos) {

  wheel_pos = 255 - wheel_pos;

  if(wheel_pos < 85) {
    return RGB_strip.Color(255 - wheel_pos * 3, 0, wheel_pos * 3);
  }
  if(wheel_pos < 170) {
    wheel_pos -= 85;
    return RGB_strip.Color(0, wheel_pos * 3, 255 - wheel_pos * 3);
  }

  wheel_pos -= 170;

  return RGB_strip.Color(wheel_pos * 3, 255 - wheel_pos * 3, 0);
}


// Fill strip pixels one after another with a color.
// Strip is NOT cleared; anything there will be covered pixel by pixel. 
// color is 32-bit value, which you can get by calling
// strip.Color(red, green, blue)
void Rgb_color_wipe(uint32_t color, int wait, int dir = 0) {
  if (dir == FORWARD) {
    if(pixel_interval != wait) {
      pixel_interval = wait;                   //  Update delay time
    }

    RGB_strip.setPixelColor(pixel_current, color); //  Set pixel's color (in RAM)
    RGB_strip.show();
    pixel_current++;

    if(pixel_current >= NUM_OF_LEDS) {
      pixel_current = 0;                               //  Loop the pattern from the first LED
    }
  }
}

void Rgb_color_wipe_delay(uint32_t color, int wait, int dir = 0) {
  if (dir == FORWARD) {
    for(int i=0; i<RGB_strip.numPixels(); i++) {
      RGB_strip.setPixelColor(i, color);
      RGB_strip.show();
      delay(wait);
    }
  } else {
    // Backward
    for(int i=RGB_strip.numPixels(); i>=0; i--) {
      RGB_strip.setPixelColor(i, color);
      RGB_strip.show();
      delay(wait);
    }
  }
  
}
 
 
// Rainbow cycle along whole pixels.
// one by one
void Rainbow(uint8_t wait) {

  if(pixel_interval != wait) {
    pixel_interval = wait;
  }

  for(uint16_t i=0; i < NUM_OF_LEDS; i++) {
    RGB_strip.setPixelColor(i, Wheel((i + pixel_cycle) & 255)); //  Update delay time
  }

  RGB_strip.show();
  pixel_cycle++;            //  Advance current cycle

  if(pixel_cycle >= 256) {
    pixel_cycle = 0;        //  Loop the cycle back to the begining
  }
}




void setup() {
  // Start USB serial print
  Serial.begin(USB_BAUD);
  delay(1000);

  // Start UART to RADAR
  Serial1.begin(RADAR_BAUD);
  delay(1000);

  DEBUG_PRINTLN("Art Light started!");
  delay(1000);

  if (TO_RADAR_RESET) {
    // Restore radar default values
    bool is_radar_factory_reset = radar.factoryReset();

    if (is_radar_factory_reset) {
      DEBUG_PRINTLN("Factory reset was successful");
    } else {
      DEBUG_PRINTLN("Can't do factory reset");
    }

    // Change radar baud rate:
    // BAUD_9600, BAUD_19200, BAUD_38400, BAUD_57600,
    // BAUD_115200, BAUD_230400, BAUD_256000, BAUD_460800
    bool is_radar_baud = radar.setBaudRate(BAUD_256000);
    if (is_radar_baud) {
      DEBUG_PRINTLN("Radar baud rate is 256000");
    } else {
      DEBUG_PRINTLN("Can't cange radar baud rate");
    }
    //radar.setGateSensConf();
    // Restart radar
    bool is_radar_restart = radar.restart();
  }

  Serial.print("Connecting to radar .");
  while (!radar.begin()) {
    DEBUG_PRINT(".");
    delay(500);
  }

  if (radar.begin()) {
    DEBUG_PRINT("Radar Firmware Version ");
    DEBUG_PRINT(radar.firmwareVersion.majorVersion);
    DEBUG_PRINT(".");
    DEBUG_PRINT(radar.firmwareVersion.minorVersion);
    DEBUG_PRINT(".");
    DEBUG_PRINTLN(radar.firmwareVersion.bugFixVersion);

    DEBUG_PRINT("Radar detection time: ");
    DEBUG_PRINTLN(radar.parameter.detectionTime);

    DEBUG_PRINT("Radar max detection Gate: ");
    DEBUG_PRINTLN(radar.parameter.maxGate);

    DEBUG_PRINT("Radar max moving Gate: ");
    DEBUG_PRINTLN(radar.parameter.maxMovingGate);

    DEBUG_PRINT("Radar max stationary Gate: ");
    DEBUG_PRINTLN(radar.parameter.maxStationaryGate);

    DEBUG_PRINTLN("Treshold/energy values per Gate:");

    for (uint8_t gate = 0; gate <= 8; gate++) {
      DEBUG_PRINT("Gate ");
      DEBUG_PRINT(gate);
      DEBUG_PRINT(": moving treshold:");
      DEBUG_PRINT(radar.parameter.movingSensitivity[gate]);
      DEBUG_PRINT(", stationary treshold:");
      DEBUG_PRINTLN(radar.parameter.stationarySensitivity[gate]);
    }

  } else {
    DEBUG_PRINTLN("Failed to get firmware version and parameters from radar.");
  }

  // Enable or disable Radar Engineering mode
  radar.enableEngMode(is_radar_eng_mode);

  // RGB strip
  
  RGB_strip.begin();
  RGB_strip.show();  // Turn OFF all pixels ASAP
  RGB_strip.setBrightness(BRIGHTNESS);
 
  randomSeed(analogRead(RANDOM_SEED_ANALOG_PIN));
  
  DEBUG_PRINTLN("Setup finished!");
}

void loop() {
  if (is_first_loop) {
    is_first_loop = false;
  }
  /*
  RGB_strip.clear(); // Set all pixel colors to 'off'
  rainbow(10); // Flowing rainbow cycle along the whole strip
  colorWipe(RGB_strip.Color(255, 0, 0), 50);  // Red
  colorWipe(RGB_strip.Color(0, 255, 0), 50);  // Green
  colorWipe(RGB_strip.Color(0, 0, 255), 50);  // Blue
  RGB_strip.clear(); // Set all pixel colors to 'off'

  RGB_strip.clear(); // Set all pixel colors to 'off'
  RGB_strip.setPixelColor(0, RGB_strip.Color(0, 150, 0));
  RGB_strip.setPixelColor(1, RGB_strip.Color(150, 0, 0));
  RGB_strip.setPixelColor(2, RGB_strip.Color(0, 0, 150));
  RGB_strip.show();   // Send the updated pixel colors to the hardware.
  */
  
  // read must be called cyclically
  if (radar.read()) {
    // Cyclic radar data
    DEBUG_PRINT("\nTarget state: ");
    DEBUG_PRINT(radar.cyclicData.targetState);

    switch (radar.cyclicData.targetState) {
      case 0x00:
        DEBUG_PRINTLN(" no target detected");
        Rgb_color_wipe(RGB_strip.Color(0, 0, 0), RGB_DELAY); // LEDs off
        break;
      case 0x01:
        DEBUG_PRINTLN(" moving target detected");
        rgb_R = random(0, 255);
        rgb_G = random(0, 255);
        rgb_B = random(0, 255);
        Rgb_color_wipe_delay(RGB_strip.Color(rgb_R, rgb_G, rgb_B), RGB_DELAY, BACKWARD);
        //Rainbow(RGB_DELAY);
        break;
      case 0x02:
        DEBUG_PRINTLN(" stationary target detected");
        Rgb_color_wipe(RGB_strip.Color(rgb_R, rgb_G, rgb_B), RGB_DELAY);
        //Rainbow(RGB_DELAY);
        break;
      case 0x03:
        DEBUG_PRINTLN(" moving and stationary target detected");
        rgb_R = random(0, 255);
        rgb_G = random(0, 255);
        rgb_B = random(0, 255);
        Rgb_color_wipe_delay(RGB_strip.Color(rgb_R, rgb_G, rgb_B), RGB_DELAY, BACKWARD);
        // Rainbow(RGB_DELAY);
        break;
    }

    DEBUG_PRINT("Moving taget distance in cm: ");
    DEBUG_PRINTLN(radar.cyclicData.movingTargetDistance);

    DEBUG_PRINT("Moving target energy valu 0-100%: ");
    DEBUG_PRINTLN(radar.cyclicData.movingTargetEnergy);

    DEBUG_PRINT("Statsionary target distance in cm: ");
    DEBUG_PRINTLN(radar.cyclicData.stationaryTargetDistance);

    DEBUG_PRINT("Statsionary target energy valu 0-100%: ");
    DEBUG_PRINTLN(radar.cyclicData.stationaryTargetEnergy);

    DEBUG_PRINT("Detection distance in cm: ");
    DEBUG_PRINTLN(radar.cyclicData.detectionDistance);

    // Engineering Mode data
    if (radar.cyclicData.radarInEngineeringMode) {
      DEBUG_PRINTLN("--Radar is in Engineering Mode--");

      DEBUG_PRINT("Max moving distance: ");
      DEBUG_PRINTLN(radar.engineeringData.maxMovingGate);

      DEBUG_PRINT("Max statsionary distance: ");
      DEBUG_PRINTLN(radar.engineeringData.maxStationaryGate);

      DEBUG_PRINT("Max moving energy: ");
      DEBUG_PRINTLN(radar.engineeringData.maxMovingEnergy);

      DEBUG_PRINT("Max statsionary energy: ");
      DEBUG_PRINTLN(radar.engineeringData.maxStationaryEnergy);

      DEBUG_PRINTLN("Energy per Gate:");
      DEBUG_PRINT("Moving: ");
      for (uint8_t gate = 0; gate <= 8; gate++) {
        DEBUG_PRINT(radar.engineeringData.movingEnergyGateN[gate]);
        DEBUG_PRINT(" ");
      }
      DEBUG_PRINTLN();
      DEBUG_PRINT("Statsionary: ");
      for (uint8_t gate = 0; gate <= 8; gate++) {
        DEBUG_PRINT(radar.engineeringData.stationaryEnergyGateN[gate]);
        DEBUG_PRINT(" ");
      }
      DEBUG_PRINTLN();
    }
  } else {
    //Serial.println("No radar data.");
  }
}