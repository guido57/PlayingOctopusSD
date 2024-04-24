#include <Arduino.h>
#include <LittleFS.h>

#ifndef __CONFIG_H__
#define __CONFIG_H__


struct bell_struct {
    int pin;
    int note;
    int target;
    int rest;
    int target_time_ms;
    int rest_time_ms;
  };

struct config_struct {
  
  bell_struct Bells[6];
  int mp3_to_smf_delay_ms;
  int volume;
  char server_url[40];
};

extern config_struct config;
extern config_struct config_default;

void writeConfigToFile(const char *filename);
void readConfigFromFile(const char *filename);


#endif

