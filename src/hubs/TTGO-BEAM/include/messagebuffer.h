#ifndef __HASHBUFFER__
#define __HASHBUFFER__

#include <Arduino.h>

class MessageBuffer {

public:

  MessageBuffer(int size)
  {    
    _size = size;
    _hashbuffer = (int *)calloc(size, sizeof(int));
    _msgbuffer = (uint8_t **)calloc(size, sizeof(uint8_t *));
  }
  
  int hashCode(uint8_t* buffer) {
      int h = 0;
      for (uint8_t* ptr = buffer; *ptr!='\0'; ++ptr)
          h = h * 31 + *ptr;
      return h;
  }

  bool exists(int hashcode)
  {    
    for (uint8_t i=0; i < _size; ++i )
        if (_hashbuffer[i] == hashcode)
          return true;
    return false;
  }

  // get newest to oldest
  uint8_t* getMsg(int index)
  {
    int offset = (_head - index) % _size;
    return _msgbuffer[offset];
  }

  int getSize()
  {
    return _size;
  }
  
  bool add(uint8_t* buffer)
  {
    int hashcode = hashCode(buffer);
    if (exists(hashcode))
      return false;
    // free existing entry 
    if (_msgbuffer[_head]!=NULL)
      free(_msgbuffer[_head]);
    _hashbuffer[_head]=hashcode;
    _msgbuffer[_head]=buffer;
    // advance
    _head = (_head + 1) % _size;
    return true;
  }

  private:
  
  int *_hashbuffer;
  uint8_t** _msgbuffer;
  unsigned short _head;
  unsigned short _tail;
  int _size;
};

#endif