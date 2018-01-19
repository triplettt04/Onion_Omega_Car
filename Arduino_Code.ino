void setup() {
  // put your setup code here, to run once:
pinMode (A0, INPUT_PULLUP);
pinMode (13, OUTPUT);
Serial.begin (9600);
}

void loop() {
  // put your main code here, to run repeatedly:
int x = analogRead (A0);
Serial.println (x);
if (x < 70) {
  digitalWrite (13, HIGH);
}
else {
  digitalWrite (13, LOW);                   
}
delay (1);
}
