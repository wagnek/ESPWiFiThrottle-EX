#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder {
  private:
    // State variables for rotary encoder.
    int counter = 0;
    int currentStateCLK;
    int lastStateCLK;

    // Encoder pins.
    byte clk;
    byte dt;
    byte sw;

  public:
    Encoder(byte clk, byte dt, byte sw);

    void init();

    void loop();

    int read();
    void write(int counter);
    
};

#endif
