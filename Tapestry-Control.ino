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

#include "datastructure.h"

#define FORMAT_SPIFFS_IF_FAILED true

#define NUM_STRIPS 8
#define NUM_LEDS_PER_STRIP 200
#define NUM_LEDS NUM_LEDS_PER_STRIP * NUM_STRIPS
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
byte BRIGHTNESS = 64; // 0-255.  This is editable on the fly

// Set our version number.  Don't forget to update when featureset changes
#define PROJECT "Tapestry-Control"
#define VERSION "V.0.26"
#define DEBUG 1
char NAME[10];

// give us an update every 5 minutes.
unsigned long update_time;

// Setup for our LEDs
CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];

#define UPDATES_PER_SECOND 100
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

unsigned long screensaver_time = 0;
unsigned long screensaver_timeout = 90000;
byte screensaver = 1;
unsigned long fade_time;
unsigned long fade_timeout = 60000; // when to begin fadeout.
int fadeAmount = 1;  // Set the amount to fade I usually do 5, 10, 15, 20, 25 etc even up to 255.
long random_screensaver = -1;

// just helps to have a counter of these things.
unsigned int star_counter = 0;
unsigned int constellation_counter = 0;
unsigned int planet_counter = 0;
unsigned int animation_counter = 0;
unsigned int last_animation_counter[20]; //keep a record of which device has which animation. 20 is widly optimistic.

// TFT screen
TFT_eSPI myGLCD = TFT_eSPI();       // Invoke custom library
#define DISPLAY_WIDTH 50
char display_strings[14][DISPLAY_WIDTH]; // This repesents our entire display screen.
char display_temp[DISPLAY_WIDTH];

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

// Button to put bluetooth in to discovery mode.
const int buttonPin = 21;
unsigned long discovery_time = 0;
byte beacon_on = -1; // -1 = always off, 0 = off, 1 = on.

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
  display_print(F("Free Ram: "));
  display_print(String(ESP.getFreeHeap()));
  display_print(F(". Free PSRam: "));
  display_println(String(ESP.getFreePsram()));

  // run through our LED setup routine.
  // needs to happen before loading our stars to ensure the LED pointers are available.
  leds_setup();

  display_print(F("Free Ram: "));
  display_print(String(ESP.getFreeHeap()));
  display_print(F(". Free PSRam: "));
  display_println(String(ESP.getFreePsram()));

  setup_animations();
  
  display_print(F("Free Ram: "));
  display_print(String(ESP.getFreeHeap()));
  display_print(F(". Free PSRam: "));
  display_println(String(ESP.getFreePsram()));

  setup_fileops();

  listDir(SPIFFS, "/", 0);
  loadConfig(SPIFFS, "/config.ini"); // load our configuration file.  If this fails, everything fails.
  display_header();

  loadConstellations(SPIFFS, "/const.csv");
  loadStars(SPIFFS, "/stars.csv");
  //loadAnimation(SPIFFS, "/ani.csv");
  loadFortune(SPIFFS, "/fortune.csv");

  // finished loading all our files in to memory.  do we have any space free?
  display_print(F("Free Ram: "));
  display_print(String(ESP.getFreeHeap()));
  display_print(F(". Free PSRam: "));
  display_println(String(ESP.getFreePsram()));

  // setup bluetooth
  if(bluetooth_enable) {
    bluetooth_setup();

    display_print(F("Free Ram: "));
    display_print(String(ESP.getFreeHeap()));
    display_print(F(". Free PSRam: "));
    display_println(String(ESP.getFreePsram()));
  }

  button_setup();

  // this is a test for more than 40 characters
  //display_print("1234567890123456789012345678901234567890111");

}

void loop() {

  // pulse report every 5 minutes.
  if(DEBUG && millis() > 300000 && update_time < millis() - 300000) {
    // particularly during development, RAM needs to be monitored
    display_print(F("Free Ram: "));
    display_print(String(ESP.getFreeHeap()));
    display_print(F(". Free PSRam: "));
    display_println(String(ESP.getFreePsram()));
    
    update_time = millis();
  }

  // If it's been 1.5 minutes since showing a constellation.
  if(!screensaver && millis() > screensaver_timeout && screensaver_time < millis() - screensaver_timeout) {
    // switch to screensaver after 1.5 minutes.
    display_println(F("Screensaver active."));
    screensaver = 1;
    screensaver_time = millis();
  }

  if(screensaver) {

    // pick a ransom screensaver.
    // if it's 0, run for a minimum of 1 minute.

    if(random_screensaver == -1) {
      random_screensaver = random(0, animation_counter);
      screensaver_time = millis();
      //if(DEBUG) {
      //  display_println("Starting Screensaver: " + random_screensaver);
        //display_println(random_screensaver);
      //}
    }

    if(random_screensaver == 0 || animation_array[random_screensaver-1].count == 0) {
      // Special case, display the splashing blue lights.
      static uint8_t startIndex = 0;
      startIndex = startIndex + 1; /* motion speed */
      FillLEDsFromPaletteColors( startIndex);

      // runs for a minute.  a bit long?
      if(screensaver_time < millis() - 60000) {
        random_screensaver = -1;
        FastLED.clear ();
      }

    } else if (random_screensaver > 0 && screensaver_time < millis() - 30000 && active_array[0].count < 10) {
        // every 30 seconds, add new random animation to the display.
        // 30 seconds might be to fast so we also check how many active stars we are showing.
        // need to offset the random screensaver
        activateAnimation(random_screensaver-1);

        random_screensaver = -1;
    }
  }
  
  if(!screensaver || random_screensaver != 0 ) {
    EVERY_N_MILLISECONDS(30) {
      openAnimation();
    }
  }
  
  // turn the beacon off after 3 minutes.
  if(beacon_on == 1 && millis() > 180000 && discovery_time < millis() - 180000) {
    beacon_on = 0;
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->stop();
    display_println(F("Bluetooth Hidden."));
    display_header(); // need to update the colour.
    
  }
  
  read_button();
  
  FastLED.show();
}
