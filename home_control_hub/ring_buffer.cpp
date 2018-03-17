#include "ring_buffer.h"

/*
RingBuffer * ring_buffer_new( ) {
  RingBuffer * rb = NULL;

  rb = (RingBuffer*)malloc( sizeof( rb[ 0 ] ) );
  
  //rb->rb = (float*)malloc( RB_CAPACITY * sizeof( rb->rb[ 0 ] ) );
  for ( byte i = 0; i < RB_CAPACITY; i++ ) {
    rb->rb[ i ] = 0.0f;
  }

  rb->current = 0;
  rb->all_set = 0;
  //rb->capacity = RB_CAPACITY;

  return rb;
} // ring_buffer_new
*/


void ring_buffer_add( RingBuffer * self, float value ) {
  self->rb[ self->current ] = value;
  if ( self->current ==  RB_CAPACITY - 1 ) {
    self->current = 0;
    self->all_set = 1;
  }
  else {
    self->current++;
  }
} // ring_buffer_add


byte ring_buffer_last_index( const RingBuffer * self ) {
    byte last_idx = self->all_set ?  RB_CAPACITY : self->current;
    return last_idx;
} // last_index


float ring_buffer_average_value( RingBuffer * self, byte * computed ) {
    byte last_idx = ring_buffer_last_index( self );
    float sum = 0.0f;
    float average = 0.0f;

    if ( last_idx > 0 ) {
        for ( byte i = 0; i < last_idx; i++ ) {
            float temp = self->rb[ i ];
            sum += temp;
        }
    
        average = sum / last_idx;
        *computed = 1;
    }
    else {
        *computed = 0;
    }

    return average;
    
} // ring_buffer_average_value


void ring_buffer_print( const RingBuffer * self ) {
    byte last_idx = ring_buffer_last_index( self );
    Serial.print( "[ last_idx: " );
    Serial.print( last_idx );
    Serial.print( " ] " );

    for ( byte i = 0; i < last_idx; i++ ) {
        float temp = self->rb[ i ];
        Serial.print( temp );
        Serial.print( " " );
    }

    Serial.println();
} // ring_buffer_print

