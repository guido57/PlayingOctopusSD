#include <Arduino.h>
#include <vector>
#include <SD_MMC.h>
//#include "FS.h"                // SD Card ESP32


// Function to split a String into substrings separated by the character "+"
String * splitString(String input, int& numSubstrings, char separator = ' ') {
  // Count the number of substrings
  int count = 0;
  for (int i = 0; i < input.length(); ++i) {
    if (input.charAt(i) == separator) {
      count++;
    }
  }
  // Allocate memory for the array of substrings
  String* substrings = new String[count + 1]; // Add 1 for the last substring
  // Split the input string into substrings
  int startPos = 0;
  int endPos = input.indexOf(separator);
  int index = 0;
  while (endPos != -1) {
    substrings[index++] = input.substring(startPos, endPos);
    startPos = endPos + 1;
    endPos = input.indexOf(separator, startPos);
  }
  // Add the last substring
  substrings[index] = input.substring(startPos);
  // Set the number of substrings
  numSubstrings = count + 1;
  return substrings;
}

// return the first position of terminator starting from the pointer cp 
// and for a maximum span of limit
inline int myindex(char * cp, char terminator, int limit = 10000){

  int ndx = 0;
  while(ndx < limit){
  
    if(*cp == terminator)
      return ndx;
    cp++;
    ndx++;
  }

  return -1;
}

std::vector<String> valuesList;
#define BUFFER_SIZE 10000


void SearchAZ(char ** keys, int numkeys){
  
  valuesList.clear();

  File azfile = SD_MMC.open("/azmin.json", "r");
  if(!azfile){
    Serial.println("Failed to open file azmin.json");
  } 
  Serial.println("file azmin.json opened");

  int count = 0;  
   
  char * buffer = (char *)  malloc(BUFFER_SIZE+1000);
  struct elvalues_type{
    char ndx[10]; 
    char artist[100]; 
    char song[200]; 
    char filecode[10];
    char tracks[200];
  }elvalues;
  while(azfile.available()){
    int totalbytes = 0;
    totalbytes += azfile.readBytes(buffer+totalbytes,BUFFER_SIZE);
    totalbytes += azfile.readBytesUntil('}',(char *)(buffer+totalbytes),1000);
    buffer[totalbytes++]='}';
    //Serial.printf(" now buffer contains a number of records: {...},{...},{...},{...} total %d bytes\r\n",totalbytes);
    //for(int i= 0;i<totalbytes;i++){ Serial.printf("%c",buffer[i]); }; Serial.println("");
    int curlyopen = 0;
    int curlyclose = 0;
    while(true){
      int co = myindex(buffer+curlyopen,'{');
      curlyopen = curlyopen + co;
      co = myindex(buffer+curlyopen,'}');
      curlyclose  = curlyopen + co;
      //Serial.printf("now a single record {...} is between curlyopen %d and curlyclose %d: \r\n",curlyopen,curlyclose);
      //for(int i=curlyopen; i<=curlyclose;i++){Serial.printf("%c", buffer[i]);} Serial.println("");
      int numelements = 0;
      int colon = curlyopen + 1;
      int comma;
      while(true){
        colon = colon + myindex(buffer+colon, ':') ; // get the beginning of value
        if(numelements < 4){
          comma = colon + myindex(buffer+colon, ',');  // get the end of value
        }else{  
          comma = curlyclose ;
        }
        // now colon and comma surrounds the element value      
        //Serial.printf("quoteopen=%d quoteclose=%d colon=%d comma=%d\r\n", 
        //            quoteopen, quoteclose, colon , comma);
        switch(numelements){
          case 0:
            memcpy(elvalues.ndx,buffer+colon+1,comma-colon-1);
            elvalues.ndx[comma-colon-1] = 0;
            break;
          case 1:
            memcpy(elvalues.artist,buffer+colon+1,comma-colon-1);
            elvalues.artist[comma-colon-1] = 0;
            break;
          case 2:
            memcpy(elvalues.song,buffer+colon+1,comma-colon-1);
            elvalues.song[comma-colon-1] = 0;
            break;
          case 3:
            memcpy(elvalues.filecode,buffer+colon+1,comma-colon-1);
            elvalues.filecode[comma-colon-1] = 0;
            break;
          case 4:
            memcpy(elvalues.tracks,buffer+colon+1,comma-colon-1);
            elvalues.tracks[comma-colon-1] = 0;
            break;
        }
        
        if(numelements == 4)
          break;
        numelements ++;
        colon = comma + 1;
      } // End of parsing 1 record
      
      // now elvalues contains the 5 values for this record
      //Serial.println("elvalues before:");
      //Serial.printf("elvalues.ndx=%s",elvalues.ndx);
      //Serial.printf("elvalues.artist=%s",elvalues.artist);
      //Serial.printf("elvalues.song=%s",elvalues.song);
      //Serial.printf("elvalues.filecode=%s",elvalues.filecode);
      //Serial.printf("elvalues.tracks=%s",elvalues.tracks);
      //Serial.println("");  
      
      bool found = true;
      for(int i = 0; i < numkeys; i++){
        if( strstr(elvalues.song, keys[i])== nullptr && strstr(elvalues.artist, keys[i])== nullptr){
          found = false;    
        }
      }
      
      if(found){
        
        valuesList.push_back(String(elvalues.ndx)); 
        //Serial.printf("pushed %s\r\n",elvalues.ndx);
        valuesList.push_back(String(elvalues.artist)); 
        //Serial.printf("pushed %s\r\n",elvalues.artist);
        valuesList.push_back(String(elvalues.song)); 
        //Serial.printf("pushed %s\r\n",elvalues.song);
        valuesList.push_back(String(elvalues.filecode)); 
        //Serial.printf("pushed %s\r\n",elvalues.filecode);
        valuesList.push_back(String(elvalues.tracks)); 
        //Serial.printf("pushed tracks\r\n");
        

      }
      numelements = 0;
      curlyopen=curlyclose+1;

      if(curlyclose >= totalbytes-1)
        break;
    } // End of parsing a number of records 
    //Serial.printf("Exited when curlyclose=%d totalbytes=%d\r\n", curlyclose, totalbytes);
    // skip , between chunks
    azfile.readBytesUntil(',', buffer,100);
    //Serial.printf("count=%d\r\n", count++);
  } // End of parsing file
  Serial.println("END");  
  azfile.close();
  free(buffer);
  return;
}



void findByFileCode(int filecode, int & curlyopen_ret, int & curlyclose_ret){
  
  valuesList.clear();

  char filecode_arr[20];
  itoa(filecode, filecode_arr, 10);

  File azfile = SD_MMC.open("/azmin.json", "r");
  if(!azfile){
    Serial.println("Failed to open file azmin.json");
  } 
  Serial.println("file azmin.json opened");

  int count = 0;  
   
  char * buffer = (char *)  malloc(BUFFER_SIZE+1000);
  struct elvalues_type{
    char ndx[10]; 
    char artist[100]; 
    char song[200]; 
    char filecode[10];
    char tracks[200];
  }elvalues;
  while(azfile.available()){
    int totalbytes = 0;
    totalbytes += azfile.readBytes(buffer+totalbytes,BUFFER_SIZE);
    totalbytes += azfile.readBytesUntil('}',(char *)(buffer+totalbytes),1000);
    buffer[totalbytes++]='}';
    //Serial.printf(" now buffer contains a number of records: {...},{...},{...},{...} total %d bytes\r\n",totalbytes);
    //for(int i= 0;i<totalbytes;i++){ Serial.printf("%c",buffer[i]); }; Serial.println("");
    int curlyopen = 0;
    int curlyclose = 0;
    while(true){
      int co = myindex(buffer+curlyopen,'{');
      curlyopen = curlyopen + co;
      co = myindex(buffer+curlyopen,'}');
      curlyclose  = curlyopen + co;
      //Serial.printf("now a single record {...} is between curlyopen %d and curlyclose %d: \r\n",curlyopen,curlyclose);
      //for(int i=curlyopen; i<=curlyclose;i++){Serial.printf("%c", buffer[i]);} Serial.println("");
      int numelements = 0;
      int colon = curlyopen + 1;
      int comma;
      while(true){
        colon = colon + myindex(buffer+colon, ':') ; // get the beginning of value
        if(numelements < 4){
          comma = colon + myindex(buffer+colon, ',');  // get the end of value
        }else{  
          comma = curlyclose ;
        }
        // now colon and comma surrounds the element value      
        //Serial.printf("quoteopen=%d quoteclose=%d colon=%d comma=%d\r\n", 
        //            quoteopen, quoteclose, colon , comma);
        switch(numelements){
          case 0:
            memcpy(elvalues.ndx,buffer+colon+1,comma-colon-1);
            elvalues.ndx[comma-colon-1] = 0;
            break;
          case 1:
            memcpy(elvalues.artist,buffer+colon+1,comma-colon-1);
            elvalues.artist[comma-colon-1] = 0;
            break;
          case 2:
            memcpy(elvalues.song,buffer+colon+1,comma-colon-1);
            elvalues.song[comma-colon-1] = 0;
            break;
          case 3:
            memcpy(elvalues.filecode,buffer+colon+1,comma-colon-1);
            elvalues.filecode[comma-colon-1] = 0;
            break;
          case 4:
            memcpy(elvalues.tracks,buffer+colon+1,comma-colon-1);
            elvalues.tracks[comma-colon-1] = 0;
            break;
        }
        
        if(numelements == 4)
          break;
        numelements ++;
        colon = comma + 1;
      } // End of parsing 1 record
      
      // now elvalues contains the 5 values for this record
      //Serial.println("elvalues before:");
      //Serial.printf("elvalues.ndx=%s",elvalues.ndx);
      //Serial.printf("elvalues.artist=%s",elvalues.artist);
      //Serial.printf("elvalues.song=%s",elvalues.song);
      //Serial.printf("elvalues.filecode=%s",elvalues.filecode);
      //Serial.printf("elvalues.tracks=%s",elvalues.tracks);
      //Serial.println("");  
      
      //Serial.printf("curlyopen=%d curlyclose=%d filecode=%s filecode_arr=%s\r\n", curlyopen, curlyclose, elvalues.filecode, filecode_arr);
      if( memcmp(elvalues.filecode+1, filecode_arr, strlen(filecode_arr))==0){
        curlyclose_ret = azfile.position()-totalbytes + curlyclose + 1;
        curlyopen_ret  = azfile.position()-totalbytes + curlyopen  + 1;
        azfile.seek(curlyopen_ret);
        //Serial.printf("azfile from %d to %d\r\n",curlyopen_ret,curlyclose_ret);
        //for(int i=0; i<curlyclose_ret-curlyopen_ret;i++)
        //  Serial.printf("%c", azfile.read());
        //Serial.println();
        azfile.close();
        free(buffer);
        return;
      }
      
      curlyopen=curlyclose+1;

      if(curlyclose >= totalbytes-1)
        break;
    } // End of parsing a number of records 
    //Serial.printf("Exited when curlyclose=%d totalbytes=%d\r\n", curlyclose, totalbytes);
    // skip , between chunks
    azfile.readBytesUntil(',', buffer,100);
    //Serial.printf("count=%d\r\n", count++);
  } // End of parsing file
  Serial.println("END");  
  azfile.close();
  free(buffer);
  return;
}

void getHighestNdxFilecode(int & maxndx, int & maxfilecode){
  
  valuesList.clear();

  maxndx = -1;
  maxfilecode = -1;

  File azfile = SD_MMC.open("/azmin.json", "r");
  if(!azfile){
    Serial.println("Failed to open file azmin.json");
  } 
  //Serial.println("file azmin.json opened");

  int count = 0;  
   
  char * buffer = (char *)  malloc(BUFFER_SIZE+1000);
  struct elvalues_type{
    char ndx[10]; 
    char artist[100]; 
    char song[200]; 
    char filecode[10];
    char tracks[200];
  }elvalues;
  while(azfile.available()){
    int totalbytes = 0;
    totalbytes += azfile.readBytes(buffer+totalbytes,BUFFER_SIZE);
    totalbytes += azfile.readBytesUntil('}',(char *)(buffer+totalbytes),1000);
    buffer[totalbytes++]='}';
    //Serial.printf(" now buffer contains a number of records: {...},{...},{...},{...} total %d bytes\r\n",totalbytes);
    //for(int i= 0;i<totalbytes;i++){ Serial.printf("%c",buffer[i]); }; Serial.println("");
    int curlyopen = 0;
    int curlyclose = 0;
    while(true){
      int co = myindex(buffer+curlyopen,'{');
      curlyopen = curlyopen + co;
      co = myindex(buffer+curlyopen,'}');
      curlyclose  = curlyopen + co;
      //Serial.printf("now a single record {...} is between curlyopen %d and curlyclose %d: \r\n",curlyopen,curlyclose);
      //for(int i=curlyopen; i<=curlyclose;i++){Serial.printf("%c", buffer[i]);} Serial.println("");
      int numelements = 0;
      int colon = curlyopen + 1;
      int comma;
      while(true){
        colon = colon + myindex(buffer+colon, ':') ; // get the beginning of value
        if(numelements < 4){
          comma = colon + myindex(buffer+colon, ',');  // get the end of value
        }else{  
          comma = curlyclose ;
        }
        // now colon and comma surrounds the element value      
        //Serial.printf("quoteopen=%d quoteclose=%d colon=%d comma=%d\r\n", 
        //            quoteopen, quoteclose, colon , comma);
        switch(numelements){
          case 0:
            memcpy(elvalues.ndx,buffer+colon+1,comma-colon-1);
            elvalues.ndx[comma-colon-1] = 0;
            break;
          case 1:
            memcpy(elvalues.artist,buffer+colon+1,comma-colon-1);
            elvalues.artist[comma-colon-1] = 0;
            break;
          case 2:
            memcpy(elvalues.song,buffer+colon+1,comma-colon-1);
            elvalues.song[comma-colon-1] = 0;
            break;
          case 3:
            memcpy(elvalues.filecode,buffer+colon+1,comma-colon-1);
            elvalues.filecode[comma-colon-1] = 0;
            break;
          case 4:
            memcpy(elvalues.tracks,buffer+colon+1,comma-colon-1);
            elvalues.tracks[comma-colon-1] = 0;
            break;
        }
        
        if(numelements == 4)
          break;
        numelements ++;
        colon = comma + 1;
      } // End of parsing 1 record
      
      // now elvalues contains the 5 values for this record
      //Serial.println("elvalues before:");
      //Serial.printf("elvalues.ndx=%s",elvalues.ndx);
      //Serial.printf("elvalues.artist=%s",elvalues.artist);
      //Serial.printf("elvalues.song=%s",elvalues.song);
      //Serial.printf("elvalues.filecode=%s",elvalues.filecode);
      //Serial.printf("elvalues.tracks=%s",elvalues.tracks);
      //Serial.println("");  
      
      //Serial.printf("curlyopen=%d curlyclose=%d filecode=%s filecode_arr=%s\r\n", curlyopen, curlyclose, elvalues.filecode, filecode_arr);
      elvalues.filecode[strlen(elvalues.filecode)-1] = 0; // null terminate above "
      int fc_int = atoi(elvalues.filecode+1); // convert to integer skipping starting "
      if( fc_int > maxfilecode)
        maxfilecode = fc_int;

      int ndx_int = atoi(elvalues.ndx); 
      if( ndx_int > maxndx)
        maxndx = ndx_int;


      curlyopen=curlyclose+1;

      if(curlyclose >= totalbytes-1)
        break;
    } // End of parsing a number of records 
    //Serial.printf("Exited when curlyclose=%d totalbytes=%d\r\n", curlyclose, totalbytes);
    // skip , between chunks
    azfile.readBytesUntil(',', buffer,100);
    //Serial.printf("count=%d\r\n", count++);
  } // End of parsing file
  Serial.println("END");  
  azfile.close();
  free(buffer);
  return;
}


bool cutFromTo(int from, int to){

  unsigned long start = millis();
  File azfile     = SD_MMC.open("/azmin.json", "r");
  if(!azfile){
    Serial.println("Failed to open file azmin.json");
    return false;
  } 
  //Serial.printf("%lu file azmin.json opened\r\n",millis()-start);
  
  //Serial.printf("%lu /open azmin_tmp.json for writing!\r\n", millis()-start);
  File azfiledest = SD_MMC.open("/azmin_tmp.json", "w", true);
  if(!azfiledest){
    Serial.println("Failed to open file azmin_tmp.json");
    azfile.close();
    return false;
  } 
  //Serial.printf("%lu file azmin_tmp.json opened\r\n", millis()-start);
  
  int count = 0;  
   
  char * buffer = (char *)  malloc(BUFFER_SIZE+1000);
  

  // copy from pos=0 to pos=from-1;
  //      from-1 =>|                                                                                
  // e.g.           {"n":59330,"a":"u2","s":"crumbs_from_your_table","f":"61510","t":"25_33_25_29_0"}
  int totalbytes = 0;
  while(azfile.available()&& totalbytes < from -1  ){
    int bytes2copy = azfile.readBytes(buffer,min(BUFFER_SIZE, from-totalbytes-1)); 
    totalbytes += bytes2copy;
    azfiledest.write( (unsigned char *) buffer, bytes2copy);
    //Serial.printf("%lu from=%d totalbytes=%d    copied %d bytes\r\n",millis()-start, from, totalbytes, bytes2copy);
  }

  // skip from "from" to "to" + 1
  // Serial.printf("%lu skip the record from %d to %d\r\n",millis()-start,from,to);
  azfile.readBytes(buffer, to-from+2); 
  
  // copy from pos=to+2 to the end;
  //      from-1 =>|                                                                                  |<= to+1
  // e.g.           {"n":59330,"a":"u2","s":"crumbs_from_your_table","f":"61510","t":"25_33_25_29_0"},
  //Serial.printf("%lu starting copying from %d to the end\r\n",millis()-start,to,azfile.size());
  int bytes2copy;
  int aval = azfile.available();
  if(aval > 2){
    while(azfile.available()){
      bytes2copy = azfile.readBytes(buffer,BUFFER_SIZE); 
      //totalbytes += bytes2copy;
      azfiledest.write( (unsigned char *) buffer, bytes2copy);
      //Serial.printf("%lu copied %d bytes\r\n",millis()-start, bytes2copy);
      if(bytes2copy == 0)
        break;
    }
  }else{
    azfiledest.seek(azfiledest.position());
    //Serial.printf("last char set to %c\r\n", ']');
    azfiledest.write(']');
  }

  azfile.close();
  azfiledest.close();
  free(buffer);
  return true;
}



bool appendNewRecord(String srcpath, String dstpath, int ndx, String artist, String song, String filecode, String tracks){

  unsigned long start = millis();
  File azfile     = SD_MMC.open(srcpath, "r");
  if(!azfile){
    Serial.printf("Failed to open file %s\r\n", srcpath.c_str());
    return false;
  } 
  
  File azfiledest = SD_MMC.open(dstpath, "w");
  if(!azfiledest){
    Serial.printf("Failed to open file %s \r\n", dstpath.c_str());
    azfile.close();
    return false;
  } 
  
  int count = 0;  
   
  char * buffer = (char *)  malloc(BUFFER_SIZE+1000);
    
  // copy the full file, escluding the closing ]
  int totalbytes = 0;
  while(azfile.available() && totalbytes < azfile.size()-1  ){
    int bytes2copy = azfile.readBytes(buffer,min(BUFFER_SIZE, (int) azfile.size() - 1 - totalbytes )); 
    totalbytes += bytes2copy;
    azfiledest.write( (unsigned char *) buffer, bytes2copy);
    // Serial.printf("%lu from=%d totalbytes=%d    copied %d bytes\r\n",millis()-start, totalbytes, bytes2copy);
  }
  // build the new record 
  String record = ",{\"n\":" + String(ndx) +",\"a\":\"" + artist + "\",\"s\":\"" + song + "\",\"f\":\"" + filecode + "\",\"t\":\"" + tracks + "\"}]";
  azfiledest.write( (const uint8_t *) record.c_str(), record.length());
  
  azfile.close();
  azfiledest.close();
  free(buffer);
  return true;
}

bool copy(String srcpath, String dstpath){

  unsigned long start = millis();
  File azfile     = SD_MMC.open(srcpath, "r");
  if(!azfile){
    Serial.printf("Failed to open file %s\r\n", srcpath.c_str());
    return false;
  } 
  
  File azfiledest = SD_MMC.open(dstpath, "w");
  if(!azfiledest){
    Serial.printf("Failed to open file %s \r\n", dstpath.c_str());
    azfile.close();
    return false;
  } 
  
   
  char * buffer = (char *)  malloc(BUFFER_SIZE+1000);
    
  // copy the full file
  while(azfile.available()  ){
    int bytes2copy = azfile.readBytes(buffer,BUFFER_SIZE); 
    azfiledest.write( (unsigned char *) buffer, bytes2copy);
    // Serial.printf("%lu from=%d totalbytes=%d    copied %d bytes\r\n",millis()-start, totalbytes, bytes2copy);
  }
  
  azfile.close();
  azfiledest.close();
  free(buffer);
  return true;
}

