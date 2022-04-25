#ifndef __RINGBUFFER__
#define __RINGBUFFER__

#include <Arduino.h>

class RingBuffer {

public:

  RingBuffer(uint8_t size)
  {
    _buffer = (uint8_t *)malloc(size);
    memset(_buffer,0,size);
    _size = size;
  }

  ~RingBuffer()
  {
    free(_buffer);
  }

  bool exists(uint8_t item)
  {
    uint8_t i=0;
    for (uint8_t *ptr = _buffer; i < _size; ++i, ++ptr )
    {
        if (*ptr == item)
          return true;
    }
    return false;
  }

  void add(uint8_t item)
  {
    _buffer[_head]=item;

    ++_head;
    if (_head>_size)
    {
      _head=0;
    }
  }
  
  private:
  
  uint8_t* _buffer;
  unsigned short _size;
  unsigned short _head;
  unsigned short _tail;
};

#endif