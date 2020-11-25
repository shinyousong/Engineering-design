#include <Servo.h>

// Arduino pin assignment
#define PIN_SERVO 10
#define PIN_IR A0

// Framework setting
#define _DIST_TARGET 255 //목표 거리
#define _DIST_MIN 100 //최소 거리
#define _DIST_MAX 430//최대 거리

// Servo range
#define _DUTY_MEU 1476 //90도
#define _DUTY_MIN 1575 //최소 높이, 서보값에 100도를 입력했을 때 실제 프레임워크 가동 시 80도로 나옴
#define _DUTY_MAX 1368 //최대 높이, 서보값에 80도를 입력했을 때 실제 프레임워크 가동 시 100도로 나옴

// period setting
#define INTERVAL 25 // sampling interval (unit: ms)

//filter setting
#define _DIST_MEDIAN 30 // 중위수필터 샘플값
#define _DIST_ALPHA 0.5 //EMA필터
#define KP 0.37 //P제어 비례이득
#define KP2 0.29 //P제어 비례이득2

int _dist_target = _DIST_TARGET;
int _dist_min = _DIST_MIN;
int _dist_max = _DIST_MAX;

int _duty_meu = _DUTY_MEU;
int _duty_min = _DUTY_MIN;
int _duty_max = _DUTY_MAX;

unsigned long last_sampling_time; // unit: ms

float median, tem; //two variable for 중위수필터
int median_list[_DIST_MEDIAN], tem_median_list[_DIST_MEDIAN]; //two array for 중위수필터
float dist_ema, alpha; //two variable for ema필터
int count = 0; //cali필터 측정을 위한 변수
float sum = 0; //cali필터 측정을 위한 변수


float kP = KP; //P제어 비례이득 1
float kP2 = KP2;

Servo myservo;


void setup() {
// initialize GPIO pins
  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_duty_meu);

// initialize serial port
  Serial.begin(57600);
  last_sampling_time = 0;

  median = 0.0;
  tem = 0.0;
  for (int i = 0; i < _DIST_MEDIAN; i++){
    median_list[i] = 0;
    tem_median_list[i] = 0;
  }
  alpha = _DIST_ALPHA;
}

float ir_distance(void){ // return value unit: mm
  float val;
  float volt = float(analogRead(PIN_IR));
  val = ((6762.0/(volt-9.0))-4.0) * 10.0;
  return val;
}

float medianfilter(float dist){ //중위수필터 보정
  tem = median_list[_DIST_MEDIAN-1];
  for (int i = 0; i < _DIST_MEDIAN; i++){
    median_list[_DIST_MEDIAN-1-i] = median_list[_DIST_MEDIAN-2-i];
  }
  median_list[0] = dist;

  for (int i = 0; i <_DIST_MEDIAN; i++){
    if (tem_median_list[i] == tem){
      for (int j = 0; j < i; j++){
        tem_median_list[i-j] = tem_median_list[i-1-j];
      }
      break;
    }
  }
  tem_median_list[0] = -1;
  
  for (int i = 0; i <_DIST_MEDIAN; i++){
    if (tem_median_list[i] >= dist){
      for (int j = 0; j < i-1; j++){
        tem_median_list[j] = tem_median_list[j+1];
      }
      tem_median_list[i-1] = dist;
      break;
    }
    else {
      for (int j = 0; j <_DIST_MEDIAN; j++){
        tem_median_list[j] = tem_median_list[j+1];
      }
      tem_median_list[_DIST_MEDIAN-1] = dist;
      break;
    }
  }
    
  if (_DIST_MEDIAN%2 == 0){
    median = (tem_median_list[_DIST_MEDIAN/2-1]+tem_median_list[_DIST_MEDIAN/2])/2;
  }
  else {
    median = tem_median_list[_DIST_MEDIAN/2];
  }
  return median;
  }

float emafilter(float dist){ //ema필터 보정
  dist_ema = ((alpha*dist)+((1-alpha)*dist_ema));
  return dist_ema;
}

float califiltersample(float dist){ //실제 측정을 통해 샘플값을 얻기 위한 함수
  sum += dist;
  count += 1;
  if (count == 300) {
    Serial.print("sum:");
    Serial.print(sum / 300.0);
    sum = 0;
    count = 0;
  }
//10cm->67, 15cm->108, 20->155, 25->184, 30->202, 35->221, 40->237, 최대: 246
}

float califilter(float dist){ //cali필터 보정, 실제 측정을 통해 얻은 샘플값을 이용해 값 보정
  int samples[] = {67, 108, 155, 184, 202, 221, 237, 246};
  for (int i = 0; i < 7; i++){
    if (samples[i] <= dist && dist < samples[i+1]){
      float dist_cali = 10*(10+5*i + 5.0 / (samples[i+1] - samples[i]) * (dist - samples[i]));
      return dist_cali;
    }
  }
  float dist_cali = 10*(10 + 31.0 / (samples[7] - samples[0]) * (dist - samples[0]));
  return dist_cali;
}

float p_move(float dist){//p제어
  float currenterror = 255 - dist;
  if (currenterror >= 0){//25.5cm미만
   float servo_needmove = _duty_meu + kP*(((_duty_min - _duty_meu) / 1.0) * (currenterror / 155.0));  
   return servo_needmove;
  }
  else {//25.5cm 이상
    float servo_needmove = _duty_meu - kP2*((_duty_meu - _duty_max / 1.0 ) * (abs(currenterror) / 155.0));
    return servo_needmove;
  }
}

  
void loop() {
  if (millis() < last_sampling_time + INTERVAL) return;
  float raw_dist = ir_distance(); //센서에서 값을 받아옴
  //float dist_midfix = medianfilter(raw_dist); //중위수필터 보정(미완성)
  float dist_emafix = emafilter(raw_dist); //ema필터 보정
  //califiltersample(dist_emafix); //cali값 샘플추출(완료했으므로 주석)
  float dist_cali = califilter(dist_emafix); //cali필터 보정

  myservo.writeMicroseconds(p_move(dist_cali)); //p제어
  
  //Serial.print("min:0,max:500,dist:");
  //Serial.print(raw_dist);
  //Serial.print(",dist_midfix:");
  //Serial.print(dist_midfix);
  Serial.print(",dist_emafix:");
  Serial.print(dist_emafix);
  Serial.print(",dist_cali:");
  Serial.println(dist_cali);
  Serial.println(myservo.read());
  delay(20);
}
