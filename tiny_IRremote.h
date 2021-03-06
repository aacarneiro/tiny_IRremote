/*
 * tiny_IRremote
 * Version 0.2 July, 2016
 * Christian D'Abrera
 * Fixed what was originally rather broken code from http://www.gammon.com.au/Arduino/
 * ...itself based on work by Ken Shirriff.
 *
 * This code was tested for both sending and receiving IR on an ATtiny85 DIP-8 chip.
 * IMPORTANT: IRsend only works from PB4 ("pin 4" according to Arduino). You will need to 
 * determine which physical pin this corresponds to for your chip, and connect your transmitter
 * LED there.
 *
 * Copyright 2009 Ken Shirriff
 * For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.htm http://arcfn.com
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 */

#ifndef tiny_IRremote_h
#define tiny_IRremote_h

// The following are compile-time library options.
// If you change them, recompile the library.
// If DEBUG is defined, a lot of debugging output will be printed during decoding.
// TEST must be defined for the IRtest unittests to work.  It will make some
// methods virtual, which will be slightly slower, which is why it is optional.
// #define DEBUG
// #define TEST

// Results returned from the decoder
class decode_results {
public:
  int decode_type; // NEC, SONY, RC5, SAMSUNG, UNKNOWN
  unsigned long value; // Decoded value
  int bits; // Number of bits in decoded value
  volatile unsigned int *rawbuf; // Raw intervals in .5 us ticks
  int rawlen; // Number of records in rawbuf.
};

// Values for decode_type
#define NEC 1
#define SONY 2
#define RC5 3
#define RC6 4
#define SAMSUNG 5
#define UNKNOWN -1

// Select which part of the code to compile

#define DECODE_NEC 1
#define DECODE_SONY 1
#define DECODE_RC5 1
#define DECODE_RC6 1
#define DECODE_SAMSUNG 1

#define SEND_NEC 0
#define SEND_SONY 0
#define SEND_RC5 0
#define SEND_RC6 0
#define SEND_SAMSUNG 0

// Decoded value for NEC when a repeat code is received
#define REPEAT 0xffffffff

// main class for receiving IR
class IRrecv
{
public:
  decode_results results;
  
  IRrecv(int recvpin);
  int decode();
  void enableIRIn();
  void resume();
  bool decodePulseDistanceData(unsigned int aNumberOfBits, unsigned int aStartOffset, unsigned int aBitMarkMicros,
        unsigned int aOneSpaceMicros, unsigned int aZeroSpaceMicros, bool aMSBfirst = true);
private:
  // These are called by decode
  int getRClevel(int *offset, int *used, int t1);
  long decodeNEC();
  
  #if DECODE_SONY
  long decodeSony();
  #endif

  #if DECODE_RC5
    long decodeRC5();
  #endif

  #if DECODE_RC6
  long decodeRC6();
  #endif

  #if DECODE_SAMSUNG
  long decodeSAMSUNG();
  #endif
} 
;

// Only used for testing; can remove virtual for shorter code
#ifdef TEST
#define VIRTUAL virtual
#else
#define VIRTUAL
#endif

class IRsend
{
public:
  IRsend() {}
  #if SEND_NEC
  void sendNEC(unsigned long data, int nbits);
  #endif // SEND_NEC

  #if SEND_SONY
  void sendSony(unsigned long data, int nbits);
  #endif //SEND_SONY

  void sendRaw(unsigned int buf[], int len, int hz);
  
  #if SEND_RC5
  void sendRC5(unsigned long data, int nbits);
  #endif // SEND_RC5

  #if SEND_RC6
  void sendRC6(unsigned long data, int nbits);
  #endif // SEND_RC6

  #if SEND_SAMSUNG
  void sendSAMSUNG(unsigned long data, int nbits);
  #endif // SEND_SAMSUNG
  
  // private:
  void enableIROut(int khz);
  VIRTUAL void mark(int usec);
  VIRTUAL void space(int usec);
}
;

// Some useful constants

#define USECPERTICK 50  // microseconds per clock interrupt tick
#define RAWBUF 76 // Length of raw duration buffer

// Marks tend to be 100us too long, and spaces 100us too short
// when received due to sensor lag.
#define MARK_EXCESS 100

// #endif