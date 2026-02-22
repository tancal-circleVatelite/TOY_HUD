/*
 * File:        TOY_HUD_software.ino
 * Author:      tancal
 * Created:     2025-12-04
 * Updated:     2026-02-22
 *
 * Description:
 *   TOY_HUD制御プログラム。Seeedstudio XIAO MG24 Sense 内蔵LSM6DS3から取得した3軸加速度、3軸角速度によりHUD表示をOLEDへ描画する。
 *
 * License:
 *   MIT License
 *
 * Notes:
 *   自由に改変・再頒布してください。
 */

#include <Arduino.h>
#include <LSM6DS3.h>
#include <Wire.h>
#include <U8g2lib.h>

//LSM6DS3のインスタンスと、計測値を入れる変数
LSM6DS3 myIMU(I2C_MODE, 0x6A);    //I2C device address 0x6A
float aX, aY, aZ, gX, gY, gZ;

// U8G2_SSD1306_128X64_NONAME_F_HW_I2C はSSD1306用の定義
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_MIRROR, /* reset=*/ U8X8_PIN_NONE);
int OLED_w = 128;
int OLED_h = 64;

//動作設定
bool CQB_show = true;         //右上の「CQB」を表示するか
bool PR_show = true;          //左上のピッチ角、ロール角を表示するか
bool closshair_followRoll = true;  //クロスヘアを鉛直・水平維持するか
bool middle_followRoll = false;       //真ん中の□を鉛直・水平維持するか
bool near_followRoll = false;         //手前の[]を鉛直・水平維持するか

void setup() {
  //内蔵IMU有効化
  pinMode(PD5,OUTPUT);
  digitalWrite(PD5,HIGH);
  myIMU.begin();

  //OLED開始
  u8g2.begin();  
  u8g2.setFont(u8g2_font_5x7_t_cyrillic); 

  //
  //★★★★★起動シーケンス★★★★★★★
  ////////////////////////////////////
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_t_cyrillic); 

  //文字
  // u8g2.drawStr(0, 8,  "Software Ver.:1.0");  
  drawText(0, 8,  "Software Ver.:1.0"); 
  u8g2.sendBuffer();
  delay(500);
  
  char buf[40]; 
  sprintf(buf, "Comp.Date:%s", __DATE__); 
  // u8g2.drawStr(0, 16, buf);
  drawText(0, 16, buf);
  u8g2.sendBuffer();
  delay(500);  

  // u8g2.drawStr(0, 24,  "Sensor:LSM6DS3");  
  drawText(0, 24,  "Sensor:LSM6DS3");
  u8g2.sendBuffer();
  delay(1000);  

  //格子点
  u8g2.clearBuffer();
  for (int k=0; k <= OLED_h; k=k+3){
    for (int i=0; i <= OLED_w; i=i+3){
      u8g2.drawPixel(i, k);
    }
    if (k % 9 == 0) {
      u8g2.sendBuffer();
    }
  }

  //十
  for (int i=0; i <= OLED_w; i=i+(OLED_w/3)){
    u8g2.drawLine(i, OLED_h/2, i+(OLED_w/3),OLED_h/2);
    u8g2.sendBuffer();    
  }
  for (int i=0; i <= OLED_h; i=i+(OLED_h/3)){
    u8g2.drawLine(OLED_w/2,i,OLED_w/2,i+(OLED_h/3)); 
    u8g2.sendBuffer();       
  }  
  
  //◎
  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/4,U8G2_DRAW_UPPER_RIGHT);
  u8g2.sendBuffer();
  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/4,U8G2_DRAW_UPPER_LEFT);
  u8g2.sendBuffer();
  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/4,U8G2_DRAW_LOWER_LEFT);
  u8g2.sendBuffer();
  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/4,U8G2_DRAW_LOWER_RIGHT);
  u8g2.sendBuffer();

  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/2,U8G2_DRAW_UPPER_RIGHT);
  u8g2.sendBuffer();
  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/2,U8G2_DRAW_UPPER_LEFT);
  u8g2.sendBuffer();
  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/2,U8G2_DRAW_LOWER_LEFT);
  u8g2.sendBuffer();
  u8g2.drawCircle(OLED_w/2,OLED_h/2,OLED_h/2,U8G2_DRAW_LOWER_RIGHT);
  u8g2.sendBuffer();

  delay(800);  

  //文字
  u8g2.clearBuffer();
  u8g2.drawStr((OLED_w/2) -35, 24,  "SYS CHECK OK");  
  u8g2.sendBuffer();
  delay(500);
  u8g2.drawStr((OLED_w/2) -35, 32,  "HUD START UP");  
  u8g2.sendBuffer();
  delay(800);

  //クロスヘアをギュルンってさせる  
  int x_offset_init[9] = {-20, -15, -10, -8, -6, -5, -3, -2, -1};
  int y_offset_init[9] = { 3,  4,  5, 5, 4, 3, 2, 1, 0};

  //初期アニメーション
  for (int i = 0; i < 9; i++) {    
    u8g2.clearBuffer();
    if (PR_show){
      u8g2.drawStr(0, 8,  "P:");
      u8g2.drawStr(0, 16,  "R:");
    }
    if (CQB_show){
      u8g2.drawStr(100, 8,  "CQB");
    }
    draw_closshair(0.0, x_offset_init[i], y_offset_init[i]);
    u8g2.sendBuffer();
    delay(20);
  }


  //★★★★★★★★★★★★★★★★★★★
  ////////////////////////////////////
}


void loop() {
  //加速度と角速度を取得　　　・・・フィルタ未実装・・・・・・
  aX = myIMU.readFloatAccelX();
  aY = myIMU.readFloatAccelY();
  aZ = myIMU.readFloatAccelZ(); 
  gX = myIMU.readFloatGyroX();
  gY = myIMU.readFloatGyroY();
  gZ = myIMU.readFloatGyroZ();

  //ピッチ角とロール角を計算
  float pitch = atan2(-aX, sqrt(aY * aY + aZ * aZ)) * 180.0 / PI;
  float roll  = atan2(aY, aZ) * 180.0 / PI;                           


  ///////////
  //姿勢＝重力加速度を含む加速度計の値に応じた照準オフセットを加える
  //水平の時は中心に表示、姿勢(ロールとピッチ）に応じてオフセットする
  //オフセットの量はテキトーに、「加速度の値　×　係数」とする
  //照準座標xのオフセット ：OLEDの横方向　→右ロールの時に右に、左ロールの時に左に　→aYの値でオフセット量を決定
  int x_offset;
  x_offset = (int)(aY * 30);

  //照準座標yのオフセット　：OLEDの縦方向　→ピッチ上向き(aXが正)の時にちょい下げ、ピッチ下向き(aXが負)の時に大きく下げ　→aXの値でオフセット量を決定
  int y_offset;
  if (aX > 0) {
    // aXが正のときの処理
    y_offset = (int)(aX * 20);
  } else if (aX < 0) {
    // aXが負のときの処理
    y_offset = -(int)(aX * 28);
  } else {
    // aXが0のときの処理（必要なら）
    y_offset = 0;
  }
  /////////////////

  /////////////////
  //角速度によるオフセットも加える　　　というかHUDサイトの挙動としてはこっちがメイン
  //角速度を検知しているとき→銃を振り回しているときなので、慣性？で照準が遅れてついてくるイメージ
  //OLEDのX軸(横方向)＝Z軸角速度gZ　、　OLEDのY軸(縦方向)＝Y軸加速度gY     ,（gXは銃身のねじり方向なのでそんなに大きく動くことはないだろう。なので考慮しない・・・）
  //gZ,gYが200くらいで画面淵まで振り切る感じ
  //→gZが200の時にオフセットOLED_w/2とすると、OLED_w/2/200＝0.OLED_h/2なので、切り捨てて係数0.3とする
  //→gYが200の時にオフセットOLED_h/2とすると、OLED_h/2/200＝0.16なので、ちょい切り捨てて0.12とする  　　　と思うじゃん？？なんか違ったので左記はボツ
  //値の正負は、gZが正の時照準は右に、gYが正の時は照準は上に　
  x_offset = x_offset + (gZ * 0.3);
  //y_offset = y_offset - (gY * 0.2);  
  //上下方向の動き、特に、銃を振り下げたときの照準の上振れ追従が鈍い気がしたので、振り下げと振り上げで係数を分ける  前者は0.4、後者が0.2
  if (gY > 0) {
    // gYが正のときの処理
    y_offset = y_offset - (gY * 0.4);
  } else if (aX < 0) {
    // gYが負のときの処理
    y_offset = y_offset - (gY * 0.2);
  } else {
    // gYが0のときの処理
    y_offset = y_offset;
  }

  /////////////////

  /////////////////
  //描画
  u8g2.clearBuffer();

  //クロスヘアをバッファへ
  draw_closshair(roll, x_offset, y_offset);

  //装飾
  //「CQB」
  if (CQB_show){
    u8g2.setFont(u8g2_font_5x7_t_cyrillic); 
    u8g2.drawStr(100, 8,  "CQB");
  }

  //ピッチ角とロール角を表示
  if (PR_show){
    u8g2.setFont(u8g2_font_5x7_t_cyrillic); 
    char buf[48];
    snprintf(buf, sizeof(buf), "P: %+.1f", -pitch);
    u8g2.drawStr(0, 8,  buf);
    snprintf(buf, sizeof(buf), "R: %+.1f", roll);
    u8g2.drawStr(0, 16,  buf);
  }

  //描画実行
  u8g2.sendBuffer();
  
}



///文字列を一文字ずつ描画していく関数
/**
 * @brief u8g2.drawstrを置き換えるように使う、文字列を一文字ずつ流れるように描画する
 * @param x 描画開始座標のX　drawstrと同じ
 * @param y 描画開始座標のY　drawstrと同じ
 * @param str 描画する文字列　drawstrと同じ
 * @note sendbufferまで行う
*/
void drawText(int x, int y, char* str) {
  for (int i = 0; str[i] != '\0'; i++) {
    u8g2.drawGlyph(x + i * 5, y, str[i]);
    if (i % 2 == 0) {
      u8g2.sendBuffer();  //結構遅いので、2文字ずつ描画する
    }
  }
}


///クロスヘアをバッファへ書き込む関数
/**
 * @brief クロスヘアの線や点をU8h2の描画バッファへ書き込む
 * @param roll ロール角（ラジアン）
 * @param x_offset クロスヘア位置のX軸オフセット量
 * @param y_offset クロスヘア位置のY軸オフセット量
 * @note clearbufferとsedbufferは行っていない。引数をどちらもゼロにしたら当然、OLEDの中央に表示される。オフセット量は照準3点のうち最も遠い点での値。
 */
void draw_closshair (float roll, int x_offset, int y_offset){
  //本家のCQBモードを模擬する… 一番奥にクロスヘア、真ん中に四隅のみの四角、手前にカギかっこ  
  //ロール角を使って水平・鉛直を保つように表示する
  //奥、真ん中、手前のぞれぞれで、設定に応じて水平鉛直維持を適用するか分ける

  //まず一番奥　＝　位置はオフセットそのまま
  if (closshair_followRoll) { //水平鉛直維持あり ＝ ロール角の値から線の向き・長さを調整する
    roll = roll * DEG_TO_RAD;
    u8g2.drawPixel(OLED_w/2 + x_offset, OLED_h/2 + y_offset); //原点　＝　サイト
    u8g2.drawLine(OLED_w/2 + x_offset + (3 * cos(roll + PI*0/2)), OLED_h/2 + y_offset - (3 * sin(roll + PI*0/2)) , OLED_w/2 + x_offset + (5 * cos(roll + PI*0/2)), OLED_h/2 + y_offset - (5 * sin(roll + PI*0/2))); 
    u8g2.drawLine(OLED_w/2 + x_offset + (3 * cos(roll + PI*1/1)), OLED_h/2 + y_offset - (3 * sin(roll + PI*1/1)) , OLED_w/2 + x_offset + (5 * cos(roll + PI*1/1)), OLED_h/2 + y_offset - (5 * sin(roll + PI*1/1))); 
    u8g2.drawLine(OLED_w/2 + x_offset + (3 * cos(roll - PI*1/2)), OLED_h/2 + y_offset - (3 * sin(roll - PI*1/2)) , OLED_w/2 + x_offset + (5 * cos(roll - PI*1/2)), OLED_h/2 + y_offset - (5 * sin(roll - PI*1/2))); 
  }else{        //水平鉛直維持なし ＝ 常にOLEDの縦横と並行のクロスヘア
    u8g2.drawPixel(OLED_w/2 + x_offset, OLED_h/2 + y_offset);
    u8g2.drawLine(OLED_w/2 + x_offset + 2, OLED_h/2 + y_offset, OLED_w/2 + x_offset + 3, OLED_h/2 + y_offset);
    u8g2.drawLine(OLED_w/2 + x_offset - 2, OLED_h/2 + y_offset, OLED_w/2 + x_offset - 3, OLED_h/2 + y_offset);
    u8g2.drawLine(OLED_w/2 + x_offset, OLED_h/2 + y_offset + 2, OLED_w/2 + x_offset, OLED_h/2 + y_offset + 3);
  }

  //次に真ん中 = オフセット0.7がけ　　　四隅＝照準位置+6＝原点+オフセット+6　、　線の長さ3
  if (middle_followRoll) { //水平鉛直維持あり ＝ ロール角の値から線の向き・長さを調整する
    //右上
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll + PI*1/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll + PI*1/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll + PI*1/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll + PI*1/8)));  //右上の―
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll + PI*1/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll + PI*1/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll + PI*3/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll + PI*3/8)));  //右上の｜
    //右下
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll + PI*3/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll + PI*3/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll + PI*5/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll + PI*5/8)));
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll + PI*3/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll + PI*3/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll + PI*7/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll + PI*7/8)));
    //左上
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll - PI*1/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll - PI*1/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll - PI*1/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll - PI*1/8)));
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll - PI*1/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll - PI*1/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll - PI*3/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll - PI*3/8)));
    //左下
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll - PI*3/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll - PI*3/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll - PI*5/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll - PI*5/8)));
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + (8 * cos(roll - PI*3/4)),     OLED_h/2 + (y_offset*0.7) - (8 * sin(roll - PI*3/4)) ,    OLED_w/2 + (x_offset*0.7) + (6.4 * cos(roll - PI*7/8)),     OLED_h/2 + (y_offset*0.7) - (6.4 * sin(roll - PI*7/8)));
  }else{        //水平鉛直維持なし ＝ 常にOLEDの縦横と並行
    //右上
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + 6, OLED_h/2 + (y_offset*0.7) - 6, OLED_w/2 + (x_offset*0.7) + 3, OLED_h/2 + (y_offset*0.7) - 6);  //右上の―
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + 6, OLED_h/2 + (y_offset*0.7) - 6, OLED_w/2 + (x_offset*0.7) + 6, OLED_h/2 + (y_offset*0.7) - 3);  //右上の｜
    //右下
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + 6, OLED_h/2 + (y_offset*0.7) + 6, OLED_w/2 + (x_offset*0.7) + 3, OLED_h/2 + (y_offset*0.7) + 6);
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) + 6, OLED_h/2 + (y_offset*0.7) + 6, OLED_w/2 + (x_offset*0.7) + 6, OLED_h/2 + (y_offset*0.7) + 3);
    //左上
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) - 6, OLED_h/2 + (y_offset*0.7) - 6, OLED_w/2 + (x_offset*0.7) - 3, OLED_h/2 + (y_offset*0.7) - 6); 
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) - 6, OLED_h/2 + (y_offset*0.7) - 6, OLED_w/2 + (x_offset*0.7) - 6, OLED_h/2 + (y_offset*0.7) - 3);
    //左下
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) - 6, OLED_h/2 + (y_offset*0.7) + 6, OLED_w/2 + (x_offset*0.7) - 3, OLED_h/2 + (y_offset*0.7) + 6);
    u8g2.drawLine(OLED_w/2 + (x_offset*0.7) - 6, OLED_h/2 + (y_offset*0.7) + 6, OLED_w/2 + (x_offset*0.7) - 6, OLED_h/2 + (y_offset*0.7) + 3);    
  }

  //最後に手前 = オフセット0.3がけ　ほとんど動かない　　近いほど初速を保ってて直進に近い・・・はず　　　　距離14ピクセルの円上にカギかっこ状に点を打つ
  if (near_followRoll) { //水平鉛直維持あり ＝ ロール角の値から線の向き・長さを調整する
    //右側
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + (14 * cos(roll + PI*0.8/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll + PI*0.8/4)) ,    OLED_w/2 + (x_offset*0.3) + (14 * cos(roll - PI*0.8/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll - PI*0.8/4)));
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + (14 * cos(roll + PI*0.8/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll + PI*0.8/4)) ,    OLED_w/2 + (x_offset*0.3) + (14 * cos(roll + PI*1.1/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll + PI*1.1/4)));
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + (14 * cos(roll - PI*0.8/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll - PI*0.8/4)) ,    OLED_w/2 + (x_offset*0.3) + (14 * cos(roll - PI*1.1/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll - PI*1.1/4)));
    //左側
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + (14 * cos(roll + PI*3.2/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll + PI*3.2/4)) ,    OLED_w/2 + (x_offset*0.3) + (14 * cos(roll - PI*3.2/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll - PI*3.2/4)));
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + (14 * cos(roll + PI*3.2/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll + PI*3.2/4)) ,    OLED_w/2 + (x_offset*0.3) + (14 * cos(roll + PI*2.9/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll + PI*2.9/4)));
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + (14 * cos(roll - PI*3.2/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll - PI*3.2/4)) ,    OLED_w/2 + (x_offset*0.3) + (14 * cos(roll - PI*2.9/4)),     OLED_h/2 + (y_offset*0.3) - (14 * sin(roll - PI*2.9/4)));
  }else{        //水平鉛直維持なし ＝ 常にOLEDの縦横と並行
    //右上
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + 12, OLED_h/2 + (y_offset*0.3) - 8, OLED_w/2 + (x_offset*0.3) + 7, OLED_h/2 + (y_offset*0.3) - 11);  //右上の＼
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + 12, OLED_h/2 + (y_offset*0.3) - 8, OLED_w/2 + (x_offset*0.3) + 12, OLED_h/2 + (y_offset*0.3));      //右上の｜
    //右下
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + 12, OLED_h/2 + (y_offset*0.3) + 8, OLED_w/2 + (x_offset*0.3) + 7, OLED_h/2 + (y_offset*0.3) + 11); 
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) + 12, OLED_h/2 + (y_offset*0.3) + 8, OLED_w/2 + (x_offset*0.3) + 12, OLED_h/2 + (y_offset*0.3)); 
    //左上
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) - 12, OLED_h/2 + (y_offset*0.3) - 8, OLED_w/2 + (x_offset*0.3) - 7, OLED_h/2 + (y_offset*0.3) - 11); 
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) - 12, OLED_h/2 + (y_offset*0.3) - 8, OLED_w/2 + (x_offset*0.3) - 12, OLED_h/2 + (y_offset*0.3));
    //左下
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) - 12, OLED_h/2 + (y_offset*0.3) + 8, OLED_w/2 + (x_offset*0.3) - 7, OLED_h/2 + (y_offset*0.3) + 11); 
    u8g2.drawLine(OLED_w/2 + (x_offset*0.3) - 12, OLED_h/2 + (y_offset*0.3) + 8, OLED_w/2 + (x_offset*0.3) - 12, OLED_h/2 + (y_offset*0.3));
  }

}


