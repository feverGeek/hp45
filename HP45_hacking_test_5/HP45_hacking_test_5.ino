//the test code for the HP45 hacking tests. Connected to an Arduino Mega 2560. All primitives and addresses go to TLC59213 drivers. All address outputs are pulled to ground with a 10k resistor on the TLC output side.

//pin names
const byte Led = 13;
const byte Switch = 10;

//variables
word nozzle = 0;
const byte LineBufferSize = 200;
byte LineBuffer[LineBufferSize][22]; //holds the information for a sweep, only holds the even side (0-12) of the printhead, even odd (1-13) is not used for now
byte LineBufferRepeat[LineBufferSize]; //holds the information on how often a sweep needs to be repeated
byte LineBufferFilled = 0;


//nozzle table
const byte NozzlePrim[300] = {
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

const byte NozzleAdd[300] = {
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

const byte CharBuffer[40][5] = {
  {B00111110, B01000101, B01001001, B01010001, B00111110}, //0
  {B00000000, B00100001, B01111111, B00000001, B00000000}, //1
  {B00100001, B01000011, B01000101, B01001001, B00110001}, //2
  {B01000010, B01000001, B01010001, B01101001, B01000110}, //3
  {B00001100, B00010100, B00100100, B01111111, B00000100}, //4
  {B01110010, B01010001, B01010001, B01010001, B01001110}, //5
  {B00011110, B00101001, B01001001, B01001001, B00000110}, //6
  {B01000000, B01000111, B01001000, B01010000, B01100000}, //7
  {B00110110, B01001001, B01001001, B01001001, B00110110}, //8
  {B00110000, B01001001, B01001001, B01001010, B00111100}, //9
  {B00000000, B00000000, B00000000, B00000000, B00000000}, //SPACE
  {B00111111, B01000100, B01000100, B01000100, B00111111}, //A
  {B01111111, B01001001, B01001001, B01001001, B00110110}, //B
  {B00111110, B01000001, B01000001, B01000001, B00100010}, //C
  {B01111111, B01000001, B01000001, B00100010, B00011100}, //D
  {B01111111, B01001001, B01001001, B01001001, B01000001}, //E
  {B01111111, B01001000, B01001000, B01001000, B01000000}, //F
  {B00111110, B01000001, B01001001, B01001001, B00101111}, //G
  {B01111111, B00001000, B00001000, B00001000, B01111111}, //H
  {B00000000, B01000001, B01111111, B01000001, B00000000}, //I
  {B00000010, B00000001, B01000001, B01111110, B01000000}, //J
  {B01111111, B00001000, B00010100, B00100010, B01000001}, //K
  {B01111111, B00000001, B00000001, B00000001, B00000001}, //L
  {B01111111, B00100000, B00011000, B00100000, B01111111}, //M
  {B01111111, B00010000, B00001000, B00000100, B01111111}, //N
  {B00111110, B01000001, B01000001, B01000001, B00111110}, //O
  {B01111111, B01001000, B01001000, B01001000, B00110000}, //P
  {B00111110, B01000001, B01000101, B01000010, B00111101}, //Q
  {B01111111, B01001000, B01001100, B01001010, B00110001}, //R
  {B00110001, B01001001, B01001001, B01001001, B01000110}, //S
  {B01000000, B01000000, B01111111, B01000000, B01000000}, //T
  {B01111110, B00000001, B00000001, B00000001, B01111110}, //U
  {B01111100, B00000010, B00000001, B00000010, B01111100}, //V
  {B01111110, B00000001, B00001110, B00000001, B01111110}, //W
  {B01100011, B00010100, B00001000, B00010100, B01100011}, //X
  {B01110000, B00001000, B00000111, B00001000, B01110000}, //Y
  {B01000011, B01000101, B01001001, B01010001, B01100001}, //Z
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
  pinMode(Switch, INPUT);

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
  if (digitalRead(Switch)) {//eject a zigzag pattern
    digitalWrite(Led, 1);

    delay(2000);

    for (int pr = 0; pr < 37; pr ++) {

      GenerateChar(pr, 10);
      PrintLinebuffer(1000);
      ClearLinebuffer();
      delay(100);
    }

    /*
    FillLinebufferTest();
    PrintLinebuffer(1000);
    delay(1000);*/



    //make the led low to show ejection is over
    digitalWrite(Led, 0);
  }
}





void ClearLinebuffer() {
  for (int lb = 0; lb < LineBufferSize; lb++) {
    for (int a = 0; a < 22; a++) {
      LineBuffer[lb][a] = 0;
    }
    LineBufferRepeat[lb] = 0;
  }
  LineBufferFilled = 0;
}


void GenerateSquare(byte SquareSize, byte SquareThickness) {
  LineBufferFilled = SquareSize;
  for (byte sz = 0; sz < SquareSize; sz++) {
    //generate start
    if (sz < SquareThickness) {
      for (int ss = 0; ss < SquareSize; ss++) { //all nozzles in the square need to be on
        word ActualNozzle = ss * 2; //needs to be multiplied to get even side
        word PrimAddress = NozzlePrim[ActualNozzle] - 1;
        PrimAddress /= 2;
        bitWrite(LineBuffer[sz][NozzleAdd[ActualNozzle] - 1], PrimAddress, 1);
        LineBufferRepeat[sz] = 1;
      }
    }

    //generate end
    else if (sz > SquareSize - SquareThickness) {
      for (int ss = 0; ss < SquareSize; ss++) { //all nozzles in the square need to be on
        word ActualNozzle = ss * 2; //needs to be multiplied to get even side
        word PrimAddress = NozzlePrim[ActualNozzle] - 1;
        PrimAddress /= 2;
        bitWrite(LineBuffer[sz][NozzleAdd[ActualNozzle] - 1], PrimAddress, 1);
        LineBufferRepeat[sz] = 1;
      }
    }

    //generate middle
    else {
      for (int ss = SquareSize; ss > SquareSize - SquareThickness; ss--) { //all nozzles on the edges of square need to be on
        word ActualNozzle = ss * 2; //needs to be multiplied to get even side
        word PrimAddress = NozzlePrim[ActualNozzle] - 1;
        PrimAddress /= 2;
        bitWrite(LineBuffer[sz][NozzleAdd[ActualNozzle] - 1], PrimAddress, 1);
        LineBufferRepeat[sz] = 1;
      }
      for (int ss = 0; ss < SquareThickness; ss++) { //all nozzles on the edges of square need to be on
        word ActualNozzle = ss * 2; //needs to be multiplied to get even side
        word PrimAddress = NozzlePrim[ActualNozzle] - 1;
        PrimAddress /= 2;
        bitWrite(LineBuffer[sz][NozzleAdd[ActualNozzle] - 1], PrimAddress, 1);
        LineBufferRepeat[sz] = 1;
      }
    }
  }
}


void GenerateChar(byte CharNumber, byte CharMultiplier) {
  byte NumberOfCols = 0;
  for (byte col = 0; col < 5; col++) { //run through all columns of the specified character
    for (byte pixelwidth = 0; pixelwidth < CharMultiplier; pixelwidth ++) {
      for (byte pixel = 0; pixel < 8; pixel ++) { //run through all inidividual pixels
        if (bitRead(CharBuffer[CharNumber][col], pixel) == 1) {
          for (byte pix = 0; pix < CharMultiplier; pix ++) {
            word ActualNozzle = pixel * CharMultiplier;
            ActualNozzle += pix;
            ActualNozzle *= 2;
            word PrimAddress = NozzlePrim[ActualNozzle] - 1;
            PrimAddress /= 2;
            bitWrite(LineBuffer[NumberOfCols][NozzleAdd[ActualNozzle] - 1], PrimAddress, 1);
            LineBufferRepeat[NumberOfCols] = 1;
          }
        }
      }
      NumberOfCols ++;
    }
  }
  LineBufferFilled = NumberOfCols;
}

void PrintLinebuffer(word LinebufferSpeed) { //prints what is stored in the linebuffer
  for (int lb = 0; lb < LineBufferFilled; lb++) { //start at the first place of the linebuffer, and work up
    for (byte r = 0; r < LineBufferRepeat[lb]; r++) { //repeat the information in the linebuffer n times
      for (byte a = 0; a < 22; a++) {
        //rewrite primitive ports
        byte TriggerPortP1 = 0;
        byte TriggerPortP2 = 0;
        byte TempPrim = LineBuffer[lb][a];
        for (int p = 0; p < 7; p++)
        {
          byte TempPrim2 = p * 2;
          if (bitRead(TempPrim, p) == 1) {
            if (TempPrim2 < 8) {
              bitWrite(TriggerPortP1, TempPrim2, 1);
            }
            else {
              bitWrite(TriggerPortP2, TempPrim2 - 8, 1);
            }
          }
        }

        //rewrite Addresses to ports
        byte TriggerPortP3 = 0;
        byte TriggerPortP4 = 0;
        byte TriggerPortP5 = 0;
        if (a >= 0 && a < 8) {
          bitWrite(TriggerPortP3, a, 1);
        }
        else if (a >= 8 && a < 16) {
          bitWrite(TriggerPortP4, a - 8, 1);
        }
        else {
          bitWrite(TriggerPortP5, a - 16, 1);
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


      }
      delayMicroseconds(LinebufferSpeed);
    }
  }
}

void FillLinebufferTest() {
  for (int lb = 0; lb < 10; lb++) {
    for (int a = 0; a < 22; a++) {
      LineBuffer[lb][a] = B01111111;
    }
    LineBufferRepeat[lb] = 10;
  }
  LineBufferFilled = 10;
}

void DepositZigzag() { //deposits an up and down zigzag pattern once
  for (int i = 0; i < 150; i ++) {
    TriggerSingleNozzle(i * 2);
    delayMicroseconds(extradelay);
  }

  for (int i = 149; i >= 0; i --) {
    TriggerSingleNozzle(i * 2);
    delayMicroseconds(extradelay);
  }
}

void TriggerSingleNozzle(int TriggerNozzle) { //triggers a single defined nozzle
  TriggerNozzle = constrain(TriggerNozzle, 0, 299);
  byte TriggerPrimitive = NozzlePrim[TriggerNozzle] - 1; //calculate what primitive needs to be on
  byte TriggerAddress = NozzleAdd[TriggerNozzle] - 1;//calculate what address needs to be on

  //rewrite Primitives to ports
  byte TriggerPortP1 = 0;
  byte TriggerPortP2 = 0;
  if (TriggerPrimitive >= 0 && TriggerPrimitive < 8) {
    bitWrite(TriggerPortP1, TriggerPrimitive, 1);
  }
  else if (TriggerPrimitive >= 8 && TriggerPrimitive < 16) {
    bitWrite(TriggerPortP2, TriggerPrimitive - 8, 1);
  }


  //rewrite Addresses to ports
  byte TriggerPortP3 = 0;
  byte TriggerPortP4 = 0;
  byte TriggerPortP5 = 0;
  if (TriggerAddress >= 0 && TriggerAddress < 8) {
    bitWrite(TriggerPortP3, TriggerAddress, 1);
  }
  else if (TriggerAddress >= 8 && TriggerAddress < 16) {
    bitWrite(TriggerPortP4, TriggerAddress - 8, 1);
  }
  else {
    bitWrite(TriggerPortP5, TriggerAddress - 16, 1);
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


