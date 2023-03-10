/*
File: main.cpp
Tauno Erik
10.03.2023
https://taunoerik.art/

Hardware:
  Raspberry Pi Pico
  LD2410 Radar Sensor
  WS2812B RGB LED strip

Pins:
Pico GPIO0 (TX) -> Radar RX
Pico GPIO1 (RX) -> Radar TX
Pico VBUS (5V)  -> Radar VCC
Pico GND        -> Radar GND
*/

#include <Arduino.h>
#include "LD2410.h"             // https://github.com/Renstec/LD2410/
#include <Adafruit_NeoPixel.h>  // https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest_nodelay/strandtest_nodelay.ino

#define RADAR_BAUD 256000
#define USB_BAUD   115200

// Pins:
//const uint8_t RADAR_RX_PIN = 1;  // Allready default RX pin
//const uint8_t RADAR_TX_PIN = 0;  // Allready default TX pin
const int LED_1_PIN = 21;
const int LED_2_PIN = 20;
const int LED_3_PIN = 19;
const int LED_4_PIN = 18;
const int RGB_PIN   = 15;

// RGB
const int NUM_OF_RGBS = 80;

int rgb_R = 0;
int rgb_G = 0;
int rgb_B = 0;

int      pixel_interval = 50;  // Pixel Interval (ms)
uint16_t pixel_current = 0;    // Pattern Current Pixel Number
int      pixel_cycle = 0;      // Pattern Pixel Cycle

Adafruit_NeoPixel RGB_strip(NUM_OF_RGBS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// Time
bool is_movement = false;
bool is_change_color = false;

const int OFF_TIME = 20000;
int time_px_prev = 0;  // Millis
int px_interval = 50;  // Millis
int no_movment_time = 0;

// LEDS
const int num_of_leds = 4;
int leds[num_of_leds] = {
  LED_1_PIN, LED_2_PIN, LED_3_PIN, LED_4_PIN
};

// Radar
LD2410 radar(Serial1);

// True enables Radar Enginering Mode
bool is_radar_eng_bode = false;

// RGB Funcs


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


// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void colorWipe(uint32_t color, int wait) {

  if(pixel_interval != wait) {
    pixel_interval = wait;                   //  Update delay time
  }

  RGB_strip.setPixelColor(pixel_current, color); //  Set pixel's color (in RAM)
  RGB_strip.show();
  pixel_current++;

  if(pixel_current >= NUM_OF_RGBS) {
    pixel_current = 0;                               //  Loop the pattern from the first LED
  }

}
 
 
// Rainbow cycle along whole pixels. Pass delay time (in ms) between frames.
// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(uint8_t wait) {

  if(pixel_interval != wait) {
    pixel_interval = wait;
  }

  for(uint16_t i=0; i < NUM_OF_RGBS; i++) {
    RGB_strip.setPixelColor(i, Wheel((i + pixel_cycle) & 255)); //  Update delay time
  }

  RGB_strip.show();                          //  Update strip to match
  pixel_cycle++;                             //  Advance current cycle

  if(pixel_cycle >= 256) {
    pixel_cycle = 0;                         //  Loop the cycle back to the begining
  }
}


void setup() {
  Serial.begin(USB_BAUD);
  delay(1000);
  Serial.println("LD2410-Radar Example");
  delay(1000);

  Serial1.begin(RADAR_BAUD);  // UART1 to radar
  delay(1000);

  for(int i = 0; i < num_of_leds; i++) {
    pinMode(leds[i], OUTPUT);
  }

  // Restore radar default values
  //bool is_radar_factory_reset = radar.factoryReset();

  // Change radar baud rate:
  // BAUD_9600, BAUD_19200, BAUD_38400, BAUD_57600,
  // BAUD_115200, BAUD_230400, BAUD_256000, BAUD_460800
  //bool is_radar_baud = radar.setBaudRate(BAUD_256000);

  //radar.setGateSensConf();

  // Restart radar
  //bool is_radar_restart = radar.restart();


  if (radar.begin()) {
    Serial.print("Radar Firmware Version ");
    Serial.print(radar.firmwareVersion.majorVersion);
    Serial.print(".");
    Serial.print(radar.firmwareVersion.minorVersion);
    Serial.print(".");
    Serial.println(radar.firmwareVersion.bugFixVersion);

    Serial.print("Radar detection time: ");
    Serial.println(radar.parameter.detectionTime);

    Serial.print("Radar max detection Gate: ");
    Serial.println(radar.parameter.maxGate);

    Serial.print("Radar max moving Gate: ");
    Serial.println(radar.parameter.maxMovingGate);

    Serial.print("Radar max stationary Gate: ");
    Serial.println(radar.parameter.maxStationaryGate);

    Serial.println("Treshold/energy values per Gate:");

    for (uint8_t gate = 0; gate <= 8; gate++) {
      Serial.print("Gate ");
      Serial.print(gate);
      Serial.print(": moving treshold:");
      Serial.print(radar.parameter.movingSensitivity[gate]);
      Serial.print(", stationary treshold:");
      Serial.println(radar.parameter.stationarySensitivity[gate]);
    }

  } else {
    Serial.println("Failed to get firmware version and parameters from radar.");
  }

  Serial.println();

  radar.enableEngMode(is_radar_eng_bode);  // If true enables

  // 
  RGB_strip.begin();
  RGB_strip.show();            // Turn OFF all pixels ASAP
  RGB_strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
 
  randomSeed(analogRead(0));
  rainbow(3);
  colorWipe(RGB_strip.Color(0, 0, 0), 0);

}

void loop() {
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
  
  // read must be called cyclically
  if (radar.read()) {
    // Cyclic radar data
    Serial.println();

    Serial.print("Target state: ");
    Serial.print(radar.cyclicData.targetState);

    switch (radar.cyclicData.targetState) {
      case 0x00:
        Serial.println(" no target detected");
        break;
      case 0x01:
        Serial.println(" moving target detected");
        break;
      case 0x02:
        Serial.println(" stationary target detected");
        break;
      case 0x03:
        Serial.println(" moving and stationary target detected");
        break;
    }

    for (int i = 0; i < num_of_leds; i++) {
      if (i == radar.cyclicData.targetState) {
        Serial.println("High");
        digitalWrite(leds[i], HIGH);
      } else {
        Serial.println("Low");
        digitalWrite(leds[i], LOW);
      }
    }


    Serial.print("Moving taget distance in cm: ");
    Serial.println(radar.cyclicData.movingTargetDistance);

    Serial.print("Moving target energy valu 0-100%: ");
    Serial.println(radar.cyclicData.movingTargetEnergy);

    Serial.print("Statsionary target distance in cm: ");
    Serial.println(radar.cyclicData.stationaryTargetDistance);

    Serial.print("Statsionary target energy valu 0-100%: ");
    Serial.println(radar.cyclicData.stationaryTargetEnergy);

    Serial.print("Detection distance in cm: ");
    Serial.println(radar.cyclicData.detectionDistance);

    // Engineering Mode data
    if (radar.cyclicData.radarInEngineeringMode) {
      Serial.println("--Radar is in Engineering Mode--");

      Serial.print("Max moving distance: ");
      Serial.println(radar.engineeringData.maxMovingGate);

      Serial.print("Max statsionary distance: ");
      Serial.println(radar.engineeringData.maxStationaryGate);

      Serial.print("Max moving energy: ");
      Serial.println(radar.engineeringData.maxMovingEnergy);

      Serial.print("Max statsionary energy: ");
      Serial.println(radar.engineeringData.maxStationaryEnergy);

      Serial.println("Energy per Gate:");
      Serial.print("Moving: ");
      for (uint8_t gate = 0; gate <= 8; gate++) {
        Serial.print(radar.engineeringData.movingEnergyGateN[gate]);
        Serial.print(" ");
      }
      Serial.println();
      Serial.print("Statsionary: ");
      for (uint8_t gate = 0; gate <= 8; gate++) {
        Serial.print(radar.engineeringData.stationaryEnergyGateN[gate]);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}