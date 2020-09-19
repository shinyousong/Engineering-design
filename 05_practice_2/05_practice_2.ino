#define PIN_LED 7
void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  digitalWrite(PIN_LED, 0);
  delay(1000);

  int i=0;
  while(1) //infinite loop
  {
  digitalWrite(PIN_LED, 1);
  delay(100);
  digitalWrite(PIN_LED, 0);
  delay(100);
  i += 1;
  if (i==5){
  digitalWrite(PIN_LED, 1);  
  break;
  }
  }
}

void loop() {
}
