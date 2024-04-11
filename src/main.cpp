#include <Arduino.h>
#include <WiFi.h>
#include <AutoConnect.h>
#include <tbservo.h>
#include <Audio.h>
#include <SD_MMC.h>            // SD Card ESP32
#include <LittleFS.h>
#include "HTMLpages.h"   // Add the HTML pages coded as PROGMEM
#include "instruments.h" // add the instruments names
#include "MD_MIDIFile.h" // MIDI timed parser


// Declare and Init the AutoConnect portal server
WebServer server;
AutoConnect portal(server);

// Declare the AutoConnectAux custom web page handlers.
AutoConnectAux auxBrowse;
AutoConnectAux auxFileSys;
AutoConnectAux auxFileSD;
AutoConnectAux auxPlay;
AutoConnectAux auxDelete;
AutoConnectAux auxSaveServerUrl;
//AutoConnectAux auxVolume;
AutoConnectAux auxBells;
AutoConnectAux auxSaveBell;
AutoConnectAux auxTestBell;
AutoConnectAux auxTestBellTarget;
AutoConnectAux auxTestBellRest;
AutoConnectAux auxDownloadBell;

// Audio definitions
// Digital I/O used
#define I2S_DOUT      21
#define I2S_BCLK      23
#define I2S_LRC       22
Audio  audio;

#include "config.h"

TBServo * mytbservo7779;           // create servo object to control a servo
int servoPin7779 = config.Bells[0].pin;    
TBServo * mytbservo8183;           // create servo object to control a servo
int servoPin8183 = config.Bells[2].pin; 
TBServo * mytbservo8486;           // create servo object to control a servo
int servoPin8486 = config.Bells[4].pin;      

#include "azmin.h"   // all the functions to manage azmin.json
//#include "search.h"   // search inside azmin.json
#include "FileSys.h" // all the functions for the FileSys web page 
#include "Browse.h"  // all the functions for the Browse web page
#include "FileSD.h"  // all the functions for the FileSD web page
#include "Bells.h"   // all the functions for the Bells web page

#define FORMAT_ON_FAIL  true


void setup() {
  
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...\n");
  
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Start the LittleFS filesystem
  if(!LittleFS.begin(FORMAT_ON_FAIL))
    Serial.printf("Error initializing LittleFS\r\n");

  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("SD Card Found!");
  
  dwnl_buffer = (unsigned char *) ps_malloc(DWNL_BUFF_SIZE);
  if(!dwnl_buffer)
    Serial.printf("ERROR allocating %d bytes in PSRAM for dwnl_buffer\r\n");

  readConfigFromFile("/config.json");

  // Set the three Servo Motors
  mytbservo8486 = new TBServo(config.Bells[4].pin);
  mytbservo8183 = new TBServo(config.Bells[2].pin);
  mytbservo7779 = new TBServo(config.Bells[0].pin);
  mytbservo7779->targetTo(config.Bells[0].rest,config.Bells[0].rest,config.Bells[0].rest_time_ms,config.Bells[0].rest_time_ms);
  mytbservo8183->targetTo(config.Bells[2].rest,config.Bells[2].rest,config.Bells[2].rest_time_ms,config.Bells[2].rest_time_ms);
  mytbservo8486->targetTo(config.Bells[4].rest,config.Bells[4].rest,config.Bells[4].rest_time_ms,config.Bells[4].rest_time_ms);

  // Attach the custom web pages
  portal.load(PAGE_ROOT);
  portal.on("/",postAuxHome);
  auxBrowse.load(PAGE_BROWSE);
  auxBrowse.on(onPostBrowse);
  auxFileSys.load(PAGE_SPIFFS);
  auxFileSys.on(postFileSys);
  auxFileSys.onUpload(uploader);
  auxFileSD.load(PAGE_SD);
  auxFileSD.on(postFileSD);
  //auxPlay.load(PAGE_PLAY);
  //auxPlay.on(postAuxPlay);    
  auxDelete.load(PAGE_DELETE);
  auxDelete.on(postAuxDelete);      
  auxSaveServerUrl.load(PAGE_SAVE_SERVER_URL);
  auxSaveServerUrl.on(postAuxSaveServerUrl);      
  auxBells.load(PAGE_BELLS);
  auxBells.on(postAuxBells);
  auxSaveBell.load(SAVE_BELL);
  auxSaveBell.on(postAuxSaveBell);      
  auxTestBell.load(TEST_BELL);
  auxTestBell.on(postAuxTestBell);  
  auxTestBellTarget.load(TEST_BELL_TARGET);
  auxTestBellTarget.on(postAuxTargetBell);  
  auxTestBellRest.load(TEST_BELL_REST);
  auxTestBellRest.on(postAuxRestBell);  
  auxDownloadBell.load(PAGE_DOWNLOAD);
  auxDownloadBell.on(postAuxDownloadBell);

  portal.join({auxBrowse, auxDelete, auxSaveServerUrl,auxFileSys , auxFileSD,
                auxBells ,auxSaveBell, auxTestBellTarget, auxTestBellRest, auxTestBell, auxDownloadBell});
  // serve the page /status directly from WebServer (wtihout using AutoConnect)
  server.on("/status",onStatus);
  server.on("/events",HTTP_GET,onSearchSongs);
  server.on("/sddir",HTTP_GET,onSdDir);
  server.on("/sddel",HTTP_GET,onSdDel);
  server.on("/get_mp3",onGetMP3);
  server.on("/play",onPlay);
  server.on("/fupload",  HTTP_POST,handleFileUploadToSDFields, handleFileUploadToSD);
  server.on("/saveconfigs",HTTP_POST,onPostConfigs);

  if(portal.begin()){
    Serial.printf("Started IP: %s\r\n", WiFi.localIP().toString().c_str());
  }
}


void loop()
{
  
  audio.loop();

  // ------------------------------------------
  // Start and Stop MP3
  // ------------------------------------------
  if( mp3_is_running && !last_mp3_is_running){
    start_mp3_ms = millis();
    Serial.printf("MP3 audio started at %lu\r\n",start_mp3_ms);
    if(audio.isRunning() == false){
      Serial.printf("loop: audio didn't start.\r\n");
    }
    last_mp3_is_running = true;
  }else if( !mp3_is_running && last_mp3_is_running){
    // play() requested to stop playing
    audio.stopSong();
    Serial.printf("mp3 audio stopped at %lu\r\n",millis());
    last_mp3_is_running = false;
  }
  // ------------------------------------------
  // Start and Stop SMF (MIDI)
  // ------------------------------------------
  if(SMF!=nullptr && SMF_is_running && !last_SMF_is_running && millis() > start_mp3_ms + config.mp3_to_smf_delay_ms){
    SMF->pause(false);        // this starts playing
    start_SMF_ms = millis();
    Serial.printf("SMF audio started at %lu  SMF_delay_ms=%lu\r\n", 
          start_SMF_ms, config.mp3_to_smf_delay_ms);
    last_SMF_is_running = true;
  }else if(SMF!=nullptr && !SMF_is_running && last_SMF_is_running){
    SMF->pause(true); // this stops playing
    Serial.printf("SMF audio stopped at %lu\r\n",millis());
    last_SMF_is_running = false;
  }
  
  // loop play the MIDI file
  if( SMF!=nullptr)
    if (!SMF->isPaused()  && !SMF->isEOF())
      SMF->getNextEvent();
  
  // loop to run tbservos' delays
  mytbservo7779->update();
  mytbservo8183->update();
  mytbservo8486->update();

  portal.handleClient();
}
