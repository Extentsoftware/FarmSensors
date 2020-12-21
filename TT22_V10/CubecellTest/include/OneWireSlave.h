#ifndef _OneWireSlave_h_
#define _OneWireSlave_h_

#include "Arduino.h"
#include "LowLevel.h"

class OneWireSlave
{
  public:

    //! Starts listening for the 1-wire master, on the specified pin, as a virtual slave device identified by the specified ROM (7 bytes, starting from the family code, CRC will be computed internally). Reset, Presence, SearchRom and MatchRom are handled automatically. The library will use the external interrupt on the specified pin (note that this is usually not possible with all pins, depending on the board), as well as one hardware timer. Blocking interrupts (either by disabling them explicitely with sei/cli, or by spending time in another interrupt) can lead to malfunction of the library, due to tight timing for some 1-wire operations.
    void begin(byte pinNumber);

    //! Stops all 1-wire activities, which frees hardware resources for other purposes.
    void end();

  private:
    static void setTimerEvent_(short delayMicroSeconds, void(*handler)());
    static void disableTimer_();

    static void onEnterInterrupt_();
    static void onLeaveInterrupt_();

    static void pullLow_();
    static void releaseBus_();

    static void beginResetDetection_();
    static void cancelResetDetection_();

    static void beginWaitReset_();
    
    // interrupt handlers
    static void waitResetTest_();
    static void waitReset_();
    static void beginPresence_();
    static void endPresence_();
    
  private:
    static Pin pin_;
};

extern OneWireSlave OWSlave;

#endif