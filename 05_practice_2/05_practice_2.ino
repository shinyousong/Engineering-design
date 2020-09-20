#define PIN_LED 7
int count=0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
}

void loop(){
  count += 1;
  
  if (count == 1)
  {
  digitalWrite(PIN_LED, 0);
  delay(1000);
  }
  
  digitalWrite(PIN_LED, 1);
  delay(100);
  digitalWrite(PIN_LED, 0);
  delay(100);

  if (count == 5)
  {
    digitalWrite(PIN_LED, 1);
    while (1){}
  }
}
