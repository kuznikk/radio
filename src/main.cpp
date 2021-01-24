#include <Arduino.h>
#include <SD.h>

#ifdef ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"


#include "ESPAsyncWebServer.h"
#include "ESPAsyncTCP.h"

AsyncWebServer server(80);

// pripojeni na wifi:
boolean state = false;

const char *SSID = "WLAN-456LB6";
const char *PASSWORD = "1grfJRD9m5rB6J0";
int stanice = 0;

const char *URL = "http://icecast3.play.cz/evropa2-128.mp3"; //Nastaveni stanice
const char *URL2 = "";
const char *URL3 = "";


AudioGeneratorMP3 *mp3;
AudioFileSourceICYStream *file;
AudioFileSourceBuffer *buff;
AudioOutputI2SNoDAC *out;

void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  (void) isUnicode; 
  char s1[32], s2[64];
  strncpy_P(s1, type, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  strncpy_P(s2, string, sizeof(s2));
  s2[sizeof(s2) - 1] = 0;
  Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
  Serial.flush();
}

// zavolá funkci, když nastane chyba
void StatusCallback(void *cbData, int code, const char *string)
{
  const char *ptr = reinterpret_cast<const char *>(cbData);
  char s1[64];
  strncpy_P(s1, string, sizeof(s1));
  s1[sizeof(s1) - 1] = 0;
  Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
  Serial.flush();
}

void pressInterrupt() {
  state = !state;
}

void setup()
{
  Serial.begin(115200);
  //delay(1000);
  Serial.println("Pripojovani k WiFi");
  //attachInterrupt(digitalPinToInterrupt(1), pressInterrupt, RISING);//zapoznámkoval jsem to, házelo mi to errory 

  /*WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);*/

  WiFi.begin(SSID, PASSWORD);

  // Funkce se bude spouštět do zastavení
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("...pripojovani k WiFi");
    delay(1000);
  }
  Serial.println("Pripojeno");

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  } 
  Serial.println(WiFi.localIP());


   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String());
  });

  server.on("/stanice0", HTTP_POST, [](AsyncWebServerRequest *request){
    stanice = 0;  
    request->send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/stanice1", HTTP_POST, [](AsyncWebServerRequest *request){
    stanice = 1;  
    request->send(200, "text/json", "{\"result\":\"ok\"}");
  });

  server.on("/stanice2", HTTP_POST, [](AsyncWebServerRequest *request){
    stanice = 2;  
    request->send(200, "text/json", "{\"result\":\"ok\"}");
  });

  

 

  audioLogger = &Serial;
  switch(stanice){
    case 0:
     file = new AudioFileSourceICYStream(URL);
     file->RegisterMetadataCB(MDCallback, (void*)"ICY");
      break;
    case 1:
     file = new AudioFileSourceICYStream(URL2);
     file->RegisterMetadataCB(MDCallback, (void*)"ICY");
      break;
    case 2:
     file = new AudioFileSourceICYStream(URL3);
     file->RegisterMetadataCB(MDCallback, (void*)"ICY");
      break;

   } 



  buff = new AudioFileSourceBuffer(file, 2048);
  buff->RegisterStatusCB(StatusCallback, (void*)"buffer");
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  mp3->RegisterStatusCB(StatusCallback, (void*)"mp3");
  mp3->begin(buff, out);

  server.begin();


   
}


void loop()
{ if (state) {
    static int lastms = 0;

    if (mp3->isRunning()) {
      if (millis() - lastms > 1000) {
        lastms = millis();
        Serial.printf("Bezi po dobu %d ms...\n", lastms);
        Serial.flush();
      }
      if (!mp3->loop()) mp3->stop();
    } else {
      Serial.printf("MP3 dohralo\n");
      delay(1000);
    }
  }

 
 Serial.println(stanice);
 delay(2000);

}