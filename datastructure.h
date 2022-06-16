/*
 * stars
 * Star Name, constellation, magnitude, pin, led
 * char [42]+'\0', byte, float, byte, byte
 * 
 * const
 * Constellation Name
 * char [19]+'\0'
 * 
 * planets
 * Are part of stars!
 */

typedef struct {
  // fun fact, Planets are represented here too!
  char name[43];
  unsigned int number; // need to refer back to this occasionaly.
  byte constellation;
  float magnitude;
  byte pin;
  byte point;
  // link back directly to the precise LED with a pointer.
  CRGB *led;
} stars;

stars *star_array; // = (stars *) ps_malloc(843 * sizeof(stars));

typedef struct {
  char name[20];
  int star_count = 0;
  stars *star_list[33]; // biggest constellation has 33 stars.
  // This array of pointers lets each constellation reference just it's stars directly rather than iterating through the 
  // large Star array and performing a huge number of if statements each cycle.  now we only have to do it once.
} constellations;

constellations *constellation_array; // = (constellations *) ps_malloc(90 * sizeof(constellations));

// We save an arbitrary number of animations to the unit to try and generate a history.
// each animation
typedef struct {
  unsigned int count = 0; // how many stars in this sequence.
  stars *star_list[MAX_SESSION_STARS]; // somewhat arbitrary size
  unsigned int times[MAX_SESSION_STARS]; // how long before starting the next led.
  unsigned int show[MAX_SESSION_STARS]; // how long to hold the led's on.
  byte colour[MAX_SESSION_STARS][3];
} animations;

animations *animation_array; // = (animation *) ps_malloc(10 * sizeof(animation));

// ok, if we keep an array of stars we are 
typedef struct {
  int count = 0; // how many stars active at the moment.
  stars *star_list[MAX_ACTIVE_STARS]; // always seems arbitrary, but as we only have 840 LED's, this leaves some buffer.
  byte rising[MAX_ACTIVE_STARS]; // we fading in or fading out?  Could be a bool
  byte colour[MAX_ACTIVE_STARS][3]; // What colour is our goal?
  unsigned long update[MAX_ACTIVE_STARS]; // when are we next updating this star.  Basically the trigger time.
  unsigned int show[MAX_ACTIVE_STARS];
  byte brightness[MAX_ACTIVE_STARS];
} active;

active *active_array;

// Fortune.  We store quotes to display.  Limited character count.
// Some Fortunes are linked to a constellation, planet or annimation.
typedef struct {
  int star;
  int constellation;
  int animation;
  char text[120];
} fortunes;

fortunes *fortune_array;