

int selected_track_to_be_played = -1;

// download setting 
#define DWNL_BUFF_SIZE 500000 // to be allocated in PSRAM
unsigned char * dwnl_buffer;

#define USE_MIDI  0  // set to 1 for MIDI output, 0 for debug output
#if USE_MIDI // set up for direct MIDI serial output

#define DEBUGS(s)
#define DEBUG(s, x)
#define DEBUGX(s, x)

#else // don't use MIDI to allow printing debug statements

#define DEBUGS(s)     do { Serial.print(s); } while(false)
#define DEBUG(s, x)   do { Serial.print(F(s)); Serial.print(x); } while(false)
#define DEBUGX(s, x)  do { Serial.print(F(s)); Serial.print(x, HEX); } while(false)
#define SERIAL_RATE 115200

#endif // USE_MIDI

// global variables to command start and stop mp3 and SMF
bool last_mp3_is_running = false;
bool last_SMF_is_running = false;
bool mp3_is_running = false;
bool SMF_is_running = false;
unsigned long start_mp3_ms = 0L;
unsigned long start_SMF_ms = 0L;

MD_MIDIFile * SMF = nullptr;

void cb_play(int note){
  unsigned long milli = millis() - start_mp3_ms;
  int audio_time = audio.getAudioFileDuration();
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

// play the mp3 and the midi
void play(String codefile, int track2play, uint8_t dac_gain){
  Serial.printf("play\r\n");

  // download mid file
  //String url_mid = "http://192.168.1.232:5000/static/" + codefile + ".mid";
  //String url_mid = String(config.server_url) + "/static/" + codefile + ".mid";
  //dwnl.Setup(url_mid,dwnl_buffer, DWNL_BUFF_SIZE);
  //Serial.printf("play: loading mid file. size=%d\r\n",dwnl.FileSize());
  unsigned long timeout_ms = 5000 + millis(); // 5 seconds
  
  // copy the mid file to psram
  String fnmid = "/" + String(codefile[0]) + "/" + codefile + ".mid";
  File fs = SD_MMC.open(fnmid);
  
  int fs_size = 0;
  while(fs.available()){
    fs_size += fs.read(dwnl_buffer,DWNL_BUFF_SIZE);
  }
  fs.close();
  if(SMF != nullptr){
    SMF->close();
    delete SMF;
  }
  SMF = new MD_MIDIFile();
  //SMF->begin(&SD_fake);
  //Serial.printf("play() before load() SMF->isEOF()=%d  SMF->isPaused()=%d\r\n",SMF->isEOF(), SMF->isPaused());
  int err = SMF->load(dwnl_buffer, fs_size); 
  //int err = SMF->load(fnmid.c_str());
  //Serial.printf("play() after load() SMF->isEOF()=%d  SMF->isPaused()=%d\r\n",SMF->isEOF(), SMF->isPaused());
  //int err = SMF->load("/12862.mid"); //GG

  String instruments = SMF->getInstruments();
  Serial.printf("Instruments=%s\r\n", instruments.c_str());

  SMF->setMidiHandler(midiCallback);
  SMF->setSysexHandler(sysexCallback);
  SMF->setMetaHandler(metaCallback);
  SMF->restart();
  SMF->dump();
  
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
  audio.setVolume(dac_gain); // 0 ... 21
  String fn = "/" + String(codefile[0]) + "/" + codefile + ".mp3";
  Serial.printf("Start playing %s\r\n",fn.c_str() );
  
  audio.connecttoFS(SD_MMC, fn.c_str());
  
  mp3_is_running = true; // start mp3 playing on loop()
}

/*
    handler to redirect to /browse
*/
String postAuxHome(AutoConnectAux& aux, PageArgument& args) {
  
  //aux.redirect("/browse");
  return String();
}

// Serve the /status web page
// responding with a json containing running info (currentTime / FileDuration) to update the progress bar 
void onStatus() {

  // Echo back the value
  String status = "{\"isRunning\":" + String(audio.isRunning()) + ", \"CurrentTime\":" + String(audio.getAudioCurrentTime())  + ",\"FileDuration\":" + String(audio.getAudioFileDuration()) + "}";
  //Serial.printf("onStatus(): %s\r\n", status.c_str()  );
  server.send(200, "text/html", status);
}

// Serve the page  /events?query=key1+key2+key3 handler
// responding with a json with all the found songs
void onSearchSongs() {

  String response  = "";
  
  String keys = server.arg("query");
  Serial.printf("keys is %s\r\n",keys.c_str());
  int num_sub_strings = 0;
  String * strs = splitString(keys, num_sub_strings);
   if(num_sub_strings>20) num_sub_strings = 20;
  char * keys_c[20];
  for(int i=0;i<num_sub_strings;i++){
    Serial.printf("strs[%d]=%s\r\n",i,strs[i].c_str());
    keys_c[i] = (char *) strs[i].c_str();
    
  }
  SearchAZ(keys_c, num_sub_strings);
  
  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Connection", "keep-alive");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/event-stream","");
  
  // Build a response like this
  // data: {"ndx": 49123, "artist": "rolling_stones_the", "song": "angie", "filecode": "51729", "tracks": []}
  
  for(int i = 0; i< valuesList.size();i+=5){
    Serial.printf("ndx=%s artist=%s song=%s filecode=%s tracks=%s\r\n",
          valuesList[i].c_str(),
          valuesList[i+1].c_str(),
          valuesList[i+2].c_str(),
          valuesList[i+3].c_str(),
          valuesList[i+4].c_str()
    );
    response= "data: {\"ndx\": "      + valuesList[i] + 
                   ", \"artist\": "   + valuesList[i+1] +  
                   ", \"song\": "     + valuesList[i+2] +  
                   ", \"filecode\": " + valuesList[i+3] +  
                   ", \"tracks\":["   ;
    int numtracks;
    String * st = splitString( valuesList[i+4].substring(1,valuesList[i+4].length()-1), numtracks,'_');
    for(int i=0;i < numtracks;i++){
      //Serial.printf("st[%d]=%s \r\n",i,st[i].c_str());
      response+= "\"" + String(instruments[st[i].toInt()]) + "\"";
      if(i < numtracks - 1) response += ",";
    }
    response +=   "]}"  ;
  
    server.sendContent(response + "\n\n");
  }

  // Send termination message
  server.sendContent("data: END\n\n");
  server.client().stop();
  
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void onGetMP3(){ 
  
  if (server.hasArg("index") == false)
    return; 

  String fn = server.arg("index");
      
  String fndwnld = "/"+ String(fn[0]) + "/" + fn + ".mp3";    
  File dwnld = SD_MMC.open(fndwnld,  "r");
  Serial.printf("getmp3 %s\r\n",fndwnld );
  if (dwnld) {
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename="+fn+".mp3");
      server.sendHeader("Connection", "close");
      server.streamFile(dwnld, "application/octet-stream");
      dwnld.close();
  } 
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This function serves /play?cmd=play&codefile=47611&track=2&volume=12&delay=300
// it returns the song position in seconds or -1 if stopped
void onPlay(){ 
  
  //if ( !server.hasArg("cmd")    || !server.hasArg("codefile") || !server.hasArg("track")
  //  || !server.hasArg("volume") || !server.hasArg("delay"))
  //  return; 

  String cmd = server.arg("cmd");
  String codefile_str = server.arg("codefile");
  String track_str = server.arg("track");
  String volume_str = server.arg("volume");
  String delay_str = server.arg("delay");

  if(volume_str != ""){
    config.volume = volume_str.toInt();
    audio.setVolume(config.volume);  
  }
  if(delay_str != ""){
    config.mp3_to_smf_delay_ms = delay_str.toInt();
  }
  
  selected_track_to_be_played = track_str == "" ? -1 : track_str.toInt();
  //Serial.printf("onPlay(): track_str=%s selected_track_to_be_played=%d\r\n",track_str,  selected_track_to_be_played);

  if(cmd == "play" && codefile_str != ""){
    play(codefile_str, selected_track_to_be_played,config.volume);

  }else if(cmd == "stop" ){
    mp3_is_running = false; // mp3 will be stopped on loop() 
    SMF_is_running = false; // SMF will be stopped on loop()
  }
  
  writeConfigToFile("/config.json");// save mp3_smf_delay_ms and volume
  
  String resp = "{\"isRunning\":" + String(audio.isRunning()) + ", \"CurrentTime\":" + String(audio.getAudioCurrentTime())  + ",\"FileDuration\":" + String(audio.getAudioFileDuration()) + "}";
  //Serial.printf("onPlay(): %s\r\n", resp.c_str()  );
  server.sendContent(resp);
  return;
}

/*
    Save configs to /configs.json

*/
void onPostConfigs() {
  if (server.hasArg("data")) {
    String newData = server.arg("data");
    Serial.println("Received new data: " + newData);

    File configsfile = SD_MMC.open("/configs.txt", "w");
    if (!configsfile) {
      Serial.println("Failed to open /configs.txt file for writing");
      return;
    }

    // Write new data to JSON file
    configsfile.print(newData);
    configsfile.close();
    Serial.println("onPostConfigs: Data updated successfully!");
    server.send(200, "text/plain", "Data updated successfully");
  } else {
    Serial.println("onPostConfigs: Bad Request!");
    server.send(400, "text/plain", "Bad Request");
  }
}

/*
    handler to update the /browse page
*/
String onPostBrowse(AutoConnectAux& aux, PageArgument& args) {
  
  Serial.printf("onBrowse ...\r\n");

  AutoConnectText& server_url = aux["server_url"].as<AutoConnectText>();
  server_url.value = String(config.server_url);

  const char* html_embed_btn_play = R"(
    <input type='button' id='btn_play' value='PLAY' onclick='play()'>
  )";

  AutoConnectElement& btn_play = aux["btn_play"];
  btn_play.value = String(html_embed_btn_play);
 

  const char* html_embed_list = R"(
    <script>
  
      // http://192.168.1.40/play?cmd=play&codefile=47610&volume=12&delay=200
      function play() {

        const xhttp = new XMLHttpRequest();
        xhttp.onload = function() {
          // {isRunning: 1, CurrentTime: 12, FileDuration: 35}
          json_resp = JSON.parse(this.responseText);
        }

        cmd = document.getElementById("btn_play");
        myurl = "";
        if(cmd.value == "PLAY"){
          myurl = "/play?cmd=play";
          cmd.value = "STOP";
        }else{
          myurl = "/play?cmd=stop";
          cmd.value = "PLAY";
        }
        codefile = document.getElementById("selected_codefile").value;
        myurl += "&codefile=" + codefile;
        volume = select_volume.value;
        myurl += "&volume=" + volume;
        delay = mp3_smf_delay_ms.value;
        myurl += "&delay=" + delay;
        track = select_track.value;
        myurl += "&track=" + track;
        
        xhttp.open("GET", myurl);
        xhttp.send();
      }

      function getStatus() {
        const xhttp = new XMLHttpRequest();
        xhttp.onload = function() {
          // {isRunning: 1, CurrentTime: 12, FileDuration: 35}
          json_resp = JSON.parse(this.responseText);
          progress_bar.max = json_resp.FileDuration;
          progress_bar.value = json_resp.CurrentTime;
          document.getElementsByClassName("magnify")[0].innerText = json_resp.CurrentTime + "/" + json_resp.FileDuration;

        }
        xhttp.open("GET", "/status");
        xhttp.send();
      }
      setInterval('getStatus()', 1000);
  
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
        var sel_codefile = document.getElementById("selected_codefile");
        sel_codefile.value = filecode;    

        var radio_id = document.querySelector('input[name="songs"]:checked').id;
        var ndx_artist_song_filecode = document.querySelector('label[for="' + radio_id + '"]').innerText.split("-");
        selected_artist.innerText = ndx_artist_song_filecode[1];
        selected_song.innerText = ndx_artist_song_filecode[2];

        var x = document.getElementById("ss_static"); 
        x.setAttribute("src", server_url.innerText + "/get_mp3?index=" + filecode);
        //x.play(); 
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
            const inputItem = document.createElement('input');
            inputItem.id = 'radio'+result.ndx
            inputItem.type = 'radio'
            inputItem.name ='songs'
            inputItem.setAttribute('onclick', 'playAudio(' + result.filecode + ')');
            const labelItem = document.createElement('label');
            inputText = result.ndx +' - ' + result.artist.replace(/_/g,' ') + ' - '+ result.song.replace(/_/g,' ') +' - filecode: ' + result.filecode + " - tracks: ";
            for(i = 0; i < result.tracks.length;i++) {
              inputText += " " +  i + "/" + result.tracks[i] + " - "; 
            }

            results.appendChild(inputItem);
            labelItem.setAttribute("for","radio"+result.ndx);
            labelItem.innerHTML = inputText;
            results.appendChild(labelItem);
            //const button_tracks = document.createElement('button');
            //button_tracks.id = 'bt' + result.filecode;
            //button_tracks.setAttribute('onclick', 'showTracks(' + result.filecode + ')' );  
            //button_tracks.textContent = 'show tracks'; // Set button text
            //results.appendChild(button_tracks);
            results.appendChild(document.createElement('br'));
        };      
      }

      function addConfig() {
        var select = document.getElementById("configs");
        var config = document.createElement("option");
        config.text = selected_artist.innerText + "- " + 
                      selected_song.innerText + "- "   + 
                      selected_codefile.value + " - "  + 
                      select_track.selectedOptions[0].innerText;
        select.add(config);
      }

      function deleteConfig() {
        var select = document.getElementById("configs");
        select.remove(select.selectedIndex);
      }

      function saveConfigs() {
        var configs = document.getElementById("configs").innerHTML;
        var xhr = new XMLHttpRequest();
        xhr.open("POST", "/saveconfigs", true);
        xhr.setRequestHeader("Content-Type", "application/x-www-form-urlencoded");
        xhr.send("data=" + encodeURIComponent(configs));
      }

      function configs_click(){
        var fields = configs.selectedOptions[0].innerText.split("-");      
        selected_artist.innerText = fields[0].trim();
        selected_song.innerText = fields[1].trim();
        selected_codefile.value = fields[2].trim();
        select_track.selectedIndex = parseInt(fields[3].trim()) + 1
      }

    </script>
    
    <br/>
    <span>Select a configuration: </span>
    <select id='configs' onClick='configs_click()'>
      %CONFIGS%
    </select>
    <input type='button' onclick='addConfig(); saveConfigs()' value='Add'/>
    <input type='button' onclick='deleteConfig(); saveConfigs()' value='Del'/>
    <br/>
    <audio controls id="ss_static" src = "" type="audio/mp3"></audio>
    <br/>
    <input type='button' id='btn_search' value='SEARCH' onclick='mysearch()'>
    <input type='text' id='search_query' value=''>
    <br/>
    <p id='results'></p>
    
  )";

  String configsFileContent = ""; 

  File configsFile = SD_MMC.open("/configs.txt", "r");
  if (configsFile) {
    configsFileContent = configsFile.readString();
    configsFile.close();
  }else{
    Serial.println("Failed to open /configs.txt file");
  }

  // Generate select options
  // Replace placeholder in HTML with options
  String html_embed_list_str = String(html_embed_list);
  html_embed_list_str.replace("%CONFIGS%", configsFileContent);
  
  AutoConnectSelect& volume = aux["select_volume"].as<AutoConnectSelect>();
  volume.selected = config.volume +1;
  
  AutoConnectInput& mp3_smf_delay_ms = aux["mp3_smf_delay_ms"].as<AutoConnectInput>();
  mp3_smf_delay_ms.value = String(config.mp3_to_smf_delay_ms);
  
  AutoConnectElement& obj = aux["embed_list"];
  obj.value = html_embed_list_str;
  
  return String();    
}

// optional audio MP3 callback functions
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
    Serial.printf("audio_eof_mp3: getFilePos=%d\r\n",audio.getFilePos());
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}