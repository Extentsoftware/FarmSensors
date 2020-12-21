#include "OneWireSlave.h"

namespace
{
  const unsigned long ResetMinDuration = 480;      // min 480us  / 484us measured
  const unsigned long ResetMaxDuration = 640;      //
  const unsigned long PresenceWaitDuration = 0;   //  spec 15us to 60us  / 40us measured
  const unsigned long PresenceDuration = 150;      //   spec  60us to 240us  / 148us measured
  const unsigned long ReadBitSamplingTime = 15;    //   spec > 15us to 60us  / 31us measured
  const unsigned long SendBitDuration = 20;   // bus release time spec 15us / measured 19us

  const byte ReceiveCommand = (byte) - 1;

  void(*timerEvent)() = 0;
}

//OneWireSlave OWSlave;

Pin OneWireSlave::pin_;


ISR(USERTIMER_COMPA_vect) // timer1 interrupt
{
  UserTimer_Stop(); // disable clock
  void(*event)() = timerEvent;
  timerEvent = 0;
  event();
}

void OneWireSlave::begin(byte pinNumber)
{
  pin_ = Pin(pinNumber);

  cli(); // disable interrupts
  pin_.inputMode();
  pin_.writeLow(); // make sure the internal pull-up resistor is disabled

  // prepare hardware timer
  UserTimer_Init();

  // start 1-wire activity
  beginWaitReset_();
  sei(); // enable interrupts
}

void OneWireSlave::end()
{
  // log("Disabling 1-wire library");

  cli();
  disableTimer_();
  pin_.detachInterrupt();
  releaseBus_();
  sei();
}


void OneWireSlave::setTimerEvent_(short delayMicroSeconds, void(*handler)())
{
  delayMicroSeconds -= 8; // remove overhead (tuned on attiny85, values 0 - 10 work ok)

  short skipTicks = delayMicroSeconds / 2; // 16mhz clock, prescaler = 32
  if (skipTicks < 1) skipTicks = 1;
  timerEvent = handler;
  UserTimer_Run(skipTicks);
}

void OneWireSlave::disableTimer_()
{
  UserTimer_Stop();
}

void OneWireSlave::onEnterInterrupt_()
{
}

void OneWireSlave::onLeaveInterrupt_()
{
}

void OneWireSlave::pullLow_()
{
  pin_.outputMode();
  pin_.writeLow();
}

void OneWireSlave::releaseBus_()
{
  pin_.inputMode();
}

void OneWireSlave::beginWaitReset_()
{
  disableTimer_();
  pin_.inputMode();
  pin_.attachInterrupt( &OneWireSlave::waitResetTest_, FALLING );
}

void OneWireSlave::waitResetTest_()
{
  // bus is low
  onEnterInterrupt_();
  pin_.detachInterrupt();
  setTimerEvent_(50, &OneWireSlave::beginPresence_);
  onLeaveInterrupt_();
}

void OneWireSlave::beginPresence_()
{
  pullLow_();
  setTimerEvent_(PresenceDuration, &OneWireSlave::endPresence_);
}

void OneWireSlave::endPresence_()
{
  releaseBus_();
  beginWaitReset_();
}

