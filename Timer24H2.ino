//参考サイト
//Webサーバー関連
// https://cherry-blossoms-bloomed.net/?p=152

#include <WiFi.h>
#include <EEPROM.h>
#include  <WiFiClient.h>
#include  <WebServer.h>
#include <string>
#include "LED.h"
#include "ntptimer.h"
#include "M5Atom.h"

#define LOOPCYCLE 1000 //ms
#define NUM_EVENT_TIME 4

#define MODE_EVER_ON  6
#define MODE_SLEEP    5
#define MODE_CONT_ON  4
#define MODE_TIMER    3
#define MODE_CONT_OFF 2
#define MODE_EVER_OFF 1

#pragma pack 1
struct st_event_time{
  uint16_t month; //作動する月　ビットで指定 0:1月 11:12月
  uint8_t day;  //作動する曜日　ビットで指定 0:SUN 6:SAT 
  uint8_t on_h;    //ON時　0~23
  uint8_t on_m;    //ON分  0~59
  uint8_t off_h;    //OF時　0~23
  uint8_t off_m;    //OFF分  0~59
  uint8_t pad;      //パディング
};
#pragma pack

//Web表示用　文字列、識別子定義
//タイマー設定時刻文字列
unsigned char event_day[7] = {1,1,1,1,1,1,1};  //設定曜日 1:ON　（日曜～土曜）
static String on_time[NUM_EVENT_TIME] =  {"11:01","12:01","13:01","14:01"}  ; //設定時刻文字列 HH:MM  4個 ここに記載の初期値が使われることはない
static String off_time[NUM_EVENT_TIME] = {"11:02","12:02","13:02","14:02"}  ; //設定時刻文字列 HH:MM  4個
static String svag_ontime[NUM_EVENT_TIME] = {"on_time1","on_time2","on_time3","on_time4"}; //server.argの引数
static String svag_offtime[NUM_EVENT_TIME] = {"off_time1","off_time2","off_time3","off_time4"};

//スリープ時間
static String sleep_time = "60"; //分
static  String svag_sleep_time = "sleep_time";
//インジケーター（未使用）
static String indicator[3]={
"<p style='background-color:rgb(236, 16, 9)'>ON</p>",
"<p style='background-color:rgb(16, 9, 236)'>タイマー</p>",
"<p style='background-color:rgb(64, 64, 64)'>OFF</p>",
};

//オペレーションモード
int opmode = MODE_EVER_ON; 
//タイマーによる出力ON/OFF状態
uint8_t timer_state = 0;
//スリープタイマー ダウンカウンタ(メインループ周期でカウントダウン)
uint32_t sleep_count = 0; 

void set_led_mode(uint8_t mode);
void wifi_setup(void); 
void ledtask(void* arg);
void ntp_setup();
void data_storage_setup();
EVENT_TIMER CheckTimerEvent();
void SetMode(uint8_t mode);

extern st_event_time event_time[];
extern uint16_t n_sleep_time;

WebServer server(80);

void debugWebParam(){
  String message = "Web param debug\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
   Serial.print("WebParam:");Serial.println(message);

}

//HH:MMを数値２つに変換
void Timestr2TimeInt(String sttime,uint8_t & hh,uint8_t & mm)
{
    String sbuf;
    char buf[5];
    uint8_t nbuf=0;
    int8_t colonpos = -1;
    for(int p=0; p<sttime.length(); p++){
      if((sttime[p])==':'){
        colonpos = p;
      }
    }

    if(colonpos == -1){
      hh = 99;
      mm = 99;
      return;
    }

    sbuf = sttime.substring(0,colonpos);
    sbuf.toCharArray(buf,5);
    hh = atoi(buf);

    sbuf = sttime.substring(colonpos+1);
    sbuf.toCharArray(buf,5);
    mm = atoi(buf);

    //Serial.print("hh="); Serial.println(hh);
    //Serial.print("mm="); Serial.println(mm);

}

//数値２つをHH:MMに変換
String TimeInt2Timestr(uint8_t hh,uint8_t mm)
{
  char buf[10]; 
  String sthhmm;
  if(hh==99 || mm==99){
    return "----";
  }
  sprintf(buf,"%02d:%02d",hh,mm);
  sthhmm = buf;
  //Serial.print("sthhmm:"); Serial.println(sthhmm);
  if(sthhmm.length() == 0){
      sthhmm = "---";
  }
  return sthhmm;
}


void handleRoot() {

  debugWebParam();
  if (server.method() == HTTP_POST) {
    Serial.println("server.method() = HTTP_POST");
    String val = server.arg("mode");
    Serial.print("mode:");   Serial.println(val);
    if (val == "2") {
        SetMode(MODE_EVER_ON);
    }else if (val == "1") {
        SetMode(MODE_CONT_OFF);
    }else if (val == "4") {
        SetMode(MODE_CONT_ON);
    }else if (val == "0") {
        SetMode(MODE_EVER_OFF);
    }else if (val == "3") {
        SetMode(MODE_SLEEP);
    }
   
    uint8_t on_h, on_m, off_h, off_m;
 
    //表示からタイマー設定取得・検証
    for(int i=0; i<NUM_EVENT_TIME; i++){
      String s_on = server.arg(svag_ontime[i]);
      //Serial.print(svag_ontime[i]);Serial.print(":"); Serial.println(s_on);
      Timestr2TimeInt(s_on, on_h, on_m);//検証し数値へ変換
      //Serial.print("on_h(int)="); Serial.println(on_h);
      //Serial.print("on_m(int)="); Serial.println(on_m);
        
      
      String s_off = server.arg(svag_offtime[i]);
      //Serial.print(svag_offtime[i]);Serial.print(":"); Serial.println(s_off);
      Timestr2TimeInt(s_off, off_h, off_m);
      //Serial.print("off_h(int)="); Serial.println(off_h);
      //Serial.print("off_m(int)="); Serial.println(off_m);
 
      //on_time[i] = s_on;   //保存文字列にサーバ文字列をそのままセットするのではなく、（数行下へ）
      //off_time[i] = s_off;
      event_time[i].on_h = on_h; 
      event_time[i].on_m = on_m;
      event_time[i].off_h = off_h;
      event_time[i].off_m = off_m;
      
      //表示用文字列へ保存数値（検証済み）をセットする
      on_time[i] = TimeInt2Timestr(event_time[i].on_h,event_time[i].on_m);  
      off_time[i]=TimeInt2Timestr(event_time[i].off_h,event_time[i].off_m); 

      Serial.print(svag_ontime[i]);Serial.print(":"); Serial.println(on_time[i]);
      Serial.print(svag_offtime[i]);Serial.print(":"); Serial.println(off_time[i]);

    }//for
    //スリープ時間取得  
    String s_sleep_time = server.arg("sleep_time");
    Serial.print("s_sleep_time");Serial.print(":"); Serial.println(s_sleep_time);
    char buf[5];
    s_sleep_time.toCharArray(buf,5);
    n_sleep_time = atoi(buf);
    if(n_sleep_time > 600){ n_sleep_time = 60;}
    sprintf(buf,"%d",n_sleep_time);
    sleep_time = buf;//表示文字列に検証後文字列をセット

    //保存する
    write_map();
  }else{
    Serial.println("server.method() != HTTP_POST (Get)");
    for(int i=0; i<NUM_EVENT_TIME; i++){
      //表示用文字列へ保存数値をセットする
      on_time[i] = TimeInt2Timestr(event_time[i].on_h,event_time[i].on_m);  //保存数値
      off_time[i]=TimeInt2Timestr(event_time[i].off_h,event_time[i].off_m); //保存数値
    }
  }


//HTML記述の注意：ダブルクォーテーション使用不可。行末に\n\を記述する。
 String mes = "\
  <html lang=\"ja\">\n\
  <meta charset=\"utf-8\">\n\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n\
  <head>\n\
   <title>IOTテーブルタップ</title>\n\
  </head>\n\
  <body style=\"font-family: sans-serif; background-color: #ffeeaa ;\" >\n\
   <h1>タイマー設定</h1>\n\
      <form action='' method='post'>\n\
        <p> :                         ON時刻　　　　　　　OFF時刻　    </br>\n\
            1：<input type='text' name='on_time1' value="+on_time[0] +" style='width:100px'>\n\
               <input type='text' name='off_time1' value="+off_time[0]+" style='width:100px'></br>\n\
            2：<input type='text' style='width:100px' name= 'on_time2' value="+on_time[1] +" >\n\
               <input type='text' name='off_time2' value="+off_time[1]+" style='width:100px'></br>\n\
            3：<input type='text' name='on_time3' value="+on_time[2] +" style='width:100px'>\n\
               <input type='text' name='off_time3' value="+off_time[2]+" style='width:100px'></br>\n\
            4：<input type='text' name='on_time4' value="+on_time[3] +" style='width:100px'>\n\
               <input type='text' name='off_time4' value="+off_time[3]+" style='width:100px'></p>\n\
            スリープ:<input type='text' name='sleep_time' value="+sleep_time +" style='width:100px'> 分</p>\n\
        <p><input type='submit' value=' 設定 '>\n\
        <input type='reset' value='キャンセル'></p>\n\
   <h1>モード</h1>\n\
   <p>\n\
        <p><button name='mode' value='2' style='width:100px;height:50px'>常時ON</button>\n\
        <button name='mode' value='0' style='width:100px;height:50px'>常時OFF</button></br></p>\n\
        <p><button name='mode' value='4' style='width:100px;height:50px'>タイマー</br>(ONで開始)</button>\n\
        <button name='mode' value='1' style='width:100px;height:50px'>タイマー</br>(OFFで開始)</button></br></P>\n\
        <button name='mode' value='3' style='width:100px;height:50px'>スリープ</button>\n\
      </form>\n\
    </p>\n\
  </body>\n\
  </html>\n";

  //Serial.println(mes);
  
  server.send(200, "text/html", mes);
}

void handleNotFound() {
 String message = "File Not Found\n\n";
 message += "URI: ";
 message += server.uri();
 message += "\nMethod: ";
 message += (server.method() == HTTP_GET) ? "GET" : "POST";
 message += "\nArguments: ";
 message += server.args();
 message += "\n";
 for (uint8_t i = 0; i < server.args(); i++) {
   message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
 }
 server.send(404, "text/plain", message);
}

void setup(void) {

  Serial.begin(115200);



  led_setup();
  wifi_setup();
  delay(1000);
  ntp_setup();
  data_storage_setup();



  /*
  char buf2[1024];
  int i;
  char *p;
  memset(buf,0,EEBUFSIZE);
  memset(buf2,0,1024);
  for(i=0,p=buf2; i<EEBUFSIZE; i++){
    sprintf(p,"%02X ",buf[i]);
    p+=5;
    if( i%16 == 0){
        sprintf(p,"\r\n");
        p+=2;
    }
  }
  Serial.println("EEPROM DUMP:");
  Serial.println(buf2);
*/


  // Wifi接続出来たら緑に点灯 500ms表示
  set_color(LED_GREEN);
  delay(500);
  
  // ルーティング
  // 第二引数でリクエストメソッドを指定する
  // リクエストパス：/display、 リクエストメソッド：POST
server.on("/", handleRoot);
//server.on("/display" ,HTTP_POST, handleDisplayText); 
  server.onNotFound(handleNotFound);

  // Webサーバ起動
  server.begin();
  Serial.println("HTTP server started");
  // Webサーバ起動したら青に点灯 1秒間
  set_color(LED_BLUE);
  delay(1000);
  set_color(LED_OFF);
  //初期状態にする
  opmode = MODE_EVER_ON;
  ctrl_output(1);
  set_led_mode(LED_RED);
  
  //LEDタスク起動
  xTaskCreatePinnedToCore(ledtask, "ledtask", 4096, NULL, 1, NULL, 0);
}

uint8_t curennt_output_status = 0;//現在の出力状態 電源投入時、リレーはOFF。

void ctrl_output(uint8_t c)
{
  if(c==1 && curennt_output_status == 0){
    Serial.println("Output ON");
    digitalWrite(25,1);
    curennt_output_status = 1;
  }
  if(c==0 && curennt_output_status == 1)
  {
    Serial.println("Output OFF");
    digitalWrite(25,0);
    curennt_output_status = 0;
  }
}

//オペレーションモードを設定
void SetMode(uint8_t mode)
{
  Serial.print("SetMode:");Serial.println(mode);
  opmode = mode;
  switch(mode){
    case MODE_EVER_ON:
        ctrl_output(1);
      set_led_mode(LED_RED);
      break;
    case MODE_SLEEP:
      set_led_mode(LED_BLINK_RED);
      //スリープタイマー開始
      sleep_count = (uint32_t)n_sleep_time * 60000/LOOPCYCLE;
      Serial.print("sleep_count:");Serial.println(sleep_count);
      break;
    case MODE_CONT_ON:
      timer_state = 1;
      ctrl_output(1);
      set_led_mode(LED_BLUE);
      break;
    case MODE_TIMER:
      if(timer_state == 0){
        ctrl_output(0);
        set_led_mode(LED_SOMETIME_BLUE);
      }else{//timer_state == 1
        ctrl_output(1);
        set_led_mode(LED_BLUE);
      }
      break;
    case MODE_CONT_OFF:
      timer_state = 0;
      ctrl_output(0);
      set_led_mode(LED_SOMETIME_BLUE2);
      break;
    case MODE_EVER_OFF:
        ctrl_output(0);
      set_led_mode(LED_OFF);
      break;

  }
}

void loop(void) {
  //M5.update();
  server.handleClient();

#if 0
  if (M5.Btn.wasPressed())
#endif
#if 1
  if(wasLEDButtonPressed())
  {
      Serial.println("Button pressed.");
      //トリガー入力によるモード切替
      if(opmode == MODE_EVER_ON){
        SetMode(MODE_SLEEP);
      }else if(opmode == MODE_SLEEP){
        SetMode(MODE_CONT_ON);
      }else if(opmode == MODE_CONT_ON){
        SetMode(MODE_TIMER);
      }else if(opmode == MODE_TIMER){
        SetMode(MODE_EVER_OFF);
      }else if(opmode == MODE_EVER_OFF){
        SetMode(MODE_CONT_OFF);
      }else if(opmode == MODE_CONT_OFF){
        SetMode(MODE_EVER_ON);

      }
  }
#endif
  EVENT_TIMER ret =  CheckTimerEvent();
   if(ret == EVENT_ON){
    if(timer_state == 0 || opmode == MODE_CONT_OFF){
      timer_state = 1;
      if(opmode == MODE_TIMER || opmode == MODE_CONT_OFF){
        ctrl_output(1);
        opmode = MODE_TIMER; //MODE_CONT_OFFだったらMODE_TIMERへ変更。
        set_led_mode(LED_BLUE);
      }
    }
  }else if (ret == EVENT_OFF){
    if(timer_state == 1 || opmode == MODE_CONT_ON){
      timer_state = 0;
      if(opmode == MODE_TIMER || opmode == MODE_CONT_ON){
        ctrl_output(0);
        opmode = MODE_TIMER; //MODE_CONT_ONだったらMODE_TIMERへ変更。
        set_led_mode(LED_SOMETIME_BLUE);
      }
    }
  }

  //スリープタイマーチェック
  if(sleep_count > 0){
    sleep_count--;
    //Serial.print("sleep_count:");Serial.println(sleep_count);
    if(sleep_count == 0 && opmode == MODE_SLEEP){
      //スリープ発動
      SetMode(MODE_TIMER);
    }
  }

  wasLEDButtonPressed(); //読み捨て
  delay(LOOPCYCLE);
}
