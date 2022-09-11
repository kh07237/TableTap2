#include <WiFi.h>
#include "time.h"
#include "ntptimer.h"


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600*9;
const int   daylightOffset_sec = 3600*0;



void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "NTP Started:%A, %B %d %Y %H:%M:%S");
}

void ntp_setup()
{
  //Serial.begin(115200);
  
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

}



EVENT_TIMER CheckTimerEvent()
{
  char str[256];
  char current_time[16];
  char current_day[16];
  struct tm timeinfo;

  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return EVENT_NONE;
  }

  strftime(current_time, 16, "%OH:%OM", &timeinfo);
  strftime(current_day, 16, "%w", &timeinfo);

  unsigned int nday=0;
  nday = atoi(current_day);
  //Serial.print("nday:");Serial.println(nday);
  if(event_day[nday] == 1){
    for(int i=0; i< NUM_EVENT_TIME; i++){
      if(timeinfo.tm_hour == event_time[i].on_h && timeinfo.tm_min == event_time[i].on_m)
      {
        Serial.print("match ontime:");
        Serial.println(current_time);
        return EVENT_ON;
      }
      if(timeinfo.tm_hour == event_time[i].off_h && timeinfo.tm_min == event_time[i].off_m)
      {
        Serial.print("match offtime:");
        Serial.println(current_time);
        return EVENT_OFF;
      }

    }
  }
  return EVENT_NONE;
}
