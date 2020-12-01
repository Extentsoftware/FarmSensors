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
      Writing,
      WritingRom,
      WritingRomCompliment,
      WritingRomChecking
    };

    //! Starts listening for the 1-wire master, on the specified pin, as a virtual slave device identified by the specified ROM (7 bytes, starting from the family code, CRC will be computed internally). Reset, Presence, SearchRom and MatchRom are handled automatically. The library will use the external interrupt on the specified pin (note that this is usually not possible with all pins, depending on the board), as well as one hardware timer. Blocking interrupts (either by disabling them explicitely with sei/cli, or by spending time in another interrupt) can lead to malfunction of the library, due to tight timing for some 1-wire operations.
    void begin( byte* rom, Pin pin);

    //! Stops all 1-wire activities, which frees hardware resources for other purposes.
    void end();

    static void writeData(byte* src, int len);

    static void writeByte(byte data);

    //! Sets (or replaces) a function to be called when something is received. The callback is executed from interrupts and should be as short as possible. Failure to return quickly can prevent the library from correctly reading the next byte.
    void setReceiveCallback(void(*callback)(ReceiveEvent evt, byte data));

    volatile static unsigned int debugValue;

  private:
    static byte rom_[8];
    static Pin pin_;
    static volatile unsigned long _readStartTime;
    static State state_;

    // buffer for reading/writing
    static byte read_bufferByte_;
    static byte read_bufferBitPos_;
    static byte read_bufferPos_;
    static byte read_buffer_[32];

    volatile static byte write_bufferLen_;
    volatile static byte write_bufferPos_;
    volatile static byte write_bufferBitPos_;
    volatile static byte* write_ptr_;
    volatile static bool write_lastBitSent;

    static void(*clientReceiveCallback_)(ReceiveEvent evt, byte data);

    static void makeRom(byte *rom);
    static void pullLow_();
    static void releaseBus_();
    static void beginRead();
    static void endRead();

    // interrupt handlers
    static void ReadInterrupt();
    static void waitRead();
    static void clearBuffer();
    static void ReadPulseWriting( bool bit );
    static void ReadBit( bool bit );
    static bool AddBitToReadBuffer( bool bit );
    static void AdvanceReadBuffer();

    static void handleCommand(byte command);
    static void handleDataByte(byte data);

    static void WriteNextBit();
    static void WriteComplement();
    static void endWriteBit();
    static void endWrite();
    static bool AdvanceWriteBuffer1Bit();

    static void beginPrePresence_();
    static void beginPresence_();
    static void endPresence_();
    static void beginSearchRom_();

    static byte crc8(const byte* data, short numBytes);
    static void setTimerEvent(short delayMicroSeconds, bool oneShot, void(*handler)());
};

extern OWSlave OneWireSlave;

#endif