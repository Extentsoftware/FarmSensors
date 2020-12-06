#include "OWSlave.h"

/* Note : The attiny85 clock speed = 16mhz (fuses L 0xF1, H 0xDF. E oxFF
          OSCCAL VALUE must also be calibrated to 16mhz

https://www.instructables.com/ATTiny-Port-Manipulation/
https://www.instructables.com/ATTiny-Port-Manipulation-Part-2-AnalogRead/
*/

namespace
{
  const unsigned int PresenceWaitDuration = 30;   // spec 15us to 60us  / 40us measured
  const unsigned int PresenceDuration = 150;      // spec  60us to 240us  / 148us measured
  const unsigned int ReadBitSamplingTime = 25;    // spec > 15us to 60us  / 31us measured
  const unsigned int ReadBitMaxDuration = 100;    // anything over this is an error
  const unsigned int SendBitDuration = 35;        // bus release time spec 15us / measured 19us
  const unsigned int ResetMinDuration = 400;      // min 480us  / 484us measured
  const unsigned int ResetMaxDuration = 640;      //

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
OWSlave::State OWSlave::state_;

volatile byte OWSlave::read_bufferByte_;
volatile byte OWSlave::read_bufferBitPos_;
volatile byte OWSlave::read_bufferPos_;
byte OWSlave::read_buffer_[32];

byte OWSlave::write_bufferLen_;
byte OWSlave::write_bufferPos_;
byte OWSlave::write_bufferBitPos_;
byte* OWSlave::write_ptr_;
byte OWSlave::write_byte;
bool OWSlave::write_lastBitSent;

volatile unsigned long OWSlave::debugValue;
volatile unsigned long OWSlave::_readStartTime;

void(*OWSlave::clientReceiveCallback_)(OWSlave::ReceiveEvent evt, byte data);

__attribute__((always_inline)) static inline void Pin2ModeOuput()
{
  DDRB |= (1 << DDB2);
}

__attribute__((always_inline)) static inline void Pin2ModeInput()
{
  // no pullup
  DDRB &= ~(1 << DDB2);
  PORTB &= ~(1 << PORTB2);  //activate pull-up resistor for PB3
}

__attribute__((always_inline)) static inline void Pin2ModeInputPullup()
{
  DDRB &= ~(1 << DDB2);
  PORTB |= (1 << PORTB2);  //activate pull-up resistor for PB3
}

__attribute__((always_inline)) static inline void Pin2Set1()
{
  PORTB |= (1 << PORTB2);
}

__attribute__((always_inline)) static inline void Pin2Set0()
{
  PORTB &= ~(1 << PORTB2);
}

__attribute__((always_inline)) static inline bool Pin2Read()
{
  return PINB & ~(1 << PINB2);
}

inline void Pin2attachInterrupt(void (*handler)(), int mode)
{
  GIFR |= (1 << INTF0) | (1<<PCIF);
  ::attachInterrupt(0, handler, mode);
}

inline void Pin2detachInterrupt() {
  ::detachInterrupt(0);

}

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
  OCR1A = skipTicks / 2;      //set the compare value
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

void OWSlave::begin(byte* rom, Pin pin)
{
  pin_ = pin;

  makeRom(rom);

  state_ = State::WaitingForReset;

  // prepare hardware timer
  UserTimer_Init();

  clearBuffer();

  debugValue = 1;

  // start 1-wire activity
  beginRead();
}

void OWSlave::makeRom(byte *rom)
{
  memcpy(rom_, rom, 7);
  rom_[7] = crc8(rom, 7);
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

  bool pinHigh = Pin2Read();
  unsigned long now = micros();
  
  // start timing
  if (!pinHigh)
  {
    _readStartTime = now;
    sei();
    return;
  }

  unsigned long duration = (now - _readStartTime);

  if (duration==0)
  {
    sei();
    return;
  }


  if (duration > ResetMinDuration )
  {    
    beginPrePresence_();
    sei();
    return;
  }
  
  if (duration > ReadBitMaxDuration)
  {
    sei();
    return;
  }

  bool bit = duration < ReadBitSamplingTime && duration!=0;

  // getting bits ok
  //if (bit)
  //  debugValue = duration;

  ReadBit( bit );
  sei();
}

void OWSlave::ReadBit( bool bit )
{
  switch (state_)
  {    
    case State::WritingRom:
      if (bit)
        WriteNextRomBit();
      break;
    case State::Writing:
      if (bit)
        WriteNextBit();
      break;
    case State::WritingRomCompliment:
      WriteComplement();
      break;
    case State::WritingRomChecking:
      ReadPulseWriting( bit );
      break;
    case State::WaitingForReset:
      break;
    case State::WaitingForCommand:      
      debugValue = ++read_bufferBitPos_;
      // if (AddBitToReadBuffer( bit ))
      // {        
      //   debugValue = 30;
      //   handleCommand(read_bufferByte_);
      // }
      break;
    case State::WaitingForData:
      if (AddBitToReadBuffer(  bit ))
      {
        AdvanceReadBuffer();
        handleDataByte(read_bufferByte_);
      }
      break;
  }
}

void OWSlave::handleCommand(byte command)
{
  switch (command)
  {
    case 0xF0: // SEARCH ROM      
      //debugValue = 40;
      beginSearchRom_();
      return;
    case 0xEC: // CONDITIONAL SEARCH ROM    
      return;
    case 0x33: // READ ROM
      writeData(rom_, 8);
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

void OWSlave::ReadPulseWriting( bool bit )
{
  // we just sent the complement (write_inverse = true)
  // master says we sent a good bit
  if (bit != write_lastBitSent )
  {
    state_ = State::WaitingForReset;
  }
  else
  {
    state_ = State::WritingRom;
    AdvanceWriteBuffer1Bit();
  }  
}

void OWSlave::clearBuffer()
{
  read_bufferByte_=0;
  read_bufferBitPos_ = 0;
  read_bufferPos_=0;
  memset( &OWSlave::read_buffer_, 0, sizeof(OWSlave::read_buffer_));
}

bool OWSlave::AddBitToReadBuffer( bool bit )
{
  read_bufferByte_ |= ((bit ? 1 : 0) << read_bufferBitPos_);
  ++read_bufferBitPos_;

  // got a whole byte?
  return (read_bufferBitPos_ >= 8);
}

void OWSlave::AdvanceReadBuffer()
{
  read_bufferBitPos_ = 0;
  read_buffer_[read_bufferPos_] = read_bufferByte_;
  ++read_bufferPos_;
  if (read_bufferPos_==sizeof(read_buffer_))
  {
    read_bufferPos_=0;
  }
  read_buffer_[read_bufferPos_] = 0;
}


void OWSlave::handleDataByte(byte data)
{
  clientReceiveCallback_(ReceiveEvent::RE_Byte, data );
}

////////////////////////  Presence
void OWSlave::beginPrePresence_()
{
  setTimerEvent(PresenceWaitDuration, true, &OWSlave::beginPresence_);
}

void OWSlave::beginPresence_()
{
  pullLow_();
  setTimerEvent(PresenceDuration, true, &OWSlave::endPresence_);
}

void OWSlave::endPresence_()
{
  clearBuffer();
  releaseBus_();
  clientReceiveCallback_(ReceiveEvent::RE_Reset, 0 );
  state_ = State::WaitingForCommand;
}

//////////////////////////////////////// Sending

void OWSlave::beginSearchRom_()
{
  // stop listening for incoming data
  //endRead();

  state_=State::WritingRom;
  write_lastBitSent=false;
  write_bufferPos_=0;
  write_bufferBitPos_=0;
  write_bufferLen_=8;
  write_ptr_ = rom_;
  write_byte = rom_[0];
  
  // beginRead();
}

////////////////////////////////////////////////////
void OWSlave::writeByte(byte data)
{
  writeData(&data, 1);
}

void OWSlave::writeData(byte* src, int len)
{
  // stop listening for incoming data
  //endRead();
  write_lastBitSent=false;
  state_=State::Writing;
  write_bufferPos_=0;
  write_bufferBitPos_=0;
  write_bufferLen_=len;
  write_ptr_ = src;
  write_byte = src[0];
  //beginRead();
}

bool OWSlave::AdvanceWriteBuffer1Bit()
{  
  ++write_bufferBitPos_;
  if (write_bufferBitPos_==8)
  {
    write_bufferBitPos_=0;
    ++write_bufferPos_;
    ++write_ptr_;
    write_byte = *write_ptr_;
    if (write_bufferPos_==write_bufferLen_)
    {
      // writing has finished;
      endWrite();
    }
    return true;
  }
  return false;
}

void OWSlave::WriteNextRomBit()
{ 
  write_lastBitSent = bitRead(write_byte, write_bufferBitPos_);

  if (write_lastBitSent)
  {
    releaseBus_();
  }
  else
  {
    pullLow_(); // this must be executed first because the timing is very tight with some master devices  
    setTimerEvent(SendBitDuration, true, &OWSlave::endWriteBit);  
  }

  state_ = State::WritingRomCompliment;
}

void OWSlave::WriteComplement()
{ 
  if (!write_lastBitSent)
  {
    releaseBus_();
  }
  else
  {
    pullLow_(); // this must be executed first because the timing is very tight with some master devices  
    setTimerEvent(SendBitDuration, true, &OWSlave::endWriteBit);  
  }     
  state_ = State::WritingRomChecking;
}

void OWSlave::WriteNextBit()
{ 
  write_lastBitSent = bitRead(write_byte, write_bufferBitPos_);

  if (write_lastBitSent)
  {
    releaseBus_();
  }
  else
  {
    pullLow_(); // this must be executed first because the timing is very tight with some master devices  
    setTimerEvent(SendBitDuration, true, &OWSlave::endWriteBit);  
  }

  AdvanceWriteBuffer1Bit();
}

void OWSlave::endWriteBit()
{
  releaseBus_();  
}

void OWSlave::endWrite()
{
  state_= State::WaitingForCommand;
}

void OWSlave::pullLow_()
{

  // pin_.outputMode();
  // pin_.writeLow();
  Pin2ModeOuput();
  Pin2Set0();
}

void OWSlave::releaseBus_()
{ 
  // pin_.inputMode(); 
  Pin2ModeInput();
  Pin2Set0();
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