/*void handle_light_switch_datagram( char datagram[ 4 ] ) {
  // datagram = [header, repeats, socket_id, command]

#ifdef DEBUG
  int repeats = read_buffer[ LIGHT_SWITCH_DATAGRAM_REPEATS_IDX ];
  char socket_id = read_buffer[ LIGHT_SWITCH_DATAGRAM_SOCKET_ID_IDX ];
  int command = read_buffer[ LIGHT_SWITCH_DATAGRAM_COMMAND_IDX ];

  Serial.print( "Socket operation: " );
  Serial.print( "repeats= " );
  Serial.print( repeats );
  Serial.print( " socket_id= " );
  Serial.print( socket_id );
  Serial.print( " command= " );
  Serial.print( command );

  Serial.println();
#endif

  char socket_id = datagram[ LIGHT_SWITCH_DATAGRAM_SOCKET_ID_IDX ];
  char repeats = datagram[ LIGHT_SWITCH_DATAGRAM_REPEATS_IDX ];
  char switch_on = datagram[ LIGHT_SWITCH_DATAGRAM_COMMAND_IDX ];

  send_rf_socket_signal( socket_id, repeats, switch_on );

  Serial.print( "Switching: " );
  Serial.print( socket_id );
  Serial.print( " " );
  Serial.println( int(switch_on) );
}


char process_serial_header( const char header ) {
  if ( header & LIGHT_SWITCH_HEADER ) {
    return LIGHT_SWITCH_DATAGRAM;
  }
}


void process_incomming_serial_communication() {
  char read_buffer[ 4 ];
  Serial.readBytes( read_buffer, 4 );

  char header = read_buffer[ DATAGRAM_HEADER_IDX ];

  if ( process_serial_header( header ) == LIGHT_SWITCH_DATAGRAM ) {
    handle_light_switch_datagram( read_buffer );
  }
  else {
    Serial.print( "Got: " );
    Serial.println( read_buffer );
  }
}*/
