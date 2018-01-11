int parse_int( char *string ) {
  int val = 0;

  /*
  while( 1 ) {
    while ( isspace( *string ) ) string++;
    if ( *string == 0 ) break;
    val = strtol( string, NULL, 10 );
    break;
  }
  */
  
  val = atoi( string );
  return val;
} // parse_int
