#include <Arduino.h>
#include <vector>
#include "FS.h"                // SD Card ESP32


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

