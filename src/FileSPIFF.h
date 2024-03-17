
#ifndef FILESPIFF_h
#define FILESPIFF_h

#include <Arduino.h>
#include <SPIFFS.h>

#define O_READ 0

class FileSPIFF {
    public:
      FileSPIFF() ;
      bool open(const String& path, const char * o_flag);         // open a file from SD
      void close();
      int fgets(char* str, int num, char* delim = nullptr) ;
      int read(void* buf, size_t nbyte);
      int read();
      bool seekSet(uint64_t pos);
      bool seekCur(uint64_t relative_pos);
      uint64_t curPosition(); 
      File fp;
    private:
       const unsigned char* data;
      uint64_t position;
      int size;
    
};


typedef FileSPIFF SDFILE;      ///< File type for files

typedef uint8_t SdCsPin_t;
/** SPISettings for SCK frequency in MHz. */
#define SD_SCK_MHZ(maxMhz) (1000000UL * (maxMhz))
#define SPI_FULL_SPEED SD_SCK_MHZ(50)


class SdFat {
    
 public:
    bool begin(SdCsPin_t csPin, uint32_t maxSck) ;
        //return begin(SdSpiConfig(csPin, SHARED_SPI, maxSck));
    bool chdir(const char* path) ;
    bool chvol() ;
    bool ls(uint8_t flags) { return true; }
};


//typedef SdFs SdFat;



#endif
