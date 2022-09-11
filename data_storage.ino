#include "M5Atom.h"

#define EEBUFSIZE 256
#define EEVER 2 //EEPROMアドレスマップのバージョン

st_event_time event_time[NUM_EVENT_TIME];
uint16_t n_sleep_time;


//EEPROMをデフォルト値で初期化する。先頭にマップバージョンをセットする
void init_map()
{
    st_event_time et;
    et.day = 0x7f;
    et.month = 0xfff;
    et.on_h = 7;
    et.on_m = 0;
    et.off_h = 17;
    et.off_m = 59;
    et.pad = 0;

    Serial.print("INITMAP:"); Serial.println(EEVER);

    EEPROM.writeByte(0,EEVER);

    int eeaddr = 1;
    for(int i=0; i<NUM_EVENT_TIME; i++){
        Serial.print("eeaddr:"); Serial.println(eeaddr);
        EEPROM.put<st_event_time>(eeaddr,et);
        eeaddr += sizeof(st_event_time);
    }
    Serial.print("eeaddr:"); Serial.println(eeaddr);
    EEPROM.put<uint16_t>(eeaddr,60);
    eeaddr += sizeof(uint16_t);

    EEPROM.commit();
}




void data_storage_setup()
{
  EEPROM.begin(EEBUFSIZE);
  Serial.println("EEPROM READ");
  uint8_t eever = EEPROM.readByte(0);
    Serial.print("eever:"); Serial.println(eever);
    if(eever != EEVER){
        init_map();
    }

    //readmap
    Serial.println("readmap:");
    char buf[10]; 
    String strOnTime; //"HH:MM"
    String strOffTime; //"HH:MM"
    int eeaddr = 1;
    st_event_time et;

    //データ読み込み（文字列と構造体へ）
    for(int i=0; i<NUM_EVENT_TIME; i++){
        EEPROM.get<st_event_time>(eeaddr,et);
        event_time[i] = et;//構造体へ
        if(et.on_h==0 && et.on_m ==0 && et.off_h==0 && et.off_m ==0)
        {
            //On/Offとも00:00の場合は"未設定”として空白をセットする
            strOnTime = "";
            strOffTime = "";
        }else{
            //通常。
            sprintf(buf,"%02d:%02d",et.on_h,et.on_m);
            strOnTime = buf;
            sprintf(buf,"%02d:%02d",et.off_h,et.off_m);
            strOffTime = buf;
        }
        Serial.print("i:"); Serial.println(i);
        Serial.print("on time:"); Serial.println(strOnTime);
        on_time[i] = strOnTime;
        Serial.print("off time:"); Serial.println(strOffTime);
        off_time[i] = strOffTime;
        eeaddr += sizeof(st_event_time);
    }

    EEPROM.get<uint16_t>(eeaddr,n_sleep_time);
    Serial.print("on_sleep_time:"); Serial.println(n_sleep_time);

}

//保存すべきデータをEEPROMへ保存する
void write_map()
{
    Serial.println("write_map():");
    int eeaddr = 1;
    for(int i=0; i<NUM_EVENT_TIME; i++){
        Serial.print("eeaddr:"); Serial.println(eeaddr);
        EEPROM.put<st_event_time>(eeaddr,event_time[i]);
        eeaddr += sizeof(st_event_time);
    }
    Serial.print("eeaddr:"); Serial.println(eeaddr);
    EEPROM.put<uint16_t>(eeaddr,n_sleep_time);
    eeaddr += sizeof(uint16_t);
    EEPROM.commit();
}
