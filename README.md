# Esp32-S2_SFPModuleTester
Small project to interface with SFP modules for fiber optic communications using an esp32-s2 microcontroller board (Wemos S2 mini). Reads module info and sends simple test bitstream.

# Pin Assignment

WARNING: dont use the 3.3V supply from the MCU board if it cant supply at least 500mA current (Wemos S2 mini can not!). Also use Caps to filter 3.3V supply for SFP module.


| Function      | SFP Module Pin| Esp32-S2 Pin |
| ------------- | ------------- | ------------ |
| Tx Disable    |   3           |   2          |
| MOD-DEF (SDA) |   4           |   7	       |
| MOD-DEF (SCL) |   5           |   6          |
| Signal Loss   |   8           |   3          |
| Tx+           |   18          |   16         |
| Tx-           |   19          |   17         |
|               |               |              |
| VeeT          |   1/20/17     |   GND        |
| VeeR          |   9/10/11/14  |   GND        |
| VccT          |   16          |   +3.3V      |
| VccR          |   17          |   +3.3V      |
      
