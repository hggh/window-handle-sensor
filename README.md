### window handle sensor

get the status of your windows. sensor is between your handle and the window.


![led-guy1.jpg](https://raw.githubusercontent.com/hggh/window-handle-sensor/master/pics/beta.jpg)


### PCBs

this project contains of two pcbs.


#### window-handle-sensor

This PCB is the sensor. It has three hall sensors placed on a **0.6mm** pcb.

library used: https://github.com/hggh/kicad-libraries

##### BOM of window-handle-sensor pcb

| number | name |
|---| ---|
| C1,C2,C3 | ceramic cap 0.1 µF SMD 0805 |
| U1,U2,U3 | Infineon TLE4913 |

#### ESP controller board

This PCB is the controller board with the ESP32-S2. The thickness of this pcb can be 1.0-1.6mm.

##### BOM of esp32-s2 pcb

| number | name |
|--- | ---|
| C1 | ceramic cap 22µF SMD 0805 |
| C2 | ceramic cap 4.7µF SMD 0805 |
| C3 | tantalum cap 220µF SMD EIA-7343-31 |
| C4 | ceramic cap 100nF SMD 0805 |
| C5 | ceramic cap 1µF SMD 0805 |
| U1 | XC6220 3.0V SOT-25 |
| U2 | ESP32-S2-WROOM |
| H1 | 2.54mm 2x3 pin header |
| J1 | JST PH S2B-PH-K |
| Q1 | BC817 |
| Q2 | BC807 |
| R1, R14 | 10k SMD0805 |
| R2, R3, R4 | 100 SMD0805 |
| R5, R6, R7, R8, R9 | 10M SMD 0805 |
| R10, R11 | 1k1 SMD 0805 |
| R12, R13 | 10k SMD 0805 |


### case

The inlay is placed on the square bolt with 3x magnets.

Magnets: Ø 2 mm, h=1mm (strength: 110g): https://www.supermagnete.de/eng/disc-magnets-neodymium/disc-magnet-2mm-1mm_S-02-01-N
