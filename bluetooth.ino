class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {

      std::string value = pCharacteristic->getValue();

      if (value.length() > 0) {

        // Two of these responses need immediate bluetooth responses, so we'll deal with them here.
        // otherwise, copy the array to the command buffer.

        if(value[1] == 'n') {
          // get the next available annimation session.

          String temp = String(value[0]); // set the first character to be the tablet ID

          // if the current session is empty, don't advance it.
          // provided the current device has this ID of course.
          if(animation_counter > 0 && last_animation_counter[value[0]-48] != -1 && animation_array[last_animation_counter[value[0]-48]].count == 0 ) {
            // our last selected array is empty, just re-use it.
            temp += last_animation_counter[value[0]-48];
          } else {
            last_animation_counter[value[0]-48] = animation_counter; // keep a record of which tablet has which number
            temp += animation_counter;

            animation_counter++;

            if(animation_counter >= MAX_SESSIONS) {
              animation_counter = 0;
            }

            // because we cycle around when we hit MAX_SESSIONS entries, we need to keep an eye on that.
            if(max_animation_counter < MAX_SESSIONS)
              max_animation_counter++;


            animation_array[animation_counter].count = 0; // reset it to zero.
          }

            if(DEBUG) {
              String temp_text;
              temp_text += "Tablet ";
              temp_text += value[0];
              temp_text += ", show us what you can do at ";
              temp_text += last_animation_counter[value[0]-48];
              //display_println(temp_text);
              strcpy(display_buffer[display_buffer_count], temp_text.c_str());
              display_buffer_count++;
            }

            pCharacteristic->setValue(temp.c_str());

        } else if (value[1] == 'N') {
          // Lets just check our current session for our tablet.
          String temp = String(value[0]); // set the first character to be the tablet ID
          temp += last_animation_counter[value[0]-48];

          pCharacteristic->setValue(temp.c_str());
          
        } else {
          //strcpy(command_array[command_count],value.c_str());
          command_array[command_count] = value;

          command_count++;
        }
      }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      bluetooth_connect++;
      if(DEBUG) {
        String temp = "We have a New Challenger.";
        strcpy(display_buffer[display_buffer_count], temp.c_str());
        display_buffer_count++;
      }
      BLEDevice::startAdvertising(); // this advertises the characteristic
      //display_header();
      display_update_bt();
    };

    void onDisconnect(BLEServer* pServer) {
      bluetooth_connect--;
      if(DEBUG) {
        String temp = "Bye Bye Tablet.";
        strcpy(display_buffer[display_buffer_count], temp.c_str());
        display_buffer_count++;
      }
      //display_header();
      display_update_bt();
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

  //display_header();
  display_update_bt();

  String temp3 = "Bluetooth Initialised.";
  strcpy(display_buffer[display_buffer_count], temp3.c_str());
  display_buffer_count++;
}

void command_processing() {
  // a little messy
  // there is a chance we'll get bluetooth adding new stuff to the array
  // so we copy it, zero out the command counter and let the command counter work it out
  // itself later.

  Serial.print(F("Free Ram: "));
    Serial.print(String(ESP.getFreeHeap()));
    Serial.print(F(". Free PSRam: "));
    Serial.println(String(ESP.getFreePsram()));

  std::string temp_command_array[200];
  int temp_command_count = command_count;
  for(int i = 0; i < temp_command_count; i++) {
    //strcpy(temp_command_array[i],command_array[i]);
      temp_command_array[i] = command_array[i];
  }
  command_count = 0; // copied, lets go back to zero.

  Serial.print(F("Free Ram: "));
    Serial.print(String(ESP.getFreeHeap()));
    Serial.print(F(". Free PSRam: "));
    Serial.println(String(ESP.getFreePsram()));

  while(temp_command_count) {
    temp_command_count--;

    std::string value2 = temp_command_array[temp_command_count];

    if (value2.length() > 0) {

      // this is debug
        if(DEBUG && false) {
          Serial.print(F("BT Update: "));
          Serial.print(String(value2.length()));
          Serial.print(F(":"));
          for (int i = 0; i < value2.length(); i++){
            Serial.print(String(value2[i] - 0));
            Serial.print(F(" "));
          }
          Serial.println(F(""));

          Serial.print(F("BT Update: "));
          Serial.print(String(value2.length()));
          Serial.print(F(":"));
          for (int i = 0; i < value2.length(); i++){
            Serial.print(String(value2[i]));
          }
          Serial.println(F(""));
        }

      if(value2[1] == 'B') {
        // adjust brightness
        FastLED.setBrightness(  value2[2] );
        BRIGHTNESS = value2[2];
      } else if(value2[1] == 'a' && value2.length() > 3) {
        // append the string to the animation.
        // strip bytes 2 and 3 in to a high-low int.
        int sessionID = (value2[2] << 8) + value2[3];

        if(last_animation_counter[value2[0]-48] == sessionID) {
          for(int i = 7; i < value2.length(); i=i+6) {
            int star_number = (value2[i] << 8) + value2[i+1];
            int timer = (value2[i+2] << 8) + value2[i+3];
            int show =  (value2[i+4] << 8) + value2[i+5];

            // append to an existing
            byte temp_colour[3] = {value2[4],value2[5],value2[6]};

            append_animation(sessionID, temp_colour, star_number, timer, show);
          }
        } else {
          // this can dump out a lot.
          display_println(F("This session doesn't match the device!"));
        }
      } else if(value2[1] == 'A') {
        // Display the animation sequence.
        // strip bytes 2 and 3 in to a high-low int.
        int sessionID = (value2[2] << 8) + value2[3];

        if(sessionID < max_animation_counter) {
          if(screensaver) {
            display_println("I'm awake! I'm awake!");
            active_array[0].count = 0;
            FastLED.clear ();
            screensaver = 0;
          }
          screensaver_time = millis(); // always need to reset this one.

          display_fortune(sessionID, false);

          activateAnimation(sessionID);
        }
      } else if(value2[1] == 'D') {
        // clear a saved animation.
        // strip bytes 2 and 3 in to a high-low int.
        int sessionID = (value2[2] << 8) + value2[3];

        if(sessionID < max_animation_counter)
          animation_array[sessionID].count = 0;
      } else if(value2[1] == 'c') {
        // Light up a constellation by constellation_id
        if(screensaver) {
          display_println("I'm awake! I'm awake!");
          active_array[0].count = 0;
          FastLED.clear();
          screensaver = 0;
        }
        screensaver_time = millis(); // always need to reset this one.
        byte temp_byte[3] = {value2[2],value2[3],value2[4]};

        int show = (value2[6] << 8) + value2[7]; // timing delay

        display_print(F("Lets look at "));
        display_println(constellation_array[value2[5]].name);

        display_fortune(value2[5], true);

        activateConstellation(value2[5], temp_byte, show);

      } else if (value2[1] == 'S') {
        // fire up the screensaver.
        // start up the screensaver.
        // special mode.  Play the raindrops screensaver.

        active_array[0].count = 0;
        FastLED.clear();
        
        if(screensaver == 2) {
          display_println("Getting Sleepy....");
          screensaver = 1;
          random_screensaver = -1;
        } else {
          screensaver = 2;
          display_println("Twinkle Twinkle!");
        }
      }
    }
  }
}
