const byte AddressClock = 7; //D7
const byte AddressClear = 6; //D6
const byte Address1 = 5; //D5
const byte led = 13;

void setup() {
  // put your setup code here, to run once:

  DDRD = DDRD | B11100000;
  pinMode(led, OUTPUT);

  digitalWrite(Address1, 0);
}

void loop() {
  digitalWrite(led, 1);
  delay(1000);
  digitalWrite(led, 0);

  //reset
  PORTD = PORTD | B01000000;
  PORTD = PORTD & B10111111;
  delay(1000);

  //set address pin
  PORTD = PORTD & B01111111;
  PORTD = PORTD | B01100000;
  delay(1000);

  digitalWrite(led, 1);
  delay(200);
  digitalWrite(led, 0);
  //trigger clock
  PORTD = PORTD | B10000000;
  PORTD = PORTD & B01111111;
  delay(1000);

  //reset address
  PORTD = PORTD & B11011111;
  delay(1000);
}
