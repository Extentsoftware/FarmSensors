#ifndef _LowLevel_h
#define _LowLevel_h

#include <inttypes.h>

#ifdef VS_INTELLISENSE
#define __attribute__(...)
#define digitalPinToPort(pin) 0
#define digitalPinToBitMask(pin) 0
#define portInputRegister(arg1) 0
#endif

#if ARDUINO >= 100
#include "Arduino.h"       // for delayMicroseconds, digitalPinToBitMask, etc
#else
#include "WProgram.h"      // for delayMicroseconds
#include "pins_arduino.h"  // for digitalPinToBitMask, etc
#endif

#if defined(__AVR__)
#define PIN_TO_BASEREG(pin)             (portInputRegister(digitalPinToPort(pin)))
#define PIN_TO_BITMASK(pin)             (digitalPinToBitMask(pin))
#define IO_REG_TYPE uint8_t
#define IO_REG_ASM asm("r30")
#define DIRECT_READ(base, mask)         (((*(base)) & (mask)) ? 1 : 0)
#define DIRECT_MODE_INPUT(base, mask)   ((*((base)+1)) &= ~(mask))
#define DIRECT_MODE_OUTPUT(base, mask)  ((*((base)+1)) |= (mask))
#define DIRECT_WRITE_LOW(base, mask)    ((*((base)+2)) &= ~(mask))
#define DIRECT_WRITE_HIGH(base, mask)   ((*((base)+2)) |= (mask))

#if defined (__AVR_ATtiny85__)

/* Note : The attiny85 clock speed = 16mhz (fuses L 0xF1, H 0xDF. E oxFF
          OSCCAL VALUE must also be calibrated to 16mhz

*/
#define CLEARINTERRUPT GIFR |= (1 << INTF0) | (1<<PCIF);
#define USERTIMER_COMPA_vect TIMER1_COMPA_vect

__attribute__((always_inline)) static inline void UserTimer_Init( void )
{
  TCCR1 = 0;                  //stop the timer
  TCNT1 = 0;
  //GTCCR |= (1<<PSR1);       //reset the prescaler
  TIMSK = 0;                  // clear timer interrupts enable
}
__attribute__((always_inline)) static inline void UserTimer_Run(int skipTicks)
{

  TCNT1 = 0;                  //zero the timer
  GTCCR |= (1 << PSR1);       //reset the prescaler
  OCR1A = skipTicks;          //set the compare value
  TCCR1 |= (1 << CTC1) |  (0 << CS13) |  (1 << CS12) | (1 << CS11) | (0 << CS10);//32 prescaler
  //TCCR1 |= (1 << CTC1) |  (0 << CS13) |  (1 << CS12) | (0 << CS11) | (0 << CS10);//8 prescaler
  TIMSK |= (1 << OCIE1A);     //interrupt on Compare Match A
}

__attribute__((always_inline)) static inline void UserTimer_Stop()
{
  TIMSK = 0;    //&= ~(1 << OCIE1A);// clear timer interrupt enable
  TCCR1 = 0;
}


#elif defined (__AVR_ATmega328P__)
#define CLEARINTERRUPT EIFR |= (1 << INTF0)
#define USERTIMER_COMPA_vect TIMER1_COMPA_vect

__attribute__((always_inline)) static inline void UserTimer_Init( void )
{
  TCCR1A = 0;
  TCCR1B = 0;
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
}
__attribute__((always_inline)) static inline void UserTimer_Run(short skipTicks)
{
  TCNT1 = 0;
  OCR1A = skipTicks;
  // turn on CTC mode with 64 prescaler
  TCCR1B = (1 << WGM12) | (1 << CS11) | (1 << CS10);
}
#define UserTimer_Stop() TCCR1B = 0
#endif

#elif defined(__MK20DX128__) || defined(__MK20DX256__)
#define PIN_TO_BASEREG(pin)             (portOutputRegister(pin))
#define PIN_TO_BITMASK(pin)             (1)
#define IO_REG_TYPE uint8_t
#define IO_REG_ASM
#define DIRECT_READ(base, mask)         (*((base)+512))
#define DIRECT_MODE_INPUT(base, mask)   (*((base)+640) = 0)
#define DIRECT_MODE_OUTPUT(base, mask)  (*((base)+640) = 1)
#define DIRECT_WRITE_LOW(base, mask)    (*((base)+256) = 1)
#define DIRECT_WRITE_HIGH(base, mask)   (*((base)+128) = 1)

#elif defined(__SAM3X8E__)
// Arduino 1.5.1 may have a bug in delayMicroseconds() on Arduino Due.
// http://arduino.cc/forum/index.php/topic,141030.msg1076268.html#msg1076268
// If you have trouble with OneWire on Arduino Due, please check the
// status of delayMicroseconds() before reporting a bug in OneWire!
#define PIN_TO_BASEREG(pin)             (&(digitalPinToPort(pin)->PIO_PER))
#define PIN_TO_BITMASK(pin)             (digitalPinToBitMask(pin))
#define IO_REG_TYPE uint32_t
#define IO_REG_ASM
#define DIRECT_READ(base, mask)         (((*((base)+15)) & (mask)) ? 1 : 0)
#define DIRECT_MODE_INPUT(base, mask)   ((*((base)+5)) = (mask))
#define DIRECT_MODE_OUTPUT(base, mask)  ((*((base)+4)) = (mask))
#define DIRECT_WRITE_LOW(base, mask)    ((*((base)+13)) = (mask))
#define DIRECT_WRITE_HIGH(base, mask)   ((*((base)+12)) = (mask))
#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#endif

#elif defined(__PIC32MX__)
#define PIN_TO_BASEREG(pin)             (portModeRegister(digitalPinToPort(pin)))
#define PIN_TO_BITMASK(pin)             (digitalPinToBitMask(pin))
#define IO_REG_TYPE uint32_t
#define IO_REG_ASM
#define DIRECT_READ(base, mask)         (((*(base+4)) & (mask)) ? 1 : 0)  //PORTX + 0x10
#define DIRECT_MODE_INPUT(base, mask)   ((*(base+2)) = (mask))            //TRISXSET + 0x08
#define DIRECT_MODE_OUTPUT(base, mask)  ((*(base+1)) = (mask))            //TRISXCLR + 0x04
#define DIRECT_WRITE_LOW(base, mask)    ((*(base+8+1)) = (mask))          //LATXCLR  + 0x24
#define DIRECT_WRITE_HIGH(base, mask)   ((*(base+8+2)) = (mask))          //LATXSET + 0x28

#else
#error "Please define I/O register types here"
#endif

#endif