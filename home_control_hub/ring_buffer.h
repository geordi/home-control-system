#ifndef RING_BUFFER__
#define RING_BUFFER__

#include <stdlib.h>
#include <Arduino.h>

#define RB_CAPACITY 4

struct RingBuffer {
  float rb[ RB_CAPACITY ];
  unsigned char current;
  unsigned char all_set;
};

typedef struct RingBuffer RingBuffer;

RingBuffer * ring_buffer_new();

void ring_buffer_add( RingBuffer * self, float value );
float ring_buffer_average_value( RingBuffer * self, unsigned char * computed );

byte ring_buffer_last_index( const RingBuffer * self );
void ring_buffer_print( const RingBuffer * self );

#endif // RING_BUFFER__
