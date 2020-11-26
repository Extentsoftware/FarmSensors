#ifndef _OWSlave_h_
#define _OWSlave_h_

#include "Arduino.h"
#include "pindirect.h"

class OWSlave
{
  public:
    enum ReceiveEvent
    {
      RE_Reset, //!< The master has sent a general reset
      RE_Byte, //!< The master just sent a byte of data
      RE_Error //!< A communication error happened (such as a timeout) ; the library will stop all 1-wire activities until the next reset
    };

    enum class State
    {
      WaitingForReset,
      WaitingForCommand,
      WaitingForData,
      Writing
    };

    //! Starts listening for the 1-wire master, on the specified pin, as a virtual slave device identified by the specified ROM (7 bytes, starting from the family code, CRC will be computed internally). Reset, Presence, SearchRom and MatchRom are handled automatically. The library will use the external interrupt on the specified pin (note that this is usually not possible with all pins, depending on the board), as well as one hardware timer. Blocking interrupts (either by disabling them explicitely with sei/cli, or by spending time in another interrupt) can lead to malfunction of the library, due to tight timing for some 1-wire operations.
    void begin(const byte* rom, Pin pin, Pin debug);

    //! Stops all 1-wire activities, which frees hardware resources for other purposes.
    void end();

    static void writeData(const byte* src, int len, bool inverse=false);

    static void writeByte(byte data);

    //! Sets (or replaces) a function to be called when something is received. The callback is executed from interrupts and should be as short as possible. Failure to return quickly can prevent the library from correctly reading the next byte.
    void setReceiveCallback(void(*callback)(ReceiveEvent evt, byte data));

    volatile static unsigned int debugValue;

  private:
    static byte rom_[8];
    static Pin pin_;
    static Pin debug_;
    static volatile unsigned long _readStartTime;
    static State state_;

    // buffer for reading/writing
    static byte bufferByte_;
    static byte bufferBitPos_;
    static short bufferPos_;
    static short bufferLen_;
    static short buffer_[8];
    static bool bufferSendWithInverse_; // special send mode for search rom
    static bool bufferInverseToggle_;   // 

    static void(*clientReceiveCallback_)(ReceiveEvent evt, byte data);

    static void pullLow_();
    static void releaseBus_();
    static void beginRead();
    static void endRead();

    // interrupt handlers
    static void ReadInterrupt();
    static void waitRead();
    static void clearBuffer();
    static void ReadPulse( unsigned long duration );
    static void ReadBit( bool bit );

    static void handleCommand(byte command);
    static void handleDataByte(byte data);

    static void WriteNextBit();
    static void endWriteBit();
    static void endWrite();
    static void AdvanceWriteBufferBy1Bit();

    static void beginPrePresence_();
    static void beginPresence_();
    static void endPresence_();
    static void beginSearchRom_();

    static byte crc8(const byte* data, short numBytes);
    static void setTimerEvent(short delayMicroSeconds, bool oneShot, void(*handler)());
};

extern OWSlave OneWireSlave;

#endif