/* Blue Robotics Lumen LED Embedded Software
------------------------------------------------

Title: Lumen LED Light Embedded Microcontroller Software
Description: This file is used on the ATtiny45 microcontroller
on the Lumen LED Light and controls the dimming of the light. It can be
control by two methods, first is standard servo PWM pulse from
1100-1900 microseconds. The second is a logic high value on the signal
line. Additionally, a thermistor is used to sense temperature and automatically
dim the light if it is overheating.

-------------------------------
The MIT License (MIT)
Copyright (c) 2016 Blue Robotics Inc.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-------------------------------*/

#include "LPFilter.h"

// HARDWARE PIN DEFINITIONS
#define SIGNAL_PIN      0
#define LED_PIN         1
#define TEMP_PIN        A1                  // A1

// DIMMING CHARACTERISTICS
// Temp to start dimming the lights at
#define DIM_ADC         265                 // 265 for 85C board temperature
// Dimming gains
#define DIM_KP          5.0
#define DIM_KI          0.5

// OUTPUT LIMIT
#define PWM_MIN         1                   // 0-255
#define PWM_MAX         230                 // 0-255 - 230 for about 15W max

// SIGNAL CHARACTERISTICS
#define PULSE_FREQ      50                  // Hz
#define PULSE_PERIOD    1000000L/PULSE_FREQ // microseconds
#define PULSE_CUTOFF    500                 // microseconds
#define PULSE_MIN       1120                // microseconds
#define PULSE_MAX       1880                // microseconds
#define INPUT_TIMEOUT   0.050               // seconds

// INPUT FILTER CHARACTERISTICS
#define FILTER_DT       0.020f              // seconds
#define FILTER_TAU      1.000f              // seconds

// HYSTERETIC ROUNDING
#define HYST_FACTOR     0.8                 // 0.5: normal rounding

int16_t pwm;
float   error;
float   control;
float   dimI;

const float smoothAlpha = 0.02;

uint32_t lastpulsetime        = 0;
uint32_t updatefilterruntime  = 0;
int16_t  pulsein              = PULSE_MIN;
LPFilter outputfilter;
LPFilter tempfilter;


// Set up pin change interrupt for PWM reader
void initializePWMReader() {
  // Enable pin change interrupts
  bitSet(GIMSK, PCIE);

  // Enable PCI for PWM input pin (PCINT0)
  bitSet(PCMSK, PCINT0);
}

// Set registers for hardware PWM output
void initializePWMOutput() {
  // Stop interrupts while changing timer settings
  cli();

  // Use 0xFF (255) as top
  bitClear(TCCR1, CTC1);

  // Enable PWM A
  bitSet  (TCCR1, PWM1A);

  // Set timer1 clock source to prescaler 4 (8M/4/256 = 7812.5 Hz)
  bitSet  (TCCR1, CS10);
  bitSet  (TCCR1, CS11);
  bitClear(TCCR1, CS12);
  bitClear(TCCR1, CS13);

  // Set non-inverting PWM mode, disable !OC1A (pin 0)
  bitClear(TCCR1, COM1A0);
  bitSet  (TCCR1, COM1A1);

  // Disable OC1B and !OC1B (pins 3 & 4)
  bitClear(GTCCR, COM1B0);
  bitClear(GTCCR, COM1B1);

  // Initialize to off
  setBrightness(0);

  // Done setting timers -> allow interrupts again
  sei();
}

// Set brightness of LED
void setBrightness(uint8_t brightness) {
  OCR1A = brightness;
}

void setup() {
  // Set pins to correct modes
  pinMode(SIGNAL_PIN, INPUT );
  pinMode(LED_PIN,    OUTPUT);
  pinMode(TEMP_PIN,   INPUT );

  // Initialize PWM input reader
  initializePWMReader();

  // Initialize hardware timer1 for PWM output
  initializePWMOutput();

  // Initialize PWM output filter
  outputfilter = LPFilter(FILTER_DT, FILTER_TAU, 0);

  // Initialize temperature output filter
  tempfilter   = LPFilter(FILTER_DT, FILTER_TAU, 0);
}

void loop() {
  // Make sure we're still receiving PWM inputs
  if ( (millis() - lastpulsetime)/1000.0f > INPUT_TIMEOUT ) {
    // Reset last pulse time to now to keep from running every loop
    lastpulsetime = millis();

    // If it has been too long since the last input, check input PWM state
    if ( digitalRead(SIGNAL_PIN) == HIGH ) {
      // Turn LED on if the signal is still high (pulse is too long)
      pulsein = PULSE_MAX;
    } else {
      // Otherwise turn off the LED (no pulse for a while)
      pulsein = PULSE_MIN;
    }
  } // end pwm input check

  // Run filters at specified interval
  if ( millis() > updatefilterruntime ) {
    // Set next filter runtime
    updatefilterruntime = millis() + FILTER_DT*1000;

    // Declare local variables
    uint8_t rawbrightness;
    uint8_t brightness;
    uint8_t maxpwm;

    // PID controller to control max temperature limit. Not a very linear
    // approach but it works well for this. This will basically control the 
    // limit to maintain DIM_ADC temperature or better.
    error = DIM_ADC - tempfilter.step(analogRead(TEMP_PIN));

    dimI += error*0.005;
    dimI  = constrain(dimI, -255/abs(DIM_KI), 255/abs(DIM_KI)); // Limit I-term spooling

    // Build control value
    control = error*DIM_KP + dimI*DIM_KI;

    // Apply control value to max PWM
    maxpwm = constrain(255 - control, PWM_MIN, PWM_MAX);

    // Reject signals that are way off (i.e. const. 0 V, noise)
    if ( pulsein >= PULSE_CUTOFF ) {
      // Map valid PWM signals to [0.0 to PWM_MAX], clamp to [0, maxpwm]
      rawbrightness = constrain(map(pulsein, PULSE_MIN, PULSE_MAX, 0, PWM_MAX), 0, maxpwm);
    } else {
      // Turn off LED if invalid
      rawbrightness = 0;
    }

    // Filter velocity
    brightness = hystereticRound(outputfilter.step(rawbrightness));

    // Set output PWM timers
    if ( brightness >= PWM_MIN ) {
      setBrightness(constrain(brightness, PWM_MIN, PWM_MAX));
    } else {
      // Turn off LED
      setBrightness(0);

      // Reset I after turning LED off
      dimI = 0;
    } // end set output PWM timers
  } // end run filters
} // end loop()

///////////////////////////
// Input Timer Functions //
///////////////////////////

// Define global variables only used for input timer
namespace {
  uint32_t inputpulsestart   = 0xFFFF;
}

// Read PWM input
SIGNAL(PCINT0_vect) {
  if ( digitalRead(SIGNAL_PIN) ) {
    // If this was a rising edge
    // Save the start time
    inputpulsestart = adjustedMicros();
  } else {
    // If this was a falling edge
    // Ignore inputs that cross a rollover
    if ( inputpulsestart < micros() ) {
      // Update pulse length
      pulsein = micros() - inputpulsestart;
    }
    lastpulsetime = millis();
  }
}

}


///////////////////////////
// Round with Hysteresis //
///////////////////////////
float oldvalue = 0.0f;

int hystereticRound(float newvalue) {
  if ( abs(oldvalue - newvalue) > HYST_FACTOR ) {
    oldvalue = round(newvalue);
  }
  return (int) oldvalue;
}
