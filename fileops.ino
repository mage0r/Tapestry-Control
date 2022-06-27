void setup_fileops() {
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      display_println("SPIFFS Mount Failed");
      return;
  }

  display_println(F("SPIFFS Initialised."));

  // declaring these here let me load them in to PSRAM
  star_array = (stars *) ps_calloc(843, sizeof(stars));
  constellation_array = (constellations *) ps_calloc(105, sizeof(constellations));
  fortune_array = (fortunes *) ps_calloc(35, sizeof(fortunes));
  
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

    file.close();

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

          // We need the array position when we write our sessions and there's no other way to get it.
          star_array[star_counter].number = star_counter;

          // we've linked this star to the constellation.
          // Lets update the constellation array to include this star too.
          constellation_array[star_array[star_counter].constellation].star_list[constellation_array[star_array[star_counter].constellation].star_count] = &star_array[star_counter];
          constellation_array[star_array[star_counter].constellation].star_count++;

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

    file.close();

    display_print(F(". Done: "));
    display_println(String(star_counter));


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

    file.close();

    display_print(F(". Done: "));
    display_println(String(constellation_counter));
    
}

void loadSession(fs::FS &fs, const char * path){

    // Renamed from LoadAnimation as Session better describes what we do.
    // <SessionID>,<RED>,<GREEN>,<BLUE>,<LED #>,<TIMES>,<SHOW>

  
    display_print(F("Loading Sessions: "));
    display_print(path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
      display_println(F("- failed to open file for reading"));
      display_print(F("Loading Backup File: "));
      fs.rename("/ani_backup.csv", path);
      file.close();
      file = fs.open(path);
      if(file)
        display_print("Success");
      else {
        display_println("Fail");
        return;
      }
    }

    // this is very specific to loading the Animations.

    byte counter1 = 0; // this counts out the length of each line.

    // collect variables.
    int temp_animation_id = 0;
    int temp_star = 0;
    byte temp_colour[3] = {0,0,0};
    int temp_times = 0;
    int temp_show = 0;
    long animation_lines = 0; // potentially 100*2000 = 200,000.  int only allows 65535

    animation_counter = 0; // make sure this has been reset to 0
    max_animation_counter = 0;
    
    
    while(file.available()){

        byte temp = file.read();

        if(temp == ',') {
          counter1++;
        } else if(temp == '\n') {
          // line complete.
          
          // just check the animation is clear.
          // this breaks things
          //animation_array[temp_animation_id].count = 0;

          if(DEBUG && false) {
            Serial.println();
            Serial.print(String(temp_animation_id));
            Serial.print(",");
            Serial.print(String(temp_colour[0]));
            Serial.print(",");
            Serial.print(String(temp_colour[1]));
            Serial.print(",");
            Serial.print(String(temp_colour[2]));
            Serial.print(",");
            Serial.print(String(temp_star));
            Serial.print(",");
            Serial.print(String(temp_times));
            Serial.print(",");
            Serial.print(String(temp_show));
          }

          append_animation(temp_animation_id, temp_colour, temp_star, temp_times, temp_show);

          // this is a bit fun.  We load animations in batches, but one line does not
          // equal one animation.  This could potentially mean empty animations but
          // they will fill as the system recycles.
          if(temp_animation_id > animation_counter)
            animation_counter = temp_animation_id;

          if(temp_animation_id > max_animation_counter)
            max_animation_counter = temp_animation_id;

          counter1 = 0;
          temp_star = 0;
          temp_animation_id = 0;
          temp_colour[0] = 0;
          temp_colour[1] = 0;
          temp_colour[2] = 0;
          temp_times = 0;
          temp_show = 0;
          animation_lines++;

          if(max_animation_counter >= MAX_SESSIONS) {
            // don't accidentally overload.
            file.close();
            break;
          }

          
        } else if (temp == '\r') {
          // skip carriage return
        } else if(counter1 == 0) {
          temp_animation_id = temp_animation_id*10 + (temp-48); // Which session/animation
        } else if (counter1 == 1) {
          temp_colour[0] = temp_colour[0]*10 + (temp-48); // RED
        } else if (counter1 == 2) {
          temp_colour[1] = temp_colour[1]*10 + (temp-48); // GREEN
        } else if (counter1 == 3) {
          temp_colour[2] = temp_colour[2]*10 + (temp-48); // BLUE
        } else if (counter1 == 4) {
          temp_star = temp_star*10 + (temp-48); // this is our LED.
        } else if (counter1 == 5) {
          temp_times = temp_times*10 + (temp-48); // Time between lighting up stars
        } else if (counter1 == 6) {
          temp_show = temp_show*10 + (temp-48); // how long to hold the light up.
        }
        
    }

    // If we don't advance these, the next time we try to access them we
    // will be adding to the existing int.
    if(animation_lines > 0) {
      animation_counter++;
      max_animation_counter++;

      if(animation_counter >= MAX_SESSIONS)
        animation_counter = 0;

      if(max_animation_counter < MAX_SESSIONS)
        max_animation_counter++;

    }

    

    //Serial.print("Animation_counter: ");
    //Serial.println(animation_counter);
    //Serial.print("max_animation_counter: ");
    //Serial.println(max_animation_counter);

    file.close();

    //display_print(F(". Done: "));
    display_print(F(". Done: "));
    //display_println(String(animation_counter));
    display_println(String(max_animation_counter));

}

// We can only store 500 of these locally
// so we delete and re-create the file ever 30 minutes(?)
void saveSession(fs::FS &fs, const char * path){

  File file;
  
  // we run this command frequently and nibble away at our session array
  // don't want to be blocking.
  if(save_counter == 0) {
    // save_counter of 0 means we're starting a new save session.
    if(DEBUG) {
      //display_print(F("Saving Session data: "));
      Serial.print(millis());
      Serial.print(F(":Saving Session data: "));
      //display_print(path);
      Serial.println(path);
    }
    
    fs.remove("/ani_bck.csv"); // first clear out our backup file.

    // take a copy.
    if(!fs.rename(path, "/ani_bck.csv")){
      // rename failed.  Terminate the function.
        Serial.println(F("Save failed"));
        return;
    } // else, the file has been deleted and we can continue.

    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println(F("Open failed"));
        return;
    }

  } else {
    // open the file for appending. We have a backup and are in the process of writing.
    file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println(F("Append failed"));
        return;
    }
  }

  for(int j = 0 ; j < animation_array[save_counter].count; j++) {
    String temp_message;

    temp_message += save_counter;
    temp_message += ',';
    temp_message += animation_array[save_counter].colour[j][0];
    temp_message += ',';
    temp_message += animation_array[save_counter].colour[j][1];
    temp_message += ',';
    temp_message += animation_array[save_counter].colour[j][2];
    temp_message += ',';
    temp_message += animation_array[save_counter].star_list[j]->number;
    temp_message += ',';
    temp_message += animation_array[save_counter].times[j];
    temp_message += ',';
    temp_message += animation_array[save_counter].show[j];
    temp_message += '\n';

    file.print(temp_message);
  }

  file.close();

  save_counter++;

  if(save_counter >= max_animation_counter) {
    // We're finished!
    // update our last saved time.
    save_animations_time = millis();
    if(DEBUG) {
      Serial.print(millis());
      Serial.print(F(":Session Data saved: "));
      Serial.println(String(save_counter));
    }
    save_counter = 0; // back to the start.
  }
}

void loadFortune(fs::FS &fs, const char * path){

    display_print(F("Loading Fortune: "));
    display_print(path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        display_println(F("- failed to open file for reading"));
        return;
    }

    // this is very specific to loading the Fortune.

    byte counter1 = 0;
    int char_pos = 0;

    // collect variables.
    int fortune_counter = 0;
     
    
    while(file.available() && fortune_counter < 120){

        byte temp = file.read();

        if(counter1 < 3 && temp == ',') {
          counter1++;
        } else if(temp == '\n') {

          fortune_array[fortune_counter].text[char_pos] = '\0';

          fortune_counter++;
          char_pos = 0;
          counter1 = 0;
          
        } else if (temp == '\r') {
          // skip carriage return
        } else if(counter1 == 0) {
          // first section is star_id
          fortune_array[fortune_counter].star = fortune_array[fortune_counter].star*10 + (temp-48);
        } else if (counter1 == 1) {
          // second section is constellation_id
          fortune_array[fortune_counter].constellation = fortune_array[fortune_counter].constellation*10 + (temp-48);
        } else if (counter1 == 2) {
          // third section is animation_id
          fortune_array[fortune_counter].animation = fortune_array[fortune_counter].animation*10 + (temp-48);
        } else if (counter1 == 3) {
          // forth section is the text.
          if(char_pos < 120) {
            fortune_array[fortune_counter].text[char_pos] = temp;
            char_pos++;
          }
        }
        
    }

    file.close();

    display_print(F(". Done: "));
    display_println(String(fortune_counter-1));

}
