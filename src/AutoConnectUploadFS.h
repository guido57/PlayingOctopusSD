//#include <FS.h>
//#include <SPIFFS.h>
#include <AutoConnect.h>


class AutoConnectUploadFS : public AutoConnectUploadHandler {
 public:
  AutoConnectUploadFS()  {}
  ~AutoConnectUploadFS() {}

 protected:
  File fsUploadFile;

  bool _open(const char* filename, const char* mode) override {
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    String fns = String(filename);
    if(fns=="/" || fns=="")
      return false;
    fsUploadFile = LittleFS.open(filename, "w");
    return fsUploadFile != File();
  }

  size_t _write(const uint8_t* buf, const size_t size) override {
    
    if (fsUploadFile){
      Serial.printf("_writing %d bytes\r\n", size);
      return fsUploadFile.write(buf, size);
    } else
      return -1;
  }

  void _close(HTTPUploadStatus status) override {
    if (fsUploadFile){
      Serial.printf("_close\r\n");  
      fsUploadFile.close();
    }
  }

 };
