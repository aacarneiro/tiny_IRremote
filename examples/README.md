# Examples

## remote_101.ino


## remote_serialcomms

Uses the Arduino serial monitor to display the data received and decode type.

1. Upload the ArduinoISP.ino to your arduino.

2. Under *tools* select your attiny board. As *tools>programmer*, select "Arduino as ISP". Connect the attiny to given pins and upload *remote_serialcomms.ino* to your chip.

3. Select your Arduino board from the *tools* menu and upload the BareMinimum.ino example to it. 

4. Connect the attiny pins 3 and 4 to the RX/TX ports of your arduino.
 
5. Connect the IR receiver to the attiny pin 0.

6. Open the serial monitor, select BAUD=19200.

7. Point an IR remote to the receiver and read the values shown in the serial monitor.

Make sure to disconnect the RX/TX ports before uploading new code into your arduino.