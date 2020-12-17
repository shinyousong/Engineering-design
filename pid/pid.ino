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
#define _DIST_ALPHA 0.3 //EMA필터
#define KP 0.25 //P제어 비례이득_1
#define KP2 0.08 //P제어 비례이득_2
#define KD 28; // D제어 비례이득
#define KI 1; // I제어 비례이득

int _dist_target = _DIST_TARGET;
int _dist_min = _DIST_MIN;
int _dist_max = _DIST_MAX;

int _duty_meu = _DUTY_MEU;
int _duty_min = _DUTY_MIN;
int _duty_max = _DUTY_MAX;

unsigned long last_sampling_time; // unit: ms

int count = 0; //cali필터 측정을 위한 변수
float sum = 0; //cali필터 측정을 위한 변수

int sample = 25; //측정값 샘플링 갯수

float dist_ema; //ema필터를 위한 변수
float alpha = _DIST_ALPHA; //ema 필터를 위한 변수

float kP = KP; //P제어 비례이득 1
float kP2 = KP2; //P제어 비례이득 2
float pterm; //P제어값

float kD = KD; //D제어 비례이득
float dterm;//D제어값
float error_prev = 0;

float kI = KI; //I제어 비례이득
float iterm; //I제어값
float _iterm_max = (_duty_min - _duty_max)/10; //적분항 최소-최댓값

Servo myservo;


void setup() {
// initialize GPIO pins
  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(_duty_meu);

// initialize serial port
  Serial.begin(57600);
  last_sampling_time = 0;
}

float ir_distance(void){ // return value unit: mm
  float val;
  float volt = float(analogRead(PIN_IR));
  val = ((6762.0/(volt-9.0))-4.0) * 10.0;
  return val;
}

float califiltersample(float dist){ //실제 측정을 통해 샘플값을 얻기 위한 함수, 측정을 위한 것이므로 측정 완료 시 사용하지 않음. 이후 califilter의 sample값으로 활용
  sum += dist;
  count += 1;
  if (count == 300) {
    Serial.print("sum:");
    Serial.print(sum / 300.0);
    sum = 0;
    count = 0;
  }

}

float califilter(float dist){ //cali필터 보정, 실제 측정을 통해 얻은 샘플값을 이용해 값 보정
  int samples[] = {69, 110, 162, 194, 224, 263, 306, 323};
  for (int i = 0; i < 7; i++){
    if (samples[i] <= dist && dist < samples[i+1]){
      float dist_cali = 10*(10+5*i + 5.0 / (samples[i+1] - samples[i]) * (dist - samples[i]));
      return dist_cali;
    }
  }
  if (dist < 69){
    float dist_cali = _dist_min;
    return dist_cali;
    }
  if (dist > 323){
    float dist_cali = _dist_max;
    return dist_cali;
    }
}

float orderfilter(){ //순서필터, 샘플들의 중간값의 평균을 얻어옴
  float sampleArray[sample]; //샘플 저장 array
  for (int i = 0; i < sample; i++){
    float raw_dist = ir_distance(); //센서에서 값을 받아옴
    float dist_cali = califilter(raw_dist); //cali필터 보정
    sampleArray[i] = dist_cali; // samplearray 저장
  }
  for (int i = 0; i < sample-1; i++){ //버블정렬
    for (int j = 0; j < sample-1; j++){
      if (sampleArray[j] > sampleArray[j+1]){
        float tem = sampleArray[j+1];
        sampleArray[j+1] = sampleArray[j];
        sampleArray[j] = tem;
      }
    }
  }
  float samplesum = 0;
  for (int i = int(sample/2)-1; i < int(sample/2)+2; i++){ //중간에 가까운 값들 더함
    samplesum += sampleArray[i];
  }
  return samplesum/3; //중간에 가까운 값들의 합의 평균을 리턴
}

float emafilter(float dist){ //ema필터 보정
  dist_ema = ((alpha*dist)+((1-alpha)*dist_ema));
  return dist_ema;
}

float p_move(float dist){//p제어
  float currenterror = _dist_target - dist;
  if (currenterror >= 0){//25.5cm이하
    pterm = kP*(((_duty_min - _duty_meu) / 1.0) * (currenterror / 155.0));  
    return pterm;
  }
  else {//25.5cm 초과
    pterm = -1*(kP2*((_duty_meu - _duty_max / 1.0 ) * (abs(currenterror) / 155.0)));
    return pterm;
    }
}

float d_move(float dist){//d제어
  float error_curr = _dist_target - dist;
  dterm = kD * (error_curr - error_prev);
  error_prev = error_curr;
  return dterm;
}

float i_move(float dist){//i제어
  iterm += kI*(_dist_target - dist);
  if(iterm > _iterm_max){
    iterm = _iterm_max;
  }
  if(iterm < -1*_iterm_max){
    iterm = -1*_iterm_max;
  }
  return iterm;
}


void loop() {
  if (millis() < last_sampling_time + INTERVAL) return;
  
  float dist_cali = orderfilter(); // 값 측정 sample회, cali필터 보정, 중간값 보정
  float dist_emafix = emafilter(dist_cali); //ema필터 보정
  
  float duty_curr = _duty_meu + p_move(dist_emafix) + d_move(dist_emafix) + i_move(dist_emafix); //PID제어
  if (duty_curr > _duty_min){
    duty_curr = _duty_min;
  }
  if (duty_curr < _duty_max){
    duty_curr = _duty_max;
  }
  myservo.writeMicroseconds(duty_curr);
  
  Serial.print("IR:");
  Serial.print(dist_emafix);
  Serial.print(",T:");
  Serial.print(_dist_target);
  Serial.print(",P:");
  Serial.print(pterm);
  //Serial.print(double(map((pterm),-1000,1000,510,610)));
  Serial.print(",D:");
  Serial.print(dterm);
  //Serial.print(double(map((dterm),-1000,1000,510,610)));
  Serial.print(",I:");
  Serial.print(iterm);
  //Serial.print(double(map((iterm),-1000,1000,510,610)));
  Serial.print(",DTT:");
  Serial.print(map(_duty_meu,1000,2000,410,510));
  Serial.print(",DTC:");
  Serial.print(map(duty_curr,1000,2000,410,510));
  Serial.println(",-G:245,+G:265,m:0,M;800");
}
