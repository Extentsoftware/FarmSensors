#ifndef __WAATCHDOG__
#define __WAATCHDOG__

#include "Arduino.h"

class Watchdog
{
  public:
    Watchdog();
    void clrWatchdog();
    void setupWatchdog(int timeout, void (*fn)(void), int channel=0);

  private:
    hw_timer_t *timer;
};

#endif