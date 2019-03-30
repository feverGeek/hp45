/*  
    HP45 standalone is software used to control the Teensy 3.2 based standalone controller for the HP45
    Copyright (C) 2018  Yvo de Haas
    
    HP45 standalone is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HP45 standalone is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
   IMPORTANT: To make sure that the timing on critical functions, run this code at 96MHz

   The buffer contains all data to be printed. The position holds the reading and decoding of the encoder
   The printhead contains all functions that control the HP45 controller

   The flow of printing. The buffer gets loaded with data. This includes the inkjet data and a position
   The buffer changes when a new coordinate is reached. Once this is done, the direction required for the new coordinate is determined.
   Based on this direction, the condition for the new coordinate is determined (higher or lower than)
   Once a coordinate is reached, the burst of the current coordinate is made active, and the next line in the buffer is loaded

    todo:
    -Remove redundant enable and disable printhead calls
*/
/*
   Temporary serial command for printing 0-74, 75-149, 150-224, 225-299 and 0
   in 10000 micron intervals:

  SBR A AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  SBR CcQ ////////////HAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  SBR E4g AAAAAAAAAAAA4////////////AAAAAAAAAAAAAAAAAAAAAAAAA
  SBR HUw AAAAAAAAAAAAAAAAAAAAAAAAA////////////HAAAAAAAAAAAA
  SBR JxA AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA4////////////
  SBR MNQ AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

  SBR A AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  SBR CcQ //////////////////////////////////////////////////
  SBR MNQ AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA

  preheat 10000 times
  PHD CcQ
*/

#include "Printhead.cpp"
#include "Buffer.cpp"
#include "Serialcom.cpp"

Printhead HP45(29, 33, 24, 16, 28, 15, 26, 14, 27, 20, 8, 18, 7, 17, 25, 19, 3, 2, 23, 22, A11, A10);
Buffer BurstBuffer;
SerialCommand Ser;

//variables
uint16_t inkjetDensity = 100; //percentage, how often the printhead should burst
int32_t inkjetMinPosition[2], inkjetMaxPosition[2]; //where the inkjet needs to start and end
uint8_t inkjetEnabled[2], inkjetEnabledHistory[2]; //whether a side a allowed to jew ink or not (+ history)
int32_t CurrentPosition[2]; //odd and even position of the printhead
int32_t CurrentVelocity; //the current velocity of the printhead
int8_t CurrentDirection; //which direction the printhead is moving
int8_t requiredPrintingDirection[2]; //the direction between the current point and the next point
int32_t targetPosition[2]; //the position where the buffer needs to go next
uint8_t nextState[2]; //handles how the next line in the buffer is used
uint16_t serialBufferCounter = 0; //counter for lines received, when target reached, return a WL over serial
uint8_t serialBufferWLToggle = 1; //whether to respond with a WL when a threshold is reached
uint8_t serialBufferFirst[2] = {1, 1}; //the first buffer entry is treated different

uint32_t inkjetBurstDelay; //how long to wait between each burst
uint32_t inkjetLastBurst; //when the last burst was
uint16_t CurrentBurst[22]; //the current printing burst
uint8_t NozzleState[300];

//variables for transfering from serial to inkjet
uint16_t inkjet_line_number;
uint32_t inkjet_command;
int32_t inkjet_small_value;
uint8_t inkjet_raw[50];

//constants
uint16_t printheadDPI = 600;
#define serialBufferTarget 50 //after how many lines of code received the printhead will return a writeleft to the controller

//upper and lower threshold are used for the wakeup WL response. When the lower threshold is reached, serialBufferWLToggle flips from 0 to 1
//when then the upper threshold is reached, the firmware will send a writeleft to the controller, notifying there is space again
//this measure is in place bacause the software may not be aware space has opened up in the meantime, relying on fairly slow updates
//An upper and lower threshold is picked so it does not send a WL every time a single threshold is passed, there is hysteresis
#define bufferLowerThreshold 200
#define bufferUpperThreshold 500

//test variable (temporary)
uint32_t temp_timer, temp_delay = 1000000;
uint8_t temp_burst_on = 0;

void setup() {
  delay(2500); //delay to give serial time to start on pc side
  Ser.Begin(); //start serial connection
  BurstBuffer.ClearAll(); //reset the buffer

  //TestFill(); //fill buffer with test program
  inkjetEnabled[0] = 0;
  inkjetEnabled[1] = 0;
  inkjetEnabledHistory[0] = 0;
  inkjetEnabledHistory[1] = 0;
  nextState[0] = 0; nextState[1] = 0; //set next state to starting position
}

void loop() {
  UpdateAll(); //update all critical functions

  /*if (millis() > temp_timer + temp_delay) {
    temp_timer = millis();
    Serial.print("Pos0: "); Serial.print(CurrentPosition[0]); Serial.print(" Burst on: "); Serial.println(temp_burst_on);
    Serial.print("Current burst: ");
    for (uint8_t a = 0; a < 22; a++) {
      Serial.print(CurrentBurst[a]); Serial.print(", ");
    }
    Serial.println("");
    }//*/
}

void UpdateAll() { //update all time critical functions
  PositionUpdate(); //get new position
  //get all velocitys and positions
  CurrentPosition[0] = PositionGetRowPositionMicrons(0);
  CurrentPosition[1] = PositionGetRowPositionMicrons(1);
  CurrentVelocity = PositionGetVelocity();
  CurrentDirection = PositionGetDirection();

  //get inkjet values
  BufferUpdateValues(); //see if new values in the buffer need to be called
  InkjetUpdateBurstDelay(); //calculate burst delay based on density and speed
  InkjetUpdateBurst(); //check if the printhead needs to be on based on required direction, actual direction, start pos and end pos

  if (Ser.Update() == 1) { //get serial, if 1, add new command
    SerialExecute(); //get command and execute it
  }
  SerialWLPush(); //check push WL requirements

  //get SPI
}

///----------------------------------------------------------
void InkjetUpdateBurst() { //checks if the head is within range to burst (direction and position) and bursts the head is conditions are met
  uint8_t temp_state_changed = 0; //value to indicate enabled states have changed
  for (uint8_t s = 0; s <= 1; s++) {
    if (requiredPrintingDirection[s] == CurrentDirection && CurrentPosition[s] > inkjetMinPosition[s] && CurrentPosition[s] < inkjetMaxPosition[s] && CurrentVelocity != 0) { //if head is within the inkjet limits and direction matches and velocity is not 0
      inkjetEnabled[s] = 1; //direction and area match, enable head
      if (inkjetEnabledHistory[s] == 0) { //if history and current are not the same, mark change buffer
        inkjetEnabledHistory[s] = 1;
        temp_state_changed = 1;
        BurstBuffer.SetActive(s, 1); //set side of buffer active
      }
    }
    else { //if requirements are not met, set enabled and history to 0
      inkjetEnabled[s] = 0;
      if (inkjetEnabledHistory[s] == 1) {  //if history and current are not the same, mark change buffer
        inkjetEnabledHistory[s] = 0;
        temp_state_changed = 1;
        BurstBuffer.SetActive(s, 0); //set side of buffer inactive
      }
    }
  }
  if (temp_state_changed == 1) { //if any of the states changed, request new data from the buffer with correct overlays
    BurstBuffer.GetBurst(CurrentBurst); //get new burst from the buffer
  }
  //check burst time conditions
  if (micros() - inkjetLastBurst > inkjetBurstDelay) { //if burst is required again based on time (updated regradless of burst conditions)
    inkjetLastBurst = micros();
    if (inkjetEnabled[0] == 1 ||  inkjetEnabled[1] == 1) { //if the head is within bounds, burst head
      HP45.SetEnable(1); //enable the head
      temp_burst_on = 1;
      HP45.Burst(CurrentBurst); //burst the printhead
    }
    else {
      HP45.SetEnable(0); //disable the head
      temp_burst_on = 0;
    }
  }
}
void InkjetUpdateBurstDelay() { //recalculates burst delay
  if (CurrentVelocity != 0) { //if velocity is more than 0
    float temp_dpi = float(printheadDPI) * float(inkjetDensity); //calculate actual DPI
    temp_dpi /= 100.0; //divide by percentage
    float temp_vel = float(CurrentVelocity) / 25.4; //get velocity in inches per second
    float temp_calc = 1000000.0; //get microseconds
    temp_calc /= temp_vel; //divide by velocity in inch per second and actual DPI to get burst delay
    temp_calc /= temp_dpi;

    inkjetBurstDelay = long(temp_calc); //write to the variable
  }
}
void BufferUpdateValues() { //checks if the next positions in the buffer can be called
  for (uint8_t s = 0; s <= 1; s++) { //check if the burst needs to change (for odd and even)
    uint8_t update_values = 0;
    if (requiredPrintingDirection[s] == -1) { //if required direction is negative
      if (CurrentPosition[s] < targetPosition[s]) { //check if new position requirement is met
        update_values = 1; //set buffer to next position
      }
    }
    else { //if required direction is positive
      if (CurrentPosition[s] > targetPosition[s]) { //check if new position requirement is met
        update_values = 1; //set buffer to next position
      }
    }

    if (update_values == 1) {//if a position in the buffer needs to advance
      //first, modify the inkjet burst data to the now reached position, regardless of whether a further line is available
      //(any last all off command should never be ignored)
      if (BurstBuffer.ReadLeftSide(s) > 0 && nextState[s] == 0) { //if there is space left to read
        nextState[s] = 1; //set next state to 1, so other target functions can happen if possible
        BurstBuffer.Next(s); //go to next position in the buffer
        BurstBuffer.GetBurst(CurrentBurst); //active the burst on the now reached position (do so regardless of new line)

        //Serial.print("Buffer Next: "); Serial.print(s); Serial.print(", Pos: "); Serial.println(targetPosition[s]);
      }

      if (BurstBuffer.ReadLeftSide(s) > 0 && nextState[s] == 1) { //if there is space left to read for the look ahead function
        int32_t temp_ahead_pos; //get old position stored before it is overwritten
        temp_ahead_pos = BurstBuffer.LookAheadPosition(s);

        if (serialBufferFirst[s] == 1) { //if not first line, use last position in the buffer
          targetPosition[s] = CurrentPosition[s];
          serialBufferFirst[s] = 0; //set first to 0 to stop future use
        }

        //Serial.print("look ahead: "); Serial.println(temp_ahead_pos);
        if (temp_ahead_pos - targetPosition[s] >= 0) requiredPrintingDirection[s] = 1; //determine required direction
        else requiredPrintingDirection[s] = -1;
        //set the inkjet limits, the coordinates where the printhead is allowed to print
        if (temp_ahead_pos < targetPosition[s]) { //determine smallest value
          inkjetMinPosition[s] = temp_ahead_pos;
          inkjetMaxPosition[s] = targetPosition[s];
        }
        else {
          inkjetMinPosition[s] = targetPosition[s];
          inkjetMaxPosition[s] = temp_ahead_pos;
        }
        targetPosition[s] = temp_ahead_pos; //set new position
        nextState[s] = 0; //set next state to next line again
      }
    }
  } //end of s(ide) for loop
}
void SerialExecute() { //on 1 in serial update, get values and execute commands
  //inkjet_line_number = Ser.GetLineNumber(); //get line number
  inkjet_command = Ser.GetCommand(); //get command
  inkjet_small_value = Ser.GetSmallValue();  //get small value
  Ser.GetRaw(inkjet_raw); //get raw

  //check line number (lol, nope, not right now, why did I bother then with a linenumber? dunno, might actually disable it)
  //Later edit, I already disabled it.

  //Serial.print("Executing command: "); Serial.println(inkjet_command);
  switch (inkjet_command) { //check command, execute based on command
    case 5456466: { //SBR, send buffer raw
        //Serial.println("Send to buffer, raw");
        //Serial.println(inkjet_small_value);
        //for (uint8_t B = 0; B < 50; B++){
        //  Serial.print(inkjet_raw[B]); Serial.print(" ");
        //}
        //Serial.println("");
        HP45.ConvertB6RawToBurst(inkjet_raw, CurrentBurst);
        //for (uint8_t B = 0; B < 22; B++){
        //  Serial.print(CurrentBurst[B]); Serial.print(" ");
        //}
        //Serial.println("");
        BurstBuffer.Add(inkjet_small_value, CurrentBurst);
      } break;
    case 5456468: { //SBT, send buffer toggle

      } break;
    case 5456210: { //SAR, send asap raw
        HP45.ConvertB6RawToBurst(inkjet_raw, CurrentBurst);
        HP45.Burst(CurrentBurst);
      } break;
    case 5456212: { //SAT, send asap toggle

      } break;
    case 5261396: { //PHT, preheat
        inkjet_small_value = constrain(inkjet_small_value, 0, 25000);
        uint8_t temp_enabled = HP45.GetEnabledState();
        HP45.SetEnable(1); //temporarily enable head
        HP45.Preheat(inkjet_small_value); //preheat for n times
        HP45.SetEnable(temp_enabled); //set enable state to previous
      } break;
    case 5263949: { //PRM, prime printhead
        inkjet_small_value = constrain(inkjet_small_value, 0, 25000);
        uint8_t temp_enabled = HP45.GetEnabledState();
        HP45.SetEnable(1); //temporarily enable head
        HP45.Prime(inkjet_small_value); //preheat for n times
        HP45.SetEnable(temp_enabled); //set enable state to previous
      } break;
    case 5523524: { //THD, Test head
        inkjet_small_value = constrain(inkjet_small_value, 0, 10);
        HP45.TestHead(NozzleState);
        Ser.RespondTestResults(1, NozzleState);
      } break;
    case 4674640: { //GTP, Get temperature
        Ser.RespondTemperature(HP45.GetTemperature());
      } break;
    case 5457232: { //SEP, Set encoder position
        PositionSetBasePositionMicrons(inkjet_small_value); //Set encoder position with small value
      } break;
    case 4670800: { //GEP, Get encoder position
        //Serial.print("sending position: "); Serial.println(PositionGetBasePositionMicrons());
        Ser.RespondEncoderPos(PositionGetBasePositionMicrons());
      } break;
    case 5456976: { //SDP, Set DPI
        HP45.SetDPI(inkjet_small_value); //set DPI with small value
      } break;
    case 5456974: { //SDN, Set Density
        inkjetDensity = inkjet_small_value; //set density with small value
      } break;
    case 4346444: { //BRL, Buffer, read left
        Ser.RespondBufferReadLeft(BurstBuffer.ReadLeft());
      } break;
    case 4347724: { //BWL, Buffer, write left
        Ser.RespondBufferWriteLeft(BurstBuffer.WriteLeft());
      } break;
    case 4342604: { //BCL, Buffer clear
        BurstBuffer.ClearAll(); //clear all data in the buffer
        serialBufferFirst[0] = 1; //set buffers to firstline
        serialBufferFirst[1] = 1; //set buffers to firstline
      } break;
    case 5391698: { //RER, reset error

      } break;
    case 5393486: { //RLN, reset line number

      } break;
    case 5457230: { //SEN, software enable

      } break;
    case 4736334: { //HEN, hardware enable
        inkjet_small_value = constrain(inkjet_small_value, 0, 1);
        HP45.SetEnable(inkjet_small_value);
      } break;
    case 4279896: { //ANX, address next
        HP45.AddressNext(); //next address
      } break;
    case 4280915: { //ARS, address reset
        HP45.AddressReset(); //reset address
      } break;
    case 4278100: { //AGT, address goto
        inkjet_small_value = constrain(inkjet_small_value, 0, 22); //limit size of small
        HP45.AddressReset(); //reset address
        for (uint8_t a = 0; a < inkjet_small_value; a++) {
          HP45.AddressNext(); //next address
        }
      } break;
    case 5264212: { //PST, primitive set

      } break;
    case 5263436: { //PPL, primitive pulse

      } break;
  }
  //auto Writeleft status block
  serialBufferCounter++;
  if (serialBufferCounter > serialBufferTarget) {
    Ser.RespondBufferWriteLeft(BurstBuffer.WriteLeft()); //respond with write left
    serialBufferCounter = 0; //reset value
  }
}

void SerialWLPush() { //checks if requirements for a pushed WL response are met
  int32_t temp_buffer = BurstBuffer.WriteLeft();
  if (temp_buffer < bufferLowerThreshold) { //if write left is lower than threshold
    serialBufferWLToggle = 0; //set flag to low
  }
  if (temp_buffer > bufferUpperThreshold) { //if write left becomes higher than threshold
    if (serialBufferWLToggle == 0) { //if write left was not higher than threshold
      serialBufferWLToggle = 1; //set flag to high
      Ser.RespondBufferWriteLeft(temp_buffer); //respond with write left
    }
  }
}

void TestFill() {
  Serial.println("test fill buffer");
  uint16_t temp_burst[22];
  uint8_t temp_nozzles[50];
  uint8_t temp_state;
  uint8_t temp_byte, temp_bit;
  for (uint8_t n = 0; n < 50; n++) { //add final side
    temp_nozzles[n] = 0;
  }
  HP45.ConvertB6RawToBurst(temp_nozzles, temp_burst);
  BurstBuffer.Add(0, temp_burst);

  int32_t temp_pos = 100000;
  for (uint8_t r = 0; r < 4; r++) {
    temp_byte = 0; temp_bit = 0;
    for (uint16_t n = 0; n < 300; n += 2) {
      if (n / 70 == r) temp_state = 1;
      else temp_state = 0;
      bitWrite(temp_nozzles[temp_byte], temp_bit, temp_state);
      temp_bit += 2;
      if (temp_bit >= 6) {
        temp_bit = 0;
        temp_byte++;
      }
    }

    HP45.ConvertB6RawToBurst(temp_nozzles, temp_burst);
    BurstBuffer.Add(temp_pos, temp_burst);
    Serial.print("adding line at: "); Serial.println(temp_pos);
    for (uint8_t a = 0; a < 22; a++) {
      Serial.print(temp_burst[a]); Serial.print(", ");
    }
    Serial.println("");
    temp_pos += 15000;
  }

  for (uint8_t n = 0; n < 50; n++) { //add final side
    temp_nozzles[n] = 0;
  }
  HP45.ConvertB6RawToBurst(temp_nozzles, temp_burst);
  BurstBuffer.Add(temp_pos, temp_burst);
  Serial.print("adding end at: "); Serial.println(temp_pos);
  //clock in data
  //BurstBuffer.Next(0);
  //BurstBuffer.Next(1);
}

void TempTestRows() {
  HP45.SetEnable(1);
  for (uint8_t n = 1; n < 300; n += 2) {
    for (uint8_t r = 0; r < 10; r++) {
      HP45.SingleNozzle(n);
      delayMicroseconds(100);
    }
  }
  HP45.SetEnable(0);
}

