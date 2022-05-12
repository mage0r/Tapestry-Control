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

  active_array = (active *) ps_calloc(1, sizeof(active)); // only need one active.
  
  //Serial.println(1 * sizeof(active)); // just curious as to how big this is. 14004

  // Load from EEPROM?

  display_println(F("Animations Initialised."));
}

// add new stars to the animation arrays.
void append_animation(int animation_position, byte colour[3], int star_number, byte timer) {

  
  // enabling this debug information significantly slows the animation transmission speed.
  /*
  if(DEBUG) {
    display_print(F("Append to animation:"));
    Serial.print(String(animation_position));
    Serial.print(F("["));
    Serial.print(String(animation_array[animation_position].count));
    Serial.print(F("]"));
    Serial.print(star_array[star_number].name);
    Serial.println("");
  }
  */
  
  
  animation_array[animation_position].star_list[animation_array[animation_position].count] = &star_array[star_number];
  animation_array[animation_position].colour[animation_array[animation_position].count][0] = colour[0];
  animation_array[animation_position].colour[animation_array[animation_position].count][1] = colour[1];
  animation_array[animation_position].colour[animation_array[animation_position].count][2] = colour[2];
  animation_array[animation_position].times[animation_array[animation_position].count] = timer;
  animation_array[animation_position].count++;

  //appendAnimation(SPIFFS, "/ani.csv", ,animation_position);

}

// This copies the requsted sequence in to the active stars array.
void activateAnimation(int animation_position) {
  unsigned long temp_millis = millis();
  // so, this takes a few seconds to run
  temp_millis += 100;

  //where is our animation up to
  int current = active_array[0].count;

   if(DEBUG && 0) {
    display_println(F("Setup animation."));
    Serial.println(current);
    Serial.println(animation_position);
    Serial.println(animation_array[animation_position].count);
   }

  for(int i=0; i < animation_array[animation_position].count; i++) {

    // just check that our animation array isn't full.
    // doing this at the top just in case it's full from the onset.
    if(current >= 1000) {
      display_print(F("Animation buffer exhausted."));
      break;
    }

    temp_millis += animation_array[animation_position].times[i];
    //temp_millis += 700;

    if(DEBUG && 0) {
      Serial.print("Animation:");
      Serial.print(i);
    }

    active_array[0].star_list[current] = animation_array[animation_position].star_list[i]; // copy the LED
    active_array[0].rising[current] = 1; // fading in.
    active_array[0].colour[current][0] = animation_array[animation_position].colour[i][0];
    active_array[0].colour[current][1] = animation_array[animation_position].colour[i][1];
    active_array[0].colour[current][2] = animation_array[animation_position].colour[i][2];
    active_array[0].update[current] = temp_millis;
    active_array[0].brightness[current] = 0;

    if(DEBUG) {
      //Serial.println(" end.");
    }

    current++;
  }

  active_array[0].count = current;

  // super, super temporary.
  // when we play the animation, clear the record of it.
  animation_array[animation_position].count = 0;
}

// This copies the requsted constellation in to the active stars array.
// functionally very similar to activateAnimation, but a lot simpiler.
void activateConstellation(byte animation_position, byte colour[3]) {
  unsigned long temp_millis = millis();
  // so, this takes a few seconds to run
  temp_millis += 100;

  //where is our animation up to
  int current = active_array[0].count;

  for(int i=0; i < constellation_array[animation_position].star_count; i++) {

    // just check that our animation array isn't full.
    // doing this at the top just in case it's full from the onset.
    if(current >= 1000) {
      display_print(F("Animation buffer exhausted."));
      break;
    }

    // we really want to fade our whole array in at once.
    // this is arbitrary and may be needed later.
    //temp_millis += 700;

    active_array[0].star_list[current] = constellation_array[animation_position].star_list[i];
    active_array[0].rising[current] = 1; // fading in.
    active_array[0].colour[current][0] = colour[0];
    active_array[0].colour[current][1] = colour[1];
    active_array[0].colour[current][2] = colour[2];
    active_array[0].update[current] = temp_millis;
    active_array[0].brightness[current] = 0;

    current++;
  }

  active_array[0].count = current;
}

void openAnimation() {

  /*
  if(DEBUG && active_array[0].count) {
    display_println("Opening Annimation");
  }
  */

  // should be slightly more efficient to run this backwards.
  // by that I mean when doing a trim we won't be moving leds that 
  // would be removed later anyway.
  for(int i=active_array[0].count-1; i >= 0; i--) {
      
      if(active_array[0].update[i] <= millis()) {
        // we're go to start the sequence.
        // increase/decrease the brightness depending on the rising flag.
        active_array[0].brightness[i] += active_array[0].rising[i];

        // testing my fading algorithim
        if(i == 0 && false) {
          Serial.print(((active_array[0].colour[i][0]/100)*active_array[0].brightness[i]));
          Serial.print(",");
          Serial.print(((active_array[0].colour[i][1]/100)*active_array[0].brightness[i]));
          Serial.print(",");
          Serial.println(((active_array[0].colour[i][2]/100)*active_array[0].brightness[i]));
        }

        // set the colour based on our brightness.
        // simply put, the brighness value is a percentage, multiple the colour by that
        *active_array[0].star_list[i]->led = CRGB(((active_array[0].colour[i][0]/100)*active_array[0].brightness[i]),
                                                  ((active_array[0].colour[i][1]/100)*active_array[0].brightness[i]),
                                                  ((active_array[0].colour[i][2]/100)*active_array[0].brightness[i]));
                                                   

        if(active_array[0].brightness[i] >= 100)
          active_array[0].rising[i] = -2; // we've hit the top, back down we go.
        else if(active_array[0].brightness[i] <= 0) {
          // trigger a cleanup to remove this LED from the active array.
          // probably going to be slow.
          trim_active(i);
        }
      }
  }
}

// remove a completed animation from the stack.
// this is probably going to be slow.
void trim_active(int to_trim) {

  // I figure all the calculations for -1 is more computationally expensive than a single int.
  int temp_array_count = active_array[0].count - 1;


  if(DEBUG) {
    Serial.print("Trimming: ");
    Serial.print(to_trim);
    Serial.print(". Active: ");
    Serial.print(temp_array_count);    
  }

  // we don't care what order the LED's are in!
  // replace the one we want gone with the last one.
  active_array[0].star_list[to_trim] = active_array[0].star_list[temp_array_count];
  active_array[0].rising[to_trim] = active_array[0].rising[temp_array_count];
  active_array[0].colour[to_trim][0] = active_array[0].colour[temp_array_count][0];
  active_array[0].colour[to_trim][1] = active_array[0].colour[temp_array_count][1];
  active_array[0].colour[to_trim][2] = active_array[0].colour[temp_array_count][2];
  active_array[0].update[to_trim] = active_array[0].update[temp_array_count];
  active_array[0].brightness[to_trim] = active_array[0].brightness[temp_array_count];

  active_array[0].count--;

  Serial.println(". Trim complete.");

}

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
