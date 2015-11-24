#*Lumen* Subsea Lights

The *Lumen* Subsea Light is a underwater, pressure-rated LED light capable of outputting up to 1080 lumens. 

The schematic and board layout are designed in EagleCAD.

#*Lumen* Subsea Light Hardware

##Description

##Features 

* 1080 lm output
* Onboard microcontroller for programmable behavior
* Standard servo PWM input signal
* Optional I2C input signal
* Daisy-chainable with PWM or I2C signal
* Thermistor temperature feedback and overheating protection
* 6-48V input range
* 0-3A LED output

##Configuration

##Electrical Ratings

| Value                              | Minimum | Nominal | Maximum | Unit    |
|-----------------------------------:|:-------:|:-------:|:-------:|:--------|
| Property                           | 7       | 12      | 26      | V       |

##Example Setups

Examples TBD

##Physical Specifications

Dimensions, screw holes, wire gauges, etc.

##License

The Lumen Subsea Light Hardware Design is released under the MIT License.

##Revision History

0.0 - Under development

#Design Notes

##Components

* A6211 LED Driver ($0.57 @ 500)
* ATTiny45 microcontroller ($0.79 @ 500)
* LED: MKRAWT-00-0000-0B00J2051 ($6.69 @ 500)
	* 6200K Temp
	* 1080 lm @ 1.4A
	* 1650 lm @ 2.5A
* MC7805 LDO ($0.30 @ 500)
	* Expected max current: 50mA (1.6W heat loss @ 37V input)
	* Voltage divider to support 48V in
		* R1 = 100, R2 = 185 - 0.65 voltage divider (6.5V @ 10V in, 32.5V @ 50V in)
* 10K Thermistor for temperature sensing

##Physical

* 1.0" x 1.5" with breakaway 1" octagonal LED board
* Right angle headers to hold LED board
* Solder pads for primary cable
* Screw terminal for daisy chain
* AVR-ISP connector for programming