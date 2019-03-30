/*
   Serial block takes care of receiving and decoding the serial connection
   The serial port used is the hardware USB to serial converter within the microcontroller itself.

   The serial, both ways sends in blocks of 64 bytes. The block buffer both ways is 64 bytes
   Each block consists of a semi fixed structure, the default is:
   CC-PPPPP-[50xR]e
   (CCC-PPPPP-RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRe)

   The actual amount can change, but a full line may not be more than 64 bytes including end characters

   - is a whitespace, to indicate an end of a line
   C = is a command. Each function has a 3 letter command
   P = is a small number, in inkjet reserved for position
   R = Raw data, in inkjet reserved for inkjet data
   e = end character ('\r' of '\n')

   Based on context, some blocks can be different, but by default all value carrying blocks will be encoded in base 64
   from 0 to 63: ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/

  The list of commands is as follows:
  -SBR: Send inkjet to buffer raw
  -SBT: Send inkjet to buffer toggle format
  -SAR: Send inkjet to print ASAP raw
  -SAT: Send inkjet to print ASAP toggle format

  -PHT: Preheat printhead
  -PRM: Prime printhead
  -THD: Test printhead (small value 1 returns n of 300 nozzles)
  -GTP: Get temperature

  -SEP: Set encoder position
  -GEP: Get encoder position
  -SDP: Set DPI
  -SDN: Set density

  -BRL: Buffer get read left
  -BWL: Buffer get write left
  -BCL: Buffer clear (::::::::::::::::To be implemented soon)
  -RER: reset error
  -RLN: reset line number

  -SEN: Software enable printhead
  -HEN: Hardware enable printhead

  -ANX: Address Next
  -ARS: Address Reset
  -AGT: Address Goto
  -PST: Primitive set
  -PPL: Primitive pulse
*/

#include "Arduino.h"

#define max_read_length 130

class SerialCommand {
    //class member variables
    uint8_t serial_update_state = 0; //whether the outside needs to call functions
    uint8_t serial_first_line = 1; //whether the line received is the first line, used to reset line numbers
    int8_t error_flag = 0; //what error if any is happening

    //read variables
    char read_buffer[64]; //buffer for storing serial characters
    char decode_buffer[max_read_length]; //buffer for storing serial line
    uint16_t decode_buffer_pos = 0; //how much is in the read buffer
    uint16_t serial_available; //number of available bytes in hardware buffer

    int16_t end_character_pos; //where the end character is

    //write variables
    char write_buffer[64]; //buffer for handling serial output
    uint8_t write_characters; //how many characters are in the buffer to be sent out

    //latest decoded line
    uint16_t serial_line_number;
    uint32_t serial_command;
    int32_t serial_small_value;
    uint8_t serial_raw[50];

  public:
    SerialCommand() {
      Serial.begin(115200); //Speed is actually irrelephant for this serial
      write_characters = 0;
    }

    void Begin() { //sends start message to indicate live connection
       Serial.println("Oasis SA 3.00.00"); //Send version number over serial
    }

    int16_t Update() { //updates all serial command functions
      /*
         Read what is currently in the buffer
         if a full line is in buffer, read and decode it
         If the program needs to act on a new line, update will return a 1, else a 0
         returns a -negative on error
      */
      serial_available = Serial.available(); //get number available
      if (serial_available) { //if there are lines in hardware buffer
        //Serial.print("Available: "); Serial.println(serial_available);
        serial_available = constrain(serial_available, 0, 64); //constrain lines available to hardware block size
        Serial.readBytes(read_buffer, serial_available); //read bytes from hardware buffer
        //send received response
        Serial.println("OK"); //print ok
        Serial.send_now(); //send data ASAP
        //Serial.print("received: "); Serial.write(read_buffer, serial_available); Serial.println("");

        //calculate decode buffer space left
        int16_t temp_space_left = max_read_length;
        temp_space_left -= decode_buffer_pos;
        temp_space_left -= serial_available;
        //Serial.print("Spaceleft: "); Serial.println(temp_space_left);

        uint16_t temp_read = 0;
        if (temp_space_left > 0) { //add read_buffer to serial_buffer if possible
          for (uint16_t b = decode_buffer_pos; b < decode_buffer_pos + serial_available; b++) { //add to read buffer
            decode_buffer[b] = read_buffer[temp_read];
            // Serial.print("Adding: '"); Serial.print(decode_buffer[b]); Serial.println("' to buffer");
            temp_read ++; //add one to readbuffer position
          }
          decode_buffer_pos += serial_available;
          //Serial.print("Buffer: '"); for (uint16_t r = 0; r < max_read_length; r++) {
          //  Serial.print(decode_buffer[r]);
          //} Serial.println("'");

          //Serial.print("Decode buffer pos: "); Serial.println(decode_buffer_pos);

          end_character_pos = -1; //reset end character pos
          for (uint16_t b = 0; b < max_read_length; b++) { //loop through buffer to find end character (start at front)
            if (IsEndCharacter(decode_buffer[b]) == 2) { //if end of line character
              end_character_pos = b; //set end character search
              break; //break from search loop
            }
          }

          uint16_t temp_start = 0, temp_end = 0; //keeps track of the blocks to decode
          uint8_t temp_character = 0; //keeps track of which character is being read
          uint8_t temp_read_error = 0, temp_ended = 0;
          uint32_t temp_decode;
          if (end_character_pos != -1) { //if end character found, start decoding from front
            //Serial.print("End of line character found: "); Serial.println(end_character_pos);
            
            //reset everything to 0 when reading starts
            //serial_line_number = 0;
            serial_command = 0;
            serial_small_value = 0;
            for (uint8_t r = 0; r < 50; r++){
              serial_raw[r] = 0;
            }           

            //look for first break, line number
            //(edit: Line number removed because I simply do not care and it is useless overhead that I do not want to deal with
            /*if (temp_read_error == 0 && temp_ended == 0) { //check if no error and not ended
              //Serial.println("Start looking for line number");
              for (uint16_t e = temp_start; e <= end_character_pos; e++) { //look for first break, lead number
                if (IsEndCharacter(decode_buffer[e])) {
                  if (IsEndCharacter(decode_buffer[e]) == 2) {
                    temp_ended = 1;  //set ended to 1 if this was a line end, not a space
                    //Serial.println("Ended");
                  }
                  //Serial.print("End char in LN: "); Serial.println(e);
                  serial_line_number = 0; //reset line number
                  temp_end = e; //note the position where the line ends
                  for (int16_t d = temp_end - 1; d >= temp_start; d--) { //decode from end (LSB) to start (MSB)
                    if (IsB64(decode_buffer[d])) { //if actual value
                      temp_decode = B64Lookup(decode_buffer[d]); //decode lead number
                      // Serial.print("Decode value to: "); Serial.println(temp_decode);
                      temp_decode = temp_decode << (6 * temp_character);
                      temp_character++;
                      serial_line_number |= temp_decode; //add value to lead number
                    }
                    else { //if any non B64 char found, go to error
                      temp_read_error = 1;
                      //Serial.print("Error");
                    }
                  }
                  //Serial.print("Decoded LN: "); Serial.println(serial_line_number);
                  temp_start = e + 1; //set new start
                  temp_character = 0;
                  break; //break from current for loop
                }
              }
            }*/

            //look for second break, command
            if (temp_read_error == 0 && temp_ended == 0) { //check if no error and not ended
              //Serial.println("Start looking for command");
              for (uint16_t e = temp_start; e <= end_character_pos; e++) { //look for second break, command
                if (IsEndCharacter(decode_buffer[e])) {
                  if (IsEndCharacter(decode_buffer[e]) == 2) {
                    temp_ended = 1;  //set ended to 1 if this was a line end, not a space
                    //Serial.println("Ended");
                  }
                  //Serial.print("End char in CM: "); Serial.println(e);
                  serial_command = 0; //reset command number
                  temp_end = e; //note the position where the line ends
                  for (int16_t d = temp_end - 1; d >= temp_start; d--) { //decode from end (LSB) to start (MSB)
                    temp_decode = decode_buffer[d];//find command (raw ascii to value)
                    temp_decode = temp_decode << 8 * temp_character; //store temp command
                    temp_character++;
                    serial_command |= temp_decode;
                  }
                  //Serial.print("Decoded CM: "); Serial.println(serial_command);
                  temp_start = e + 1; //set new start
                  temp_character = 0;
                  break; //break from current for loop
                }
              }
            }

            //look for third break, small value
            int8_t temp_signed = 1;
            if (temp_read_error == 0 && temp_ended == 0) { //check if no error and not ended
              //Serial.println("Start looking for small value");
              for (uint16_t e = temp_start; e <= end_character_pos; e++) { //look for third break, small
                if (IsEndCharacter(decode_buffer[e])) {
                  if (IsEndCharacter(decode_buffer[e]) == 2) {
                    temp_ended = 1;  //set ended to 1 if this was a line end, not a space
                    //Serial.println("Ended");
                  }
                  //Serial.print("End char in SV: "); Serial.println(e);
                  //look for first character being a sign
                  //Serial.println("Looking for sign: ");
                  if (decode_buffer[temp_start] == '-') { //if first character is sign
                    //Serial.println("Sign found");
                    temp_start ++; //ignore first character
                    temp_signed = -1; //set sign to -1
                  }
                  serial_small_value = 0; //reset command number
                  temp_end = e; //note the position where the line ends
                  for (int16_t d = temp_end - 1; d >= temp_start; d--) { //decode from end (LSB) to start (MSB)
                    if (IsB64(decode_buffer[d])) { //if B64 value
                      temp_decode = B64Lookup(decode_buffer[d]); //decode lead number
                      // Serial.print("Decode value to: "); Serial.println(temp_decode);
                      temp_decode = temp_decode << (6 * temp_character);
                      temp_character++;
                      serial_small_value |= temp_decode; //add value to lead number
                    }
                    else { //if any non B64 char found, go to error
                      temp_read_error = 1;
                      //Serial.print("Error");
                    }
                  }
                  serial_small_value = serial_small_value * temp_signed; //add sign to value
                  //Serial.print("Decoded SV: "); Serial.println(serial_small_value);
                  temp_start = e + 1; //set new start
                  temp_character = 0;
                  break; //break from current for loop
                }
              }
            }

            //look for fourth break
            if (temp_read_error == 0 && temp_ended == 0) { //check if no error and not ended
              //Serial.println("Start looking for small value");
              for (uint16_t e = temp_start; e <= end_character_pos; e++) { //look for last break, command
                if (IsEndCharacter(decode_buffer[e])) {
                  if (IsEndCharacter(decode_buffer[e]) == 2) {
                    temp_ended = 1;  //set ended to 1 if this was a line end, not a space
                    //Serial.println("Ended");
                  }
                  //Serial.print("End char in Raw: "); Serial.println(e);
                  temp_end = e; //note the position where the line ends
                  for (int16_t d = temp_end - 1; d >= temp_start; d--) { //decode from end (LSB) to start (MSB)
                    if (IsB64(decode_buffer[d])) { //if B64 value
                      temp_decode = B64Lookup(decode_buffer[d]); //decode lead number
                      serial_raw[temp_character] = temp_decode;
                      temp_character++;
                    }
                    else { //if any non B64 char found, go to error
                      temp_read_error = 1;
                      //Serial.print("Error");
                    }
                  }
                  //While block is disabled because now at the start the value is reset to 0 ------
                  /*while (temp_character < 50){ //fill the remainder of the buffer with 0
                    serial_raw[temp_character] = 0;
                    temp_character++;
                  }*/
                  //Serial.print("Decoded Raw: ");
                  //for (uint8_t r = 0; r < 50; r++){
                  //  Serial.print(serial_raw[r]); Serial.print(", ");
                  //}
                  //Serial.println("");
                  break; //break from current for loop
                }
              }
            }

            //remove read bytes from decode buffer
            //Serial.println("Removing read lines");
            int16_t temp_reset = serial_available;
            for (uint16_t r = 0; r < max_read_length; r++) {
              if (temp_reset > 0) {
                decode_buffer[r] = decode_buffer[temp_reset];
                decode_buffer_pos--; //remove one pos from decode buffer
                temp_reset--; //remove one to read value
              }
              else {
                decode_buffer[r] = 0;
              }
            }
            //Serial.print("Decode buffer pos: "); Serial.println(decode_buffer_pos);

            if (temp_read_error == 0 && temp_ended == 1) { //if ended and no mistakes
              return 1;  //return a 1
            }
            else {
              return 0; //mistake somewhere along the way
            }
          } //end of end character found if
        } //end of spaceleft > 0
        else { //if there was no space left in buffer
          return -1;
        }
      } //end of serial available
      return 0; //no new data, return a 0
    }
    uint16_t GetBufferLeft(){ //gets how many characters are left in the buffer
      uint16_t temp_return = max_read_length - 1;
      temp_return -= decode_buffer_pos;
      return temp_return;
    }
    uint16_t GetLineNumber(){ //returns the last read line number
      return serial_line_number;
    }
    uint32_t GetCommand(){ //returns the last read command
      return serial_command;
    }
    int32_t GetSmallValue(){ //returns the last read small value
      return serial_small_value;
    }
    void GetRaw(uint8_t* temp_raw){ //takes an uint8_t[50] array and fills it with the last read value
      for (uint8_t r = 0; r < 50; r++){
        temp_raw[r] = serial_raw[r];
      }
    }
    uint8_t IsEndCharacter(char temp_input) { //checks if a char is an end character, 1 for block end, 2 for line end
      if (temp_input == 10) return 2; //new line
      if (temp_input == 13) return 2; //carriage return
      if (temp_input == 32) return 1; //space
      return 0;
    }
    uint8_t IsB10(char temp_input) {  //reads a character and determines if it is a valid base 10 character
      if (temp_input >= '0' && temp_input <= '9') return 1;
      if (temp_input == '-') return 1; //negative is accepted in base 10 mode
      return 0; //no match found
    }
    uint8_t IsB64(char temp_input) { //reads a character and determines if it is a valid base 64 character
      if (temp_input >= 'A' && temp_input <= 'Z') return 1;
      if (temp_input >= 'a' && temp_input <= 'z') return 1;
      if (temp_input >= '0' && temp_input <= '9') return 1;
      if (temp_input == '+') return 1;
      if (temp_input == '/') return 1;
      return 0; //no match found
    }
    int8_t B10Lookup(char c) { //returns the 0-9 from ascii
      if (c >= '0' && c <= '9') return c - 48; //0-9
      return -1; //-1
    }
    int8_t B64Lookup(char c) { //Convert a base 64 character to decimal, (returns -1 if no valid character is found)    
      if (c >= 'A' && c <= 'Z') return c - 'A'; //0-25
      if (c >= 'a' && c <= 'z') return c - 71; //26-51
      if (c >= '0' && c <= '9') return c + 4; //52-61
      if (c == '+') return 62; //62
      if (c == '/') return 63; //63
      return -1; //-1
    }
    char ToB64Lookup(uint8_t temp_input){
      if (temp_input >= 0 && temp_input <= 25) return temp_input + 'A'; //0-25
      if (temp_input >= 26 && temp_input <= 51) return temp_input + 71; //26-51
      if (temp_input >= 52 && temp_input <= 61) return temp_input - 4; //52-61
      if (temp_input == 62) return '+';
      if (temp_input == 63) return '/';
      return '&'; //return '&' if false, this will trigger code reading it to mark as mistake
    }
    void WriteValueToB64(int32_t temp_input){ //adds a value to the writebuffer in B64
      //Serial.println("Encoding to B64");
      //Serial.print("Number: "); Serial.println(temp_input);
      uint8_t temp_sign = 0;
      uint8_t temp_6bit;
      char temp_reverse_array[5];
      if (temp_input < 0){ //if value is negative
        temp_sign = 1; //set negative flag to 1
        temp_input = abs(temp_input); //make the input positive
      }
      for (int8_t a = 0; a < 5; a++){ //loop through the input to convert it
        //Serial.print("Encoding: "); Serial.println(a);
        temp_6bit = 0; //reset 6 bit value
        temp_6bit = temp_input & 63; //read first 6 bits
        //Serial.print("Turns to: "); Serial.println(temp_6bit);
        temp_reverse_array[a] = ToB64Lookup(temp_6bit);
        temp_input = temp_input >> 6; //shift bits
      }
      //add reverse array to output buffer
      if (temp_sign == 1){ //add negative to value
        write_buffer[write_characters] = '-';
        write_characters++;
      }
      uint8_t temp_started = 0;
      for (int8_t a = 4; a >= 0; a--){
        if (temp_started == 0){ //until a value is seen all zero's ('A''s) are ignored
          if (temp_reverse_array[a] != 'A') temp_started = 1; //start when first non 0 is found
          if (a == 0) temp_started = 1; //also start when the first character is reached
        }
        if (temp_started == 1){ //when started, write to output buffer
          write_buffer[write_characters] = temp_reverse_array[a];
          write_characters++;
        }
      }
    }
    void WriteTestArrayToB64(uint8_t temp_input[]){ //adds a test array to the writebuffer
      uint16_t temp_n = 0; //nozzle counter
      uint8_t temp_6bit; //6 bit decode value
      char temp_output[50]; //50 char output array
      for (uint8_t r = 0; r < 50; r++){
        temp_output[r] = ' '; //reset array values
      }
      //decode 300 array to B64
      for (uint8_t B = 0; B < 50; B++){
        temp_6bit = 0; //reset 6 bit value
        for (uint8_t b = 0; b < 6; b++){
          //Serial.print("Decoding: "); Serial.print(temp_n); Serial.print(", Value: "); Serial.println(temp_input[temp_n]);
          temp_6bit |= (temp_input[temp_n] & 1) << b;
          temp_n++;
        }
        temp_output[B] = ToB64Lookup(temp_6bit); //write 6 bit value to output
        //Serial.print("Encoding: "); Serial.print(temp_6bit); Serial.print(", Value: "); Serial.println(temp_output[B]);
      }

      //write B64 to output buffer
      for (int8_t B = 49; B >= 0; B--){
        write_buffer[write_characters] = temp_output[B];
        write_characters++;
      }
    }
    void SendResponse(){ //will write the line in the buffer and mark it to be sent ASAP
      write_buffer[write_characters] = '\n'; //add carriage return
      write_characters++;
      Serial.write(write_buffer, write_characters);
      Serial.send_now(); //send all in the buffer ASAP
      write_characters = 0;//reset write counter
    }
    void RespondTestPrinthead(){
      
    }
    void RespondTemperature(int32_t temp_temp){
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'G';
      write_buffer[1] = 'T';
      write_buffer[2] = 'P';
      write_buffer[3] = ':';
      WriteValueToB64(temp_temp); //convert temperature to 64 bit
      SendResponse(); //send temperature
    }
    void RespondEncoderPos(int32_t temp_pos){
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'G';
      write_buffer[1] = 'E';
      write_buffer[2] = 'P';
      write_buffer[3] = ':';
      WriteValueToB64(temp_pos); //convert temperature to 64 bit
      SendResponse(); //send temperature
    }
    void RespondBufferReadLeft(int32_t temp_left){
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'B';
      write_buffer[1] = 'R';
      write_buffer[2] = 'L';
      write_buffer[3] = ':';
      WriteValueToB64(temp_left); //convert temperature to 64 bit
      SendResponse(); //send left
    }
    void RespondBufferWriteLeft(int32_t temp_left){
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'B';
      write_buffer[1] = 'W';
      write_buffer[2] = 'L';
      write_buffer[3] = ':';
      WriteValueToB64(temp_left); //convert temperature to 64 bit
      SendResponse(); //send left
    }
    void RespondTestResults(uint8_t temp_mode, uint8_t temp_result[]){
      write_characters = 4; //set characters to value after adding response header
      write_buffer[0] = 'T';
      write_buffer[1] = 'H';
      write_buffer[2] = 'D';
      write_buffer[3] = ':';
      WriteTestArrayToB64(temp_result); //convert 1B array to 64 bit
      SendResponse(); //send left
    }
};



