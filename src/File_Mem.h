#ifndef FILE_MEM_H
#define FILE_MEM_H

#include <Arduino.h>


#define O_READ 0

class File_Mem {
    public:
      File_Mem() ;
      File_Mem(const unsigned char* _data, int _len) ;
      bool open(const String& path, int o_flag);         // open a file from SD
      bool open(const unsigned char * buffer, int len);  // open a char array
      void close();
      int fgets(char* str, int num, char* delim = nullptr) ;
      int read(void* buf, size_t nbyte);
      int readBytes(void* buf, size_t nbyte);
      int read();
      bool seekSet(uint64_t pos);
      bool seekCur(uint64_t relative_pos);
      uint64_t curPosition(); 
      uint64_t position(); 
   
    private:
      const unsigned char* data;
      uint64_t pos;
      int size;
    
};



#endif
