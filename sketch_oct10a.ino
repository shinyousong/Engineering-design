// Arduino pin assignment
#define PIN_LED 9
#define PIN_TRIG 12
#define PIN_ECHO 13

// configurable parameters
#define SND_VEL 346.0 // sound velocity at 24 celsius degree (unit: m/s)
#define INTERVAL 25 // sampling interval (unit: ms)
#define _DIST_MIN 100 // minimum distance to be measured (unit: mm)
#define _DIST_MAX 300 // maximum distance to be measured (unit: mm)
#define _DIST_MEDIAN 30 // amount of samples for median

// global variables
float timeout; // unit: us
float dist_min, dist_max, dist_raw, median, tem; // unit: mm
int median_list[_DIST_MEDIAN], tem_median_list[_DIST_MEDIAN]; // make two list
unsigned long last_sampling_time; // unit: ms
float scale; // used for pulse duration to distance conversion

void setup() {
// initialize GPIO pins
  pinMode(PIN_LED,OUTPUT);
  pinMode(PIN_TRIG,OUTPUT);
  digitalWrite(PIN_TRIG, LOW);
  pinMode(PIN_ECHO,INPUT);

// initialize USS related variables
  dist_min = _DIST_MIN; 
  dist_max = _DIST_MAX;
  timeout = (INTERVAL / 2) * 1000.0; // precalculate pulseIn() timeout value. (unit: us)
  dist_raw = 0.0; // raw distance output from USS (unit: mm)
  median = 0.0;
  tem = 0.0;
  scale = 0.001 * 0.5 * SND_VEL;

  for (int i = 0; i < _DIST_MEDIAN; i++){
    median_list[i] = 0;
    tem_median_list[i] = 0;
  }
  

// initialize serial port
  Serial.begin(57600);

// initialize last sampling time
  last_sampling_time = 0;
}

void loop() {
// wait until next sampling time. 
// millis() returns the number of milliseconds since the program started. Will overflow after 50 days.
  if(millis() < last_sampling_time + INTERVAL) return;

// get a distance reading from the USS
  dist_raw = USS_measure(PIN_TRIG,PIN_ECHO);

  tem = median_list[_DIST_MEDIAN-1]; //update new sample to median_list
  for (int i = 0; i < _DIST_MEDIAN; i++){
   median_list[_DIST_MEDIAN-1-i] = median_list[_DIST_MEDIAN-2-i];
  }
  median_list[0] = dist_raw;

  for (int i = 0; i < _DIST_MEDIAN; i++) //del old sample from tem_median_list(sorted)
  {
    if (tem_median_list[i] == tem){
      for (int j = 0; j < i; j++){
        tem_median_list[i-j] = tem_median_list[i-1-j];
        }
        break;
      }
    }
  tem_median_list[0] = -1;

  for (int i = 0; i < _DIST_MEDIAN; i++){ //update new sample to tem_median_list(sorted)
    if (tem_median_list[i] >= dist_raw){
      for (int j = 0; j < i-1; j++){
        tem_median_list[j] = tem_median_list[j+1];
        }
      tem_median_list[i-1] = dist_raw;
      break;
    }
    else {
      for (int j = 0; j < _DIST_MEDIAN; j++){
        tem_median_list[j] = tem_median_list[j+1];        
      }
      tem_median_list[_DIST_MEDIAN-1] = dist_raw;
      break;
      }
    }
  
  if (_DIST_MEDIAN%2 == 0) {
    median = (tem_median_list[_DIST_MEDIAN/2-1]+tem_median_list[_DIST_MEDIAN/2])/2;
    }
  else {
    median = tem_median_list[_DIST_MEDIAN/2];
    }
 

// output the read value to the serial port
  Serial.print("Min:0,");
  Serial.print("raw:");
  Serial.print(dist_raw);
  Serial.print(",");
  Serial.print("median:");
  Serial.print(map(median,0,400,100,500));
  Serial.print(",");
  Serial.println("Max:500");

// turn on the LED if the distance is between dist_min and dist_max
  if(dist_raw < dist_min || dist_raw > dist_max) {
    analogWrite(PIN_LED, 255);
  }
  else {
    analogWrite(PIN_LED, 0);
  }

// update last sampling time
  last_sampling_time += INTERVAL;
}

// get a distance reading from USS. return value is in millimeter.
float USS_measure(int TRIG, int ECHO)
{
  float reading;
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  reading = pulseIn(ECHO, HIGH, timeout) * scale; // unit: mm
  if(reading < dist_min || reading > dist_max) reading = 0.0; // return 0 when out of range.
  return reading;
}
