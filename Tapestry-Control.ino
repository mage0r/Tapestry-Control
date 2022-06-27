/* Tapestry Panel Controller
 * John and Sarah Spencer 2021
 * 
 * This Arduino sketch is designed to run on a TinyPico ESP32 board.
 * Ensure the settings allow for:
 * PSRAM enabled.
 * 3MB program space with 1MB SPIFFS.
 * 
 * The entire purpose of this board is to be a gateway to drive the LED behind the tapestry.
 * This sketch can either be driven by wifi using MQTT or by directly connecting to bluetooth with the custom Android APP.
 * 
 * In order to keep functions somewhat separate, each subsystem has it's own file.
 * 
 * This was originally built on Arduino 1.8.16
 * 
 */
#include "FS.h"
#include "SPIFFS.h"
#include <FastLED.h>
#include "SPI.h"
#include "TFT_eSPI.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include "esp32-hal-psram.h" // do not forget this.

#define FORMAT_SPIFFS_IF_FAILED true

#define NUM_STRIPS 8
#define NUM_LEDS_PER_STRIP 200
#define NUM_LEDS NUM_LEDS_PER_STRIP * NUM_STRIPS
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
byte BRIGHTNESS = 150; // 0-255.  This is editable on the fly

// Define some of our array sizes.
#define MAX_ACTIVE_STARS 8000
#define MAX_SESSION_STARS 2000
#define MAX_SESSIONS 50
#define MAX_BUFFER 6100

// Set our version number.  Don't forget to update when featureset changes
#define PROJECT "Tapestry-Control"
#define VERSION "V.1.32"
#define DEBUG 1
char NAME[10];



// give us an update every 5 minutes.
unsigned long update_time;
unsigned long save_animations_time;

// Setup for our LEDs
CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];

#define UPDATES_PER_SECOND 100
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

unsigned long screensaver_time = 0;
unsigned long screensaver_timeout = 60000;
byte screensaver = 1;
long random_screensaver = -1;
unsigned long tigger_time = 0;
unsigned long trigger_reset = 1800000;

// just helps to have a counter of these things.
unsigned int star_counter = 0;
unsigned int constellation_counter = 0;
unsigned int planet_counter = 0;
unsigned int animation_counter = 0;
unsigned int max_animation_counter = 0; // need an extra counter for when we max out the array
int last_animation_counter[20]; //keep a record of which device has which animation. 20 is widly optimistic.
int save_counter = 0; // because we nibble away at the save, we need to keep track of how far in to that process we are.

// TFT screen
TFT_eSPI myGLCD = TFT_eSPI();       // Invoke custom library
#define DISPLAY_WIDTH 60
char display_strings[14][DISPLAY_WIDTH+1]; // This repesents our entire display screen.
char display_temp[DISPLAY_WIDTH+1];

// Bluetooth
char SERVICE_UUID[40]; // we load these configs from the file.
char CHARACTERISTIC_UUID[40];
char DATA_UUID[40];
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL; // main Characteristic.
BLECharacteristic* pCharacteristic2 = NULL; // this ble entry lets you query the unit.
int bluetooth_connect = 0;
boolean bluetooth_enable = true;
uint32_t value = 0;
int command_count = 0;
int processed_command_count = 0;
char display_buffer[10][40];
int display_buffer_count = 0;

// Button to put bluetooth in to discovery mode.
const int buttonPin = 21;
unsigned long discovery_time = 0;
byte beacon_on = -1; // -1 = always off, 0 = off, 1 = on.

// interestingly, if this isn't defined here the datastructures don't load properly.
#include "datastructure.h"

void assign_config(String name, String value) {
  if(name == "device_name")
    value.toCharArray(NAME, value.length()+1);
  else if(name == "bluetooth_enable") {
    if(value.toInt())
      bluetooth_enable=true;
    else
      bluetooth_enable=false;
  } else if(name == "bluetooth_serv_UUID") {
    value.toCharArray(SERVICE_UUID, value.length()+1);
    // do some testing.  If this value is empty the device will segfault.
    if(value.length() < 1)
      bluetooth_enable=false;
  } else if(name == "bluetooth_char_UUID") {
    value.toCharArray(CHARACTERISTIC_UUID, value.length()+1);
    if(value.length() < 1)
      bluetooth_enable=false;
  } else if(name == "bluetooth_data_UUID") {
    value.toCharArray(DATA_UUID, value.length()+1);
    if(value.length() < 1)
      bluetooth_enable=false;
  }

}

void setup() {

  Serial.begin(115200);

  // Setup display first so we can output boot information.
  setup_display();

  // First, build information
  display_println(PROJECT " " VERSION);
  display_println(F("Build Date: " __DATE__ " " __TIME__));
  String temp_text = "Free Ram: ";
  temp_text += ESP.getFreeHeap();
  temp_text += ". Free PSRam: ";
  temp_text += ESP.getFreePsram();
  display_println(temp_text);

  // run through our LED setup routine.
  // needs to happen before loading our stars to ensure the LED pointers are available.
  leds_setup();

  setup_animations();
  
  setup_fileops();

  if(DEBUG)
    listDir(SPIFFS, "/", 0); // only really care about this one for debug.
  loadConfig(SPIFFS, "/config.ini"); // load our configuration file.  If this fails, everything fails.
  display_header();

  loadConstellations(SPIFFS, "/const.csv");
  loadStars(SPIFFS, "/stars.csv");
  loadSession(SPIFFS, "/animations.csv");
  loadFortune(SPIFFS, "/fortune.csv");

  // setup bluetooth
  if(bluetooth_enable) {
    bluetooth_setup();

    if(DEBUG) {
      String temp_text = "Free Ram: ";
      temp_text += ESP.getFreeHeap();
      temp_text += ". Free PSRam: ";
      temp_text += ESP.getFreePsram();
      display_println(temp_text);
    }
  }

  // found this needs to be initalised to 0
  for(int i = 0; i < 20; i++) {
    last_animation_counter[i] = -1;
  }

}

void loop() {

  // check if there's anything in the command buffer.
  //if(command_count > 0)
  //EVERY_N_MILLISECONDS(30)
  if(processed_command_count < command_count)
    command_processing();

  // There's some display buffer stuff the bluetooth system can't send until it's clear.
  if(display_buffer_count > 0) {
    char temp_display_buffer[10][40];
    int temp_display_buffer_count = display_buffer_count;
    for(int i = 0; i < temp_display_buffer_count; i++) {
      strcpy(temp_display_buffer[i],display_buffer[i]);
    }
    display_buffer_count = 0; // copied, lets go back to zero.
    while(temp_display_buffer_count) {
      temp_display_buffer_count--;
      display_println(String(temp_display_buffer[temp_display_buffer_count]));
    }
  }
  

  // pulse report every 5 minutes.
  if( millis() > 300000 && update_time < millis() - 300000) {
    // particularly during development, RAM needs to be monitored
    Serial.print(F("Free Ram: "));
    Serial.print(String(ESP.getFreeHeap()));
    Serial.print(F(". Free PSRam: "));
    Serial.println(String(ESP.getFreePsram()));
    
    update_time = millis();
  }

  // If it's been 1.5 minutes since showing any sessions.
  if(screensaver == 0 && millis() > screensaver_timeout && screensaver_time < millis() - screensaver_timeout && active_array[0].count < 1) {
    // switch to screensaver after 1 minute.
    if(DEBUG)
      display_println(F("Getting Sleepy...."));
    screensaver = 1;
    screensaver_time = millis();
    tigger_time = millis(); // reset the trigger time every time we go in to screensaver mode
  }

  if(screensaver == 1 && max_animation_counter > 2) {
    // if we don't wait a few seconds when we start it goes a bit haywire
    // pick a ransom screensaver.
    // if it's 0, run for a minimum of 1 minute.

    if(random_screensaver == -1) {
      random_screensaver = random(0, max_animation_counter-1); // max animation is always one ahead.
      screensaver_time = millis();

      if(DEBUG && animation_array[random_screensaver].count > 0) {
        String temp_text = "Dreaming about....";
        temp_text += random_screensaver+1;
        temp_text += "/";
        temp_text += max_animation_counter;
        display_println(temp_text);
      }
    }

    //if (screensaver_time < millis() - 30000 && active_array[0].count < 10) {
    if (active_array[0].count < 10) {
      // every 30 seconds, add new random animation to the display.
      // 30 seconds might be to fast so we also check how many active stars we are showing.
      // need to offset the random screensaver

      activateAnimation(random_screensaver);

      screensaver_time = millis();

      random_screensaver = -1;
    }

    EVERY_N_MILLISECONDS(30) {
      openAnimation();
    }

  } else if (screensaver == 2) {

    // special case, just run the raindrops/twinkling.
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    FillLEDsFromPaletteColors(startIndex);

  } else {
   EVERY_N_MILLISECONDS(30) {
      openAnimation();
    }
  }

  // If the screensaver has been running for 30 minutes, there's a chance
  // the system has gone in to a failed state where it's not accepting connections.
  // The restart on this system is pretty quick.  Just boot it and get it over with.
  /*
  if(millis() > trigger_reset && tigger_time < millis() - trigger_reset) {
    display_println("Pre-emptive restart!");
    ESP.restart();
  }
  */

  
  if(millis() > 60000 && save_animations_time < millis() - 60000 && max_animation_counter > 0) {
    // save every minute.
    // probably don't want to save if it's empty.
    saveSession(SPIFFS, "/animations.csv");
    
  }

  FastLED.show();
}
