#include "OWSlave.h"

/* Note : The attiny85 clock speed = 16mhz (fuses L 0xF1, H 0xDF. E oxFF
          OSCCAL VALUE must also be calibrated to 16mhz

*/

namespace
{
  const unsigned long PresenceWaitDuration = 30;   // spec 15us to 60us  / 40us measured
  const unsigned long PresenceDuration = 150;      // spec  60us to 240us  / 148us measured
  const unsigned long ReadBitSamplingTime = 15;    // spec > 15us to 60us  / 31us measured
  const unsigned long ReadBitMaxDuration = 70;     // anything over this is an error
  const unsigned long SendBitDuration = 60;        // bus release time spec 15us / measured 19us
  const unsigned long ResetMinDuration = 400;      // min 480us  / 484us measured
  const unsigned long ResetMaxDuration = 640;      //

  const byte ReceiveCommand = (byte) - 1;
}

void setTimerEvent(short delayMicroSeconds, bool oneShot, void(*handler)());

#define CLEARINTERRUPT GIFR |= (1 << INTF0) | (1<<PCIF);
#define USERTIMER_COMPA_vect TIMER1_COMPA_vect
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)

static void(*timerEvent)() = 0;
static volatile bool _oneShot = false;

byte OWSlave::rom_[8];
Pin OWSlave::pin_;
Pin OWSlave::debug_;
OWSlave::State OWSlave::state_;

byte OWSlave::bufferByte_;
byte OWSlave::bufferBitPos_;
short OWSlave::bufferPos_;
short OWSlave::bufferLen_;
short OWSlave::buffer_[8];
bool OWSlave::bufferInverseToggle_;
bool OWSlave::bufferSendWithInverse_;

volatile unsigned int OWSlave::debugValue;
volatile unsigned long OWSlave::_readStartTime;

void(*OWSlave::clientReceiveCallback_)(OWSlave::ReceiveEvent evt, byte data);

__attribute__((always_inline)) static inline void UserTimer_Init( void )
{
  TCCR1 = 0;                  //stop the timer
  TCNT1 = 0;
  //GTCCR |= (1<<PSR1);       //reset the prescaler
  TIMSK |= (1 << OCIE1A);;    // clear timer interrupts enable
}

__attribute__((always_inline)) static inline void UserTimer_Run(int skipTicks)
{
  TCNT1 = 0;                  //zero the timer
  GTCCR |= (1 << PSR1);       //reset the prescaler
  OCR1A = skipTicks / 2;          //set the compare value
  TCCR1 |= (1 << CTC1) |  (0 << CS13) |  (1 << CS12) | (1 << CS11) | (0 << CS10);//32 prescaler
  TIMSK |= (1 << OCIE1A);     //interrupt on Compare Match A
}

__attribute__((always_inline)) static inline void UserTimer_Stop()
{
  //TIMSK = 0;    //&= ~(1 << OCIE1A);// clear timer interrupt enable
  TIMSK &= ~(1 << OCIE1A);
  TCCR1 = 0;
}

ISR(USERTIMER_COMPA_vect) // timer1 interrupt
{
  void(*event)() = timerEvent;
	if (_oneShot)
	{
  	UserTimer_Stop(); // disable clock
  	timerEvent = 0;
	}
  event();
}

void OWSlave::begin(const byte* rom, Pin pin, Pin debug)
{
  pin_ = pin;
  debug_ = debug;
  

  memcpy(rom_, rom, 7);
  rom_[7] = crc8(rom_, 7);

  debug_.outputMode();
  debug_.writeLow();

  state_ = State::WaitingForReset;

  // prepare hardware timer
  UserTimer_Init();

  clearBuffer();

  debugValue = 0;

  // start 1-wire activity
  beginRead();
}

void OWSlave::end()
{
  cli();
  UserTimer_Stop();
  pin_.detachInterrupt();
  releaseBus_();
  sei();
}

void OWSlave::beginRead()
{
  UserTimer_Stop();

  releaseBus_();

  _readStartTime = micros();
  pin_.attachInterrupt( &OWSlave::ReadInterrupt, CHANGE );
}

void OWSlave::endRead()
{
  pin_.detachInterrupt();
}

void OWSlave::ReadInterrupt()
{
  cli();

  bool v = pin_.read();
  unsigned long now = micros();
  unsigned long duration = now - _readStartTime;
  _readStartTime = now;

  if (!v)
  {
    // reset counter    
    return;
  }

  if (duration > ResetMinDuration )
  {
    beginPrePresence_();
    sei();
    return;
  }
  
  if (state_ == State::Writing )
  {
    if (duration < ReadBitSamplingTime)
    {
      sei();
      WriteNextBit();
    }
  }
  else
  {
    ReadPulse( duration );
  }
  sei();
}


void OWSlave::ReadPulse( unsigned long duration )
{
    debugValue = duration;
    if (duration > ReadBitSamplingTime && duration < ReadBitMaxDuration)
    {
      // handle read 0
      ReadBit( false );
      return;
    }

    if (duration < ReadBitSamplingTime)
    {
      // handle read 1
      ReadBit( true );
      return;
    }
}

void OWSlave::ReadBit( bool bit )
{
  bufferByte_ |= ((bit ? 1 : 0) << bufferBitPos_);
  ++bufferBitPos_;

  // got a whole byte
  if (bufferBitPos_ == 8)
  {
    bufferBitPos_ = 0;
    ++bufferPos_;
    if (bufferPos_==sizeof(buffer_))
    {
      bufferPos_=0;
    }
    buffer_[bufferPos_] = bufferByte_;

    switch (state_)
    {
      case State::Writing:
      case State::WaitingForReset:
        // throw away
        break;
      case State::WaitingForCommand:
        handleCommand(bufferByte_);
        break;
      case State::WaitingForData:
        handleDataByte(bufferByte_);
        break;
    }
  }
}

void OWSlave::handleCommand(byte command)
{
  switch (command)
  {
    case 0xF0: // SEARCH ROM
      beginSearchRom_();
      return;
    case 0xEC: // CONDITIONAL SEARCH ROM
      return;
    case 0x33: // READ ROM
      writeData(rom_, sizeof(rom_), false);
      return;
    case 0x55: // MATCH ROM
      // beginReceiveBytes_(scratchpad_, 8, &OneWireSlave::matchRomBytesReceived_);
      return;
    case 0xCC: // SKIP ROM
      // beginReceiveBytes_(scratchpad_, 1, &OneWireSlave::notifyClientByteReceived_);
      return;
    case 0xA5: // RESUME
      return;
    default:
      clientReceiveCallback_(ReceiveEvent::RE_Byte, command );
      return;
  }
}

void OWSlave::beginSearchRom_()
{
  // write the rom with complement
  writeData(rom_, sizeof(rom_), true);
}

void OWSlave::handleDataByte(byte data)
{
 
}

////////////////////////  Presence
void OWSlave::beginPrePresence_()
{
  endRead();    // a reset was received
  setTimerEvent(PresenceWaitDuration, true, &OWSlave::beginPresence_);

}

void OWSlave::beginPresence_()
{
  pullLow_();
  setTimerEvent(PresenceDuration, true, &OWSlave::endPresence_);
}

void OWSlave::endPresence_()
{
  releaseBus_();
  clearBuffer();
  state_ = State::WaitingForCommand;
  clientReceiveCallback_(ReceiveEvent::RE_Reset, 0 );
  beginRead();
}

//////////////////////////////////////// Sending

void OWSlave::writeByte(byte data)
{
  writeData(&data, 1, false);
}

void OWSlave::writeData(const byte* src, int len, bool inverse)
{
  // stop listening for incoming data
  endRead();
  clearBuffer();
  memcpy(buffer_, src, len);
  bufferLen_=sizeof(len);
  bufferByte_ = buffer_[bufferPos_];
  bufferSendWithInverse_ = inverse;
  bufferInverseToggle_ = false;
  state_=State::Writing;
  beginRead();
}

void OWSlave::clearBuffer()
{
  bufferByte_=0;
  bufferBitPos_ = 0;
  bufferPos_=0;
  bufferLen_=0;
  bufferSendWithInverse_ = false;
  bufferInverseToggle_ = false;
  memset( &OWSlave::buffer_, 0, sizeof(OWSlave::buffer_));
}

void OWSlave::AdvanceWriteBufferBy1Bit()
{  
  ++bufferBitPos_;
  if (bufferBitPos_==8)
  {
    bufferBitPos_=0;
    ++bufferPos_;
    if (bufferPos_==bufferLen_)
    {
      // writing has finished;
      endWrite();
    }
  }

}

void OWSlave::WriteNextBit()
{  
  bool currentBit = bitRead(bufferByte_, bufferBitPos_);
  bool bitToSend = currentBit;

  if (bufferSendWithInverse_)
  {
    if (bufferInverseToggle_)
    {
      bitToSend = !bitToSend; 
      AdvanceWriteBufferBy1Bit();
    }
    bufferInverseToggle_ = !bufferInverseToggle_;
  }
  else
  {    
    AdvanceWriteBufferBy1Bit();
  }
  
  if (bitToSend)
  {
    releaseBus_();
  }
  else
  {
    pullLow_(); // this must be executed first because the timing is very tight with some master devices  
    setTimerEvent(SendBitDuration, true, &OWSlave::endWriteBit);  
  }     
}

void OWSlave::endWriteBit()
{
  releaseBus_();
}

void OWSlave::endWrite()
{
  clearBuffer();
  state_= State::WaitingForCommand;
}

void OWSlave::pullLow_()
{
  pin_.writeLow();
  pin_.outputMode();
  pin_.writeLow();
}

void OWSlave::releaseBus_()
{  
  pin_.writeLow(); // make sure the internal pull-up resistor is disabled
  pin_.inputMode();
  pin_.writeLow(); // make sure the internal pull-up resistor is disabled
}

byte OWSlave::crc8(const byte* data, short numBytes)
{
  byte crc = 0;

  while (numBytes--) {
    byte inbyte = *data++;
    for (byte i = 8; i; i--) {
      byte mix = (crc ^ inbyte) & 0x01;
      crc >>= 1;
      if (mix) crc ^= 0x8C;
      inbyte >>= 1;
    }
  }
  return crc;
}

void OWSlave::setReceiveCallback(void(*callback)(ReceiveEvent evt, byte data)) {
  clientReceiveCallback_ = callback;
}

void OWSlave::setTimerEvent(short delayMicroSeconds, bool oneShot, void(*handler)())
{  
	_oneShot = oneShot;
  timerEvent = handler;
  UserTimer_Run(delayMicroSeconds);
}