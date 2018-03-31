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

#include "Lumen-Arduino.h"
#include "LPFilter.h"

// GLOBAL VARIABLES
uint32_t lastpulsetime        = 0;          // ms
uint32_t updatefilterruntime  = 0;          // ms
volatile int16_t  pulsein     = 0;          // us
LPFilter outputfilter;


// Set up pin change interrupt for PWM reader
void initializePWMReader() {
  // Enable pin change interrupts
  bitSet(GIMSK, PCIE);

  // Enable PCI for PWM input pin (PCINT0)
  bitSet(PCMSK, PCINT0);

  // Set timer0 prescaler to 8 (millis() and micros() will run 8x fast)
  bitClear(TCCR0B, CS02); // 0
  bitSet  (TCCR0B, CS01); // 1
  bitClear(TCCR0B, CS00); // 0
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

// Exponentially map input to output
float expMap(float input, float imin, float imax, float omin, float omax) {
  float a = omin;
  float b = log(omax/omin)/(imax-imin);
  float c = imin;

  return a*exp(b*(input-c));
}

// Read temperature (Celsius)
float getTemp(uint8_t pin) {
  float adc = analogRead(pin);
  float r   = (TEMP_SENSE_R*adc)/(ADC_MAX-adc);

  return 1.0f/(1.0f/NTC_T0 + log(r/NTC_R0)/NTC_B) - CELSIUS_0;
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
}

void loop() {
  // Make sure we're still receiving PWM inputs
  if ( (adjustedMillis() - lastpulsetime)/1000.0f > INPUT_TIMEOUT ) {
    // Reset last pulse time to now to keep from running every loop
    lastpulsetime = adjustedMillis();

    // If it has been too long since the last input, check input PWM state
    if ( digitalRead(SIGNAL_PIN) == HIGH ) {
      // Turn LED on if the signal is still high (pulse is too long)
      pulsein = PULSE_MAX;
    } else {
      // Otherwise turn off the LED (no pulse for a while)
      pulsein = 0;
    }
  } // end pwm input check

  // Run filters at specified interval
  if ( adjustedMillis() > updatefilterruntime ) {
    // Set next filter runtime
    updatefilterruntime = adjustedMillis() + FILTER_DT*1000;

    // Calculate output limit to limit temperature
    float maxoutput = constrain((T_MAX - getTemp(TEMP_PIN))*T_KP, OUTPUT_MIN,
      OUTPUT_MAX);

    // Declare local variables
    float   rawbrightness;
    int16_t pulsewidth = pulsein;

    // Reject signals that are too short (i.e. const. 0 V, noise)
    if ( pulsewidth >= PULSE_MIN ) {
      // Map valid PWM signals to [0.0 to OUTPUT_MAX], clamp to [0, maxpwm]
      rawbrightness = constrain(expMap(pulsewidth, PULSE_MIN, PULSE_MAX,
        OUTPUT_MIN, OUTPUT_MAX), 0, maxoutput);
    } else {
      // Turn off LED if invalid
      rawbrightness = 0;
    }

    // Filter velocity
    uint8_t brightness = hystereticRound(outputfilter.step(rawbrightness));

    // Set output PWM timers
    if ( brightness >= OUTPUT_MIN ) {
      setBrightness(constrain(brightness, OUTPUT_MIN, OUTPUT_MAX));
    } else {
      // Turn off LED
      setBrightness(0);
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
    if ( inputpulsestart < adjustedMicros() ) {
      // Update pulse length
      pulsein = adjustedMicros() - inputpulsestart;
    }
    lastpulsetime = adjustedMillis();
  }
}

// Adjust for increased timer0 speed - microseconds
uint32_t adjustedMicros() {
  return micros()/(64/TIM0_PRESCALE);
}

// Adjust for increased timer0 speed - milliseconds
uint32_t adjustedMillis() {
  return millis()/(64/TIM0_PRESCALE);
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
