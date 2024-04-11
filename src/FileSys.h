// Include the class to handle the upload process
#include <AutoConnectUploadFS.h>
AutoConnectUploadFS uploader;

#include "config.h"

/* 
Append all the filenames found in the LittleFS root ("/") to the radio element 
as RadioButtons
*/
void ListAllFiles(AutoConnectRadio * radio){
  Serial.printf("LittleFS total size: %d bytes. Used %d bytes.\r\n", LittleFS.totalBytes(), LittleFS.usedBytes());
  File root = LittleFS.open("/");
 
  File file = root.openNextFile();
 
  while(file){
      String fn = String(file.name());
      int f_size = file.size();
      file.close();
      radio->add(fn + " --------- " + String(f_size) + " bytes " );
      file = root.openNextFile();
  }
  root.close();
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
