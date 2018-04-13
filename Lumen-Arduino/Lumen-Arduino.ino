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

#include <util/atomic.h>
#include "Lumen-Arduino.h"
#include "LPFilter.h"
#include "HystRound.h"

// GLOBAL VARIABLES
uint32_t lastfilterruntime    = 0;          // ms
volatile uint32_t pulsetime   = 0;          // ms
volatile int16_t  pulsein     = 0;          // us

LPFilter   inputfilter;
HystRound  inputhysteretic;

/*----------------------------------------------------------------------------*/

///////////
// SETUP //
///////////

void setup() {
  // Set pins to correct modes
  pinMode(SIGNAL_PIN, INPUT );
  pinMode(LED_PIN,    OUTPUT);
  pinMode(TEMP_PIN,   INPUT );

  // Initialize PWM input reader
  initializePWMReader();

  // Initialize hardware timer1 for PWM output
  initializePWMOutput();

  // Initialize PWM input filter
  inputfilter = LPFilter(FILTER_DT, FILTER_TAU, 0);

  // Initialize input hysteresis rounder
  inputhysteretic = HystRound(0, HYST_FACTOR);
}


//////////
// LOOP //
//////////

void loop() {
  // Declare local variables for holding large volatile variables
  uint32_t lastpulsetime;
  int16_t  lastpulsein;

  // Copy 16-bit and larger volatile variables to local variables
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    lastpulsetime = pulsetime;
    lastpulsein   = pulsein;
  }

  // Make sure we're still receiving PWM inputs
  if ( (adjustedMillis() - lastpulsetime)/1000.0f > INPUT_TIMEOUT ) {
    // Reset last pulse time to now to keep from running every loop
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      pulsetime = adjustedMillis();
    }

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
  if ( (adjustedMillis() - lastfilterruntime)/1000.0f > FILTER_DT) {
    // Set next filter runtime
    lastfilterruntime = adjustedMillis();

    // Calculate output limit to limit temperature
    int maxinput = constrain((T_MAX - getTemp(TEMP_PIN))*T_KP, 0, N_STEPS-1);

    // Declare local variables
    float rawinput;

    // Reject signals that are too short (i.e. const. 0 V, noise)
    if ( lastpulsein >= PULSE_CUTOFF ) {
      // Discretize valid PWM signals to [0, maxinput]
      rawinput = constrain((float)(lastpulsein - PULSE_MIN)*(N_STEPS-1)
        /(PULSE_MAX -PULSE_MIN), 0, maxinput);
    } else {
      // Set input to 0 (off) if invalid
      rawinput = 0;
    }

    // Filter and apply hysteretic rounding to input
    uint8_t input = inputhysteretic.hystRound(inputfilter.step(rawinput));

    // Map input to brightness
    uint8_t brightness = expMap(input);

    // Set output PWM timers
    setBrightness(constrain(brightness, 0, OUTPUT_MAX));
  } // end run filters
} // end loop()


/*----------------------------------------------------------------------------*/

//////////////////////////
// PWM Output Functions //
//////////////////////////

/******************************************************************************
 * void initializePWMOutput()
 *
 * Sets timer1 registers for hardware PWM output
 ******************************************************************************/
void initializePWMOutput() {
  // Stop interrupts while changing timer settings
  cli();

  // Use 0xFF (255) as top
  bitClear(TCCR1, CTC1);

  // Enable PWM A
  bitSet  (TCCR1, PWM1A);

  // Set timer1 clock source to prescaler 4 (8M/4/256 = 7812.5 Hz)
  bitClear(TCCR1, CS13); // 0
  bitClear(TCCR1, CS12); // 0
  bitSet  (TCCR1, CS11); // 1
  bitSet  (TCCR1, CS10); // 1

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

/******************************************************************************
 * void setBrightness(uint8_t brightness)
 *
 * Sets the duty cycle (brightness) for the output pin (LED)
 ******************************************************************************/
void setBrightness(uint8_t brightness) {
  OCR1A = brightness;
}

///////////////////////////
// Input Timer Functions //
///////////////////////////

// Define global variables only used for input timer
uint32_t inputpulsestart   = 0xFFFF;

/******************************************************************************
 * void initializePWMReader()
 *
 * Sets pin change interrupt and timer0 prescaler registers for PWM reader
 ******************************************************************************/
void initializePWMReader() {
  // Enable pin change interrupts
  bitSet(GIMSK, PCIE);

  // Enable PCI for PWM input pin (PCINT0)
  bitSet(PCMSK, PCINT0);

  // Stop interrupts while changing timer settings
  cli();

  // Set timer0 prescaler to 8 (millis() and micros() will run 8x fast)
  bitClear(TCCR0B, CS02); // 0
  bitSet  (TCCR0B, CS01); // 1
  bitClear(TCCR0B, CS00); // 0

  // Done setting timers -> allow interrupts again
  sei();
}

/******************************************************************************
 * SIGNAL(PCINT0_vect)
 *
 * Pin change interrupt which measures the length of input PWM signals
 ******************************************************************************/
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
    pulsetime = adjustedMillis();
  }
}

/******************************************************************************
 * uint32_t adjustedMicros()
 *
 * Returns the time since this program started in microseconds, corrected for
 * increased timer0 speed
 ******************************************************************************/
uint32_t adjustedMicros() {
  return micros()/(64/TIM0_PRESCALE);
}

/******************************************************************************
 * uint32_t adjustedMillis()
 *
 * Returns the time since this program started in milliseconds, corrected for
 * increased timer0 speed
 ******************************************************************************/
uint32_t adjustedMillis() {
  return millis()/(64/TIM0_PRESCALE);
}

/////////////////////////////
// Miscellaneous Functions //
/////////////////////////////

/******************************************************************************
 * uint8_t expMap(float x)
 *
 * Maps input from [0, N_STEPS-1] to [0, OUTPUT_MAX] pseudo-exponentially
 ******************************************************************************/
uint8_t expMap(float x) {
  return round(EXP_MAP_A0 + EXP_MAP_A1*x + EXP_MAP_A2*x*x + EXP_MAP_A3*x*x*x
    + EXP_MAP_A4*x*x*x*x);
}

/******************************************************************************
 * float getTemp(uint8_t pin)
 *
 * Returns the temperature of an NTC thermistor on given ADC input
 ******************************************************************************/
float getTemp(uint8_t pin) {
  float adc = analogRead(pin);
  float r   = (TEMP_SENSE_R*adc)/(ADC_MAX-adc);

  return 1.0f/(1.0f/NTC_T0 + log(r/NTC_R0)/NTC_B) - CELSIUS_0;
}
