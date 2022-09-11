#include <M5Atom.h>
#include <FastLED.h>
#include "LED.h"

#define CYCLE_TIME 10  //ms 1サイクルの時間（delayの時間）
#define NUM_STEP 4 //色変化ステップ数最大
int16_t disp_table[NUM_STEP]; //ステップごとの点灯色
int16_t count_table[NUM_STEP];//ステップごとの点灯時間　　-1:無限


uint8_t DisBuff[2 + 5 * 5 * 3];
int color1_time = 0;
int color2_time = 0;
uint8_t color1 = LED_RED;
uint8_t color2 = LED_RED;


void setBuff(uint8_t Rdata, uint8_t Gdata, uint8_t Bdata)
{
    DisBuff[0] = 0x05;
    DisBuff[1] = 0x05;
    for (int i = 0; i < 25; i++)
    {
        DisBuff[2 + i * 3 + 0] = Rdata;
        DisBuff[2 + i * 3 + 1] = Gdata;
        DisBuff[2 + i * 3 + 2] = Bdata;
    }
}




void led_setup(void) {
  M5.begin(true,false,true);
  pinMode(25, OUTPUT);
}

uint8_t  new_mode = LED_OFF; //変更後のモード（スレッド間共有）
uint8_t  cur_mode = LED_OFF; //現在のモード

//メインスレッドからコールされる関数
void set_led_mode(uint8_t mode)
{
  new_mode = mode;//これ以上のことはここでは何もしない。
}


void set_color(uint8_t color)
{
    //Serial.print("set_color = ");  Serial.println(color);
    switch(color){
    case LED_OFF:
      default:
        setBuff(0x00, 0x00, 0x00);
        break;
      case LED_RED:
        setBuff(0xff, 0x00, 0x00);
        break;
      case LED_GREEN:
        setBuff(0x00, 0xff, 0x00);
        break;
      case LED_BLUE:
        setBuff(0x00, 0x00, 0xff);
        break;
    }
    M5.dis.displaybuff(DisBuff);

}


uint8_t fButton = 0;
bool wasLEDButtonPressed()
{
  if (fButton == 1){
    fButton = 0;
    return true;
  }else{
    return false;
  }
}


void ledtask(void* arg)
{
  Serial.println("LED task started.");
  int count = 0;
  int step = 0; //色変化ステップ数
  while(1){
#if 1
    const uint8_t buttonA_GPIO = 39;
    if(digitalRead(buttonA_GPIO) == 0){
      fButton = 1;
      //Serial.println("Button pressed(ledtask_gpio).");
    }
#endif
#if 0
    M5.update();
#endif
    //モード変更検出
    if(new_mode != cur_mode){  //new_modeはメインスレッドから書き換えられる可能性あり
      cur_mode = new_mode;
      //Serial.print("LED_MODE = ");  Serial.println(cur_mode);
      //モード切替処理
      for(int i=0; i<NUM_STEP; i++){
        disp_table[i]=-1;
      }
      switch(cur_mode){//モードに応じて点滅パターンを設定する
        case LED_SOMETIME_GREEN:
          disp_table[0] = LED_GREEN;
          count_table[0] = 1000/CYCLE_TIME;
          disp_table[1] = LED_OFF;
          count_table[1] = 5900/CYCLE_TIME;
          break;
       case LED_SOMETIME_BLUE2:
          disp_table[0] = LED_BLUE;
          count_table[0] = 1000/CYCLE_TIME;
          disp_table[1] = LED_OFF;
          count_table[1] = 5000/CYCLE_TIME;
          break;
       case LED_SOMETIME_BLUE:
          disp_table[0] = LED_BLUE;
          count_table[0] = 300/CYCLE_TIME;
          disp_table[1] = LED_OFF;
          count_table[1] = 5700/CYCLE_TIME;
          break;
        case LED_RED:
          disp_table[0] = LED_RED;
          count_table[0] = -1;
          break;
        case LED_BLINK_RED_BLUE:
          disp_table[0] = LED_RED;
          count_table[0] = 1000/CYCLE_TIME;
          disp_table[1] = LED_BLUE;
          count_table[1] = 1000/CYCLE_TIME;
          break;
        case LED_BLINK_RED:
          disp_table[0] = LED_RED;
          count_table[0] = 1000/CYCLE_TIME;
          disp_table[1] = LED_OFF;
          count_table[1] = 1000/CYCLE_TIME;
          break;
        case LED_BLUE:
          disp_table[0] = LED_BLUE;
          count_table[0] = -1;
          break;
        case LED_GREEN:
          disp_table[0] = LED_GREEN;
          count_table[0] = -1;
          break;
        case LED_BLINK_GREEN:
          disp_table[0] = LED_GREEN;
          count_table[0] = 500/CYCLE_TIME;
          disp_table[1] = LED_OFF;
          count_table[1] = 500/CYCLE_TIME;
          break;
        case LED_OFF:
          disp_table[0] = LED_OFF;
          count_table[0] = -1;
          break;
      }
      step = 0;
      count = count_table[step];
      set_color(disp_table[step]);
      //モード切替処理ここまで
    }
    if(  cur_mode == LED_SOMETIME_GREEN 
      || cur_mode == LED_SOMETIME_BLUE
      || cur_mode == LED_SOMETIME_BLUE2
      || cur_mode == LED_BLINK_RED_BLUE
      || cur_mode == LED_BLINK_RED
      || cur_mode == LED_BLINK_GREEN)
    {
      if(count > 0){
        count--;
      }
      if(count == 0){//tickカウントが0になったら次のステップへ
        if(step < NUM_STEP){
          step++;
        }
        if(step == NUM_STEP ){//上限を超えたら0に戻す
          step = 0;
        }else if(disp_table[step] == -1){//未使用なら0に戻す
          step = 0;
        }
        set_color(disp_table[step]);
        count = count_table[step];
        //Serial.print("count = ");  Serial.println(count);

      }
    }else{ //連続
        set_color(cur_mode);
    }
#if 0
    if (M5.Btn.wasPressed()){
      Serial.println("Button pressed(ledtask).");
    }
#endif  
    delay(CYCLE_TIME);
  }
}