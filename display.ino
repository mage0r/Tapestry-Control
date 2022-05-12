void setup_display() {
  // Setup the TFT display
  myGLCD.init();
  myGLCD.setRotation(1);
  myGLCD.fillScreen(TFT_BLACK);

  display_header();

  /* display can print
   *  14 lines
   *  40 Characters
   */

   display_println(F("Display Initialised."));
}

void display_header() {
  myGLCD.fillRect(0, 0, 319, 31,TFT_BLUE); // clear the top of the screen

  String temp_header; // we re-use this a bit.

  // First, lets be public with the unit name.
  myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
  temp_header = PROJECT;
  temp_header += ".";
  temp_header += NAME;
  myGLCD.drawString(temp_header,0,0,2);
  myGLCD.drawString(VERSION,200,0,2);

  temp_header = " WIFI";
  if(wifi_connect) {
    myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
    temp_header += ": ";
    temp_header += WiFi.localIP().toString();
    myGLCD.drawString(temp_header,0,16,2);
  }
  /*else {
    myGLCD.setTextColor(TFT_BLACK, TFT_RED);
    temp_header += " ";

  }*/
  
  //myGLCD.drawString(temp_header,0,16,2);

  // mqtt status
  if(mqtt_connect) {
    myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
    myGLCD.drawString(" MQTT ",150,16,2);
  } //else
  //  myGLCD.setTextColor(TFT_BLACK, TFT_RED);
  //myGLCD.drawString(" MQTT ",150,16,2);

  // Bluetooth Status
  temp_header = " BT: ";
  if(bluetooth_connect) {
    if(beacon_on == 0)
      myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
    else
      myGLCD.setTextColor(TFT_WHITE, TFT_GREEN);
    temp_header += String(bluetooth_connect - 1);
  } else {
    myGLCD.setTextColor(TFT_BLACK, TFT_RED);
    temp_header += "0";
  }

  myGLCD.drawRightString(temp_header,320,0,2);

  // next line, start under the header.
}

void display_update() {

  myGLCD.fillRect(0, 31, 319, 239,TFT_BLACK); // hide everything!
  myGLCD.setCursor(0, 31, 2);
  myGLCD.setTextColor(TFT_WHITE,TFT_BLACK);
  myGLCD.setTextSize(1);
  for(int i = 0; i < 12; i++) {
    myGLCD.println(display_strings[i]);
  }
}

void safe_append(String temp) {

  // do something here with newlines cause they mess stuff up.
  temp.replace('\n',' ');
  temp.replace('\r',' ');

  //Serial.println(strlen(display_strings[12]));
  int count = 0;
  for (int i = strlen(display_strings[12]); i < DISPLAY_WIDTH; i++) {
    display_strings[12][i] = temp.charAt(count);
    count++;
    if(count > temp.length())
      i=DISPLAY_WIDTH;
  }

  display_strings[12][DISPLAY_WIDTH-1] = '\0';
  
}

// these really need to be char arrays
void display_print(String temp) {

 safe_append(temp);

  myGLCD.print(temp); // don't need to redraw the whole screen.
  Serial.print(temp);

}

void display_println(String temp) {
  safe_append(temp);

  // Do the shuffle.
  for(int i = 0; i < 12; i++) {
    strcpy(display_strings[i],display_strings[i+1]);
  }

  display_strings[12][0] = '\0';

  display_update();
  Serial.println(temp);

}
