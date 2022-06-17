void setup_display() {
  // Setup the TFT display
  myGLCD.init();
  myGLCD.setRotation(1);
  myGLCD.fillScreen(TFT_BLACK);
  myGLCD.setTextWrap(false); // don't want to accidentally end up on the next line.

  display_header();

  /* display can print
   *  14 lines
   *  40 Characters
   */

   display_println(F("Display Initialised."));
}

void display_header() {
  
  //myGLCD.fillRect(0, 0, 319, 16,TFT_BLUE); // clear the top of the screen

  String temp_header; // we re-use this a bit.

  // First, lets be public with the unit name.
  myGLCD.setTextPadding(320);
  myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
  temp_header = PROJECT;
  temp_header += ".";
  temp_header += NAME;
  myGLCD.drawString(temp_header,0,0,2);
  myGLCD.drawString(VERSION,200,0,2);

  display_update_bt();
}

void display_update_bt() {
    // Bluetooth Status
  String temp_header = " BT: ";
  if(bluetooth_connect) {
    if(beacon_on == 0)
      myGLCD.setTextColor(TFT_WHITE, TFT_BLUE);
    else if (bluetooth_connect == 1) // no connected devices
      myGLCD.setTextColor(TFT_GREEN, TFT_BLACK);
    else
      myGLCD.setTextColor(TFT_BLACK, TFT_GREEN);
    temp_header += String(bluetooth_connect - 1);
  } else {
    myGLCD.setTextColor(TFT_BLACK, TFT_RED);
    temp_header += "0";
  }

  myGLCD.setTextPadding(0);
  myGLCD.drawRightString(temp_header,320,0,2);
}

void display_update() {

  myGLCD.setTextPadding(320);
  myGLCD.setTextColor(TFT_WHITE,TFT_BLACK);
  myGLCD.setTextSize(1);
  for(int i = 0; i < 13; i++) {
    myGLCD.drawString(display_strings[i],0,16+(i*16),2);
  }
}

void safe_append(String temp) {
  // do something here with newlines cause they mess stuff up.
  temp.replace('\n',' ');
  temp.replace('\r',' ');

  String temp_text = display_strings[13];
  temp_text += temp;
  temp_text.trim();
  //Serial.println(myGLCD.textWidth(temp_text,2)); // this shows the length of the string.

  int break_point = temp_text.length();

  // work backwards from the max width.
  if(myGLCD.textWidth(temp_text,2) > 320) {
    for(int i = break_point; i > 0; i--) {
        if(myGLCD.textWidth(temp_text.substring(0,i),2) < 320 && temp_text.charAt(i) == ' ' ) {
          break_point = i;
          break;
        }
    }
  }

  // save our new string to the array.
  if(temp_text.substring(0,break_point).length() < DISPLAY_WIDTH)
    strcpy(display_strings[13],temp_text.substring(0,break_point).c_str());

  // we've got a long string
  if(break_point != temp_text.length()) {
    // Do the shuffle.
    for(int i = 0; i < 13; i++) {
      strcpy(display_strings[i],display_strings[i+1]);
    }
    display_strings[13][0] = '\0';

    safe_append(temp_text.substring(break_point));
  }

  /*
  // the old way

  int count = 0;
  int count2 = 0;
  for (int i = strlen(display_strings[13]); i < DISPLAY_WIDTH; i++) {
    display_strings[13][i] = temp.charAt(count);
    
    count++;
    count2 = i;
    if(count > temp.length()) // || count >= max_copy)
      i = DISPLAY_WIDTH;
    
  }

  display_strings[13][count2+1] = '\0';
    
  
  */
  
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

boolean display_fortune(int animation_position, boolean constellation) {
    // special case!  If we have a special comment related to our star, throw it up!.
    // cheat a little here.  We only have 4 fortunes specific to constellations or stars, don't bother checking
    // the whole array.
    // we also don't have anything on the first star or constellation, so only pay attention if it's 
    // not 0
    // we only want to set one fortune display, so check stars, then check constellation.
    String display_text;

    if(animation_position) {
      for(int i = 0; i < 5; i++) {
        if(!constellation && fortune_array[i].star == animation_position) {
          display_text = fortune_array[i].text;
        } else if (constellation && fortune_array[i].constellation == animation_position) {
          display_text = fortune_array[i].text;
        }
      }
    }

    // no specific fortune, pick a random one.
    // only do this every 10 times?
    int random_fortune = random(0,9);
    if(random_fortune == 1 && display_text.length() == 0) {
      random_fortune = random(4, 101);
      display_text = fortune_array[random_fortune].text;
    }

    if(display_text.length() > 0) {
      display_println(display_text);
      return true;
    }

    return false;
}
