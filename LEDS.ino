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

  display_println("LED's Initialised.");
}

void setup_animations() {
  // Animations are a bit of their own animals.
  // They need to be persistent.
  // probably need to be shared between units.  future problem.

  // By loading these in to PSRAM we can create a lot of them.
  animation_array = (animation *) ps_malloc(100 * sizeof(animation)); //100?  feels like a lot.
  //Serial.println(100 * sizeof(animation)); // just curious as to how big this is.

  // Load from EEPROM?

  display_println("Animations Initialised.");
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

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; display_println("Rainbow");}
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND; display_println("Rainbow Stripe"); }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; display_println("Rainbow Stripe Blend"); }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; display_println("Purple Green"); }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; display_println("Random"); }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; display_println("Black and white"); }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; display_println("black and whipe. Blend"); }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; display_println("Cloud"); }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; display_println("Party"); }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND; display_println("Red White Blue"); }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; display_println("Red White Blue Blend"); }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; ++i) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};
