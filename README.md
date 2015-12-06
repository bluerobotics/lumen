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

##Requirements:

- 1000+ lumen output
- 9-48V input power
- 0-3A LED output
- Controllable with standard servo PWM pulse (1100-1900 µs @ 50-400 Hz) or on/off with 5V input signal
- Daisy-chainable so a single power and PWM signal can be used for 2-4 lights
- Exposed pads for I2C control for potential future use
- Thermistor and automatic overheating protection/dimming

##Tentative Components:

- A6211 LED Driver
- ATTiny45 Microcontroller (for servo PWM input, LED PWM output, and temperature control)
- LED: Cree MKRAWT-00-0000-0B00J2051 (1080 lm @ 8.4W, 1728 lm @ 15W)
- Low cost LDO such as MC7805 with voltage divider to support 48V operation (switching supply is okay too if cost is good - we only need about 10 mA for the µC)
- 10K thermistor for temperature sensing

##Enclosure:

- Aluminum case with tube, clear front end-cap, and rear flange cap
- Two cable penetrators at the rear connect to input cable and optional output cable for daisy-chaining (one will be a solid penetrator when not daisy-chaining)
- 25mm ID, adjustable length, preferably about 40mm L
- Front of LED PCB pressed against aluminum with thermal compound
- Front glass is acrylic or optical nylon
- Enclosure is air-filled and sealed from water
- See attached images

##PCB:

- A rectangular PCB will have all components except LED and thermistor
- LED and thermistor will be on a separate octagonal PCB perpendicular to rectangular PCB
- Tabs at back of rectangular PCB will align it with the rear flange-cap
- The two PCBs should be rigidly attached with header pins or other right angle connector
- Screw or solder terminals at the rear will connect to input cable and optional output cable
- The two PCBs should be in the same board file with a V-score or tab routing to separate
- PCBs should be black to help with radiative heat transfer to the case


##Cable:

- We'll get custom made 3-conductor 20AWG for ground, 9-48V, and 3-5V signal
- Pressure extruded urethane jacket

##Software:

- Read in PWM pulse using hardware interrupt
- Output PWM pulse using hardware output compare registers
- Measure thermistor and use PI controller or fuzzy logic to provide overheating protection
- Implement idle power, especially if using an LDO

##Bonus:

- It'd be great if the 3-5V logic input for control could tolerate the full 48V for safety and so it could be tied to the V_in line to turn the light on in scuba diving applications