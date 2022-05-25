#include <wd.h>

Watchdog::Watchdog()
{

}

void Watchdog::clrWatchdog()
{
  timerWrite(timer, 0); //reset timer (feed watchdog)
}

void Watchdog::setupWatchdog(int timeout, void (*fn)(void), int channel)
{
  timer = timerBegin(channel, 80, true); //timer 0, div 80
  timerAttachInterrupt(timer, fn, true); //attach callback
  timerAlarmWrite(timer, timeout * 1000, false); //set time in us
  timerAlarmEnable(timer); //enable interrupt
}

