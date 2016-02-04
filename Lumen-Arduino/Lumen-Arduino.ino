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
#define DIM_ADC 293 // 293 for 80C
// Dimming gains
#define DIM_KP 0.30
#define DIM_KI 0.30

// OUTPUT LIMIT
#define PWM_MIN 10 // 0-255
#define PWM_MAX 200 // 0-255 - 180 for about 12W max

// SIGNAL CHARACTERISTICS
#define PULSE_MIN 1100 // microseconds
#define PULSE_MAX 1900 // microseconds
#define PERIOD_MAX 100000ul // microseconds

int16_t signal = 1100;
int16_t pwm;
float error;
float control;
float dimI;
float maxPWM = 255;

const float smoothAlpha = 0.05;

void setup() {
  pinMode(SIGNAL_PIN,INPUT);
  pinMode(LED_PIN,OUTPUT); 
}

void loop() {
  // Read in PWM signal with low-pass filter for smoothing
  signal = (1-smoothAlpha)*signal + smoothAlpha*pulseIn(SIGNAL_PIN,HIGH,PERIOD_MAX);

  // Determine appropriate output signal. 
  if ( signal == 0 ) {
    // Allow light to be turned on by tying the signal pin high.
    if ( digitalRead(SIGNAL_PIN) == HIGH ) {
      pwm = PWM_MAX;
    } else {
      pwm = 0;
    }
  } else if ( signal < PULSE_MIN ) {
    pwm = 0;
  } else if ( signal > PULSE_MAX ) {
    pwm = 0xFF;
  } else {
    pwm = map(signal,PULSE_MIN,PULSE_MAX,0,PWM_MAX);
  }

  // PID controller to control max temperature limit. Not a very linear
  // approach but it works well for this. This will basically control the 
  // limit to maintain DIM_ADC temperature or better.
  error = DIM_ADC-analogRead(TEMP_PIN);

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

  delay(3);
}