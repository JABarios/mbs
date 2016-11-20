/* due_CMD.h */
#ifndef DUE_CMD_H
#define DUE_CMD_H
#include "Arduino.h"
#include "ads1298.h"
#include <SPI.h>  // include the SPI library:
#include "adsCMD.h"


//constants define pins on Arduino 
// const int IPIN_PWDN = 2; //not required for TI demo kit
//const int PIN_CLKSEL = 6;//6;//*optional
//const int IPIN_RESET  =3;//*optional

const int PIN_START = 4;
const int IPIN_DRDY = 5;
const int IPIN_CS = 10;//10
//const int PIN_DOUT = 11;//SPI out
//const int PIN_DIN = 12;//SPI in
//const int PIN_SCLK = 13;//SPI clock
const int kPIN_LED = 13;//pin con luz led, en Teensy3, is ALSO spi clock!
const int kPIN_RESET = 8; //Reset en Pin 8
const int kPIN_CLKSEL = 7; //ClkSel en Pin 7


/* 
The Arduino Due has three additional 3.3V TTL serial ports: Serial1 on pins 19 (RX) and 18 (TX); 
Serial2 on pins 17 (RX) and 16 (TX), Serial3 on pins 15 (RX) and 14 (TX). 
Pins 0 and 1 are also connected to the corresponding pins of the ATmega16U2 USB-to-TTL Serial chip,
which is connected to the USB debug port. 
Additionally, there is a native USB-serial port on the SAM3X chip, SerialUSB. 
y el Serial es el programming port
*/


/*  pines utilizados  del arduino due
  2 -- 
  3 --
  4 --
  5 --
  6 --
  7 -- clksel
  8 -- reset
  10 -- CS
  11 -- spi out
  12 -- spi in 
  13 -- spi luz led (preasignado) 
  14 -- HC06
  15 -- HC06
*/


void due_inicia_hw();

#endif 


