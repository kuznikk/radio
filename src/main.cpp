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
#include "ESPAsyncWiFiManager.h"

AsyncWebServer server(80);
DNSServer dns;

// pripojeni na wifi:
boolean state = false;

char *URL = "http://icecast3.play.cz/evropa2-128.mp3"; //Nastaveni stanice
String stanice = "stanice";




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

  AsyncWiFiManager wifiManager(&server,&dns);
  wifiManager.autoConnect("RadioAP");
  Serial.println("connected");
  Serial.println(WiFi.localIP());

  if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  } 
  //Serial.println(WiFi.localIP());


   server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String());
  }); 

  server.on("/url", HTTP_POST, [](AsyncWebServerRequest *request) {    
    stanice = request->arg("url").c_str();    
    strcpy(URL, stanice.c_str());
    request->send_P(200, "text/json", "{\"result\":\"ok\"}");
  });


  audioLogger = &Serial;


  file = new AudioFileSourceICYStream(URL);
  file->RegisterMetadataCB(MDCallback, (void*)"ICY");
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
  Serial.println(URL);
  Serial.println(stanice);
  delay(2000);


}