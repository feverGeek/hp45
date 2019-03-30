//the test code for the HP45 hacking tests. Connected to an Arduino Mega 2560. All primitives and addresses go to TLC59213 drivers. All address outputs are pulled to ground with a 10k resistor on the TLC output side.

//pin names
const byte Led = 13;

//variables
word nozzle = 0;

//nozzle table
int NozzlePrim[300] = {
  1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
  1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
  1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
  1, 2, 1, 2, 1, 2, 1, 2, 1, 2,
  3, 4, 3, 4, 3, 4, 3, 4, 3, 4,
  3, 4, 3, 4, 3, 4, 3, 4, 3, 4,
  3, 4, 3, 4, 3, 4, 3, 4, 3, 4,
  3, 4, 3, 4, 3, 4, 3, 4, 3, 4,
  3, 4, 3, 4, 5, 6, 5, 6, 5, 6,
  5, 6, 5, 6, 5, 6, 5, 6, 5, 6,
  5, 6, 5, 6, 5, 6, 5, 6, 5, 6,
  5, 6, 5, 6, 5, 6, 5, 6, 5, 6,
  5, 6, 5, 6, 5, 6, 5, 6, 7, 8,
  7, 8, 7, 8, 7, 8, 7, 8, 7, 8,
  7, 8, 7, 8, 7, 8, 7, 8, 7, 8,
  7, 8, 7, 8, 7, 8, 7, 8, 7, 8,
  7, 8, 7, 8, 7, 8, 7, 8, 7, 8,
  7, 8, 9, 10, 9, 10, 9, 10, 9, 10,
  9, 10, 9, 10, 9, 10, 9, 10, 9, 10,
  9, 10, 9, 10, 9, 10, 9, 10, 9, 10,
  9, 10, 9, 10, 9, 10, 9, 10, 9, 10,
  9, 10, 9, 10, 9, 10, 11, 12, 11, 12,
  11, 12, 11, 12, 11, 12, 11, 12, 11, 12,
  11, 12, 11, 12, 11, 12, 11, 12, 11, 12,
  11, 12, 11, 12, 11, 12, 11, 12, 11, 12,
  11, 12, 11, 12, 11, 12, 11, 12, 11, 12,
  13, 14, 13, 14, 13, 14, 13, 14, 13, 14,
  13, 14, 13, 14, 13, 14, 13, 14, 13, 14,
  13, 14, 13, 14, 13, 14, 13, 14, 13, 14,
  13, 14, 13, 14, 13, 14, 13, 14, 13, 14
};

int NozzleAdd[300] = {
  1, 9, 16, 2, 9, 17, 2, 10, 17, 3,
  10, 18, 3, 11, 18, 4, 11, 19, 4, 12,
  19, 5, 12, 20, 5, 13, 20, 6, 13, 21,
  6, 14, 21, 7, 14, 22, 17, 15, 22, 8,
  15, 1, 8, 13, 1, 9, 16, 2, 9, 17,
  2, 10, 17, 3, 10, 18, 3, 11, 18, 4,
  11, 19, 4, 12, 19, 5, 12, 20, 5, 13,
  20, 6, 13, 21, 6, 14, 21, 7, 14, 22,
  7, 15, 22, 8, 15, 1, 8, 16, 1, 9,
  16, 2, 9, 17, 2, 10, 17, 3, 10, 18,
  3, 11, 18, 4, 11, 19, 4, 12, 19, 5,
  12, 20, 5, 13, 20, 6, 13, 21, 6, 14,
  21, 7, 14, 22, 7, 15, 22, 8, 15, 1,
  8, 16, 1, 9, 16, 2, 9, 17, 2, 10,
  17, 3, 10, 18, 3, 11, 18, 4, 11, 19,
  4, 12, 19, 5, 12, 20, 5, 13, 20, 6,
  13, 21, 6, 14, 21, 7, 14, 22, 7, 15,
  1, 8, 16, 1, 8, 16, 1, 9, 16, 2,
  9, 17, 2, 10, 19, 3, 10, 18, 3, 11,
  18, 4, 11, 19, 4, 12, 19, 5, 12, 20,
  5, 13, 20, 6, 13, 21, 6, 14, 21, 7,
  14, 22, 7, 15, 22, 8, 15, 1, 8, 16,
  1, 9, 16, 2, 9, 17, 2, 10, 17, 3,
  10, 18, 3, 11, 18, 4, 11, 19, 4, 12,
  19, 5, 12, 22, 5, 13, 20, 6, 13, 21,
  6, 14, 21, 7, 14, 22, 7, 15, 22, 8,
  15, 1, 8, 16, 1, 9, 16, 2, 9, 17,
  2, 10, 17, 3, 10, 18, 3, 11, 18, 4,
  11, 19, 4, 12, 19, 5, 12, 20, 5, 13,
  20, 6, 13, 21, 6, 14, 21, 7, 14, 21
};

/*
   pins on the Arduino Mega
   Address clock PH3 (6)
   Address _clear PH4 (7)
   Primitive clock PH5 (8)
   Primitive _clear PH6 (9)

   Primitive 1-8 PL0-PL7 (49;48;47;46;45;44;43;42)
   Primitive 9-14 PK0-PK5 (A8;A9;A10;A11;A12;A13)
   Address 1-8 PC0-PC7 (37;36;35;34;33;32;31;30)
   Address 9-16 PA0-PA7 (22;23;24;25;26;27;28;29)
   Address 17-22 PF0-PF5 (A0;A1;A2;A3;A4;A5)

   Temp sense
   10x resistor
   Nozzle check pin
*/

//global variables
int Loops = 1000; //the amount of loops done while triggering the printhead.
int extradelay = 500;

void setup() {

  //set pin directions
  pinMode(Led, OUTPUT);

  DDRH = DDRH | B01111000; //clears and clocks
  DDRL = DDRL | B11111111; //primitive 1-8
  DDRK = DDRK | B00111111; //primitive 9-14
  DDRC = DDRC | B11111111; //address 1-8
  DDRA = DDRA | B11111111; //address 9-16
  DDRF = DDRF | B00111111; //address 17-22

  //reset clears
  PORTH = PORTH | B01010000;
  PORTH = PORTH & B10101111;

  //set addresses and primitives to 0
  PORTL - PORTL & B00000000;
  PORTK - PORTK & B11000000;
  PORTC - PORTC & B00000000;
  PORTA - PORTA & B00000000;
  PORTF - PORTF & B11000000;
}

void loop() {

  //set the led high to show ejecting is in progress
  digitalWrite(Led, 1);

  for (int a = 0; a < 10; a++) {
    for (int i = 0; i < 150; i ++) {
      TriggerSingleNozzle(i * 2);
      delayMicroseconds(extradelay);
    }

    for (int i = 149; i >= 0; i --) {
      TriggerSingleNozzle(i * 2);
      delayMicroseconds(extradelay);
    }
  }


  /*delay(500);

  for (int a = 0; a < 22; a++){
    for (int p = 0; p < 14; p++){
      for (int b = 0; b < 5; b++){
      TriggerRaw(p, a);
      }
    }
  }*/

  //make the led low to show ejection is over
  digitalWrite(Led, 0);

  //wait for a second
  delay (1000);
}






void TriggerSingleNozzle(int TriggerNozzle) {
  TriggerNozzle = constrain(TriggerNozzle, 0, 299);
  byte TriggerPrimitive = NozzlePrim[TriggerNozzle]; //calculate what primitive needs to be on
  byte TriggerAddress = NozzleAdd[TriggerNozzle];//calculate what address needs to be on

  //rewrite Primitives to ports
  byte TriggerPortP1 = 0;
  byte TriggerPortP2 = 0;
  if (TriggerPrimitive == 1) TriggerPortP1 = 1;
  if (TriggerPrimitive == 2) TriggerPortP1 = 2;
  if (TriggerPrimitive == 3) TriggerPortP1 = 4;
  if (TriggerPrimitive == 4) TriggerPortP1 = 8;
  if (TriggerPrimitive == 5) TriggerPortP1 = 16;
  if (TriggerPrimitive == 6) TriggerPortP1 = 32;
  if (TriggerPrimitive == 7) TriggerPortP1 = 64;
  if (TriggerPrimitive == 8) TriggerPortP1 = 128;
  if (TriggerPrimitive == 9) TriggerPortP2 = 1;
  if (TriggerPrimitive == 10) TriggerPortP2 = 2;
  if (TriggerPrimitive == 11) TriggerPortP2 = 4;
  if (TriggerPrimitive == 12) TriggerPortP2 = 8;
  if (TriggerPrimitive == 13) TriggerPortP2 = 16;
  if (TriggerPrimitive == 14) TriggerPortP2 = 32;


  //rewrite Addresses to ports
  byte TriggerPortP3 = 0;
  byte TriggerPortP4 = 0;
  byte TriggerPortP5 = 0;
  if (TriggerAddress == 1) TriggerPortP3 = 1;
  if (TriggerAddress == 2) TriggerPortP3 = 2;
  if (TriggerAddress == 3) TriggerPortP3 = 4;
  if (TriggerAddress == 4) TriggerPortP3 = 8;
  if (TriggerAddress == 5) TriggerPortP3 = 16;
  if (TriggerAddress == 6) TriggerPortP3 = 32;
  if (TriggerAddress == 7) TriggerPortP3 = 64;
  if (TriggerAddress == 8) TriggerPortP3 = 128;
  if (TriggerAddress == 9) TriggerPortP4 = 1;
  if (TriggerAddress == 10) TriggerPortP4 = 2;
  if (TriggerAddress == 11) TriggerPortP4 = 4;
  if (TriggerAddress == 12) TriggerPortP4 = 8;
  if (TriggerAddress == 13) TriggerPortP4 = 16;
  if (TriggerAddress == 14) TriggerPortP4 = 32;
  if (TriggerAddress == 15) TriggerPortP4 = 64;
  if (TriggerAddress == 16) TriggerPortP4 = 128;
  if (TriggerAddress == 17) TriggerPortP5 = 1;
  if (TriggerAddress == 18) TriggerPortP5 = 2;
  if (TriggerAddress == 19) TriggerPortP5 = 4;
  if (TriggerAddress == 20) TriggerPortP5 = 8;
  if (TriggerAddress == 21) TriggerPortP5 = 16;
  if (TriggerAddress == 22) TriggerPortP5 = 32;


  //open address
  PORTH = PORTH & B11110111; //set clock to low
  PORTH = PORTH | B00010000; //set clear to high
  PORTC = PORTC | TriggerPortP3; //set address pin high
  PORTA = PORTA | TriggerPortP4; //set address pin high
  PORTF = PORTF | TriggerPortP5; //set address pin high
  PORTH = PORTH | B00001000; //set clock to high
  PORTH = PORTH & B11110111; //set clock low
  PORTC = PORTC & B00000000; //set address pin low
  PORTA = PORTA & B00000000; //set address pin low
  PORTF = PORTF & B11000000; //set address pin low

  //address clear remains high until it needs to be reset
  delayMicroseconds(2); //opening delay

  //trigger primtive
  PORTH = PORTH & B11011111; //turn clock off
  PORTH = PORTH | B01000000; //turn clear high
  PORTL = TriggerPortP1; //set primitive1 pins high
  PORTK = TriggerPortP2; //set primitive2 pins high
  PORTH = PORTH | B00100000; //trigger clock
  PORTH = PORTH & B11011111; //set clock to low
  PORTL = PORTL & B00000000; //set primitive pins low
  PORTK = PORTK & B11000000; //set primitive pins low
  //primitive clear remains high until the primitives need to be reset

  //trigger delay
  delayMicroseconds(2);

  //close primitive
  PORTH = PORTH | B01000000; //set clear to high
  PORTH = PORTH & B10111111;//set clear to low
  delayMicroseconds(3); //delay to allow the ports to close properly

  //close address
  PORTH = PORTH | B00010000; //set address clear to high
  PORTH = PORTH & B11101111; //set address clear to low
  delayMicroseconds(2);

  delayMicroseconds(200);//end delay
}









void TriggerRaw(int Prim, int Add) {
  byte TriggerPrimitive = Prim;
  byte TriggerAddress = Add;

  //rewrite Primitives to ports
  byte TriggerPortP1 = 0;
  byte TriggerPortP2 = 0;
  if (TriggerPrimitive >= 0 && TriggerPrimitive < 8) {
    bitWrite(TriggerPortP1, TriggerPrimitive, 1);
  }
  else {
    bitWrite(TriggerPortP2, TriggerPrimitive - 8, 1);
  }

  //rewrite Addresses to ports
  byte TriggerPortP3 = 0;
  byte TriggerPortP4 = 0;
  byte TriggerPortP5 = 0;
  if (TriggerAddress >= 0 && TriggerAddress < 8) {
    bitWrite(TriggerPortP3, TriggerPrimitive, 1);
  }
  else if (TriggerAddress >= 8 && TriggerAddress < 16) {
    bitWrite(TriggerPortP4, TriggerPrimitive - 8, 1);
  }
  else {
    bitWrite(TriggerPortP5, TriggerPrimitive - 16, 1);
  }

  //open address
  PORTH = PORTH & B11110111; //set clock to low
  PORTH = PORTH | B00010000; //set clear to high
  PORTC = PORTC | TriggerPortP3; //set address pin high
  PORTA = PORTA | TriggerPortP4; //set address pin high
  PORTF = PORTF | TriggerPortP5; //set address pin high
  PORTH = PORTH | B00001000; //set clock to high
  PORTH = PORTH & B11110111; //set clock low
  PORTC = PORTC & B00000000; //set address pin low
  PORTA = PORTA & B00000000; //set address pin low
  PORTF = PORTF & B11000000; //set address pin low

  //address clear remains high until it needs to be reset
  delayMicroseconds(2); //opening delay

  //trigger primtive
  PORTH = PORTH & B11011111; //turn clock off
  PORTH = PORTH | B01000000; //turn clear high
  PORTL = TriggerPortP1; //set primitive1 pins high
  PORTK = TriggerPortP2; //set primitive2 pins high
  PORTH = PORTH | B00100000; //trigger clock
  PORTH = PORTH & B11011111; //set clock to low
  PORTL = PORTL & B00000000; //set primitive pins low
  PORTK = PORTK & B11000000; //set primitive pins low
  //primitive clear remains high until the primitives need to be reset

  //trigger delay
  delayMicroseconds(2);

  //close primitive
  PORTH = PORTH | B01000000; //set clear to high
  PORTH = PORTH & B10111111;//set clear to low
  delayMicroseconds(3); //delay to allow the ports to close properly

  //close address
  PORTH = PORTH | B00010000; //set address clear to high
  PORTH = PORTH & B11101111; //set address clear to low
  delayMicroseconds(2);

  delayMicroseconds(200);//end delay
}

