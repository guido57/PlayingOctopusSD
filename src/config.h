#ifndef __BELL_STRUCT__
#define __BELL_STRUCT__


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
  long mp3_to_smf_delay_ms;
  int volume;
  char server_url[40];
};

#endif