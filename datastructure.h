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
  char name[14];
  byte count = 0; // how many stars in this sequence.
  stars *star_list[255]; // somewhat arbitrary size
  int times[255];
  byte colour[255][3];
} animation;

animation *animation_array; // = (animation *) ps_malloc(10 * sizeof(animation));

// ok, if we keep an array of stars we are 
typedef struct {
  int count = 0; // how many stars active at the moment.
  stars *star_list[1000]; // always seems arbitrary, but as we only have 840 LED's, this leaves some buffer.
  byte rising[1000]; // we fading in or fading out?  Could be a bool
  byte colour[1000][3]; // What colour is our goal?
  unsigned long update[1000]; // when are we next updating this star.  Basically the trigger time.
  byte brightness[1000];
} active;

active *active_array;
