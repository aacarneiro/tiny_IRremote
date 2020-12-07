/*
   tiny_IRremote
   Version 0.2 July, 2016
   Christian D'Abrera
   Fixed what was originally rather broken code from http://www.gammon.com.au/Arduino/
   ...itself based on work by Ken Shirriff.

   This code was tested for both sending and receiving IR on an ATtiny85 DIP-8 chip.
   IMPORTANT: IRsend only works from PB4 ("pin 4" according to Arduino). You will need to
   determine which physical pin this corresponds to for your chip, and connect your transmitter
   LED there.

   Copyright 2009 Ken Shirriff
   For details, see http://arcfn.com/2009/08/multi-protocol-infrared-remote-library.html

   Interrupt code based on NECIRrcv by Joe Knapp
   http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
   Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
*/

#include "tiny_IRremote.h"
#include "tiny_IRremoteInt.h"

// Provides ISR
#include <avr/interrupt.h>

volatile irparams_t irparams;

// These versions of MATCH, MATCH_MARK, and MATCH_SPACE are only for debugging.
// To use them, set DEBUG in tiny_IRremoteInt.h
// Normally macros are used for efficiency
#ifdef DEBUG
#error debug enabled
int MATCH(int measured, int desired) {
  Serial.print("Testing: ");
  Serial.print(TICKS_LOW(desired), DEC);
  Serial.print(" <= ");
  Serial.print(measured, DEC);
  Serial.print(" <= ");
  Serial.println(TICKS_HIGH(desired), DEC);
  return measured >= TICKS_LOW(desired) && measured <= TICKS_HIGH(desired);
}

int MATCH_MARK(int measured_ticks, int desired_us) {
  Serial.print("Testing mark ");
  Serial.print(measured_ticks * USECPERTICK, DEC);
  Serial.print(" vs ");
  Serial.print(desired_us, DEC);
  Serial.print(": ");
  Serial.print(TICKS_LOW(desired_us + MARK_EXCESS), DEC);
  Serial.print(" <= ");
  Serial.print(measured_ticks, DEC);
  Serial.print(" <= ");
  Serial.println(TICKS_HIGH(desired_us + MARK_EXCESS), DEC);
  return measured_ticks >= TICKS_LOW(desired_us + MARK_EXCESS) && measured_ticks <= TICKS_HIGH(desired_us + MARK_EXCESS);
}

int MATCH_SPACE(int measured_ticks, int desired_us) {
  Serial.print("Testing space ");
  Serial.print(measured_ticks * USECPERTICK, DEC);
  Serial.print(" vs ");
  Serial.print(desired_us, DEC);
  Serial.print(": ");
  Serial.print(TICKS_LOW(desired_us - MARK_EXCESS), DEC);
  Serial.print(" <= ");
  Serial.print(measured_ticks, DEC);
  Serial.print(" <= ");
  Serial.println(TICKS_HIGH(desired_us - MARK_EXCESS), DEC);
  return measured_ticks >= TICKS_LOW(desired_us - MARK_EXCESS) && measured_ticks <= TICKS_HIGH(desired_us - MARK_EXCESS);
}
#endif

#if SEND_NEC
void IRsend::sendNEC(unsigned long data, int nbits)
{
  enableIROut(38);
  mark(NEC_HEADER_MARK);
  space(NEC_HEADER_SPACE);
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(NEC_BIT_MARK);
      space(NEC_ONE_SPACE);
    }
    else {
      mark(NEC_BIT_MARK);
      space(NEC_ZERO_SPACE);
    }
    data <<= 1;
  }
  mark(NEC_BIT_MARK);
  space(0);
}
#endif // SEND_NEC

#if SEND_SONY
void IRsend::sendSony(unsigned long data, int nbits) {
  enableIROut(40);
  mark(SONY_HEADER_MARK);
  space(SONY_HEADER_SPACE);
  data = data << (32 - nbits);
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(SONY_ONE_MARK);
      space(SONY_HEADER_SPACE);
    }
    else {
      mark(SONY_ZERO_MARK);
      space(SONY_HEADER_SPACE);
    }
    data <<= 1;
  }
}
#endif //SEND_SONY

#if SEND_SAMSUNG
void IRsend::sendSAMSUNG(unsigned long data, int nbits) {
  // Set IR carrier frequency
  enableIROut(38);

  // Header
  mark(SAMSUNG_HEADER_MARK);
  space(SAMSUNG_HEADER_SPACE);

  // Data
  // old version, this function is not implemented for attiny
  // sendPulseDistanceWidthData(SAMSUNG_BIT_MARK, SAMSUNG_ONE_SPACE, SAMSUNG_BIT_MARK, SAMSUNG_ZERO_SPACE, data, nbits);

  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      mark(SAMSUNG_BIT_MARK);
      space(SAMSUNG_ONE_SPACE);
    }
    else {
      mark(SAMSUNG_BIT_MARK);
      space(SAMSUNG_ZERO_SPACE);
    }
    data <<= 1;
  }

  // Footer
  mark(SAMSUNG_BIT_MARK);
  space(0);  // Always end with the LED off
}
#endif // SEND_SAMSUNG

void IRsend::sendRaw(unsigned int buf[], int len, int hz)
{

  enableIROut(hz);
  for (int i = 0; i < len; i++) {
    if (i & 1) {
      space(buf[i]);
    }
    else {
      mark(buf[i]);
    }
  }
  space(0); // Just to be sure
}

#if SEND_RC5
// Note: first bit must be a one (start bit)
void IRsend::sendRC5(unsigned long data, int nbits)
{
  enableIROut(36);
  data = data << (32 - nbits);
  mark(RC5_T1); // First start bit
  space(RC5_T1); // Second start bit
  mark(RC5_T1); // Second start bit
  for (int i = 0; i < nbits; i++) {
    if (data & TOPBIT) {
      space(RC5_T1); // 1 is space, then mark
      mark(RC5_T1);
    }
    else {
      mark(RC5_T1);
      space(RC5_T1);
    }
    data <<= 1;
  }
  space(0); // Turn off at end
}
#endif // SEND_RC5

#if SEND_RC6
// Caller needs to take care of flipping the toggle bit
void IRsend::sendRC6(unsigned long data, int nbits)
{
  enableIROut(36);
  data = data << (32 - nbits);
  mark(RC6_HEADER_MARK);
  space(RC6_HEADER_SPACE);
  mark(RC6_T1); // start bit
  space(RC6_T1);
  int t;
  for (int i = 0; i < nbits; i++) {
    if (i == 3) {
      // double-wide trailer bit
      t = 2 * RC6_T1;
    }
    else {
      t = RC6_T1;
    }
    if (data & TOPBIT) {
      mark(t);
      space(t);
    }
    else {
      space(t);
      mark(t);
    }

    data <<= 1;
  }
  space(0); // Turn off at end
}
#endif // SEND_RC6


void IRsend::mark(int time) {
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  GTCCR |= _BV(COM1B1); // Enable pin 3 PWM output (PB4 - Arduino D4)
  delayMicroseconds(time);
}

/* Leave pin off for time (given in microseconds) */
void IRsend::space(int time) {
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  GTCCR &= ~(_BV(COM1B1)); // Disable pin 3 PWM output (PB4 - Arduino D4)
  delayMicroseconds(time);
}

void IRsend::enableIROut(int khz) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  // The IR output will be on pin 3 (PB4 - Arduino D4) (OC1B).
  // This routine is designed for 36-40KHz; if you use it for other values, it's up to you
  // to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
  // TIMER1 is used in fast PWM mode, with OCR1Ccontrolling the frequency and OCR1B
  // controlling the duty cycle.
  // There is no prescaling, so the output frequency is 8MHz / (2 * OCR1C)
  // To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
  // A few hours staring at the ATmega documentation and this will all make sense.
  // See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.


  // Disable the Timer1 Interrupt (which is used for receiving IR)
  TIMSK &= ~_BV(TOIE1); //Timer1 Overflow Interrupt


  pinMode(4, OUTPUT);  // (PB4 - Arduino D4 - physical pin 3)
  digitalWrite(4, LOW); // When not sending PWM, we want it low


  // CTC1 = 1: TOP value set to OCR1C
  // CS = 0001: No Prescaling
  TCCR1 = _BV(CTC1) | _BV(CS10);

  // PWM1B = 1: Enable PWM for OCR1B
  GTCCR = _BV(PWM1B);

  // The top value for the timer.  The modulation frequency will be SYSCLOCK / OCR1C.
  OCR1C = SYSCLOCK / khz / 1000;
  OCR1B = OCR1C / 3; // 33% duty cycle

}

IRrecv::IRrecv(int recvpin)
{
  irparams.recvpin = recvpin;
}

// initialization
void IRrecv::enableIRIn() {
  // setup pulse clock timer interrupt
  GTCCR = 0;  // normal, non-PWM mode

  //Prescale /4 (8M/4 = 0.5 microseconds per tick)
  // Therefore, the timer interval can range from 0.5 to 128 microseconds
  // depending on the reset value (255 to 0)
  TCCR1 = _BV(CS11) | _BV(CS10);

  //TIMER1 Overflow Interrupt Enable
  TIMSK |= _BV(TOIE1);




  RESET_TIMER1;

  sei();  // enable interrupts

  // initialize state machine variables
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;


  // set pin modes
  pinMode(irparams.recvpin, INPUT);
}



// TIMER1 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 50 microseconds.
// rawlen counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a SPACE gets long, ready is set, state switches to IDLE, timing of SPACE continues.
// As soon as first MARK arrives, gap width is recorded, ready is cleared, and new logging starts
ISR(TIM1_OVF_vect)
{
  RESET_TIMER1;

  uint8_t irdata = (uint8_t)digitalRead(irparams.recvpin);

  irparams.timer++; // One more 50us tick
  if (irparams.rawlen >= RAWBUF) {
    // Buffer overflow
    irparams.rcvstate = STATE_STOP;
  }
  switch (irparams.rcvstate) {
    case STATE_IDLE: // In the middle of a gap
      if (irdata == MARK) {
        if (irparams.timer < GAP_TICKS) {
          // Not big enough to be a gap.
          irparams.timer = 0;
        }
        else {
          // gap just ended, record duration and start recording transmission
          irparams.rawlen = 0;
          irparams.rawbuf[irparams.rawlen++] = irparams.timer;
          irparams.timer = 0;
          irparams.rcvstate = STATE_MARK;
        }
      }
      break;
    case STATE_MARK: // timing MARK
      if (irdata == SPACE) {   // MARK ended, record time
        irparams.rawbuf[irparams.rawlen++] = irparams.timer;
        irparams.timer = 0;
        irparams.rcvstate = STATE_SPACE;
      }
      break;
    case STATE_SPACE: // timing SPACE
      if (irdata == MARK) { // SPACE just ended, record it
        irparams.rawbuf[irparams.rawlen++] = irparams.timer;
        irparams.timer = 0;
        irparams.rcvstate = STATE_MARK;
      }
      else { // SPACE
        if (irparams.timer > GAP_TICKS) {
          // big SPACE, indicates gap between codes
          // Mark current code as ready for processing
          // Switch to STOP
          // Don't reset timer; keep counting space width
          irparams.rcvstate = STATE_STOP;
        }
      }
      break;
    case STATE_STOP: // waiting, measuring gap
      if (irdata == MARK) { // reset gap timer
        irparams.timer = 0;
      }
      break;
  }

}

void IRrecv::resume() {
  irparams.rcvstate = STATE_IDLE;
  irparams.rawlen = 0;
}



// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in results
int IRrecv::decode() {
  results.rawbuf = irparams.rawbuf;
  results.rawlen = irparams.rawlen;
  if (irparams.rcvstate != STATE_STOP) {
    return ERR;
  }
#ifdef DEBUG
  Serial.println("Attempting NEC decode");
#endif
  if (decodeNEC()) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting Sony decode");
#endif

#if DECODE_SONY
  if (decodeSony()) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting RC5 decode");
#endif
#endif // DECODE_SONY

#if DECODE_RC5
  if (decodeRC5()) {
    return DECODED;
  }
#ifdef DEBUG
  Serial.println("Attempting RC6 decode");
#endif
#endif // DECODE_RC5

#if DECODE_SAMSUNG
  if (decodeSAMSUNG()) {
    return DECODED;
  }
  #ifdef DEBUG
    Serial.println("Attempting Samsung decode");
  #endif
#endif //DECODE_SAMSUNG

#if DECODE_RC6
  if (decodeRC6()) {
    return DECODED;
  }
#endif // DECODE_RC6

  if (results.rawlen >= 6) {
    // Only return raw buffer if at least 6 bits
    results.decode_type = UNKNOWN;
    results.bits = 0;
    results.value = 0;
    return DECODED;
  }
  // Throw away and start over
  resume();
  return ERR;
}

long IRrecv::decodeNEC() {
  long data = 0;
  int offset = 1; // Skip first space
  // Initial mark
  if (!MATCH_MARK(results.rawbuf[offset], NEC_HEADER_MARK)) {
    return ERR;
  }
  offset++;
  // Check for repeat
  if (irparams.rawlen == 4 &&
      MATCH_SPACE(results.rawbuf[offset], NEC_REPEAT_SPACE) &&
      MATCH_MARK(results.rawbuf[offset + 1], NEC_BIT_MARK)) {
    results.bits = 0;
    results.value = REPEAT;
    results.decode_type = NEC;
    return DECODED;
  }
  if (irparams.rawlen < 2 * NEC_BITS + 4) {
    return ERR;
  }
  // Initial space
  if (!MATCH_SPACE(results.rawbuf[offset], NEC_HEADER_SPACE)) {
    return ERR;
  }
  offset++;
  for (int i = 0; i < NEC_BITS; i++) {
    if (!MATCH_MARK(results.rawbuf[offset], NEC_BIT_MARK)) {
      return ERR;
    }
    offset++;
    if (MATCH_SPACE(results.rawbuf[offset], NEC_ONE_SPACE)) {
      data = (data << 1) | 1;
    }
    else if (MATCH_SPACE(results.rawbuf[offset], NEC_ZERO_SPACE)) {
      data <<= 1;
    }
    else {
      return ERR;
    }
    offset++;
  }
  // Success
  results.bits = NEC_BITS;
  results.value = data;
  results.decode_type = NEC;
  return DECODED;
}
#if DECODE_SONY
long IRrecv::decodeSony() {
  long data = 0;
  if (irparams.rawlen < 2 * SONY_BITS + 2) {
    return ERR;
  }
  int offset = 1; // Skip first space
  // Initial mark
  if (!MATCH_MARK(results.rawbuf[offset], SONY_HEADER_MARK)) {
    return ERR;
  }
  offset++;

  while (offset + 1 < irparams.rawlen) {
    if (!MATCH_SPACE(results.rawbuf[offset], SONY_HEADER_SPACE)) {
      break;
    }
    offset++;
    if (MATCH_MARK(results.rawbuf[offset], SONY_ONE_MARK)) {
      data = (data << 1) | 1;
    }
    else if (MATCH_MARK(results.rawbuf[offset], SONY_ZERO_MARK)) {
      data <<= 1;
    }
    else {
      return ERR;
    }
    offset++;
  }

  // Success
  results.bits = (offset - 1) / 2;
  if (results.bits < 12) {
    results.bits = 0;
    return ERR;
  }
  results.value = data;
  results.decode_type = SONY;
  return DECODED;
}
#endif // DECODE_SONY

// Gets one undecoded level at a time from the raw buffer.
// The RC5/6 decoding is easier if the data is broken into time intervals.
// E.g. if the buffer has MARK for 2 time intervals and SPACE for 1,
// successive calls to getRClevel will return MARK, MARK, SPACE.
// offset and used are updated to keep track of the current position.
// t1 is the time interval for a single bit in microseconds.
// Returns -1 for error (measured time interval is not a multiple of t1).
int IRrecv::getRClevel(int *offset, int *used, int t1) {
  if (*offset >= results.rawlen) {
    // After end of recorded buffer, assume SPACE.
    return SPACE;
  }
  int width = results.rawbuf[*offset];
  int val = ((*offset) % 2) ? MARK : SPACE;
  int correction = (val == MARK) ? MARK_EXCESS : - MARK_EXCESS;

  int avail;
  if (MATCH(width, t1 + correction)) {
    avail = 1;
  }
  else if (MATCH(width, 2 * t1 + correction)) {
    avail = 2;
  }
  else if (MATCH(width, 3 * t1 + correction)) {
    avail = 3;
  }
  else {
    return -1;
  }

  (*used)++;
  if (*used >= avail) {
    *used = 0;
    (*offset)++;
  }
#ifdef DEBUG
  if (val == MARK) {
    Serial.println("MARK");
  }
  else {
    Serial.println("SPACE");
  }
#endif
  return val;
}
#if DECODE_RC5
long IRrecv::decodeRC5() {
  if (irparams.rawlen < MIN_RC5_SAMPLES + 2) {
    return ERR;
  }
  int offset = 1; // Skip gap space
  long data = 0;
  int used = 0;
  // Get start bits
  if (getRClevel(&offset, &used, RC5_T1) != MARK) return ERR;
  if (getRClevel(&offset, &used, RC5_T1) != SPACE) return ERR;
  if (getRClevel(&offset, &used, RC5_T1) != MARK) return ERR;
  int nbits;
  for (nbits = 0; offset < irparams.rawlen; nbits++) {
    int levelA = getRClevel(&offset, &used, RC5_T1);
    int levelB = getRClevel(&offset, &used, RC5_T1);
    if (levelA == SPACE && levelB == MARK) {
      // 1 bit
      data = (data << 1) | 1;
    }
    else if (levelA == MARK && levelB == SPACE) {
      // zero bit
      data <<= 1;
    }
    else {
      return ERR;
    }
  }

  // Success
  results.bits = nbits;
  results.value = data;
  results.decode_type = RC5;
  return DECODED;
}
#endif  //DECODE_RC5

#if DECODE_RC6
long IRrecv::decodeRC6() {
  if (results.rawlen < MIN_RC6_SAMPLES) {
    return ERR;
  }
  int offset = 1; // Skip first space
  // Initial mark
  if (!MATCH_MARK(results.rawbuf[offset], RC6_HEADER_MARK)) {
    return ERR;
  }
  offset++;
  if (!MATCH_SPACE(results.rawbuf[offset], RC6_HEADER_SPACE)) {
    return ERR;
  }
  offset++;
  long data = 0;
  int used = 0;
  // Get start bit (1)
  if (getRClevel(&offset, &used, RC6_T1) != MARK) return ERR;
  if (getRClevel(&offset, &used, RC6_T1) != SPACE) return ERR;
  int nbits;
  for (nbits = 0; offset < results.rawlen; nbits++) {
    int levelA, levelB; // Next two levels
    levelA = getRClevel(&offset, &used, RC6_T1);
    if (nbits == 3) {
      // T bit is double wide; make sure second half matches
      if (levelA != getRClevel(&offset, &used, RC6_T1)) return ERR;
    }
    levelB = getRClevel(&offset, &used, RC6_T1);
    if (nbits == 3) {
      // T bit is double wide; make sure second half matches
      if (levelB != getRClevel(&offset, &used, RC6_T1)) return ERR;
    }
    if (levelA == MARK && levelB == SPACE) { // reversed compared to RC5
      // 1 bit
      data = (data << 1) | 1;
    }
    else if (levelA == SPACE && levelB == MARK) {
      // zero bit
      data <<= 1;
    }
    else {
      return ERR; // Error
    }
  }
  // Success
  results.bits = nbits;
  results.value = data;
  results.decode_type = RC6;
  return DECODED;
}
#endif //DECODE_RC6
//==============================================================================
//              SSSS   AAA    MMM    SSSS  U   U  N   N   GGGG
//             S      A   A  M M M  S      U   U  NN  N  G
//              SSS   AAAAA  M M M   SSS   U   U  N N N  G  GG
//                 S  A   A  M   M      S  U   U  N  NN  G   G
//             SSSS   A   A  M   M  SSSS    UUU   N   N   GGG
//==============================================================================


//+=============================================================================



//+=============================================================================
// SAMSUNGs have a repeat only 4 items long
//
#if DECODE_SAMSUNG
long IRrecv::decodeSAMSUNG() {
  unsigned int offset = 1;  // Skip first space

  // Initial mark
  if (!MATCH_MARK(results.rawbuf[offset], SAMSUNG_HEADER_MARK)) {
    return ERR;
  }
  offset++;

  // Check for repeat
  if ((results.rawlen == 4) && MATCH_SPACE(results.rawbuf[offset], SAMSUNG_REPEAT_SPACE)
      && MATCH_MARK(results.rawbuf[offset + 1], SAMSUNG_BIT_MARK)) {
    results.bits = 0;
    results.value = REPEAT;
    //results.isRepeat = true;
    results.decode_type = SAMSUNG;
    return true;
  }
  if (results.rawlen < (2 * SAMSUNG_BITS) + 4) {
    return ERR;
  }

  // Initial space
  if (!MATCH_SPACE(results.rawbuf[offset], SAMSUNG_HEADER_SPACE)) {
    return ERR;
  }
  offset++;

  if (!decodePulseDistanceData(SAMSUNG_BITS, offset, SAMSUNG_BIT_MARK, SAMSUNG_ONE_SPACE, SAMSUNG_ZERO_SPACE)) {
    return ERR;
  }

  // Success
  results.bits = SAMSUNG_BITS;
  results.decode_type = SAMSUNG;
  return true;
}
#endif

bool IRrecv::decodePulseDistanceData(unsigned int aNumberOfBits, unsigned int aStartOffset, unsigned int aBitMarkMicros,
                                     unsigned int aOneSpaceMicros, unsigned int aZeroSpaceMicros, bool aMSBfirst) {
  unsigned long tDecodedData = 0;

  if (aMSBfirst) {
    for (unsigned int i = 0; i < aNumberOfBits; i++) {
      // Check for constant length mark
      if (!MATCH_MARK(results.rawbuf[aStartOffset], aBitMarkMicros)) {
        return false;
      }
      aStartOffset++;

      // Check for variable length space indicating a 0 or 1
      if (MATCH_SPACE(results.rawbuf[aStartOffset], aOneSpaceMicros)) {
        tDecodedData = (tDecodedData << 1) | 1;
      } else if (MATCH_SPACE(results.rawbuf[aStartOffset], aZeroSpaceMicros)) {
        tDecodedData = (tDecodedData << 1) | 0;
      } else {
        return false;
      }
      aStartOffset++;
    }
  }
#if defined(LSB_FIRST_REQUIRED)
  else {
    for (unsigned long mask = 1UL; aNumberOfBits > 0; mask <<= 1, aNumberOfBits--) {
      // Check for constant length mark
      if (!MATCH_MARK(results.rawbuf[aStartOffset], aBitMarkMicros)) {
        return false;
      }
      aStartOffset++;

      // Check for variable length space indicating a 0 or 1
      if (MATCH_SPACE(results.rawbuf[aStartOffset], aOneSpaceMicros)) {
        tDecodedData |= mask; // set the bit
      } else if (MATCH_SPACE(results.rawbuf[aStartOffset], aZeroSpaceMicros)) {
        // do not set the bit
      } else {
        return false;
      }

      aStartOffset++;
    }
  }
#endif
  results.value = tDecodedData;
  return true;
}
