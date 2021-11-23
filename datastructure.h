/*
 * Funny name so it gets loaded before fileops and bluetooth :)
 * 
 * stars
 * Star Name, constellation, magnitude, pin, led
 * char [42]+'\0', byte, float, byte, byte
 * 
 * const
 * Constellation Name
 * char [19]+'\0'
 * 
 * planets
 * Planet Name, pin, led
 * char [7]+'\0', byte, byte
 */

typedef struct {
  char name[43];
  byte constellation;
  float magnitude;
  byte pin;
  byte point;
  // link back directly to the precise LED with a pointer.
  CRGB *led;
} stars;

stars *star_array; // = (stars *) ps_malloc(833 * sizeof(stars));

typedef struct {
  char name[20];
  int star_count = 0;
  stars *star_list[33]; // biggest constellation has 33 stars.
  // This array of pointers lets each constellation reference just it's stars directly rather than iterating through the 
  // large Star array and performing a huge number of if statements each cycle.  now we only have to do it once.
} constellations;

constellations *constellation_array; // = (constellations *) ps_malloc(89 * sizeof(constellations));

typedef struct {
  char name[8];
  byte pin;
  byte point;
  CRGB *led;
} planets;

planets *planet_array; // = (planets *) ps_malloc(10 * sizeof(planets));

typedef struct {
  char name[14];
  byte unit; // which unit created this animation.
  byte id; // how did the unit number this animation.
  byte count; // how many stars in this sequence.
  stars *star_list[100]; // somewhat arbitrary size
  byte times[100];
  
} animation;

animation *animation_array; // = (animation *) ps_malloc(10 * sizeof(animation));
