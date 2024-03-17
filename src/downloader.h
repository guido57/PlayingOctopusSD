#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class Downloader{

  protected:
        HTTPClient http;
        WiFiClient * stream;
        int total_size_to_download;
        int count;
        // buffer parameters
        const uint8_t * dst;
        int dst_size;
        bool success;
        
  public:
    Downloader(){
        success = false;
    };
    ~Downloader(){};
    void Setup(String url, const uint8_t * dst_, int dst_size_ ){

        dst = dst_;
        dst_size = dst_size_;    
        count = 0;
        success = false;
        //Serial.print("[HTTP] begin...\n");

        // configure server and url
        http.begin(url);
        //http.begin("http://192.168.1.232:5000/static/73112.mid");
        //http.begin("http://192.168.1.12/test.html");
        //http.begin("192.168.1.12", 80, "/test.html");

        //Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();
        if(httpCode > 0) {
            // HTTP header has been sent and Server response header has been handled
            //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                //Serial.printf("httpCode=%d start downloading\r\n", httpCode);
  
                // get length of document (is -1 when Server sends no Content-Length header)
                total_size_to_download = http.getSize();
                if(total_size_to_download > dst_size){
                    Serial.printf("Downloader Error: total_size_to_download=%d > dst_size=%d\r\n", total_size_to_download,dst_size);    
                    http.end();
                    return;
                }else if(total_size_to_download ==0){
                    Serial.printf("http server said that the file size is 0\r\n");    
                    http.end();
                    return;
                }else if(total_size_to_download == -1){
                    Serial.printf("http server sent: no Content-Length header\r\n");    
                    http.end();
                    return;
                }else{
                    // Serial.printf("file length=%d\r\n", total_size_to_download);
                }
                // get tcp stream
                stream = http.getStreamPtr();
            }
        } else {
            Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
    };

    bool Loop(){

        if(!http.connected())    
            return false;
        // read all data from server
        if( count < total_size_to_download) {
            // get available data size
            size_t size = stream->available();
            //Serial.printf("stream->available()=%d\r\n", size);
                            
            if(size) {
                // read up to 1024 byte
                int rb = min(1024,total_size_to_download - count); 
                int c = stream->readBytes(((uint8_t *) dst)+count, rb);
                // write it to Serial
                //USE_SERIAL.write(buff, c);
                count += c;
                //Serial.printf("Received %d bytes. count=%d\r\n",c, count);
                if(count == total_size_to_download)
                    success = true;
            }
        }else{
            Serial.printf("count=%d total_size_to_download=%d http.end\r\n",count, total_size_to_download);
            http.end();
        }
 
        if(http.connected() )
            return true;
        else
            return false;    

    };

    bool Connected() {
        return http.connected();
    }
    bool Success() {
        return success;
    }
    int FileSize(){
        if(success)
            return total_size_to_download;
        else
            return -1;    
    }

}
;