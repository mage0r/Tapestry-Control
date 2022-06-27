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
            animation_array[animation_counter].count = 0; // reset it to zero.

            animation_counter++;

            if(animation_counter >= MAX_SESSIONS) {
              animation_counter = 0;
            }

            // because we cycle around when we hit MAX_SESSIONS entries, we need to keep an eye on that.
            if(max_animation_counter < MAX_SESSIONS)
              max_animation_counter++;

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
              if(display_buffer_count >= 10)
                display_buffer_count = 0;
            }

            pCharacteristic->setValue(temp.c_str());

        } else if (value[1] == 'N') {
          // Lets just check our current session for our tablet.
          String temp = String(value[0]); // set the first character to be the tablet ID

          // whoa whoa whoa, what happens if we restart with an open session?!
          // you get a -1, which nothing really likes.
          // the tablet reads that as 65535.
          // 65535 doesn't match -1
          // everything complains if you're lucky, crashes if you aren't.
          // I hate putting processing in this section, but I guess we gotta.

          if(last_animation_counter[value[0]-48] == -1) {
            last_animation_counter[value[0]-48] = animation_counter;
            animation_array[animation_counter].count = 0; // reset animation to zero.

            animation_counter++;
            if(animation_counter >= MAX_SESSIONS)
              animation_counter = 0;
            // because we cycle around when we hit MAX_SESSIONS entries, we need to keep an eye on that.
            if(max_animation_counter < MAX_SESSIONS)
              max_animation_counter++;
          }

          temp += last_animation_counter[value[0]-48];

          pCharacteristic->setValue(temp.c_str());
          
        } else {
          //strcpy(command_array[command_count],value.c_str());
          //command_array[command_count] = value;
          command_buffer[command_count].length = value.length();
          for(int j = 0; j < value.length(); j++) {
            command_buffer[command_count].text[j] = value[j];
          }

          command_count++;

          if(command_count >= MAX_BUFFER)
            command_count = 0;
        }
      }
    }
};

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      bluetooth_connect++;
      /*
      if(DEBUG) {
        String temp = "We have a New Challenger.";
        strcpy(display_buffer[display_buffer_count], temp.c_str());
        display_buffer_count++;
      }*/
      display_println(F("We have a New Challenger."));
      BLEDevice::startAdvertising(); // this advertises the characteristic
      //display_header();
      display_update_bt();
    };

    void onDisconnect(BLEServer* pServer) {
      bluetooth_connect--;
      /*
      if(DEBUG) {
        String temp = "Bye Bye Tablet.";
        strcpy(display_buffer[display_buffer_count], temp.c_str());
        display_buffer_count++;
      }*/
      display_println(F("Bye Bye Tablet."));
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

  command_buffer = (ble_buffer*) ps_calloc(MAX_BUFFER, sizeof(ble_buffer));

  display_println(F("Bluetooth Initialised."));

}

void command_processing() {
  // a little messy
  // this is a rolling buffer.  Each call we will process 10 of these so we don't go blocking.

  for(int x = 0; x < 10; x++) {
    
    if(command_buffer[processed_command_count].length > 0) {

      if(command_buffer[processed_command_count].text[1] == 'B') {
        // adjust brightness
        FastLED.setBrightness(  command_buffer[processed_command_count].text[2] );
        BRIGHTNESS = command_buffer[processed_command_count].text[2];
      } else if(command_buffer[processed_command_count].text[1] == 'a' && command_buffer[processed_command_count].length > 3) {
        // append the string to the animation.
        // strip bytes 2 and 3 in to a high-low int.
        int sessionID = (command_buffer[processed_command_count].text[2] << 8) + command_buffer[processed_command_count].text[3];

        if(last_animation_counter[command_buffer[processed_command_count].text[0]-48] == sessionID) {
          for(int i = 7; i < command_buffer[processed_command_count].length; i=i+6) {
            int star_number = (command_buffer[processed_command_count].text[i] << 8) + command_buffer[processed_command_count].text[i+1];
            int timer = (command_buffer[processed_command_count].text[i+2] << 8) + command_buffer[processed_command_count].text[i+3];
            int show =  (command_buffer[processed_command_count].text[i+4] << 8) + command_buffer[processed_command_count].text[i+5];

            // append to an existing
            byte temp_colour[3] = {command_buffer[processed_command_count].text[4],command_buffer[processed_command_count].text[5],command_buffer[processed_command_count].text[6]};

            append_animation(sessionID, temp_colour, star_number, timer, show);
          }
        } else {
          // this can dump out a lot.
          display_print(String(last_animation_counter[command_buffer[processed_command_count].text[0]-48]));
          display_print("<>");
          display_print(String(sessionID));
          display_print(":");
          display_println(F("This session doesn't match the device!"));
        }
      } else if(command_buffer[processed_command_count].text[1] == 'A') {
        // Display the animation sequence.
        // strip bytes 2 and 3 in to a high-low int.
        int sessionID = (command_buffer[processed_command_count].text[2] << 8) + command_buffer[processed_command_count].text[3];

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
      } else if(command_buffer[processed_command_count].text[1] == 'D') {
        // clear a saved animation.
        // strip bytes 2 and 3 in to a high-low int.
        int sessionID = (command_buffer[processed_command_count].text[2] << 8) + command_buffer[processed_command_count].text[3];

        if(sessionID < max_animation_counter)
          animation_array[sessionID].count = 0;
      } else if(command_buffer[processed_command_count].text[1] == 'c') {
        // Light up a constellation by constellation_id
        if(screensaver) {
          display_println("I'm awake! I'm awake!");
          active_array[0].count = 0;
          FastLED.clear();
          screensaver = 0;
        }
        screensaver_time = millis(); // always need to reset this one.
        byte temp_byte[3] = {command_buffer[processed_command_count].text[2],command_buffer[processed_command_count].text[3],command_buffer[processed_command_count].text[4]};

        int show = (command_buffer[processed_command_count].text[6] << 8) + command_buffer[processed_command_count].text[7]; // timing delay

        display_print(F("Lets look at "));
        display_println(constellation_array[command_buffer[processed_command_count].text[5]].name);

        display_fortune(command_buffer[processed_command_count].text[5], true);

        activateConstellation(command_buffer[processed_command_count].text[5], temp_byte, show);

      } else if (command_buffer[processed_command_count].text[1] == 'S') {
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

      processed_command_count++;

      if(processed_command_count >= MAX_BUFFER)
        processed_command_count = 0;

      if(processed_command_count >= command_count)
       x = 10;

    }
  }
}
