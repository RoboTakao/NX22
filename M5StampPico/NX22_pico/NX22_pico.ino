#include "M5Atom.h"
#include <DabbleESP32.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> //別途「Adafruit BusIO」ライブラリ必要

// OLED設定
#define SCREEN_WIDTH 128  //OLED 幅指定
#define SCREEN_HEIGHT 32  //OLED 高さ指定
#define OLED_RESET -1     //リセット端子（未使用-1）

#define CUSTOM_SETTINGS
#define INCLUDE_GAMEPAD_MODULE

// I2Cに接続されたSSD1306用「display」の宣言
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 変数宣言
int cnt = 0;  //ボタンを押した回数カウント用
// FastLED（CRGB構造体）設定
CRGB dispColor(uint8_t r, uint8_t g, uint8_t b) {
  return (CRGB)((r << 16) | (g << 8) | b);
}

const uint8_t Srv0 = 26; //GPIO Right Front
const uint8_t Srv1 = 18; //GPIO Right Back
const uint8_t Srv2 = 19; //GPIO Left Front
const uint8_t Srv3 = 25; //GPIO Left Back

const uint8_t srv_CH0 = 0, srv_CH1 = 1, srv_CH2 = 2, srv_CH3 = 3; //チャンネル
const double PWM_Hz = 50;   //PWM周波数
const uint8_t PWM_level = 14; //PWM 14bit(0～16384)

int pulseMIN = 410;  //0deg 500μsec 50Hz 14bit : PWM周波数(Hz) x 2^14(bit) x PWM時間(μs) / 10^6
int pulseMAX = 2048;  //180deg 2500μsec 50Hz 14bit : PWM周波数(Hz) x 2^14(bit) x PWM時間(μs) / 10^6

int cont_min = 0;
int cont_max = 180;

int angZero[] = {91,95,85,83}; //Trimming
int ang0[4];
int ang1[4];
int ang_b[4];
char ang_c[4];
float ts=100;  //100msごとに次のステップに移る
float td=10;   //20回で分割

int position_status = 0;

// Forward Step
int f_s[4][6]={
  {0,-30,30,0},
  {-30,30,-30,30},
  {-30,0,0,30},
  {30,-30,30,-30}};

// Back Step
int b_s[4][6]={
  {0,30,-30,0},
  {30,-30,30,-30},
  {30,0,0,-30},
  {-30,30,-30,30}};
  
// Left Turn_Step
int l_s[3][6]={
  {-30,30,30,-30},
  {-30,0,0,-30},
  {0,0,0,0}};
  
// Right Turn Step
int r_s[3][6]={
  {30,-30,-30,30},
  {30,0,0,30},
  {0,0,0,0}};
  
// Home position
int h_p[6]={0,0,0,0};

// Up Step
int u_s[6]={-15,-60,15,60};

// Down_Step
int d_s[6]={60,15,-60,-15};

// Right_Down_Step
int rd_s[6]={70,30,0,0};

// Left_Down_Step
int ld_s[6]={0,0,-70,-30};

int angry_state = 0;

void Initial_Value(){  //initial servo angle
  int cn = 50;
  for (int j=0; j <=3; j++){
    Srv_drive(j, angZero[j]);
    ang0[j] = angZero[j];
    ang1[j] = angZero[j];
    delay(cn);
  }
}

void face_clear(){
  display.clearDisplay();     //表示クリア
}

void face_center_eye(void *pvParameters){
  while(1)
  {
    face_clear();
    display.fillCircle(41, 15, 8, WHITE);      //円（塗り潰し）
    display.fillCircle(105, 15, 8, WHITE);      //円（塗り潰し）
    display.display();  //表示実行
    delay(1500);
    face_clear();
    display.fillRect(36, 12, 10, 7,WHITE);      //円（塗り潰し）
    display.fillRect(99, 12, 10, 7, WHITE);      //円（塗り潰し）
    display.display();  //表示実行
    delay(200);
  }
}

void face_center(){
  face_clear();
  display.fillCircle(41, 15, 8, WHITE);      //円（塗り潰し）
  display.fillCircle(105, 15, 8, WHITE);      //円（塗り潰し）
  display.display();  //表示実行
}

void face_up(){
  face_clear();
  display.fillCircle(41, 5, 8, WHITE);      //円（塗り潰し）
  display.fillCircle(105, 5, 8, WHITE);      //円（塗り潰し）
  display.display();  //表示実行
}

void face_down(){
  face_clear();
  display.fillCircle(41, 25, 8, WHITE);      //円（塗り潰し）
  display.fillCircle(105, 25, 8, WHITE);      //円（塗り潰し）
  display.display();  //表示実行
}

void face_right(){
  face_clear();
  display.fillCircle(31, 15, 8, WHITE);      //円（塗り潰し）
  display.fillCircle(94, 15, 8, WHITE);      //円（塗り潰し）
  display.display();  //表示実行
}

void face_left(){
  face_clear();
  display.fillCircle(51, 15, 8, WHITE);      //円（塗り潰し）
  display.fillCircle(115, 15, 8, WHITE);      //円（塗り潰し）
  display.display();  //表示実行
}

void Srv_drive(int srv_CH,int SrvAng){
  SrvAng = map(SrvAng, cont_min, cont_max, pulseMIN, pulseMAX);
  ledcWrite(srv_CH, SrvAng);
}


void servo_set(){
  int a[4],b[4];
  
  for (int j=0; j <=3 ; j++){
      a[j] = ang1[j] - ang0[j];
      b[j] = ang0[j];
      ang0[j] = ang1[j];
  }

  for (int k=0; k <=td ; k++){

      Srv_drive(srv_CH0, a[0]*float(k)/td+b[0]);
      Srv_drive(srv_CH1, a[1]*float(k)/td+b[1]);
      Srv_drive(srv_CH2, a[2]*float(k)/td+b[2]);
      Srv_drive(srv_CH3, a[3]*float(k)/td+b[3]);

      delay(ts/td);
  }
}

void forward_step()
{
  face_center();
  for (int i=0; i <=3 ; i++){
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + f_s[i][j];
    }
  servo_set();
  }
}

void back_step()
{
  face_center();
  for (int i=0; i <=3 ; i++){
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + b_s[i][j];
    }
  servo_set();
  }
}

void right_step()
{
  face_right();
  for (int i=0; i <=2 ; i++){
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + r_s[i][j];
    }
  servo_set();
  }
  face_center();
}

void left_step()
{
  face_left();
  for (int i=0; i <=2 ; i++){
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + l_s[i][j];
    }
  servo_set();
  }
  face_center();
}

void up_step()
{
  if(position_status == 0)
  {
    face_up();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + u_s[j];
    }
    servo_set();
    position_status = 1;
  }
  else if(position_status == 1)
  {
    face_center();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + h_p[j];
    }
    servo_set();
    position_status = 0;
  }
}

void down_step()
{
  if(position_status == 0)
  {
    face_down();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + d_s[j];
    }
    servo_set();
    position_status = 1;
  }
  else if(position_status == 1)
  {
    face_center();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + h_p[j];
    }
    servo_set();
    position_status = 0;
  }
}

void right_down_step()
{
  if(position_status == 0)
  {
    face_right();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + rd_s[j];
    }
    servo_set();
    position_status = 1;
  }
  else if(position_status == 1)
  {
    face_center();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + h_p[j];
    }
    servo_set();
    position_status = 0;
  }
}

void left_down_step()
{
  if(position_status == 0)
  {
    face_left();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + ld_s[j];
    }
    servo_set();
    position_status = 1;
  }
  else if(position_status == 1)
  {
    face_center();
    for (int j=0; j <=3 ; j++){
      ang1[j] = angZero[j] + h_p[j];
    }
    servo_set();
    position_status = 0;
  }
}

void home_position()
{
  for (int j=0; j <=3 ; j++){
    ang1[j] = angZero[j] + h_p[j];
  }
  servo_set();
}

void setup() {
  Serial.begin(151200);
  M5.begin(false, false, true);
  Wire.begin(21, 22);   //Grove端子をI2C設定(SDA,SCL)

  // OLED初期設定
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306:0 allocation failed"));
    for (;;); //エラーなら無限ループ
  }
  // OLED表示設定
  display.setTextColor(SSD1306_WHITE);  //文字色
  
  //M5.Lcd.setRotation(4);
  Dabble.begin("M5StampPico");       //set bluetooth name of your device
  
  pinMode(Srv0, OUTPUT);
  pinMode(Srv1, OUTPUT);
  pinMode(Srv2, OUTPUT);
  pinMode(Srv3, OUTPUT);
  
  //モータのPWMのチャンネル、周波数の設定
  ledcSetup(srv_CH0, PWM_Hz, PWM_level);
  ledcSetup(srv_CH1, PWM_Hz, PWM_level);
  ledcSetup(srv_CH2, PWM_Hz, PWM_level);
  ledcSetup(srv_CH3, PWM_Hz, PWM_level);

  //モータのピンとチャンネルの設定
  ledcAttachPin(Srv0, srv_CH0);
  ledcAttachPin(Srv1, srv_CH1);
  ledcAttachPin(Srv2, srv_CH2);
  ledcAttachPin(Srv3, srv_CH3);

  face_center();

  Initial_Value();

  xTaskCreatePinnedToCore(face_center_eye, "face_center_eye", 4096, NULL, 1, NULL, 1);
}

void loop() {
  M5.update();
  
  if ( M5.Btn.wasReleased() ) {
    Initial_Value();
  }
  
  Dabble.processInput();             //this function is used to refresh data obtained from smartphone.Hence calling this function is mandatory in order to get data properly from your mobile.
  
  int a = GamePad.getAngle();
  int b = GamePad.getRadius();
  
  if (GamePad.isTrianglePressed())
  {
    up_step();
    //USBSerial.println("UP STEP");
  }

  if (GamePad.isCrossPressed())
  {
    down_step();
    //USBSerial.println("DOWN STEP");
  }

  if (GamePad.isCirclePressed())
  {
    right_down_step();
    //USBSerial.println("RIGHT DOWN STEP");
  }

  if (GamePad.isSquarePressed())
  {
    left_down_step();
    //USBSerial.println("LEFT DOWN STEP");
  }

  if ((position_status == 0)&&(b < 5))
  {
    home_position();
  }
  else if(b >=5)
  {
    if ((a >= 45)&&(a < 135))
    {
      forward_step();
    }
    if ((a >= 135)&&(a < 225))
    {
      left_step();  
    }
    if ((a >= 225)&&(a < 315))
    {
      back_step();
    }
    if ((a >= 315)||(a < 45))
    {
      right_step();
    }
  }
  //delay(100);
}
