/*
   The buffer is a class than handles all data regarding the inkjet. It takes burst and position information and stores it in a FiFo buffer. The buffer is what takes the most memory on the microcontroller

   Todo:
   - Do not meticulously calculate buffer read and write left, but simply hold a variable that keeps this data. It will not magically add 15 lines without the functions knowing about it.
*/

#include "Arduino.h"
#include "Buffer.h" //<*whispers: "there is actually nothing in there"

#define buffer_size 1000 //the number of 48 byte blocks the buffer consists of (44 for inkjet, 4 for coordinate)
#define primitive_overlay_even 5461 //B0001010101010101
#define primitive_overlay_odd 10922 //B0010101010101010

class Buffer {
    //class member variables
    uint16_t burstBuffer[buffer_size][22]; //burst data
    int32_t positionBuffer[buffer_size]; //position data
    int32_t readPosition[2], writePosition; //read and write positions for odd and even
    uint16_t primitiveOverlay[2]; //0 or 1 for even or odd
    //int32_t readLeft[2], writeLeft; //future additions
    uint8_t side_active[2] = {0,0}; //turns the overlay on or off for a side

  public:
    Buffer() {
      //make odd and even overlays, used for  the 2 positions in the buffer
      primitiveOverlay[0] = primitive_overlay_odd;
      primitiveOverlay[1] = primitive_overlay_even;

      ClearAll(); //reset the buffer
    }

    int32_t Next(uint8_t temp_side) { //if possible, adds one to the read position and returns lines left. Data needs to be fetched using other functions
      temp_side &= 1; //constrain side
      if (ReadLeftSide(temp_side) > 0) { //if there is something left ot read
        readPosition[temp_side] ++; //add one to the read position
        readPosition[temp_side] = readPosition[temp_side] % buffer_size; //overflow protection
        //Serial.print("Buffer next side: "); Serial.print(temp_side);  Serial.print(", left: "); Serial.println(ReadLeftSide(temp_side));
        return ReadLeftSide(temp_side);
      }
      return -1; //return back -1 if it is not possible
    }
    int32_t LookAheadPosition(uint8_t temp_side){ //takes the position in the next buffer position
      temp_side &= 1; //constrain side
      int32_t temp_read_pos;
      if (ReadLeftSide(temp_side) > 0) { //if there is something left ot read
        temp_read_pos = readPosition[temp_side]; //make a temporary read position and make it go to the next pos
        temp_read_pos ++;
        temp_read_pos = temp_read_pos % buffer_size; //overflow protection
        return positionBuffer[temp_read_pos];
      }
    }
    int32_t Add(int32_t temp_position, uint16_t temp_input[22]) { //adds a coordinate (int32_t and a burst to the buffer, returns space left if successful, -1 if failed
      int32_t temp_left = WriteLeft();
      if (temp_left > 0) { //if there is space left in the buffer
        positionBuffer[writePosition] = temp_position; //add position
        //Serial.print("Add to buffer: "); Serial.print(temp_position); Serial.print(": ");
        for (uint8_t a = 0; a < 22; a++) { //add burst
          burstBuffer[writePosition][a] = temp_input[a];
          //Serial.print(temp_input[a]); Serial.print(", ");
        }
        //Serial.println("");
        writePosition++; //add one to write position
        writePosition = writePosition % buffer_size; //overflow protection
        temp_left--;
        //Serial.print("Buffer write left: "); Serial.println(temp_left);
        //Serial.print("Buffer read left: "); Serial.println(ReadLeft());
        return temp_left;
      }
      return -1; //return a -1 if this failed
    }
    int32_t ReadLeft() { //returns the number of free buffer read slots. Automatically returns the smallest value
      //calculate read lines left on pos 0 by subtracting read position and adding write position, and then take the modulo of the buffer size to protect from overflows
      int32_t temp_calc1, temp_calc2;
      temp_calc1 = buffer_size;
      temp_calc1 += writePosition;
      temp_calc2 = temp_calc1;
      temp_calc1 -= readPosition[0];
      temp_calc2 -= readPosition[1];
      temp_calc1 = temp_calc1 % buffer_size; //constrain to buffer size
      temp_calc2 = temp_calc2 % buffer_size;
      temp_calc1 -= 1; //subtract one because read can never be equal to write
      temp_calc2 -= 1;
      if (temp_calc1 < temp_calc2) { //return smallest value
        return temp_calc1;
      }
      else {
        return temp_calc2;
      }
    }
    int32_t ReadLeftSide(uint8_t temp_side) {//returns the number of lines left to read for a given size
      temp_side &= 1; //constrain side
      int32_t temp_calc;
      temp_calc = buffer_size;
      temp_calc += writePosition;
      temp_calc -= readPosition[temp_side];
      temp_calc = temp_calc % buffer_size; //constrain to buffer size
      temp_calc -= 1; //subtract one because read can never be equal to write
      return temp_calc;
    }
    int32_t WriteLeft() { //returns the number of free buffer write slots. Automatically returns the smallest value
      //calculate write lines left on pos 0 by subtracting write position and adding read position, and then take the modulo of the buffer size to protect from overflows
      int32_t temp_calc1, temp_calc2;
      temp_calc1 = buffer_size;
      temp_calc1 -= writePosition;
      temp_calc2 = temp_calc1;
      temp_calc1 += readPosition[0];
      temp_calc2 += readPosition[1];
      temp_calc1 = temp_calc1 % buffer_size; //constrain to buffer size
      temp_calc2 = temp_calc2 % buffer_size;
      temp_calc1 -= 2; //subtract to create some protection
      temp_calc2 -= 2;
      if (temp_calc1 < temp_calc2) { //return smallest value
        return temp_calc1;
      }
      else {
        return temp_calc2;
      }
    }
    void ClearAll() { //resets the read and write positions in the buffer
      for (uint16_t b = 0; b < buffer_size; b++){
        for (uint8_t a = 0; a < 22; a++){
          burstBuffer[b][a] = 0;
        }
        positionBuffer[b] = 0;
      }
      readPosition[0] = 0;
      readPosition[1] = 0;
      writePosition = 1;
    }
    uint16_t GetPulse(uint8_t temp_address) {
      temp_address = constrain(temp_address, 0, 21);
      uint16_t temp_pulse = 0; //make the return pulse
      temp_pulse = temp_pulse & primitive_overlay_even & burstBuffer[readPosition[0]][temp_address];
      temp_pulse = temp_pulse & primitive_overlay_odd & burstBuffer[readPosition[1]][temp_address];
      return temp_pulse;
    }
    void SetActive(uint8_t temp_side, uint8_t temp_state){
      temp_side = constrain(temp_side,0,1);
      temp_state = constrain(temp_state,0,1);
      side_active[temp_side] = temp_state;
    }
    uint16_t *GetBurst(uint16_t temp_burst[22]) { //gets the complete current burst, based on the 2 positions from buffer and writes it to the input uint16_t[22] array
      uint16_t temp_odd, temp_even; //temporary burst values
      for (uint8_t a = 0; a < 22; a++) { //walk through the entire pulse
        temp_burst[a] = 0; //reset value
        if (side_active[0] == 1){ //if odd side is active
          temp_odd = primitive_overlay_odd & burstBuffer[readPosition[0]][a]; //odd side
        }
        else {
          temp_odd = 0;
        }
        if (side_active[1] == 1){ //if even side is active
          temp_even = primitive_overlay_even & burstBuffer[readPosition[1]][a]; //even side
        }
        else {
          temp_even = 0;
        }
        temp_burst[a] = temp_burst[a] | temp_even; //add even
        temp_burst[a] = temp_burst[a] | temp_odd; //add odd
      }
      return temp_burst;
    }
    int32_t GetPosition(uint8_t temp_side) {
      temp_side &= 1; //constrain side
      return positionBuffer[readPosition[temp_side]];
    }

  private:
};
