# tiny_IRremote

Library to receive and send IR signals using attiny microcontrollers.

## Supported protocols

Samsung, Sony, RC5, RC6

## Credits

Based on the code published by SeeJayDee [here](https://gist.github.com/SeeJayDee/caa9b5cc29246df44e45b8e7d1b1cdc5).

## Todos

* Implement new functions from [IRRemote](https://www.arduino.cc/reference/en/libraries/irremote/): decodePulseDistanceData
* Add decode_results as a member from IRrecv; remove it from function calls
