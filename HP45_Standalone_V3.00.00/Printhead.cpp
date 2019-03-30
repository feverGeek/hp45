/*
  Todo:
  Add DPI to B6raw, B6toggle and B8(Raw) formats
  Add B6 toggle format to decoding
*/

/*
   The printhead class holds all functions that make the printhead tick. It holds raw printhead pin manipulations,
   simple measurements, and complicated pin manipulation functions for testing and printing with the head.
   The printhead class if built somewhat protected, but is expected to be given proper input. Destroying any printhead 
   will take some effort, getting errors will not.

   Some notes:
   -Try not to run any interrupts if possible. The absolute critical parts have nointerrupts for microsecond periods 
   -Measuring the temperature will reset the address. Else the 10x will give odd values and the temperature will not measure.
*/



#include "Arduino.h"
#include "Printhead.h" //<*whispers: "there is actually nothing in there"

#define pulseSplits 3 //how many splits there are in a pulse
#define checkThreshold 10 //how many pulses each nozzle needs to take without signal to consider it broken.
#define maxPreheatPulses 20000 //the max number of pulses for a preheat per nozzle

class Printhead {
    //class member variables
    //pins
    uint8_t primitiveClock; //the clock pin that latches the primitive chanel
    uint8_t primitiveClear; //the clear pin for the primitive chanel (active low)
    uint8_t primtivePins[14]; //the array for the primitive pins

    uint8_t addressClock; //the pin that makes the address advance
    uint8_t addressReset; //the reset for the address
    uint8_t headEnable; //the pin that enables the ground of the printhead
    uint8_t nozzleCheck; //the pin that checks the condition of the selected nozzle
    uint8_t senseTSR; //the thermal sense resistor
    uint8_t sense10X; //the 10x calibration resistor

    uint16_t dpi = 600; //resolution of printhead
    uint16_t dpi_repeat = 1; //how often each to be printed pixel is repeated (calculated from DPI)

    //variables
    uint8_t headEnabled; //whether the printhead is enabled or not
    uint16_t pulseSplit[pulseSplits]; //splits the burst in
    uint16_t burstVar[22]; //a universaly usable variable for a burst

    //nozzle tables
    const uint8_t nozzleTableAddress[300] = {
      21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11,
      13, 15, 11, 20, 15, 3, 20, 0, 3, 9, 5, 21, 9, 1, 21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18,
      4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 3, 20, 0, 3, 9, 0, 21, 9, 5, 21, 16,
      5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15,
      11, 20, 15, 3, 20, 0, 3, 9, 0, 21, 9, 5, 21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7,
      18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 3, 20, 0, 3, 9, 0, 21, 9, 5, 21, 16, 5, 19,
      16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20,
      15, 3, 20, 0, 3, 9, 0, 21, 9, 5,  21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12,
      7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 3, 20, 0, 3, 9, 0, 21, 9, 5, 21, 16, 5, 19, 16, 10,
      19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 3
    };

    const uint8_t nozzleTablePrimitive[300] = {
      0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
      0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
      2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 4, 5, 4, 5, 4, 5,
      4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5,
      4, 5, 4, 5, 4, 5, 4, 5, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7,
      6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 8, 9, 8, 9, 8, 9, 8, 9,
      8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9,
      8, 9, 8, 9, 8, 9, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11,
      10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 10, 11, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13,
      12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13, 12, 13
    };
    int16_t nozzleTableReverse[16][22]; //the return table

  public:
    Printhead(uint8_t primclk, uint8_t primclr, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t p5, uint8_t p6, uint8_t p7, uint8_t p8, uint8_t p9, uint8_t p10, uint8_t p11, uint8_t p12, uint8_t p13, uint8_t addclk, uint8_t addrst, uint8_t hen, uint8_t ncheck, uint8_t stsr, uint8_t s10x) {
      //declare pins and in-/outputs
      primitiveClock = primclk;
      pinMode(primitiveClock, OUTPUT);
      primitiveClear = primclr;
      pinMode(primitiveClear, OUTPUT);

      primtivePins[0] = p0;
      primtivePins[1] = p1;
      primtivePins[2] = p2;
      primtivePins[3] = p3;
      primtivePins[4] = p4;
      primtivePins[5] = p5;
      primtivePins[6] = p6;
      primtivePins[7] = p7;
      primtivePins[8] = p8;
      primtivePins[9] = p9;
      primtivePins[10] = p10;
      primtivePins[11] = p11;
      primtivePins[12] = p12;
      primtivePins[13] = p13;
      for (uint8_t p = 0; p < 14; p++) {
        pinMode(primtivePins[p], OUTPUT);
      }


      addressClock = addclk;
      pinMode(addressClock, OUTPUT);
      addressReset = addrst;
      pinMode(addressReset, OUTPUT);
      headEnable = hen;
      pinMode(headEnable, OUTPUT);
      nozzleCheck = ncheck;
      pinMode(nozzleCheck, INPUT);
      senseTSR = stsr;
      pinMode(senseTSR, INPUT);
      sense10X = s10x;
      pinMode(sense10X, INPUT);

      //generate reverse nozzle table ----------------------------------------------------
      for (uint8_t a = 0; a < 22; a++) { //fill -1 in all positions
        for (uint8_t p = 0; p < 16; p++) {
          nozzleTableReverse[p][a] = -1; //set to nothing attached value (-1)
        }
      }
      //delay(2000); Serial.println("Generating reverse table: ");
      for (uint16_t n = 0; n < 300; n++) { //fill the correct nozzle in all filled positions
        nozzleTableReverse[nozzleTablePrimitive[n]][nozzleTableAddress[n]] = n;
        //Serial.print(n); Serial.print(", "); Serial.print(nozzleTablePrimitive[n]); Serial.print(", "); Serial.print(nozzleTableAddress[n]); Serial.println("");
      }

      //fill in split value
      pulseSplit[0] = 4681; //B0001001001001001
      pulseSplit[1] = 9362; //B0010010010010010
      pulseSplit[2] = 2340; //B0000100100100100

    }

    //complex functions
    int8_t Burst(uint16_t temp_input[22]) { //takes a 16 bit, 22 column array with the primitives that need to be on in each pulse
      uint16_t temp_pulse;
      if (headEnabled == 0) return 0; //check if burst is possible, return a 0 if not
      AddressReset(); //set address to 0
      for (uint8_t a = 0; a < 22; a++) {
        AddressNext(); //go to next address
        for (uint8_t p = 0; p < pulseSplits; p++) {
          temp_pulse = temp_input[a] & pulseSplit[p]; //split the pulse in smaller portions
          PrimitivePulse(temp_pulse); //pulse given fraction of the given address
        }
      }
      return 1; //return a 1 if successful
    }
    void TestHead(uint8_t* temp_nozzle_state) { //takes an empty uint8_t array of 300 as an input, and returns the state of each nozzle (0 for broken, 1 for working)
      int16_t test_pulse;
      uint8_t temp_address, temp_primitive;
      int8_t temp_tests;

      for (uint16_t n = 0; n < 300; n++) {
        temp_address = nozzleTableAddress[n];
        temp_primitive = nozzleTablePrimitive[n];

        //go to address
        AddressReset();
        AddressNext();
        for (uint8_t a = 0; a < temp_address; a++) { //step to the right address
          AddressNext();
        }
        test_pulse = 0;
        bitWrite(test_pulse, temp_primitive, 1); //set right primitive in pulse

        //discharge the capacitor
        SetEnable(1);
        delayMicroseconds(50);
        SetEnable(0);

        temp_tests = checkThreshold;

        //test nozzle
        while (1) {
          //do a pulse
          PrimitiveShortPulse(test_pulse);

          //check if test pin is low (test circuit pulls down on positive)
          if (GetNozzleCheck() == 0) {
            break;
          }
          temp_tests--; //subtract one from tests
          if (temp_tests <= 0) { //if the max number of tests is reached
            break;
          }
        }

        if (temp_tests == 0) { //if variable reached 0, nozzle never tested positive
          temp_nozzle_state[n] = 0;
        }
        else { //if anything other than 0, nozzle is positive
          temp_nozzle_state[n] = 1;
        }
      }//
    }
    /*void TestHead(uint8_t* temp_nozzle_state) { //takes an empty uint8_t array of 300 as an input, and returns the state of each nozzle (0 for broken, 1 for working)
      int16_t test_pulse;
      int16_t test_nozzle;

      //walk through all nozzles
      AddressReset();

      for (uint8_t a = 0; a < 22; a++) { //walk through addresses
        AddressNext();
        for (uint8_t p = 0; p < 14; p++) { //walk through primitives
          //discharge the capacitor
          SetEnable(1);
          delayMicroseconds(50);
          SetEnable(0);

          //set variables
          int8_t temp_tests = checkThreshold;
          test_pulse = 0;
          bitWrite(test_pulse, p, 1);
          test_nozzle = nozzleTableReverse[p][a]; //write which nozzle is tested
          if (test_nozzle == -1) { //if the nozzle does not exist for some dark and mysterious reason
            continue;
          }

          //test nozzle
          while (1) {
            //do a pulse
            PrimitiveShortPulse(test_pulse);

            //check if test pin is low (test circuit pulls down on positive)
            if (GetNozzleCheck() == 0) {
              break;
            }
            temp_tests--; //subtract one from tests
            if (temp_tests <= 0) { //if the max number of tests is reached
              break;
            }
          }

          if (temp_tests == 0) { //if variable reached 0, nozzle never tested positive
            temp_nozzle_state[test_nozzle] = 0;
          }
          else { //if anything other than 0, nozzle is positive
            temp_nozzle_state[test_nozzle] = 1;
          }
        } //end of primitive loop
      } //end of address loop
      } //end of function*/
    void SingleNozzle(uint16_t temp_nozzle) { //triggers a single indicated nozzle
      temp_nozzle = constrain(temp_nozzle, 0, 299);
      ResetBurst(); //reset the burst variable
      bitWrite(burstVar[nozzleTableAddress[temp_nozzle]], nozzleTablePrimitive[temp_nozzle], 1); //get the right nozzle to fire
      Burst(burstVar);
    }
    int8_t Preheat(uint16_t temp_pulses) { //does a given number of short pulses on the printhead to preheat the nozzles
      uint16_t temp_pulse = 16383;
      //Serial.print("Preheating: "); Serial.println(temp_pulses);
      if (headEnabled == 0) return 0; //check if burst is possible, return a 0 if not
      //Serial.print("Head enabled, preheating");
      //temp_pulses = constrain(temp_pulses, 0, maxPreheatPulses);
      for (uint16_t pulses = 0; pulses < temp_pulses; pulses++) {
        AddressReset(); //set address to 0
        for (uint8_t a = 0; a < 22; a++) {
        AddressNext(); //go to next address
          PrimitiveShortPulse(temp_pulse); //pulse the given address
        }
      }
      return 1; //return a 1 if successful
    }
    int8_t Prime(uint16_t temp_pulses) { //does a given number of long pulses on the printhead to start the nozzles
      uint16_t temp_pulse = 16383;
      //Serial.print("Preheating: "); Serial.println(temp_pulses);
      if (headEnabled == 0) return 0; //check if burst is possible, return a 0 if not
      //Serial.print("Head enabled, preheating");
      //temp_pulses = constrain(temp_pulses, 0, maxPreheatPulses);
      for (uint16_t pulses = 0; pulses < temp_pulses; pulses++) {
        AddressReset(); //set address to 0
        for (uint8_t a = 0; a < 22; a++) {
          AddressNext(); //go to next address
          for (uint8_t p = 0; p < pulseSplits; p++) {
            temp_pulse = 16383 & pulseSplit[p]; //split the pulse in smaller portions
            PrimitivePulse(temp_pulse); //pulse long given fraction of the given address
          }
        }
      }
      return 1; //return a 1 if successful
    }
    //void Calibrate() //does a series of preheat cycles to determine optimal ejection time (future reference)?????????????????????????????????????????????????

    //Simple pin triggers
    void PrimitivePulse(uint16_t tempState) { //do one pulse of the printhead with the specified input on the primitives (LSB to MSB, P0 to P13)
      SetPrimitiveClock(0); //set clock to 0
      SetPrimitiveClear(1); //set clear to 1
      SetPrimitivePins(tempState); //set primitive pins
      noInterrupts(); //allow no interrupts during pulse
      SetPrimitiveClock(1); //set clock to 1
      for (uint8_t d = 0; d < 12; d++) { //NOP delay (12 loops of 8 NOP's is ca 1.8us) (10=1.6, 14=2.1, 17=2.4)
        __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"); //8 NOP's
      }
      SetPrimitiveClear(0); //set clear to 0
      SetPrimitiveClock(0); //set clock to 0
      interrupts(); //allow interrupts again
    }
    void PrimitiveShortPulse(uint16_t tempState) { //do one pulse of the printhead with the specified input on the primitives for a shorter while (LSB to MSB, P0 to P13)
      SetPrimitiveClock(0); //set clock to 0
      SetPrimitiveClear(1); //set clear to 1
      SetPrimitivePins(tempState); //set primitive pins
      noInterrupts(); //allow no interrupts during pulse
      SetPrimitiveClock(1); //set clock to 1
      __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"); //18 NOP's, ca. 200ns
      __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"); //18 NOP's, ca. 200ns
      __asm__("nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t""nop\n\t"); //18 NOP's, ca. 200ns
      SetPrimitiveClear(0); //set clear to 0
      SetPrimitiveClock(0); //set clock to 0
      interrupts(); //allow interrupts again
    }
    void AddressNext() { //reset clock
      SetAddressClock(1);
      delayMicroseconds(1);
      SetAddressClock(0);
      delayMicroseconds(1);
    }
    void AddressReset() { //reset address
      SetAddressReset(1);
      delayMicroseconds(1);
      SetAddressReset(0);
      delayMicroseconds(1);
    }

    //Read values -----------------------------------------------------
    uint32_t GetTSRRaw(uint8_t tempResolution) { //read TSR
      tempResolution = constrain(tempResolution, 0, 13); //limit resolution input
      analogReadResolution(tempResolution); //set resolution
      uint16_t temp = analogRead(senseTSR);
      analogReadResolution(10); //return resolution to 10 (default)
      return temp;
    }
    uint32_t Get10XRaw(uint8_t tempResolution) { //Read 10x
      tempResolution = constrain(tempResolution, 0, 13); //limit resolution input
      analogReadResolution(tempResolution); //set resolution
      uint16_t temp = analogRead(sense10X);
      analogReadResolution(10); //return resolution to 10 (default)
      return temp;
    }
    uint8_t GetNozzleCheck() { //Read nozzle check
      return digitalRead(nozzleCheck);
    }
    int32_t GetTemperature() { //enables the head if required and calculates temperature (in .1C. 200 is 20.0C)
      SetEnableTemp(1); //set the head (temporarily) to enabled while checking
      AddressReset(); //reset address because else for magical reasons the 10X will fail to read

      int16_t temp_10x = Get10XRaw(13); //get analog read in 13 bit resolution
      int16_t temp_tsr = GetTSRRaw(13); //get analog read in 13 bit resolution
      const float temp_vin = 3.3;
      const float temp_r1 = 330.0;

      //Vout = (R2 / (R2 + R1)) * Vin, R1 is 330ohm
      //R2 = ((Vout x R1)/(Vin-Vout))

      //calculate in celcius
      float temp_f_calc;
      //get the 10x voltage
      temp_f_calc = float(temp_10x);
      temp_f_calc /= 8192; //get the fraction
      temp_f_calc *= temp_vin; //get the voltage
      float temp_10x_res = ((temp_f_calc * temp_r1) / (temp_vin - temp_f_calc)); //get the 10x resistance
      //Serial.print("10X resistor: "); Serial.println(temp_10x_res);

      //get the tsr voltage
      temp_f_calc = float(temp_tsr);
      temp_f_calc /= 8192; //get the fraction
      temp_f_calc *= temp_vin; //get the voltage
      float temp_tsr_res = ((temp_f_calc * temp_r1) / (temp_vin - temp_f_calc)); //get the tsr resistance
      //Serial.print("RSR resistor: "); Serial.println(temp_tsr_res);

      //check both to be reasonable values
      if (temp_10x_res < 150.0 || temp_10x_res > 500.0) return -2;
      if (temp_tsr_res < 150.0 || temp_tsr_res > 500.0) return -2;

      //get the TSR - 10X
      //at 10 ohms, the temperature is 20C, for every 1.1 ohms the temperature rises 1 degree
      //T = 1.1R+10
      temp_f_calc = temp_tsr_res - temp_10x_res;
      float temp_temp = 1.1 * temp_f_calc + 10.0;
      temp_temp *= 10.0; //multiply by ten to add the decimal value
      //Serial.print("Temperature: "); Serial.println(temp_temp);
      return long(temp_temp); //return celcius

      EnableReset(); //set the head to previous state
    }

    //Raw pin modification -------------------------------------------
    void SetPrimitiveClock(uint8_t tempState) { //set primitive clock
      if (tempState == 1) {
        digitalWrite(primitiveClock, 1);
      }
      else {
        digitalWrite(primitiveClock, 0);
      }
    }
    void SetPrimitiveClear(uint8_t tempState) { //set primitive clear
      if (tempState == 1) {
        digitalWrite(primitiveClear, 1);
      }
      else {
        digitalWrite(primitiveClear, 0);
      }
    }
    void SetPrimitivePins(uint16_t tempState) { //set primitive pins
      uint8_t tempValue;
      for (uint8_t p = 0; p < 14; p++) { //loop through
        tempValue = bitRead(tempState, p); //get correct bit from value
        digitalWrite(primtivePins[p], tempValue); //
      }
    }
    void SetAddressClock(uint8_t tempState) { //set address next
      if (tempState == 1) {
        digitalWrite(addressClock, 1);
      }
      else {
        digitalWrite(addressClock, 0);
      }
    }
    void SetAddressReset(uint8_t tempState) { //set address reset
      if (tempState == 1) {
        digitalWrite(addressReset, 1);
      }
      else {
        digitalWrite(addressReset, 0);
      }
    }
    void SetEnable(uint8_t tempState) { //sets the enable state of the printhead to 0 or 1
      if (tempState == 1) {
        digitalWrite(headEnable, 1);
        headEnabled = 1; //set enabled state to 1
      }
      else {
        digitalWrite(headEnable, 0);
        headEnabled = 0; //set enabled state to 0
      }
    }
    void SetEnableTemp(uint8_t tempState) { //sets the enable state of the printhead to 0 or 1, but does not alter the enabled variable
      if (tempState == 1) {
        digitalWrite(headEnable, 1);
      }
      else {
        digitalWrite(headEnable, 0);
      }
    }
    void EnableReset() { //set enable to last official state
      digitalWrite(headEnable, headEnabled);
    }

    //variable functions ----------------------------------------------------------------------------------
    void ResetBurst() { //resets the burst variable
      for (uint8_t a = 0; a < 22; a++) {
        burstVar[a] = 0;
      }
    }
    uint8_t GetPrimitive(uint16_t temp_nozzle) { //returns the primitive location of a given nozzle
      temp_nozzle = constrain(temp_nozzle, 0, 299);
      return nozzleTablePrimitive[temp_nozzle];
    }
    uint8_t GetAddress(uint16_t temp_nozzle) { //returns the address location of a given nozzle
      temp_nozzle = constrain(temp_nozzle, 0, 299);
      return nozzleTableAddress[temp_nozzle];
    }
    int16_t GetNozzle(uint8_t temp_primitive, uint8_t temp_address) { //returns the nozzle of a primitive and address, -1 if no nozzle is there
      temp_primitive = constrain(temp_primitive, 0, 13);
      temp_address = constrain(temp_address, 0, 21);
      return nozzleTableReverse[temp_primitive][temp_address];
    }
    uint16_t *ConvertB6RawToBurst(uint8_t temp_input[50], uint16_t temp_burst[22]) { //takes an array of 50 bytes where the 6 LSB are nozzle on or off, starting at 0 and ending at 299 and converts to a pointed uint16_t[22] burst array
      uint16_t temp_nozzle = 0; //keeps track of the current nozzle
      uint8_t temp_state; //used to write on or off to
      uint8_t temp_add, temp_prim;
      for (uint8_t a = 0; a < 22; a++) {
        temp_burst[a] = 0;
      }
      for (uint8_t B = 0; B < 50; B++) { //bytes within byte
        for (uint8_t b = 0; b < 6; b++) { //bits within byte
          for (uint8_t r = 0; r < dpi_repeat; r++) { //repeat pixels   
            if (temp_nozzle < 300){ //only look if nozzle is lower than 300 (some resolutions give halve filled B64 values)
              temp_add = nozzleTableAddress[temp_nozzle];
              temp_prim = nozzleTablePrimitive[temp_nozzle];
              //if (b%2 == 1){ //TEMPORARY, ONLY PRINT ODD OR EVEN
                temp_state = bitRead(temp_input[B], b); //get on or off from input
              //}
              //else {
              //  temp_state = 0;
              //}
              bitWrite(temp_burst[temp_add], temp_prim, temp_state); //set nozzle in burst on or off
              temp_nozzle ++; //add one to nozzle
            }
          }
        }
      }
      return temp_burst;
    }
    uint16_t *ConvertB6ToggleToBurst(uint8_t temp_input[50], uint16_t temp_burst[22]) { //takes raw data in toggle format and converts it to burst
      uint16_t temp_nozzle = 0; //keeps track of the current nozzle
      uint8_t temp_state = 1; //used to write on or off to
      uint8_t temp_value;
      uint8_t temp_add, temp_prim;
      for (uint8_t B = 0; B < 50; B++) { //loop through all array values, turn on and off, stop when 300 is reached
        temp_value = temp_input[B];
        for (uint8_t R = 0; R < temp_value; R++) { //for the amount of repeats given
          for (uint8_t r = 0; r < dpi_repeat; r++) { //repeat pixels
            temp_add = nozzleTableAddress[temp_nozzle];
            temp_prim = nozzleTablePrimitive[temp_nozzle];
            bitWrite(temp_burst[temp_add], temp_prim, temp_state); //set nozzle in burst on or off
            temp_nozzle++; //go to next nozzle
            if (temp_nozzle == 300) { //if 300 is reached, end
              return temp_burst; //return value if 300 is reached
            }
          }
        }
        //toggle state
        if (temp_state == 1) temp_state = 0;
        else temp_state = 1;
      }
      return temp_burst;
    }
    uint16_t *ConvertB8ToBurst(uint8_t temp_input[38], uint16_t temp_burst[22]) { //takes an array of 38 bytes where the 8 LSB are nozzle on or off, starting at 0 and ending at 299 and converts to a pointed uint16_t[22] burst array
      uint16_t temp_nozzle = 0; //keeps track of the current nozzle
      uint8_t temp_state; //used to write on or off to
      for (uint8_t B = 0; B < 38; B++) { //bytes within byte
        for (uint8_t b = 0; b < 8; b++) { //bits within byte
          for (uint8_t r = 0; r < dpi_repeat; r++) { //repeat pixels
            temp_state = bitRead(temp_input[B], b); //get on or off from input
            bitWrite(temp_burst[nozzleTableAddress[temp_nozzle]], nozzleTablePrimitive[temp_nozzle], temp_state); //set nozzle in burst on or off
            temp_nozzle ++; //add one to nozzle
          }
        }
      }
      return temp_burst;
    }
    void SetDPI(uint16_t temp_dpi) { //takes dpi, does some maths on it and sets the resolution on decoding
      if (temp_dpi <= 600) { //on valid dip (max is 600
        dpi_repeat = 600 / temp_dpi; //calculate repeat value. Repeat is how often each pixel is repeated going from nozzle 0 to 299
        dpi = 600 / dpi_repeat; //get actual DPI, based on input, rounding to nearest number that is a division from 600
        Serial.print("Setting DPI to: "); Serial.println(dpi);
      }
    }
    uint8_t GetEnabledState() { //returns the current head enable state
      return headEnabled;
    }
};

/*//nozzle tables old
    const uint8_t nozzleTableAddress[300] = {
      21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11,
      13, 15, 11, 20, 15, 3, 10, 0, 3, 9, 0, 21, 9, 1, 21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18,
      4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 3, 20, 0, 3, 9, 0, 21, 9, 5, 21, 16,
      5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15,
      11, 20, 15, 3, 20, 0, 3, 9, 0, 21, 9, 5, 21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7,
      18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 3, 20, 0, 21, 9, 5, 21, 9, 5, 21, 16, 5, 19,
      16, 10, 19, 8, 7, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20,
      15, 3, 20, 0, 3, 9, 0, 21, 9, 5,  21, 16, 5, 19, 16, 10, 19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12,
      7, 17, 12, 3, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 3, 20, 0, 3, 9, 0, 21, 9, 5, 21, 16, 5, 19, 16, 10,
      19, 8, 10, 14, 8, 6, 14, 4, 6, 18, 4, 7, 18, 12, 7, 17, 12, 2, 17, 1, 2, 13, 1, 11, 13, 15, 11, 20, 15, 11
    };*/

