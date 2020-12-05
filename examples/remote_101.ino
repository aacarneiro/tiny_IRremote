/*
Author: Antonio Carneiro

Uses the serial monitor to check if the library is working as intended.

*/

#include <SoftwareSerial.h>
#include "tiny_IRremote.h"

// Pin on your attiny to communicate with the arduino
// ATTINY PINS:
// RST    --- VCC
// PIN 3  --- PIN 2
// PIN 4  --- PIN 1
// GND    --- PIN 0
int RX = 3;
int TX = 4;
SoftwareSerial Monitor(RX,TX);

int ir_pin = 0;
int light_pin = 1;
IRrecv receiver(ir_pin);
decode_results results; // create a results object of the decode_results class
// unsigned long key_value = 0; // variable to store the pressed key value

void setup() {
  // put your setup code here, to run once:
  // inMode(ir_pin, INPUT);
  pinMode(light_pin, OUTPUT);
  digitalWrite(light_pin, HIGH);
  receiver.enableIRIn(); // enable the receiver
  Monitor.begin(19200);
  delay(500);
  Monitor.println("Starting...");
}

void loop() {
  Monitor.println("I AM HERE");
  if (receiver.decode(&results)) { // decode the received signal and store it in results
    receiver.resume(); // reset the receiver for the next code
  }

  switch (results.value) { // compare the value to the following cases
    case 0xE0E08877: // if the value is equal to 0xE0E08877
      digitalToggle(light_pin); // clear the display
      results.value = 0; //resets the value to avoid infinite switching on/off
      break;
  }

  // Switches on the led if the IR code matches the desired controller type.
  if(results.decode_type == SAMSUNG){
    digitalToggle(light_pin);
  }

}

inline void digitalToggle(int pin) {
  /* Toggles the value of a digital port.
  */
  static bool state = false;
  state = !state;
  digitalWrite(pin, state);
}
