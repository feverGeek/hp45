//the test code for the HP45 hacking tests

//pin names
const byte AddressClock = 7; //C0 //is connected to both _clear and clock
const byte AddressClear = 6; //D6
const byte Addres1 = 5; //C1
const byte PrimitiveClock = 4; //C2 //is connected to both _clear and clock
const byte Primitive1 = 3; //C3
const byte PrimitiveClear = 2; //D6
const byte Led = 13;

//global variables
int Loops = 5000; //the amount of loops done while triggering the printhead.

void setup() {

  //set pin directions
  pinMode(Led, OUTPUT);

  DDRD = DDRD | B11111100;

  PORTD = PORTD | B01001000;
  PORTD = PORTD & B10110111;
}

void loop() {

  //set the led high to show ejecting is in progress
  digitalWrite(Led, 1);

  for (int i = 0; i < Loops; i++) {
    //open the address:
    PORTD = PORTD & B01111111;  //turn 7 (address clock) low
    PORTD = PORTD | B01100000;  //turn 5 and 6 (address _clear & address pin1) high
    PORTD = PORTD | B10000000;  //turn 7 (address clock) high
    PORTD = PORTD & B01011111;  //turn 5 and 7 (address pin1 & address clock) low
    //address _clear remains low until the pins need to be reset
    delayMicroseconds(2);

    //open the primitive:
    PORTD = PORTD & B11101111;  //turn 4 (prim clock) low
    PORTD = PORTD | B00001100;  //turn 2 and 3 (prim _clear & prim pin1) high
    PORTD = PORTD | B00010000;  //turn 4 (prim clock) high
    PORTD = PORTD & B11101011;  //turn 2 amd 4 (prim pin1 & prim clock) low
    //primitive _clear remains low until the pins need to be reset
  
    //delay
    delayMicroseconds(2);
    /*for (int d = 0; d < 30; d++) {
      byte o = 0;
    }*/

    //close primitive
    PORTD = PORTD | B00001000; //turn 3 (prim _clear) high
    PORTD = PORTD & B11110111; //turn 3 (prim _clear) low 
    delayMicroseconds(3);

    //close address
    PORTD = PORTD | B01000000; //turn 6 (address _clear) high
    PORTD = PORTD & B10111111; //turn 6 (prim _clear) low 
    delayMicroseconds(140);
  }

  //make the led low to show ejection is over
  digitalWrite(Led, 0);

  //wait for a second
  delay (1000);
}
