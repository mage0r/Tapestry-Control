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

typedef struct {
  char name[14];
  byte count = 0; // how many stars in this sequence.
  stars *star_list[255]; // somewhat arbitrary size
  byte times[255];
  unsigned long show[255];
  byte colour[255][3];
} animation;

animation *animation_array; // = (animation *) ps_malloc(10 * sizeof(animation));
