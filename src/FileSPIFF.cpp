#include <Arduino.h>
#include <SPIFFS.h>
#include <FileSPIFF.h>


    
FileSPIFF::FileSPIFF() {
        data = nullptr;
        size = 0;
 }

void FileSPIFF::close() {
  fp.close();
}

// Open a file from SD
bool FileSPIFF::open(const String& path, const char * o_flag){
    
    fp = SPIFFS.open(path,o_flag);
    if(fp)
      return true;
    return false;    
}


int FileSPIFF::fgets(char* str, int num, char* delim) {

  char ch;
  int n = 0;
  int r = -1;
  while ((n + 1) < num && (r = read(&ch, 1)) == 1) {
    // delete CR
    if (ch == '\r') {
      continue;
    }
    str[n++] = ch;
    if (!delim) {
      if (ch == '\n') {
        break;
      }
    } else {
      if (strchr(delim, ch)) {
        break;
      }
    }
  }
  if (r < 0) {
    // read error
    return -1;
  }
  str[n] = '\0';
  return n;
}

int FileSPIFF::read(void* buf, size_t nbyte){
    return fp.read( (uint8_t *) buf,nbyte);
    
}

int FileSPIFF::read(){
    return fp.read();
}

bool FileSPIFF::seekSet(uint64_t _position) {
    return fp.seek(_position,SeekMode::SeekSet);
}

bool FileSPIFF::seekCur(uint64_t _relative_position) {
    return fp.seek(_relative_position,SeekMode::SeekCur);
}

uint64_t FileSPIFF::curPosition() {
  size_t st = fp.position();
  return (uint64_t) st ;
}

