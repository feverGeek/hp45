/*
   The position tab (not class, got issues) handles the coordinates of the printhead using a linear optical encoder strip.

   Todo:
   -Change test conditions to also include a timebase (of say 100us or 1ms) to make 0 a posible velocity.
   Right now the only recalculation is when there was a change, and this only happens with movement
   -Velocity output seems fairly eratic (varying +/-5mm/s @100mm/s with a simple serial test)
   See if there is mathematical reasons for this eratic time behavior, else add smoothing
*/

#include "Arduino.h"

#define encoderResolution 600.0 //the lines per inch (150) x4 for encoder type (quadrature) as a float
#define rowGap 4050 //the distance in microns between odd and even row in long

//#define ENCODER_DO_NOT_USE_INTERRUPTS //interrupts risk firing while printhead is triggering, off for now
#include "Encoder.h"

#define positionStepCount 50 //how many steps are used to calculate velocity
#define positionStepTimeout 100000 //how many microseconds no step has to be seen to set the velocity to 0

Encoder positionEncoder(31, 30); //make an encoder instance

int32_t positionRaw, positionHistory; //the current and history pulse position
int32_t positionBaseMicrons, positionBaseMicronsHistory; //the reference micron position and history
int32_t positionRowMicrons[2]; //the micron positions of odd and even
int32_t positionVelocity; //the current velocity of the printhead
uint32_t positionLastStepTime, positionLastCalcTime; //the value that holds the time of the last encoder pulse in microseconds and when the last velocity measurement was done
int32_t positionVelocityLastMicrons; //where the last recalculation of velocity was
int8_t positionDirection; //1 or -1, tells the direction of the printhead
uint16_t positionVelocityUpdateCounter; //counts up every encoder pulse, when reached StepCount, converts to velocity

void PositionUpdate() {
  positionRaw = positionEncoder.read();
  if (positionHistory != positionRaw) { //if history and raw do not match, recalculate positions
    uint32_t temp_time = micros(); //write down time of change

    //get micron position
    float temp_calc = float(positionRaw);
    temp_calc *= 25400.0; //by microns in an inch
    temp_calc /= encoderResolution; //divide by lines per inch
    positionBaseMicrons = long(temp_calc);

    //get odd and even micron position
    temp_calc = float(rowGap) / 2.0; //divide by 2 to give the offset from 0
    temp_calc += float(positionBaseMicrons); //add to current position
    positionRowMicrons[1] = long(temp_calc); //even (0) is on the positive side
    positionRowMicrons[0] = positionRowMicrons[1] - rowGap; //odd (1) is on the negative side, subtract row gap

    //calculate velocity (new pos - old pos)/ time it took is microns per microsecond
    positionVelocityUpdateCounter++; //add one encoder pulse to counter
    if (positionVelocityUpdateCounter >= positionStepCount) { //if number of pulses is reached
      positionVelocityUpdateCounter = 0;

      temp_calc = float(positionBaseMicrons);
      temp_calc -= float(positionBaseMicronsHistory); //get delta distance
      float temp_calc2 = float(temp_time);
      temp_calc2 -= float(positionLastCalcTime);
      temp_calc = temp_calc / temp_calc2; //get microns per microsecond (is mm/millisecond because metric)
      temp_calc *= 1000.0; //multiply by 1000 to get millimeters per second
      positionVelocity = long(temp_calc);
      positionDirection = constrain(positionVelocity, -1, 1); //set direction
      positionBaseMicronsHistory = positionBaseMicrons; //set new micron history
      positionLastCalcTime = micros(); //reset last velocity calculation time
    }

    positionLastStepTime = temp_time; //set new time
    positionHistory = positionRaw; //set new history
  }

  if (micros() - positionLastCalcTime > positionStepTimeout) { //velocity timeout conditions
      positionVelocity = 0;
    }
}

int32_t PositionGetBasePositionRaw() { //returns the position of the base in pulses
  PositionUpdate();
  return positionRaw;
}

int32_t PositionGetBasePositionMicrons() { //returns the position of the base in microns
  return positionBaseMicrons;
}

void PositionSetBasePositionMicrons(int32_t temp_position){ //sets the new encoder pulse position
  int32_t temp_raw = map(temp_position, 0, 25400, 0, long(encoderResolution)); //recalculate encoder position from microns to pulses
  positionEncoder.write(temp_raw); //set new position
}

int32_t PositionGetRowPositionMicrons(uint8_t temp_side) { //returns the position of the given side
  temp_side = constrain(temp_side, 0, 1);
  return positionRowMicrons[temp_side];
}

int32_t PositionGetVelocity() { //returns the current velocity in millimeters per second
  return positionVelocity;
}

int8_t PositionGetDirection(){ //returns the current direction
  return positionDirection;
}


