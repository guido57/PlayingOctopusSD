
/*
    Serve the page  /sddel?filepath=pereper.mp3
                    /sddel?filecode=12345
    to delete a fle by name or by codefile
*/
void onSdDel() {

  String filedel = server.arg("filepath");
  if(filedel != ""){
    Serial.printf("deleting %s\r\n",filedel.c_str());
    int num_sub_strings = 0;
    
    if(SD_MMC.remove(filedel)){
      Serial.printf("%s deleted!\r\n",filedel.c_str());
      server.send(200);
    }else{
      Serial.printf("%s not found!\r\n",filedel.c_str());
      server.send(404);
    }
  }

  String resp = "";
  String filecode = server.arg("filecode");
  if(filecode != ""){
    
    // delete /n/filecode.mp3
    String fc_mp3 = "/" + String(filecode[0]) + "/" + filecode + ".mp3";
    if(SD_MMC.remove(fc_mp3))
      resp += fc_mp3 + " deleted";
    else
      resp += fc_mp3 + " not found";
    
    // delete /n/filecode.mid
    String fc_mid = "/" + String(filecode[0]) + "/" + filecode + ".mid";
    if(SD_MMC.remove(fc_mid))
      resp += "<br/>" + fc_mid + " deleted";
    else
      resp += "<br/>" + fc_mid + " not found";
  
    // delete element filecode in azmin.json
    int curlyopen = -1;
    int curlyclose = -1;
    findByFileCode(filecode.toInt(),curlyopen,curlyclose);
    if(curlyopen != -1 && curlyclose != -1){
      if(cutFromTo(curlyopen,curlyclose)){
        copy("/azmin_tmp.json", "/azmin.json");
        resp += "<br/>" + filecode + " deleted from azmin.json";
      }
    }else{
      resp += "<br/>" + filecode + " not found in azmin.json";
    }

    server.send(200, "text/html", resp);
  }
}

/*
    list the requested SD folder into the FilesInFolder vector
*/
std::vector<String> FilesInFolder;
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    FilesInFolder.clear();
    Serial.printf("Listing directory: %s\n", dirname);
    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.println(file.name());
            FilesInFolder.push_back(file.name());
            FilesInFolder.push_back("");
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print(file.name());
            FilesInFolder.push_back(file.name());
            Serial.println(file.size());
            FilesInFolder.push_back(String(file.size()));
            
        }
        file = root.openNextFile();
    }
}


/*
    Serve the page  /sddir?query=_directory_subdir
    sending the requested folder content into a json 
*/
void onSdDir() {

  String response  = "";
  
  String query = server.arg("query");
  Serial.printf("query is %s\r\n",query.c_str());
  int num_sub_strings = 0;
  String * strs = splitString(query, num_sub_strings,'_');
  if(num_sub_strings>20) num_sub_strings = 20;
  String path = strs[0]; // build the dir path to show 
  for(int i=1;i<num_sub_strings;i++){
    path+= "/" + strs[i];
  }
  listDir(SD_MMC, path.c_str(),0); 

  server.sendHeader("Cache-Control", "no-cache");
  server.sendHeader("Connection", "keep-alive");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/event-stream","");
  
  // Build a response like this
  // data: {"file": "/pippo", "size": 0}
  // data: {"file": "qui", "size": 314}
  
  for(int i = 0; i<  FilesInFolder.size();i+=2){
    if(FilesInFolder[i+1].length() == 0)
       FilesInFolder[i+1] = "0";
    Serial.printf("name=%s size=%s\r\n",
          FilesInFolder[i].c_str(),
          FilesInFolder[i+1].c_str()
    );
    response= "data: {\"file\":\""   + FilesInFolder[i] + "\"" + 
                   ", \"size\": "   + FilesInFolder[i+1] ;
                   
    response +=   "}"  ;
  
    server.sendContent(response + "\n\n");
  }

  // Send termination message
  server.sendContent("data: END\n\n");
  server.client().stop();
  
}

void handleFileUploadToSDFields(){

  server.send(200);
}

/*
 *  Upload and other info handler.
 *  read upload status (START WRITE END)
 *  write the uploaded files (.mid and .mp3) to the SD card renaming them to filecode.mid and filecode.mp3
 *  calculate the ndx to assign (the highest ndx + 1)
 *  calculate the instruments used in the mid file 
 *  delete the codefile entry in azmin.json (if existing)
 *  add the new entry in azmin.json
*/
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String lastfile4;
File UploadToSDFile; 
String filepath_mid;
String filepath_mp3;
int ndx; 
int codefile;
void handleFileUploadToSD(){ // upload a new file to the SD file system
  HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
                                            // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  String artist;
  String song;
  String filecode;
  
  for (uint8_t i = 0; i < server.args(); i++) {
    String message = " " + server.argName(i) + ": " + server.arg(i) + "\n";
    Serial.printf("uploadtosdfile %d message=%s\r\n",i, message.c_str());
  }
 
  if(uploadfile.status == UPLOAD_FILE_START){
    lastfile4 = uploadfile.filename.substring(uploadfile.filename.length()-4);
    if(lastfile4 != ".mid" && lastfile4 != ".mp3"){
      Serial.printf("Error: filename=%s file extension is %s instead of .mp3 or .mid\r\n",uploadfile.filename, lastfile4);  
      return;
    }

    artist   = server.arg("artist");
    song     = server.arg("song");
    filecode = server.arg("filecode");
    Serial.printf("UploadToSDFile: START artist=%s song=%s filecode=%s\r\n",artist.c_str(),song.c_str(), filecode.c_str());
    if(artist == "" or song== "" or filecode.toInt() == 0){
      Serial.printf("artist and song must be present\r\n");  
      return;
    }
    ndx = -1; codefile = -1;
    getHighestNdxFilecode(ndx,codefile);
    if(ndx == -1 || codefile == -1)
      return;

    ndx += 1;
    codefile = (codefile < 80000) ? 80000 : codefile + 1;
    
    filepath_mp3 = "/" + String(filecode[0])  + "/" + filecode + ".mp3" ;
    filepath_mid = "/" + String(filecode[0])  + "/" + filecode + ".mid" ;
    String filepath;
    filepath = lastfile4 == ".mp3" ? filepath_mp3 : filepath_mid;
    Serial.print("Upload File Name: "); Serial.println(filepath);
    SD_MMC.remove(filepath);                  // Remove a previous version, otherwise data is appended the file again
    UploadToSDFile = SD_MMC.open(filepath, "w");  // Open the file for writing in SD (create it, if doesn't exist)
  }else if (uploadfile.status == UPLOAD_FILE_WRITE){
    if(UploadToSDFile) 
      UploadToSDFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  }else if (uploadfile.status == UPLOAD_FILE_END){
    if(UploadToSDFile){          // If the file was successfully created
      UploadToSDFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      //String filecode = uploadfile.filename.substring(0,uploadfile.filename.length()-4);
      if(lastfile4 == ".mp3" && 
        SD_MMC.exists(filepath_mid)   && 
        SD_MMC.exists(filepath_mp3)){
          // edit azmin.json
    
          artist   = server.arg("artist");
          song     = server.arg("song");
          filecode = server.arg("filecode");
          Serial.printf("UploadToSDFile: END artist=%s song=%s filecode=%s\r\n",artist.c_str(),song.c_str(), filecode.c_str());
          if(artist == "" or song== "" or filecode.toInt() == 0){
            Serial.printf("artist and song must be present\r\n");  
            return;
          }
          // calculate the instruments
          Serial.printf("opening %s to calculate instruments\r\n", filepath_mid.c_str());
          File fs = SD_MMC.open(filepath_mid,"r");
          if(!fs){
            Serial.printf("handleFileUploadToSD: error opening %s\r\n", filepath_mid.c_str());  
            return;
          }
          int fs_size = 0;
          while(fs.available()){
            fs_size += fs.read(dwnl_buffer+fs_size,DWNL_BUFF_SIZE);
          }
          fs.close();
          SMF = new MD_MIDIFile();
          int err = SMF->load(dwnl_buffer, fs_size); 
          SMF->pause(true);
          String instrs = SMF->getInstruments();
          Serial.printf("Instruments=%s\r\n", instrs.c_str());
          fs.close();
          delete(SMF);
          SMF = nullptr;
          // if {"codefile" exists} delete the element
          int curlyopen = -1;
          int curlyclose = -1;
          findByFileCode(filecode.toInt(),curlyopen,curlyclose);
          if(curlyopen != -1 && curlyclose != -1){
            if(cutFromTo(curlyopen,curlyclose)){
              copy("/azmin_tmp.json", "/azmin.json");
            }
          }
          artist.toLowerCase();  artist.replace(" ","_");
          song.toLowerCase();    song.replace(" ","_");
          Serial.printf("add new record ndx=%d artist=%s song=%s filecode=%s tracks=%s to azfile.json \r\n%", 
                            ndx, artist.c_str(), song.c_str(), filecode.c_str(), instrs.c_str());                
          if(appendNewRecord("/azmin.json","/azmin_tmp.json",ndx,artist,song,filecode,instrs)){
            if(copy("/azmin_tmp.json", "/azmin.json")){
              Serial.printf("new record added\r\n");                
            }else{
              Serial.printf("error adding new record\r\n");                
            }  
          } 
        }   
      }
      server.send(200);
       
    }else{
      Serial.printf("ReportCouldNotCreateFile\r\n");
    }
  
};

/*
    handler to upload and delete files in filesystem SD
*/
String postFileSD(AutoConnectAux& aux, PageArgument& args) {
  
  int maxndx = -1;
  int maxfilecode = -1;
  getHighestNdxFilecode(maxndx,maxfilecode);

  AutoConnectText& status = aux["status"].as<AutoConnectText>();
  status.value = "Free RAM: " + String(esp_get_free_heap_size()) + " bytes"  
       + " ---- Used SD: " + String(SD_MMC.usedBytes()) + " / " + String(SD_MMC.totalBytes()) + " bytes.";
  status.value += "<br/>azmin.json     max ndx=" + String(maxndx) + "  max filecode=" + String(maxfilecode);
  
  AutoConnectInput& filecode = aux["codefile"].as<AutoConnectInput>();
  filecode.value = maxfilecode < 80000 ? 80000 : maxfilecode + 1;

  //listDir(SD_MMC, "/", 0);

  const char* html_embed_list = R"(
    <p><progress /></p>
    <span id='log_id'></span><br/>
    <input type='button' name='upload' id='upload_btn_id' value='UPLOAD' onClick='upload_mid()' >
    <input type='button' name='abort' id='abort_btn_id' value='ABORT' >
    <br/>
    <input type='button' name='delete' id='delete_btn_id' value='DELETE' onClick='delete_codefile()' >
    <label for="codefile_del">Codefile</label>
    <input type='text' name='codefile_del' id='codefile_del_id' >
    <p id='results'></p>
    <script>
    const progressBar = document.querySelector("progress");
    progressBar.style = "display : none";
    const abortButton = document.getElementById("abort_btn_id");
    
    function delete_codefile(){
      cod_del = parseInt(codefile_del_id.value);
      if(cod_del > 0){
        if(confirm("Are you sure to delete " + cod_del + ".mp3 and " + cod_del + ".mid ?")){
          const xhr = new XMLHttpRequest();
          xhr.open("GET", "/sddel?filecode="+ cod_del, true);
          xhr.send();
        }
      }  
    }

    function upload_mid(){  
        const xhr = new XMLHttpRequest();
        xhr.timeout = 60000; // 60 seconds

        // Link abort button
        abortButton.addEventListener(
          "click",
          () => {
            xhr.abort();
          },
          { once: true },
        );

        // When the upload starts, we display the progress bar
        xhr.upload.addEventListener("loadstart", (event) => {
          //progressBar.classList.add("visible");
          progressBar.style = "display : inline";
          progressBar.value = 0;
          progressBar.max = event.total;
          log_id.textContent = "Uploading (0%)…";
          abortButton.disabled = false;
        });

        // Each time a progress event is received, we update the bar
        xhr.upload.addEventListener("progress", (event) => {
          progressBar.value = event.loaded;
          progressBar.max = event.total;
          log_id.textContent = `Uploading ${event.loaded} / ${event.total} 
            ${(
            (event.loaded / event.total) *
            100
          ).toFixed(0)}%)…`;
        });

        // When the upload is finished, we hide the progress bar.
        xhr.upload.addEventListener("loadend", (event) => {
          //progressBar.classList.remove("visible");
          progressBar.style = "display : none";
          if (event.loaded !== 0) {
            log_id.textContent = "Upload finished.";
          }
          abortButton.disabled = true;
        });

        // In case of an error, an abort, or a timeout, we hide the progress bar
        // Note that these events can be listened to on the xhr object too
        function errorAction(event) {
          progressBar.classList.remove("visible");
          log_id.textContent = `Upload failed: ${event.type}`;
        }
        xhr.upload.addEventListener("error", errorAction);
        xhr.upload.addEventListener("abort", errorAction);
        xhr.upload.addEventListener("timeout", errorAction);
      
        // Build the payload
        const fileData = new FormData();
        fileData.append("artist", artist.value );
        fileData.append("song", song.value );
        fileData.append("filecode", codefile.value );
        fileData.append("file", upload_file_mid.files[0]);
        fileData.append("file", upload_file_mp3.files[0]);

        xhr.open("POST", "/fupload", true);

        // Note that the event listener must be set before sending (as it is a preflighted request)
        xhr.send(fileData);
    };

    var starting_dir = "/"
    function sddir(_starting_dir){
        starting_dir = _starting_dir;
        const eventSource = new EventSource('/sddir?query='+starting_dir);
        results.innerHTML = '';
        eventSource.onmessage = function(event) {

            if (event.data === 'END') {
                eventSource.close();
                return;
            }
            const result = JSON.parse(event.data);
            const inputItem = document.createElement('input');
            inputItem.id = 'radio-'+result.file
            inputItem.type = 'radio'
            inputItem.name ='filefolder'
            //inputItem.setAttribute('onclick', 'playAudio(' + result.filecode + ')');
            const labelItem = document.createElement('label');
            if(starting_dir == "/" )
              inputText = starting_dir + result.file; 
            else
              inputText = starting_dir + "/" + result.file ;
            if(result.size > 0){
              inputText +=  " - " + result.size + " bytes";    
            } 

            results.appendChild(inputItem);
            //labelItem.setAttribute("for","radio"+result.ndx);
            labelItem.innerHTML = inputText;
            if(result.size == 0)
              labelItem.setAttribute('onclick', 'sddir(\"' + starting_dir + result.file + '\")');
            
            results.appendChild(labelItem);
            results.appendChild(document.createElement('br'));
        };      
    }
    sddir(starting_dir);
    </script>
    
  )";

  AutoConnectElement& obj = aux["object"];
  obj.value = String(html_embed_list);
 
  return String();
  }
