class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {

      // ble can only have 20 characters.
      // what does that look like.
      // <DeviceID><command_type><string>
      // 1C<256><256><256>Chamaeleon                           // Uppercase C for Constellation Name.
      // 1c<256><256><256>2                                    // Lowercase c for Constellation number.
      // 1S<256><256><256>Alpha Chamaeleontis                  // Uppercase S for Star Name.
      // 1s<256><256><256>3                                    // Lowercase s for Star number.  Not a byte.
      // 1A<256><256><256><star id><star id>                   // Uppercase A, <green><red><blue>, string of numbers.
      // 1a<256><256><256><star id><delay ms><star id>         // Lower a, <green><red><blue>, string of numbers intersperced with time.
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
          // this has a temporary text to translate on it.
          *star_array[value[5]-48].led = CRGB(value[2],value[3],value[4]);
        } else if(value[1] == 'c') {
          // Light up a constellation by constellation_id
          fade_time = millis();
          if(screensaver) {
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis(); // always need to reset this one.
          for(int i = 0 ; i < constellation_array[value[5]-48].star_count; i++) {
            //Serial.println(constellation_array[value[5]-48].star_list[i]->name);  // keep this in mind.  it's how we reference the star.
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
