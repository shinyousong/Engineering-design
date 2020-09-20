#define PIN_LED 13
unsigned int count, toggle;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
  Serial.println("Hello world!");
  count = toggle = 0;
  digitalWrite(PIN_LED, toggle); //LED OFF.
}

void loop() {
  Serial.println(++count);
  
  toggle = toggle_state(toggle+1); //toogle LED value=1.  
  digitalWrite(PIN_LED, toggle); //update LED.
  delay(1000);
  
  toggle = toggle_state(toggle-1); //update LED value=0.
  digitalWrite(PIN_LED, toggle); //update LED.
  delay(1000);
}
int toggle_state(int toggle) {
  return toggle;
}
