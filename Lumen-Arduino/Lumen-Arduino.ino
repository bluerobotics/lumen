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

// HARDWARE PIN DEFINITIONS
#define SIGNAL_PIN 0
#define LED_PIN 1
#define TEMP_PIN A1

// DIMMING CHARACTERISTICS
// Temp to start dimming the lights at
#define DIM_ADC 265 // 265 for 85C board temperature
// Dimming gains
#define DIM_KP 5.0
#define DIM_KI 0.5

// OUTPUT LIMIT
#define PWM_MIN 20 // 0-255
#define PWM_MAX 230 // 0-255 - 230 for about 15W max

// SIGNAL CHARACTERISTICS
#define PULSE_MIN 1120 // microseconds
#define PULSE_MAX 1880 // microseconds
#define PERIOD_MAX 400000ul // microseconds

float signal = 1100;
int16_t pwm;
float adc;
float error;
float control;
float dimI;
float maxPWM = 255;

const float smoothAlpha = 0.02;

// This function is needed as a consequence of the PixHawk having 3.3V
// PWM logic as well as noise that gets into the signal from motors. 
// It listens for a pulse and ignore noise within that pulse, even if 
// the noise drops to a low logic level momentarily.
int16_t pulseInNoiseReduced(uint8_t pin,uint32_t maxPeriod) {
  uint32_t timeoutCounter = micros();
  uint32_t risingEdge;
  uint32_t fallingEdge;

  // If we're in the middle of a pulse, wait till it's over
  if ( digitalRead(pin) == HIGH ) {
    delayMicroseconds(30000);
  }

  // Wait for the rising edge
  while ( micros() - timeoutCounter < maxPeriod ) {
    if ( digitalRead(pin) == HIGH ) {
      risingEdge = micros();
      fallingEdge = risingEdge;
      break;
    }
  }

  // Push back falling edge until it's been low for 50 µs
  while ( micros() - timeoutCounter < maxPeriod ) {
    if ( digitalRead(pin) == HIGH ) {
      fallingEdge = micros();
    }

    if ( micros() - fallingEdge > 400 ) {
      return (fallingEdge - risingEdge)/8;
    }
  }

  return 0;
}

void setup() {
  pinMode(SIGNAL_PIN,INPUT);
  pinMode(LED_PIN,OUTPUT); 

  // Setup up PWM on output pin: Set prescalar to 8 for 8M/8/256 = 3906 Hz
  // The default analogWrite frequency can mess with cameras.
  TCCR0B = _BV(CS01); // Set prescalar to 8 for 8M/8/256 = 3906 Hz
}

void loop() {
  // Read in PWM signal with low-pass filter for smoothing
  signal = (1-smoothAlpha)*signal + smoothAlpha*pulseInNoiseReduced(SIGNAL_PIN,PERIOD_MAX);

  // Determine appropriate output signal. 
  if ( signal <= 500 ) {
    // Allow light to be turned on by tying the signal pin high.
    if ( digitalRead(SIGNAL_PIN) == HIGH ) {
      pwm = PWM_MAX;
    } else {
      pwm = 0;
    }
  } else if ( signal < PULSE_MIN ) {
    pwm = 0;
  } else if ( signal > PULSE_MAX ) {
    pwm = PWM_MAX;
  } else {
    pwm = map(signal,PULSE_MIN,PULSE_MAX,0,PWM_MAX);
  }

  // PID controller to control max temperature limit. Not a very linear
  // approach but it works well for this. This will basically control the 
  // limit to maintain DIM_ADC temperature or better.
  adc = adc*(1-smoothAlpha) + smoothAlpha*analogRead(TEMP_PIN);
  error = DIM_ADC-adc;

  dimI += error*0.005;
  dimI = constrain(dimI,-255/abs(DIM_KI),255/abs(DIM_KI)); // Limit I-term spooling

  // Reset I after turning LED off
  if ( pwm == 0 ) {
    dimI = 0;
  }

  // Build control value
  control = error*DIM_KP + dimI*DIM_KI;

  // Apply control value to max PWM
  maxPWM = constrain(255 - control,PWM_MIN,PWM_MAX);

  // Output PWM to LED driver
  if ( pwm > PWM_MIN ) {
    analogWrite(LED_PIN, constrain(pwm,PWM_MIN,maxPWM));
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  delay(1);
}
