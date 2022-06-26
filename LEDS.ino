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

  FastLED.show();

  display_println(F("LED's Initialised."));
}

void setup_animations() {
  // Animations are a bit of their own animals.
  // They need to be persistent.
  // probably need to be shared between units.  future problem.

  // By loading these in to PSRAM we can create a lot of them.
  animation_array = (animations *) ps_calloc(MAX_SESSIONS, sizeof(animations)); //any more than this and we run out of ram.

  // Crunch time.  This doesn't need to be an array, but it does need to be allocated to lock
  // up the RAM.  Don't have time to get this fixed
  active_array = (active *) ps_calloc(1, sizeof(active)); // only need one active.
  
  //Serial.println(1 * sizeof(active)); // just curious as to how big this is. 14004

  // Load from EEPROM?

  display_println(F("Animations Initialised."));
}

// add new stars to the animation arrays.
void append_animation(int animation_position, byte colour[3], int star_number, int timer, int show) {

  if(animation_array[animation_position].count > MAX_SESSION_STARS) {
    display_print(String(animation_array[animation_position].count));
    display_println(": Too many stars!");

  } else {

    animation_array[animation_position].star_list[animation_array[animation_position].count] = &star_array[star_number];
    animation_array[animation_position].colour[animation_array[animation_position].count][0] = colour[0];
    animation_array[animation_position].colour[animation_array[animation_position].count][1] = colour[1];
    animation_array[animation_position].colour[animation_array[animation_position].count][2] = colour[2];
    animation_array[animation_position].times[animation_array[animation_position].count] = timer;
    animation_array[animation_position].show[animation_array[animation_position].count] = show;
    animation_array[animation_position].count++;

  }

}

// This copies the requsted sequence in to the active stars array.
void activateAnimation(int animation_position) {
  unsigned long temp_millis = millis();
  // so, this takes a few seconds to run
  //temp_millis += 100;

  //where is our animation up to
  int current = active_array[0].count;

   if(DEBUG && false) {
    Serial.println(F("Setup animation."));
    Serial.println(String(current));
    Serial.println(animation_position);
    Serial.println(animation_array[animation_position].count);
   }

  for(int i=0; i < animation_array[animation_position].count; i++) {

    // just check that our animation array isn't full.
    // doing this at the top just in case it's full from the onset.
    if(current >= MAX_ACTIVE_STARS) {
      display_print(String(current));
      display_println(F(": Animation Buffer Full."));
      break;
    }

    temp_millis += animation_array[animation_position].times[i];

    active_array[0].star_list[current] = animation_array[animation_position].star_list[i]; // copy the LED
    active_array[0].rising[current] = 1; // fading in.
    active_array[0].colour[current][0] = animation_array[animation_position].colour[i][0];
    active_array[0].colour[current][1] = animation_array[animation_position].colour[i][1];
    active_array[0].colour[current][2] = animation_array[animation_position].colour[i][2];
    active_array[0].update[current] = temp_millis;
    active_array[0].show[current] = animation_array[animation_position].show[i];
    active_array[0].brightness[current] = 0;

    current++;

  }

  active_array[0].count = current;

}

// This copies the requsted constellation in to the active stars array.
// functionally very similar to activateAnimation
void activateConstellation(byte animation_position, byte colour[3], int show) {
  unsigned long temp_millis = millis();
  // so, this takes a few seconds to run
  //temp_millis += 100;

  // When displaying a constellation, we also copy it in to the session information.
  unsigned int temp_animation = animation_counter;
  animation_array[temp_animation].count = 0; // reset it to zero.
  animation_counter++;

  if(animation_counter >= MAX_SESSIONS)
    animation_counter = 0;

  if(max_animation_counter < MAX_SESSIONS)
    max_animation_counter++;


  for(int i=0; i < constellation_array[animation_position].star_count; i++) {

    // copy our constellation to the animation list.
    // we need to keep a record of our animation list so we can play it back. 

    animation_array[temp_animation].star_list[animation_array[temp_animation].count] = constellation_array[animation_position].star_list[i];
    animation_array[temp_animation].colour[animation_array[temp_animation].count][0] = colour[0];
    animation_array[temp_animation].colour[animation_array[temp_animation].count][1] = colour[1];
    animation_array[temp_animation].colour[animation_array[temp_animation].count][2] = colour[2];
    animation_array[temp_animation].times[animation_array[temp_animation].count] = 0;
    animation_array[temp_animation].show[animation_array[temp_animation].count] = show;
    animation_array[temp_animation].count++;

  }

  activateAnimation(temp_animation);

}

void openAnimation() {

  // should be slightly more efficient to run this backwards.
  // by that I mean when doing a trim we won't be moving leds that 
  // would be removed later anyway.
  for(int i=active_array[0].count-1; i >= 0; i--) {

      if(active_array[0].update[i] <= millis()) {
        // we're go to start the sequence.
        // increase/decrease the brightness depending on the rising flag.
        active_array[0].brightness[i] += active_array[0].rising[i];

        // set the colour based on our brightness.
        // simply put, the brighness value is a percentage, multiple the colour by that
        *active_array[0].star_list[i]->led = CRGB(((active_array[0].colour[i][0]/100)*active_array[0].brightness[i]),
                                                  ((active_array[0].colour[i][1]/100)*active_array[0].brightness[i]),
                                                  ((active_array[0].colour[i][2]/100)*active_array[0].brightness[i]));
                                                   

        if(active_array[0].brightness[i] >= 100) {
          active_array[0].rising[i] = -2; // we've hit the top, back down we go.
          active_array[0].update[i] = long(active_array[0].show[i]) + millis();// don't forget to wait a bit at max brightness!
        } else if(active_array[0].brightness[i] <= 0) {
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


  if(DEBUG && false) {
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
  active_array[0].show[to_trim] = active_array[0].show[temp_array_count];
  active_array[0].brightness[to_trim] = active_array[0].brightness[temp_array_count];

  active_array[0].count--;

  if(DEBUG && false)
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
