#include <Arduino.h>
#include <WiFi.h>
#include <AutoConnect.h>
#include <tbservo.h>
#include "Audio.h"
#include "downloader.h"

// MIDI timed parser
#include <SdFat.h>
//#include <FileSPIFF.h>
#include <MD_MIDIFile.h>
#define USE_MIDI  0  // set to 1 for MIDI output, 0 for debug output

#if USE_MIDI // set up for direct MIDI serial output

#define DEBUGS(s)
#define DEBUG(s, x)
#define DEBUGX(s, x)
#define SERIAL_RATE 31250

#else // don't use MIDI to allow printing debug statements

#define DEBUGS(s)     do { Serial.print(s); } while(false)
#define DEBUG(s, x)   do { Serial.print(F(s)); Serial.print(x); } while(false)
#define DEBUGX(s, x)  do { Serial.print(F(s)); Serial.print(x, HEX); } while(false)
#define SERIAL_RATE 115200

#endif // USE_MIDI

// SD chip select pin for SPI comms.
// Default SD chip select is the SPI SS pin (10 on Uno, 53 on Mega).
const uint8_t SD_SELECT = SS;  

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

// The files in the tune list should be located on the SD card 
// or an error will occur opening the file and the next in the 
// list will be opened (skips errors).
const char *loopfile = "LOOPDEMO.MID";  // simple and short file

SDFAT	SD_fake;
MD_MIDIFile * SMF = nullptr;

// Declare the AudioFileSource objects

#include "config.h"
config_struct config;
config_struct config_default;

void init_config_default(){

  config_default.Bells[0] = { 14, 77, 112, 120, 100,200};
  config_default.Bells[1] = { 14, 79, 130, 120, 100,200};
  config_default.Bells[2] = { 33, 81,  94, 105,  20, 50};
  config_default.Bells[3] = { 33, 83, 115, 105,  30, 30};
  config_default.Bells[4] = { 13, 84,  70,  80,  70,200};
  config_default.Bells[5] = { 13, 86,  90,  80,  80,200};
  config_default.mp3_to_smf_delay_ms = 200L;
  config_default.volume = 10;
  strcpy(config_default.server_url,"http://192.168.1.232:5000");
};

// download setting 
#define DWNL_BUFF_SIZE 300000 // to be allocated in PSRAM
//unsigned char dwnl_buffer[DWNL_BUFF_SIZE];
unsigned char * dwnl_buffer;
Downloader dwnl;

unsigned long start_mp3_ms = 0L;
unsigned long start_SMF_ms = 0L;

TBServo * mytbservo7779;                               // create servo object to control a servo
int servoPin7779 = config.Bells[0].pin;    //TMS       // the ESP32 pin where the servo is connected to              GPIO14/TMS
TBServo * mytbservo8183;           // create servo object to control a servo
int servoPin8183 = config.Bells[2].pin;    // GPIO33   // the ESP32 pin where the servo is connected to              GPIO33
TBServo * mytbservo8486;           // create servo object to control a servo
int servoPin8486 = config.Bells[4].pin;    // TCK      // the ESP32 pin where the servo is connected to              GPIO13/TCK

Audio audio;
#define I2S_LRC     25   
#define I2S_DOUT    22   
#define I2S_BCLK    26  
#define I2S_MCLK     0

void cb_play(int note){
  unsigned long milli = millis() - start_mp3_ms;
  int audio_time = audio.getTotalPlayingTime();
  Serial.printf("\r\n%lu millis  callback with note=%d mp3_current=%d diff=%d\r\n", 
    milli, note, audio_time, milli - audio_time);
  
  switch(note%12){ // note%12 means that all the notes are remapped between 77 and 87
    
    case 4:  // 76    76%12 = 4
    case 5:  // 77
    case 6:  // 78
      mytbservo7779->targetTo(&config.Bells[0]); // mapped to Bell 0
      break;
    
    case 7:  // 79 
    case 8:  // 80
      mytbservo7779->targetTo(&config.Bells[1]);
      break;
    case 9:  // 81
    case 10: // 82
      mytbservo8183->targetTo(&config.Bells[2]);
      break;
    case 11: // 83
      mytbservo8183->targetTo(&config.Bells[3]);
      break;
    case 0:  // 84
    case 1:  // 85
      mytbservo8486->targetTo(&config.Bells[4]);
      break;
    case 2:  // 86
    case 3:  // 87
      mytbservo8486->targetTo(&config.Bells[5]);
      break;
    default:
      Serial.printf("No Bell to play note %d\r\n",note);

  } 
  //Serial.printf("%lu target to %d\r\n", millis(),target);
}

int selected_track_to_be_played = -1;
void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
#if USE_MIDI
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0))
  {
    Serial.write(pev->data[0] | pev->channel);
    Serial.write(&pev->data[1], pev->size-1);
  }
  else
    Serial.write(pev->data, pev->size);
#endif

  DEBUG("", millis()-start_mp3_ms);
  
  DEBUG(" M T", pev->track);
  DEBUG(":  Ch ", pev->channel+1);
  DEBUGS(" Data");
  for (uint8_t i=0; i<pev->size; i++)
  {
    DEBUGX(" ", pev->data[i]);
  }
  DEBUG("\r\n","");

  if(pev->data[0] == 0x90 && pev->track==selected_track_to_be_played){  // Note On
    cb_play(pev->data[1]);
  }
}

void sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a system Exclusive (sysex) file event needs 
// to be processed through the midi communications interface. Most sysex events cannot 
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
{
  DEBUG("\n", millis()-start_mp3_ms);
  DEBUG(" S T", pev->track);
  DEBUGS(": Data");
  for (uint8_t i=0; i<pev->size; i++)
    DEBUGX(" ", pev->data[i]);
}

void metaCallback(const meta_event *mev)
// Called by the MIDIFile library when a META event needs 
// to be processed.
{
  DEBUG("\n", millis()-start_mp3_ms);
  DEBUG(" E T", mev->track);
  DEBUG(": type ", mev->type);
  DEBUGS("  Data");
  for (uint8_t i=0; i<mev->size; i++)
    DEBUGX(" ", mev->data[i]);

  if(mev->type == 0x51){
    DEBUG("\nSMF->getTempo()=", SMF->getTempo());  
    DEBUG("\nSMF->getTicksPerQuarterNote()=", SMF->getTicksPerQuarterNote());  
    DEBUG("\nSMF->getTimeSignature()=", SMF->getTimeSignature());  
    DEBUG("\nSMF->getTickTime()=", SMF->getTickTime());  
  }
}
uint8_t lclBPM = 120;

//#include <FS.h>
//#include <SPIFFS.h>
#include <LittleFS.h>
//fs::SPIFFSFS& FlashFS = SPIFFS;

// Include the class to handle the upload process
#include <AutoConnectUploadFS.h>
AutoConnectUploadFS uploader;

#define FORMAT_ON_FAIL  true

// Set the memory buffer where to store the uploaded file 
//#define MAX_UPL_FILE_BUFFER_SIZE 60000 // 60 KBytes
//char upl_file_buffer[MAX_UPL_FILE_BUFFER_SIZE];

// Declare and Init the AutoConnect portal server
WebServer server;
AutoConnect portal(server);

#include "HTMLpages.h"   // Add the HTML pages coded as PROGMEM
#include "instruments.h" // add the instruments names

// Declare the AutoConnectAux custom web page handlers.
AutoConnectAux auxBrowse;
AutoConnectAux auxFileSys;
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

// global variables to command start and stop mp3 and SMF
bool last_mp3_is_running = false;
bool last_SMF_is_running = false;
bool mp3_is_running = false;
bool SMF_is_running = false;

// play the mp3 and the midi
void play(String codefile, int track2play, uint8_t dac_gain){
  Serial.printf("play\r\n");

  // download mid file
  //String url_mid = "http://192.168.1.232:5000/static/" + codefile + ".mid";
  String url_mid = String(config.server_url) + "/static/" + codefile + ".mid";
  dwnl.Setup(url_mid,dwnl_buffer, DWNL_BUFF_SIZE);
  unsigned long timeout_ms = 5000 + millis(); // 5 seconds
  while(dwnl.Loop()){
    if(millis()>timeout_ms)
      break;
  }  
  if(!dwnl.Success()){
    Serial.printf("Error trying to download %s\r\n", url_mid.c_str());
    return;
  }

  Serial.printf("play: loading mid file. size=%d\r\n",dwnl.FileSize());
  
  if(SMF != nullptr){
    SMF->close();
    delete SMF;
  }
  SMF = new MD_MIDIFile();
  SMF->begin(&SD_fake);
  SMF->setMidiHandler(midiCallback);
  SMF->setSysexHandler(sysexCallback);
  SMF->setMetaHandler(metaCallback);
  //SMF.restart();
  Serial.printf("play() before load() SMF->isEOF()=%d  SMF->isPaused()=%d\r\n",SMF->isEOF(), SMF->isPaused());
  int err = SMF->load(dwnl_buffer, dwnl.FileSize()); //GG
  Serial.printf("play() after load() SMF->isEOF()=%d  SMF->isPaused()=%d\r\n",SMF->isEOF(), SMF->isPaused());
  //int err = SMF->load("/12862.mid"); //GG
  SMF->pause(true);
  
 if (err != MD_MIDIFile::E_OK)
  {
    DEBUG("\nSMF load Error ", err);
    return;
  }
  
  SMF_is_running = true; // this will start playing on loop()
  DEBUG("\nSMF->getTempo()=", SMF->getTempo());  
  DEBUG("\nSMF->getTicksPerQuarterNote()=", SMF->getTicksPerQuarterNote());  
  DEBUG("\nSMF->getTimeSignature()=", SMF->getTimeSignature());  
  DEBUG("\nSMF->getTickTime()=", SMF->getTickTime());  
  DEBUG("\nSMF->getTrackCount=", SMF->getTrackCount());  

  Serial.printf("\r\nplay: set mp3 volume at gain=%d\r\n",dac_gain);
  audio.setVolume(dac_gain);
  audio.setTone(6,6,6);
  //String url = "http://192.168.1.232:5000/output.mp3?query="+codefile;
  String url = String(config.server_url) + "/output.mp3?query="+codefile;
  Serial.printf("play: audio.connecttohost %s\r\n", url.c_str());
  audio.connecttohost(url.c_str());
  if(audio.isRunning()){
    audio.pauseResume(); // this should stop audio
    Serial.printf("play(): mp3 paused\r\n");
  }
  mp3_is_running = true; // start mp3 playing on loop()
  
}

/* 
Append all the filenames found in the SPIFFS root ("/") to the radio element 
as RadioButtons
*/
void ListAllFiles(AutoConnectRadio * radio){
  Serial.printf("SPIFFS total size: %d bytes. Used %d bytes.\r\n", LittleFS.totalBytes(), LittleFS.usedBytes());
  File root = LittleFS.open("/");
 
  File file = root.openNextFile();
 
  while(file){
      String fn = String(file.name());
      int f_size = file.size();
      //Serial.printf("FILE: %s  size: %d bytes\r\n",fn.c_str(),f_size);
      //file.readBytes(upl_file_buffer,file.size());
      file.close();
      radio->add(fn + " --------- " + String(f_size) + " bytes " );
      file = root.openNextFile();
  }
  root.close();
}

/*
    handler to update the /browse page
*/
String postBrowse(AutoConnectAux& aux, PageArgument& args) {
  // this script allows the client browser pointing to /browse to check every second 
  // if playing has ended (status = Idle)
  // and update the page consequently  

  Serial.printf("postBrowse ...\r\n");

  const char* scCopyText = R"(
    <script>
      function loadDoc() {
        const xhttp = new XMLHttpRequest();
        xhttp.onload = function() {
          if(this.responseText.startsWith("Idle")){
            window.location = window.location.href;
          }
        }
        xhttp.open("GET", "/status");
        xhttp.send();
      }
      function autoRefresh() {
        loadDoc();
      }
      setInterval('autoRefresh()', 1000);
           
    </script>
  )";
  AutoConnectElement& obj = aux["object"];
  obj.value = String(scCopyText);

  const char* html_embed_list = R"(
    <script>

      function showTracks(filecode){
             fetch(server_url.innerText + '/tracks?index='+filecode)
            .then(response => response.json())
            .then(tracks_json => {
                tracks_text = ""

                // Iterate through the JSON data
                for (var i = 0; i < tracks_json.length; i++) {
                    var track_json = tracks_json[i];
                    // Collect text from each track 
                    tracks_text = tracks_text + track_json.track + ":" + track_json.instrument+ "/" + track_json.track_name + "   ";
                }

                this_btn = document.getElementById("bt"+filecode)
                // append the tracks to the li text as a span
                const spanItem = document.createElement('span');
                spanItem.textContent = " tracks: " + tracks_text
                this_btn.insertAdjacentHTML("afterend",spanItem.outerHTML);
                this_btn.remove();
            })
            .catch(error => {
                console.error('Error:', error);
            });
      }


      function playAudio(filecode) { 
            // ask the server to get the mp3 file    
            var xhr = new XMLHttpRequest();
            xhr.open('GET', server_url.innerText + '/get_mp3?index=' + filecode, true);
            xhr.responseType = 'blob';

            xhr.onload = function() {
                if (xhr.status === 200) {
                    var sel_codefile = document.getElementById("selected_codefile");
                    sel_codefile.value = filecode;    

                    var x = document.getElementById("ss_static"); 
                    x.setAttribute("src", server_url.innerText + "/static/" + filecode + ".mp3");
                    x.play(); 
                }
            };
            xhr.send();
      } 

      function mysearch(){
        search_query = document.getElementById("search_query");
        const eventSource = new EventSource(server_url.innerText + '/events?query='+search_query.value);
        results.innerHTML = '';
        eventSource.onmessage = function(event) {

            if (event.data === 'END') {
                eventSource.close();
                return;
            }
            const result = JSON.parse(event.data);
            //const listItem = document.createElement('li');
            const inputItem = document.createElement('input');
            inputItem.id = 'radio'+result.ndx
            inputItem.type = 'radio'
            inputItem.name ='songs'
            inputItem.setAttribute('onclick', 'playAudio(' + result.filecode + ')');
            const labelItem = document.createElement('label');
            inputText = result.ndx +' - ' + result.artist.replace(/_/g,' ') + ' - '+ result.song.replace(/_/g,' ') +' - filecode: ' + result.filecode;
            results.appendChild(inputItem);
            labelItem.setAttribute("for","radio"+result.ndx);
            labelItem.innerHTML = inputText;
            results.appendChild(labelItem);
            const button_tracks = document.createElement('button');
            button_tracks.id = 'bt' + result.filecode;
            button_tracks.setAttribute('onclick', 'showTracks(' + result.filecode + ')' );  
            button_tracks.textContent = 'show tracks'; // Set button text
            results.appendChild(button_tracks);
            results.appendChild(document.createElement('br'));
        };      
      }
    </script>
    
    <audio controls id="ss_static" src = "" type="audio/mp3"></audio>
    <br/>
    <input type='button' id='btn_search' value='SEARCH' onclick='mysearch()'>
    <input type='text' id='search_query' value=''>
    <br/>
    <p id='results'></p>
    
  )";
  
  AutoConnectText& server_url = aux["server_url"].as<AutoConnectText>();
  server_url.value = String(config.server_url);

  AutoConnectSelect& volume = aux["select_volume"].as<AutoConnectSelect>();
  volume.selected = config.volume +1;
  
  AutoConnectInput& mp3_smf_delay_ms = aux["mp3_smf_delay_ms"].as<AutoConnectInput>();
  mp3_smf_delay_ms.value = String(config.mp3_to_smf_delay_ms);
  
  AutoConnectElement& embed_list = aux["embed_list"];
  embed_list.value = String(html_embed_list);
    
  return String();
}

/*
    handler to list files, upload and delete files in filesystem SPIFFS
*/
String postFileSys(AutoConnectAux& aux, PageArgument& args) {
  
  AutoConnectRadio& radio = aux["radio"].as<AutoConnectRadio>();
  radio.empty();
  ListAllFiles(&radio);
  AutoConnectText& status = aux["status"].as<AutoConnectText>();
  status.value = "Free RAM: " + String(esp_get_free_heap_size()) + " bytes"  
       + " ---- Used SPIFFS: " + String(LittleFS.usedBytes()) + " / " + String(LittleFS.totalBytes()) + " bytes.";
  
  return String();
}

/*
    handler to edit and save bells' configuration
*/
String postAuxBells(AutoConnectAux& aux, PageArgument& args) {
  
  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();
  Serial.printf("postAuxBells: the selected_bell is %d\r\n",selected_bell.selected);

  if(selected_bell.selected >0){
    AutoConnectText& pin = auxBells["pin"].as<AutoConnectText>();
    pin.value = String(config.Bells[selected_bell.selected-1].pin);
    AutoConnectText& note = auxBells["note"].as<AutoConnectText>();
    note.value = String(config.Bells[selected_bell.selected-1].note);
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    target.value = String(config.Bells[selected_bell.selected-1].target);
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    rest.value = String(config.Bells[selected_bell.selected-1].rest);
    AutoConnectText& target_time_ms = auxBells["target_time"].as<AutoConnectText>();
    target_time_ms.value = String(config.Bells[selected_bell.selected-1].target_time_ms);
    AutoConnectText& rest_time_ms = auxBells["rest_time"].as<AutoConnectText>();
    rest_time_ms.value = String(config.Bells[selected_bell.selected-1].rest_time_ms);
  }

  // this script allows to /bells to show the 
  // settings of the selected bell without relaoding the page 
  const char* scCopyText = R"(
    <script>
      function loadBell(bell_index) {
        const xhttp = new XMLHttpRequest();
        xhttp.onload = function() {
          bell_settings = this.responseText.split(",");
          document.getElementById("pin").value = bell_settings[0];          
          document.getElementById("note").value = bell_settings[1];          
          document.getElementById("target").value = bell_settings[2];          
          document.getElementById("rest").value = bell_settings[3];          
          document.getElementById("target_time").value = bell_settings[4];          
          document.getElementById("rest_time").value = bell_settings[5];          
        }
        xhttp.open("GET", "/download?bell=" + bell_index);
        xhttp.send();
      }
      
      // add an onchange event to select_bell 
      var select_bell_obj = document.getElementById("select_bell"); 
      select_bell_obj.addEventListener(
            'change',
            function() { 
              loadBell(select_bell_obj.selectedIndex);  
            },
            false
      );
      loadBell(select_bell_obj.selectedIndex);
       
    </script>
  )";
  AutoConnectElement& obj = aux["object"];
  obj.value = String(scCopyText);
 
  return String();
}


/*
    handler to play a file
*/
String postAuxPlay(AutoConnectAux& aux, PageArgument& args) {
  
  AutoConnectButton& btn_play = auxBrowse["play"].as<AutoConnectButton>();

  AutoConnectInput& selected_codefile = auxBrowse["selected_codefile"].as<AutoConnectInput>();
  String filecode = selected_codefile.value;
  Serial.printf("Selected filecode=%s\r\n", filecode.c_str());
  
  AutoConnectSelect&  selected_volume = auxBrowse["select_volume"].as<AutoConnectSelect>();
  String selected_volume_str = selected_volume.value();
  Serial.printf("Selected volume=%s\r\n", selected_volume_str.c_str());
  config.volume = selected_volume_str.toInt();
  
  AutoConnectInput& mp3_smf_delay_ms = auxBrowse["mp3_smf_delay_ms"].as<AutoConnectInput>();
  String delay_ms_str = mp3_smf_delay_ms.value;
  Serial.printf("mp3_smf_delay_ms=%s\r\n", delay_ms_str.c_str());
  config.mp3_to_smf_delay_ms = delay_ms_str.toInt();

  writeConfigToFile("/config.json");// save mp3_smf_delay_ms and volume

  AutoConnectSelect&  selected_track = auxBrowse["select_track"].as<AutoConnectSelect>();
  String selected_track_str = selected_track.value();
  Serial.printf("Selected track=%s\r\n", selected_track_str.c_str());
  selected_track_to_be_played = selected_track_str.toInt();  

  //play(filename, selected_track_str.toInt(),selected_volume_str.toFloat());  
  if(btn_play.value == "PLAY"){
    play(filecode, selected_track_str.toInt(),selected_volume_str.toInt());  
    btn_play.value = "STOP";
  }else{
    mp3_is_running = false; // mp3 will be stopped on loop() 
    SMF_is_running = false; // SMF will be stopped on loop()
    btn_play.value = "PLAY";
  }
  Serial.printf("postAuxPlay: redirect to /browse\r\n");
  aux.redirect("/browse");
  return String();
}

/*
    handler to download a bell configuration as a csv string
*/
String postAuxDownloadBell(AutoConnectAux& aux, PageArgument& args) {
  
  if(args.hasArg("bell")){
    int bell_ndx = args.arg("bell").toInt();
    
    if(bell_ndx >=0 && bell_ndx <= 5){
      String sd = String(config.Bells[bell_ndx].pin)   + "," + 
                  String(config.Bells[bell_ndx].note)   + "," + 
                  String(config.Bells[bell_ndx].target) + "," + 
                  String(config.Bells[bell_ndx].rest)   +  "," + 
                  String(config.Bells[bell_ndx].target_time_ms) + "," + 
                  String(config.Bells[bell_ndx].rest_time_ms);

      server.send(200, "text/html", sd );
    }
    else
      server.send(200, "text/html", "Please, specify a valid bell index (n=0...5)   /download?bell=n ");
  }else{
      server.send(200, "text/html", "Please, specify a valid bell index (n=0...5)   /download?bell=n ");
  }

  return String();
}


/*
    handler to save a bell by the /bells page
*/
String postAuxSaveBell(AutoConnectAux& aux, PageArgument& args) {

  // copy the values from the page /bells to the struct Bells
  Serial.printf("postAuxSaveBell:\r\n");
    
  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();
  
  if(selected_bell.selected >0){
    Serial.printf("postAuxSaveBell: the selected_bell which will be saved to disk is %d\r\n",selected_bell.selected);
    AutoConnectText& pin = auxBells["pin"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].pin = pin.value.toInt();
    AutoConnectText& note = auxBells["note"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].note = note.value.toInt();
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].target = target.value.toInt();
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].rest = rest.value.toInt();
    AutoConnectText& target_time_ms = auxBells["target_time"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].target_time_ms = target_time_ms.value.toInt();
    AutoConnectText& rest_time_ms = auxBells["rest_time"].as<AutoConnectText>();
    config.Bells[selected_bell.selected-1].rest_time_ms = rest_time_ms.value.toInt();
    // Simply save to file
    writeConfigToFile("/config.json");
  }

  aux.redirect("/bells");
  return String();
}

/*
    handler to test a bell moving the mallet to the target position
*/
String postAuxTargetBell(AutoConnectAux& aux, PageArgument& args) {

  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();

  if(selected_bell.selected >0){
    Serial.printf("postAuxtargetBell: test target the selected_bell to test the target position \r\n");
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    int target_int = target.value.toInt();
    Serial.printf("postAuxtargetBell: test target %d for bell %d\r\n", target_int, selected_bell.selected-1);
    switch(selected_bell.selected){
      case 1:
      case 2: 
        mytbservo7779->servo->write(target_int);
        break;
      case 3:
      case 4: 
        mytbservo8183->servo->write(target_int);
        break;
      case 5:
      case 6: 
        mytbservo8486->servo->write(target_int);
        break;
    }
  }

  aux.redirect("/bells");
  return String();
}

/*
    handler to test a bell moving the mallet to the rest position
*/
String postAuxRestBell(AutoConnectAux& aux, PageArgument& args) {

  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();

  if(selected_bell.selected >0){
    Serial.printf("postAuxRestBell: test rest the selected_bell to test the rest position \r\n");
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    int rest_int = rest.value.toInt();
    Serial.printf("postAuxRestBell: test rest %d for bell %d\r\n", rest_int, selected_bell.selected-1);
    switch(selected_bell.selected){
      case 1:
      case 2: 
        mytbservo7779->servo->write(rest_int);
        break;
      case 3:
      case 4: 
        mytbservo8183->servo->write(rest_int);
        break;
      case 5:
      case 6: 
        mytbservo8486->servo->write(rest_int);
        break;
    }
  }

  aux.redirect("/bells");
  return String();
}

/*
    handler to hit a bell 
*/
String postAuxTestBell(AutoConnectAux& aux, PageArgument& args) {

  AutoConnectSelect& selected_bell = auxBells["select_bell"].as<AutoConnectSelect>();

  if(selected_bell.selected >0){
    Serial.printf("postAuxTestBell: hit the selected_bell \r\n");
    bell_struct bs;
    AutoConnectText& target = auxBells["target"].as<AutoConnectText>();
    bs.target = target.value.toInt();
    AutoConnectText& rest = auxBells["rest"].as<AutoConnectText>();
    bs.rest = rest.value.toInt();
    AutoConnectText& target_time = auxBells["target_time"].as<AutoConnectText>();
    bs.target_time_ms = target_time.value.toInt();
    AutoConnectText& rest_time = auxBells["rest_time"].as<AutoConnectText>();
    bs.rest_time_ms = rest_time.value.toInt();
    
    Serial.printf("postAuxtargetBell: hit bell %d\r\n", selected_bell.selected-1);
    switch(selected_bell.selected){
      case 1:
      case 2: 
        mytbservo7779->targetTo(&bs);
        break;
      case 3:
      case 4: 
        mytbservo8183->targetTo(&bs);;
        break;
      case 5:
      case 6: 
        mytbservo8486->targetTo(&bs);;
        break;
    }
  }

  aux.redirect("/bells");
  return String();
}


/*
    handler to delete a file
*/
String postAuxDelete(AutoConnectAux& aux, PageArgument& args) {
  Serial.printf("postAuxDelete: ...\r\n");
  
  AutoConnectElement * rr = auxFileSys.getElement("radio");
  if( rr == nullptr ){
    Serial.printf("postAuxDelete: no radio buttons\r\n");
    aux.redirect("/spiffs");
    return String();
  }
  
  AutoConnectRadio&  radio = auxFileSys["radio"].as<AutoConnectRadio>();
  if(radio.checked>0){ // 0 means no radio button pushed
    String filename = radio[radio.checked-1];
    filename = "/" + filename.substring(0, filename.indexOf(" "));
    Serial.printf("postAuxDelete: deleting %s\r\n",filename);
    LittleFS.remove(filename);
  }
  aux.redirect("/spiffs");
  return String();
}

/*
    handler to save the server_url
*/
String postAuxSaveServerUrl(AutoConnectAux& aux, PageArgument& args) {
  Serial.printf("postAuxSaveServerUrl: ...\r\n");
  
  
  AutoConnectInput&  server_url = auxFileSys["server_url"].as<AutoConnectInput>();
  strcpy(config.server_url, server_url.value.c_str());
  Serial.printf("Saving server_url=%s\r\n",config.server_url);
  writeConfigToFile("/config.json");

  aux.redirect("/spiffs");
  return String();
}

/*
    handler to set the audio volume
*/
String postAuxVolume(AutoConnectAux& aux, PageArgument& args) {
  
  AutoConnectElement * vol = auxBrowse.getElement("select_volume");
  if( vol == nullptr )
    return String();
  
  AutoConnectSelect&  selected_volume = auxBrowse["select_volume"].as<AutoConnectSelect>();
  String selected_volume_str = selected_volume.value();
  Serial.printf("Selected volume=%s\r\n", selected_volume_str.c_str());
  audio.setVolume(selected_volume_str.toInt());
  
  aux.redirect("/browse");
  return String();
}

/*
    handler to redirect to /browse
*/
String postAuxHome(AutoConnectAux& aux, PageArgument& args) {
  
  aux.redirect("/browse");
  return String();
}

// Here, /status handler
void onStatus() {

  String status = ""; // midi == nullptr ? "Idle" : "Playing";

  // Echo back the value
  //String echo = "<html><p style=\"color:blue;font-family:verdana;font-size:300%;\">" + feel + String(" and a bold world!</p></html>");
  server.send(200, "text/html", status);
}

void setup()
{
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting up...\n");

  String st = "Free RAM: " + String(esp_get_free_heap_size()) + " bytes"  
       + " ---- Used SPIFFS: " + String(LittleFS.usedBytes()) + " / " + String(LittleFS.totalBytes()) + " bytes.";
  Serial.printf("%s\r\n",st.c_str());

  // Start the filesystem
  LittleFS.begin(FORMAT_ON_FAIL);

  st = "Free RAM after FlashFS: " + String(esp_get_free_heap_size()) + " bytes"  
       + " ---- Used SPIFFS: " + String(LittleFS.usedBytes()) + " / " + String(LittleFS.totalBytes()) + " bytes.";
  Serial.printf("%s\r\n",st.c_str());

  dwnl_buffer = (unsigned char *) ps_malloc(DWNL_BUFF_SIZE);
  if(!dwnl_buffer)
    Serial.printf("ERROR allocating %d bytes for dwnl_buffer\r\n");

  st = "Free RAM after malloc: " + String(esp_get_free_heap_size()) + " bytes"  
       + " ---- Used SPIFFS: " + String(LittleFS.usedBytes()) + " / " + String(LittleFS.totalBytes()) + " bytes.";
  Serial.printf("%s\r\n",st.c_str());

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
  auxBrowse.on(postBrowse);
  auxFileSys.load(PAGE_SPIFFS);
  auxFileSys.on(postFileSys);
  auxFileSys.onUpload(uploader);
  auxPlay.load(PAGE_PLAY);
  auxPlay.on(postAuxPlay);    
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

  portal.join({auxBrowse, auxPlay, auxDelete, auxSaveServerUrl,auxFileSys, 
                auxBells,auxSaveBell,
                auxTestBellTarget, auxTestBellRest, auxTestBell, auxDownloadBell});
  // serve the page /status directly from WebServer (wtihout using AutoConnect)
  server.on("/status",onStatus);
    
  if(portal.begin()){
    Serial.printf("Started IP: %s\r\n", WiFi.localIP().toString().c_str());
  }

  st = "Free RAM after AutoConnect: " + String(esp_get_free_heap_size()) + " bytes"  
       + " ---- Used SPIFFS: " + String(LittleFS.usedBytes()) + " / " + String(LittleFS.totalBytes()) + " bytes.";
  Serial.printf("%s\r\n",st.c_str());


  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(3); // 0...21
  audio.setConnectionTimeout(5000, 2700);
  //audio.connecttohost("http://us3.internet-radio.com:8342/stream");
  // audio.connecttohost(URL);
  Serial.printf("audio.inBufferSize=%d audio.inBufferFilled()=%d audio.inBufferFree()=%d bytes\r\n", 
        audio.inBufferSize(), audio.inBufferFilled(), audio.inBufferFree());

  st = "Free RAM after Audio: " + String(esp_get_free_heap_size()) + " bytes"  
       + " ---- Used SPIFFS: " + String(LittleFS.usedBytes()) + " / " + String(LittleFS.totalBytes()) + " bytes.";
  Serial.printf("%s\r\n",st.c_str());


}

long print_interval_ms = 1000;
long last_print_ms = 0;
void loop()
{
  static int lastms = 0;

  audio.loop();

  // ------------------------------------------
  // Start and Stop MP3
  // ------------------------------------------
  if(mp3_is_running && !last_mp3_is_running){
    // play() requested to start playing
    audio.pauseResume(); // this starts playing
    start_mp3_ms = millis();
    Serial.printf("mp3 audio started at %lu\r\n",start_mp3_ms);
    last_mp3_is_running = true;
  }else if(!mp3_is_running && last_mp3_is_running){
    // play() requested to stop playing
    audio.pauseResume(); // this stops playing
    Serial.printf("mp3 audio stopped at %lu\r\n",millis());
    last_mp3_is_running = false;
  }
  // ------------------------------------------
  // Start and Stop SMF i.e. midi
  // ------------------------------------------
  if(SMF!=nullptr && SMF_is_running && !last_SMF_is_running && millis() > start_mp3_ms + config.mp3_to_smf_delay_ms){
    // play() requested to start playing
    SMF->pause(false); // this starts playing
    start_SMF_ms = millis();
    Serial.printf("SMF audio started at %lu  SMF_delay_ms=%lu\r\n", 
          start_SMF_ms, config.mp3_to_smf_delay_ms);
    last_SMF_is_running = true;
  }else if(SMF!=nullptr && !SMF_is_running && last_SMF_is_running){
    // play() requested to stop playing
    SMF->pause(true); // this stops playing
    Serial.printf("SMF audio stopped at %lu\r\n",millis());
    last_SMF_is_running = false;
  }
  
  // play the MIDI file
  if( SMF!=nullptr){

    if (!SMF->isPaused()  && !SMF->isEOF())
    {
      SMF->getNextEvent();
    }
  }
  
  mytbservo7779->update();
  mytbservo8183->update();
  mytbservo8486->update();

  portal.handleClient();
}
