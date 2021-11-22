

void wifi_setup() {
  // accounts for about 70000Bytes.
  
  // We start by connecting to a WiFi network
  display_print(F("Connecting to "));
  display_println(ssid);

  WiFi.mode(WIFI_STA);

  WiFi.setHostname("ESP32-test");

  WiFi.begin(ssid, password);

  for(int i = 0; i < 20; i++) {
    if(WiFi.status() == WL_CONNECTED) {
      wifi_connect = true;
      i=20;
    }
    delay(500); // need this to be non-blocking
    display_print(F("."));
  }

  display_header();

  if(wifi_connect) {
    display_println(""); // start a fresh line.
    display_println(F("WiFi connected"));
    display_print(F("IP address: "));
    display_println(WiFi.localIP().toString());
    //display_println(String(WiFi.localIP())); // this translation is not good.
    //Serial.println(WiFi.localIP());
    /*setup MDNS for ESP32 */
    if (!MDNS.begin("esp32")) {
        display_println(F("Error setting up MDNS responder!"));
        while(1) {
            delay(1000);
        }
    }
  } else {
    display_println(""); // start a fresh line.
    display_println(F("WiFi not found - retry in 5 mins"));
    
  }
}
