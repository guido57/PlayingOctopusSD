#include <SdFat.h>


 File_Mem::File_Mem(const unsigned char* _data, int _len) {
        data = _data;
        size = _len;
 }
    
File_Mem::File_Mem() {
        data = nullptr;
        size = 0;
 }

void File_Mem::close() {
}

// Open a file from SD
bool File_Mem::open(const String& path, int o_flag){
    position = 0;
    return true;    
}

bool File_Mem::open(const unsigned char * buffer, int len){
    data = buffer;
    size = len;
    position = 0;
    return true;    
}

int File_Mem::fgets(char* str, int num, char* delim) {
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

int File_Mem::read(void* buf, size_t nbyte){
    int count = 0;   
    char *p = (char *)buf;
    while(count<nbyte){
       int c = read();
       if(c==-1)
            break;
       *(p + count) = c;
       count ++;
    }
    return count;
}

int File_Mem::read(){
     if (position >= size) {
      return -1; // End of file
    }
    return data[position++];
}

bool File_Mem::seekSet(uint64_t _position) {
    if ( _position >=0 && _position < size) {
      position = _position;
      return true; // success
    }
    return false; // out of bounds
}

bool File_Mem::seekCur(uint64_t _relative_position) {
    return seekSet( position + _relative_position);
}

uint64_t File_Mem::curPosition() {
    return position ;
}

// ---------------------------------------------------------------------

bool SdFat::chdir(const char* path) {
    return true;
}
bool SdFat::chvol() {
    return true;
}

bool SdFat::begin(uint8_t _pp, unsigned int){

    return true; 
}
