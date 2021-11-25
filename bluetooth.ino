class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {

      // ble can only have 20 characters.
      // what does that look like.
      // <DeviceID><command_type><string>
      // 1C<256><256><256>Chamaeleon                           // Uppercase C for Constellation Name.
      // 1c<256><256><256>2                                    // Lowercase c for Constellation number.
      // 1S<256><256><256>Alpha Chamaeleontis                  // Uppercase S for Star Name.
      // 1s<256><256><256><high byte><low byte>                // Lowercase s for Star number.  Not a byte.
      // 1A<256><256><256><high byte><low byte>                // Uppercase A, <green><red><blue>, string of numbers.
      // 1a<256><256><256><ID><high byte><low byte><delay ms>  // Lower a, <green><red><blue>, string of numbers intersperced with time.
      // 1B<256>                                               // Uppercase B for brightness
      // 1P<256><256><256>Mercury                              // Uppercase P for Planet Name.
      // 1p<256><256><256>2                                    // Lowercase c for Planet number.

      //char serverHostname[20];
      
      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {
        
        display_print(F("BT Update: "));
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
          *star_array[temp].led = CRGB(value[2],value[3],value[4]);
        } else if(value[1] == 'a') {
          // append the string to the animation.
          for(int i = 6; i < value.length(); i=i+3) {
            int star_number = (value[i] << 8) + value[i+1];
            //Serial.println((value[i] << 8) + value[i+1]);
            //Serial.println(value[i+2]);
            // append to an existing?
            byte temp_byte[3] = {value[2],value[3],value[4]};
            append_animation(value[5], temp_byte, star_number, value[i+2]);
          }
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
        } else if(value[1] == 'p') {
          // Individual Star, ID
          if(screensaver) {
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis();
          // this has a temporary text to translate on it.
          *planet_array[value[5]-48].led = CRGB(value[2],value[3],value[4]);
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
          temp += annimation_counter;
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
                                         "6f485ef4-4d28-11ec-81d3-0242ac130003",
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
