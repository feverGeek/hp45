
//the generate tables function is used to generate, well, new tables. It takes the raw printhead tables, a conversion table that takes the raw values and alters them
//and the prints the new table over serial. All you need to do is correct the conversion table, run the function, and copy, paste the new table in the place of the old.
void GenerateNewRawTables() {
  //raw printhead locations
  const uint8_t temp_raw_nozzle_table_address[300] = //the table that returns the address for a given nozzle (do not alter)
  {
    0, 8, 15, 1, 8, 16, 1, 9, 16, 2,
    9, 17, 2, 10, 17, 3, 10, 18, 3, 11,
    18, 4, 11, 19, 4, 12, 19, 5, 12, 20,
    5, 13, 20, 6, 13, 21, 16, 14, 21, 7,
    14, 0, 7, 12, 0, 8, 15, 1, 8, 16,
    1, 9, 16, 2, 9, 17, 2, 10, 17, 3,
    10, 18, 3, 11, 18, 4, 11, 19, 4, 12,
    19, 5, 12, 20, 5, 13, 20, 6, 13, 21,
    6, 14, 21, 7, 14, 0, 7, 15, 0, 8,
    15, 1, 8, 16, 1, 9, 16, 2, 9, 17,
    2, 10, 17, 3, 10, 18, 3, 11, 18, 4,
    11, 19, 4, 12, 19, 5, 12, 20, 5, 13,
    20, 6, 13, 21, 6, 14, 21, 7, 14, 0,
    7, 15, 0, 8, 15, 1, 8, 16, 1, 9,
    16, 2, 9, 17, 2, 10, 17, 3, 10, 18,
    3, 11, 18, 4, 11, 19, 4, 12, 19, 5,
    12, 20, 5, 13, 20, 6, 13, 21, 6, 14,
    0, 7, 15, 0, 7, 15, 0, 8, 15, 1,
    8, 16, 1, 9, 18, 2, 9, 17, 2, 10,
    17, 3, 10, 18, 3, 11, 18, 4, 11, 19,
    4, 12, 19, 5, 12, 20, 5, 13, 20, 6,
    13, 21, 6, 14, 21, 7, 14, 0, 7, 15,
    0, 8, 15, 1, 8, 16, 1, 9, 16, 2,
    9, 17, 2, 10, 17, 3, 10, 18, 3, 11,
    18, 4, 11, 21, 4, 12, 19, 5, 12, 20,
    5, 13, 20, 6, 13, 21, 6, 14, 21, 7,
    14, 0, 7, 15, 0, 8, 15, 1, 8, 16,
    1, 9, 16, 2, 9, 17, 2, 10, 17, 3,
    10, 18, 3, 11, 18, 4, 11, 19, 4, 12,
    19, 5, 12, 20, 5, 13, 20, 6, 13, 20
  };

  const uint8_t temp_raw_nozzle_table_primitive[300] = //the table that returns the primitive for a given nozzle (do not alter)
  {
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
    2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
    2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
    2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
    2, 3, 2, 3, 2, 3, 2, 3, 2, 3,
    2, 3, 2, 3, 4, 5, 4, 5, 4, 5,
    4, 5, 4, 5, 4, 5, 4, 5, 4, 5,
    4, 5, 4, 5, 4, 5, 4, 5, 4, 5,
    4, 5, 4, 5, 4, 5, 4, 5, 4, 5,
    4, 5, 4, 5, 4, 5, 4, 5, 6, 7,
    6, 7, 6, 7, 6, 7, 6, 7, 6, 7,
    6, 7, 6, 7, 6, 7, 6, 7, 6, 7,
    6, 7, 6, 7, 6, 7, 6, 7, 6, 7,
    6, 7, 6, 7, 6, 7, 6, 7, 6, 7,
    6, 7, 8, 9, 8, 9, 8, 9, 8, 9,
    8, 9, 8, 9, 8, 9, 8, 9, 8, 9,
    8, 9, 8, 9, 8, 9, 8, 9, 8, 9,
    8, 9, 8, 9, 8, 9, 8, 9, 8, 9,
    8, 9, 8, 9, 8, 9, 10, 11, 10, 11,
    10, 11, 10, 11, 10, 11, 10, 11, 10, 11,
    10, 11, 10, 11, 10, 11, 10, 11, 10, 11,
    10, 11, 10, 11, 10, 11, 10, 11, 10, 11,
    10, 11, 10, 11, 10, 11, 10, 11, 10, 11,
    12, 13, 12, 13, 12, 13, 12, 13, 12, 13,
    12, 13, 12, 13, 12, 13, 12, 13, 12, 13,
    12, 13, 12, 13, 12, 13, 12, 13, 12, 13,
    12, 13, 12, 13, 12, 13, 12, 13, 12, 13
  };

  //conversion values --------------------------------------------------------------------------------------------------------------------------------
  //these are alteres. The given number is the controller pin the given printhead pin is on, in order from P1 to P14, and A1 to A22
  int8_t temp_primitive_conversion[14] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
  uint8_t temp_address_conversion[22] = {21, 19, 14, 18, 17, 13, 20, 9, 16, 8, 4, 12, 1, 15, 0, 5, 10, 6, 7, 2, 11, 3};

  //primitive
  Serial.println("New Primitive Array");
  for (uint16_t n = 0; n < 300; n++) { //walk through all values
    uint8_t temp_val = temp_raw_nozzle_table_primitive[n]; //take the current value
    temp_val = temp_primitive_conversion[temp_val]; //convert it using the conversion table
    Serial.print(temp_val); //print new value
    if (n != 299) { //don't print the last comma
      Serial.print(","); //comma
    }
    if (n % 10 == 9) { //new line after 10 positions
      Serial.println(""); //<-- new line
    }
  } //<- curly bracket
  // <- Blank space :)
  //address
  Serial.println("");
  Serial.println("New Address Array");
  for (uint16_t n = 0; n < 300; n++) { //walk through all values
    uint8_t temp_val = temp_raw_nozzle_table_address[n]; //take the current value
    temp_val = temp_address_conversion[temp_val]; //convert it using the conversion table
    Serial.print(temp_val); //print new value
    if (n != 299) { //don't print the last comma
      Serial.print(","); //comma
    }
    if (n % 10 == 9) { //new line after 10 positions
      Serial.println(""); //<-- new line
    }
  }
}


