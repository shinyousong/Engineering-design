#include <Servo.h>

// Arduino pin assignment
#define PIN_SERVO 10
#define PIN_IR A0

Servo myservo;
int a, b; // unit: mm

void setup() {
// initialize GPIO pins
  myservo.attach(PIN_SERVO);
  myservo.writeMicroseconds(1476);

// initialize serial port
  Serial.begin(57600);

  a = 70;
  b = 250;
}

float ir_distance(void){ // return value unit: mm
  float val;
  float volt = float(analogRead(PIN_IR));
  val = ((6762.0/(volt-9.0))-4.0) * 10.0;
  return val;
}

float dist_cali_2(float dist_cali){//fix error based on error range
  int error[5] = {170, 255, 300, 335, 370};
  int fixerror[4] = {20, 50, 35, 35}; //min(error)
  float dist_cali2 = dist_cali;
  for (int i = 0; i < 4; i++){
    if (error[i] < dist_cali && dist_cali <error[i+1]) {
      dist_cali2 = dist_cali - fixerror[i];
    }
  }
  return dist_cali2;
}

void loop() {
  float raw_dist = ir_distance();
  float dist_cali = 100 + 300.0 / (b - a) * (raw_dist - a);
  float dist_cali2 = dist_cali_2(dist_cali);

  if (dist_cali2 < 255){
  myservo.write(1575);
  }
  else{
  myservo.write(1368);
    }
  Serial.print("min:0,max:500,dist:");
  Serial.print(raw_dist);
  Serial.print(",dist_cali:");
  Serial.println(dist_cali2);
  delay(20);
}
