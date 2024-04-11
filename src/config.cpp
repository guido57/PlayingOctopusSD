#include "config.h"

config_struct config;
config_struct config_default;

void init_config_default(){

  config_default.Bells[0] = { 25, 77, 112, 120, 100,200};
  config_default.Bells[1] = { 25, 79, 130, 120, 100,200};
  config_default.Bells[2] = { 26, 81,  94, 105,  20, 50};
  config_default.Bells[3] = { 26, 83, 115, 105,  30, 30};
  config_default.Bells[4] = { 27, 84,  70,  80,  70,200};
  config_default.Bells[5] = { 27, 86,  90,  80,  80,200};
  config_default.mp3_to_smf_delay_ms = 200L;
  config_default.volume = 10;
  strcpy(config_default.server_url,"http://192.168.1.232:5000");
};

void writeConfigToFile(const char *filename) {
    File file = LittleFS.open(filename, "wb"); // Open file in binary write mode

    if (!file) {
        perror("Error opening file for writing");
        return;
    }

    // Write the array of structs to the file
    //file.write( (const unsigned char *) config, sizeof(config));
    uint8_t * config_ptr = reinterpret_cast<uint8_t *>(&config);
    file.write(config_ptr , sizeof(config));
    Serial.printf("writeConfigToFile: sizeof(config)=%d\r\n", sizeof(config));

    // Close the file
    file.close();
}

void readConfigFromFile(const char *filename) {
    File file = LittleFS.open(filename, "rb"); // Open file in binary read mode
    Serial.printf("LittleFS.open(%s, 'rb').size() returned %d \r\n",filename, file.size());
    if (file.size() == 0) {
        // Handle the case when the file doesn't exist yet
        Serial.printf("File doesn't exist yet. Initializing with default values.\n");
        // You can initialize the bellArray with default values or take appropriate action.
        init_config_default();
        memcpy(&config,&config_default,sizeof(config_struct));
        Serial.printf("sizeof(config_default)=%d\r\n",sizeof(config_struct));
        writeConfigToFile(filename);
        return;
    }

    // Read the array of structs from the file
    file.readBytes((char *) &config, sizeof(config));
    Serial.printf("From %s retrieved server_url=%s\r\n", 
        filename, config.server_url);
    // Close the file
    file.close();
}
