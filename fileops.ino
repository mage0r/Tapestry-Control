void setup_fileops() {
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      display_println("SPIFFS Mount Failed");
      return;
  }

  display_println(F("SPIFFS Initialised."));

  star_array = (stars *) ps_calloc(833, sizeof(stars));
  constellation_array = (constellations *) ps_calloc(89, sizeof(constellations));
  planet_array = (planets *) ps_calloc(10, sizeof(planets));
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    display_print(F("Listing directory: "));
    display_println(dirname);

    File root = fs.open(dirname);
    if(!root){
        display_println(F("- failed to open directory"));
        return;
    }
    if(!root.isDirectory()){
        display_println(F(" - not a directory"));
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            display_print(F("  DIR : "));
            display_println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            display_print(F("  FILE: "));
            display_print(file.name());
            display_print(F("    SIZE: "));
            display_println(String(file.size()));
        }
        file = root.openNextFile();
    }
}

void loadConfig(fs::FS &fs, const char * path) {
  display_print(F("Loading Config: "));
  display_print(path);

  File file = fs.open(path);
    if(!file || file.isDirectory()){
        display_println(F("- failed to open file for reading"));
        return;
    }

    byte counter1 = 0;

    String temp_name; // lazy and using strings
    String temp_value;
    
    while(file.available()){

        byte temp = file.read();

        if(temp == ',') {
          counter1++;
        } else if(temp == '\n') {
          // run an interpretation.
          assign_config(temp_name, temp_value);
          temp_name = ""; // reset our variables.
          temp_value = "";
          counter1 = 0;
        } else if (temp == '\r') {
          // skip carriage return
        } else if(counter1 == 0) {
          // append to the service name.
          temp_name += char(temp);
        } else if (counter1 == 1) {
          // append to the variable.
          temp_value += char(temp);
        }

        //display_print(F("."));
    }

    display_println(F(". Done."));

}

void loadStars(fs::FS &fs, const char * path){
    display_print(F("Loading Stars: "));
    display_print(path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        display_println(F("- failed to open file for reading"));
        return;
    }

    // this is very specific to loading the stars.

    byte counter1 = 0;
    star_counter = 0;
    int positive = 1;
    int char_pos = 0;
    byte point = 0; // this is a decimal point.
    char temp_mag[10];
    
    while(file.available()){

        byte temp = file.read();

        if(temp == ',') {
          counter1++;
        } else if(temp == '\n') {
          star_array[star_counter].name[char_pos] = '\0';  // terminate our name nicely
          // we've got everything we need.  Link to a LED.
          star_array[star_counter].led = &leds[star_array[star_counter].pin][star_array[star_counter].point];

          char_pos = 0;
          counter1 = 0;
          point = 0;
          star_counter++;
          positive = 1;
        } else if (temp == '\r') {
          // skip carriage return
        } else if(counter1 == 0) {
          // append to the name
          star_array[star_counter].name[char_pos] = temp; // this is probably stupid.
          char_pos++;
        } else if (counter1 == 1) {
          star_array[star_counter].constellation = star_array[star_counter].constellation*10 + (temp-48); // sets a byte reference
          // we've linked this star to the constellation.
          // Lets update the constellation array to include this star too.
          constellation_array[star_array[star_counter].constellation].star_list[constellation_array[star_array[star_counter].constellation].star_count] = &star_array[star_counter];
          constellation_array[star_array[star_counter].constellation].star_count++;
        } else if (counter1 == 2) {
          if(temp == 46) {
            point = 10;
          } else if (temp == 45) {
            positive = -1;
          } else if (point) {
            star_array[star_counter].magnitude = star_array[star_counter].magnitude + (positive*(float(temp-48)/point));
            point = point * 10;
          } else
            star_array[star_counter].magnitude = star_array[star_counter].magnitude*10 + (positive*(temp-48));

        } else if (counter1 == 3) {
          star_array[star_counter].pin = temp-48;
        } else if (counter1 == 4) {
          star_array[star_counter].point = star_array[star_counter].point*10 + (temp-48);
        }
    }

    display_print(F(". Done: "));
    display_println(String(star_counter));

      /*
      Serial.println(star_array[3].name);
      Serial.println(star_array[3].constellation);
      Serial.println(star_array[3].magnitude);
      Serial.println(star_array[3].pin);
      Serial.println(star_array[3].point);
      */
      
      //*star_array[3].led = CRGB::Red;  // this is how we change a star colour.
}

void loadConstellations(fs::FS &fs, const char * path){
    display_print(F("Loading Constellations: "));
    display_print(path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        display_println(F("- failed to open file for reading"));
        return;
    }

    constellation_counter = 0;
    int char_pos = 0;
    
    while(file.available()){

        byte temp = file.read();

        if(temp == '\n') {
          // end of entry.
          constellation_array[constellation_counter].name[char_pos] = '\0';
          char_pos = 0;
          constellation_counter++;
         } else if (temp == '\r') {
          // skip carriage return
        } else {
          constellation_array[constellation_counter].name[char_pos] = temp;
          char_pos++;
        }
    }

    display_print(F(". Done: "));
    display_println(String(constellation_counter));

    /*
      Serial.println(constellation_array[2].name); // Triangulum
      for(int i = 0 ; i < constellation_array[2].star_count; i++) {
        Serial.println(constellation_array[2].star_list[i]->name);  // keep this in mind.  it's how we reference the star.
        *constellation_array[2].star_list[i]->led = CRGB::Red;
      }
      */
    
}

void loadPlanets(fs::FS &fs, const char * path){
    display_print(F("Loading Planets: "));
    display_print(path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        display_println(F("- failed to open file for reading"));
        return;
    }

    // this is very specific to loading the planets.

    byte counter1 = 0;
    planet_counter = 0;
    int char_pos = 0;
    
    while(file.available()){

        byte temp = file.read();

        if(temp == ',') {
          counter1++;
        } else if(temp == '\n') {
          planet_array[planet_counter].name[char_pos] = '\0'; // terminate our name nicely
          // at this point we have all the details we need to link to the leds array.
          planet_array[planet_counter].led = &leds[planet_array[planet_counter].pin][planet_array[planet_counter].point];
          char_pos = 0;
          counter1 = 0;
          planet_counter++;
        } else if (temp == '\r') {
          // skip carriage return
        } else if(counter1 == 0) {
          // append to the name
          planet_array[planet_counter].name[char_pos] = temp; // this is probably stupid.
          char_pos++;
        } else if (counter1 == 1) {
          planet_array[planet_counter].pin = temp-48;
        } else if (counter1 == 2) {
          planet_array[planet_counter].point = planet_array[planet_counter].point*10 + (temp-48);
        }
    }

    display_print(F(". Done: "));
    display_println(String(planet_counter));

      /*
      Serial.println(star_array[3].name);
      Serial.println(star_array[3].constellation);
      Serial.println(star_array[3].magnitude);
      Serial.println(star_array[3].pin);
      Serial.println(star_array[3].point);
      */

}
