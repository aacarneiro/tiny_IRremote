#include <SoftwareSerial.h>
#include "tiny_IRremote.h"
#include <stdint.h>

int RX = 3;
int TX = 4;
SoftwareSerial Monitor(RX,TX);

uint8_t ir_pin = 0;
uint8_t light_pin = 1;
IRrecv receiver(ir_pin);
// decode_results results; // create a results object of the decode_results class
// unsigned long key_value = 0; // variable to store the pressed key value
//uint8_t vad[10];

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
  if (receiver.decode()) { // decode the received signal and store it in results
    Monitor.println(receiver.results.value);
    Monitor.println(receiver.results.decode_type);
    receiver.resume(); // reset the receiver for the next code
  }
  switch (receiver.results.value) { // compare the value to the following cases
    case 0xE0E08877: // if the value is equal to 0xE0E08877
      digitalToggle(light_pin); // clear the display
      receiver.results.value = 0; //resets the value to avoid infinite switching on/off
      break;
  }
}

inline void digitalToggle(int pin) {
  static bool state = false;
  state = !state;
  digitalWrite(pin, state);
}
