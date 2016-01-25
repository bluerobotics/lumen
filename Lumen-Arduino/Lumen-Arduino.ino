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
Copyright (c) 2015 Blue Robotics Inc.
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
#define TEMP_PIN 2

// DIMMING CHARACTERISTICS
// Temp to start dimming the lights at
#define DIM_ADC 355 // 355 for 70C
// Temp to shut off
#define MIN_ADC 235 // 235 for 90C
// Steinhart slope near values of interest
#define SLOPE (-7) // ADC steps per degree (7.5)
// Dimming gain
#define DIM_GAIN -10 // 255ths of max power per degree of overtemp

// OUTPUT LIMIT
#define MAX_LED 255

// SIGNAL CHARACTERISTICS
#define PULSE_MIN 1100 // microseconds
#define PULSE_MAX 1900 // microseconds
#define PERIOD_MAX 100000ul // microseconds

int16_t signal = 1100;
int16_t pwm;
int16_t tempAdjust;

void setup() {
  pinMode(SIGNAL_PIN,INPUT);
  pinMode(LED_PIN,OUTPUT);  
}

void loop() {
  pulseIn(SIGNAL_PIN,HIGH,PERIOD_MAX);

  if ( signal == 0 ) {
    if ( digitalRead(SIGNAL_PIN) == HIGH ) {
      pwm = 0xFF;
    } else {
      pwm = 0;
    }
  } else if ( signal < PULSE_MIN ) {
    pwm = 0;
  } else if ( signal > PULSE_MAX ) {
    pwm = 0xFF;
  } else {
    pwm = map(signal,PULSE_MIN,PULSE_MAX,0,0xFF);
  }

  if ( signal > 0 ) {
    float tempADC = analogRead(TEMP_PIN);
  
    if ( tempADC < MIN_ADC && signal ) {
      pwm = 10; // Minimal level so you can tell it's turned on.
    } else if ( tempADC <= DIM_ADC ) {
      tempAdjust = (DIM_ADC-tempADC)*DIM_GAIN/SLOPE;  
    } else {
      tempAdjust = 0;
    }
  
    tempAdjust = constrain(tempAdjust,-pwm+10,0);
  } else {
    tempAdjust = 0;
  }

  if ( pwm > 0 ) {
    analogWrite(LED_PIN, constrain(pwm+tempAdjust,0,MAX_LED));
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  delay(50);
}
