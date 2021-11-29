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
      

      //char serverHostname[20];
      
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {

        // this is debug
        display_print(F("BT Update: "));
        display_print(String(value.length()));
        display_print(":");
        for (int i = 0; i < value.length(); i++){
          display_print(String(value[i]));

        }
        display_println("");

        // make this a function for cleanliness.
        if(value[1] == 'B') {
          // adjust brightness
          FastLED.setBrightness(  value[2] );
          BRIGHTNESS = value[2];
        } else if(value[1] == 's') {
          // Individual Star, ID
          if(screensaver) {
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis();
          int temp = (value[5] << 8) + value[6];
          // this has a temporary text to translate on it.
          *star_array[temp-48].led = CRGB(value[2],value[3],value[4]);
          display_print("Display Star:");
          display_println(star_array[temp-48].name);
        } else if(value[1] == 'a') {
          // append the string to the animation.
          for(int i = 6; i < value.length(); i=i+3) {
            int star_number = (value[i] << 8) + value[i+1];

            // This is all debug
            Serial.print(i);
            Serial.print(":");
            Serial.println(value.length());
            Serial.print("high:");
            Serial.println(int(value[i] << 8));
            Serial.print("low:");
            Serial.println(int(value[i+1]));
            Serial.print("combined:");
            Serial.println(star_number);
            Serial.print("timer:");
            Serial.println(int(value[i+2]));
            Serial.print("animation id:");
            Serial.println(int(value[5]));
            
            // append to an existing?
            byte temp_byte[3] = {value[2],value[3],value[4]};
            append_animation(int(value[5]), temp_byte, star_number, value[i+2]);
          }
        } else if(value[1] == 'A') {
          // Display the animation sequence.
          
        } else if(value[1] == 'c') {
          // Light up a constellation by constellation_id
          fade_time = millis();
          if(screensaver) {
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis(); // always need to reset this one.
          for(int i = 0 ; i < constellation_array[value[5]-48].star_count; i++) {
            *constellation_array[value[5]-48].star_list[i]->led = CRGB(value[2],value[3],value[4]);
          }
          display_print("Display Constellation:");
          display_println(constellation_array[value[5]-48].name);
        } else if(value[1] == 'p') {
          // Individual Star, ID
          if(screensaver) {
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis();
          // this has a temporary text to translate on it.
          *planet_array[value[5]-48].led = CRGB(value[2],value[3],value[4]);
          display_print("Display Planet:");
          display_println(planet_array[value[5]-48].name);
        }

        //FastLED.show();

      }
    }
};

// Next query slot.
class QueryCallback: public BLECharacteristicCallbacks {
  // <DeviceID><command_type>
  // <DeviceID>v                      Set value to Version number
  // <DeviceID>a                      Set value to next animation numbe
    void onWrite(BLECharacteristic *pCharacteristic2) {
      std::string value = pCharacteristic2->getValue();
      
      if (value.length() > 0) {
        String temp = String(value[0]);
        if(value[1] == 'a') {
          temp += animation_counter;
          animation_counter++;
          if(animation_counter)
            animation_counter = 0;            

          animation_array[animation_counter].count = 0;
        } else if(value[1] == 'v') {
          temp += VERSION;
        }
        pCharacteristic2->setValue(temp.c_str());
      }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      bluetooth_connect++;
      display_println(F("New device connected."));
      BLEDevice::startAdvertising(); // this advertises the characteristic
      display_header();
    };

    void onDisconnect(BLEServer* pServer) {
      bluetooth_connect--;
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

  pCharacteristic2 = pService->createCharacteristic(
                                         DATA_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic2->setCallbacks(new QueryCallback());
  pCharacteristic2->setValue("Query for details.");

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
