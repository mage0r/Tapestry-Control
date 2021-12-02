void leds_setup() {

  // each strip uses about 600 bytes of space.... makes sense.
  
  FastLED.addLeds<LED_TYPE, 4, COLOR_ORDER>(leds[0], NUM_LEDS_PER_STRIP);

  FastLED.addLeds<LED_TYPE, 14, COLOR_ORDER>(leds[1], NUM_LEDS_PER_STRIP);

  FastLED.addLeds<LED_TYPE, 15, COLOR_ORDER>(leds[2], NUM_LEDS_PER_STRIP);

  FastLED.addLeds<LED_TYPE, 27, COLOR_ORDER>(leds[3], NUM_LEDS_PER_STRIP);

  FastLED.addLeds<LED_TYPE, 26, COLOR_ORDER>(leds[4], NUM_LEDS_PER_STRIP);

  FastLED.addLeds<LED_TYPE, 25, COLOR_ORDER>(leds[5], NUM_LEDS_PER_STRIP);

  FastLED.addLeds<LED_TYPE, 19, COLOR_ORDER>(leds[6], NUM_LEDS_PER_STRIP);

  FastLED.addLeds<LED_TYPE, 22, COLOR_ORDER>(leds[7], NUM_LEDS_PER_STRIP);

  FastLED.setBrightness(  BRIGHTNESS );

  currentPalette = CloudColors_p;
  currentBlending = LINEARBLEND;

  //leds[0][10] = CRGB::Red;

  FastLED.show();

  display_println(F("LED's Initialised."));
}

void setup_animations() {
  // Animations are a bit of their own animals.
  // They need to be persistent.
  // probably need to be shared between units.  future problem.

  // By loading these in to PSRAM we can create a lot of them.
  animation_array = (animation *) ps_calloc(100, sizeof(animation)); //100?  feels like a lot.
  
  //Serial.println(100 * sizeof(animation)); // just curious as to how big this is.

  // Load from EEPROM?

  display_println(F("Animations Initialised."));
}

void append_animation(int animation_position, byte colour[3], int star_number, byte timer) {

  /*
  // enabling this debug information significantly slows the animation transmission speed.
  display_print(F("Append to animation:"));
  display_print(String(animation_position));
  display_print(F("["));
  display_print(String(animation_array[animation_position].count));
  display_print(F("]"));
  display_print(star_array[star_number].name);
  display_println("");
  */
  
  animation_array[animation_position].star_list[animation_array[animation_position].count] = &star_array[star_number];
  animation_array[animation_position].colour[animation_array[animation_position].count][0] = colour[0];
  animation_array[animation_position].colour[animation_array[animation_position].count][1] = colour[1];
  animation_array[animation_position].colour[animation_array[animation_position].count][2] = colour[2];
  animation_array[animation_position].times[animation_array[animation_position].count] = timer;
  animation_array[animation_position].count++;

  //appendAnimation(SPIFFS, "/ani.csv", ,animation_position);

}

void setupAnimation(int animation_position) {
  unsigned long temp_millis = millis();
  // so, this takes a few seconds to run
  temp_millis += 2000;
  for(int i=0; i < animation_array[animation_position].count; i++) {
    // cheating a bit, setting the max brightness
    *animation_array[animation_position].star_list[i]->led &= CRGB(animation_array[animation_position].colour[i][0],animation_array[animation_position].colour[i][1],animation_array[animation_position].colour[i][2]);

    // lets set our temporary delays.  these can't be runtime driven.
    animation_array[animation_position].show[i] = temp_millis + animation_array[animation_position].times[i];
  }
}

int openAnimation(int animation_position, boolean wipe_animation) {
  /*
  display_print(F("Show Animation #"));
  display_print(String(animation_position));
  display_print(F(":"));
  //display_print(animation_array[animation_position].name);
  display_println("");
  */

  unsigned long temp_millis = millis();
  for(int i=0; i < animation_array[animation_position].count; i++) {
    // cheating a bit, setting the max brightness
    //*animation_array[animation_position].star_list[i]->led &= CRGB(animation_array[animation_position].colour[i][0],animation_array[animation_position].colour[i][1],animation_array[animation_position].colour[i][2]);

    if(millis() > animation_array[animation_position].show[i]) {
      *animation_array[animation_position].star_list[i]->led++;
  
    }

    if(*animation_array[animation_position].star_list[i]->led != CRGB(animation_array[animation_position].colour[i][0],animation_array[animation_position].colour[i][1],animation_array[animation_position].colour[i][2]))
        wipe_animation = false;
    
    //*animation_array[animation_position].star_list[i]->led = CRGB(animation_array[animation_position].colour[i][0],animation_array[animation_position].colour[i][1],animation_array[animation_position].colour[i][2]);
    //display_println(animation_array[animation_position].star_list[i]->name);
    
    //delay(animation_array[animation_position].times[i]); //blocking :/
  }

  if(wipe_animation) {
    animation_array[animation_position].count = 0;
    Serial.println("wipe");
    return(-1);
  }
  return(animation_position);
}

// Fade out
void fadeOut() {
  
  EVERY_N_MILLISECONDS(10) {
    for(int j = 0; j < NUM_STRIPS; j++) {
      for( int i = 0; i < NUM_LEDS_PER_STRIP; ++i) {
          leds[j][i].fadeToBlackBy(fadeAmount);
      }
    }
  }
}

/*
void fadeIn(unsigned long temp_millis) {
    EVERY_N_MILLISECONDS(10) {
      if(millis() > temp_millis)
              leds[j][i].brighten8_video( fadeAmount);
    }

}
*/

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;

    for(int j = 0; j < NUM_STRIPS; j++) {
      for( int i = 0; i < NUM_LEDS_PER_STRIP; ++i) {
          leds[j][i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
          colorIndex += 3;
      }
    }
}
