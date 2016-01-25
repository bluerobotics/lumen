%% Thermistor curve for Lumen Subsea Light

clear all;
clc;

% THERMISTOR SPECIFICATIONS
% resistance at 25 degrees C
THERMISTORNOMINAL = 10000      
% temp. for nominal resistance (almost always 25 C)
TEMPERATURENOMINAL = 25   
% The beta coefficient of the thermistor (usually 3000-4000)
BCOEFFICIENT = 3900
% the value of the 'other' resistor
SERIESRESISTOR = 3300

adc = [1:1022];

resistance = SERIESRESISTOR./(1023./adc-1);

steinhart = resistance ./ THERMISTORNOMINAL;
steinhart = log(steinhart);
steinhart = steinhart./BCOEFFICIENT;
steinhart = steinhart + 1./(TEMPERATURENOMINAL+273.15);
steinhart = 1./steinhart;
steinhart = steinhart - 273.15;

plot(steinhart,adc);
xlabel('Temperature (deg C)');
ylabel('ADC Reading (10 bit)');
axis([-25 150 0 1023]);
grid on;
hold on;

% 60C: 443
% 70C: 355
% 80C: 293
% 90C: 235
% 100C: 184
% Slope: -7.5 steps per degree