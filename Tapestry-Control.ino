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
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
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
#define VERSION "V.0.14"
char NAME[10];

// give us an update every 5 minutes.
unsigned long update_time;

// Wireless config
/* create an instance of WiFiClientSecure */
// pull this data from a config file.
WiFiClientSecure espClient;
//change it with your ssid-password
char ssid[40];
char password[40];
boolean wifi_connect = false; // this is probably also stored in the library
boolean wifi_enable = false;
unsigned long wifi_time;

// mqtt setup
PubSubClient client(espClient);
// this is the MDNS name of PC where you installed MQTT Server
char serverHostname[40];
/* topics */
char MQTT_TOPIC[20];
char MQTT_USER[20];
char MQTT_PASS[20];
unsigned long lastMsg = 0;
char msg[20];
int counter = 0;
boolean mqtt_connect = false; // this is probably also stored in the library
boolean mqtt_enable = false;

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

// just helps to have a counter of these things.
unsigned int star_counter = 0;
unsigned int constellation_counter = 0;
unsigned int planet_counter = 0;
unsigned int animation_counter = 0;
int play_animation = -1;

// TFT screen
TFT_eSPI myGLCD = TFT_eSPI();       // Invoke custom library
#define DISPLAY_WIDTH 46
char display_strings[13][DISPLAY_WIDTH]; // This repesents our entire display screen.
char display_temp[DISPLAY_WIDTH];

// Bluetooth
char SERVICE_UUID[40]; // we load these configs from the file.
char CHARACTERISTIC_UUID[40];
char DATA_UUID[40];
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL; // main Characteristic.
BLECharacteristic* pCharacteristic2 = NULL; // this ble entry lets you query the unit.
byte bluetooth_connect = 0;
boolean bluetooth_enable = true;
uint32_t value = 0;

// Button to put bluetooth in to discovery mode.
const int buttonPin = 21;
unsigned long discovery_time = 0;
byte beacon_on = -1; // -1 = always off, 0 = off, 1 = on.

void assign_config(String name, String value) {
  if(name == "device_name")
    value.toCharArray(NAME, value.length()+1);
  else if(name == "wifi_enable") {
    if(value.toInt())
      wifi_enable=true;
    else
      wifi_enable=false;
  } else if(name == "wifi_ssid")
    value.toCharArray(ssid, value.length()+1);
  else if(name == "wifi_password")
    value.toCharArray(password, value.length()+1);
  else if(name == "mqtt_enable") {
    if(value.toInt())
      mqtt_enable=true;
    else
      mqtt_enable=false;
  } else if(name == "mqtt_host")
    value.toCharArray(serverHostname, value.length()+1);
  else if(name == "mqtt_topic")
    value.toCharArray(MQTT_TOPIC, value.length()+1);
  else if(name == "mqtt_user")
    value.toCharArray(MQTT_USER, value.length()+1);
  else if(name == "mqtt_pass")
    value.toCharArray(MQTT_PASS, value.length()+1);
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

  // finished loading all our files in to memory.  do we have any space free?
  display_print(F("Free Ram: "));
  display_print(String(ESP.getFreeHeap()));
  display_print(F(". Free PSRam: "));
  display_println(String(ESP.getFreePsram()));

  // setup wifi
  if(wifi_enable) {
    wifi_setup();

    display_print(F("Free Ram: "));
    display_print(String(ESP.getFreeHeap()));
    display_print(F(". Free PSRam: "));
    display_println(String(ESP.getFreePsram()));
  }
  
  //setup mqtt broker
  if(mqtt_enable && wifi_connect) {
    mqtt_setup();

    display_print(F("Free Ram: "));
    display_print(String(ESP.getFreeHeap()));
    display_print(F(". Free PSRam: "));
    display_println(String(ESP.getFreePsram()));
  }

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

  // only do mqtt stuff if wifi is enabled.
  if(mqtt_enable && wifi_connect) {
    /* if client was disconnected then try to reconnect again */
    if ( !client.connected()) {
      mqttconnect();
    }
  
    // send a "I'm alive" every X 
    if(millis() > 200000 && lastMsg < millis() - 200000) {
      char cstr[10];
      //client.publish(COUNTER_TOPIC,itoa(ESP.getFreeHeap(),cstr,10));
      client.publish(MQTT_TOPIC, NAME );
      display_println(String(ESP.getFreeHeap()));
      lastMsg = millis();
    }
  
    //display_println("test");
  
    // read from mqtt
    client.loop();

  }

  // pulse report every 5 minutes.
  if(millis() > 300000 && update_time < millis() - 300000) {
    // particularly during development, RAM needs to be monitored
    display_print(F("Free Ram: "));
    display_print(String(ESP.getFreeHeap()));
    display_print(F(". Free PSRam: "));
    display_println(String(ESP.getFreePsram()));
    
    update_time = millis();
  }

  // Check reconnections after 5 minutes.
  if(!wifi_connect && wifi_enable && millis() > 300000 && wifi_time < millis() - 300000) {
    // it's been 5 minutes.  Re-run setup.
    wifi_setup();
    wifi_time = millis();
  }

  // If it's been 5 minutes since showing a constellation.
  if(!screensaver && millis() > screensaver_timeout && screensaver_time < millis() - screensaver_timeout) {
    // switch to screensaver after 5 minutes.
    display_println(F("Screensaver active."));
    screensaver = 1;
    screensaver_time = millis();
  }
  
  //ChangePalettePeriodically();

  if(screensaver) {
    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */
    
    FillLEDsFromPaletteColors( startIndex);
  } else if(millis() > fade_timeout && fade_time < millis() - fade_timeout) {
    // Fade out the LED's over 1 minute.
    fadeOut();
  }

  //if(play_animation >= 0) {
    // we have an animation to play.
    EVERY_N_MILLISECONDS(10) {
      openAnimation();
    }
  //}

  // turn the becon off after 3 minutes.
  if(beacon_on == 1 && millis() > 180000 && discovery_time < millis() - 180000) {
    beacon_on = 0;
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->stop();
    display_println(F("Bluetooth Hidden."));
    display_header(); // need to update the colour.
    
  }
  
  read_button();
    
    //leds[0][10] = CRGB::Red;
    
    FastLED.show();
    FastLED.delay(1000 / UPDATES_PER_SECOND);
}
