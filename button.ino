// Setup the button

void button_setup() {
  
  pinMode(buttonPin, INPUT_PULLUP);
  
}

// what to do when reading the button.
void read_button() {

  if(beacon_on == 0 && digitalRead(buttonPin) == LOW) {
    discovery_time = millis();
    beacon_on = 1;
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();
    display_println(F("Bluetooth Discoverable."));
    display_header();
  }
  
}
