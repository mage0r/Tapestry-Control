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
  myGLCD.fillRect(0, 0, 319, 16,TFT_BLUE); // clear the top of the screen

  String temp_header; // we re-use this a bit.

  // First, lets be public with the unit name.
  myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
  temp_header = PROJECT;
  temp_header += ".";
  temp_header += NAME;
  myGLCD.drawString(temp_header,0,0,2);
  myGLCD.drawString(VERSION,200,0,2);

  // Bluetooth Status
  temp_header = " BT: ";
  if(bluetooth_connect) {
    if(beacon_on == 0)
      myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
    else if (bluetooth_connect == 1) // no connected devices
      myGLCD.setTextColor(TFT_GREEN, TFT_WHITE);
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

  myGLCD.fillRect(0, 16, 319, 239,TFT_BLACK); // hide everything!
  myGLCD.setCursor(0, 16, 2);
  myGLCD.setTextColor(TFT_WHITE,TFT_BLACK);
  myGLCD.setTextSize(1);
  for(int i = 0; i < 13; i++) {
    myGLCD.println(display_strings[i]);
  }
}

void safe_append(String temp) {

  // do something here with newlines cause they mess stuff up.
  temp.replace('\n',' ');
  temp.replace('\r',' ');

  int count = 0;
  for (int i = strlen(display_strings[13]); i < DISPLAY_WIDTH; i++) {
    display_strings[13][i] = temp.charAt(count);
    count++;
    if(count > temp.length())
      i=DISPLAY_WIDTH;
  }

  display_strings[13][DISPLAY_WIDTH-1] = '\0';

  // send the leftover string to a new line.
  if(count < temp.length()) {

    // Do the shuffle.
    for(int i = 0; i < 13; i++) {
      strcpy(display_strings[i],display_strings[i+1]);
    }

    display_strings[13][0] = '\0';
    display_update();

    temp = temp.substring(count);
    safe_append(temp);
  }  
  
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
  for(int i = 0; i < 13; i++) {
    strcpy(display_strings[i],display_strings[i+1]);
  }

  display_strings[13][0] = '\0';

  display_update();
  Serial.println(temp);

}
