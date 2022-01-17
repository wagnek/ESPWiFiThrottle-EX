#include "Encoder.h"

Encoder::Encoder( byte clk, byte dt, byte sw ) {
  this->clk = clk;
  this->dt = dt;
  this->sw = sw;
  init();
}

void Encoder::init() {
  // Pins for the rotary encoder.
  pinMode(clk, INPUT);
  pinMode(dt, INPUT);
  pinMode(sw, INPUT_PULLUP);

  // Read the initial state of CLK
  lastStateCLK = digitalRead(clk);
}

void Encoder::loop() {
  // Handled the rotary encoder.
  // Read the current state of CLK
  currentStateCLK = digitalRead(clk);

  // If last and current state of CLK are different, then pulse occurred
  // React to only 1 state change to avoid double count
  if (currentStateCLK != lastStateCLK  && currentStateCLK == 1) {

    // If the DT state is different than the CLK state then
    // the encoder is rotating CCW so decrement
    if (digitalRead(dt) != currentStateCLK) {
      counter ++;
    } else {
      // Encoder is rotating CW so increment
      counter --;
    }
  }

  // Remember last CLK state
  lastStateCLK = currentStateCLK;
}

int Encoder::read() {
  return this->counter;
}

void Encoder::write( int counter ) {
  this->counter = counter;
}

  
