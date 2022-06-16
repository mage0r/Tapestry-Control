class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {

      // ble can only have 20 characters.
      // what does that look like.
      // <DeviceID>c<256><256><256>2                                    // Lowercase c for Constellation number.
      // <DeviceID>s<256><256><256><high byte><low byte>                // Lowercase s for Star number.  Not a byte.
      // <DeviceID>p<256><256><256>2                                    // Lowercase c for Planet number.
      // <DeviceID>a<256><256><256><ID><high byte><low byte><delay ms>  // Lower a, <green><red><blue>, string of numbers intersperced with time.
      // <DeviceID>A<ID>                                                // Uppercase A, Animation ID.  Display Animation.
      // <DeviceID>B<256>                                               // Uppercase B for brightness
      // <DeviceID>N<ID>'Char string up to 14'                          // Name of the animation, up to 14 characters.
      

      //char serverHostname[20];
      
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {

        // this is debug
        if(DEBUG && false ) {
          display_print(F("BT Update: "));
          display_print(String(value.length()));
          display_print(F(":"));
          for (int i = 0; i < value.length(); i++){
            display_print(String(value[i] - 0));
            display_print(F(" "));
          }
          display_println(F(""));

          display_print(F("BT Update: "));
          display_print(String(value.length()));
          display_print(F(":"));
          for (int i = 0; i < value.length(); i++){
            display_print(String(value[i]));
          }
          display_println(F(""));
        }

        // make this a function for cleanliness.
        if(value[1] == 'B') {
          // adjust brightness
          FastLED.setBrightness(  value[2] );
          BRIGHTNESS = value[2];
        } else if(value[1] == 's') {
          // Individual Star, ID
          // not really used, we could get rid of this whole subsection.
          if(screensaver) {
            active_array[0].count = 0;
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis();
          int temp = (value[5] << 8) + value[6];
          // this has a temporary text to translate on it.
          // broken
          *star_array[temp].led = CRGB(value[2],value[3],value[4]);

          display_print(F("Display Star:"));
          display_println(star_array[temp].name);
          
        } else if(value[1] == 'a' && value.length() > 3) {
          // append the string to the animation.
          // strip bytes 2 and 3 in to a high-low int.
          int sessionID = (value[2] << 8) + value[3];

          if(last_animation_counter[value[0]-48] == sessionID) {
            for(int i = 7; i < value.length(); i=i+6) {
              int star_number = (value[i] << 8) + value[i+1];
              int timer = (value[i+2] << 8) + value[i+3];
              int show =  (value[i+4] << 8) + value[i+5];

              // append to an existing
              byte temp_colour[3] = {value[4],value[5],value[6]};
              append_animation(sessionID, temp_colour, star_number, timer, show);
            }
          } else {
            display_println("Session ID mismatch!");
          }
        } else if(value[1] == 'A') {
          // Display the animation sequence.
          // strip bytes 2 and 3 in to a high-low int.
          int sessionID = (value[2] << 8) + value[3];

          fade_time = millis();
          if(screensaver) {
            active_array[0].count = 0;
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis(); // always need to reset this one.
          activateAnimation(sessionID, false);
        } else if(value[1] == 'D') {
          // clear a saved animation.
          // strip bytes 2 and 3 in to a high-low int.
          int sessionID = (value[2] << 8) + value[3];

          animation_array[sessionID].count = 0;
        } else if(value[1] == 'c') {
          // Light up a constellation by constellation_id
          fade_time = millis();
          if(screensaver) {
            active_array[0].count = 0;
            FastLED.clear();
            screensaver = 0;
          }
          screensaver_time = millis(); // always need to reset this one.
          byte temp_byte[3] = {value[2],value[3],value[4]};

          int show = (value[6] << 8) + value[7]; // timing delay

          display_print(F("Display Constellation: "));
          display_println(constellation_array[value[5]].name);

          activateConstellation(value[5], temp_byte, show);

        } else if(value[1] == 'n') {
          // get the next available annimation session.

          String temp = String(value[0]); // set the first character to be the tablet ID

          // if the current session is empty, don't advance it.
          // provided the current device has this ID of course.
          if(animation_counter > 0 && animation_array[last_animation_counter[value[0]-48]].count == 0) {
            // our last selected array is empty, just re-use it.
            temp += last_animation_counter[value[0]-48];
          } else {
            last_animation_counter[value[0]-48] = animation_counter; // keep a record of which tablet has which number
            temp += animation_counter;

            animation_counter++;
            if(animation_counter >= 500)
              animation_counter = 0;

            // because we cycle around when we hit 500 entries, we need to keep an eye on that.
            if(max_animation_counter < 500)
              max_animation_counter++;

            animation_array[animation_counter].count = 0; // reset it to zero.
          }

          if(DEBUG) {
            display_print("New Session for Tablet ");
            display_print(String(value[0]));
            display_print(":");
            display_println(String(last_animation_counter[value[0]-48]));
          }
 
          pCharacteristic->setValue(temp.c_str());
        } else if (value[1] == 'N') {
          // Lets just check our current session for our tablet.
          String temp = String(value[0]); // set the first character to be the tablet ID
          temp += last_animation_counter[value[0]-48];

          pCharacteristic->setValue(temp.c_str());
          
        } else if (value[1] == 'S') {
          // fire up the screensaver.
          // start up the screensaver.
          // special mode.  Play the raindrops screensaver.

          active_array[0].count = 0;
          FastLED.clear();
          screensaver = 2;

          //Serial.println("twinkle twinkle: ");

          display_println("Twinkle Twinkle!");
        }

      }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      bluetooth_connect++;
      if(DEBUG)
        display_println(F("New device connected."));
      BLEDevice::startAdvertising(); // this advertises the characteristic
      display_header();
    };

    void onDisconnect(BLEServer* pServer) {
      bluetooth_connect--;
      if(DEBUG)
        display_println(F("Device disconnected."));
      display_header();
    }

};

void bluetooth_setup() {
  //Bluetooth accounts for about 50000Byte.
  String temp = String(PROJECT) + "." + String(NAME);
  char temp2[temp.length()+1];
  temp.toCharArray(temp2,temp.length()+1);
  BLEDevice::init(temp2);
  

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); //connections

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks());

  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("Ready for commands");

  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);

  if(beacon_on != 0)
    pAdvertising->start();

  // I use a value of one to show the device is ready to be connected too
  // maybe a little counterintuitive.
  bluetooth_connect = 1;

  display_header();

  display_println(F("Bluetooth Initialised."));
}
